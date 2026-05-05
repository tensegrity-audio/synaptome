#include "ofApp.h"
#include "io/ConsoleStore.h"
#include "ui/overlays/ControlsHudWidget.h"
#include "ui/overlays/StatusHudWidget.h"
#include "ui/overlays/LayersHudWidget.h"
#include "ui/overlays/SensorsHudWidget.h"
#include "ui/overlays/MenuMirrorHudWidget.h"
#include "ui/overlays/TelemetryWidget.h"
#include "ui/overlays/KeyListWidget.h"
#include "ui/ControlMappingHubState.h"
#include "ui/MenuSkin.h"
#include "ui/ControlHubEventBridge_clean.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "ofAppGLFWWindow.h"
#include "GLFW/glfw3.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <unordered_set>

namespace {
    constexpr const char* kSceneAutosavePath = "config/scene-last.json";
    constexpr const char* kDefaultScenePath = "layers/scenes/default.json";
    constexpr int kSceneSchemaVersion = 2;
    constexpr const char* kSceneMappingsKey = "mappings";
    constexpr const char* kSceneRouterMappingsKey = "router";
    constexpr const char* kSceneSlotAssignmentsKey = "slotAssignments";
    constexpr const char* kRunLogPath = "log.txt";
    constexpr const char* kRunLogArchiveDir = "logs/runs";
    constexpr std::size_t kRetainedRunLogs = 3;

    constexpr uint64_t kHudFreshMs = 1000;
    constexpr uint64_t kHudStaleMs = 5000;
    bool startsWith(const std::string& text, const std::string& prefix) {
        return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
    }

    bool endsWith(const std::string& text, const std::string& suffix) {
        return text.size() >= suffix.size()
            && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    void pruneArchivedRunLogs() {
        ofDirectory dir(ofToDataPath(kRunLogArchiveDir, true));
        if (!dir.exists()) {
            return;
        }
        dir.allowExt("txt");
        dir.listDir();

        std::vector<ofFile> archives;
        archives.reserve(dir.size());
        for (std::size_t i = 0; i < dir.size(); ++i) {
            ofFile file = dir.getFile(i);
            const std::string filename = file.getFileName();
            if (file.isFile() && startsWith(filename, "log-") && endsWith(filename, ".txt")) {
                archives.push_back(std::move(file));
            }
        }
        std::sort(archives.begin(), archives.end(), [](const ofFile& a, const ofFile& b) {
            return a.getFileName() < b.getFileName();
        });

        while (archives.size() > kRetainedRunLogs) {
            ofFile::removeFile(archives.front().getAbsolutePath(), false);
            archives.erase(archives.begin());
        }
    }

    void rotateRunLog() {
        const std::string activeLogPath = ofToDataPath(kRunLogPath, true);
        if (ofFile::doesFileExist(activeLogPath, false)) {
            ofDirectory::createDirectory(ofToDataPath(kRunLogArchiveDir, true), false, true);
            std::string archivePath = ofToDataPath(
                std::string(kRunLogArchiveDir) + "/log-" + ofGetTimestampString() + ".txt",
                true);
            int suffix = 1;
            while (ofFile::doesFileExist(archivePath, false)) {
                archivePath = ofToDataPath(
                    std::string(kRunLogArchiveDir) + "/log-" + ofGetTimestampString() + "-" + ofToString(suffix) + ".txt",
                    true);
                ++suffix;
            }
            ofFile::moveFromTo(activeLogPath, archivePath, false, true);
        }
        pruneArchivedRunLogs();
    }

    std::string controlHubSlotAssignmentsPath() {
        return ofToDataPath("config/ui/slot_assignments.json", true);
    }

    ofJson loadJsonSnapshotIfExists(const std::string& path) {
        if (path.empty() || !ofFile::doesFileExist(path)) {
            return ofJson();
        }
        try {
            return ofLoadJson(path);
        } catch (const std::exception& ex) {
            ofLogWarning("Scene") << "failed to parse snapshot " << path << " : " << ex.what();
        }
        return ofJson();
    }

    bool writeJsonSnapshotAtomically(const std::string& path, const ofJson& snapshot) {
        if (path.empty()) {
            return false;
        }
        const auto directory = ofFilePath::getEnclosingDirectory(path, false);
        if (!directory.empty()) {
            ofDirectory::createDirectory(directory, true, true);
        }
        const std::string tmpPath = path + ".tmp";
        try {
            if (!ofSavePrettyJson(tmpPath, snapshot)) {
                ofLogWarning("Scene") << "failed to write snapshot temp file " << tmpPath;
                return false;
            }
            if (!ofFile::moveFromTo(tmpPath, path, true, true)) {
                ofLogWarning("Scene") << "failed to commit snapshot file " << path;
                ofFile::removeFile(tmpPath, false);
                return false;
            }
            return true;
        } catch (const std::exception& ex) {
            ofLogWarning("Scene") << "failed to save snapshot " << path << " : " << ex.what();
        }
        ofFile::removeFile(tmpPath, false);
        return false;
    }

    ofJson emptySlotAssignmentsSnapshot() {
        ofJson doc = ofJson::object();
        doc["assignments"] = ofJson::array();
        return doc;
    }

    std::string normalizeScenePath(std::string path) {
        std::replace(path.begin(), path.end(), '\\', '/');
        while (!path.empty() && path.front() == '/') {
            path.erase(path.begin());
        }
        return ofFilePath::removeTrailingSlash(path);
    }

    bool isAbsolutePath(const std::string& path) {
        if (path.size() > 1 && path[1] == ':') {
            return true;
        }
        return !path.empty() && (path.front() == '/' || path.front() == '\\');
    }

    std::string sceneFilesystemPath(const std::string& path) {
        return isAbsolutePath(path) ? path : ofToDataPath(path, true);
    }

    bool isHrMetric(const std::string& metric) {
        return startsWith(metric, "heart-") || startsWith(metric, "hr-");
    }

    bool isImuMetric(const std::string& metric) {
        return startsWith(metric, "imu-");
    }

    bool isMicMetric(const std::string& metric) {
        return startsWith(metric, "mic-");
    }

    bool isBioMetric(const std::string& metric) {
        return startsWith(metric, "bioamp-");
    }

    std::optional<int> consoleLayerIndexFromParam(const std::string& paramId) {
        static const std::string kConsolePrefix = "console.layer";
        if (paramId.rfind(kConsolePrefix, 0) != 0) {
            return std::nullopt;
        }
        std::size_t pos = kConsolePrefix.size();
        std::string digits;
        while (pos < paramId.size() && std::isdigit(static_cast<unsigned char>(paramId[pos]))) {
            digits.push_back(paramId[pos]);
            ++pos;
        }
        if (digits.empty()) {
            return std::nullopt;
        }
        int index = ofToInt(digits);
        if (index <= 0) {
            return std::nullopt;
        }
        return index;
    }

    bool isFxType(const std::string& type) {
        return startsWith(type, "fx.");
    }

    bool isUiOverlayType(const std::string& type) {
        return startsWith(type, "ui.");
    }


    ofColor parseHexColor(const std::string& hex, const ofColor& fallback = ofColor::black) {
        std::string value = ofTrim(hex);
        if (value.empty()) {
            return fallback;
        }
        if (!value.empty() && value[0] == '#') {
            value.erase(0, 1);
        }
        if (value.size() > 2 && (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0)) {
            value.erase(0, 2);
        }
        if (!(value.size() == 6 || value.size() == 8)) {
            return fallback;
        }
        unsigned int encoded = 0;
        std::stringstream ss;
        ss << std::hex << value;
        if (!(ss >> encoded)) {
            return fallback;
        }
        ofColor color = fallback;
        color.r = static_cast<unsigned char>((encoded >> 16) & 0xFF);
        color.g = static_cast<unsigned char>((encoded >> 8) & 0xFF);
        color.b = static_cast<unsigned char>(encoded & 0xFF);
        color.a = (value.size() == 8) ? static_cast<unsigned char>((encoded >> 24) & 0xFF) : 255;
        return color;
    }

    struct MonitorSelection {
        GLFWmonitor* handle = nullptr;
        std::string label;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool matchedHint = false;
    };

    std::string toLowerCopy(std::string value);

    MonitorSelection selectMonitor(const std::string& hint) {
        MonitorSelection selection;
        int monitorCount = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
        if (!monitors || monitorCount == 0) {
            return selection;
        }
        auto assign = [&](GLFWmonitor* monitor, bool matchedHint = false) {
            if (!monitor) {
                return;
            }
            selection.handle = monitor;
            selection.matchedHint = matchedHint;
            const char* name = glfwGetMonitorName(monitor);
            selection.label = name ? name : "Monitor";
            glfwGetMonitorPos(monitor, &selection.x, &selection.y);
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (mode) {
                selection.width = mode->width;
                selection.height = mode->height;
            }
        };
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        std::string trimmed = ofTrim(hint);
        std::string lowered = ofToLower(trimmed);
        auto tryMatchByName = [&](const std::string& needle) -> bool {
            if (needle.empty()) {
                return false;
            }
            for (int i = 0; i < monitorCount; ++i) {
                const char* monitorName = glfwGetMonitorName(monitors[i]);
                if (!monitorName) {
                    continue;
                }
                std::string loweredName = ofToLower(std::string(monitorName));
                if (loweredName.find(needle) != std::string::npos) {
                    assign(monitors[i], true);
                    return true;
                }
            }
            return false;
        };
        if (!trimmed.empty()) {
            bool matched = false;
            if (std::all_of(trimmed.begin(), trimmed.end(), ::isdigit)) {
                int idx = ofClamp(ofToInt(trimmed) - 1, 0, monitorCount - 1);
                assign(monitors[idx], true);
                matched = selection.handle != nullptr;
            } else if (lowered == "primary") {
                assign(primary ? primary : monitors[0], true);
                matched = selection.handle != nullptr;
            } else if (lowered == "secondary" && monitorCount > 1) {
                for (int i = 0; i < monitorCount; ++i) {
                    if (monitors[i] != primary) {
                        assign(monitors[i], true);
                        matched = true;
                        break;
                    }
                }
            } else {
                matched = tryMatchByName(lowered);
            }
            if (matched) {
                return selection;
            }
        }
        if (monitorCount > 1 && primary) {
            for (int i = 0; i < monitorCount; ++i) {
                if (monitors[i] != primary) {
                    assign(monitors[i]);
                    if (selection.handle) {
                        return selection;
                    }
                }
            }
        }
        assign(primary ? primary : monitors[0]);
        return selection;
    }

    ofJson defaultAudioConfig() {
        ofJson cfg = ofJson::object();
        ofJson mic = ofJson::object();
        mic["enabled"] = true;
        mic["deviceIndex"] = -1;
        mic["deviceNameContains"] = "";
        mic["sampleRate"] = 48000;
        mic["bufferSize"] = 256;
        mic["channels"] = 1;

        ofJson ingest = ofJson::object();
        ingest["enabled"] = true;
        ingest["addressPrefix"] = "/sensor/host/localmic";
        ingest["rateLimitHz"] = 60.0f;
        ingest["deadband"] = 0.01f;
        mic["ingest"] = ingest;

        ofJson osc = ofJson::object();
        osc["enabled"] = false;
        osc["host"] = "127.0.0.1";
        osc["port"] = 9001;
        mic["osc"] = osc;

        ofJson modifier = ofJson::object();
        modifier["enabled"] = true;
        modifier["target"] = "fx.master";
        modifier["blend"] = "scale";
        ofJson inputRange = ofJson::object();
        inputRange["min"] = 0.02f;
        inputRange["max"] = 0.4f;
        modifier["inputRange"] = inputRange;
        ofJson outputRange = ofJson::object();
        outputRange["min"] = 0.0f;
        outputRange["max"] = 1.0f;
        outputRange["relativeToBase"] = false;
        modifier["outputRange"] = outputRange;
        mic["modifier"] = modifier;

        cfg["localMic"] = mic;
        return cfg;
    }

    std::string normalizeOscPrefix(const std::string& raw) {
        std::string prefix = ofTrim(raw);
        if (prefix.empty()) {
            prefix = "/sensor/host/localmic";
        }
        if (prefix.front() != '/') {
            prefix.insert(prefix.begin(), '/');
        }
        while (prefix.size() > 1 && prefix.back() == '/') {
            prefix.pop_back();
        }
        return prefix;
    }

    int resolveInputDeviceIndex(int requestedIndex,
                                const std::string& nameContains,
                                const std::vector<std::pair<int, std::string>>& devices) {
        auto matches = [&](const std::string& haystack, const std::string& needle) {
            if (needle.empty()) return false;
            std::string loweredHaystack = toLowerCopy(haystack);
            return loweredHaystack.find(toLowerCopy(needle)) != std::string::npos;
        };
        if (requestedIndex >= 0) {
            for (const auto& entry : devices) {
                if (entry.first == requestedIndex) {
                    return entry.first;
                }
            }
        }
        if (!nameContains.empty()) {
            for (const auto& entry : devices) {
                if (matches(entry.second, nameContains)) {
                    return entry.first;
                }
            }
        }
        if (!devices.empty()) {
            return devices.front().first;
        }
        return -1;
    }

#ifdef _WIN32
    bool printableKeyPhysicallyDown(int base) {
        int vk = base;
        if (base >= 'a' && base <= 'z') {
            vk = 'A' + (base - 'a');
        }
        SHORT state = GetAsyncKeyState(vk);
        return (state & 0x8000) != 0;
    }
#else
    bool printableKeyPhysicallyDown(int) {
        return true;
    }
#endif

    std::string modifierTypeToString(modifier::Type type) {
        switch (type) {
        case modifier::Type::kKey: return "Key";
        case modifier::Type::kMidiCc: return "MidiCc";
        case modifier::Type::kMidiNote: return "MidiNote";
        case modifier::Type::kOsc: return "Osc";
        case modifier::Type::kAutomation: return "Automation";
        case modifier::Type::kScript: return "Script";
        }
        return "Key";
    }

    std::string modifierBlendToString(modifier::BlendMode mode) {
        switch (mode) {
        case modifier::BlendMode::kAdditive: return "Additive";
        case modifier::BlendMode::kAbsolute: return "Absolute";
        case modifier::BlendMode::kScale: return "Scale";
        case modifier::BlendMode::kClamp: return "Clamp";
        case modifier::BlendMode::kToggle: return "Toggle";
        }
        return "Additive";
    }

    std::string toLowerCopy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    std::string normalizeDualDisplayMode(const std::string& value) {
        std::string lower = toLowerCopy(value);
        std::string compact;
        compact.reserve(lower.size());
        for (char c : lower) {
            if (c == '-' || c == '_' || c == ' ') {
                continue;
            }
            compact.push_back(c);
        }
        if (compact == "dual" || compact == "dualscreen" || compact == "dualdisplay") {
            return "dual";
        }
        return "single";
    }

    float clampMenuTextScale(float value) {
        return ofClamp(value, 0.75f, 2.5f);
    }

    class ConfiguredOverlayWidget : public OverlayWidget {
    public:
        ConfiguredOverlayWidget(std::unique_ptr<OverlayWidget> inner, OverlayWidget::Metadata metadata)
            : inner_(std::move(inner))
            , metadata_(std::move(metadata)) {}

        const Metadata& metadata() const override { return metadata_; }
        void setup(const SetupParams& params) override {
            if (inner_) {
                inner_->setup(params);
            }
        }
        void update(const UpdateParams& params) override {
            if (inner_) {
                inner_->update(params);
            }
        }
        void draw(const DrawParams& params) override {
            if (inner_) {
                inner_->draw(params);
            }
        }
        float preferredHeight(float width) const override {
            if (!inner_) {
                return metadata_.defaultHeight;
            }
            return inner_->preferredHeight(width);
        }
        bool isVisible() const override { return inner_ ? inner_->isVisible() : true; }

    private:
        std::unique_ptr<OverlayWidget> inner_;
        OverlayWidget::Metadata metadata_;
    };

    std::unique_ptr<OverlayWidget> createHudWidgetForModule(const std::string& module) {
        const std::string lowered = toLowerCopy(module);
        if (lowered == "controls") {
            return std::make_unique<ControlsHudWidget>();
        }
        if (lowered == "status") {
            return std::make_unique<StatusHudWidget>();
        }
        if (lowered == "layers") {
            return std::make_unique<LayersHudWidget>();
        }
        if (lowered == "sensors") {
            return std::make_unique<SensorsHudWidget>();
        }
        if (lowered == "menu") {
            return std::make_unique<MenuMirrorHudWidget>();
        }
        if (lowered == "telemetry") {
            return std::make_unique<TelemetryWidget>();
        }
        if (lowered == "keylist" || lowered == "keys") {
            return std::make_unique<KeyListWidget>();
        }
        return nullptr;
    }

    modifier::Type modifierTypeFromJson(const ofJson& node) {
        modifier::Type type = modifier::Type::kKey;
        if (node.contains("type")) {
            const auto& raw = node["type"];
            if (raw.is_string()) {
                std::string str = toLowerCopy(raw.get<std::string>());
                if (str == "key") type = modifier::Type::kKey;
                else if (str == "midicc" || str == "midi_cc") type = modifier::Type::kMidiCc;
                else if (str == "midinote" || str == "midi_note") type = modifier::Type::kMidiNote;
                else if (str == "osc") type = modifier::Type::kOsc;
                else if (str == "automation") type = modifier::Type::kAutomation;
                else if (str == "script") type = modifier::Type::kScript;
            } else if (raw.is_number_integer()) {
                int rawType = raw.get<int>();
                if (rawType >= static_cast<int>(modifier::Type::kKey) && rawType <= static_cast<int>(modifier::Type::kScript)) {
                    type = static_cast<modifier::Type>(rawType);
                }
            }
        }
        return type;
    }

    modifier::BlendMode modifierBlendFromJson(const ofJson& node) {
        modifier::BlendMode mode = modifier::BlendMode::kAdditive;
        if (node.contains("blend")) {
            const auto& raw = node["blend"];
            if (raw.is_string()) {
                std::string str = toLowerCopy(raw.get<std::string>());
                if (str == "additive") mode = modifier::BlendMode::kAdditive;
                else if (str == "absolute") mode = modifier::BlendMode::kAbsolute;
                else if (str == "scale") mode = modifier::BlendMode::kScale;
                else if (str == "clamp") mode = modifier::BlendMode::kClamp;
                else if (str == "toggle") mode = modifier::BlendMode::kToggle;
            } else if (raw.is_number_integer()) {
                int rawBlend = raw.get<int>();
                if (rawBlend >= static_cast<int>(modifier::BlendMode::kAdditive) && rawBlend <= static_cast<int>(modifier::BlendMode::kToggle)) {
                    mode = static_cast<modifier::BlendMode>(rawBlend);
                }
            }
        }
        return mode;
    }

    ofJson rangeToJson(const modifier::Range& range) {
        ofJson node;
        node["min"] = range.min;
        node["max"] = range.max;
        node["relative"] = range.relativeToBase;
        return node;
    }

    modifier::Range rangeFromJson(const ofJson& node, const modifier::Range& defaults = {}) {
        modifier::Range range = defaults;
        if (!node.is_object()) {
            return range;
        }
        if (node.contains("min") && node["min"].is_number()) {
            range.min = node["min"].get<float>();
        }
        if (node.contains("max") && node["max"].is_number()) {
            range.max = node["max"].get<float>();
        }
        if (node.contains("relativeToBase") && node["relativeToBase"].is_boolean()) {
            range.relativeToBase = node["relativeToBase"].get<bool>();
        } else if (node.contains("relative") && node["relative"].is_boolean()) {
            range.relativeToBase = node["relative"].get<bool>();
        }
        return range;
    }

    ofJson serializeRuntimeModifier(const ParameterRegistry::RuntimeModifier& runtime) {
        ofJson node;
        node["type"] = modifierTypeToString(runtime.descriptor.type);
        node["blend"] = modifierBlendToString(runtime.descriptor.blend);
        node["enabled"] = runtime.descriptor.enabled;
        node["invert"] = runtime.descriptor.invertInput;
        node["inputRange"] = rangeToJson(runtime.descriptor.inputRange);
        node["outputRange"] = rangeToJson(runtime.descriptor.outputRange);
        node["inputValue"] = runtime.inputValue;
        node["active"] = runtime.active;
        return node;
    }

    ofJson serializeRuntimeModifiers(const std::vector<ParameterRegistry::RuntimeModifier>& modifiers) {
        ofJson array = ofJson::array();
        for (const auto& runtime : modifiers) {
            if (runtime.descriptor.type == modifier::Type::kOsc) {
                continue;
            }
            array.push_back(serializeRuntimeModifier(runtime));
        }
        return array;
    }

    modifier::Modifier modifierFromJson(const ofJson& node) {
        modifier::Modifier mod;
        mod.type = modifierTypeFromJson(node);
        mod.blend = modifierBlendFromJson(node);
        mod.enabled = node.value("enabled", true);
        mod.invertInput = node.value("invert", false);
        if (node.contains("inputRange")) {
            mod.inputRange = rangeFromJson(node["inputRange"], mod.inputRange);
        }
        if (node.contains("outputRange")) {
            mod.outputRange = rangeFromJson(node["outputRange"], mod.outputRange);
        }
        return mod;
    }

    void loadFloatModifiersFromJson(ParameterRegistry& registry, const std::string& id, const ofJson& array) {
        if (!array.is_array()) {
            return;
        }
        for (const auto& element : array) {
            if (!element.is_object()) {
                continue;
            }
            modifier::Modifier descriptor = modifierFromJson(element);
            auto& runtime = registry.addFloatModifier(id, descriptor);
            runtime.inputValue = element.value("inputValue", 0.0f);
            runtime.active = element.value("active", descriptor.enabled);
            if (runtime.descriptor.type == modifier::Type::kOsc) {
                runtime.active = false;
                runtime.inputValue = 0.0f;
            }
        }


    }

    void loadBoolModifiersFromJson(ParameterRegistry& registry, const std::string& id, const ofJson& array) {
        if (!array.is_array()) {
            return;
        }
        for (const auto& element : array) {
            if (!element.is_object()) {
                continue;
            }
            modifier::Modifier descriptor = modifierFromJson(element);
            auto& runtime = registry.addBoolModifier(id, descriptor);
            runtime.inputValue = element.value("inputValue", 0.0f);
            runtime.active = element.value("active", descriptor.enabled);
        }
    }

    float parseBaseFloat(const ofJson& node, float fallback) {
        if (node.is_object()) {
            if (node.contains("base") && node["base"].is_number()) {
                return node["base"].get<float>();
            }
            if (node.contains("value") && node["value"].is_number()) {
                return node["value"].get<float>();
            }
        } else if (node.is_number()) {
            return node.get<float>();
        } else if (node.is_boolean()) {
            return node.get<bool>() ? 1.0f : 0.0f;
        }
        return fallback;
    }

    bool parseBaseBool(const ofJson& node, bool fallback) {
        if (node.is_object()) {
            if (node.contains("base") && node["base"].is_boolean()) {
                return node["base"].get<bool>();
            }
            if (node.contains("value") && node["value"].is_boolean()) {
                return node["value"].get<bool>();
            }
            if (node.contains("value") && node["value"].is_number()) {
                return node["value"].get<int>() != 0;
            }
        } else if (node.is_boolean()) {
            return node.get<bool>();
        } else if (node.is_number()) {
            return node.get<int>() != 0;
        }
        return fallback;
    }

    std::string parseBaseString(const ofJson& node, const std::string& fallback) {
        if (node.is_object()) {
            if (node.contains("base") && node["base"].is_string()) {
                return node["base"].get<std::string>();
            }
            if (node.contains("value") && node["value"].is_string()) {
                return node["value"].get<std::string>();
            }
        } else if (node.is_string()) {
            return node.get<std::string>();
        }
        return fallback;
    }

    ofJson encodeFloatParam(const ParameterRegistry::FloatParam& param) {
        if (param.modifiers.empty()) {
            return param.baseValue;
        }
        ofJson node;
        node["base"] = param.baseValue;
        node["modifiers"] = serializeRuntimeModifiers(param.modifiers);
        return node;
    }

    ofJson encodeBoolParam(const ParameterRegistry::BoolParam& param) {
        if (param.modifiers.empty()) {
            return param.baseValue;
        }
        ofJson node;
        node["base"] = param.baseValue;
        node["modifiers"] = serializeRuntimeModifiers(param.modifiers);
        return node;
    }
}  // namespace

class SecondaryDisplayView : public ofBaseApp {
public:
    explicit SecondaryDisplayView(ofApp* host)
        : host_(host) {}

    void draw() override {
        if (!host_) {
            ofBackground(ofColor::black);
            return;
        }
        if (host_->secondaryDisplayRenderPaused_ || host_->sceneLoadInProgress()) {
            host_->drawSceneLoadSnapshot(static_cast<float>(ofGetWidth()),
                                         static_cast<float>(ofGetHeight()));
            return;
        }
        host_->drawSecondaryDisplayWindow(static_cast<float>(ofGetWidth()),
                                          static_cast<float>(ofGetHeight()));
    }

private:
    ofApp* host_ = nullptr;
};

ofApp::ofApp() = default;

void ofApp::setLaunchArguments(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--control-monitor", 0) == 0) {
            std::string value;
            auto eq = arg.find('=');
            if (eq != std::string::npos) {
                value = arg.substr(eq + 1);
            } else if (i + 1 < argc) {
                value = argv[++i];
            }
            std::string lowered = ofToLower(value);
            if (lowered == "off" || lowered == "disable" || lowered == "disabled" || lowered == "false") {
                secondaryDisplayCliOverride_ = false;
                ofLogNotice("ofApp") << "CLI override: controller monitor disabled";
            } else {
                secondaryDisplayCliOverride_ = true;
                if (!value.empty() && lowered != "auto" && lowered != "true") {
                    secondaryDisplayMonitorCliOverride_ = value;
                    ofLogNotice("ofApp") << "CLI override: controller monitor target='" << value << "'";
                } else {
                    secondaryDisplayMonitorCliOverride_.reset();
                    ofLogNotice("ofApp") << "CLI override: controller monitor auto-selected";
                }
            }
        }
    }
}

// ---------------- lifecycle ----------------

std::vector<std::string> ofApp::loadOscChannelHints() const {
    std::vector<std::string> hints;
    std::string cfgPath = ofToDataPath("config/osc-channels.txt", true);
    ofFile file(cfgPath);
    if (!file.exists()) {
        ofBuffer sample;
        sample.append("# One OSC address per line\n");
        sample.append("# Example entries for MatrixPortal device 0x0101\n");
        sample.append("/sensor/matrix/0x0101/mic-level\n");
        sample.append("/sensor/matrix/0x0101/mic-peak\n");
        sample.append("/sensor/matrix/0x0101/mic-gain\n");
        sample.append("/sensor/matrix/0x0101/bioamp-raw\n");
        sample.append("/sensor/matrix/0x0101/bioamp-signal\n");
        sample.append("/sensor/matrix/0x0101/bioamp-rms\n");
        sample.append("# Example entries for Cyberdeck device 0x0201\n");
        sample.append("/sensor/deck/0x0201/deck-intensity\n");
        sample.append("/sensor/deck/0x0201/deck-scene\n");
        sample.append("/sensor/deck/0x0201/battery-soc\n");
        sample.append("/sensor/deck/0x0201/battery-volt\n");
        ofBufferToFile(cfgPath, sample);
        return hints;
    }
    ofBuffer buf = ofBufferFromFile(cfgPath);
    for (auto line : buf.getLines()) {
        std::string trimmed = ofTrim(line);
        if (trimmed.empty()) continue;
        if (trimmed[0] == '#') continue;
        hints.push_back(trimmed);
    }
    return hints;
}

void ofApp::setupLocalMicBridge() {
    localMicSettings_ = LocalMicSettings();
    localMicModifierIndex = static_cast<std::size_t>(-1);
    localMicIngestInitialized_ = false;
    lastLocalMicIngestMs_ = 0;
    lastLocalMicLevel_ = 0.0f;
    lastLocalMicPeak_ = 0.0f;

    std::string cfgPath = ofToDataPath("config/audio.json", true);
    ofJson audioCfg;
    ofFile cfgFile(cfgPath);
    if (!cfgFile.exists()) {
        audioCfg = defaultAudioConfig();
        ofSavePrettyJson(cfgPath, audioCfg);
        ofLogNotice("ofApp") << "Wrote sample audio config to " << cfgPath;
    } else {
        audioCfg = ofLoadJson(cfgPath);
    }
    if (!audioCfg.contains("localMic") || !audioCfg["localMic"].is_object()) {
        ofLogWarning("ofApp") << "audio config missing 'localMic'; host mic disabled";
        return;
    }
    const ofJson& node = audioCfg["localMic"];
    localMicSettings_.enabled = node.value("enabled", false);
    if (!localMicSettings_.enabled) {
        ofLogNotice("ofApp") << "Local mic capture disabled in config/audio.json";
        return;
    }
    localMicSettings_.requestedDeviceIndex = node.value("deviceIndex", -1);
    localMicSettings_.deviceNameContains = node.value("deviceNameContains", std::string());
    localMicSettings_.sampleRate = node.value("sampleRate", localMicSettings_.sampleRate);
    localMicSettings_.bufferSize = node.value("bufferSize", localMicSettings_.bufferSize);
    localMicSettings_.channels = node.value("channels", localMicSettings_.channels);

    if (node.contains("ingest") && node["ingest"].is_object()) {
        const ofJson& ingest = node["ingest"];
        localMicSettings_.ingestLocally = ingest.value("enabled", true);
        localMicSettings_.addressPrefix = normalizeOscPrefix(ingest.value("addressPrefix", localMicSettings_.addressPrefix));
        localMicSettings_.levelAddress = localMicSettings_.addressPrefix + "/mic-level";
        localMicSettings_.peakAddress = localMicSettings_.addressPrefix + "/mic-peak";
        localMicSettings_.ingestRateLimitHz = ingest.value("rateLimitHz", localMicSettings_.ingestRateLimitHz);
        localMicSettings_.ingestDeadband = ingest.value("deadband", localMicSettings_.ingestDeadband);
    } else {
        localMicSettings_.addressPrefix = normalizeOscPrefix(localMicSettings_.addressPrefix);
        localMicSettings_.levelAddress = localMicSettings_.addressPrefix + "/mic-level";
        localMicSettings_.peakAddress = localMicSettings_.addressPrefix + "/mic-peak";
    }
    if (localMicSettings_.ingestRateLimitHz > 0.0f) {
        localMicSettings_.ingestIntervalMs = static_cast<uint64_t>(std::max(1.0f, 1000.0f / localMicSettings_.ingestRateLimitHz));
    } else {
        localMicSettings_.ingestIntervalMs = 0;
    }

    if (node.contains("osc") && node["osc"].is_object()) {
        const ofJson& osc = node["osc"];
        localMicSettings_.publishOsc = osc.value("enabled", false);
        localMicSettings_.oscHost = osc.value("host", localMicSettings_.oscHost);
        localMicSettings_.oscPort = osc.value("port", localMicSettings_.oscPort);
    } else {
        localMicSettings_.publishOsc = false;
        localMicSettings_.oscPort = 0;
    }

    if (node.contains("modifier") && node["modifier"].is_object()) {
        const ofJson& modifierNode = node["modifier"];
        bool modifierEnabled = modifierNode.value("enabled", false);
        if (modifierEnabled) {
            localMicTargetParam = modifierNode.value("target", localMicTargetParam);
            modifier::Modifier micMod;
            micMod.type = modifier::Type::kOsc;
            micMod.blend = modifierBlendFromJson(modifierNode);
            micMod.enabled = true;
            micMod.inputRange.min = 0.0f;
            micMod.inputRange.max = 1.0f;
            micMod.outputRange.min = 0.0f;
            micMod.outputRange.max = 1.0f;
            if (modifierNode.contains("inputRange")) {
                micMod.inputRange = rangeFromJson(modifierNode["inputRange"], micMod.inputRange);
            }
            if (modifierNode.contains("outputRange")) {
                micMod.outputRange = rangeFromJson(modifierNode["outputRange"], micMod.outputRange);
            }
            if (auto* param = paramRegistry.findFloat(localMicTargetParam)) {
                auto& runtime = paramRegistry.addFloatModifier(localMicTargetParam, micMod);
                runtime.ownerTag = "local-mic";
                runtime.active = micMod.enabled;
                runtime.inputValue = 0.0f;
                if (auto* modifiers = paramRegistry.floatModifiers(localMicTargetParam)) {
                    localMicModifierIndex = modifiers->empty() ? static_cast<std::size_t>(-1)
                                                               : modifiers->size() - 1;
                }
                ofLogNotice("ofApp") << "Local mic modifier attached to " << localMicTargetParam
                                     << " (blend=" << modifierBlendToString(micMod.blend) << ")";
            } else {
                ofLogWarning("ofApp") << "Local mic modifier target '" << localMicTargetParam << "' not found";
                localMicModifierIndex = static_cast<std::size_t>(-1);
            }
        }
    }

    auto devices = audioBridge.listInputDevices();
    if (devices.empty()) {
        ofLogWarning("ofApp") << "No audio input devices detected; host mic disabled";
        return;
    }
    int resolvedIndex = resolveInputDeviceIndex(localMicSettings_.requestedDeviceIndex,
                                                localMicSettings_.deviceNameContains,
                                                devices);
    if (resolvedIndex < 0) {
        ofLogWarning("ofApp") << "Unable to resolve audio input device for host mic";
        return;
    }
    auto resolvedLabel = std::find_if(devices.begin(), devices.end(), [&](const auto& entry) {
        return entry.first == resolvedIndex;
    });
    if (resolvedLabel != devices.end()) {
        localMicSettings_.deviceLabel = resolvedLabel->second;
    } else {
        localMicSettings_.deviceLabel = "Audio Device";
    }
    if (!audioBridge.setupDevice(resolvedIndex,
                                 localMicSettings_.sampleRate,
                                 localMicSettings_.bufferSize,
                                 localMicSettings_.channels)) {
        ofLogWarning("ofApp") << "Failed to initialize audio device for host mic (index=" << resolvedIndex << ")";
        return;
    }
    localMicSettings_.deviceIndex = resolvedIndex;
    localMicSettings_.armed = true;
    if (localMicSettings_.publishOsc && localMicSettings_.oscPort > 0) {
        audioBridge.startOscPublisher(localMicSettings_.oscHost, localMicSettings_.oscPort);
        ofLogNotice("ofApp") << "Publishing local mic OSC -> " << localMicSettings_.oscHost << ":" << localMicSettings_.oscPort;
    }
    ofLogNotice("ofApp") << "Local mic armed: device='" << localMicSettings_.deviceLabel
                         << "' index=" << localMicSettings_.deviceIndex
                         << " sampleRate=" << localMicSettings_.sampleRate
                         << " buffer=" << localMicSettings_.bufferSize
                         << " channels=" << localMicSettings_.channels
                         << " prefix=" << localMicSettings_.addressPrefix;
}

void ofApp::updateLocalMicBridge(uint64_t nowMs) {
    if (!localMicSettings_.armed) {
        return;
    }
    if (localMicModifierIndex != static_cast<std::size_t>(-1)) {
        audioBridge.update(paramRegistry, localMicTargetParam, localMicModifierIndex);
    }
    float rms = audioBridge.lastRms();
    float peak = audioBridge.lastPeak();
    if (!lastMicLogMs || nowMs - lastMicLogMs >= 1000) {
        ofLogVerbose("AudioInputBridge") << "local mic rms=" << rms << " peak=" << peak;
        lastMicLogMs = nowMs;
    }
    if (!localMicSettings_.ingestLocally) {
        return;
    }
    bool shouldEmit = false;
    if (!localMicIngestInitialized_) {
        shouldEmit = true;
    } else {
        bool intervalOk = (localMicSettings_.ingestIntervalMs == 0) ||
                          (nowMs - lastLocalMicIngestMs_ >= localMicSettings_.ingestIntervalMs);
        bool levelDelta = std::fabs(rms - lastLocalMicLevel_) >= localMicSettings_.ingestDeadband;
        bool peakDelta = std::fabs(peak - lastLocalMicPeak_) >= localMicSettings_.ingestDeadband;
        shouldEmit = intervalOk && (levelDelta || peakDelta);
    }
    if (!shouldEmit) {
        return;
    }
    ingestOscMessage(localMicSettings_.levelAddress, rms);
    ingestOscMessage(localMicSettings_.peakAddress, peak);
    lastLocalMicLevel_ = rms;
    lastLocalMicPeak_ = peak;
    lastLocalMicIngestMs_ = nowMs;
    localMicIngestInitialized_ = true;
}

void ofApp::suspendOscRoute(const std::string& paramId, uint64_t durationMs) const {
    if (paramId.empty()) {
        return;
    }
    uint64_t now = ofGetElapsedTimeMillis();
    oscRouteMuteUntilMs_[paramId] = now + durationMs;
}

bool ofApp::oscRouteWriteAllowed(const std::string& paramId) const {
    if (paramId.empty()) {
        return true;
    }
    auto it = oscRouteMuteUntilMs_.find(paramId);
    if (it == oscRouteMuteUntilMs_.end()) {
        return true;
    }
    uint64_t now = ofGetElapsedTimeMillis();
    if (now >= it->second) {
        oscRouteMuteUntilMs_.erase(it);
        return true;
    }
    return false;
}

void ofApp::emitOscModifierTelemetry(const std::string& paramId, float rawValue) const {
    if (paramId.empty()) {
        return;
    }
    static constexpr uint64_t kMinIntervalMs = 200;
    uint64_t now = ofGetElapsedTimeMillis();
    auto it = oscModifierTelemetryMs_.find(paramId);
    if (it != oscModifierTelemetryMs_.end() && now - it->second < kMinIntervalMs) {
        return;
    }
    oscModifierTelemetryMs_[paramId] = now;
    float baseValue = 0.0f;
    if (auto* fp = paramRegistry.findFloat(paramId)) {
        baseValue = fp->baseValue;
    }
    std::ostringstream detail;
    detail << paramId << " base=" << ofToString(baseValue, 3);
    publishHudTelemetrySample("hud.sensors", "osc.mod", rawValue, detail.str());
}

void ofApp::setup() {
    ofSetFrameRate(60);
    ofBackground(0);
    ofSetWindowTitle("Synaptome");
    ofSetEscapeQuitsApp(false);
    rotateRunLog();
    ofLogToFile(ofToDataPath(kRunLogPath, true), false);
    ofLogNotice("ofApp") << "Run log initialized; retaining last "
                         << kRetainedRunLogs
                         << " archived run log(s) in "
                         << kRunLogArchiveDir;

    t = 0.0f;

    param_speed = speed;
    param_camDist = camDist;
    param_bpm = 120.0f;
    param_masterFx = 0.0f;

    paramRegistry = ParameterRegistry();
    configureDefaultBanks();

    auto makeRange = [](float min, float max, float step) {
        ParameterRegistry::Range range;
        range.min = min;
        range.max = max;
        range.step = step;
        return range;
    };

    auto addFloat = [&](const std::string& id, float* ptr, float def, const std::string& label, const std::string& group, const ParameterRegistry::Range& range, bool quick = false, int order = 0, const std::string& units = std::string(), const std::string& desc = std::string()) {
        ParameterRegistry::Descriptor meta;
        meta.label = label;
        meta.group = group;
        meta.units = units;
        meta.description = desc;
        meta.range = range;
        meta.quickAccess = quick;
        meta.quickAccessOrder = order;
        paramRegistry.addFloat(id, ptr, def, meta);
    };

    auto addBool = [&](const std::string& id, bool* ptr, bool def, const std::string& label, const std::string& group, const std::string& desc = std::string()) {
        ParameterRegistry::Descriptor meta;
        meta.label = label;
        meta.group = group;
        meta.description = desc;
        paramRegistry.addBool(id, ptr, def, meta);
    };

    auto addString = [&](const std::string& id, std::string* ptr, const std::string& def, const std::string& label, const std::string& group, const std::string& desc = std::string()) {
        ParameterRegistry::Descriptor meta;
        meta.label = label;
        meta.group = group;
        meta.description = desc;
        paramRegistry.addString(id, ptr, def, meta);
    };

    addFloat("transport.bpm", &param_bpm, param_bpm, "BPM", "Transport", makeRange(40.0f, 240.0f, 1.0f), true, 0, "bpm", "Session tempo in beats per minute");
    addFloat("globals.speed", &param_speed, param_speed, "Speed", "Transport", makeRange(0.0f, 5.0f, 0.05f), true, 5, "x", "Global animation speed multiplier");
    addFloat("fx.master", &param_masterFx, param_masterFx, "Master FX", "Post FX", makeRange(0.0f, 1.0f, 0.01f), true, 40, std::string(), "Master post-processing amount");
    addFloat("camera.dist", &param_camDist, param_camDist, "Camera Distance", "Camera", makeRange(150.0f, 4000.0f, 1.0f));

    addBool("ui.hud", &param_showHud, param_showHud, "HUD Visible", "UI", "Toggle on-screen HUD overlay");
    addBool("ui.console.visible",
            &param_showConsole,
            param_showConsole,
            "Console Visible",
            "UI",
            "Toggle the console grid overlay");
    addBool("ui.hub.visible",
            &param_showControlHub,
            param_showControlHub,
            "Browser Visible",
            "UI",
            "Toggle the Browser overlay (Control Panel launcher for the console + slots)");
    addBool("ui.menu.visible",
            &param_showMenus,
            param_showMenus,
            "Menu Stack Visible",
            "UI",
            "Toggle auxiliary menus (parameters, key map, devices, layout editor)");
    addFloat("ui.menu_text_size",
             &param_menuTextSize,
             param_menuTextSize,
             "Menu Text Size",
             "UI",
             makeRange(0.75f, 2.5f, 0.05f),
             false,
             0,
             "x",
             "Scale text across HUD, Console, and Browser menus");
    addBool("console.controller.focus_console",
            &param_controllerFocusConsole,
            param_controllerFocusConsole,
            "Controller Focus (Console)",
            "UI",
            "Keep keyboard shortcuts routed through the projector console window");
    addString("console.dual_display.mode",
              &param_dualDisplayMode,
              param_dualDisplayMode,
              "Dual Display Mode",
              "UI",
              "Select 'single' (default) or 'dual' controller surface");
    addBool("console.secondary_display.enabled",
            &param_secondaryDisplayEnabled,
            param_secondaryDisplayEnabled,
            "Controller Monitor Enabled",
            "UI",
            "Spawn the detachable controller monitor window");
    addString("console.secondary_display.monitor",
              &param_secondaryDisplayMonitor,
              param_secondaryDisplayMonitor,
              "Controller Monitor Target",
              "UI",
              "Optional monitor identifier for the controller window (blank = auto)");
    addBool("console.secondary_display.follow_primary",
            &param_secondaryDisplayFollowPrimary,
            param_secondaryDisplayFollowPrimary,
            "Controller Layout Follows Main",
            "UI",
            "When enabled, the controller monitor mirrors the projector overlays.");
    addBool("console.secondary_display.layout_watchdog",
            &param_layoutWatchdogEnabled,
            param_layoutWatchdogEnabled,
            "Controller Layout Watchdog",
            "UI",
            "Keep controller HUD layouts synchronized across DPI/resume events.");
    addBool("console.secondary_display.force_resync",
            &param_layoutResyncRequest,
            param_layoutResyncRequest,
            "Force HUD Layout Resync",
            "UI",
            "Set true to rebroadcast controller/projector HUD layout snapshots.");

    auto& textState = TextLayerState::instance();
    textState.refreshAvailableFonts();
    const std::string overlayGroup = "Overlay";
    addString("overlay.text.content",
              &textState.content,
              textState.content,
              "Text Content",
              overlayGroup,
              "Text displayed by the HUD overlay");
    addString("overlay.text.font",
              &textState.font,
              textState.font,
              "Font File",
              overlayGroup,
              "TrueType font filename under data/fonts");
    float fontIndexMax = textState.fontIndexMax();
    addFloat("overlay.text.fontIndex",
             &textState.fontIndex,
             textState.fontIndex,
             "Font Index",
             overlayGroup,
             makeRange(0.0f, fontIndexMax, 1.0f),
             false,
             0,
             std::string(),
             "Select discovered font by index");
    addFloat("overlay.text.size",
             &textState.fontSize,
             textState.fontSize,
             "Text Size",
             overlayGroup,
             makeRange(12.0f, 256.0f, 1.0f),
             false,
             0,
             "px",
             "Font size in pixels");
    addFloat("overlay.text.color.r",
             &textState.colorR,
             textState.colorR,
             "Text Color R",
             overlayGroup,
             makeRange(0.0f, 255.0f, 1.0f));
    addFloat("overlay.text.color.g",
             &textState.colorG,
             textState.colorG,
             "Text Color G",
             overlayGroup,
             makeRange(0.0f, 255.0f, 1.0f));
    addFloat("overlay.text.color.b",
             &textState.colorB,
             textState.colorB,
             "Text Color B",
             overlayGroup,
             makeRange(0.0f, 255.0f, 1.0f));
    struct SensorParamDef {
        const char* id;
        float* valuePtr;
        const char* label;
        ParameterRegistry::Range range;
        const char* units;
    };
    const SensorParamDef kBioAmpParams[] = {
        {"sensors.bioamp.raw", &bioAmpParameters_.raw, "BioAmp Raw", makeRange(-2.0f, 2.0f, 0.001f), ""},
        {"sensors.bioamp.signal", &bioAmpParameters_.signal, "BioAmp Filtered", makeRange(-2.0f, 2.0f, 0.001f), ""},
        {"sensors.bioamp.mean", &bioAmpParameters_.mean, "BioAmp Mean", makeRange(-2.0f, 2.0f, 0.001f), ""},
        {"sensors.bioamp.rms", &bioAmpParameters_.rms, "BioAmp RMS", makeRange(0.0f, 2.0f, 0.001f), ""},
        {"sensors.bioamp.dom_hz", &bioAmpParameters_.domHz, "BioAmp Dominant Hz", makeRange(0.0f, 60.0f, 0.01f), "Hz"},
        {"sensors.bioamp.sample_rate", &bioAmpParameters_.sampleRate, "BioAmp Sample Rate", makeRange(0.0f, 1024.0f, 1.0f), "Hz"},
        {"sensors.bioamp.window", &bioAmpParameters_.window, "BioAmp Window", makeRange(0.0f, 1024.0f, 1.0f), "samples"}
    };
    for (const auto& def : kBioAmpParams) {
        addFloat(def.id, def.valuePtr, *def.valuePtr, def.label, "Sensors", def.range, false, 0, def.units);
    }

    postEffects.setup(paramRegistry);

    auto& factory = LayerFactory::instance();
    factory.registerType("grid", []() { return std::make_unique<GridLayer>(); });
    factory.registerType("geodesic", []() { return std::make_unique<GeodesicLayer>(); });
    factory.registerType("oscilloscope", []() { return std::make_unique<OscilloscopeLayer>(); });
    factory.registerType("perlin", []() { return std::make_unique<PerlinNoiseLayer>(); });
    factory.registerType("stlModel", []() { return std::make_unique<StlModelLayer>(); });
    factory.registerType("gameOfLife", []() { return std::make_unique<GameOfLifeLayer>(); });
    factory.registerType("agentField", []() { return std::make_unique<AgentFieldLayer>(); });
    factory.registerType("flocking", []() { return std::make_unique<FlockingLayer>(); });
    factory.registerType("media.webcam", []() { return std::make_unique<VideoGrabberLayer>(); });
    factory.registerType("media.clip", []() { return std::make_unique<VideoClipLayer>(); });
    factory.registerType("text", []() { return std::make_unique<TextLayer>(); });

    std::string layersRoot = ofToDataPath("layers", true);
    layerLibrary.reload(layersRoot);

    midiMapPath = ofToDataPath("config/midi-map.json", true);
    midi.load(midiMapPath);
    registerCoreMidiTargets();
    hotkeyMapPath = ofToDataPath("config/hotkeys.json", true);
    hudConfigPath = ofToDataPath("config/hud.json", true);
    overlayLayoutPath = ofToDataPath("config/overlays.json", true);
    deviceMapsDir = ofToDataPath("device_maps", true);
    controlHubPrefsPath = ofToDataPath("config/control_hub_prefs.json", true);

    overlayManager.setHudSkin(menuSkin.hud);
    overlayManager.setHost(this);
    hudRegistry.setOverlayManager(&overlayManager);
    hudRegistry.setLayoutChangedCallback([this]() {
        if (controlMappingHub) {
            controlMappingHub->notifyHudLayoutChanged();
        }
    });
    controlHubEventBridge = std::make_unique<ControlHubEventBridge>(&hudRegistry);
    controlHubEventBridge->setTelemetrySink([this](const std::string& widgetId,
                                                   const std::string& feedId,
                                                   float value,
                                                   const std::string& detail) {
        applyHudTelemetryOverride(widgetId, feedId, value, detail);
    });
    controlHubEventBridge->setSensorTelemetrySink([this](const std::string& parameterId,
                                                         float value,
                                                         uint64_t timestampMs) {
        handleSensorTelemetrySample(parameterId, value, timestampMs);
    });

    auto registerHudToggle = [&](const std::string& id,
                                 const std::string& label,
                                 const std::string& description,
                                 bool* valuePtr) {
        HudRegistry::Toggle toggle;
        toggle.id = id;
        toggle.label = label;
        toggle.description = description;
        toggle.defaultValue = valuePtr ? *valuePtr : false;
        toggle.valuePtr = valuePtr;
        hudRegistry.registerToggle(toggle);
        if (valuePtr) {
            addBool(id, valuePtr, toggle.defaultValue, label, "HUD", description);
        }
    };

    registerHudToggle("hud.controls", "Controls", "Show key and control hints", &hudShowControls);
    registerHudToggle("hud.status", "Status", "Show system status summary", &hudShowStatus);
    registerHudToggle("hud.layers", "Layers", "Show active layer summary", &hudShowLayers);
    registerHudToggle("hud.sensors", "Sensors", "Show recent sensor telemetry", &hudShowSensors);
    registerHudToggle("hud.menu", "Menu Mirror", "Show current menu breadcrumbs", &hudShowMenu);
    registerHudWidgetsFromCatalog();

    consoleSlots.resize(8);
    consoleConfigPath = ofToDataPath("config/console.json", true);
    consolePersistenceSuspended_ = true;
    bool consoleConfigNeedsUpgrade = false;
    {
        auto persisted = ConsoleStore::loadState(consoleConfigPath);
        consoleConfigNeedsUpgrade = persisted.version < 3 || !persisted.overlaysDefined || !persisted.dualDisplayDefined || !persisted.secondaryDisplayDefined || !persisted.controllerFocusDefined;
        for (const auto& info : persisted.layers) {
            if (addAssetToConsoleLayer(info.index, info.assetId, info.active, info.opacity)) {
                if (auto* slot = consoleSlotForIndex(info.index)) {
                    if (!info.label.empty()) {
                        slot->label = info.label;
                    }
                    if (info.coverage.defined) {
                        importConsoleCoverageFromInfo(info.index, info.coverage);
                    }
                }
            }
        }
        overlayVisibility_.hud = persisted.overlays.hudVisible;
        overlayVisibility_.console = persisted.overlays.consoleVisible;
        overlayVisibility_.controlHub = persisted.overlays.controlHubVisible;
        overlayVisibility_.menus = persisted.overlays.menuVisible;
        persistedDualDisplayMode_ = normalizeDualDisplayMode(persisted.dualDisplay.mode);
        param_showHud = overlayVisibility_.hud;
        param_showConsole = overlayVisibility_.console;
        param_showControlHub = overlayVisibility_.controlHub;
        param_showMenus = overlayVisibility_.menus;
        param_dualDisplayMode = persistedDualDisplayMode_;
        param_secondaryDisplayEnabled = persisted.secondaryDisplay.enabled;
        param_secondaryDisplayMonitor = persisted.secondaryDisplay.monitorId;
        param_secondaryDisplayX = persisted.secondaryDisplay.x;
        param_secondaryDisplayY = persisted.secondaryDisplay.y;
        param_secondaryDisplayWidth = persisted.secondaryDisplay.width;
        param_secondaryDisplayHeight = persisted.secondaryDisplay.height;
        param_secondaryDisplayVsync = persisted.secondaryDisplay.vsync;
        param_secondaryDisplayDpi = persisted.secondaryDisplay.dpiScale;
        param_secondaryDisplayBackground = persisted.secondaryDisplay.background;
        param_secondaryDisplayFollowPrimary = persisted.secondaryDisplay.followPrimary;
        if (persisted.controllerFocusDefined) {
            param_controllerFocusConsole = persisted.controllerFocus.consolePreferred;
        }
        if (persisted.sensorsDefined && persisted.sensors.bioAmpDefined) {
            pendingBioAmpSnapshot_ = persisted.sensors.bioAmp;
            pendingBioAmpSeedDefined_ =
                pendingBioAmpSnapshot_.hasRaw || pendingBioAmpSnapshot_.hasSignal || pendingBioAmpSnapshot_.hasMean ||
                pendingBioAmpSnapshot_.hasRms || pendingBioAmpSnapshot_.hasDomHz || pendingBioAmpSnapshot_.hasSampleRate ||
                pendingBioAmpSnapshot_.hasWindow;
        }
    }
    if (secondaryDisplayCliOverride_) {
        param_secondaryDisplayEnabled = *secondaryDisplayCliOverride_;
    }
    if (secondaryDisplayMonitorCliOverride_) {
        param_secondaryDisplayMonitor = *secondaryDisplayMonitorCliOverride_;
    }
    secondaryDisplay_.enabled = param_secondaryDisplayEnabled;
    secondaryDisplay_.monitorId = param_secondaryDisplayMonitor;
    secondaryDisplay_.x = param_secondaryDisplayX;
    secondaryDisplay_.y = param_secondaryDisplayY;
    secondaryDisplay_.width = param_secondaryDisplayWidth;
    secondaryDisplay_.height = param_secondaryDisplayHeight;
    secondaryDisplay_.vsync = param_secondaryDisplayVsync;
    secondaryDisplay_.dpiScale = param_secondaryDisplayDpi;
    secondaryDisplay_.background = param_secondaryDisplayBackground;
    secondaryDisplay_.followPrimary = param_secondaryDisplayFollowPrimary;
    controllerFocus_.preferConsole = param_controllerFocusConsole;
    auto setBoolBaseIfPresent = [&](const std::string& id, bool value) {
        if (paramRegistry.findBool(id)) {
            paramRegistry.setBoolBase(id, value, true);
        }
    };
    auto setStringBaseIfPresent = [&](const std::string& id, const std::string& value) {
        if (paramRegistry.findString(id)) {
            paramRegistry.setStringBase(id, value, true);
        }
    };
    setBoolBaseIfPresent("ui.hud", param_showHud);
    setBoolBaseIfPresent("ui.console.visible", param_showConsole);
    setBoolBaseIfPresent("ui.hub.visible", param_showControlHub);
    setBoolBaseIfPresent("ui.menu.visible", param_showMenus);
    setBoolBaseIfPresent("console.controller.focus_console", param_controllerFocusConsole);
    setStringBaseIfPresent("console.dual_display.mode", param_dualDisplayMode);
    setBoolBaseIfPresent("console.secondary_display.enabled", param_secondaryDisplayEnabled);
    setStringBaseIfPresent("console.secondary_display.monitor", param_secondaryDisplayMonitor);
    setBoolBaseIfPresent("console.secondary_display.follow_primary", param_secondaryDisplayFollowPrimary);
    seedConsoleDefaultsIfEmpty();
    refreshLayerReferences();

    bool sceneLoaded = loadScene(kSceneAutosavePath);
    if (!sceneLoaded) {
        sceneLoaded = loadScene(kDefaultScenePath);
    }
    seedConsoleDefaultsIfEmpty();
    refreshLayerReferences();
    ensureActiveBankValid();
    consolePersistenceSuspended_ = false;
    if (sceneLoaded || consoleConfigNeedsUpgrade) {
        persistConsoleAssignments();
    }
    requestSecondaryDisplay(secondaryDisplay_.enabled, "startup-config");
    handleControllerFocusParamChange();

    // Console UI state (scaffold)
    consoleState = std::make_shared<ConsoleState>();
    consoleState->setMenuSkin(menuSkin);
    // Hook ConsoleState -> host callback so Console can request an asset picker
    // for a specific console layer (1..8).
    consoleState->setParameterRegistry(&paramRegistry);
    consoleState->setRequestAssetBrowserCallback([this](int layerIndex) {
        openAssetBrowserForConsole(layerIndex);
    });

    threeBandLayout.setGutters(24.0f, 8.0f);
    consoleState->setRequestClearLayerCallback([this](int layerIndex) {
        int idx = layerIndex - 1;
        if (idx < 0 || idx >= static_cast<int>(consoleSlots.size())) {
            ofLogWarning("Console") << "Cannot clear console layer " << layerIndex << " (out of range)";
            return;
        }
        clearConsoleSlot(idx);
        persistConsoleAssignments();
        menuController.requestViewModelRefresh();
    });
    consoleState->setQueryLayerCallback([this](int layerIndex) -> ConsoleLayerInfo {
        ConsoleLayerInfo info;
        if (layerIndex < 1 || layerIndex > static_cast<int>(consoleSlots.size())) return info;
        info.index = layerIndex;
        if (const auto* slot = consoleSlotForIndex(layerIndex)) {
            info.assetId = slot->assetId;
            if (!slot->assetId.empty()) {
                info.label = slot->label.empty() ? slot->assetId : slot->label;
                info.active = slot->active;
                info.opacity = slot->opacity;
                info.coverage = slot->coverage;
            }
        }
        return info;
    });
    // Parameter-slot preview callback: return a short preview string for a given
    // console layer (1..8) and row (0-based). Rows map directly to MIDIMix slot
    // order so the console mirrors the physical controller.
    consoleState->setQueryParameterPreviewCallback([this](int layerIndex, int rowIndex) -> ConsoleState::ParameterPreview {
        ConsoleState::ParameterPreview preview;
        static const char* kSlotOrder[] = {"K1", "K2", "K3", "B1", "B2", "F"};
        constexpr int kSlotOrderCount = static_cast<int>(sizeof(kSlotOrder) / sizeof(kSlotOrder[0]));
        if (rowIndex < 0 || rowIndex >= kSlotOrderCount) {
            return preview;
        }
        std::string prefix = consoleSlotPrefix(layerIndex);
        if (prefix.empty()) {
            return preview;
        }
        std::string columnId = "column" + ofToString(layerIndex);
        const std::string slotId = kSlotOrder[rowIndex];
        auto matchesColumn = [&](const std::string& bindingColumn) {
            return bindingColumn.empty() || bindingColumn == columnId;
        };
        auto matchesTarget = [&](const std::string& target) {
            return target.rfind(prefix + ".", 0) == 0;
        };
        auto describeValue = [&](const std::string& paramId, std::string& labelOut, std::string& valueOut) {
            if (auto* fp = paramRegistry.findFloat(paramId)) {
                labelOut = fp->meta.label.empty() ? fp->meta.id : fp->meta.label;
                if (fp->value) {
                    valueOut = ofToString(*fp->value, 2);
                } else {
                    valueOut = ofToString(fp->baseValue, 2);
                }
                return;
            }
            if (auto* bp = paramRegistry.findBool(paramId)) {
                labelOut = bp->meta.label.empty() ? bp->meta.id : bp->meta.label;
                bool current = bp->value ? *bp->value : bp->baseValue;
                valueOut = current ? "1" : "0";
                return;
            }
            if (auto* sp = paramRegistry.findString(paramId)) {
                labelOut = sp->meta.label.empty() ? sp->meta.id : sp->meta.label;
                valueOut = sp->value ? *sp->value : sp->baseValue;
                return;
            }
            labelOut = paramId;
        };
        auto applyPreview = [&](const std::string& paramId, const std::string& mappingDetail) {
            preview.parameterId = paramId;
            preview.valid = true;
            std::string label;
            std::string value;
            describeValue(paramId, label, value);
            preview.label = label.empty() ? paramId : label;
            if (!mappingDetail.empty() && !value.empty()) {
                preview.detail = mappingDetail + " = " + value;
            } else if (!mappingDetail.empty()) {
                preview.detail = mappingDetail;
            } else {
                preview.detail = value;
            }
        };
        auto describeCc = [](const MidiRouter::CcMap& map) {
            std::string detail = "CC";
            if (map.channel >= 0) {
                detail += " ch" + ofToString(map.channel);
            }
            detail += " #" + ofToString(map.cc);
            if (map.hardwareKnown) {
                detail += " hw=" + ofToString(map.lastHardwareNorm, 2);
            }
            return detail;
        };
        auto describeBtn = [](const MidiRouter::BtnMap& map) {
            std::string detail = map.type.empty() ? std::string("Btn") : ("Btn " + map.type);
            if (map.channel >= 0) {
                detail += " ch" + ofToString(map.channel);
            }
            detail += " #" + ofToString(map.num);
            return detail;
        };

        const bool wantsButton = (slotId == "B1" || slotId == "B2");
        if (!wantsButton) {
            const auto& ccMaps = midi.getCcMaps();
            for (const auto& map : ccMaps) {
                if (map.slotId != slotId) {
                    continue;
                }
                if (!matchesColumn(map.columnId)) {
                    continue;
                }
                if (!matchesTarget(map.target)) {
                    continue;
                }
                applyPreview(map.target, describeCc(map));
                break;
            }
        } else {
            const auto& btnMaps = midi.getBtnMaps();
            for (const auto& map : btnMaps) {
                if (map.slotId != slotId) {
                    continue;
                }
                if (!matchesColumn(map.columnId)) {
                    continue;
                }
                if (!matchesTarget(map.target)) {
                    continue;
                }
                applyPreview(map.target, describeBtn(map));
                break;
            }
        }
        return preview;
    });

    std::string slotAssignmentsPath = controlHubSlotAssignmentsPath();
    controlMappingHub = std::make_shared<ControlMappingHubState>();
    if (controlMappingHub) {
        controlMappingHub->setMenuSkin(menuSkin);
        controlMappingHub->setPreferencesPath(controlHubPrefsPath);
        controlMappingHub->setParameterRegistry(&paramRegistry);
        controlMappingHub->setMidiRouter(&midi);
        controlMappingHub->setLayerLibrary(&layerLibrary);
        controlMappingHub->setDeviceMapsDirectory(deviceMapsDir);
        controlMappingHub->setSlotAssignmentsPath(slotAssignmentsPath);
        controlMappingHub->setConsoleSlotLoadCallback([this](int layerIndex, const std::string& assetId) {
            return addAssetToConsoleLayer(layerIndex, assetId, true);
        });
        controlMappingHub->setConsoleSlotUnloadCallback([this](int layerIndex) {
            if (layerIndex < 1 || layerIndex > static_cast<int>(consoleSlots.size())) {
                return false;
            }
            clearConsoleSlot(layerIndex - 1);
            persistConsoleAssignments();
            return true;
        });
        controlMappingHub->setConsoleSlotInventoryCallback([this]() {
            std::vector<ConsoleLayerInfo> inventory;
            inventory.reserve(consoleSlots.size());
            for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
                ConsoleLayerInfo info;
                info.index = static_cast<int>(i) + 1;
                info.assetId = consoleSlots[i].assetId;
                info.active = consoleSlots[i].active;
                info.opacity = consoleSlots[i].opacity;
                info.label = consoleSlots[i].label;
                info.coverage = consoleSlots[i].coverage;
                inventory.push_back(info);
            }
            return inventory;
        });
        controlMappingHub->setSavedSceneListCallback([this]() {
            return listSavedScenes();
        });
        controlMappingHub->setSavedSceneLoadCallback([this](const std::string& sceneId) {
            return loadSavedSceneById(sceneId);
        });
        controlMappingHub->setSavedSceneSaveAsCallback([this](const std::string& sceneName, bool overwrite) {
            return saveNamedScene(sceneName, overwrite);
        });
        controlMappingHub->setSavedSceneOverwriteCallback([this](const std::string& sceneId) {
            return overwriteSavedSceneById(sceneId);
        });
        controlMappingHub->setConsoleAssetResolver([this](const std::string& prefix) -> const LayerLibrary::Entry* {
            static const std::string kPrefix = "console.layer";
            if (prefix.rfind(kPrefix, 0) != 0) {
                return nullptr;
            }
            std::string suffix = prefix.substr(kPrefix.size());
            int layerIndex = ofToInt(suffix);
            const ConsoleSlot* slot = consoleSlotForIndex(layerIndex);
            if (!slot || slot->assetId.empty()) {
                return nullptr;
            }
            return layerLibrary.find(slot->assetId);
        });
        controlMappingHub->setFloatValueCommitCallback([this](const std::string& paramId, float value) {
            suspendOscRoute(paramId, 750);
            static const std::string kConsolePrefix = "console.layer";
            static const std::string kOpacitySuffix = ".opacity";
            auto effectFromCoverageParam = [](const std::string& id) -> std::optional<std::string> {
                if (id == "effects.dither.coverage") return "fx.dither";
                if (id == "effects.ascii.coverage") return "fx.ascii";
                if (id == "effects.asciiSupersample.coverage") return "fx.ascii_supersample";
                if (id == "effects.crt.coverage") return "fx.crt";
                if (id == "effects.motion.coverage") return "fx.motion_extract";
                return std::nullopt;
            };

            if (auto effectType = effectFromCoverageParam(paramId)) {
                propagateEffectCoverageChange(*effectType, value);
                return;
            }

            if (paramId.rfind(kConsolePrefix, 0) != 0) {
                return;
            }
            auto layerIndex = consoleLayerIndexFromParam(paramId);
            if (!layerIndex) {
                return;
            }
            if (paramId.size() < kOpacitySuffix.size() ||
                paramId.compare(paramId.size() - kOpacitySuffix.size(), kOpacitySuffix.size(), kOpacitySuffix) != 0) {
                return;
            }
            persistConsoleAssignments();
        });
        controlMappingHub->setHudVisibilityCallback([this](bool visible) {
            handleHudVisibilityChanged(visible);
        });
        controlMappingHub->setHudToggleCallback([this](const std::string& id, bool enabled) {
            hudRegistry.setValue(id, enabled);
        });
        controlMappingHub->setHudPlacementProvider([this]() {
            std::vector<ControlMappingHubState::HudPlacementSnapshot> placements;
            const auto widgets = hudRegistry.widgets();
            placements.reserve(widgets.size());
            for (const auto& widget : widgets) {
                ControlMappingHubState::HudPlacementSnapshot snapshot;
                snapshot.id = widget.metadata.id;
                snapshot.bandId = overlayBandToString(widget.band);
                switch (widget.band) {
                case OverlayWidget::Band::Console: snapshot.bandLabel = "Console"; break;
                case OverlayWidget::Band::Workbench: snapshot.bandLabel = "Workbench"; break;
                case OverlayWidget::Band::Hud:
                default:
                    snapshot.bandLabel = "HUD";
                    break;
                }
                snapshot.columnIndex = widget.columnIndex;
                snapshot.columnLabel = widget.columnId.empty()
                    ? ("Column " + ofToString(widget.columnIndex + 1))
                    : widget.columnId;
                snapshot.visible = widget.visible;
                snapshot.collapsed = widget.collapsed;
                snapshot.target = overlayTargetToString(widget.metadata.target);
                placements.push_back(std::move(snapshot));
            }
            return placements;
        });
        controlMappingHub->setHudPlacementCallback([this](const std::string& widgetId, int columnIndex) {
            hudRegistry.setWidgetColumn(widgetId, columnIndex);
        });
        controlMappingHub->setHudFeedRegistry(&hudFeedRegistry);
        controlMappingHub->setEventCallback([this](const std::string& payload) {
            handleControlHubEvent(payload);
            if (controlHubEventBridge) {
                controlHubEventBridge->onEventJson(payload);
            }
        });
        if (pendingBioAmpSeedDefined_) {
            auto seedMetric = [&](const char* metricId, bool hasField, float value, uint64_t ts) {
                if (hasField) {
                    controlMappingHub->setBioAmpMetric(metricId, value, ts);
                }
            };
            seedMetric("bioamp-raw", pendingBioAmpSnapshot_.hasRaw, pendingBioAmpSnapshot_.raw, pendingBioAmpSnapshot_.rawTimestampMs);
            seedMetric("bioamp-signal", pendingBioAmpSnapshot_.hasSignal, pendingBioAmpSnapshot_.signal, pendingBioAmpSnapshot_.signalTimestampMs);
            seedMetric("bioamp-mean", pendingBioAmpSnapshot_.hasMean, pendingBioAmpSnapshot_.mean, pendingBioAmpSnapshot_.meanTimestampMs);
            seedMetric("bioamp-rms", pendingBioAmpSnapshot_.hasRms, pendingBioAmpSnapshot_.rms, pendingBioAmpSnapshot_.rmsTimestampMs);
            seedMetric("bioamp-dom-hz", pendingBioAmpSnapshot_.hasDomHz, pendingBioAmpSnapshot_.domHz, pendingBioAmpSnapshot_.domTimestampMs);
            if (pendingBioAmpSnapshot_.hasSampleRate) {
                controlMappingHub->setBioAmpMetadata("bioamp-sample-rate",
                                                     static_cast<float>(pendingBioAmpSnapshot_.sampleRate),
                                                     pendingBioAmpSnapshot_.sampleRateTimestampMs);
            }
            if (pendingBioAmpSnapshot_.hasWindow) {
                controlMappingHub->setBioAmpMetadata("bioamp-window",
                                                     static_cast<float>(pendingBioAmpSnapshot_.window),
                                                     pendingBioAmpSnapshot_.windowTimestampMs);
            }
            pendingBioAmpSeedDefined_ = false;
        }
        bool migratedHud = controlMappingHub->importLegacyHudConfig(hudConfigPath, overlayLayoutPath);
        if (migratedHud) {
            auto pruneLegacy = [](const std::string& path, const std::string& label) {
                ofFile file(path);
                if (!file.exists()) {
                    return;
                }
                if (!file.remove()) {
                    ofLogWarning("ofApp") << "Failed to prune legacy " << label << " at " << path;
                }
            };
            pruneLegacy(hudConfigPath, "HUD config");
            pruneLegacy(overlayLayoutPath, "HUD layout");
        }
        // Sync console/post-effect inventory restored from disk so the Browser + console previews are accurate on launch.
        controlMappingHub->refreshConsoleSlotBindings();
        syncActiveFxWithConsoleSlots();
        syncHudLayoutTarget();
        broadcastHudLayoutSnapshots("startup");
        broadcastHudRoutingManifest();
    }
    applyMenuTextSkin();

    keyMappingUi = std::make_shared<KeyMappingUI>(&hotkeyManager);
    hudLayoutEditor = std::make_shared<HudLayoutEditor>(&hudRegistry, &overlayManager);

    ensureActiveBankValid();

    auto oscHints = loadOscChannelHints();
    if (oscHints.empty()) {
        oscHints = {
            "/sensor/matrix/0x0101/mic-level",
            "/sensor/matrix/0x0101/mic-peak",
            "/sensor/matrix/0x0101/mic-gain",
            "/sensor/matrix/0x0101/bioamp-raw",
            "/sensor/matrix/0x0101/bioamp-signal",
            "/sensor/matrix/0x0101/bioamp-rms",
            "/sensor/deck/0x0201/deck-intensity",
            "/sensor/deck/0x0201/deck-scene",
            "/sensor/deck/0x0201/battery-soc",
            "/sensor/deck/0x0201/battery-volt"
        };
    }
    midi.seedOscSources(oscHints);
    midi.setFloatTargetTouchedCallback([this](const std::string& paramId) {
        suspendOscRoute(paramId, 500);
    });

    midi.onOscMapAdded = nullptr;
    midi.onOscRoutesChanged = [this]() {
        rebuildDynamicOscRoutes();
    };

    collector.setLogTag("CollectorSerial");
    collector.setAutoPortHints({ "usb vid:pid=239a:811b ser=10:51:db:31:12:cc", "ser=10:51:db:31:12:cc", "feather esp32-s3", "vid:pid=239a:811b", "239a:811b", "239a", "feather", "usbmodem", "usbserial" });
    collector.setReconnectInterval(1500);
    collector.setBaudRate(115200);

    rebuildDynamicOscRoutes();
    setupLocalMicBridge();
    oscHistory.clear();

    devicesPanel = std::make_shared<DevicesPanel>();
    if (devicesPanel) {
        devicesPanel->setMidiRouter(&midi);
        devicesPanel->setDeviceMapsDirectory(deviceMapsDir);
    }
    if (controlMappingHub && keyMappingUi) {
        controlMappingHub->setKeyMappingState(keyMappingUi);
    }
    menuController.addViewModelListener([this](const MenuController::ViewModel& vm) {
        menuHudSnapshot.hasState = vm.hasState;
        menuHudSnapshot.breadcrumbs = vm.breadcrumbs;
        menuHudSnapshot.scope = vm.scope;
        menuHudSnapshot.hotkeys = vm.state.hotkeys;
        if (vm.hasState && vm.state.selectedIndex >= 0 && vm.state.selectedIndex < static_cast<int>(vm.state.entries.size())) {
            const auto& entry = vm.state.entries[static_cast<std::size_t>(vm.state.selectedIndex)];
            menuHudSnapshot.selectedLabel = entry.label.empty() ? entry.id : entry.label;
            menuHudSnapshot.selectedDescription = entry.description;
        } else {
            menuHudSnapshot.selectedLabel.clear();
            menuHudSnapshot.selectedDescription.clear();
        }
        if (!vm.hasState) {
            menuHudSnapshot.scope.clear();
            menuHudSnapshot.hotkeys.clear();
        }
    });

    hotkeyManager.setStoragePath(hotkeyMapPath);
    auto registerHotkey = [&](const std::string& id,
                              const std::string& label,
                              const std::string& description,
                              int defaultKey,
                              MenuController::HotkeyCallback callback) {
        HotkeyManager::Binding binding;
        binding.id = id;
        binding.displayName = label;
        binding.description = description;
        binding.defaultKey = defaultKey;
        binding.callback = std::move(callback);
        hotkeyManager.defineBinding(std::move(binding));
    };

    registerHotkey("menu.console",
                   "Console Visualizer",
                   "Toggle the console grid",
                   MenuController::HOTKEY_MOD_CTRL | 's',
                   [this](MenuController& controller) {
                       return toggleMenuState(controller, consoleState);
                   });
    registerHotkey("menu.consoleHub",
                   "Browser Launcher",
                   "Toggle the Browser + console",
                   MenuController::HOTKEY_MOD_CTRL | 'c',
                   [this](MenuController& controller) {
                       return toggleConsoleAndControlHub(controller);
                   });
    registerHotkey("menu.keymap",
                   "Key Map",
                   "Toggle key mapping menu",
                   MenuController::HOTKEY_MOD_CTRL | 'k',
                   [this](MenuController& controller) {
                       return toggleMenuState(controller, keyMappingUi);
                   });
    registerHotkey("menu.devices",
                   "Device Mapper",
                   "Toggle the device mapper",
                   MenuController::HOTKEY_MOD_CTRL | 'd',
                   [this](MenuController& controller) {
                       return toggleMenuState(controller, devicesPanel);
                   });
    registerHotkey("menu.hudTools",
                   "HUD Tools",
                   "Toggle HUD and open HUD tools",
                   MenuController::HOTKEY_MOD_CTRL | 'h',
                   [this](MenuController& controller) {
                       return toggleHudTools(controller);
                   });
    registerHotkey("overlay.hudToggle",
                   "HUD Visibility (Quick)",
                   "Toggle HUD overlay without opening HUD tools",
                   MenuController::HOTKEY_MOD_SHIFT | 'h',
                   [this](MenuController&) {
                       if (!controlMappingHub) {
                           return false;
                       }
                       controlMappingHub->setHudVisible(!controlMappingHub->hudVisible());
                       return true;
                   });
    registerHotkey("monitor.secondary",
                   "Controller Monitor",
                   "Toggle controller monitor window",
                   MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | 'd',
                   [this](MenuController&) {
                       toggleDualDisplayMode();
                       return true;
                   });
    registerHotkey("display.follow",
                   "Controller Layout Follow",
                   "Toggle follow vs. freeform controller layout",
                   MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | 'f',
                   [this](MenuController&) {
                       toggleSecondaryDisplayFollow();
                       return true;
                   });
    registerHotkey("display.migratePrimary",
                   "Send Overlays to Projector",
                   "Route overlays to primary projector layout",
                   MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | ',',
                   [this](MenuController&) {
                       migrateOverlaysToPrimary();
                       return true;
                   });
    registerHotkey("display.migrateController",
                   "Send Overlays to Controller",
                   "Route overlays to controller monitor layout",
                   MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | '.',
                   [this](MenuController&) {
                       migrateOverlaysToController();
                       return true;
                   });
    registerHotkey("display.layoutResync",
                   "Force Layout Resync",
                   "Rebroadcast HUD layout snapshots",
                   MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | 'r',
                   [this](MenuController&) {
                       forceHudLayoutResync("hotkey");
                       return true;
                   });
    registerHotkey("display.focusOwner",
                   "Controller Focus Owner",
                   "Toggle controller vs. console keyboard focus",
                   MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | OF_KEY_TAB,
                   [this](MenuController&) {
                       requestControllerFocusToggle();
                       return true;
                   });
    registerHotkey("app.fullscreen",
                   "Fullscreen",
                   "Toggle fullscreen",
                   MenuController::HOTKEY_MOD_CTRL | 'f',
                   [](MenuController&) {
                       ofSetFullscreen(!ofGetWindowMode());
                       return true;
                   });
    registerHotkey("app.quit",
                   "Quit",
                   "Exit the application",
                   MenuController::HOTKEY_MOD_CTRL | 'q',
                   [](MenuController&) {
                       ofExit();
                       return true;
                   });

    hotkeyManager.setController(&menuController);
    hotkeyManager.loadFromDisk();

    publishOverlayVisibilityTelemetry("overlay.hud.visible", overlayVisibility_.hud);
    publishOverlayVisibilityTelemetry("overlay.console.visible", overlayVisibility_.console);
    publishOverlayVisibilityTelemetry("overlay.hub.visible", overlayVisibility_.controlHub);
    publishOverlayVisibilityTelemetry("overlay.menus.visible", overlayVisibility_.menus);
    publishDualDisplayTelemetry(persistedDualDisplayMode_);
    publishSecondaryDisplayTelemetry();
    publishSecondaryDisplayFollowTelemetry();
}

void ofApp::applyMenuTextSkin() {
    const float scale = clampMenuTextScale(param_menuTextSize);
    if (std::abs(appliedMenuTextSize_ - scale) <= 0.001f) {
        return;
    }
    param_menuTextSize = scale;
    appliedMenuTextSize_ = scale;

    MenuSkin updatedSkin = MenuSkin::ConsoleHub();
    updatedSkin.metrics.typographyScale = scale;
    updatedSkin.metrics.columnHeaderHeight *= scale;
    updatedSkin.metrics.rowHeight *= scale;
    updatedSkin.metrics.treeRowHeight *= scale;
    updatedSkin.hud.typographyScale = scale;
    updatedSkin.hud.lineHeight *= scale;
    updatedSkin.hud.blockPadding *= scale;
    updatedSkin.hud.badgePadding *= scale;
    updatedSkin.hud.badgeHeight *= scale;
    menuSkin = updatedSkin;

    overlayManager.setHudSkin(menuSkin.hud);
    if (consoleState) {
        consoleState->setMenuSkin(menuSkin);
    }
    if (controlMappingHub) {
        controlMappingHub->setMenuSkin(menuSkin);
    }
}


void ofApp::update() {
    ensureActiveBankValid();
    oscRouter.updateBaseValues();
    midi.update();
    bool collectorPacketThisFrame = false;
    static constexpr uint32_t kCollectorLogSample = 2000;
    collector.update([&](const std::string& address, float value) {
        uint64_t now = static_cast<uint64_t>(ofGetElapsedTimeMillis());
        collectorPacketThisFrame = true;
        collectorPacketsSeen_++;
        lastCollectorPacketMs_ = now;
        if (collectorPacketsSeen_ <= 5 || (collectorPacketsSeen_ % kCollectorLogSample) == 0) {
            ofLogVerbose("CollectorSerial") << "packet#" << collectorPacketsSeen_
                                            << " t=" << now
                                            << " address=" << address
                                            << " value=" << value;
        }
        ingestOscMessage(address, value);
    });
    uint64_t nowMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    updateLocalMicBridge(nowMs);
    if (collector.isConnected()) {
        bool shouldLogWait = false;
        if (collectorPacketsSeen_ == 0) {
            shouldLogWait = (nowMs - lastCollectorWaitLogMs_) > 1000;
        } else if (!collectorPacketThisFrame && lastCollectorPacketMs_ != 0) {
            uint64_t delta = nowMs - lastCollectorPacketMs_;
            if (delta > 1000 && (nowMs - lastCollectorWaitLogMs_) > 500) {
                shouldLogWait = true;
            }
        }
        if (shouldLogWait) {
            if (collectorPacketsSeen_ == 0) {
                ofLogNotice("CollectorSerial") << "waiting for first packet on " << collector.currentPort();
            } else {
                uint64_t delta = nowMs - lastCollectorPacketMs_;
                ofLogNotice("CollectorSerial") << "no packets for " << delta << " ms (total=" << collectorPacketsSeen_ << ")";
            }
            lastCollectorWaitLogMs_ = nowMs;
        }
    }
    const bool midiConnected = midi.isConnected();
    if (!midiTelemetryInitialized_ || midiConnected != lastMidiConnected_) {
        publishHudTelemetrySample("hud.status",
                                  "midi",
                                  midiConnected ? 1.0f : 0.0f,
                                  midiConnected ? midi.connectedPortName() : "disconnected");
        lastMidiConnected_ = midiConnected;
        midiTelemetryInitialized_ = true;
    }
    const bool collectorConnected = collector.isConnected();
    if (!collectorTelemetryInitialized_ || collectorConnected != lastCollectorConnected_) {
        publishHudTelemetrySample("hud.status",
                                  "collector",
                                  collectorConnected ? 1.0f : 0.0f,
                                  collectorConnected ? collector.currentPort() : "disconnected");
        lastCollectorConnected_ = collectorConnected;
        collectorTelemetryInitialized_ = true;
    }
    paramRegistry.evaluateAllModifiers();
    syncActiveFxWithConsoleSlots(false);
    applyMenuTextSkin();
    if (controlMappingHub && param_showHud != controlMappingHub->hudVisible()) {
        controlMappingHub->setHudVisible(param_showHud);
    }
    bool consoleStateDirty = false;
    auto syncOverlayState = [&](bool& cached, bool desired, const std::string& feedId) {
        if (cached == desired) {
            return false;
        }
        cached = desired;
        publishOverlayVisibilityTelemetry(feedId, desired);
        return true;
    };
    const bool routeUiToController = secondaryDisplay_.active && !secondaryDisplay_.followPrimary;
    bool desiredHudVisible = routeUiToController ? false : param_showHud;
    bool desiredConsoleVisible = routeUiToController ? false : param_showConsole;
    bool desiredControlHubVisible = routeUiToController ? false : param_showControlHub;
    bool desiredMenuVisible = routeUiToController ? false : param_showMenus;
    consoleStateDirty |= syncOverlayState(overlayVisibility_.hud, desiredHudVisible, "overlay.hud.visible");
    consoleStateDirty |= syncOverlayState(overlayVisibility_.console, desiredConsoleVisible, "overlay.console.visible");
    consoleStateDirty |= syncOverlayState(overlayVisibility_.controlHub, desiredControlHubVisible, "overlay.hub.visible");
    consoleStateDirty |= syncOverlayState(overlayVisibility_.menus, desiredMenuVisible, "overlay.menus.visible");
    std::string normalizedMode = normalizeDualDisplayMode(param_dualDisplayMode);
    if (normalizedMode != param_dualDisplayMode) {
        param_dualDisplayMode = normalizedMode;
    }
    if (normalizedMode != persistedDualDisplayMode_) {
        persistedDualDisplayMode_ = normalizedMode;
        publishDualDisplayTelemetry(persistedDualDisplayMode_);
        consoleStateDirty = true;
    }
    if (consoleStateDirty) {
        persistConsoleAssignments();
    }
    handleSecondaryDisplayParamChange();
    handleControllerFocusParamChange();
    if (param_layoutResyncRequest) {
        forceHudLayoutResync("param");
        param_layoutResyncRequest = false;
    }
    monitorWindowContentScale();
    manageControllerFocusHolds();
    updateControllerFocusWatchdog();
    updateSecondaryDisplayWatchdog();
    if (controllerFocusDirty_) {
        persistConsoleAssignments();
        controllerFocusDirty_ = false;
    }

    float frameDt = ofGetLastFrameTime();
    uint64_t nowTelemetryMs = ofGetElapsedTimeMillis();
    if (controlMappingHub) {
        controlMappingHub->pollHudLayoutDrift(nowTelemetryMs);
    }
    if (param_layoutWatchdogEnabled) {
        updateLayoutSyncWatchdog(nowTelemetryMs);
    } else if (layoutSyncGuardActive_) {
        leaveLayoutSyncGuard("watchdog-disabled");
        layoutSyncResyncPending_ = false;
    }
    if (nowTelemetryMs - lastHudTimingTelemetryMs_ >= 1000) {
        publishHudTelemetrySample("hud.status", "timing", frameDt, "frameDt");
        lastHudTimingTelemetryMs_ = nowTelemetryMs;
    }
    if (!paused) {
        t += frameDt * speed;
    }

    float clampedSpeed = ofClamp(param_speed, 0.0f, 5.0f);
    if (param_speed != clampedSpeed) param_speed = clampedSpeed;
    speed = clampedSpeed;

    float camTarget = ofClamp(param_camDist, 150.0f, 4000.0f);
    if (param_camDist != camTarget) param_camDist = camTarget;
    camDist = camTarget;

    param_bpm = ofClamp(param_bpm, 40.0f, 240.0f);
    param_masterFx = ofClamp(param_masterFx, 0.0f, 1.0f);

    TextLayerState::instance().syncFontSelection();

    LayerUpdateParams layerParams;
    layerParams.dt = paused ? 0.0f : frameDt * speed;
    layerParams.time = t;
    layerParams.bpm = param_bpm;
    layerParams.speed = speed;

    updateConsoleLayers(layerParams);

    OverlayManager::UpdateParams overlayUpdate;
    overlayUpdate.deltaTime = frameDt;
    overlayUpdate.app = this;
    overlayManager.update(overlayUpdate);
    if (controlHubEventBridge) {
        controlHubEventBridge->update(static_cast<uint64_t>(ofGetElapsedTimeMillis()));
    }
}


void ofApp::draw() {
    ofBackground(0);

    glm::vec3 eye {
        camDist * cosf(camPhi) * sinf(camTheta),
        camDist * sinf(camPhi),
        camDist * cosf(camPhi) * cosf(camTheta)
    };
    cam.setPosition(eye);
    cam.lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    float beatPhase = (param_bpm / 60.0f) * t;
    glm::ivec2 viewport{ ofGetWidth(), ofGetHeight() };
    ThreeBandLayout layout = threeBandLayout.layoutForSize(static_cast<float>(viewport.x),
                                                           static_cast<float>(viewport.y));
    drawConsole(viewport, beatPhase);

    OverlayManager::DrawParams drawParams;
    drawParams.bounds = ofRectangle(0.0f, 0.0f, static_cast<float>(viewport.x), static_cast<float>(viewport.y));
    drawParams.app = this;
    drawParams.layout = layout;
    drawParams.useThreeBandLayout = true;
    if (overlayVisibility_.hud) {
        overlayManager.draw(drawParams);
    }
    drawMenuPanels(layout,
                   overlayVisibility_.console,
                   overlayVisibility_.controlHub,
                   overlayVisibility_.menus);
}

void ofApp::drawMenuPanels(const ThreeBandLayout& layout,
                           bool showConsole,
                           bool showControlHub,
                           bool showMenus) {
    if (controlMappingHub && showControlHub) {
        controlMappingHub->setLayoutBand(layout.workbench.bounds);
        controlMappingHub->draw();
        controlMappingHub->clearLayoutBand();
    }

    if (consoleState && showConsole && menuController.contains(consoleState->id())) {
        consoleState->setLayoutBand(layout.console.bounds);
        consoleState->draw();
        consoleState->clearLayoutBand();
    }

    if (showMenus && devicesPanel) {
        devicesPanel->draw();
    }

    if (showMenus && hudLayoutEditor) {
        hudLayoutEditor->draw();
    }

    if (showMenus && keyMappingUi) {
        keyMappingUi->draw();
    }

    if (showMenus && assetBrowser) {
        assetBrowser->draw();
    }
}


std::string ofApp::composeHudControls() const {
    struct ControlAction {
        std::string keys;
        std::string description;
    };
    struct ControlLine {
        std::string prefix;
        std::vector<ControlAction> actions;
    };

    ControlLine navigationLine;
    navigationLine.actions = {
        { "Mouse drag", "orbit" },
        { "Up/Down", "zoom" },
        { "Ctrl+F", "fullscreen" }
    };

    ControlLine basicsLine;
    basicsLine.actions = {
        { "Space", "pause" },
        { "+/-", "speed" },
        { "H", "HUD menu" },
        { "K", "key map" },
        { "Ctrl+S", "console" },
        { "P", "parameters" },
        { "R", "reconnect" }
    };
    std::vector<ControlAction> layerSpecificHints;
    if (gameOfLifeLayer) {
        layerSpecificHints.push_back({ "Ctrl+N", "reseed GoL" });
        layerSpecificHints.push_back({ "Ctrl+O", "pause" });
    }

    ControlLine workflowLine;
    workflowLine.prefix = "Console workflow: ";
    workflowLine.actions = {
        { "Ctrl+S", "focus grid" },
        { "Enter", "load slot" },
        { "Ctrl+1..8", "open picker" },
        { "Ctrl+U", "unload focused slot" }
    };

    ControlLine controllerLine;
    controllerLine.prefix = controllerFocusStatusBadge();
    controllerLine.actions = {
        { "Ctrl+Shift+D", "toggle monitor" },
        { "Ctrl+Shift+F", "follow/freeform" },
        { "Ctrl+Shift+Tab", "focus toggle" }
    };

    ControlLine modifiersLine;
    modifiersLine.actions = {
        { "Ctrl+G", "cycle grid density" },
        { "[ [ ] ]", "geodesic subdivision (now " +
            ofToString(geodesicLayer ? geodesicLayer->subdivisions() : 0) + ")" }
    };

    auto writeActions = [](std::ostringstream& stream, const std::vector<ControlAction>& actions) {
        for (std::size_t i = 0; i < actions.size(); ++i) {
            if (i > 0) {
                stream << "   ";
            }
            stream << "[" << actions[i].keys << "] " << actions[i].description;
        }
    };

    std::ostringstream out;
    writeActions(out, navigationLine.actions);
    out << "\n";
    writeActions(out, basicsLine.actions);
    if (!layerSpecificHints.empty()) {
        out << "   ";
        for (std::size_t i = 0; i < layerSpecificHints.size(); ++i) {
            if (i > 0) {
                out << " / ";
            }
            out << "[" << layerSpecificHints[i].keys << "] " << layerSpecificHints[i].description;
        }
    }
    out << "\n";
    if (!workflowLine.prefix.empty()) {
        out << workflowLine.prefix;
    }
    writeActions(out, workflowLine.actions);
    out << "\n";
    if (!controllerLine.prefix.empty()) {
        out << controllerLine.prefix << "  ";
    }
    writeActions(out, controllerLine.actions);
    out << "\n";
    writeActions(out, modifiersLine.actions);
    out << "\n";

    ofJson feed = ofJson::object();
    ofJson lines = ofJson::array();
    auto appendLineJson = [&lines](const ControlLine& line,
                                   const std::vector<ControlAction>& extra = {}) {
        if (line.actions.empty() && extra.empty()) {
            return;
        }
        ofJson entry;
        if (!line.prefix.empty()) {
            entry["prefix"] = line.prefix;
        }
        ofJson actions = ofJson::array();
        auto serializeActions = [&actions](const std::vector<ControlAction>& src) {
            for (const auto& action : src) {
                ofJson jsonAction;
                jsonAction["keys"] = action.keys;
                jsonAction["description"] = action.description;
                actions.push_back(std::move(jsonAction));
            }
        };
        serializeActions(line.actions);
        if (!extra.empty()) {
            serializeActions(extra);
        }
        entry["actions"] = std::move(actions);
        lines.push_back(std::move(entry));
    };

    appendLineJson(navigationLine);
    appendLineJson(basicsLine, layerSpecificHints);
    appendLineJson(workflowLine);
    appendLineJson(controllerLine);
    appendLineJson(modifiersLine);
    feed["lines"] = std::move(lines);
    hudFeedRegistry.publish("hud.controls", std::move(feed));

    return out.str();
}

std::string ofApp::composeHudLayerSummary() const {
    std::ostringstream out;
    if (gridLayer) {
        out << "\nGrid segments: " << ofToString(static_cast<int>(*gridLayer->segmentsParamPtr()))
            << "   faces: " << ofToString(*gridLayer->faceOpacityParamPtr(), 2)
            << "   visible: " << (gridLayer->isEnabled() ? "yes" : "no");
    }
    if (geodesicLayer) {
        out << "\nSphere spin: " << ofToString(*geodesicLayer->spinParamPtr(), 1)
            << "   hover: " << ofToString(*geodesicLayer->hoverParamPtr(), 1)
            << "   baseY: " << ofToString(*geodesicLayer->baseHeightParamPtr(), 1)
            << "   orbitR: " << ofToString(*geodesicLayer->orbitRadiusParamPtr(), 1)
            << "   orbitSpd: " << ofToString(*geodesicLayer->orbitSpeedParamPtr(), 1)
            << "   radius: " << ofToString(*geodesicLayer->radiusParamPtr(), 1)
            << "   faces: " << ofToString(*geodesicLayer->faceOpacityParamPtr(), 2)
            << "   visible: " << (geodesicLayer->isEnabled() ? "yes" : "no");
    }
    return out.str();
}

std::string ofApp::composeHudLayerDetails() const {
    std::ostringstream out;
    out << "\nConsole slots:";
    if (consoleSlots.empty()) {
        out << " (none)";
        return out.str();
    }
    for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
        const auto& slot = consoleSlots[i];
        out << "\n  [" << (i + 1) << "] ";
        if (slot.assetId.empty()) {
            out << "(empty)";
            continue;
        }
        const auto* entry = layerLibrary.find(slot.assetId);
        std::string label = slot.label;
        if (label.empty()) {
            label = entry ? entry->label : slot.assetId;
        }
        out << label << (slot.active ? " *" : " (off)");
        if (slot.layer) {
            const Layer* base = slot.layer.get();
            if (auto* webcam = dynamic_cast<const VideoGrabberLayer*>(base)) {
                out << "  [" << webcam->currentDeviceLabel() << "]";
                out << " gain=" << ofToString(webcam->gain(), 2);
                if (webcam->mirror()) out << " mirror";
            } else if (auto* clip = dynamic_cast<const VideoClipLayer*>(base)) {
                out << "  [" << clip->currentClipLabel() << "]";
                out << " gain=" << ofToString(clip->gain(), 2);
                if (clip->mirror()) out << " mirror";
                if (!clip->loop()) out << " loop=off";
            } else if (auto* perlin = dynamic_cast<const PerlinNoiseLayer*>(base)) {
                out << "  scale=" << ofToString(*perlin->scaleParamPtr(), 2);
                out << " texZoom=" << ofToString(*perlin->texelZoomParamPtr(), 2);
                out << " oct=" << ofToString(static_cast<int>(std::round(*perlin->octavesParamPtr())));
                out << " palette=" << ofToString(static_cast<int>(std::round(*perlin->paletteIndexParamPtr())));
            } else if (auto* golLayer = dynamic_cast<const GameOfLifeLayer*>(base)) {
                int preset = std::max(0, std::min(golLayer->presetCount() - 1,
                                                  static_cast<int>(std::round(*golLayer->presetParamPtr()))));
                out << (golLayer->isPaused() ? " paused" : " running");
                out << " preset=" << ofToString(preset);
                out << " dens=" << ofToString(*golLayer->densityParamPtr(), 2);
                out << " fade=" << ofToString(static_cast<int>(std::round(*golLayer->fadeFramesParamPtr())));
                out << " bpmSync=" << (*golLayer->bpmSyncParamPtr() ? "on" : "off");
                out << " aAlpha=" << ofToString(*golLayer->aliveAlphaParamPtr(), 2);
                out << " dAlpha=" << ofToString(*golLayer->deadAlphaParamPtr(), 2);
            }
            out << " opacity=" << ofToString(slot.opacity, 2);
        } else if (isFxType(slot.type)) {
            out << " (FX)";
        } else if (isUiOverlayType(slot.type)) {
            out << " (overlay)";
        }
    }

    auto routeName = [](float routeVal) {
        int r = static_cast<int>(std::round(ofClamp(routeVal, 0.0f, 2.0f)));
        switch (r) {
        case 1: return std::string("Console");
        case 2: return std::string("Global");
        default: return std::string("Off");
        }
    };
    out << "\nEffects:";
    out << " Dither=" << routeName(postEffects.ditherRouteValue())
        << " (cell " << ofToString(static_cast<int>(postEffects.ditherCellSizeValue())) << ")";
    out << " ASCII=" << routeName(postEffects.asciiRouteValue())
        << " (mode " << ofToString(static_cast<int>(std::round(postEffects.asciiColorModeValue())))
        << ", block " << ofToString(static_cast<int>(postEffects.asciiBlockSizeValue())) << ")";
    out << " CRT=" << routeName(postEffects.crtRouteValue())
        << " (scan " << ofToString(postEffects.crtScanlineValue(), 2)
        << " vig " << ofToString(postEffects.crtVignetteValue(), 2)
        << " bleed " << ofToString(postEffects.crtBleedValue(), 2) << ")";

    return out.str();
}

std::string ofApp::composeHudLayers() const {
    std::string summary = composeHudLayerSummary();
    std::string details = composeHudLayerDetails();
    std::string combined;
    combined.reserve(summary.size() + details.size());
    combined += summary;
    combined += details;

    ofJson feed = ofJson::object();
    ofJson summaryJson = ofJson::object();
    if (gridLayer) {
        ofJson grid;
        grid["segments"] = static_cast<int>(*gridLayer->segmentsParamPtr());
        grid["faceOpacity"] = *gridLayer->faceOpacityParamPtr();
        grid["visible"] = gridLayer->isEnabled();
        summaryJson["grid"] = std::move(grid);
    }
    if (geodesicLayer) {
        ofJson sphere;
        sphere["spin"] = *geodesicLayer->spinParamPtr();
        sphere["hover"] = *geodesicLayer->hoverParamPtr();
        sphere["baseHeight"] = *geodesicLayer->baseHeightParamPtr();
        sphere["orbitRadius"] = *geodesicLayer->orbitRadiusParamPtr();
        sphere["orbitSpeed"] = *geodesicLayer->orbitSpeedParamPtr();
        sphere["radius"] = *geodesicLayer->radiusParamPtr();
        sphere["faceOpacity"] = *geodesicLayer->faceOpacityParamPtr();
        sphere["visible"] = geodesicLayer->isEnabled();
        summaryJson["geodesic"] = std::move(sphere);
    }
    feed["summary"] = std::move(summaryJson);

    auto routeName = [](float routeVal) {
        int r = static_cast<int>(std::round(ofClamp(routeVal, 0.0f, 2.0f)));
        switch (r) {
        case 1: return std::string("Console");
        case 2: return std::string("Global");
        default: return std::string("Off");
        }
    };

    ofJson slotsJson = ofJson::array();
    for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
        const auto& slot = consoleSlots[i];
        ofJson slotJson;
        slotJson["index"] = static_cast<int>(i) + 1;
        slotJson["assetId"] = slot.assetId;
        const auto* entry = layerLibrary.find(slot.assetId);
        std::string label = slot.label;
        if (label.empty()) {
            label = entry ? entry->label : slot.assetId;
        }
        slotJson["label"] = label;
        slotJson["active"] = slot.active;
        slotJson["opacity"] = slot.opacity;
        slotJson["type"] = slot.type;
        slotJson["empty"] = slot.assetId.empty();
        slotJson["hasLayer"] = static_cast<bool>(slot.layer);
        if (slot.coverage.defined) {
            ofJson coverage = ofJson::object();
            if (!slot.coverage.mode.empty()) {
                coverage["mode"] = slot.coverage.mode;
            }
            coverage["columns"] = std::max(0, slot.coverage.columns);
            slotJson["coverage"] = std::move(coverage);
        }
        if (slot.layer) {
            const Layer* base = slot.layer.get();
            ofJson metadata;
            std::string module;
            if (auto* webcam = dynamic_cast<const VideoGrabberLayer*>(base)) {
                module = "video.grabber";
                metadata["deviceLabel"] = webcam->currentDeviceLabel();
                metadata["gain"] = webcam->gain();
                metadata["mirror"] = webcam->mirror();
            } else if (auto* clip = dynamic_cast<const VideoClipLayer*>(base)) {
                module = "video.clip";
                metadata["clipLabel"] = clip->currentClipLabel();
                metadata["gain"] = clip->gain();
                metadata["mirror"] = clip->mirror();
                metadata["loop"] = clip->loop();
            } else if (auto* perlin = dynamic_cast<const PerlinNoiseLayer*>(base)) {
                module = "perlin";
                metadata["scale"] = *perlin->scaleParamPtr();
                metadata["texelZoom"] = *perlin->texelZoomParamPtr();
                metadata["octaves"] = static_cast<int>(std::round(*perlin->octavesParamPtr()));
                metadata["paletteIndex"] = static_cast<int>(std::round(*perlin->paletteIndexParamPtr()));
            } else if (auto* golLayer = dynamic_cast<const GameOfLifeLayer*>(base)) {
                module = "gameOfLife";
                int preset = std::max(0, std::min(golLayer->presetCount() - 1,
                                                  static_cast<int>(std::round(*golLayer->presetParamPtr()))));
                metadata["paused"] = golLayer->isPaused();
                metadata["preset"] = preset;
                metadata["density"] = *golLayer->densityParamPtr();
                metadata["fadeFrames"] = static_cast<int>(std::round(*golLayer->fadeFramesParamPtr()));
                metadata["bpmSync"] = *golLayer->bpmSyncParamPtr();
                metadata["bpmMultiplier"] = *golLayer->bpmMultiplierParamPtr();
                metadata["reseedQuantizeBeats"] = static_cast<int>(std::round(*golLayer->reseedQuantizeBeatsParamPtr()));
                metadata["autoReseed"] = *golLayer->autoReseedParamPtr();
                metadata["autoReseedEveryBeats"] = static_cast<int>(std::round(*golLayer->autoReseedEveryBeatsParamPtr()));
                metadata["aliveAlpha"] = *golLayer->aliveAlphaParamPtr();
                metadata["deadAlpha"] = *golLayer->deadAlphaParamPtr();
            }
            if (!module.empty()) {
                slotJson["module"] = module;
            }
            if (!metadata.empty()) {
                slotJson["metadata"] = std::move(metadata);
            }
        } else if (isFxType(slot.type)) {
            slotJson["module"] = "fx";
        } else if (isUiOverlayType(slot.type)) {
            slotJson["module"] = "overlay";
        }
        slotsJson.push_back(std::move(slotJson));
    }
    feed["slots"] = std::move(slotsJson);

    ofJson effects = ofJson::object();
    ofJson dither;
    dither["route"] = routeName(postEffects.ditherRouteValue());
    dither["routeValue"] = postEffects.ditherRouteValue();
    dither["cellSize"] = static_cast<int>(postEffects.ditherCellSizeValue());
    effects["dither"] = std::move(dither);

    ofJson asciiFx;
    asciiFx["route"] = routeName(postEffects.asciiRouteValue());
    asciiFx["routeValue"] = postEffects.asciiRouteValue();
    asciiFx["colorMode"] = static_cast<int>(std::round(postEffects.asciiColorModeValue()));
    asciiFx["blockSize"] = static_cast<int>(postEffects.asciiBlockSizeValue());
    asciiFx["characterSet"] = static_cast<int>(std::round(postEffects.asciiCharacterSetValue()));
    asciiFx["aspectMode"] = postEffects.asciiAspectModeValue();
    asciiFx["padding"] = postEffects.asciiPaddingValue();
    asciiFx["gamma"] = postEffects.asciiGammaValue();
    asciiFx["jitter"] = postEffects.asciiJitterValue();
    effects["ascii"] = std::move(asciiFx);

    ofJson crt;
    crt["route"] = routeName(postEffects.crtRouteValue());
    crt["routeValue"] = postEffects.crtRouteValue();
    crt["scanline"] = postEffects.crtScanlineValue();
    crt["vignette"] = postEffects.crtVignetteValue();
    crt["bleed"] = postEffects.crtBleedValue();
    effects["crt"] = std::move(crt);

    feed["effects"] = std::move(effects);
    hudFeedRegistry.publish("hud.layers", std::move(feed));

    return combined;
}

std::string ofApp::composeHudStatus() const {
    std::ostringstream hud;
    int activeSlots = 0;
    int assignedSlots = 0;
    for (const auto& slot : consoleSlots) {
        if (!slot.assetId.empty()) {
            ++assignedSlots;
            if (slot.active) {
                ++activeSlots;
            }
        }
    }
    hud << "\nConsole slots: " << activeSlots << " active / " << std::max<std::size_t>(consoleSlots.size(), 1);
    hud << "   Assigned: " << assignedSlots;

    ofJson feed = ofJson::object();
    ofJson slotJson;
    slotJson["active"] = activeSlots;
    slotJson["assigned"] = assignedSlots;
    slotJson["capacity"] = static_cast<int>(consoleSlots.size());
    feed["slots"] = std::move(slotJson);

    ofJson controllerJson;
    controllerJson["enabled"] = secondaryDisplay_.enabled;
    controllerJson["active"] = secondaryDisplay_.active;
    controllerJson["follow"] = secondaryDisplay_.followPrimary;
    controllerJson["focusOwner"] = controllerFocus_.preferConsole ? "console" : "controller";
    controllerJson["needsAttention"] = controllerFocus_.needsAttention;
    feed["controller"] = std::move(controllerJson);

    hud << "\n" << secondaryDisplayLabel();
    hud << "  Focus: " << (controllerFocus_.preferConsole ? "Console" : "Controller");
    if (controllerFocus_.needsAttention) {
        hud << "  [ATTN]";
    }

    auto routeName = [](float routeValue) -> std::string {
        int rv = static_cast<int>(std::round(ofClamp(routeValue, 0.0f, 2.0f)));
        switch (rv) {
        case 1: return "Console";
        case 2: return "Global";
        default: return "Off";
        }
    };
    auto captureRoute = [&](float routeValue, const std::function<void(ofJson&)>& extras) {
        ofJson entry;
        entry["state"] = routeName(routeValue);
        entry["value"] = routeValue;
        if (extras) {
            extras(entry);
        }
        return entry;
    };
    ofJson fxRoutes = ofJson::object();
    fxRoutes["dither"] = captureRoute(postEffects.ditherRouteValue(), [&](ofJson& entry) {
        entry["cellSize"] = static_cast<int>(postEffects.ditherCellSizeValue());
    });
    fxRoutes["ascii"] = captureRoute(postEffects.asciiRouteValue(), [&](ofJson& entry) {
        entry["colorMode"] = static_cast<int>(std::round(postEffects.asciiColorModeValue()));
        entry["blockSize"] = static_cast<int>(postEffects.asciiBlockSizeValue());
        entry["characterSet"] = static_cast<int>(std::round(postEffects.asciiCharacterSetValue()));
        entry["aspectMode"] = postEffects.asciiAspectModeValue();
        entry["padding"] = postEffects.asciiPaddingValue();
        entry["gamma"] = postEffects.asciiGammaValue();
        entry["jitter"] = postEffects.asciiJitterValue();
    });
    fxRoutes["crt"] = captureRoute(postEffects.crtRouteValue(), [&](ofJson& entry) {
        entry["scanline"] = postEffects.crtScanlineValue();
        entry["vignette"] = postEffects.crtVignetteValue();
        entry["bleed"] = postEffects.crtBleedValue();
    });
    feed["fxRoutes"] = fxRoutes;

    hud << "\nFX routes: Dither=" << routeName(postEffects.ditherRouteValue())
        << "  ASCII=" << routeName(postEffects.asciiRouteValue())
        << "  CRT=" << routeName(postEffects.crtRouteValue());

    hud << "\nMIDI: ";
    auto midiSample = hudTelemetryOverrideSample("hud.status", "midi", 5000);
    bool midiConnected = midiSample ? midiSample->value >= 0.5f : midi.isConnected();
    std::string midiLabel = midiSample ? midiSample->detail : midi.connectedPortName();
    if (midiConnected) {
        if (midiLabel.empty()) {
            hud << "connected";
        } else {
            hud << "connected (" << midiLabel << ")";
        }
    } else {
        hud << "not connected";
    }
    ofJson midiConn;
    midiConn["connected"] = midiConnected;
    midiConn["label"] = midiLabel;
    midiConn["overridden"] = static_cast<bool>(midiSample);
    if (midiSample) {
        midiConn["timestampMs"] = midiSample->timestampMs;
    }

    hud << "\nCollector: ";
    auto collectorSample = hudTelemetryOverrideSample("hud.status", "collector", 5000);
    bool collectorConnected = collectorSample ? collectorSample->value >= 0.5f : collector.isConnected();
    std::string collectorLabel = collectorSample ? collectorSample->detail : collector.currentPort();
    if (collectorConnected) {
        if (collectorLabel.empty()) {
            hud << "connected";
        } else {
            hud << "connected (" << collectorLabel << ")";
        }
    } else {
        hud << "searching...";
    }
    ofJson collectorConn;
    collectorConn["connected"] = collectorConnected;
    collectorConn["label"] = collectorLabel;
    collectorConn["overridden"] = static_cast<bool>(collectorSample);
    if (collectorSample) {
        collectorConn["timestampMs"] = collectorSample->timestampMs;
    }

    ofJson connections = ofJson::object();
    connections["midi"] = midiConn;
    connections["collector"] = collectorConn;
    feed["connections"] = connections;

    const std::string bankLabel = activeMidiBank.empty() ? std::string("global") : activeMidiBank;
    hud << "\nActive MIDI bank: " << bankLabel;
    feed["activeBank"] = bankLabel;
    auto takeoverStates = midi.pendingTakeovers();
    if (!takeoverStates.empty()) {
        hud << "\nSoft takeover pending:";
        for (const auto& takeover : takeoverStates) {
            const std::string takeoverBank = takeover.bankId.empty() ? std::string("global") : takeover.bankId;
            hud << "\n  [" << takeoverBank << "] " << takeover.controlId << " delta=" << ofToString(takeover.delta, 3);
        }
    }
    ofJson takeoverJson = ofJson::array();
    for (const auto& takeover : takeoverStates) {
        ofJson entry;
        entry["bank"] = takeover.bankId.empty() ? "global" : takeover.bankId;
        entry["controlId"] = takeover.controlId;
        entry["targetId"] = takeover.targetId;
        entry["delta"] = takeover.delta;
        entry["hardwareValue"] = takeover.hardwareValue;
        entry["catchValue"] = takeover.catchValue;
        entry["pendingSinceMs"] = takeover.pendingSinceMs;
        takeoverJson.push_back(std::move(entry));
    }
    if (!takeoverJson.empty()) {
        feed["takeovers"] = takeoverJson;
    }

    const auto& oscSources = midi.getOscSources();
    hud << "\nOSC sources learned: " << ofToString(static_cast<int>(oscSources.size()));
    ofJson oscJson = ofJson::array();
    for (const auto& source : oscSources) {
        ofJson entry;
        entry["address"] = source.address;
        entry["lastValue"] = source.lastValue;
        entry["lastSeenMs"] = source.lastSeenMs;
        entry["seen"] = source.seen;
        oscJson.push_back(std::move(entry));
    }
    feed["oscSources"] = oscJson;

    auto layoutSample = hudTelemetryOverrideSample("hud.status", "layout_drift", 0);
    bool layoutDriftActive = layoutSample && layoutSample->value >= 0.5f;
    ofJson layoutJson;
    layoutJson["drift"] = layoutDriftActive;
    if (layoutSample && !layoutSample->detail.empty()) {
        layoutJson["detail"] = layoutSample->detail;
    }
    feed["layout"] = std::move(layoutJson);
    if (layoutDriftActive) {
        std::string detailText = layoutSample->detail;
        if (detailText.empty()) {
            detailText = "controller layout";
        }
        hud << "\nLayout drift: " << detailText;
    }

    hudFeedRegistry.publish("hud.status", std::move(feed));
    return hud.str();
}

std::string ofApp::composeHudSensors() const {
    uint64_t nowMs = ofGetElapsedTimeMillis();
    auto indicator = [&](uint64_t lastMs) -> std::string {
        if (!lastMs) {
            return "[ ]";
        }
        uint64_t age = nowMs >= lastMs ? (nowMs - lastMs) : 0;
        if (age <= kHudFreshMs) {
            return "[*]";
        }
        if (age <= kHudStaleMs) {
            return "[~]";
        }
        return "[ ]";
    };
    auto categoryLine = [&](const std::string& label, const HudCategoryActivity& cat) {
        std::string line = label + " " + indicator(cat.lastSeenMs);
        if (cat.lastSeenMs) {
            if (cat.hasValue) {
                line += " " + ofToString(cat.lastValue, 3);
            }
            if (!cat.lastMetric.empty()) {
                line += " (" + cat.lastMetric + ")";
            }
            uint64_t age = nowMs >= cat.lastSeenMs ? (nowMs - cat.lastSeenMs) : 0;
            line += "  (" + ofToString(age / 1000.0f, 1) + "s ago)";
        } else {
            line += " --";
        }
        return line;
    };

    std::ostringstream hud;
    hud << "\n\nSensor HUD:";
    hud << "\n  Cyberdeck TLVs " << indicator(hudDeckActivity.lastAnyMs);
    if (hudDeckActivity.lastAnyMs) {
        hud << "\n    HR " << categoryLine("HR", hudDeckActivity.hr);
        hud << "\n    IMU " << categoryLine("IMU", hudDeckActivity.imu);
        hud << "\n    AUX " << categoryLine("AUX", hudDeckActivity.aux);
    }
    hud << "\n  Host Sensors " << indicator(hudHostActivity.lastAnyMs);
    if (hudHostActivity.lastAnyMs) {
        hud << "\n    MIC " << categoryLine("MIC", hudHostActivity.mic);
        hud << "\n    AUX " << categoryLine("AUX", hudHostActivity.aux);
    }
    hud << "\n  Matrix TLVs " << indicator(hudMatrixActivity.lastAnyMs);
    if (hudMatrixActivity.lastAnyMs) {
        hud << "\n    MIC " << categoryLine("MIC", hudMatrixActivity.mic);
        hud << "\n    BIO " << categoryLine("BIO", hudMatrixActivity.bio);
        hud << "\n    IMU " << categoryLine("IMU", hudMatrixActivity.imu);
        hud << "\n    AUX " << categoryLine("AUX", hudMatrixActivity.aux);
    }

    hud << "\n\nRecent OSC:";
    for (const auto& entry : oscHistory) {
        hud << "\n  " << entry.first << " -> " << ofToString(entry.second, 4);
    }

    auto categoryJson = [&](const HudCategoryActivity& cat) {
        ofJson node;
        node["lastSeenMs"] = cat.lastSeenMs;
        node["lastValue"] = cat.lastValue;
        node["lastMetric"] = cat.lastMetric;
        node["hasValue"] = cat.hasValue;
        node["ageMs"] = cat.lastSeenMs && nowMs >= cat.lastSeenMs ? (nowMs - cat.lastSeenMs) : 0;
        node["indicator"] = indicator(cat.lastSeenMs);
        return node;
    };
    auto hubJson = [&](const HudDeckActivity& hub) {
        ofJson node;
        node["lastAnyMs"] = hub.lastAnyMs;
        node["indicator"] = indicator(hub.lastAnyMs);
        ofJson categories = ofJson::object();
        categories["hr"] = categoryJson(hub.hr);
        categories["imu"] = categoryJson(hub.imu);
        categories["aux"] = categoryJson(hub.aux);
        node["categories"] = std::move(categories);
        return node;
    };
    auto hostJson = [&](const HudHostActivity& host) {
        ofJson node;
        node["lastAnyMs"] = host.lastAnyMs;
        node["indicator"] = indicator(host.lastAnyMs);
        ofJson categories = ofJson::object();
        categories["mic"] = categoryJson(host.mic);
        categories["aux"] = categoryJson(host.aux);
        node["categories"] = std::move(categories);
        return node;
    };
    auto matrixJson = [&](const HudMatrixActivity& hub) {
        ofJson node;
        node["lastAnyMs"] = hub.lastAnyMs;
        node["indicator"] = indicator(hub.lastAnyMs);
        ofJson categories = ofJson::object();
        categories["mic"] = categoryJson(hub.mic);
        categories["bio"] = categoryJson(hub.bio);
        categories["imu"] = categoryJson(hub.imu);
        categories["aux"] = categoryJson(hub.aux);
        node["categories"] = std::move(categories);
        return node;
    };

    ofJson feed = ofJson::object();
    feed["timestampMs"] = nowMs;
    feed["deck"] = hubJson(hudDeckActivity);
    feed["host"] = hostJson(hudHostActivity);
    feed["matrix"] = matrixJson(hudMatrixActivity);
    ofJson osc = ofJson::array();
    for (const auto& entry : oscHistory) {
        ofJson sample;
        sample["address"] = entry.first;
        sample["value"] = entry.second;
        osc.push_back(std::move(sample));
    }
    feed["oscHistory"] = std::move(osc);
    hudFeedRegistry.publish("hud.sensors", std::move(feed));

    return hud.str();
}

std::string ofApp::composeHudMenu() const {
    std::ostringstream hud;
    ofJson feed = ofJson::object();
    if (menuHudSnapshot.hasState) {
        hud << "\n\nMenu: ";
        ofJson breadcrumbs = ofJson::array();
        for (std::size_t i = 0; i < menuHudSnapshot.breadcrumbs.size(); ++i) {
            hud << menuHudSnapshot.breadcrumbs[i];
            breadcrumbs.push_back(menuHudSnapshot.breadcrumbs[i]);
            if (i + 1 < menuHudSnapshot.breadcrumbs.size()) {
                hud << " / ";
            }
        }
        feed["breadcrumbs"] = std::move(breadcrumbs);
        if (!menuHudSnapshot.scope.empty()) {
            hud << "\n  Scope: " << menuHudSnapshot.scope;
            feed["scope"] = menuHudSnapshot.scope;
        }
        if (!menuHudSnapshot.hotkeys.empty()) {
            hud << "\n  Keys:";
            ofJson hotkeys = ofJson::array();
            for (const auto& hint : menuHudSnapshot.hotkeys) {
                hud << " [" << hint.label << "] " << hint.description;
                ofJson entry;
                entry["label"] = hint.label;
                entry["description"] = hint.description;
                hotkeys.push_back(std::move(entry));
            }
            feed["hotkeys"] = std::move(hotkeys);
        }
        if (!menuHudSnapshot.selectedLabel.empty()) {
            hud << "\n  > " << menuHudSnapshot.selectedLabel;
            feed["selected"] = {
                { "label", menuHudSnapshot.selectedLabel },
                { "description", menuHudSnapshot.selectedDescription }
            };
            if (!menuHudSnapshot.selectedDescription.empty()) {
                hud << " :: " << menuHudSnapshot.selectedDescription;
            }
        } else {
            feed["selected"] = ofJson::object();
        }
        feed["hasState"] = true;
    } else {
        feed["hasState"] = false;
    }

    const auto& hotkeyConflicts = menuController.hotkeyConflicts();
    if (!hotkeyConflicts.empty()) {
        hud << "\nHotkey conflicts:";
        ofJson conflicts = ofJson::array();
        for (const auto& conflict : hotkeyConflicts) {
            hud << "\n  " << conflict;
            conflicts.push_back(conflict);
        }
        feed["hotkeyConflicts"] = std::move(conflicts);
    }
    feed["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    hudFeedRegistry.publish("hud.menu", std::move(feed));
    return hud.str();
}

std::optional<HudFeedRegistry::FeedEntry> ofApp::latestHudFeed(const std::string& widgetId) const {
    return hudFeedRegistry.latest(widgetId);
}

void ofApp::ensureConsoleLayerViewports(glm::ivec2 viewport) {
    if (!compositeFbo.isAllocated() || compositeWidth != viewport.x || compositeHeight != viewport.y) {
        ofFbo::Settings settings;
        settings.width = viewport.x;
        settings.height = viewport.y;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.internalformat = GL_RGBA;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.minFilter = GL_LINEAR;
        settings.maxFilter = GL_LINEAR;
        settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
        settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
        compositeFbo.allocate(settings);
        compositeWidth = viewport.x;
        compositeHeight = viewport.y;

        for (auto& slot : consoleSlots) {
            if (slot.layer) {
                slot.layer->onWindowResized(viewport.x, viewport.y);
            }
        }
    }
}

void ofApp::ensureSlotFbo(ofFbo& fbo, glm::ivec2 viewport) {
    if (viewport.x <= 0 || viewport.y <= 0) {
        return;
    }
    if (fbo.isAllocated() && static_cast<int>(fbo.getWidth()) == viewport.x &&
        static_cast<int>(fbo.getHeight()) == viewport.y) {
        return;
    }

    ofFbo::Settings settings;
    settings.width = viewport.x;
    settings.height = viewport.y;
    settings.useDepth = false;
    settings.useStencil = false;
    settings.internalformat = GL_RGBA;
    settings.textureTarget = GL_TEXTURE_2D;
    settings.minFilter = GL_LINEAR;
    settings.maxFilter = GL_LINEAR;
    settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
    fbo.allocate(settings);
    fbo.begin();
    ofClear(0, 0, 0, 0);
    fbo.end();
}

void ofApp::drawConsole(glm::ivec2 viewport, float beatPhase) {
    ensureConsoleLayerViewports(viewport);
    postEffects.resize(viewport.x, viewport.y);
    const int layerCount = static_cast<int>(consoleSlots.size());
    postEffects.prepareLayerBuffers(layerCount, viewport.x, viewport.y);

    auto clearFbo = [](ofFbo& fbo) {
        if (!fbo.isAllocated()) return;
        fbo.begin();
        ofClear(0, 0, 0, 0);
        fbo.end();
    };

    auto copySlotToLayerBuffer = [&](int slotIndex) {
        ofFbo* buffer = postEffects.layerBufferPtr(slotIndex);
        if (!buffer) return;
        buffer->begin();
        ofClear(0, 0, 0, 0);
        if (slotIndex >= 0 && slotIndex < static_cast<int>(consoleSlots.size())) {
            const auto& srcSlot = consoleSlots[slotIndex];
            if (srcSlot.layerFbo.isAllocated()) {
                srcSlot.layerFbo.draw(0, 0, viewport.x, viewport.y);
            }
        }
        buffer->end();
    };

    auto routeStateForSlot = [&](const ConsoleSlot& slot) -> int {
        const float* routePtr = fxRouteParamForType(slot.type);
        float raw = routePtr ? *routePtr : 0.0f;
        return static_cast<int>(std::round(ofClamp(raw, 0.0f, 2.0f)));
    };

    ofPushStyle();
    for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
        auto& slot = consoleSlots[i];
        if (!slot.active) {
            clearFbo(slot.layerFbo);
            copySlotToLayerBuffer(static_cast<int>(i));
            continue;
        }

        if (isFxType(slot.type)) {
            int routeState = routeStateForSlot(slot);
            if (routeState != 1) {
                clearFbo(slot.layerFbo);
                copySlotToLayerBuffer(static_cast<int>(i));
                continue;
            }

            const int effectColumn = static_cast<int>(i) + 1;
            float coverageValue = slot.coverage.defined ? slot.coverageParamValue
                                                        : postEffects.defaultCoverageForType(slot.type);
            auto window = postEffects.resolveCoverageWindow(effectColumn, coverageValue);
            auto columnInWindow = [&](int columnIndex) -> bool {
                if (window.lastColumn == 0) return false;
                if (window.includesAll) return columnIndex < effectColumn;
                return columnIndex >= window.firstColumn && columnIndex <= window.lastColumn;
            };

            ensureSlotFbo(slot.layerFbo, viewport);
            ensureSlotFbo(slot.upstreamFbo, viewport);
            ensureSlotFbo(slot.effectFbo, viewport);

            slot.upstreamFbo.begin();
            ofClear(0, 0, 0, 0);
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            bool haveInput = false;
            for (int column = 1; column < effectColumn; ++column) {
                if (!columnInWindow(column)) continue;
                const auto& upstreamSlot = consoleSlots[column - 1];
                if (!upstreamSlot.active) continue;
                if (!upstreamSlot.layerFbo.isAllocated()) continue;
                upstreamSlot.layerFbo.draw(0, 0, viewport.x, viewport.y);
                haveInput = true;
            }
            ofDisableBlendMode();
            slot.upstreamFbo.end();

            slot.layerFbo.begin();
            ofClear(0, 0, 0, 0);
            ofDisableBlendMode();
            slot.layerFbo.end();

            if (haveInput) {
                slot.effectFbo.begin();
                ofClear(0, 0, 0, 0);
                slot.effectFbo.end();
            }

            if (haveInput && applyEffectSlot(slot, slot.upstreamFbo, slot.effectFbo)) {
                slot.layerFbo.begin();
                ofEnableBlendMode(OF_BLENDMODE_ALPHA);
                slot.effectFbo.draw(0, 0, viewport.x, viewport.y);
                ofDisableBlendMode();
                slot.layerFbo.end();
            }

            slot.layerFbo.begin();
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            for (int column = 1; column < effectColumn; ++column) {
                if (columnInWindow(column)) continue;
                const auto& upstreamSlot = consoleSlots[column - 1];
                if (!upstreamSlot.active) continue;
                if (!upstreamSlot.layerFbo.isAllocated()) continue;
                upstreamSlot.layerFbo.draw(0, 0, viewport.x, viewport.y);
            }
            ofDisableBlendMode();
            slot.layerFbo.end();
        } else if (slot.layer) {
            ensureSlotFbo(slot.layerFbo, viewport);
            float slotOpacity = ofClamp(slot.opacity, 0.0f, 1.0f);
            slot.layerFbo.begin();
            ofClear(0, 0, 0, 0);
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            LayerDrawParams params{
                cam,
                viewport,
                t,
                beatPhase,
                slotOpacity
            };
            slot.layer->draw(params);
            ofDisableBlendMode();
            slot.layerFbo.end();
        } else {
            clearFbo(slot.layerFbo);
        }

        copySlotToLayerBuffer(static_cast<int>(i));
    }
    ofPopStyle();

    compositeFbo.begin();
    ofClear(0, 0, 0, 0);
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    for (const auto& slot : consoleSlots) {
        if (!slot.active) continue;
        if (!slot.layerFbo.isAllocated()) continue;
        ofSetColor(255);
        slot.layerFbo.draw(0, 0, viewport.x, viewport.y);
    }
    ofDisableBlendMode();
    ofPopStyle();
    compositeFbo.end();

    postEffects.applyGlobal(compositeFbo);

    ofSetColor(255);
    compositeFbo.draw(0, 0);
}

void ofApp::updateConsoleLayers(const LayerUpdateParams& params) {
    for (auto& slot : consoleSlots) {
        if (!slot.layer || !slot.active) continue;
        slot.layer->update(params);
    }
}


void ofApp::keyPressed(int key) {
    // Windows delivers Ctrl+letter as control characters (1..26). Map them back to
    // the corresponding printable key so we can attach modifier flags consistently.
    bool ctrlDown = ofGetKeyPressed(OF_KEY_CONTROL);
    bool mappedControlChar = false;
    if (ctrlDown && key >= 1 && key <= 26) {
        key = 'a' + (key - 1);
        mappedControlChar = true;
    }

    auto isModifierKey = [](int code) {
        return code == OF_KEY_CONTROL || code == OF_KEY_SHIFT || code == OF_KEY_ALT;
    };

    if (!mappedControlChar) {
        bool anyPrintableDown = false;
        for (int c = 32; c <= 126; ++c) {
            if (ofGetKeyPressed(c)) {
                anyPrintableDown = true;
                break;
            }
        }
        // Only ignore pure modifier key events. Don't drop modifier+digit combos
        // here because on some platforms the printable key may not be visible
        // via ofGetKeyPressed() at the exact event timing.
        if (isModifierKey(key) && !anyPrintableDown) {
            ofLogNotice("ofApp") << "Ignoring standalone modifier key base=" << key;
            return;
        }
    }

    // Encode modifier state into the high bits so MenuController hotkeys can match Ctrl/Shift/Alt combos.
    int combinedKey = key;
    if (ctrlDown) combinedKey |= MenuController::HOTKEY_MOD_CTRL;
    if (ofGetKeyPressed(OF_KEY_SHIFT)) combinedKey |= MenuController::HOTKEY_MOD_SHIFT;
    if (ofGetKeyPressed(OF_KEY_ALT)) combinedKey |= MenuController::HOTKEY_MOD_ALT;

    int base = combinedKey & 0xFFFF;
    const int mods = combinedKey & MenuController::HOTKEY_MOD_MASK;
#ifdef _WIN32
    if ((mods & MenuController::HOTKEY_MOD_CTRL) && base >= 32 && base <= 126) {
        if (!printableKeyPhysicallyDown(base)) {
            ofLogVerbose("ofApp") << "Ignoring synthetic modifier combo base=" << base << " combined=" << combinedKey;
            return;
        }
    }
#endif
    bool allowNonPrintableHotkey = base == OF_KEY_TAB;
    if (mods != 0 && (base < 32 || base > 126) && !allowNonPrintableHotkey) {
        bool foundPrintable = false;
        for (int c = 32; c <= 126; ++c) {
#ifdef _WIN32
            if (printableKeyPhysicallyDown(c)) {
                base = c;
                foundPrintable = true;
                break;
            }
#else
            if (ofGetKeyPressed(c)) {
                base = c;
                foundPrintable = true;
                break;
            }
#endif
        }
        if (foundPrintable) {
            combinedKey = mods | base;
            key = base;
        } else {
            ofLogNotice("ofApp") << "No printable key detected for modifier event; ignoring base=" << (combinedKey & 0xFFFF)
                                   << " combined=" << combinedKey;
            return;
        }
    }

    int normalizedBase = combinedKey & 0xFFFF;
    if ((combinedKey & MenuController::HOTKEY_MOD_CTRL) &&
        (combinedKey & MenuController::HOTKEY_MOD_ALT) &&
        normalizedBase >= 32 && normalizedBase <= 126) {
        // Windows AltGr may report as Ctrl+Alt; drop Alt so Ctrl shortcuts work.
        combinedKey &= ~MenuController::HOTKEY_MOD_ALT;
    }
    ofLogNotice("ofApp") << "keyPressed: key=" << key << " combinedKey=" << combinedKey
                          << " label='" << HotkeyManager::keyLabel(combinedKey) << "'";

    if (menuController.handleInput(combinedKey)) {
        return;
    }

    if (key == 'r' || key == 'R') {
        collector.requestReconnect();
        ofLogNotice("CollectorSerial") << "Manual reconnect requested";
        return;
    }

    const int baseKey = combinedKey & 0xFFFF;

    switch (baseKey) {
    case 'f':
    case 'F':
        if ((combinedKey & MenuController::HOTKEY_MOD_CTRL) == 0) {
            break;
        }
        ofSetFullscreen(!ofGetWindowMode());
        break;
    case ' ': {
        paused = !paused;
        break;
    }
    case OF_KEY_F1:
        activeMidiBank = "home";
        ensureActiveBankValid();
        break;
    case OF_KEY_F2:
        activeMidiBank = "scene";
        ensureActiveBankValid();
        break;
    case '+':
    case '=':
        param_speed = ofClamp(param_speed + 0.1f, 0.0f, 5.0f);
        break;
    case '-':
    case '_':
        param_speed = ofClamp(param_speed - 0.1f, 0.0f, 5.0f);
        break;
    case 'g':
    case 'G':
        if ((combinedKey & MenuController::HOTKEY_MOD_CTRL) == 0) {
            break;
        }
        if (gridLayer) {
            gridLayer->cycleSegments();
        }
        break;
    case OF_KEY_UP:
        param_camDist = ofClamp(param_camDist * 0.9f, 150.0f, 4000.0f);
        break;
    case OF_KEY_DOWN:
        param_camDist = ofClamp(param_camDist * 1.1f, 150.0f, 4000.0f);
        break;
    case ']':
        if (geodesicLayer) {
            geodesicLayer->incrementSubdivision();
        }
        break;
    case '[':
        if (geodesicLayer) {
            geodesicLayer->decrementSubdivision();
        }
        break;
    case 'n':
    case 'N':
        if ((combinedKey & MenuController::HOTKEY_MOD_CTRL) == 0) {
            break;
        }
        if (gameOfLifeLayer) {
            gameOfLifeLayer->randomize();
        }
        break;
    case 'o':
    case 'O':
        if ((combinedKey & MenuController::HOTKEY_MOD_CTRL) == 0) {
            break;
        }
        if (gameOfLifeLayer) {
            gameOfLifeLayer->setPaused(!gameOfLifeLayer->isPaused());
        }
        break;
    default:
        break;
    }
}


void ofApp::mousePressed(int x, int y, int button) {
    dragging = true;
    lastMouse = { (float)x, (float)y };
}

void ofApp::mouseDragged(int x, int y, int button) {
    if (!dragging) return;
    glm::vec2 cur(x, y);
    glm::vec2 d = cur - lastMouse;
    lastMouse = cur;

    const float sens = glm::radians(0.3f);
    camTheta += d.x * sens;
    camPhi -= d.y * sens;
    camPhi = ofClamp(camPhi, glm::radians(-89.0f), glm::radians(89.0f));
}

void ofApp::mouseReleased(int x, int y, int button) {
    dragging = false;
}

void ofApp::exit() {
    ofLogNotice("ofApp") << "exit begin";
    saveScene(kSceneAutosavePath);
    ofLogNotice("ofApp") << "scene saved";
    hotkeyManager.saveIfDirty();
    ofLogNotice("ofApp") << "hotkeys saved";
    midi.close();
    ofLogNotice("ofApp") << "exit done";
}

void ofApp::registerDynamicOscRoute(const MidiRouter::OscMap& m) {
    auto routeActive = [this, bankId = m.bankId]() {
        return bankId.empty() || midi.activeBank().empty() || bankId == midi.activeBank();
    };

    if (paramRegistry.findFloat(m.target)) {
        modifier::Modifier descriptor;
        descriptor.type = modifier::Type::kOsc;
        descriptor.blend = m.blend;
        descriptor.inputRange = {m.inMin, m.inMax, false};
        descriptor.outputRange = {m.outMin, m.outMax, m.relativeToBase};
        auto& runtime = paramRegistry.addFloatModifier(m.target, descriptor);
        runtime.ownerTag = "dynamic-osc:" + m.pattern;
        runtime.active = false;
        runtime.inputValue = 0.0f;
        auto* mods = paramRegistry.floatModifiers(m.target);
        if (!mods || mods->empty()) {
            ofLogWarning("ofApp") << "failed to register OSC modifier for " << m.target;
            return;
        }
        std::size_t modifierIndex = mods->size() - 1;

        OscParameterRouter::FloatRouteConfig cfg;
        cfg.pattern = m.pattern;
        cfg.inMin = m.inMin;
        cfg.inMax = m.inMax;
        cfg.outMin = m.outMin;
        cfg.outMax = m.outMax;
        cfg.smooth = m.smooth;
        cfg.deadband = m.deadband;
        cfg.useMappedValue = false;
        cfg.writeGuard = [this, paramId = m.target, routeActive]() {
            return routeActive() && oscRouteWriteAllowed(paramId);
        };
        cfg.targetSetter = [this, modifierIndex, paramId = m.target](float rawValue) {
            try {
                paramRegistry.setFloatModifierInput(paramId, modifierIndex, rawValue, true);
                emitOscModifierTelemetry(paramId, rawValue);
            } catch (const std::exception& e) {
                ofLogVerbose("ofApp") << "osc modifier input failed for " << paramId << ": " << e.what();
            }
        };
        oscRouter.addFloatRoute(cfg);
    } else if (auto* bp = paramRegistry.findBool(m.target)) {
        OscParameterRouter::BoolRouteConfig cfg;
        cfg.pattern = m.pattern;
        cfg.target = bp->value;
        cfg.threshold = 0.5f;
        cfg.writeGuard = routeActive;
        oscRouter.addBoolRoute(cfg);
    } else {
        ofLogVerbose("ofApp") << "Skipping OSC route for unresolved target " << m.target
                              << " pattern=" << m.pattern;
    }
}

void ofApp::rebuildDynamicOscRoutes() {
    oscRouter = OscParameterRouter();
    oscRouteMuteUntilMs_.clear();
    oscModifierTelemetryMs_.clear();

    for (const auto& entry : paramRegistry.floats()) {
        paramRegistry.clearFloatModifiersMatching(entry.meta.id, [](const ParameterRegistry::RuntimeModifier& runtime) {
            return runtime.ownerTag.rfind("dynamic-osc:", 0) == 0;
        });
    }

    setupOscRoutes();
    for (const auto& map : midi.getOscMaps()) {
        registerDynamicOscRoute(map);
    }
}

// ---------------- helpers ----------------

void ofApp::setupOscRoutes() {
    OscParameterRouter::FloatRouteConfig cfg;

    cfg.pattern = "/sensor/hr/*/bpm";
    cfg.target = &param_speed;
    cfg.inMin = 40.0f;
    cfg.inMax = 180.0f;
    cfg.outMin = 0.0f;
    cfg.outMax = 5.0f;
    cfg.smooth = 0.25f;
    cfg.deadband = 0.02f;
    oscRouter.addFloatRoute(cfg);

    if (geodesicLayer) {
        cfg.pattern = "/sensor/matrix/*/mic-level";
        cfg.target = geodesicLayer->hoverParamPtr();
        cfg.inMin = 0.0f;
        cfg.inMax = 1.0f;
        cfg.outMin = 0.0f;
        cfg.outMax = 200.0f;
        cfg.smooth = 0.20f;
        cfg.deadband = 0.5f;
        oscRouter.addFloatRoute(cfg);

        cfg.pattern = "/sensor/matrix/*/mic-peak";
        cfg.target = geodesicLayer->spinParamPtr();
        cfg.inMin = 0.0f;
        cfg.inMax = 1.0f;
        cfg.outMin = 0.0f;
        cfg.outMax = 360.0f;
        cfg.smooth = 0.25f;
        cfg.deadband = 0.5f;
        oscRouter.addFloatRoute(cfg);
    }

    if (gridLayer) {
        oscRouter.addBoolRoute("/sensor/deck/*/deck-scene", gridLayer->enabledParamPtr(), 0.5f);
        oscRouter.addBoolRoute("/sensor/deck/*/scene", gridLayer->enabledParamPtr(), 0.5f);
    }
}

void ofApp::ingestOscMessage(const std::string& address, float value) {
    static uint64_t oscLogCount = 0;
    ++oscLogCount;
    if (oscLogCount <= 5 || (oscLogCount % 2000) == 0) {
        ofLogVerbose("OscIn") << "address=" << address << " value=" << value;
    }
    midi.onOscMessage(address, value);
    oscRouter.onMessage(address, value);
    noteSensorActivity(address, value);
    oscHistory.emplace_back(address, value);
    publishHudTelemetrySample("hud.sensors", "osc", value, address);
    while (oscHistory.size() > oscHistoryMax) {
        oscHistory.pop_front();
    }
}

void ofApp::noteSensorActivity(const std::string& address, float value) {
    auto tokens = ofSplitString(address, "/", true, true);
    if (tokens.size() < 4) return;
    if (tokens[0] != "sensor") return;

    const std::string deviceRaw = tokens[1];
    const std::string metricRaw = tokens.back();
    std::string device = ofToLower(deviceRaw);
    std::string scope = tokens.size() >= 3 ? ofToLower(tokens[2]) : std::string();
    std::string metric = ofToLower(metricRaw);
    uint64_t nowMs = ofGetElapsedTimeMillis();

    auto markDeck = [&](HudCategoryActivity& cat, const std::string& feedId) {
        hudDeckActivity.lastAnyMs = nowMs;
        cat.mark(nowMs, metricRaw, value);
        publishHudTelemetrySample("hud.sensors", feedId, value, metricRaw);
    };
    auto markMatrix = [&](HudCategoryActivity& cat, const std::string& feedId) {
        hudMatrixActivity.lastAnyMs = nowMs;
        cat.mark(nowMs, metricRaw, value);
        publishHudTelemetrySample("hud.sensors", feedId, value, metricRaw);
    };
    auto markHost = [&](HudCategoryActivity& cat, const std::string& feedId) {
        hudHostActivity.lastAnyMs = nowMs;
        cat.mark(nowMs, metricRaw, value);
        publishHudTelemetrySample("hud.sensors", feedId, value, metricRaw);
    };
    auto propagateBioMetric = [&](const std::string& metricName) {
        if (controlMappingHub) {
            controlMappingHub->setBioAmpMetric(metricName, value);
        }
    };
    auto propagateBioMetadata = [&](const std::string& metricName) {
        if (controlMappingHub) {
            controlMappingHub->setBioAmpMetadata(metricName, value);
        }
    };

    if (device == "deck" || device == "cyberdeck") {
        if (isHrMetric(metric)) {
            markDeck(hudDeckActivity.hr, "hr");
        } else if (isImuMetric(metric)) {
            markDeck(hudDeckActivity.imu, "imu");
        } else {
            markDeck(hudDeckActivity.aux, metric);
        }
    } else if (device == "hr") {
        markDeck(hudDeckActivity.hr, "hr");
    } else if (device == "matrix" || device == "matrixportal") {
        if (isMicMetric(metric)) {
            markMatrix(hudMatrixActivity.mic, "mic");
        } else if (metric == "bioamp-sample-rate" || metric == "bioamp-window") {
            propagateBioMetadata(metric);
        } else if (isBioMetric(metric)) {
            markMatrix(hudMatrixActivity.bio, "bio");
            propagateBioMetric(metric);
        } else if (isImuMetric(metric)) {
            markMatrix(hudMatrixActivity.imu, "imu");
        } else {
            markMatrix(hudMatrixActivity.aux, metric);
        }
    } else if (device == "host") {
        if (scope == "localmic" || isMicMetric(metric)) {
            markHost(hudHostActivity.mic, "host.mic");
        } else {
            markHost(hudHostActivity.aux, scope.empty() ? "host.aux" : scope);
        }
    }
}
bool ofApp::addAssetToConsoleLayer(int layerIndex,
                                   const std::string& assetId,
                                   bool activate,
                                   std::optional<float> opacityOverride) {
    const bool timingSceneLoad = sceneLoadInProgress();
    const uint64_t installStartedMs = timingSceneLoad ? static_cast<uint64_t>(ofGetElapsedTimeMillis()) : 0;
    uint64_t installStepStartedMs = installStartedMs;
    auto logSceneLoadInstallStep = [&](const std::string& step) {
        if (!timingSceneLoad) {
            return;
        }
        const uint64_t nowMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
        ofLogNotice("Scene") << "scene load layer " << layerIndex
                             << " asset " << assetId
                             << " " << step << ": "
                             << (nowMs - installStepStartedMs) << " ms";
        installStepStartedMs = nowMs;
    };

    if (layerIndex < 1 || layerIndex > 8) {
        ofLogWarning("ofApp") << "Console layer index out of range: " << layerIndex;
        return false;
    }
    const auto* entry = layerLibrary.find(assetId);
    if (!entry) {
        ofLogWarning("ofApp") << "Console asset not found: " << assetId;
        return false;
    }
    int idx = layerIndex - 1;
    if (idx >= static_cast<int>(consoleSlots.size())) {
        consoleSlots.resize(8);
    }

    int duplicate = findConsoleSlotByAsset(assetId);
    if (duplicate >= 0 && duplicate != idx) {
        ofLogWarning("ofApp") << "Asset " << assetId << " is already assigned to console layer " << (duplicate + 1)
                              << "; cannot assign to " << layerIndex;
        return false;
    }

    const std::string entryLabel = entry->label.empty() ? entry->id : entry->label;
    float defaultOpacity = std::clamp(entry->opacity, 0.0f, 1.0f);
    float requestedOpacity = opacityOverride.has_value()
                                 ? static_cast<float>(std::clamp<double>(*opacityOverride, 0.0, 1.0))
                                 : defaultOpacity;
    ConsoleSlot& slot = consoleSlots[idx];
    auto finish = [&](bool success) {
        if (success) {
            rebuildDynamicOscRoutes();
            persistConsoleAssignments();
            if (controlMappingHub) {
                controlMappingHub->markConsoleSlotsDirty();
            }
            if (timingSceneLoad) {
                const uint64_t nowMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
                ofLogNotice("Scene") << "scene load layer " << layerIndex
                                     << " asset " << assetId
                                     << " commit: " << (nowMs - installStepStartedMs)
                                     << " ms, total " << (nowMs - installStartedMs) << " ms";
                installStepStartedMs = nowMs;
            }
            ofLogNotice("ofApp") << "Console layer " << layerIndex << " loaded asset " << assetId;
            menuController.requestViewModelRefresh();
        }
        return success;
    };

    if (slot.assetId == assetId) {
        slot.label = entryLabel;
        if (slot.layer) {
            slot.layer->setExternalEnabled(activate);
        } else if (isFxType(slot.type)) {
            setFxRouteForType(slot.type, activate ? 1.0f : 0.0f);
            if (!slot.coverage.defined) {
                applyEffectCoverageDefaults(slot, slot.type);
            }
            registerConsoleLayerCoverageParam(layerIndex, slot);
        }
        slot.active = activate;
        return finish(true);
    }

    clearConsoleSlot(idx);
    logSceneLoadInstallStep("clear");
    slot.assetId = entry->id;
    slot.label = entryLabel;
    slot.type = entry->type;
    slot.opacity = requestedOpacity;
    slot.active = activate;

    if (isFxType(entry->type)) {
        slot.paramPrefix = entry->registryPrefix.empty() ? entry->id : entry->registryPrefix;
        applyEffectCoverageDefaults(slot, entry->type);
        registerConsoleLayerCoverageParam(layerIndex, slot);
        setFxRouteForType(entry->type, activate ? 1.0f : 0.0f);
        return finish(true);
    }

    if (isUiOverlayType(entry->type)) {
        slot.paramPrefix = entry->registryPrefix.empty() ? entry->id : entry->registryPrefix;
        return finish(true);
    }

    std::string prefix = "console.layer" + std::to_string(layerIndex);
    auto creator = [type = entry->type]() -> std::unique_ptr<Layer> {
        return LayerFactory::instance().create(type);
    };

    std::unique_ptr<Layer> l = creator();
    logSceneLoadInstallStep("create");
    if (!l) {
        ofLogWarning("ofApp") << "Failed to create layer for asset " << assetId;
        slot.assetId.clear();
        slot.type.clear();
        slot.paramPrefix.clear();
        slot.active = false;
        return false;
    }
    l->configure(entry->config);
    logSceneLoadInstallStep("configure");
    l->setRegistryPrefix(prefix);
    l->setInstanceId(entry->id);
    l->setup(paramRegistry);
    logSceneLoadInstallStep("setup");
    l->setExternalEnabled(activate);
    logSceneLoadInstallStep("enable");
    slot.layer = std::move(l);
    slot.paramPrefix = prefix;

    if (slot.layer) {
        Layer* layerPtr = slot.layer.get();
        if (auto* grid = dynamic_cast<GridLayer*>(layerPtr)) {
            gridLayer = grid;
        } else if (auto* geo = dynamic_cast<GeodesicLayer*>(layerPtr)) {
            geodesicLayer = geo;
        } else if (auto* perlin = dynamic_cast<PerlinNoiseLayer*>(layerPtr)) {
            perlinLayer = perlin;
            registerPerlinMidi(perlin);
        } else if (auto* gol = dynamic_cast<GameOfLifeLayer*>(layerPtr)) {
            gameOfLifeLayer = gol;
            registerGameOfLifeMidi(gol);
            if (activate) {
                gol->randomize();
            }
        }
        registerConsoleLayerOpacityParam(layerIndex, slot);
    }

    return finish(true);
}

void ofApp::openAssetBrowserForConsole(int layerIndex) {
    // Create a temporary AssetBrowser instance dedicated to selecting an
    // asset for the requested console layer. On selection, install the
    // asset into the console (via addAssetToConsoleLayer) and close the
    // picker.
    assetBrowser = std::make_shared<AssetBrowser>();
    assetBrowser->setLibrary(&layerLibrary);
    assetBrowser->setAllowEntryPredicate([](const LayerLibrary::Entry& entry) {
        return !entry.isHudWidget();
    });
    assetBrowser->setPresenceQuery([this](const std::string& id) {
        return findConsoleSlotByAsset(id) >= 0;
    });
    assetBrowser->setActiveQuery([this](const std::string& id) {
        int idx = findConsoleSlotByAsset(id);
        if (idx < 0 || idx >= static_cast<int>(consoleSlots.size())) {
            return false;
        }
        const auto& slot = consoleSlots[idx];
        return !slot.assetId.empty() && slot.active;
    });
    // Console picker ignores deck toggles; Enter loads the highlighted asset/effect
    // directly into the requested console layer.
    assetBrowser->setCommandHandler([this, layerIndex](const LayerLibrary::Entry& entry, int key) {
        if (key == OF_KEY_RETURN || key == '\r') {
            if (addAssetToConsoleLayer(layerIndex, entry.id, true)) {
                menuController.popState();
            }
        }
    });
    // Push the browser on top of the current Console state so popping it returns
    // to the Console (instead of replacing the Console state).
    menuController.pushState(assetBrowser);
}

void ofApp::clearConsoleSlot(int index) {
    if (index < 0 || index >= static_cast<int>(consoleSlots.size())) return;
    ConsoleSlot& slot = consoleSlots[index];
    bool changedRouteTargets = !slot.paramPrefix.empty();
    unregisterConsoleLayerCoverageParam(index + 1);
    if (slot.layer) {
        std::string oldPrefix = slot.layer->registryPrefix();
        midi.unbindByPrefix(oldPrefix);
        paramRegistry.removeByPrefix(oldPrefix);
        Layer* ptr = slot.layer.get();
        if (ptr == gridLayer) gridLayer = nullptr;
        if (ptr == geodesicLayer) geodesicLayer = nullptr;
        if (ptr == perlinLayer) perlinLayer = nullptr;
        if (ptr == gameOfLifeLayer) gameOfLifeLayer = nullptr;
        slot.layer.reset();
    } else if (!slot.assetId.empty()) {
        if (const auto* entry = layerLibrary.find(slot.assetId)) {
            if (isFxType(entry->type)) {
                setFxRouteForType(entry->type, 0.0f);
            }
        }
    }
    slot.assetId.clear();
    slot.label.clear();
    slot.type.clear();
    slot.paramPrefix.clear();
    slot.active = false;
    slot.opacity = 1.0f;
    slot.coverage = ConsoleLayerCoverageInfo();
    slot.coverageParamValue = 0.0f;
    if (changedRouteTargets) {
        rebuildDynamicOscRoutes();
    }
    if (controlMappingHub) {
        controlMappingHub->markConsoleSlotsDirty();
    }
}

void ofApp::persistConsoleAssignments() {
    if (consolePersistenceSuspended_) return;
    if (consoleConfigPath.empty()) return;
    ConsolePresentationState state;
    state.layers.reserve(consoleSlots.size());
    for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
        ConsoleLayerInfo info;
        info.index = static_cast<int>(i) + 1;
        const auto& slot = consoleSlots[i];
        info.assetId = slot.assetId;
        info.active = slot.active;
        info.opacity = consoleSlotBaseOpacity(info.index);
        info.label = slot.label;
        info.coverage = slot.coverage;
        state.layers.push_back(info);
    }
    state.overlays.hudVisible = overlayVisibility_.hud;
    state.overlays.consoleVisible = overlayVisibility_.console;
    state.overlays.controlHubVisible = overlayVisibility_.controlHub;
    state.overlays.menuVisible = overlayVisibility_.menus;
    state.dualDisplay.mode = persistedDualDisplayMode_.empty() ? "single" : persistedDualDisplayMode_;
    state.secondaryDisplay.enabled = secondaryDisplay_.enabled;
    state.secondaryDisplay.monitorId = secondaryDisplay_.monitorId;
    state.secondaryDisplay.x = secondaryDisplay_.x;
    state.secondaryDisplay.y = secondaryDisplay_.y;
    state.secondaryDisplay.width = secondaryDisplay_.width;
    state.secondaryDisplay.height = secondaryDisplay_.height;
    state.secondaryDisplay.vsync = secondaryDisplay_.vsync;
    state.secondaryDisplay.dpiScale = secondaryDisplay_.dpiScale;
    state.secondaryDisplay.background = secondaryDisplay_.background;
    state.secondaryDisplay.followPrimary = secondaryDisplay_.followPrimary;
    state.controllerFocus.consolePreferred = controllerFocus_.preferConsole;
    state.controllerFocusDefined = true;
    if (controlMappingHub) {
        auto captureSnapshot = [&](ControlMappingHubState::HudLayoutTarget target) {
            ConsoleOverlayLayoutSnapshot snapshot;
            snapshot.capturedAtMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
            const auto placements = controlMappingHub->exportHudLayoutSnapshot(target);
            snapshot.widgets.reserve(placements.size());
            for (const auto& placement : placements) {
                ConsoleOverlayWidgetPlacement widget;
                widget.id = placement.id;
                widget.columnIndex = placement.columnIndex;
                widget.visible = placement.visible;
                widget.collapsed = placement.collapsed;
                widget.bandId = placement.bandId.empty() ? "hud" : placement.bandId;
                widget.target = placement.target.empty() ? "projector" : placement.target;
                snapshot.widgets.push_back(std::move(widget));
            }
            return snapshot;
        };
        state.overlayLayouts.projector = captureSnapshot(ControlMappingHubState::HudLayoutTarget::Projector);
        state.overlayLayouts.controller = captureSnapshot(ControlMappingHubState::HudLayoutTarget::Controller);
        state.overlayLayouts.activeTarget =
            ControlMappingHubState::hudLayoutTargetName(controlMappingHub->hudLayoutTarget());
        state.overlayLayouts.lastSyncMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
        state.overlayLayoutsDefined = true;
    }
    if (controlMappingHub) {
        const auto& bio = controlMappingHub->bioAmpState();
        ConsoleBioAmpSnapshot snap;
        bool hasAny = false;
        auto captureSample = [&](const ControlMappingHubState::BioAmpMetricSample& sample,
                                 bool& hasField,
                                 float& value,
                                 uint64_t& ts) {
            if (sample.valid) {
                hasField = true;
                value = sample.value;
                ts = sample.timestampMs;
                hasAny = true;
            }
        };
        captureSample(bio.raw, snap.hasRaw, snap.raw, snap.rawTimestampMs);
        captureSample(bio.signal, snap.hasSignal, snap.signal, snap.signalTimestampMs);
        captureSample(bio.mean, snap.hasMean, snap.mean, snap.meanTimestampMs);
        captureSample(bio.rms, snap.hasRms, snap.rms, snap.rmsTimestampMs);
        captureSample(bio.domHz, snap.hasDomHz, snap.domHz, snap.domTimestampMs);
        if (bio.sampleRate != 0) {
            snap.hasSampleRate = true;
            snap.sampleRate = bio.sampleRate;
            snap.sampleRateTimestampMs = bio.sampleRateTimestampMs;
            hasAny = true;
        }
        if (bio.windowSize != 0) {
            snap.hasWindow = true;
            snap.window = bio.windowSize;
            snap.windowTimestampMs = bio.windowTimestampMs;
            hasAny = true;
        }
        if (hasAny) {
            state.sensorsDefined = true;
            state.sensors.bioAmpDefined = true;
            state.sensors.bioAmp = snap;
        }
    }
    ConsoleStore::saveState(consoleConfigPath, state);
}

void ofApp::clearAllConsoleSlots() {
    for (int i = 0; i < static_cast<int>(consoleSlots.size()); ++i) {
        clearConsoleSlot(i);
    }
}

void ofApp::seedConsoleDefaultsIfEmpty() {
    static const std::pair<int, const char*> kDefaultSeeds[] = {
        {1, "geometry.grid"},
        {2, "geometry.geodesic"},
        {3, "generative.perlin"},
        {4, "generative.gameOfLife"}
    };

    int assignedCount = 0;
    bool onlyGameOfLife = true;
    for (const auto& slot : consoleSlots) {
        if (slot.assetId.empty()) {
            continue;
        }
        ++assignedCount;
        if (slot.assetId != "generative.gameOfLife") {
            onlyGameOfLife = false;
        }
    }

    auto installSeeds = [this]() {
        for (const auto& seed : kDefaultSeeds) {
            addAssetToConsoleLayer(seed.first, seed.second, true);
        }
    };

    if (assignedCount == 0) {
        installSeeds();
        return;
    }

    if (assignedCount == 1 && onlyGameOfLife) {
        clearAllConsoleSlots();
        installSeeds();
    }
}

bool ofApp::loadConsoleLayoutFromScene(const ofJson& consoleNode) {
    if (!consoleNode.is_object()) {
        return false;
    }
    if (!consoleNode.contains("slots") || !consoleNode["slots"].is_array()) {
        return false;
    }
    clearAllConsoleSlots();
    bool appliedAllSlots = true;
    for (const auto& slotNode : consoleNode["slots"]) {
        if (!slotNode.contains("index") || !slotNode.contains("assetId")) continue;
        int index = slotNode["index"].get<int>();
        std::string assetId = slotNode["assetId"].get<std::string>();
        bool active = slotNode.value("active", true);
        float opacity = static_cast<float>(slotNode.value("opacity", 1.0));
        std::string label = slotNode.value("label", std::string());
        ConsoleLayerCoverageInfo coverage;
        if (slotNode.contains("coverage") && slotNode["coverage"].is_object()) {
            const auto& coverageNode = slotNode["coverage"];
            coverage.defined = true;
            if (coverageNode.contains("mode") && coverageNode["mode"].is_string()) {
                coverage.mode = coverageNode["mode"].get<std::string>();
            }
            if (coverageNode.contains("columns") && coverageNode["columns"].is_number_integer()) {
                coverage.columns = std::max(0, coverageNode["columns"].get<int>());
            }
        }
        if (addAssetToConsoleLayer(index, assetId, active, opacity)) {
            if (auto* slot = consoleSlotForIndex(index)) {
                if (!label.empty()) {
                    slot->label = label;
                }
                if (coverage.defined) {
                    importConsoleCoverageFromInfo(index, coverage);
                }
            }
        } else {
            appliedAllSlots = false;
        }
    }
    return appliedAllSlots;
}

void ofApp::writeConsoleLayoutToScene(ofJson& scene) const {
    ofJson slots = ofJson::array();
    for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
        const auto& slot = consoleSlots[i];
        if (slot.assetId.empty()) continue;
        ofJson node;
        node["index"] = static_cast<int>(i) + 1;
        node["assetId"] = slot.assetId;
        node["active"] = slot.active;
        node["opacity"] = consoleSlotBaseOpacity(static_cast<int>(i) + 1);
        if (!slot.label.empty()) {
            node["label"] = slot.label;
        }
        if (slot.coverage.defined) {
            ofJson coverageNode;
            coverageNode["mode"] = slot.coverage.mode;
            coverageNode["columns"] = slot.coverage.columns;
            node["coverage"] = std::move(coverageNode);
        }
        writeConsoleParametersToScene(node, slot);
        slots.push_back(std::move(node));
    }
    if (!slots.empty()) {
        ofJson consoleNode;
        consoleNode["slots"] = std::move(slots);
        scene["console"] = std::move(consoleNode);
    }
}

void ofApp::writeConsoleParametersToScene(ofJson& slotNode, const ConsoleSlot& slot) const {
    if (slot.paramPrefix.empty()) {
        return;
    }

    const std::string prefix = slot.paramPrefix + ".";
    const std::string opacityId = slot.paramPrefix + ".opacity";
    const std::string visibleId = slot.paramPrefix + ".visible";
    const bool slotIsFx = isFxType(slot.type);
    ofJson parameters = ofJson::object();

    for (const auto& param : paramRegistry.floats()) {
        const std::string& id = param.meta.id;
        if (!startsWith(id, prefix) || id == opacityId || id == visibleId) {
            continue;
        }
        if (slotIsFx && id == slot.paramPrefix + ".route") {
            continue;
        }
        parameters[id] = encodeFloatParam(param);
    }
    for (const auto& param : paramRegistry.bools()) {
        const std::string& id = param.meta.id;
        if (!startsWith(id, prefix) || id == opacityId || id == visibleId) {
            continue;
        }
        parameters[id] = encodeBoolParam(param);
    }
    for (const auto& param : paramRegistry.strings()) {
        const std::string& id = param.meta.id;
        if (!startsWith(id, prefix)) {
            continue;
        }
        parameters[id] = param.baseValue;
    }

    if (!parameters.empty()) {
        slotNode["parameters"] = std::move(parameters);
    }
}

ofApp::ConsoleSlot* ofApp::consoleSlotForIndex(int layerIndex) {
    int idx = layerIndex - 1;
    if (idx < 0 || idx >= static_cast<int>(consoleSlots.size())) return nullptr;
    return &consoleSlots[idx];
}

const ofApp::ConsoleSlot* ofApp::consoleSlotForIndex(int layerIndex) const {
    int idx = layerIndex - 1;
    if (idx < 0 || idx >= static_cast<int>(consoleSlots.size())) return nullptr;
    return &consoleSlots[idx];
}

int ofApp::findConsoleSlotByAsset(const std::string& assetId) const {
    if (assetId.empty()) return -1;
    for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
        if (consoleSlots[i].assetId == assetId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string ofApp::consoleSlotPrefix(int layerIndex) const {
    const ConsoleSlot* slot = consoleSlotForIndex(layerIndex);
    if (!slot) return std::string();
    if (!slot->paramPrefix.empty()) {
        return slot->paramPrefix;
    }
    if (slot->layer) {
        return slot->layer->registryPrefix();
    }
    if (!slot->assetId.empty()) {
        if (const auto* entry = layerLibrary.find(slot->assetId)) {
            return entry->registryPrefix.empty() ? entry->id : entry->registryPrefix;
        }
    }
    return std::string();
}

void ofApp::registerConsoleLayerOpacityParam(int layerIndex, ConsoleSlot& slot) {
    if (slot.paramPrefix.empty()) {
        return;
    }
    float defaultValue = ofClamp(slot.opacity, 0.0f, 1.0f);
    slot.opacity = defaultValue;
    ParameterRegistry::Descriptor meta;
    meta.label = "Layer Opacity";
    meta.group = "Visibility";
    meta.description = "Base opacity for this layer before FX or modifiers are applied";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    meta.quickAccess = true;
    meta.quickAccessOrder = 0;
    const std::string paramId = slot.paramPrefix + ".opacity";
    try {
        paramRegistry.addFloat(paramId, &slot.opacity, defaultValue, meta);
        midi.bindFloat(paramId,
                       &slot.opacity,
                       meta.range.min,
                       meta.range.max,
                       false,
                       meta.range.step);
    } catch (const std::exception& ex) {
        ofLogWarning("ofApp") << "Failed to register opacity parameter for slot " << layerIndex << ": " << ex.what();
    }

    const std::string visibleId = slot.paramPrefix + ".visible";
    if (auto* visibleParam = paramRegistry.findBool(visibleId)) {
        if (visibleParam->value) {
            midi.bindBool(visibleId, visibleParam->value, MidiRouter::BoolMode::Toggle);
        }
    }
}

void ofApp::registerConsoleLayerCoverageParam(int layerIndex, ConsoleSlot& slot) {
    if (!isFxType(slot.type)) {
        unregisterConsoleLayerCoverageParam(layerIndex);
        return;
    }
    if (!slot.coverage.defined) {
        slot.coverage.defined = true;
    }
    if (slot.coverage.mode.empty()) {
        slot.coverage.mode = "upstream";
    }
    slot.coverage.columns = std::max(0, slot.coverage.columns);
    slot.coverageParamValue = static_cast<float>(slot.coverage.columns);
}

void ofApp::unregisterConsoleLayerCoverageParam(int layerIndex) {}

void ofApp::importConsoleCoverageFromInfo(int layerIndex, const ConsoleLayerCoverageInfo& coverage) {
    ConsoleSlot* slot = consoleSlotForIndex(layerIndex);
    if (!slot || !isFxType(slot->type)) {
        return;
    }
    slot->coverage = coverage;
    slot->coverage.defined = true;
    if (slot->coverage.mode.empty()) {
        slot->coverage.mode = "upstream";
    }
    slot->coverage.columns = std::max(0, slot->coverage.columns);
    slot->coverageParamValue = static_cast<float>(slot->coverage.columns);
    registerConsoleLayerCoverageParam(layerIndex, *slot);
}

void ofApp::applyEffectCoverageDefaults(ConsoleSlot& slot, const std::string& effectType) {
    slot.coverage.defined = true;
    slot.coverage.mode = "upstream";
    slot.coverage.columns = std::max(0, static_cast<int>(std::round(postEffects.defaultCoverageForType(effectType))));
    slot.coverageParamValue = static_cast<float>(slot.coverage.columns);
}

void ofApp::propagateEffectCoverageChange(const std::string& effectType, float coverage) {
    int columns = std::max(0, static_cast<int>(std::round(coverage)));
    bool changed = false;
    for (std::size_t i = 0; i < consoleSlots.size(); ++i) {
        auto& slot = consoleSlots[i];
        if (slot.type != effectType) {
            continue;
        }
        slot.coverage.defined = true;
        slot.coverage.mode = "upstream";
        slot.coverage.columns = columns;
        slot.coverageParamValue = static_cast<float>(columns);
        registerConsoleLayerCoverageParam(static_cast<int>(i) + 1, slot);
        changed = true;
    }
    if (changed) {
        persistConsoleAssignments();
        if (controlMappingHub) {
            controlMappingHub->markConsoleSlotsDirty();
        }
    }
}

bool ofApp::applyEffectSlot(ConsoleSlot& slot, ofFbo& src, ofFbo& dst) {
    if (!src.isAllocated() || !dst.isAllocated()) {
        return false;
    }
    if (slot.type == "fx.dither") {
        postEffects.applyDither(src, dst);
        return true;
    }
    if (slot.type == "fx.ascii") {
        postEffects.applyAscii(src, dst);
        return true;
    }
    if (slot.type == "fx.ascii_supersample") {
        postEffects.applyAsciiSupersample(src, dst);
        return true;
    }
    if (slot.type == "fx.crt") {
        postEffects.applyCrt(src, dst);
        return true;
    }
    if (slot.type == "fx.motion_extract") {
        postEffects.applyMotionExtract(src, dst);
        return true;
    }
    return false;
}

float ofApp::consoleSlotBaseOpacity(int layerIndex) const {
    float fallback = 1.0f;
    if (const auto* slot = consoleSlotForIndex(layerIndex)) {
        fallback = ofClamp(slot->opacity, 0.0f, 1.0f);
    }
    std::string prefix = consoleSlotPrefix(layerIndex);
    if (!prefix.empty()) {
        if (auto* param = paramRegistry.findFloat(prefix + ".opacity")) {
            fallback = ofClamp(param->baseValue, 0.0f, 1.0f);
        }
    }
    return fallback;
}


bool ofApp::toggleMenuState(MenuController& controller, const std::shared_ptr<MenuController::State>& state, bool allowStack) {
    if (!state) {
        ofLogNotice("ofApp") << "toggleMenuState called with null state";
        return false;
    }
    ofLogNotice("ofApp") << "toggleMenuState: stateId='" << state->id() << "' label='" << state->label() << "' current=" << controller.isCurrent(state->id());
    if (controller.isCurrent(state->id())) {
        ofLogNotice("ofApp") << "toggleMenuState: popping state '" << state->id() << "'";
        controller.popState();
        return true;
    }
    if (allowStack) {
        if (controller.contains(state->id())) {
            ofLogNotice("ofApp") << "toggleMenuState: removing stacked state '" << state->id() << "'";
            controller.removeState(state->id());
            return true;
        }
        ofLogNotice("ofApp") << "toggleMenuState: pushing stacked state '" << state->id() << "'";
        controller.pushState(state);
        return true;
    }
    auto vm = controller.viewModel();
    if (vm.hasState) {
        ofLogNotice("ofApp") << "toggleMenuState: replacing existing state with '" << state->id() << "'";
        controller.replaceState(state);
    } else {
        ofLogNotice("ofApp") << "toggleMenuState: pushing state '" << state->id() << "'";
        controller.pushState(state);
    }
    return true;
}

bool ofApp::toggleConsoleAndControlHub(MenuController& controller) {
    if (!consoleState || !controlMappingHub) {
        ofLogWarning("ofApp") << "Console or Browser not initialized";
        return false;
    }
    auto setUiVisibility = [&](const std::string& id,
                               bool& parameter,
                               bool& cachedVisibility,
                               bool visible,
                               const std::string& feedId) {
        if (paramRegistry.findBool(id)) {
            paramRegistry.setBoolBase(id, visible, true);
        } else {
            parameter = visible;
        }

        const bool routeUiToController = secondaryDisplay_.active && !secondaryDisplay_.followPrimary;
        const bool primaryVisible = routeUiToController ? false : visible;
        if (cachedVisibility != primaryVisible) {
            cachedVisibility = primaryVisible;
            publishOverlayVisibilityTelemetry(feedId, primaryVisible);
        }
    };

    bool consoleActive = controller.contains(consoleState->id());
    bool hubActive = controller.contains(controlMappingHub->id());
    if (consoleActive && hubActive) {
        controller.removeState(controlMappingHub->id());
        controller.removeState(consoleState->id());
        setUiVisibility("ui.console.visible", param_showConsole, overlayVisibility_.console, false, "overlay.console.visible");
        setUiVisibility("ui.hub.visible", param_showControlHub, overlayVisibility_.controlHub, false, "overlay.hub.visible");
        persistConsoleAssignments();
        return true;
    }
    if (!consoleActive) {
        controller.pushState(consoleState);
    }
    if (!hubActive) {
        controller.pushState(controlMappingHub);
    }
    setUiVisibility("ui.console.visible", param_showConsole, overlayVisibility_.console, true, "overlay.console.visible");
    setUiVisibility("ui.hub.visible", param_showControlHub, overlayVisibility_.controlHub, true, "overlay.hub.visible");
    persistConsoleAssignments();
    return true;
}

void ofApp::registerPerlinMidi(PerlinNoiseLayer* layer) {
    if (!layer) return;
    const std::string prefix = layer->registryPrefix();
    midi.bindFloat(prefix + ".scale", layer->scaleParamPtr(), 0.1f, 10.0f, false, 0.0f);
    midi.bindFloat(prefix + ".texelZoom", layer->texelZoomParamPtr(), 0.25f, 4.0f, false, 0.0f);
    midi.bindFloat(prefix + ".speed", layer->speedParamPtr(), 0.0f, 5.0f, false, 0.0f);
    midi.bindFloat(prefix + ".brightness", layer->brightnessParamPtr(), 0.0f, 2.0f, false, 0.0f);
    midi.bindFloat(prefix + ".contrast", layer->contrastParamPtr(), 0.1f, 4.0f, false, 0.0f);
    midi.bindFloat(prefix + ".alpha", layer->alphaParamPtr(), 0.0f, 1.0f, false, 0.0f);
    midi.bindFloat(prefix + ".colorR", layer->colorRParamPtr(), 0.0f, 2.0f, false, 0.0f);
    midi.bindFloat(prefix + ".colorG", layer->colorGParamPtr(), 0.0f, 2.0f, false, 0.0f);
    midi.bindFloat(prefix + ".colorB", layer->colorBParamPtr(), 0.0f, 2.0f, false, 0.0f);
    midi.bindFloat(prefix + ".octaves", layer->octavesParamPtr(), 1.0f, 8.0f, true, 1.0f);
    midi.bindFloat(prefix + ".lacunarity", layer->lacunarityParamPtr(), 1.0f, 6.0f, false, 0.0f);
    midi.bindFloat(prefix + ".persistence", layer->persistenceParamPtr(), 0.05f, 1.0f, false, 0.0f);
    midi.bindFloat(prefix + ".palette", layer->paletteIndexParamPtr(), 0.0f, static_cast<float>(std::max(0, layer->paletteCount() - 1)), true, 1.0f);
    midi.bindFloat(prefix + ".paletteRate", layer->paletteRateParamPtr(), -3.0f, 3.0f, false, 0.0f);
}

void ofApp::registerGameOfLifeMidi(GameOfLifeLayer* layer) {
    if (!layer) return;
    const std::string prefix = layer->registryPrefix();
    midi.bindFloat(prefix + ".speed", layer->speedParamPtr(), 0.0f, 20.0f, false, 0.0f);
    midi.bindBool(prefix + ".bpmSync", layer->bpmSyncParamPtr(), MidiRouter::BoolMode::Toggle);
    midi.bindFloat(prefix + ".bpmMultiplier", layer->bpmMultiplierParamPtr(), 0.25f, 8.0f, false, 0.0f);
    midi.bindBool(prefix + ".paused", layer->pausedParamPtr(), MidiRouter::BoolMode::Toggle);
    midi.bindBool(prefix + ".wrap", layer->wrapParamPtr(), MidiRouter::BoolMode::Toggle);
    midi.bindFloat(prefix + ".density", layer->densityParamPtr(), 0.0f, 1.0f, false, 0.0f);
    midi.bindFloat(prefix + ".fadeFrames", layer->fadeFramesParamPtr(), 1.0f, 32.0f, true, 1.0f);
    midi.bindFloat(prefix + ".preset", layer->presetParamPtr(), 0.0f, static_cast<float>(std::max(0, layer->presetCount() - 1)), true, 1.0f);
    midi.bindFloat(prefix + ".alpha", layer->alphaParamPtr(), 0.0f, 1.0f, false, 0.0f);
    midi.bindFloat(prefix + ".aliveAlpha", layer->aliveAlphaParamPtr(), 0.0f, 1.0f, false, 0.0f);
    midi.bindFloat(prefix + ".deadAlpha", layer->deadAlphaParamPtr(), 0.0f, 1.0f, false, 0.0f);
    midi.bindFloat(prefix + ".reseedQuantizeBeats", layer->reseedQuantizeBeatsParamPtr(), 0.0f, 16.0f, true, 1.0f);
    midi.bindBool(prefix + ".autoReseed", layer->autoReseedParamPtr(), MidiRouter::BoolMode::Toggle);
    midi.bindFloat(prefix + ".autoReseedEveryBeats", layer->autoReseedEveryBeatsParamPtr(), 1.0f, 64.0f, true, 1.0f);
    midi.bindBool(prefix + ".reseed", layer->reseedParamPtr(), MidiRouter::BoolMode::Assign);
}


float* ofApp::fxRouteParamForType(const std::string& type) {
    if (type == "fx.dither") return postEffects.ditherRouteParamPtr();
    if (type == "fx.ascii") return postEffects.asciiRouteParamPtr();
    if (type == "fx.ascii_supersample") return postEffects.asciiSupersampleRouteParamPtr();
    if (type == "fx.crt") return postEffects.crtRouteParamPtr();
    if (type == "fx.motion_extract") return postEffects.motionRouteParamPtr();
    return nullptr;
}

void ofApp::registerCoreMidiTargets() {
    midi.bindFloat("fx.master", &param_masterFx, 0.0f, 1.0f, false, 0.0f);
    midi.bindFloat("globals.speed", &param_speed, 0.0f, 5.0f, false, 0.0f);
    midi.bindFloat("transport.bpm", &param_bpm, 40.0f, 240.0f, false, 0.0f);
    midi.bindFloat("camera.dist", &param_camDist, 150.0f, 4000.0f, false, 0.0f);
    midi.bindBool("ui.hud", &param_showHud, MidiRouter::BoolMode::Toggle);
    midi.bindBool("ui.console.visible", &param_showConsole, MidiRouter::BoolMode::Toggle);
    midi.bindBool("ui.hub.visible", &param_showControlHub, MidiRouter::BoolMode::Toggle);
    midi.bindBool("ui.menu.visible", &param_showMenus, MidiRouter::BoolMode::Toggle);
}

const float* ofApp::fxRouteParamForType(const std::string& type) const {
    return const_cast<ofApp*>(this)->fxRouteParamForType(type);
}

void ofApp::setFxRouteForType(const std::string& type, float routeValue) {
    std::string paramId;
    if (type == "fx.dither") {
        paramId = "effects.dither.route";
    } else if (type == "fx.ascii") {
        paramId = "effects.ascii.route";
    } else if (type == "fx.ascii_supersample") {
        paramId = "effects.asciiSupersample.route";
    } else if (type == "fx.crt") {
        paramId = "effects.crt.route";
    } else if (type == "fx.motion_extract") {
        paramId = "effects.motion.route";
    }
    routeValue = ofClamp(routeValue, 0.0f, 2.0f);
    if (!paramId.empty() && paramRegistry.findFloat(paramId)) {
        paramRegistry.setFloatBase(paramId, routeValue, true);
        return;
    }
    if (float* route = fxRouteParamForType(type)) {
        *route = routeValue;
    }
}

void ofApp::syncActiveFxWithConsoleSlots(bool enablePresent) {
    std::unordered_set<std::string> fxAssets;
    for (const auto& slot : consoleSlots) {
        if (slot.assetId.empty()) {
            continue;
        }
        if (!slot.active) {
            continue;
        }
        if (!isFxType(slot.type)) {
            continue;
        }
        fxAssets.insert(slot.assetId);
    }
    auto disableIfMissing = [&](const std::string& assetId, const std::string& paramId) {
        if (fxAssets.count(assetId) > 0) {
            return;
        }
        auto* param = paramRegistry.findFloat(paramId);
        if (!param) {
            return;
        }
        paramRegistry.setFloatBase(paramId, 0.0f, true);
    };
    auto enableIfPresent = [&](const std::string& assetId, const std::string& paramId) {
        if (fxAssets.count(assetId) == 0) {
            return;
        }
        auto* param = paramRegistry.findFloat(paramId);
        if (!param || !param->value) {
            return;
        }
        float current = *param->value;
        if (std::round(ofClamp(current, 0.0f, 2.0f)) == 1.0f) {
            return;
        }
        // Console FX slots own their routing; stale scene data can otherwise leave
        // the shared route at Off/Global and make the slot appear loaded but inert.
        paramRegistry.setFloatBase(paramId, 1.0f, true);
    };

    if (enablePresent) {
        enableIfPresent("fx.dither", "effects.dither.route");
        enableIfPresent("fx.ascii", "effects.ascii.route");
        enableIfPresent("fx.ascii_supersample", "effects.asciiSupersample.route");
        enableIfPresent("fx.crt", "effects.crt.route");
        enableIfPresent("fx.motion_extract", "effects.motion.route");
    }

    disableIfMissing("fx.dither", "effects.dither.route");
    disableIfMissing("fx.ascii", "effects.ascii.route");
    disableIfMissing("fx.ascii_supersample", "effects.asciiSupersample.route");
    disableIfMissing("fx.crt", "effects.crt.route");
    disableIfMissing("fx.motion_extract", "effects.motion.route");
}

void ofApp::refreshLayerReferences() {
    gridLayer = nullptr;
    geodesicLayer = nullptr;
    perlinLayer = nullptr;
    gameOfLifeLayer = nullptr;

    for (auto& slot : consoleSlots) {
        if (!slot.layer) continue;
        Layer* base = slot.layer.get();
        if (!gridLayer) gridLayer = dynamic_cast<GridLayer*>(base);
        if (!geodesicLayer) geodesicLayer = dynamic_cast<GeodesicLayer*>(base);
        if (!perlinLayer) perlinLayer = dynamic_cast<PerlinNoiseLayer*>(base);
        if (!gameOfLifeLayer) gameOfLifeLayer = dynamic_cast<GameOfLifeLayer*>(base);
    }
}

std::string ofApp::canonicalScenePath(const std::string& path) const {
    std::string normalized = normalizeScenePath(ofTrim(path));
    if (normalized.empty()) {
        return normalized;
    }

    if (isAbsolutePath(normalized)) {
        std::string dataRoot = normalizeScenePath(ofToDataPath("", true));
        if (!dataRoot.empty() && startsWith(normalized, dataRoot)) {
            std::string relative = normalized.substr(dataRoot.size());
            while (!relative.empty() && relative.front() == '/') {
                relative.erase(relative.begin());
            }
            normalized = relative;
        } else {
            return normalized;
        }
    }

    if (normalized.find('/') == std::string::npos) {
        normalized = "layers/scenes/" + normalized;
    }
    if (ofFilePath::getFileExt(normalized).empty()) {
        normalized += ".json";
    }
    return normalizeScenePath(normalized);
}

bool ofApp::isAutosaveScenePath(const std::string& path) const {
    return canonicalScenePath(path) == normalizeScenePath(kSceneAutosavePath);
}

std::string ofApp::sceneDisplayNameForPath(const std::string& path) const {
    std::string canonical = canonicalScenePath(path);
    if (canonical.empty()) {
        return std::string();
    }
    return ofFilePath::getBaseName(canonical);
}

const char* ofApp::sceneLoadPhaseLabel(SceneLoadPhase phase) const {
    switch (phase) {
        case SceneLoadPhase::Idle: return "idle";
        case SceneLoadPhase::Requested: return "request";
        case SceneLoadPhase::Parsing: return "parse";
        case SceneLoadPhase::Validating: return "validate";
        case SceneLoadPhase::Building: return "build";
        case SceneLoadPhase::Applying: return "apply";
        case SceneLoadPhase::Publishing: return "publish";
        case SceneLoadPhase::Succeeded: return "success";
        case SceneLoadPhase::Failed: return "failure";
    }
    return "unknown";
}

void ofApp::beginSceneLoadPhase(SceneLoadPhase phase,
                                const std::string& canonicalPath,
                                const std::string& status) {
    const uint64_t nowMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    const SceneLoadPhase previousPhase = sceneLoadPhase_;
    if (sceneLoadUiSnapshot_.active &&
        sceneLoadUiSnapshot_.phaseStartedMs != 0 &&
        previousPhase != SceneLoadPhase::Idle &&
        previousPhase != phase) {
        const uint64_t elapsedMs = nowMs - sceneLoadUiSnapshot_.phaseStartedMs;
        sceneLoadUiSnapshot_.lastPhaseElapsedMs = elapsedMs;
        ofLogNotice("Scene") << "scene load " << sceneLoadPhaseLabel(previousPhase)
                             << " elapsed: " << elapsedMs << " ms";
    }
    if (phase == SceneLoadPhase::Requested || !sceneLoadUiSnapshot_.active) {
        sceneLoadUiSnapshot_ = SceneLoadUiSnapshot{};
        sceneLoadUiSnapshot_.startedMs = nowMs;
        sceneLoadUiSnapshot_.scenePath = canonicalPath;
        sceneLoadUiSnapshot_.displayName = sceneDisplayNameForPath(canonicalPath);
    }
    sceneLoadPhase_ = phase;
    sceneLoadUiSnapshot_.active = phase != SceneLoadPhase::Idle &&
                                  phase != SceneLoadPhase::Succeeded &&
                                  phase != SceneLoadPhase::Failed;
    sceneLoadUiSnapshot_.phase = phase;
    sceneLoadUiSnapshot_.scenePath = canonicalPath;
    if (sceneLoadUiSnapshot_.displayName.empty()) {
        sceneLoadUiSnapshot_.displayName = sceneDisplayNameForPath(canonicalPath);
    }
    sceneLoadUiSnapshot_.status = status.empty() ? sceneLoadPhaseLabel(phase) : status;
    sceneLoadUiSnapshot_.updatedMs = nowMs;
    sceneLoadUiSnapshot_.phaseStartedMs = nowMs;
    sceneLoadUiSnapshot_.totalElapsedMs = sceneLoadUiSnapshot_.startedMs == 0
        ? 0
        : nowMs - sceneLoadUiSnapshot_.startedMs;
    if (phase != SceneLoadPhase::Failed) {
        sceneLoadUiSnapshot_.failure.clear();
    }
    ofLogNotice("Scene") << "scene load " << sceneLoadPhaseLabel(phase)
                         << ": " << canonicalPath
                         << (status.empty() ? std::string() : " - " + status);
}

void ofApp::finishSceneLoad(bool success,
                            const std::string& canonicalPath,
                            const std::string& status) {
    const uint64_t nowMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    if (sceneLoadUiSnapshot_.active &&
        sceneLoadUiSnapshot_.phaseStartedMs != 0 &&
        sceneLoadPhase_ != SceneLoadPhase::Idle) {
        const uint64_t elapsedMs = nowMs - sceneLoadUiSnapshot_.phaseStartedMs;
        sceneLoadUiSnapshot_.lastPhaseElapsedMs = elapsedMs;
        ofLogNotice("Scene") << "scene load " << sceneLoadPhaseLabel(sceneLoadPhase_)
                             << " elapsed: " << elapsedMs << " ms";
    }
    sceneLoadPhase_ = success ? SceneLoadPhase::Succeeded : SceneLoadPhase::Failed;
    if (sceneLoadUiSnapshot_.startedMs == 0) {
        sceneLoadUiSnapshot_.startedMs = nowMs;
    }
    const uint64_t totalElapsedMs = nowMs - sceneLoadUiSnapshot_.startedMs;
    sceneLoadUiSnapshot_.active = false;
    sceneLoadUiSnapshot_.phase = sceneLoadPhase_;
    sceneLoadUiSnapshot_.scenePath = canonicalPath;
    if (sceneLoadUiSnapshot_.displayName.empty()) {
        sceneLoadUiSnapshot_.displayName = sceneDisplayNameForPath(canonicalPath);
    }
    sceneLoadUiSnapshot_.status = status.empty() ? sceneLoadPhaseLabel(sceneLoadPhase_) : status;
    sceneLoadUiSnapshot_.failure = success ? std::string() : sceneLoadUiSnapshot_.status;
    sceneLoadUiSnapshot_.updatedMs = nowMs;
    sceneLoadUiSnapshot_.phaseStartedMs = nowMs;
    sceneLoadUiSnapshot_.totalElapsedMs = totalElapsedMs;

    if (success) {
        ofLogNotice("Scene") << "scene load success: " << canonicalPath
                             << (status.empty() ? std::string() : " - " + status)
                             << " (" << totalElapsedMs << " ms total)";
    } else {
        ofLogWarning("Scene") << "scene load failure: " << canonicalPath
                              << (status.empty() ? std::string() : " - " + status)
                              << " (" << totalElapsedMs << " ms total)";
    }
    sceneLoadPhase_ = SceneLoadPhase::Idle;
}

bool ofApp::sceneLoadInProgress() const {
    return sceneLoadUiSnapshot_.active && sceneLoadPhase_ != SceneLoadPhase::Idle;
}

bool ofApp::parseSceneLoadPlan(const std::string& canonicalPath,
                               SceneApplyPlan& plan,
                               std::string& error) const {
    plan = SceneApplyPlan{};
    plan.canonicalPath = canonicalPath;
    plan.fullPath = sceneFilesystemPath(canonicalPath);

    ofFile file(plan.fullPath);
    if (!file.exists()) {
        error = "scene file not found: " + plan.fullPath;
        return false;
    }

    try {
        plan.scene = ofLoadJson(plan.fullPath);
    } catch (const std::exception& e) {
        error = std::string("failed to parse scene: ") + e.what();
        return false;
    }
    return true;
}

bool ofApp::validateSceneConsoleLayout(const ofJson& consoleNode,
                                       std::string& error) const {
    if (!consoleNode.is_object()) {
        error = "console layout must be an object";
        return false;
    }
    if (!consoleNode.contains("slots") || !consoleNode["slots"].is_array()) {
        error = "console layout missing slots array";
        return false;
    }

    std::unordered_set<int> seenIndexes;
    std::unordered_set<std::string> seenAssets;
    for (const auto& slotNode : consoleNode["slots"]) {
        if (!slotNode.is_object()) {
            error = "console slot must be an object";
            return false;
        }
        if (!slotNode.contains("index") || !slotNode["index"].is_number_integer()) {
            error = "console slot missing integer index";
            return false;
        }
        if (!slotNode.contains("assetId") || !slotNode["assetId"].is_string()) {
            error = "console slot missing assetId";
            return false;
        }

        const int index = slotNode["index"].get<int>();
        if (index < 1 || index > 8) {
            error = "console slot index out of range: " + ofToString(index);
            return false;
        }
        if (!seenIndexes.insert(index).second) {
            error = "duplicate console slot index: " + ofToString(index);
            return false;
        }

        const std::string assetId = slotNode["assetId"].get<std::string>();
        if (assetId.empty()) {
            error = "console slot assetId is empty";
            return false;
        }
        if (!seenAssets.insert(assetId).second) {
            error = "duplicate console asset in scene: " + assetId;
            return false;
        }

        const auto* entry = layerLibrary.find(assetId);
        if (!entry) {
            error = "console asset not found: " + assetId;
            return false;
        }
        if (!isFxType(entry->type) && !isUiOverlayType(entry->type)) {
            auto probe = LayerFactory::instance().create(entry->type);
            if (!probe) {
                error = "layer factory cannot create type '" + entry->type + "' for asset " + assetId;
                return false;
            }
        }
    }
    return true;
}

bool ofApp::buildSceneApplyPlan(const std::string& canonicalPath,
                                const ofJson& scene,
                                SceneApplyPlan& plan,
                                std::string& error) const {
    if (!scene.is_object()) {
        error = "scene document must be an object";
        return false;
    }

    plan.canonicalPath = canonicalPath;
    plan.fullPath = sceneFilesystemPath(canonicalPath);
    plan.scene = scene;
    plan.restoreSecondaryDisplay = secondaryDisplay_.enabled && secondaryDisplay_.active;
    plan.restoreControllerFocusConsole = param_controllerFocusConsole;
    plan.restorePersistenceSuspended = consolePersistenceSuspended_;
    plan.routerSnapshot = ofJson::object();
    plan.slotAssignmentsSnapshot = emptySlotAssignmentsSnapshot();

    if (isAutosaveScenePath(canonicalPath)) {
        plan.activeNamedScenePath.clear();
        if (scene.contains("scene") && scene["scene"].is_object()) {
            const auto& meta = scene["scene"];
            if (meta.contains("source") && meta["source"].is_object()) {
                const auto& source = meta["source"];
                if (source.contains("path") && source["path"].is_string()) {
                    const std::string sourcePath = canonicalScenePath(source["path"].get<std::string>());
                    if (!sourcePath.empty() &&
                        !isAutosaveScenePath(sourcePath) &&
                        startsWith(sourcePath, "layers/scenes/") &&
                        ofFile(sceneFilesystemPath(sourcePath)).exists()) {
                        plan.activeNamedScenePath = sourcePath;
                    } else if (!sourcePath.empty()) {
                        ofLogWarning("Scene") << "ignoring invalid autosave source scene path: "
                                              << sourcePath;
                    }
                }
            }
        }
    } else {
        plan.activeNamedScenePath = canonicalPath;
    }

    if (scene.contains("console")) {
        if (!validateSceneConsoleLayout(scene["console"], error)) {
            return false;
        }
        plan.consoleLayoutDefined = true;
    }

    if (scene.contains(kSceneMappingsKey)) {
        if (!scene[kSceneMappingsKey].is_object()) {
            error = "scene mappings must be an object";
            return false;
        }
        const auto& mappingsNode = scene[kSceneMappingsKey];
        if (mappingsNode.contains(kSceneRouterMappingsKey)) {
            if (!mappingsNode[kSceneRouterMappingsKey].is_object()) {
                error = "scene router mappings must be an object";
                return false;
            }
            plan.routerSnapshot = mappingsNode[kSceneRouterMappingsKey];
        }
        if (mappingsNode.contains(kSceneSlotAssignmentsKey)) {
            if (!mappingsNode[kSceneSlotAssignmentsKey].is_object()) {
                error = "scene slot assignments must be an object";
                return false;
            }
            plan.slotAssignmentsSnapshot = mappingsNode[kSceneSlotAssignmentsKey];
        }
    }

    return true;
}

ofApp::SceneLoadRollbackSnapshot ofApp::captureSceneRollbackSnapshot(const std::string& targetCanonicalPath) const {
    SceneLoadRollbackSnapshot snapshot;
    snapshot.activeScenePath = activeScenePath_;
    snapshot.activeNamedScenePath = activeNamedScenePath_;
    const std::string snapshotPath = activeScenePath_.empty()
        ? (targetCanonicalPath.empty() ? canonicalScenePath(kSceneAutosavePath) : targetCanonicalPath)
        : activeScenePath_;
    snapshot.scene = encodeSceneJson(snapshotPath);
    snapshot.routerSnapshot = midi.exportMappingSnapshot();
    const ofJson slotSnapshot = loadJsonSnapshotIfExists(controlHubSlotAssignmentsPath());
    snapshot.slotAssignmentsSnapshot = slotSnapshot.is_object()
        ? slotSnapshot
        : emptySlotAssignmentsSnapshot();
    snapshot.secondaryDisplayEnabled = secondaryDisplay_.enabled;
    snapshot.paramSecondaryDisplayEnabled = param_secondaryDisplayEnabled;
    snapshot.paramControllerFocusConsole = param_controllerFocusConsole;
    snapshot.controllerFocusConsole = controllerFocus_.preferConsole;
    snapshot.secondaryDisplayRenderPaused = secondaryDisplayRenderPaused_;
    snapshot.consolePersistenceSuspended = consolePersistenceSuspended_;
    return snapshot;
}

ofJson ofApp::encodeSceneJson(const std::string& path) const {
    const std::string canonicalPath = canonicalScenePath(path);
    const bool autosave = isAutosaveScenePath(canonicalPath);
    std::string sourcePath = autosave ? canonicalScenePath(activeNamedScenePath_) : canonicalPath;

    ofJson scene;
    ofJson sceneMeta;
    sceneMeta["schemaVersion"] = kSceneSchemaVersion;
    sceneMeta["savedAt"] = ofGetTimestampString("%Y-%m-%dT%H:%M:%S%z");

    ofJson storage;
    storage["kind"] = autosave ? "autosave" : "named";
    storage["path"] = canonicalPath;
    storage["name"] = sceneDisplayNameForPath(canonicalPath);
    sceneMeta["storage"] = std::move(storage);

    if (!sourcePath.empty()) {
        ofJson source;
        source["path"] = sourcePath;
        source["name"] = sceneDisplayNameForPath(sourcePath);
        sceneMeta["source"] = std::move(source);
    }
    scene["scene"] = std::move(sceneMeta);

    writeConsoleLayoutToScene(scene);

    auto saveValue = [&](const std::string& id, ofJson& target) {
        auto* floatParam = paramRegistry.findFloat(id);
        if (floatParam && floatParam->value) {
            target[id] = encodeFloatParam(*floatParam);
            return;
        }
        auto* boolParam = paramRegistry.findBool(id);
        if (boolParam && boolParam->value) {
            target[id] = encodeBoolParam(*boolParam);
            return;
        }
        auto* stringParam = paramRegistry.findString(id);
        if (stringParam && stringParam->value) {
            target[id] = stringParam->baseValue;
        }
    };

    ofJson globalsJson;
      const std::vector<std::string> globalIds = {
          "transport.bpm",
          "globals.speed",
          "fx.master",
          "camera.dist",
          "overlay.text.content",
          "overlay.text.font",
          "overlay.text.size",
          "overlay.text.color.r",
          "overlay.text.color.g",
          "overlay.text.color.b"
      };
    for (const auto& id : globalIds) {
        saveValue(id, globalsJson);
    }
    if (!globalsJson.empty()) {
        scene["globals"] = globalsJson;
    }

    ofJson effectsJson;
    auto assignEffectParam = [&](ofJson& parent, const std::string& key, const std::string& paramId) {
        auto* param = paramRegistry.findFloat(paramId);
        if (param && param->value) {
            parent[key] = encodeFloatParam(*param);
        }
    };
    assignEffectParam(effectsJson["dither"], "route", "effects.dither.route");
    assignEffectParam(effectsJson["dither"], "cellSize", "effects.dither.cellSize");
    assignEffectParam(effectsJson["ascii"], "route", "effects.ascii.route");
    assignEffectParam(effectsJson["ascii"], "colorMode", "effects.ascii.colorMode");
    assignEffectParam(effectsJson["ascii"], "characterSet", "effects.ascii.characterSet");
    assignEffectParam(effectsJson["ascii"], "aspectMode", "effects.ascii.aspect");
    assignEffectParam(effectsJson["ascii"], "padding", "effects.ascii.padding");
    assignEffectParam(effectsJson["ascii"], "gamma", "effects.ascii.gamma");
    assignEffectParam(effectsJson["ascii"], "jitter", "effects.ascii.jitter");
    assignEffectParam(effectsJson["ascii"], "block", "effects.ascii.block");
    assignEffectParam(effectsJson["asciiSupersample"], "route", "effects.asciiSupersample.route");
    assignEffectParam(effectsJson["asciiSupersample"], "colorMode", "effects.asciiSupersample.colorMode");
    assignEffectParam(effectsJson["asciiSupersample"], "characterSet", "effects.asciiSupersample.characterSet");
    assignEffectParam(effectsJson["asciiSupersample"], "aspectMode", "effects.asciiSupersample.aspect");
    assignEffectParam(effectsJson["asciiSupersample"], "padding", "effects.asciiSupersample.padding");
    assignEffectParam(effectsJson["asciiSupersample"], "gamma", "effects.asciiSupersample.gamma");
    assignEffectParam(effectsJson["asciiSupersample"], "jitter", "effects.asciiSupersample.jitter");
    assignEffectParam(effectsJson["asciiSupersample"], "block", "effects.asciiSupersample.block");
    assignEffectParam(effectsJson["crt"], "route", "effects.crt.route");
    assignEffectParam(effectsJson["crt"], "scanline", "effects.crt.scanline");
    assignEffectParam(effectsJson["crt"], "vignette", "effects.crt.vignette");
    assignEffectParam(effectsJson["crt"], "bleed", "effects.crt.bleed");
    assignEffectParam(effectsJson["crt"], "softness", "effects.crt.softness");
    assignEffectParam(effectsJson["crt"], "glow", "effects.crt.glow");
    assignEffectParam(effectsJson["crt"], "perChannelOffset", "effects.crt.perChannelOffset");
    assignEffectParam(effectsJson["crt"], "scanlineJitter", "effects.crt.scanlineJitter");
    assignEffectParam(effectsJson["crt"], "subpixelDensity", "effects.crt.subpixelDensity");
    assignEffectParam(effectsJson["crt"], "subpixelAspect", "effects.crt.subpixelAspect");
    assignEffectParam(effectsJson["crt"], "subpixelPadding", "effects.crt.subpixelPadding");
    assignEffectParam(effectsJson["crt"], "rgbMisalignment", "effects.crt.rgbMisalignment");
    assignEffectParam(effectsJson["crt"], "syncTear", "effects.crt.syncTear");
    assignEffectParam(effectsJson["crt"], "trackingWobble", "effects.crt.trackingWobble");
    assignEffectParam(effectsJson["crt"], "lumaNoise", "effects.crt.lumaNoise");
    assignEffectParam(effectsJson["crt"], "headSwitch", "effects.crt.headSwitch");
    assignEffectParam(effectsJson["motion"], "route", "effects.motion.route");
    assignEffectParam(effectsJson["motion"], "threshold", "effects.motion.threshold");
    assignEffectParam(effectsJson["motion"], "boost", "effects.motion.boost");
    assignEffectParam(effectsJson["motion"], "mix", "effects.motion.mix");
    assignEffectParam(effectsJson["motion"], "softness", "effects.motion.softness");
    assignEffectParam(effectsJson["motion"], "fadeBeats", "effects.motion.fadeBeats");
    assignEffectParam(effectsJson["motion"], "blur", "effects.motion.blur");
    assignEffectParam(effectsJson["motion"], "alphaMix", "effects.motion.alphaMix");
    assignEffectParam(effectsJson["motion"], "headColorR", "effects.motion.headColorR");
    assignEffectParam(effectsJson["motion"], "headColorG", "effects.motion.headColorG");
    assignEffectParam(effectsJson["motion"], "headColorB", "effects.motion.headColorB");
    assignEffectParam(effectsJson["motion"], "tailColorR", "effects.motion.tailColorR");
    assignEffectParam(effectsJson["motion"], "tailColorG", "effects.motion.tailColorG");
    assignEffectParam(effectsJson["motion"], "tailColorB", "effects.motion.tailColorB");
    assignEffectParam(effectsJson["motion"], "tailOpacity", "effects.motion.tailOpacity");
    scene["effects"] = effectsJson;

    ofJson banksJson;
    auto globalBanks = bankRegistry.globalBanks();
    if (!globalBanks.empty()) {
        banksJson["global"] = BankRegistry::definitionsToJson(globalBanks);
    }
    auto sceneBanks = bankRegistry.sceneBanks();
    if (!sceneBanks.empty()) {
        banksJson["scene"] = BankRegistry::definitionsToJson(sceneBanks);
    }
    auto layerBanks = bankRegistry.layerBanks();
    if (!layerBanks.empty()) {
        ofJson layersNode = ofJson::object();
        for (const auto& kv : layerBanks) {
            if (!kv.second.empty()) {
                layersNode[kv.first] = BankRegistry::definitionsToJson(kv.second);
            }
        }
        if (!layersNode.empty()) {
            banksJson["layers"] = std::move(layersNode);
        }
    }
    if (!banksJson.empty()) {
        scene["banks"] = std::move(banksJson);
    }

    ofJson mappingsJson = ofJson::object();
    mappingsJson[kSceneRouterMappingsKey] = midi.exportMappingSnapshot();
    const ofJson slotAssignmentsSnapshot = loadJsonSnapshotIfExists(controlHubSlotAssignmentsPath());
    mappingsJson[kSceneSlotAssignmentsKey] = slotAssignmentsSnapshot.is_object()
        ? slotAssignmentsSnapshot
        : emptySlotAssignmentsSnapshot();
    scene[kSceneMappingsKey] = std::move(mappingsJson);

    return scene;
}

bool ofApp::writeSceneJson(const std::string& path, const ofJson& scene) const {
    const std::string canonicalPath = canonicalScenePath(path);
    std::string fullPath = sceneFilesystemPath(canonicalPath);
    ofDirectory dir(ofFilePath::getEnclosingDirectory(fullPath));
    if (!dir.exists() && !dir.create(true)) {
        ofLogWarning("Scene") << "failed to create scene directory for " << fullPath;
        return false;
    }

    const std::string tmpPath = fullPath + ".tmp";
    {
        ofFile out(tmpPath, ofFile::WriteOnly, true);
        if (!out.is_open()) {
            ofLogWarning("Scene") << "failed to open temp scene file " << tmpPath;
            return false;
        }
        out << scene.dump(2);
    }

    std::remove(fullPath.c_str());
    if (std::rename(tmpPath.c_str(), fullPath.c_str()) != 0) {
        ofLogWarning("Scene") << "failed to rename " << tmpPath << " -> " << fullPath;
        ofFile::removeFile(tmpPath, false);
        return false;
    }
    return true;
}

bool ofApp::applyScenePlan(SceneApplyPlan& plan) {
    const ofJson& scene = plan.scene;
    const bool persistenceSuspendedBeforeApply = consolePersistenceSuspended_;
    consolePersistenceSuspended_ = true;
    struct ConsolePersistenceRestore {
        bool& target;
        bool value;
        ~ConsolePersistenceRestore() {
            target = value;
        }
    } restoreConsolePersistence{consolePersistenceSuspended_, persistenceSuspendedBeforeApply};

    bool consoleApplied = false;
    if (scene.contains("console") && scene["console"].is_object()) {
        consoleApplied = loadConsoleLayoutFromScene(scene["console"]);
        if (!consoleApplied) {
            return false;
        }
    }
    if (consoleApplied) {
        refreshLayerReferences();
    }

    for (const auto& entry : paramRegistry.floats()) {
        paramRegistry.clearFloatModifiers(entry.meta.id);
    }
    for (const auto& entry : paramRegistry.bools()) {
        paramRegistry.clearBoolModifiers(entry.meta.id);
    }

    bankRegistry.clearSceneBanks();
    bankRegistry.clearLayerBanks();

    if (scene.contains("banks") && scene["banks"].is_object()) {
        const auto& banksNode = scene["banks"];
        if (banksNode.contains("global") && banksNode["global"].is_array()) {
            auto defs = BankRegistry::definitionsFromJson(banksNode["global"], BankRegistry::Scope::kGlobal);
            if (!defs.empty()) {
                bankRegistry.setGlobalBanks(std::move(defs));
            }
        }
        if (banksNode.contains("scene") && banksNode["scene"].is_array()) {
            bankRegistry.setSceneBanks(BankRegistry::definitionsFromJson(banksNode["scene"], BankRegistry::Scope::kScene));
        }
        if (banksNode.contains("layers") && banksNode["layers"].is_object()) {
            for (auto it = banksNode["layers"].begin(); it != banksNode["layers"].end(); ++it) {
                if (!it.value().is_array()) continue;
                bankRegistry.setLayerBanks(it.key(), BankRegistry::definitionsFromJson(it.value(), BankRegistry::Scope::kLayer));
            }
        }
    }

    if (bankRegistry.globalBanks().empty()) {
        configureDefaultBanks();
    }

    auto applyParameterNode = [&](const std::string& paramId, const ofJson& node) {
        if (auto* fp = paramRegistry.findFloat(paramId)) {
            float base = parseBaseFloat(node, fp->baseValue);
            if (paramId == "effects.motion.headColorR" ||
                paramId == "effects.motion.headColorG" ||
                paramId == "effects.motion.headColorB" ||
                paramId == "effects.motion.tailColorR" ||
                paramId == "effects.motion.tailColorG" ||
                paramId == "effects.motion.tailColorB") {
                if (base <= 1.0f) {
                    base *= 255.0f;
                }
                base = ofClamp(base, 0.0f, 255.0f);
            } else if (paramId == "effects.motion.tailOpacity") {
                if (base <= 1.0f) {
                    base *= 100.0f;
                }
                base = ofClamp(base, 0.0f, 100.0f);
            }
            fp->baseValue = base;
            fp->applyBaseToLive();
            paramRegistry.clearFloatModifiers(paramId);
        if (node.is_object() && node.contains("modifiers")) {
                loadFloatModifiersFromJson(paramRegistry, paramId, node["modifiers"]);
            }
        } else if (auto* bp = paramRegistry.findBool(paramId)) {
            bool base = parseBaseBool(node, bp->baseValue);
            bp->baseValue = base;
            bp->applyBaseToLive();
            paramRegistry.clearBoolModifiers(paramId);
            if (node.is_object() && node.contains("modifiers")) {
                loadBoolModifiersFromJson(paramRegistry, paramId, node["modifiers"]);
            }
        } else if (auto* sp = paramRegistry.findString(paramId)) {
            std::string base = parseBaseString(node, sp->baseValue);
            sp->baseValue = base;
            sp->applyBaseToLive();
        }
    };

    if (consoleApplied && scene.contains("console") && scene["console"].is_object()) {
        const auto& consoleNode = scene["console"];
        if (consoleNode.contains("slots") && consoleNode["slots"].is_array()) {
            for (const auto& slotNode : consoleNode["slots"]) {
                if (!slotNode.contains("parameters") || !slotNode["parameters"].is_object()) continue;
                for (auto it = slotNode["parameters"].begin(); it != slotNode["parameters"].end(); ++it) {
                    applyParameterNode(it.key(), it.value());
                }
            }
        }
    }


    refreshLayerReferences();

    auto applyValue = [&](const std::string& id, const ofJson& value) {
        applyParameterNode(id, value);
    };

    if (scene.contains("globals") && scene["globals"].is_object()) {
        const auto& globals = scene["globals"];
        for (auto it = globals.begin(); it != globals.end(); ++it) {
            const std::string& paramId = it.key();
            if (paramId == "ui.hud" ||
                paramId == "ui.console.visible" ||
                paramId == "ui.hub.visible" ||
                paramId == "ui.menu.visible" ||
                paramId == "console.dual_display.mode") {
                continue;
            }
            applyValue(it.key(), it.value());
        }
    }

    if (scene.contains("effects") && scene["effects"].is_object()) {
        const auto& effects = scene["effects"];
        if (effects.contains("dither") && effects["dither"].contains("route")) {
            applyValue("effects.dither.route", effects["dither"]["route"]);
            if (effects["dither"].contains("cellSize")) {
                applyValue("effects.dither.cellSize", effects["dither"]["cellSize"]);
            }
        }
        if (effects.contains("ascii")) {
            const auto& ascii = effects["ascii"];
            if (ascii.contains("route")) applyValue("effects.ascii.route", ascii["route"]);
            if (ascii.contains("block")) applyValue("effects.ascii.block", ascii["block"]);
            if (ascii.contains("colorMode")) {
                applyValue("effects.ascii.colorMode", ascii["colorMode"]);
            }
            if (ascii.contains("characterSet")) {
                applyValue("effects.ascii.characterSet", ascii["characterSet"]);
            } else {
                applyValue("effects.ascii.characterSet", 2.0f);
            }
            if (ascii.contains("aspectMode")) {
                applyValue("effects.ascii.aspect", ascii["aspectMode"]);
            } else {
                applyValue("effects.ascii.aspect", 0.0f);
            }
            if (ascii.contains("padding")) {
                applyValue("effects.ascii.padding", ascii["padding"]);
            } else {
                applyValue("effects.ascii.padding", 0.0f);
            }
            if (ascii.contains("gamma")) {
                applyValue("effects.ascii.gamma", ascii["gamma"]);
            } else {
                applyValue("effects.ascii.gamma", 1.0f);
            }
            if (ascii.contains("jitter")) {
                applyValue("effects.ascii.jitter", ascii["jitter"]);
            } else {
                applyValue("effects.ascii.jitter", 0.0f);
            }
        }
        if (effects.contains("asciiSupersample")) {
            const auto& asciiSs = effects["asciiSupersample"];
            if (asciiSs.contains("route")) applyValue("effects.asciiSupersample.route", asciiSs["route"]);
            if (asciiSs.contains("block")) applyValue("effects.asciiSupersample.block", asciiSs["block"]);
            if (asciiSs.contains("colorMode")) {
                applyValue("effects.asciiSupersample.colorMode", asciiSs["colorMode"]);
            }
            if (asciiSs.contains("characterSet")) {
                applyValue("effects.asciiSupersample.characterSet", asciiSs["characterSet"]);
            } else {
                applyValue("effects.asciiSupersample.characterSet", 2.0f);
            }
            if (asciiSs.contains("aspectMode")) {
                applyValue("effects.asciiSupersample.aspect", asciiSs["aspectMode"]);
            } else {
                applyValue("effects.asciiSupersample.aspect", 0.0f);
            }
            if (asciiSs.contains("padding")) {
                applyValue("effects.asciiSupersample.padding", asciiSs["padding"]);
            } else {
                applyValue("effects.asciiSupersample.padding", 0.0f);
            }
            if (asciiSs.contains("gamma")) {
                applyValue("effects.asciiSupersample.gamma", asciiSs["gamma"]);
            } else {
                applyValue("effects.asciiSupersample.gamma", 1.0f);
            }
            if (asciiSs.contains("jitter")) {
                applyValue("effects.asciiSupersample.jitter", asciiSs["jitter"]);
            } else {
                applyValue("effects.asciiSupersample.jitter", 0.0f);
            }
        }
        if (effects.contains("crt")) {
            const auto& crt = effects["crt"];
            if (crt.contains("route")) applyValue("effects.crt.route", crt["route"]);
            if (crt.contains("scanline")) applyValue("effects.crt.scanline", crt["scanline"]);
            if (crt.contains("vignette")) applyValue("effects.crt.vignette", crt["vignette"]);
            if (crt.contains("bleed")) applyValue("effects.crt.bleed", crt["bleed"]);
            if (crt.contains("softness")) applyValue("effects.crt.softness", crt["softness"]);
            if (crt.contains("glow")) applyValue("effects.crt.glow", crt["glow"]);
            if (crt.contains("perChannelOffset")) applyValue("effects.crt.perChannelOffset", crt["perChannelOffset"]);
            if (crt.contains("scanlineJitter")) applyValue("effects.crt.scanlineJitter", crt["scanlineJitter"]);
            if (crt.contains("subpixelDensity")) applyValue("effects.crt.subpixelDensity", crt["subpixelDensity"]);
            if (crt.contains("subpixelAspect")) applyValue("effects.crt.subpixelAspect", crt["subpixelAspect"]);
            if (crt.contains("subpixelPadding")) applyValue("effects.crt.subpixelPadding", crt["subpixelPadding"]);
            if (crt.contains("rgbMisalignment")) applyValue("effects.crt.rgbMisalignment", crt["rgbMisalignment"]);
            if (crt.contains("syncTear")) applyValue("effects.crt.syncTear", crt["syncTear"]);
            if (crt.contains("trackingWobble")) applyValue("effects.crt.trackingWobble", crt["trackingWobble"]);
            if (crt.contains("lumaNoise")) applyValue("effects.crt.lumaNoise", crt["lumaNoise"]);
            if (crt.contains("headSwitch")) applyValue("effects.crt.headSwitch", crt["headSwitch"]);
        }
        if (effects.contains("motion")) {
            const auto& motion = effects["motion"];
            if (motion.contains("route")) applyValue("effects.motion.route", motion["route"]);
            if (motion.contains("threshold")) applyValue("effects.motion.threshold", motion["threshold"]);
            if (motion.contains("boost")) applyValue("effects.motion.boost", motion["boost"]);
            if (motion.contains("mix")) applyValue("effects.motion.mix", motion["mix"]);
            if (motion.contains("softness")) applyValue("effects.motion.softness", motion["softness"]);
            if (motion.contains("fadeBeats")) applyValue("effects.motion.fadeBeats", motion["fadeBeats"]);
            if (motion.contains("blur")) applyValue("effects.motion.blur", motion["blur"]);
            if (motion.contains("alphaMix")) applyValue("effects.motion.alphaMix", motion["alphaMix"]);
            if (motion.contains("headColorR")) applyValue("effects.motion.headColorR", motion["headColorR"]);
            if (motion.contains("headColorG")) applyValue("effects.motion.headColorG", motion["headColorG"]);
            if (motion.contains("headColorB")) applyValue("effects.motion.headColorB", motion["headColorB"]);
            if (motion.contains("tailColorR")) applyValue("effects.motion.tailColorR", motion["tailColorR"]);
            if (motion.contains("tailColorG")) applyValue("effects.motion.tailColorG", motion["tailColorG"]);
            if (motion.contains("tailColorB")) applyValue("effects.motion.tailColorB", motion["tailColorB"]);
            if (motion.contains("tailOpacity")) applyValue("effects.motion.tailOpacity", motion["tailOpacity"]);
        }
    }

    paramRegistry.evaluateAllModifiers();
    ensureActiveBankValid();
    plan.consoleApplied = consoleApplied;
    return true;
}

bool ofApp::publishScenePlan(const SceneApplyPlan& plan,
                             const SceneLoadRollbackSnapshot&,
                             std::string& error) {
    if (!midi.importMappingSnapshot(plan.routerSnapshot, true)) {
        error = "failed to import router mapping snapshot";
        return false;
    }
    syncActiveFxWithConsoleSlots();
    if (plan.restoreSecondaryDisplay) {
        secondaryDisplay_.enabled = true;
        param_secondaryDisplayEnabled = true;
        param_controllerFocusConsole = plan.restoreControllerFocusConsole;
        controllerFocus_.preferConsole = plan.restoreControllerFocusConsole;
        if (paramRegistry.findBool("console.secondary_display.enabled")) {
            paramRegistry.setBoolBase("console.secondary_display.enabled", true, true);
        }
        if (paramRegistry.findBool("console.controller.focus_console")) {
            paramRegistry.setBoolBase("console.controller.focus_console", plan.restoreControllerFocusConsole, true);
        }
        secondaryDisplayRenderPaused_ = false;
        handleControllerFocusParamChange();
    } else {
        secondaryDisplayRenderPaused_ = false;
    }

    const std::string slotAssignmentsPath = controlHubSlotAssignmentsPath();
    if (!writeJsonSnapshotAtomically(slotAssignmentsPath, plan.slotAssignmentsSnapshot)) {
        error = "failed to apply slot assignment snapshot from scene";
        return false;
    }
    if (controlMappingHub) {
        controlMappingHub->setSlotAssignmentsPath(slotAssignmentsPath);
    }
    activeScenePath_ = plan.canonicalPath;
    activeNamedScenePath_ = plan.activeNamedScenePath;
    consolePersistenceSuspended_ = plan.restorePersistenceSuspended;
    if (plan.consoleApplied) {
        persistConsoleAssignments();
    }
    return true;
}

bool ofApp::rollbackSceneLoad(const SceneLoadRollbackSnapshot& snapshot,
                              const std::string& failedCanonicalPath,
                              const std::string& reason,
                              bool restorePersistedFiles) {
    ofLogWarning("Scene") << "rolling back scene load for " << failedCanonicalPath
                          << (reason.empty() ? std::string() : " - " + reason);

    bool restored = true;
    SceneApplyPlan rollbackPlan;
    std::string error;
    const std::string rollbackPath = snapshot.activeScenePath.empty()
        ? canonicalScenePath(kSceneAutosavePath)
        : snapshot.activeScenePath;
    if (buildSceneApplyPlan(rollbackPath, snapshot.scene, rollbackPlan, error)) {
        rollbackPlan.restorePersistenceSuspended = true;
        const bool suspendedBeforeRollback = consolePersistenceSuspended_;
        consolePersistenceSuspended_ = true;
        if (!applyScenePlan(rollbackPlan)) {
            restored = false;
        }
        consolePersistenceSuspended_ = suspendedBeforeRollback;
    } else {
        restored = false;
        ofLogWarning("Scene") << "rollback plan build failed: " << error;
    }

    activeScenePath_ = snapshot.activeScenePath;
    activeNamedScenePath_ = snapshot.activeNamedScenePath;
    secondaryDisplay_.enabled = snapshot.secondaryDisplayEnabled;
    param_secondaryDisplayEnabled = snapshot.paramSecondaryDisplayEnabled;
    param_controllerFocusConsole = snapshot.paramControllerFocusConsole;
    controllerFocus_.preferConsole = snapshot.controllerFocusConsole;
    secondaryDisplayRenderPaused_ = snapshot.secondaryDisplayRenderPaused;
    consolePersistenceSuspended_ = snapshot.consolePersistenceSuspended;
    if (paramRegistry.findBool("console.secondary_display.enabled")) {
        paramRegistry.setBoolBase("console.secondary_display.enabled", param_secondaryDisplayEnabled, true);
    }
    if (paramRegistry.findBool("console.controller.focus_console")) {
        paramRegistry.setBoolBase("console.controller.focus_console", param_controllerFocusConsole, true);
    }
    if (!midi.importMappingSnapshot(snapshot.routerSnapshot, true)) {
        restored = false;
    }
    if (restorePersistedFiles) {
        const std::string slotAssignmentsPath = controlHubSlotAssignmentsPath();
        if (!writeJsonSnapshotAtomically(slotAssignmentsPath, snapshot.slotAssignmentsSnapshot)) {
            restored = false;
            ofLogWarning("Scene") << "failed to restore slot assignment snapshot during rollback";
        }
        if (controlMappingHub) {
            controlMappingHub->setSlotAssignmentsPath(slotAssignmentsPath);
        }
    }
    syncActiveFxWithConsoleSlots();
    handleControllerFocusParamChange();
    refreshLayerReferences();
    return restored;
}

bool ofApp::loadScene(const std::string& path) {
    const std::string canonicalPath = canonicalScenePath(path);
    beginSceneLoadPhase(SceneLoadPhase::Requested, canonicalPath, "request received");

    SceneApplyPlan plan;
    std::string error;

    beginSceneLoadPhase(SceneLoadPhase::Parsing, canonicalPath, "reading scene JSON");
    if (!parseSceneLoadPlan(canonicalPath, plan, error)) {
        finishSceneLoad(false, canonicalPath, error);
        return false;
    }

    beginSceneLoadPhase(SceneLoadPhase::Validating, canonicalPath, "checking scene document");
    beginSceneLoadPhase(SceneLoadPhase::Building, canonicalPath, "building scene apply plan");
    if (!buildSceneApplyPlan(canonicalPath, plan.scene, plan, error)) {
        finishSceneLoad(false, canonicalPath, error);
        return false;
    }

    const SceneLoadRollbackSnapshot rollback = captureSceneRollbackSnapshot(canonicalPath);
    sceneLoadUiSnapshot_.secondaryDisplayWasActive = plan.restoreSecondaryDisplay;
    sceneLoadUiSnapshot_.secondaryDisplayPreserved = plan.restoreSecondaryDisplay && secondaryWindow_ != nullptr;
    sceneLoadUiSnapshot_.controllerFocusConsole = plan.restoreControllerFocusConsole;
    beginSceneLoadPhase(SceneLoadPhase::Applying,
                        canonicalPath,
                        plan.restoreSecondaryDisplay
                            ? "applying scene while preserving Control Window"
                            : "applying scene state");
    secondaryDisplayRenderPaused_ = plan.restoreSecondaryDisplay;
    consolePersistenceSuspended_ = true;
    bool applied = false;
    try {
        applied = applyScenePlan(plan);
    } catch (const std::exception& ex) {
        error = std::string("exception while applying scene state: ") + ex.what();
    } catch (...) {
        error = "unknown exception while applying scene state";
    }
    if (!applied) {
        if (error.empty()) {
            error = "failed to apply scene state";
        }
        rollbackSceneLoad(rollback, canonicalPath, error, false);
        finishSceneLoad(false, canonicalPath, error + "; previous state restored");
        return false;
    }

    beginSceneLoadPhase(SceneLoadPhase::Publishing, canonicalPath, "publishing scene state");
    bool published = false;
    try {
        published = publishScenePlan(plan, rollback, error);
    } catch (const std::exception& ex) {
        error = std::string("exception while publishing scene state: ") + ex.what();
    } catch (...) {
        error = "unknown exception while publishing scene state";
    }
    if (!published) {
        if (error.empty()) {
            error = "failed to publish scene state";
        }
        rollbackSceneLoad(rollback, canonicalPath, error, true);
        finishSceneLoad(false, canonicalPath, error + "; previous state restored");
        return false;
    }

    finishSceneLoad(true,
                    canonicalPath,
                    plan.restoreSecondaryDisplay
                        ? "scene published; Control Window preserved"
                        : "scene published");
    return true;
}

void ofApp::saveScene(const std::string& path) {
    const std::string canonicalPath = canonicalScenePath(path);
    if (writeSceneJson(canonicalPath, encodeSceneJson(canonicalPath))) {
        activeScenePath_ = canonicalPath;
        if (!isAutosaveScenePath(canonicalPath)) {
            activeNamedScenePath_ = canonicalPath;
        }
    }
}

std::vector<ControlMappingHubState::SavedSceneInfo> ofApp::listSavedScenes() const {
    std::vector<ControlMappingHubState::SavedSceneInfo> scenes;
    ofDirectory dir(sceneFilesystemPath("layers/scenes"));
    dir.allowExt("json");
    if (!dir.exists()) {
        return scenes;
    }
    dir.listDir();
    dir.sort();
    scenes.reserve(dir.size());
    for (std::size_t i = 0; i < dir.size(); ++i) {
        const ofFile& file = dir.getFile(static_cast<int>(i));
        if (!file.isFile()) {
            continue;
        }
        ControlMappingHubState::SavedSceneInfo info;
        info.path = canonicalScenePath(file.getAbsolutePath());
        info.id = info.path;
        info.label = sceneDisplayNameForPath(info.path);
        info.active = (!activeNamedScenePath_.empty() && info.id == canonicalScenePath(activeNamedScenePath_));
        if (info.id.empty() || info.label.empty() || isAutosaveScenePath(info.id)) {
            continue;
        }
        scenes.push_back(std::move(info));
    }
    return scenes;
}

bool ofApp::loadSavedSceneById(const std::string& sceneId) {
    const std::string canonicalPath = canonicalScenePath(sceneId);
    if (canonicalPath.empty() || isAutosaveScenePath(canonicalPath)) {
        return false;
    }
    return loadScene(canonicalPath);
}

bool ofApp::saveNamedScene(const std::string& sceneName, bool overwrite) {
    std::string trimmed = ofTrim(sceneName);
    if (trimmed.empty()) {
        return false;
    }

    std::string stem;
    stem.reserve(trimmed.size());
    for (char ch : trimmed) {
        const unsigned char uc = static_cast<unsigned char>(ch);
        if (uc < 32) {
            continue;
        }
        switch (ch) {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*':
            continue;
        default:
            stem.push_back(ch);
            break;
        }
    }
    stem = ofTrim(stem);
    while (!stem.empty() && (stem.back() == '.' || std::isspace(static_cast<unsigned char>(stem.back())))) {
        stem.pop_back();
    }
    if (stem.empty()) {
        return false;
    }

    const std::string canonicalPath = canonicalScenePath("layers/scenes/" + stem + ".json");
    const std::string fullPath = sceneFilesystemPath(canonicalPath);
    if (!overwrite && ofFile::doesFileExist(fullPath)) {
        return false;
    }
    saveScene(canonicalPath);
    return ofFile::doesFileExist(fullPath);
}

bool ofApp::overwriteSavedSceneById(const std::string& sceneId) {
    const std::string canonicalPath = canonicalScenePath(sceneId);
    if (canonicalPath.empty() || isAutosaveScenePath(canonicalPath)) {
        return false;
    }
    const std::string fullPath = sceneFilesystemPath(canonicalPath);
    if (!ofFile::doesFileExist(fullPath)) {
        return false;
    }
    saveScene(canonicalPath);
    return ofFile::doesFileExist(fullPath);
}

void ofApp::configureDefaultBanks() {
    if (!bankRegistry.globalBanks().empty()) {
        return;
    }
    auto makeControl = [](std::string id, std::string label, std::string target, std::string description = std::string()) {
        BankRegistry::Control ctrl;
        ctrl.id = std::move(id);
        ctrl.label = std::move(label);
        ctrl.targetId = std::move(target);
        ctrl.description = std::move(description);
        ctrl.softTakeover = true;
        return ctrl;
    };

    BankRegistry::Definition home;
    home.id = "home";
    home.scope = BankRegistry::Scope::kGlobal;
    home.label = "Home";
    home.controls = {
        makeControl("masterFx", "Master FX", "fx.master", "Master post-processing amount"),
        makeControl("speed", "Speed", "globals.speed", "Global animation speed"),
        makeControl("bpm", "Tempo", "transport.bpm", "Session tempo"),
        makeControl("hud", "HUD Visible", "ui.hud", "Toggle HUD visibility"),
        makeControl("camDist", "Camera Distance", "camera.dist", "Camera distance")
    };

    bankRegistry.setGlobalBanks({home});
}

void ofApp::ensureActiveBankValid() {
    if (bankRegistry.globalBanks().empty()) {
        configureDefaultBanks();
    }
    if (activeMidiBank.empty() || !bankRegistry.hasBank(activeMidiBank)) {
        activeMidiBank = bankRegistry.firstBankId();
        if (activeMidiBank.empty()) {
            auto globals = bankRegistry.globalBanks();
            if (!globals.empty()) {
                activeMidiBank = globals.front().id;
            }
        }
    }
    if (!activeMidiBank.empty()) {
        midi.setActiveBank(activeMidiBank);
    }
}

void ofApp::handleHudVisibilityChanged(bool visible) {
    param_showHud = visible;
    hudRegistry.setHudVisible(visible);
}

void ofApp::ensureHudToolStack(MenuController& controller) {
    if (controlMappingHub && !controller.contains(controlMappingHub->id())) {
        controller.pushState(controlMappingHub);
    }
}

void ofApp::registerHudWidgetsFromCatalog() {
    const auto& entries = layerLibrary.entries();
    for (const auto& entry : entries) {
        if (!entry.isHudWidget()) {
            continue;
        }
        auto sample = createHudWidgetForModule(entry.hud.module);
        if (!sample) {
            ofLogWarning("ofApp") << "HUD widget module '" << entry.hud.module << "' missing for asset '" << entry.id << "'";
            continue;
        }
        OverlayWidget::Metadata metadata = sample->metadata();
        if (!entry.label.empty()) {
            metadata.label = entry.label;
        }
        if (!entry.category.empty()) {
            metadata.category = entry.category;
        }
        if (entry.config.contains("description") && entry.config["description"].is_string()) {
            metadata.description = entry.config["description"].get<std::string>();
        }
        metadata.defaultColumn = std::max(0, entry.hud.defaultColumn);
        metadata.band = overlayBandFromString(entry.hud.defaultBand, metadata.band);
        if (metadata.id.empty()) {
            metadata.id = entry.id;
        }
        const OverlayWidget::Metadata metadataCopy = metadata;
        std::string toggleId = entry.hud.toggleId.empty() ? entry.registryPrefix : entry.hud.toggleId;
        if (toggleId.empty()) {
            toggleId = metadata.id;
        }
        HudRegistry::WidgetDescriptor descriptor;
        descriptor.metadata = metadataCopy;
        descriptor.toggleId = toggleId;
        descriptor.factory = [module = entry.hud.module, metadataCopy]() -> std::unique_ptr<OverlayWidget> {
            auto inner = createHudWidgetForModule(module);
            if (!inner) {
                ofLogWarning("ofApp") << "HUD widget module '" << module << "' unavailable during factory creation";
                return nullptr;
            }
            return std::make_unique<ConfiguredOverlayWidget>(std::move(inner), metadataCopy);
        };
        if (!hudRegistry.registerWidget(std::move(descriptor))) {
            ofLogWarning("ofApp") << "Failed to register HUD widget '" << entry.id << "'";
        }
    }
}

void ofApp::publishHudTelemetrySample(const std::string& widgetId,
                                      const std::string& feedId,
                                      float value,
                                      const std::string& detail) const {
    if (controlMappingHub) {
        controlMappingHub->publishHudTelemetrySample(widgetId, feedId, value, detail);
    }
}

void ofApp::publishOverlayVisibilityTelemetry(const std::string& feedId, bool visible) {
    publishHudTelemetrySample("hud.controls", feedId, visible ? 1.0f : 0.0f, visible ? "visible" : "hidden");
}

void ofApp::publishDualDisplayTelemetry(const std::string& mode) {
    publishHudTelemetrySample("hud.controls", "overlay.dual_display.mode", mode == "dual" ? 1.0f : 0.0f, mode);
}

void ofApp::handleSecondaryDisplayParamChange() {
    bool stateChanged = false;
    bool geometryChanged = false;
    bool monitorChanged = false;
    bool vsyncChanged = false;
    bool followChanged = false;
    if (secondaryDisplay_.monitorId != param_secondaryDisplayMonitor) {
        secondaryDisplay_.monitorId = param_secondaryDisplayMonitor;
        stateChanged = true;
        monitorChanged = true;
    }
    if (secondaryDisplay_.x != param_secondaryDisplayX || secondaryDisplay_.y != param_secondaryDisplayY) {
        secondaryDisplay_.x = param_secondaryDisplayX;
        secondaryDisplay_.y = param_secondaryDisplayY;
        geometryChanged = true;
    }
    if (secondaryDisplay_.width != param_secondaryDisplayWidth || secondaryDisplay_.height != param_secondaryDisplayHeight) {
        secondaryDisplay_.width = param_secondaryDisplayWidth;
        secondaryDisplay_.height = param_secondaryDisplayHeight;
        geometryChanged = true;
    }
    if (secondaryDisplay_.vsync != param_secondaryDisplayVsync) {
        secondaryDisplay_.vsync = param_secondaryDisplayVsync;
        stateChanged = true;
        vsyncChanged = true;
    }
    if (std::abs(secondaryDisplay_.dpiScale - param_secondaryDisplayDpi) > 0.0001f) {
        secondaryDisplay_.dpiScale = param_secondaryDisplayDpi;
        geometryChanged = true;
    }
    if (secondaryDisplay_.background != param_secondaryDisplayBackground) {
        secondaryDisplay_.background = param_secondaryDisplayBackground;
        stateChanged = true;
    }
    if (secondaryDisplay_.followPrimary != param_secondaryDisplayFollowPrimary) {
        secondaryDisplay_.followPrimary = param_secondaryDisplayFollowPrimary;
        stateChanged = true;
        followChanged = true;
        publishSecondaryDisplayFollowTelemetry();
    }
    if (secondaryDisplay_.enabled != param_secondaryDisplayEnabled) {
        secondaryDisplay_.enabled = param_secondaryDisplayEnabled;
        stateChanged = true;
        if (!requestSecondaryDisplay(secondaryDisplay_.enabled, "param-change")) {
            secondaryDisplay_.enabled = false;
            param_secondaryDisplayEnabled = false;
        }
    }
    if (geometryChanged) {
        ofLogNotice("ofApp") << "Secondary display geometry updated ("
                             << secondaryDisplay_.width << "x" << secondaryDisplay_.height
                             << " @ " << secondaryDisplay_.x << "," << secondaryDisplay_.y << ")";
    }
    if (vsyncChanged && secondaryWindow_) {
        secondaryWindow_->setVerticalSync(secondaryDisplay_.vsync);
    }
    if ((geometryChanged || monitorChanged) && secondaryWindow_ && secondaryDisplay_.enabled) {
        requestSecondaryDisplay(false, "reconfigure");
        requestSecondaryDisplay(true, "reconfigure");
    }
    if ((geometryChanged || monitorChanged) && secondaryDisplay_.enabled) {
        requestHudLayoutResync("secondary_display.geometry");
    }
    if (vsyncChanged && secondaryDisplay_.enabled) {
        requestHudLayoutResync("secondary_display.vsync");
    }
    if (followChanged && secondaryDisplay_.enabled) {
        ofLogNotice("ofApp") << "Controller monitor set to "
                             << (secondaryDisplay_.followPrimary ? "follow projector" : "freeform") << " mode";
    }
    if (followChanged) {
        syncHudLayoutTarget();
        emitOverlayRouteTelemetry("follow_toggle", false);
    }
    if (stateChanged || geometryChanged || followChanged) {
        publishSecondaryDisplayTelemetry();
        persistConsoleAssignments();
    }
}

bool ofApp::requestSecondaryDisplay(bool enable, const std::string& reason) {
    if (enable) {
        if (spawnSecondaryDisplayShell(reason)) {
            return true;
        }
        ofLogWarning("ofApp") << "Failed to spawn controller monitor for reason '" << reason << "'";
        publishHudTelemetrySample("hud.controls", "secondary_display.event", 0.0f, "spawn_failed:" + reason);
        secondaryDisplay_.active = false;
        return false;
    }
    destroySecondaryDisplayShell(reason);
    return true;
}

bool ofApp::spawnSecondaryDisplayShell(const std::string& reason) {
    if (secondaryWindow_) {
        secondaryDisplay_.active = true;
        publishSecondaryDisplayTelemetry();
        return true;
    }
    auto baseWindow = ofGetCurrentWindow();
    if (!baseWindow) {
        ofLogWarning("ofApp") << "Cannot spawn controller monitor: no active base window";
        return false;
    }
    const std::string trimmedMonitorHint = ofTrim(secondaryDisplay_.monitorId);
    MonitorSelection selection = selectMonitor(trimmedMonitorHint);
    if (!selection.handle) {
        ofLogWarning("ofApp") << "Cannot spawn controller monitor: GLFW did not report any monitors; "
                                 "defaulting to primary window geometry";
        selection.label = selection.label.empty() ? "Primary Window" : selection.label;
        selection.x = ofGetWindowPositionX();
        selection.y = ofGetWindowPositionY();
        selection.width = ofGetWindowWidth();
        selection.height = ofGetWindowHeight();
    }
    if (selection.width <= 0) {
        selection.width = std::max(selection.width, 640);
    }
    if (selection.height <= 0) {
        selection.height = std::max(selection.height, 360);
    }
    const bool monitorHintProvided = !trimmedMonitorHint.empty();
    const bool monitorHintResolved = selection.matchedHint && selection.handle;
    if (!monitorHintResolved && monitorHintProvided) {
        ofLogNotice("ofApp") << "Requested controller monitor '" << trimmedMonitorHint
                             << "' not available; defaulting to '" << selection.label << "'";
    }
    int width = secondaryDisplay_.width > 0 ? secondaryDisplay_.width : selection.width;
    int height = secondaryDisplay_.height > 0 ? secondaryDisplay_.height : selection.height;
    int posX = secondaryDisplay_.x;
    int posY = secondaryDisplay_.y;
    if (!monitorHintProvided || !monitorHintResolved) {
        posX = selection.x;
        posY = selection.y;
        secondaryDisplay_.x = posX;
        secondaryDisplay_.y = posY;
        param_secondaryDisplayX = posX;
        param_secondaryDisplayY = posY;
    }
    ofGLFWWindowSettings settings;
    settings.shareContextWith = baseWindow;
    settings.setGLVersion(3, 2);
    settings.setSize(width, height);
    settings.setPosition({static_cast<float>(posX), static_cast<float>(posY)});
    settings.windowMode = OF_WINDOW;
    settings.visible = true;
    settings.resizable = true;
    settings.decorated = true;
    settings.multiMonitorFullScreen = false;
    settings.doubleBuffering = true;
    settings.title = "Controller Monitor";

    secondaryWindow_ = ofCreateWindow(settings);
    if (!secondaryWindow_) {
        ofLogWarning("ofApp") << "Failed to create controller monitor window";
        return false;
    }
    secondaryWindowApp_ = std::make_shared<SecondaryDisplayView>(this);
    ofRunApp(secondaryWindow_, secondaryWindowApp_);
    secondaryWindow_->setVerticalSync(secondaryDisplay_.vsync);
    secondaryDisplay_.active = true;
    secondaryDisplay_.spawnReason = reason;
    secondaryDisplayActiveMonitor_ = selection.label;
    ofLogNotice("ofApp") << "Controller monitor spawned (" << reason << ") on '" << secondaryDisplayActiveMonitor_
                         << "' @ " << width << "x" << height << " pos " << posX << "," << posY;
    publishHudTelemetrySample("hud.controls", "secondary_display.event", 1.0f, "spawn:" + reason);
    publishSecondaryDisplayTelemetry();
    requestHudLayoutResync("secondary_display.spawn");
    handleControllerFocusParamChange();
    return true;
}

void ofApp::destroySecondaryDisplayShell(const std::string& reason) {
    if (secondaryWindow_) {
        secondaryWindow_->setWindowShouldClose();
        secondaryWindowApp_.reset();
        secondaryWindow_.reset();
    }
    if (!secondaryDisplay_.active) {
        return;
    }
    secondaryDisplay_.active = false;
    secondaryDisplay_.spawnReason = reason;
    secondaryDisplayActiveMonitor_.clear();
    param_controllerFocusConsole = true;
    controllerFocus_.preferConsole = true;
    controllerFocus_.owner = ControllerFocusOwner::Console;
    controllerFocus_.needsAttention = false;
    controllerFocusDirty_ = true;
    publishControllerFocusTelemetry("controller-offline", false);
    focusPrimaryWindow("controller-offline");
    ofLogNotice("ofApp") << "Controller monitor shutdown (" << reason << ")";
    publishHudTelemetrySample("hud.controls", "secondary_display.event", 0.0f, "destroy:" + reason);
    publishSecondaryDisplayTelemetry();
}

void ofApp::publishSecondaryDisplayTelemetry() const {
    const std::string detail = secondaryDisplay_.active ? secondaryDisplayLabel() : "inactive";
    publishHudTelemetrySample("hud.controls",
                              "secondary_display.active",
                              secondaryDisplay_.active ? 1.0f : 0.0f,
                              detail);
}

void ofApp::handleControllerFocusParamChange() {
    bool desiredConsole = param_controllerFocusConsole;
    if (!secondaryDisplay_.enabled) {
        desiredConsole = true;
        if (!param_controllerFocusConsole) {
            param_controllerFocusConsole = true;
        }
    }
    const bool preferenceChanged = controllerFocus_.preferConsole != desiredConsole;
    controllerFocus_.preferConsole = desiredConsole;
    if (preferenceChanged) {
        controllerFocusDirty_ = true;
    }
    if (!controllerFocus_.holdActive) {
        if (!controllerFocus_.preferConsole && !secondaryDisplay_.active) {
            controllerFocus_.needsAttention = true;
            return;
        }
        applyDesiredControllerFocus("param-change");
    }
    if (!controllerFocus_.preferConsole && !secondaryDisplay_.active) {
        controllerFocus_.needsAttention = true;
        return;
    }
    controllerFocus_.needsAttention = false;
}

void ofApp::requestControllerFocusToggle() {
    param_controllerFocusConsole = !param_controllerFocusConsole;
    ofLogNotice("ofApp") << "Controller focus preference -> "
                         << (param_controllerFocusConsole ? "console" : "controller");
    handleControllerFocusParamChange();
}

bool ofApp::focusPrimaryWindow(const std::string& reason) {
    auto mainWindow = ofGetCurrentWindow();
    auto glfwWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(mainWindow);
    GLFWwindow* handle = glfwWindow ? glfwWindow->getGLFWWindow() : nullptr;
    if (!handle) {
        ofLogWarning("ofApp") << "Cannot focus console window (" << reason << "): no GLFW window";
        controllerFocus_.lastCommandSucceeded = false;
        controllerFocus_.needsAttention = true;
        publishControllerFocusTelemetry("console_focus_failed:" + reason, false);
        return false;
    }
    glfwFocusWindow(handle);
    controllerFocus_.owner = ControllerFocusOwner::Console;
    controllerFocus_.lastCommandSucceeded = true;
    controllerFocus_.needsAttention = false;
    controllerFocus_.lastCommandMs = ofGetElapsedTimeMillis();
    controllerFocus_.lastDetail = "console:" + reason;
    controllerFocus_.lastTelemetryMs = controllerFocus_.lastCommandMs;
    publishControllerFocusTelemetry(controllerFocus_.lastDetail, true);
    return true;
}

bool ofApp::focusSecondaryWindow(const std::string& reason) {
    if (!secondaryDisplay_.active || !secondaryWindow_) {
        ofLogWarning("ofApp") << "Cannot focus controller window (" << reason << "): inactive";
        controllerFocus_.lastCommandSucceeded = false;
        controllerFocus_.needsAttention = true;
        publishControllerFocusTelemetry("controller_focus_failed:" + reason, false);
        return false;
    }
    auto glfwWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(secondaryWindow_);
    GLFWwindow* handle = glfwWindow ? glfwWindow->getGLFWWindow() : nullptr;
    if (!handle) {
        ofLogWarning("ofApp") << "Cannot focus controller window (" << reason << "): no GLFW window";
        controllerFocus_.lastCommandSucceeded = false;
        controllerFocus_.needsAttention = true;
        publishControllerFocusTelemetry("controller_focus_failed:" + reason, false);
        return false;
    }
    glfwFocusWindow(handle);
    controllerFocus_.owner = ControllerFocusOwner::Controller;
    controllerFocus_.lastCommandSucceeded = true;
    controllerFocus_.needsAttention = false;
    controllerFocus_.lastCommandMs = ofGetElapsedTimeMillis();
    controllerFocus_.lastDetail = "controller:" + reason;
    controllerFocus_.lastTelemetryMs = controllerFocus_.lastCommandMs;
    publishControllerFocusTelemetry(controllerFocus_.lastDetail, true);
    return true;
}

ofApp::ControllerFocusOwner ofApp::currentControllerFocusOwner() const {
    auto controllerWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(secondaryWindow_);
    if (controllerWindow) {
        GLFWwindow* handle = controllerWindow->getGLFWWindow();
        if (handle && glfwGetWindowAttrib(handle, GLFW_FOCUSED) == GLFW_TRUE) {
            return ControllerFocusOwner::Controller;
        }
    }
    auto consoleWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(ofGetCurrentWindow());
    if (consoleWindow) {
        GLFWwindow* handle = consoleWindow->getGLFWWindow();
        if (handle && glfwGetWindowAttrib(handle, GLFW_FOCUSED) == GLFW_TRUE) {
            return ControllerFocusOwner::Console;
        }
    }
    return controllerFocus_.owner;
}

void ofApp::publishControllerFocusTelemetry(const std::string& detail, bool success) {
    publishHudTelemetrySample("hud.controls", "controller.focus", success ? 1.0f : 0.0f, detail);
}

void ofApp::monitorWindowContentScale() {
    auto baseWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(ofGetCurrentWindow());
    if (baseWindow) {
        if (GLFWwindow* handle = baseWindow->getGLFWWindow()) {
            int iconified = glfwGetWindowAttrib(handle, GLFW_ICONIFIED);
            if (iconified == GLFW_FALSE && lastPrimaryIconified_) {
                lastPrimaryIconified_ = false;
                forceHudLayoutResync("resume.primary");
            } else if (iconified == GLFW_TRUE) {
                lastPrimaryIconified_ = true;
            }
            float sx = 1.0f;
            float sy = 1.0f;
            glfwGetWindowContentScale(handle, &sx, &sy);
            glm::vec2 scale(sx, sy);
            float dx = std::abs(scale.x - lastPrimaryWindowScale_.x);
            float dy = std::abs(scale.y - lastPrimaryWindowScale_.y);
            if (std::max(dx, dy) > 0.05f) {
                lastPrimaryWindowScale_ = scale;
                forceHudLayoutResync("dpi.primary");
            }
        }
    }
    if (secondaryWindow_) {
        if (auto glfwWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(secondaryWindow_)) {
            if (GLFWwindow* handle = glfwWindow->getGLFWWindow()) {
                int iconified = glfwGetWindowAttrib(handle, GLFW_ICONIFIED);
                if (iconified == GLFW_FALSE && lastSecondaryIconified_) {
                    lastSecondaryIconified_ = false;
                    forceHudLayoutResync("resume.controller");
                } else if (iconified == GLFW_TRUE) {
                    lastSecondaryIconified_ = true;
                }
                float sx = 1.0f;
                float sy = 1.0f;
                glfwGetWindowContentScale(handle, &sx, &sy);
                glm::vec2 scale(sx, sy);
                float dx = std::abs(scale.x - lastSecondaryWindowScale_.x);
                float dy = std::abs(scale.y - lastSecondaryWindowScale_.y);
                if (std::max(dx, dy) > 0.05f) {
                    lastSecondaryWindowScale_ = scale;
                    forceHudLayoutResync("dpi.controller");
                }
            }
        }
    } else {
        lastSecondaryIconified_ = false;
    }
}

ofApp::ControllerFocusOwner ofApp::desiredControllerFocusOwner() const {
    if (!controllerFocusHolds_.empty()) {
        return controllerFocusHolds_.back().owner;
    }
    return controllerFocus_.preferConsole ? ControllerFocusOwner::Console : ControllerFocusOwner::Controller;
}

void ofApp::applyDesiredControllerFocus(const std::string& reason) {
    ControllerFocusOwner target = desiredControllerFocusOwner();
    const std::string detail = reason.empty() ? "focus" : reason;
    if (target == ControllerFocusOwner::Console) {
        focusPrimaryWindow(detail);
    } else {
        focusSecondaryWindow(detail);
    }
}

std::string ofApp::acquireControllerFocusHold(ControllerFocusOwner owner, const std::string& reason) {
    if (owner == ControllerFocusOwner::Controller && (!secondaryDisplay_.enabled || !secondaryDisplay_.active)) {
        return std::string();
    }
    ControllerFocusHoldEntry entry;
    entry.owner = owner;
    entry.reason = reason;
    entry.token = reason + "#" + ofToString(++controllerFocusHoldCounter_);
    controllerFocusHolds_.push_back(entry);
    controllerFocus_.holdActive = true;
    controllerFocus_.holdOwner = entry.owner;
    controllerFocus_.holdReason = entry.reason;
    applyDesiredControllerFocus("hold:" + entry.reason);
    return entry.token;
}

void ofApp::releaseControllerFocusHold(const std::string& token) {
    if (token.empty()) {
        return;
    }
    auto it = std::find_if(controllerFocusHolds_.begin(),
                           controllerFocusHolds_.end(),
                           [&](const ControllerFocusHoldEntry& entry) { return entry.token == token; });
    if (it == controllerFocusHolds_.end()) {
        return;
    }
    controllerFocusHolds_.erase(it);
    if (controllerFocusHolds_.empty()) {
        controllerFocus_.holdActive = false;
        controllerFocus_.holdReason.clear();
        controllerFocus_.holdOwner = controllerFocus_.preferConsole ? ControllerFocusOwner::Console
                                                                    : ControllerFocusOwner::Controller;
    } else {
        controllerFocus_.holdActive = true;
        controllerFocus_.holdOwner = controllerFocusHolds_.back().owner;
        controllerFocus_.holdReason = controllerFocusHolds_.back().reason;
    }
    applyDesiredControllerFocus("hold-release");
}

void ofApp::manageFocusHold(std::string& token,
                            bool shouldHold,
                            ControllerFocusOwner owner,
                            const std::string& reason) {
    auto findByToken = [&](const std::string& lookup) -> std::vector<ControllerFocusHoldEntry>::iterator {
        return std::find_if(controllerFocusHolds_.begin(),
                            controllerFocusHolds_.end(),
                            [&](const ControllerFocusHoldEntry& entry) { return entry.token == lookup; });
    };
    if (shouldHold) {
        if (!token.empty()) {
            auto it = findByToken(token);
            if (it == controllerFocusHolds_.end() || it->owner != owner) {
                releaseControllerFocusHold(token);
                token.clear();
            }
        }
        if (token.empty()) {
            token = acquireControllerFocusHold(owner, reason);
        }
    } else if (!token.empty()) {
        releaseControllerFocusHold(token);
        token.clear();
    }
}

void ofApp::manageControllerFocusHolds() {
    ControllerFocusOwner preferredOwner = controllerFocus_.preferConsole
        ? ControllerFocusOwner::Console
        : ControllerFocusOwner::Controller;
    bool hudPickerActive = controlMappingHub && controlMappingHub->hudColumnPickerVisible();
    manageFocusHold(controllerFocus_.hudPickerFocusHoldToken, hudPickerActive, preferredOwner, "hud.picker");
    bool devicesBinding = devicesPanel && devicesPanel->isBindingCaptureActive();
    manageFocusHold(controllerFocus_.devicesFocusHoldToken, devicesBinding, preferredOwner, "devices.binding");
    bool learningActive = midi.isLearning();
    bool oscLearn = learningActive && midi.isLearningOsc();
    std::string reason = oscLearn ? "osc.learn" : "midi.learn";
    manageFocusHold(controllerFocus_.midiFocusHoldToken, learningActive, preferredOwner, reason);
}

void ofApp::updateControllerFocusWatchdog() {
    const ControllerFocusOwner desired = desiredControllerFocusOwner();
    const ControllerFocusOwner actual = currentControllerFocusOwner();
    uint64_t now = ofGetElapsedTimeMillis();
    if (desired == ControllerFocusOwner::Controller && !secondaryDisplay_.active) {
        controllerFocus_.needsAttention = true;
        if (now - controllerFocus_.lastTelemetryMs > 1000) {
            controllerFocus_.lastTelemetryMs = now;
            publishControllerFocusTelemetry("controller-inactive", false);
        }
        return;
    }
    if (desired != actual) {
        const bool recentlyCommanded = now - controllerFocus_.lastCommandMs < 400;
        if (!recentlyCommanded) {
            if (desired == ControllerFocusOwner::Console) {
                focusPrimaryWindow("watchdog");
            } else {
                focusSecondaryWindow("watchdog");
            }
        }
        controllerFocus_.needsAttention = true;
    } else if (controllerFocus_.needsAttention) {
        controllerFocus_.needsAttention = false;
        publishControllerFocusTelemetry("focus-aligned", true);
    }
    if (now - controllerFocus_.lastTelemetryMs > 2000) {
        controllerFocus_.lastTelemetryMs = now;
        const std::string detail = controllerFocus_.preferConsole ? "console-preferred" : "controller-preferred";
        publishControllerFocusTelemetry(detail, !controllerFocus_.needsAttention);
    }
}

std::string ofApp::controllerFocusStatusBadge() const {
    std::ostringstream badge;
    badge << "Controller Window: ";
    if (!secondaryDisplay_.enabled) {
        badge << "disabled";
        return badge.str();
    }
    if (!secondaryDisplay_.active) {
        badge << "spawning...";
        return badge.str();
    }
    badge << (param_controllerFocusConsole ? "Console focus" : "Controller focus");
    badge << " | Mode " << (secondaryDisplay_.followPrimary ? "Follow" : "Freeform");
    if (controllerFocus_.needsAttention) {
        badge << "  [ATTENTION]";
    }
    return badge.str();
}

void ofApp::updateSecondaryDisplayWatchdog() {
    uint64_t now = ofGetElapsedTimeMillis();
    if (!secondaryDisplay_.enabled) {
        if (secondaryDisplayWatchdog_.tripped) {
            secondaryDisplayWatchdog_.tripped = false;
            publishSecondaryDisplayWatchdog("disabled", true);
        }
        return;
    }
    if (!secondaryDisplay_.active) {
        if (now - secondaryDisplayWatchdog_.lastRecoveryAttemptMs > 3000) {
            secondaryDisplayWatchdog_.lastRecoveryAttemptMs = now;
            requestSecondaryDisplay(true, "watchdog-retry");
        }
        if (now - secondaryDisplayWatchdog_.lastEmitMs > 1500) {
            publishSecondaryDisplayWatchdog("inactive", false);
        }
        return;
    }
    bool monitorMissing = false;
    std::string monitorLabel = secondaryDisplayActiveMonitor_;
    if (monitorLabel.empty()) {
        monitorLabel = secondaryDisplay_.monitorId;
    }
    if (!monitorLabel.empty()) {
        monitorMissing = !isSecondaryMonitorPresent(monitorLabel);
    }
    bool windowMissing = !secondaryWindow_;
    bool windowClosed = false;
    if (!windowMissing) {
        auto glfwWindow = std::dynamic_pointer_cast<ofAppGLFWWindow>(secondaryWindow_);
        GLFWwindow* handle = glfwWindow ? glfwWindow->getGLFWWindow() : nullptr;
        if (!handle) {
            windowMissing = true;
        } else {
            windowClosed = glfwWindowShouldClose(handle);
        }
    }
    if (monitorMissing || windowMissing || windowClosed) {
        std::string reason = monitorMissing ? "monitor_removed" : windowMissing ? "window_missing" : "window_closed";
        handleSecondaryDisplayWatchdogTrip(reason);
        return;
    }
    if (secondaryDisplayWatchdog_.tripped) {
        secondaryDisplayWatchdog_.tripped = false;
        publishSecondaryDisplayWatchdog("healthy", true);
    } else if (now - secondaryDisplayWatchdog_.lastEmitMs > 4000) {
        publishSecondaryDisplayWatchdog("healthy", true);
    }
}

void ofApp::publishSecondaryDisplayWatchdog(const std::string& detail, bool healthy) {
    secondaryDisplayWatchdog_.lastEmitMs = ofGetElapsedTimeMillis();
    publishHudTelemetrySample("hud.controls",
                              "secondary_display.watchdog",
                              healthy ? 1.0f : 0.0f,
                              detail);
}

void ofApp::handleSecondaryDisplayWatchdogTrip(const std::string& reason) {
    if (secondaryDisplayWatchdog_.tripped && secondaryDisplayWatchdog_.lastReason == reason) {
        if (ofGetElapsedTimeMillis() - secondaryDisplayWatchdog_.lastEmitMs > 1000) {
            publishSecondaryDisplayWatchdog(reason, false);
        }
        return;
    }
    secondaryDisplayWatchdog_.tripped = true;
    secondaryDisplayWatchdog_.lastReason = reason;
    publishSecondaryDisplayWatchdog(reason, false);
    ofLogWarning("ofApp") << "Secondary display watchdog tripped (" << reason << "); attempting controller monitor recovery";
    const uint64_t now = ofGetElapsedTimeMillis();
    secondaryDisplayWatchdog_.lastRecoveryAttemptMs = now;
    destroySecondaryDisplayShell("watchdog:" + reason);
    param_dualDisplayMode = "dual";
    persistedDualDisplayMode_ = "dual";
    publishDualDisplayTelemetry(persistedDualDisplayMode_);
    param_secondaryDisplayEnabled = true;
    secondaryDisplay_.enabled = true;
    if (paramRegistry.findString("console.dual_display.mode")) {
        paramRegistry.setStringBase("console.dual_display.mode", param_dualDisplayMode, true);
    }
    if (paramRegistry.findBool("console.secondary_display.enabled")) {
        paramRegistry.setBoolBase("console.secondary_display.enabled", param_secondaryDisplayEnabled, true);
    }
    requestSecondaryDisplay(true, "watchdog-recover:" + reason);
    handleSecondaryDisplayParamChange();
    persistConsoleAssignments();
}

bool ofApp::isSecondaryMonitorPresent(const std::string& label) const {
    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    if (!monitors || monitorCount <= 0) {
        return false;
    }
    std::string lowered = ofToLower(label);
    std::string trimmed = ofTrim(lowered);
    if (trimmed.empty()) {
        return true;
    }
    if (trimmed == "primary") {
        return glfwGetPrimaryMonitor() != nullptr;
    }
    if (trimmed == "secondary") {
        return monitorCount > 1;
    }
    if (!trimmed.empty() && std::all_of(trimmed.begin(), trimmed.end(), ::isdigit)) {
        int requested = ofToInt(trimmed);
        return requested >= 1 && requested <= monitorCount;
    }
    for (int i = 0; i < monitorCount; ++i) {
        const char* monitorName = glfwGetMonitorName(monitors[i]);
        if (!monitorName) {
            continue;
        }
        std::string current = ofToLower(std::string(monitorName));
        if (current == trimmed || current.find(trimmed) != std::string::npos || trimmed.find(current) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void ofApp::toggleDualDisplayMode() {
    const std::string current = normalizeDualDisplayMode(param_dualDisplayMode);
    const bool toDual = current != "dual";
    param_dualDisplayMode = toDual ? "dual" : "single";
    persistedDualDisplayMode_ = param_dualDisplayMode;
    publishDualDisplayTelemetry(persistedDualDisplayMode_);
    param_secondaryDisplayEnabled = toDual;
    handleSecondaryDisplayParamChange();
    persistConsoleAssignments();
    ofLogNotice("ofApp") << "Dual display mode toggled -> " << persistedDualDisplayMode_
                         << " (controller monitor " << (toDual ? "enabled" : "disabled") << ")";
}

void ofApp::toggleSecondaryDisplayFollow() {
    param_secondaryDisplayFollowPrimary = !param_secondaryDisplayFollowPrimary;
    handleSecondaryDisplayParamChange();
}

void ofApp::publishSecondaryDisplayFollowTelemetry() const {
    publishHudTelemetrySample("hud.controls",
                              "secondary_display.follow_primary",
                              secondaryDisplay_.followPrimary ? 1.0f : 0.0f,
                              secondaryDisplay_.followPrimary ? "follow" : "freeform");
}

void ofApp::syncHudLayoutTarget() {
    if (!controlMappingHub) {
        return;
    }
    ControlMappingHubState::HudLayoutTarget target = secondaryDisplay_.followPrimary
        ? ControlMappingHubState::HudLayoutTarget::Projector
        : ControlMappingHubState::HudLayoutTarget::Controller;
    controlMappingHub->setHudLayoutTarget(target);
}

void ofApp::migrateOverlaysToPrimary() {
    announceOverlayMigration("projector");
    param_secondaryDisplayFollowPrimary = true;
    handleSecondaryDisplayParamChange();
    emitOverlayRouteTelemetry("shortcut.migrate_primary", true);
}

void ofApp::migrateOverlaysToController() {
    announceOverlayMigration("controller");
    param_secondaryDisplayFollowPrimary = false;
    handleSecondaryDisplayParamChange();
    emitOverlayRouteTelemetry("shortcut.migrate_controller", true);
}

void ofApp::announceOverlayMigration(const std::string& target) {
    ofLogNotice("ofApp") << "Overlay layout migration requested -> " << target;
    publishHudTelemetrySample("hud.controls", "secondary_display.overlay_migrate", 1.0f, "to:" + target);
}

void ofApp::emitOverlayRouteTelemetry(const std::string& source, bool forceEvent) {
    if (!controlMappingHub) {
        return;
    }
    const std::string target = secondaryDisplay_.followPrimary
        ? ControlMappingHubState::hudLayoutTargetName(ControlMappingHubState::HudLayoutTarget::Projector)
        : ControlMappingHubState::hudLayoutTargetName(ControlMappingHubState::HudLayoutTarget::Controller);
    if (!forceEvent && target == lastOverlayRouteTarget_) {
        return;
    }
    lastOverlayRouteTarget_ = target;
    controlMappingHub->emitOverlayRouteEvent(target, source, secondaryDisplay_.followPrimary);
    LayoutSyncGuardScope guard(this, source);
    if (!guard.owns()) {
        requestHudLayoutResync(source);
        return;
    }
    broadcastHudLayoutSnapshots(source, true);
    broadcastHudRoutingManifest();
}

void ofApp::broadcastHudLayoutSnapshots(const std::string& reason, bool guardHeld) {
    if (!controlMappingHub) {
        return;
    }
    if (!guardHeld) {
        LayoutSyncGuardScope guard(this, reason);
        if (!guard.owns()) {
            requestHudLayoutResync(reason);
            return;
        }
        broadcastHudLayoutSnapshots(reason, true);
        broadcastHudRoutingManifest();
        return;
    }
    auto publishSnapshot = [&](ControlMappingHubState::HudLayoutTarget target) {
        auto snapshot = controlMappingHub->exportHudLayoutSnapshot(target);
        std::string detail = reason;
        const std::string targetLabel = ControlMappingHubState::hudLayoutTargetName(target);
        if (detail.empty()) {
            detail = targetLabel;
        } else {
            detail += ":" + targetLabel;
        }
        controlMappingHub->emitHudLayoutSnapshot(target, snapshot, detail);
        return snapshot;
    };
    publishSnapshot(ControlMappingHubState::HudLayoutTarget::Projector);
    publishSnapshot(ControlMappingHubState::HudLayoutTarget::Controller);
}

void ofApp::broadcastHudRoutingManifest() {
    if (!controlMappingHub) {
        return;
    }
    const auto widgets = hudRegistry.widgets();
    std::vector<ControlMappingHubState::HudRoutingEntry> manifest;
    manifest.reserve(widgets.size());
    for (const auto& widget : widgets) {
        if (widget.metadata.id.empty()) {
            continue;
        }
        ControlMappingHubState::HudRoutingEntry entry;
        entry.id = widget.metadata.id;
        entry.label = widget.metadata.label;
        entry.category = widget.metadata.category;
        entry.target = overlayTargetToString(widget.metadata.target);
        manifest.push_back(std::move(entry));
    }
    if (!manifest.empty()) {
        controlMappingHub->emitHudRoutingManifest(manifest);
    }
}

void ofApp::handleControlHubEvent(const std::string& payload) {
    if (payload.empty()) {
        return;
    }
    ofJson event;
    try {
        event = ofJson::parse(payload);
    } catch (const std::exception& ex) {
        ofLogWarning("ofApp") << "Failed to parse Browser event payload: " << ex.what();
        return;
    }
    const std::string type = event.value("type", std::string());
    if (type == "hud.mapping.changed") {
        handleHudMappingChangedEvent(event);
    }
}

void ofApp::handleHudMappingChangedEvent(const ofJson& event) {
    const std::string reason = event.value("reason", std::string());
    if (reason != "drift" && reason != "drift_cleared") {
        return;
    }
    const std::string target = event.value("detail", std::string());
    const std::string widgetId = event.value("widgetId", std::string());
    std::string indicator = target;
    if (!widgetId.empty()) {
        if (!indicator.empty()) {
            indicator += " -> ";
        }
        indicator += widgetId;
    }
    if (indicator.empty()) {
        indicator = reason;
    }
    const float sampleValue = reason == "drift" ? 1.0f : 0.0f;
    applyHudTelemetryOverride("hud.status", "layout_drift", sampleValue, indicator);
}

ofApp::LayoutSyncGuardScope::LayoutSyncGuardScope(ofApp* host, const std::string& reason)
    : host_(host)
    , reason_(reason) {
    if (host_) {
        owns_ = host_->tryEnterLayoutSyncGuard(reason_);
    }
}

ofApp::LayoutSyncGuardScope::~LayoutSyncGuardScope() {
    if (owns_ && host_) {
        host_->leaveLayoutSyncGuard(reason_);
    }
}

bool ofApp::tryEnterLayoutSyncGuard(const std::string& reason) {
    if (layoutSyncGuardActive_) {
        std::string detail = reason.empty() ? "guard_busy" : "guard_busy:" + reason;
        publishHudTelemetrySample("hud.controls", "layout_sync.skipped", 1.0f, detail);
        return false;
    }
    layoutSyncGuardActive_ = true;
    layoutSyncGuardSinceMs_ = ofGetElapsedTimeMillis();
    layoutSyncGuardFrame_ = static_cast<uint64_t>(ofGetFrameNum());
    layoutSyncGuardReason_ = reason;
    layoutSyncGuardStalled_ = false;
    return true;
}

void ofApp::leaveLayoutSyncGuard(const std::string& reason) {
    if (!layoutSyncGuardActive_) {
        return;
    }
    layoutSyncGuardActive_ = false;
    if (layoutSyncGuardStalled_) {
        std::string detail = reason.empty() ? layoutSyncGuardReason_ : reason;
        if (!detail.empty()) {
            publishHudTelemetrySample("hud.controls", "layout_sync.recovering", 1.0f, detail);
        } else {
            publishHudTelemetrySample("hud.controls", "layout_sync.recovering", 1.0f, "recover");
        }
        layoutSyncGuardStalled_ = false;
    }
    layoutSyncGuardReason_.clear();
}

void ofApp::requestHudLayoutResync(const std::string& reason) {
    layoutSyncResyncPending_ = true;
    layoutSyncResyncReason_ = reason;
    layoutSyncResyncLastAttemptMs_ = 0;
}

void ofApp::forceHudLayoutResync(const std::string& reason) {
    std::string detail = reason.empty() ? "manual" : reason;
    requestHudLayoutResync(detail);
    publishHudTelemetrySample("hud.controls", "layout_sync.forced", 1.0f, detail);
    ofLogNotice("ofApp") << "HUD layout resync requested (" << detail << ")";
}

void ofApp::updateLayoutSyncWatchdog(uint64_t nowMs) {
    if (layoutSyncGuardActive_) {
        if (!layoutSyncGuardStalled_) {
            const uint64_t frameNow = static_cast<uint64_t>(ofGetFrameNum());
            if (frameNow > layoutSyncGuardFrame_ + 2) {
                layoutSyncGuardStalled_ = true;
                std::string detail = layoutSyncGuardReason_.empty()
                    ? "stall"
                    : "stall:" + layoutSyncGuardReason_;
                publishHudTelemetrySample("hud.controls", "layout_sync.skipped", 1.0f, detail);
            }
        }
    }
    if (layoutSyncResyncPending_ && !layoutSyncGuardActive_) {
        if (layoutSyncResyncLastAttemptMs_ == 0 || nowMs - layoutSyncResyncLastAttemptMs_ >= 16) {
            LayoutSyncGuardScope guard(this, layoutSyncResyncReason_);
            layoutSyncResyncLastAttemptMs_ = nowMs;
            if (guard.owns()) {
                broadcastHudLayoutSnapshots(layoutSyncResyncReason_, true);
                broadcastHudRoutingManifest();
                layoutSyncResyncPending_ = false;
            }
        }
    }
}

ofColor ofApp::secondaryDisplayBackgroundColor() const {
    return parseHexColor(secondaryDisplay_.background, ofColor::black);
}

std::string ofApp::secondaryDisplayLabel() const {
    if (!secondaryDisplay_.active) {
        return std::string("Controller Monitor (") +
               (secondaryDisplay_.followPrimary ? "Follow Projector" : "Freeform Layout") + ", inactive)";
    }
    std::string label = secondaryDisplayActiveMonitor_;
    if (label.empty()) {
        label = secondaryDisplay_.monitorId.empty() ? "Auto Monitor" : secondaryDisplay_.monitorId;
    }
    std::string mode = secondaryDisplay_.followPrimary ? "Follow Projector" : "Freeform Layout";
    return "Controller Monitor - " + label + " [" + mode + "]";
}

void ofApp::drawSceneLoadSnapshot(float width, float height) const {
    ofPushStyle();
    ofBackground(secondaryDisplayBackgroundColor());

    const float pad = 18.0f;
    float y = 30.0f;
    const std::string sceneName = sceneLoadUiSnapshot_.displayName.empty()
        ? sceneLoadUiSnapshot_.scenePath
        : sceneLoadUiSnapshot_.displayName;

    ofSetColor(255);
    ofDrawBitmapString("Loading Scene", pad, y);
    y += 22.0f;
    if (!sceneName.empty()) {
        ofSetColor(220);
        ofDrawBitmapString(sceneName, pad, y);
        y += 18.0f;
    }

    ofSetColor(180);
    ofDrawBitmapString(std::string("Phase: ") + sceneLoadPhaseLabel(sceneLoadUiSnapshot_.phase), pad, y);
    y += 18.0f;
    if (!sceneLoadUiSnapshot_.status.empty()) {
        ofDrawBitmapString(sceneLoadUiSnapshot_.status, pad, y);
        y += 18.0f;
    }

    const std::string monitorStatus = sceneLoadUiSnapshot_.secondaryDisplayPreserved
        ? "Control Window held open"
        : (sceneLoadUiSnapshot_.secondaryDisplayWasActive ? "Control Window active state preserved" : "Control Window inactive");
    ofSetColor(150);
    ofDrawBitmapString(monitorStatus, pad, y);
    y += 18.0f;

    if (width > 0.0f && height > 0.0f) {
        const float elapsed = sceneLoadUiSnapshot_.startedMs == 0
            ? 0.0f
            : static_cast<float>(ofGetElapsedTimeMillis() - sceneLoadUiSnapshot_.startedMs) / 1000.0f;
        ofSetColor(110);
        ofDrawBitmapString("Elapsed: " + ofToString(elapsed, 1) + "s", pad, std::max(y, height - 24.0f));
    }

    ofPopStyle();
}

void ofApp::drawSecondaryDisplayWindow(float width, float height) {
    ofPushStyle();
    ofBackground(secondaryDisplayBackgroundColor());
    ofSetColor(255);
    ofDrawBitmapString(secondaryDisplayLabel(), 18, 30);
    bool routeFreeform = !secondaryDisplay_.followPrimary;
    bool shouldDrawHud = routeFreeform ? param_showHud : overlayVisibility_.hud;
    ThreeBandLayout controllerLayout = threeBandLayout.layoutForSize(width, height);
    if (shouldDrawHud) {
        OverlayManager::DrawParams controllerParams;
        controllerParams.bounds = ofRectangle(0.0f, 0.0f, width, height);
        controllerParams.app = this;
        controllerParams.layout = controllerLayout;
        controllerParams.useThreeBandLayout = true;
        overlayManager.draw(controllerParams);
    } else if (!routeFreeform) {
        float y = 52.0f;
        ofDrawBitmapString("Freeform layout mode", 18, y);
        y += 18.0f;
        ofDrawBitmapString("Ctrl+Shift+,  Mirror projector overlays", 18, y);
        y += 16.0f;
        ofDrawBitmapString("Ctrl+Shift+.  Route overlays here", 18, y);
        y += 16.0f;
        ofDrawBitmapString("Ctrl+Shift+F  Toggle follow mode", 18, y);
        y += 16.0f;
        ofDrawBitmapString("Ctrl+Shift+Tab  Focus console window", 18, y);
    }
    if (routeFreeform) {
        drawMenuPanels(controllerLayout, param_showConsole, param_showControlHub, param_showMenus);
    }
    ofPopStyle();
}

void ofApp::handleSensorTelemetrySample(const std::string& parameterId,
                                        float value,
                                        uint64_t timestampMs) {
    if (timestampMs == 0) {
        timestampMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    }
    auto updateFloatField = [&](bool& hasField,
                                float& snapshotValue,
                                uint64_t& tsField,
                                float& paramValue) {
        hasField = true;
        snapshotValue = value;
        tsField = timestampMs;
        paramValue = value;
    };
    auto markBioHud = [&](const std::string& metricLabel) {
        hudMatrixActivity.lastAnyMs = timestampMs;
        hudMatrixActivity.bio.mark(timestampMs, metricLabel, value);
        publishHudTelemetrySample("hud.sensors", "bio", value, metricLabel);
    };
    if (parameterId == "sensors.bioamp.raw") {
        updateFloatField(liveBioAmpSnapshot_.hasRaw, liveBioAmpSnapshot_.raw, liveBioAmpSnapshot_.rawTimestampMs, bioAmpParameters_.raw);
        return;
    }
    if (parameterId == "sensors.bioamp.signal") {
        updateFloatField(liveBioAmpSnapshot_.hasSignal, liveBioAmpSnapshot_.signal, liveBioAmpSnapshot_.signalTimestampMs, bioAmpParameters_.signal);
        return;
    }
    if (parameterId == "sensors.bioamp.mean") {
        updateFloatField(liveBioAmpSnapshot_.hasMean, liveBioAmpSnapshot_.mean, liveBioAmpSnapshot_.meanTimestampMs, bioAmpParameters_.mean);
        return;
    }
    if (parameterId == "sensors.bioamp.rms") {
        updateFloatField(liveBioAmpSnapshot_.hasRms, liveBioAmpSnapshot_.rms, liveBioAmpSnapshot_.rmsTimestampMs, bioAmpParameters_.rms);
        return;
    }
    if (parameterId == "sensors.bioamp.dom_hz") {
        updateFloatField(liveBioAmpSnapshot_.hasDomHz, liveBioAmpSnapshot_.domHz, liveBioAmpSnapshot_.domTimestampMs, bioAmpParameters_.domHz);
        return;
    }
    if (parameterId == "sensors.bioamp.sample_rate") {
        liveBioAmpSnapshot_.hasSampleRate = true;
        liveBioAmpSnapshot_.sampleRate = static_cast<uint16_t>(std::max(0.0f, value));
        liveBioAmpSnapshot_.sampleRateTimestampMs = timestampMs;
        bioAmpParameters_.sampleRate = value;
        markBioHud("sample_rate");
        return;
    }
    if (parameterId == "sensors.bioamp.window") {
        liveBioAmpSnapshot_.hasWindow = true;
        liveBioAmpSnapshot_.window = static_cast<uint16_t>(std::max(0.0f, value));
        liveBioAmpSnapshot_.windowTimestampMs = timestampMs;
        bioAmpParameters_.window = value;
        markBioHud("window");
        return;
    }
}

void ofApp::applyHudTelemetryOverride(const std::string& widgetId,
                                      const std::string& feedId,
                                      float value,
                                      const std::string& detail) {
    HudTelemetrySampleOverride sample;
    sample.value = value;
    sample.timestampMs = ofGetElapsedTimeMillis();
    sample.detail = detail;
    hudTelemetryOverrides_[hudTelemetryKey(widgetId, feedId)] = sample;

    auto markCategory = [&](HudCategoryActivity& cat, uint64_t& lastAnyMs) {
        lastAnyMs = sample.timestampMs;
        const std::string label = detail.empty() ? feedId : detail;
        cat.mark(sample.timestampMs, label, value);
    };

    if (widgetId == "hud.sensors") {
        if (feedId == "hr") {
            markCategory(hudDeckActivity.hr, hudDeckActivity.lastAnyMs);
        } else if (feedId == "imu") {
            markCategory(hudDeckActivity.imu, hudDeckActivity.lastAnyMs);
        } else if (feedId == "mic") {
            markCategory(hudMatrixActivity.mic, hudMatrixActivity.lastAnyMs);
        } else if (feedId == "bio") {
            markCategory(hudMatrixActivity.bio, hudMatrixActivity.lastAnyMs);
        } else if (feedId == "osc") {
            oscHistory.emplace_back(detail.empty() ? std::string("/sensor/osc") : detail, value);
            while (oscHistory.size() > oscHistoryMax) {
                oscHistory.pop_front();
            }
        } else {
            markCategory(hudDeckActivity.aux, hudDeckActivity.lastAnyMs);
        }
    } else if (widgetId == "hud.status") {
        if (feedId == "midi") {
            lastMidiConnected_ = value >= 0.5f;
            midiTelemetryInitialized_ = true;
        } else if (feedId == "collector") {
            lastCollectorConnected_ = value >= 0.5f;
            collectorTelemetryInitialized_ = true;
        } else if (feedId == "timing") {
            lastHudTimingTelemetryMs_ = sample.timestampMs;
        }
    }
}

float ofApp::hudTelemetryValueOr(const std::string& widgetId,
                                 const std::string& feedId,
                                 float fallback,
                                 uint64_t maxAgeMs) const {
    auto sample = hudTelemetryOverrideSample(widgetId, feedId, maxAgeMs);
    if (sample) {
        return sample->value;
    }
    return fallback;
}

std::optional<ofApp::HudTelemetrySampleOverride> ofApp::hudTelemetryOverrideSample(const std::string& widgetId,
                                                                                   const std::string& feedId,
                                                                                   uint64_t maxAgeMs) const {
    auto it = hudTelemetryOverrides_.find(hudTelemetryKey(widgetId, feedId));
    if (it == hudTelemetryOverrides_.end()) {
        return std::nullopt;
    }
    if (maxAgeMs > 0) {
        uint64_t now = ofGetElapsedTimeMillis();
        if (now > it->second.timestampMs + maxAgeMs) {
            return std::nullopt;
        }
    }
    return it->second;
}

std::string ofApp::hudTelemetryKey(const std::string& widgetId, const std::string& feedId) const {
    return widgetId + ":" + feedId;
}

bool ofApp::toggleHudTools(MenuController& controller) {
    if (!controlMappingHub) {
        return false;
    }
    ensureHudToolStack(controller);
    controlMappingHub->setHudVisible(!controlMappingHub->hudVisible());
    controlMappingHub->focusCategory("HUD");
    controller.requestViewModelRefresh();
    return true;
}

bool ofApp::openHudLayoutEditor(MenuController& controller) {
    if (!hudLayoutEditor || !controlMappingHub) {
        return false;
    }
    ensureHudToolStack(controller);
    if (controller.contains(hudLayoutEditor->id())) {
        controller.removeState(hudLayoutEditor->id());
        return true;
    }
    controller.pushState(hudLayoutEditor);
    return true;
}

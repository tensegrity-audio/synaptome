#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <queue>
#include <utility>
#include <vector>

#include "../synaptome/src/core/ParameterRegistry.h"
#include "../synaptome/src/ui/MenuController.h"
#include "../synaptome/src/ui/MenuController.cpp"
#include "../synaptome/src/ui/HotkeyManager.cpp"
#define private public
#define protected public
#include "../synaptome/src/ui/ControlMappingHubState.h"
#undef private
#undef protected
#include "../synaptome/src/ui/ColumnControls.h"
#include "../synaptome/src/ui/ColumnControls.cpp"
#include "../synaptome/src/ui/DevicesPanel.h"
#include "../synaptome/src/ui/DevicesPanel.cpp"
#include "../synaptome/src/io/MidiRouter.h"
#include "../synaptome/src/io/MidiRouter.cpp"
#include "../synaptome/src/visuals/LayerFactory.cpp"
#include "../synaptome/src/visuals/LayerLibrary.cpp"
#include "../synaptome/src/visuals/effects/PostEffectChain.h"
#include "../synaptome/src/visuals/effects/PostEffectChain.cpp"
#define TENSEGRITY_CUSTOM_VIDEO_GRABBER_HEADER "../../../tests/stubs/ofVideoGrabber.h"
#include "../synaptome/src/visuals/VideoGrabberLayer.h"
#undef TENSEGRITY_CUSTOM_VIDEO_GRABBER_HEADER
#include "../synaptome/src/visuals/VideoGrabberLayer.cpp"
#include "../synaptome/src/io/ConsoleStore.h"
#include "../synaptome/src/ofJson.h"
#include "../synaptome/src/ui/HudFeedRegistry.h"
// Use headless HUD stubs for native test
#include "HudRegistry.h"
#include "OverlayManager.h"
#include "stubs/SynaptomeTestPaths.h"
#include "stubs/ofEvents.h"
#include "stubs/ofUtils.h"
// HUD registry test includes (removed during iterative test attempt)

namespace browser_flow {
namespace {
std::string escape_json(const std::string& input) {
    std::ostringstream out;
    for (char c : input) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(static_cast<unsigned char>(c));
            } else {
                out << c;
            }
            break;
        }
    }
    return out.str();
}
}

struct ControlHubEvent {
    std::string type;
    std::string parameterId;
    std::string source;
    std::string detail;
    float value = 0.0f;
    uint64_t timestampMs = 0;
};

struct FakeVideoGrabber : VideoGrabberLayer::Grabber {
    struct SetupEvent {
        int deviceId = -1;
        int width = 0;
        int height = 0;
        bool success = false;
    };

    std::vector<ofVideoDevice> devices;
    std::vector<SetupEvent> setupEvents;
    std::vector<std::string> frameHistory;
    int closeCount = 0;
    bool setupShouldSucceed = true;

    std::vector<ofVideoDevice> listDevices() override { return devices; }

    bool isInitialized() const override { return initialized_; }

    void close() override { initialized_ = false; ++closeCount; }

    void update() override {
        if (!initialized_) {
            lastFrameNew_ = false;
            return;
        }
        if (pendingFrames_.empty()) {
            lastFrameNew_ = false;
            return;
        }
        lastFrameNew_ = pendingFrames_.front();
        pendingFrames_.pop();
        if (lastFrameNew_) {
            frameHistory.push_back(currentDeviceLabel());
        }
    }

    bool isFrameNew() const override { return initialized_ && lastFrameNew_; }

    float getWidth() const override { return width_; }
    float getHeight() const override { return height_; }

    ofTexture* getTexture() override { return nullptr; }

    void draw(float, float, float, float) override {}

    void setDeviceID(int id) override { currentDeviceId_ = id; }

    void setDesiredFrameRate(int fps) override { fps_ = fps; }

    bool setup(int width, int height) override {
        SetupEvent evt;
        evt.deviceId = currentDeviceId_;
        evt.width = width;
        evt.height = height;
        evt.success = setupShouldSucceed;
        setupEvents.push_back(evt);
        if (!setupShouldSucceed) {
            initialized_ = false;
            return false;
        }
        width_ = static_cast<float>(width);
        height_ = static_cast<float>(height);
        initialized_ = true;
        return true;
    }

    void queueFrame(bool isNew = true) { pendingFrames_.push(isNew); }

    std::string currentDeviceLabel() const {
        auto it = std::find_if(devices.begin(), devices.end(), [&](const ofVideoDevice& dev) {
            return dev.id == currentDeviceId_;
        });
        return it != devices.end() ? it->deviceName : std::string();
    }

private:
    bool initialized_ = false;
    bool lastFrameNew_ = false;
    int currentDeviceId_ = -1;
    int fps_ = 0;
    float width_ = 640.0f;
    float height_ = 360.0f;
    std::queue<bool> pendingFrames_;
};

class ControlHubHarness {
public:
    ControlHubHarness();

    void addFloatParameter(std::string id,
                           std::string label,
                           std::string group,
                           float defaultValue,
                           float minValue,
                           float maxValue);

    void bindKey(const std::string& id, int key, float delta, std::string label);
    void assignMidi(const std::string& id, int cc, float outMin, float outMax, std::string label);
    void assignOsc(const std::string& id,
                   std::string address,
                   float inMin,
                   float inMax,
                   float outMin,
                   float outMax,
                   std::string label);

    void simulateKeyPress(int key);
    void simulateMidi(int cc, float normalized);
    void simulateOsc(const std::string& address, float reading);

    void writeArtifact(const std::filesystem::path& path) const;

    float valueOf(const std::string& id) const;
    int countEvents(const std::string& type) const;

private:
    struct FloatSignal {
        std::string id;
        std::string label;
        std::string group;
        float value = 0.0f;
        std::vector<float> history;
    };

    struct HarnessState : MenuController::State {
        const std::string id_ = "tests.control_hub";
        const std::string label_ = "Control Hub Harness";
        const std::string scope_ = "ControlHub";
        MenuController::StateView view_;

        const std::string& id() const override { return id_; }
        const std::string& label() const override { return label_; }
        const std::string& scope() const override { return scope_; }
        MenuController::StateView view() const override { return view_; }
        bool handleInput(MenuController&, int) override { return false; }

        void setView(MenuController::StateView v) { view_ = std::move(v); }
    };

    struct KeyBinding {
        std::string parameterId;
        int key = 0;
        float delta = 0.0f;
        std::string label;
    };

    struct MidiBinding {
        std::string parameterId;
        int cc = -1;
        float outMin = 0.0f;
        float outMax = 1.0f;
        std::string label;
    };

    struct OscBinding {
        std::string parameterId;
        std::string address;
        float inMin = 0.0f;
        float inMax = 1.0f;
        float outMin = 0.0f;
        float outMax = 1.0f;
        std::string label;
    };

    ParameterRegistry::FloatParam& requireFloat(const std::string& id);
    const ParameterRegistry::FloatParam& requireFloat(const std::string& id) const;
    void refreshMenuView();
    void recordSample(const std::string& id, float value);
    float clampToRange(const ParameterRegistry::Range& range, float value) const;
    void logEvent(std::string type,
                  const std::string& parameterId,
                  std::string source,
                  std::string detail,
                  float value);
    void applyKey(const KeyBinding& binding);
    void applyControllerValue(const std::string& id,
                              float value,
                              std::string type,
                              std::string source,
                              std::string detail);

    ParameterRegistry registry_;
    MenuController controller_;
    std::shared_ptr<HarnessState> menuState_;
    std::vector<std::string> parameterOrder_;
    std::unordered_map<std::string, std::unique_ptr<FloatSignal>> floatSignals_;
    std::vector<std::shared_ptr<KeyBinding>> keyBindings_;
    std::vector<MidiBinding> midiBindings_;
    std::vector<OscBinding> oscBindings_;
    std::vector<ControlHubEvent> events_;
    std::chrono::steady_clock::time_point startTime_;
};

ControlHubHarness::ControlHubHarness() : menuState_(std::make_shared<HarnessState>()),
                                         startTime_(std::chrono::steady_clock::now()) {
    controller_.pushState(menuState_);
}

void ControlHubHarness::addFloatParameter(std::string id,
                                          std::string label,
                                          std::string group,
                                          float defaultValue,
                                          float minValue,
                                          float maxValue) {
    auto slot = std::make_unique<FloatSignal>();
    slot->id = id;
    slot->label = label;
    slot->group = group;
    slot->value = defaultValue;
    slot->history.push_back(defaultValue);

    ParameterRegistry::Descriptor descriptor;
    descriptor.id = id;
    descriptor.label = label;
    descriptor.group = group;
    descriptor.range.min = minValue;
    descriptor.range.max = maxValue;
    descriptor.range.step = 0.01f;
    descriptor.quickAccess = true;

    registry_.addFloat(id, &slot->value, defaultValue, descriptor);
    parameterOrder_.push_back(id);
    floatSignals_[id] = std::move(slot);
    refreshMenuView();
}

void ControlHubHarness::bindKey(const std::string& id, int key, float delta, std::string label) {
    auto binding = std::make_shared<KeyBinding>();
    binding->parameterId = id;
    binding->key = key;
    binding->delta = delta;
    binding->label = std::move(label);

    MenuController::HotkeyBinding hotkey;
    hotkey.id = "tests.key." + id + "." + std::to_string(key);
    hotkey.key = key;
    hotkey.callback = [this, binding](MenuController&) {
        applyKey(*binding);
        return true;
    };
    controller_.registerHotkey(hotkey);
    keyBindings_.push_back(binding);
    logEvent("hub.key.bound", id, "KEY", binding->label, registry_.getFloatBase(id));
}

void ControlHubHarness::assignMidi(const std::string& id, int cc, float outMin, float outMax, std::string label) {
    MidiBinding binding;
    binding.parameterId = id;
    binding.cc = cc;
    binding.outMin = outMin;
    binding.outMax = outMax;
    binding.label = std::move(label);
    midiBindings_.push_back(binding);
    logEvent("hub.midi.bound", id, "MIDI", binding.label, registry_.getFloatBase(id));
}

void ControlHubHarness::assignOsc(const std::string& id,
                                  std::string address,
                                  float inMin,
                                  float inMax,
                                  float outMin,
                                  float outMax,
                                  std::string label) {
    OscBinding binding;
    binding.parameterId = id;
    binding.address = std::move(address);
    binding.inMin = inMin;
    binding.inMax = inMax;
    binding.outMin = outMin;
    binding.outMax = outMax;
    binding.label = std::move(label);
    oscBindings_.push_back(binding);
    logEvent("hub.osc.bound", id, "OSC", binding.address + " " + binding.label, registry_.getFloatBase(id));
}

void ControlHubHarness::simulateKeyPress(int key) {
    if (!controller_.handleInput(key)) {
        throw std::runtime_error("MenuController rejected synthetic key press");
    }
}

void ControlHubHarness::simulateMidi(int cc, float normalized) {
    auto it = std::find_if(midiBindings_.begin(), midiBindings_.end(), [&](const MidiBinding& binding) {
        return binding.cc == cc;
    });
    if (it == midiBindings_.end()) {
        throw std::runtime_error("No MIDI binding for CC " + std::to_string(cc));
    }
    float t = std::clamp(normalized, 0.0f, 1.0f);
    float value = it->outMin + (it->outMax - it->outMin) * t;
    applyControllerValue(it->parameterId, value, "hub.midi.input", "MIDI", it->label);
}

void ControlHubHarness::simulateOsc(const std::string& address, float reading) {
    auto it = std::find_if(oscBindings_.begin(), oscBindings_.end(), [&](const OscBinding& binding) {
        return binding.address == address;
    });
    if (it == oscBindings_.end()) {
        throw std::runtime_error("No OSC binding for address " + address);
    }
    float span = it->inMax - it->inMin;
    float t = span == 0.0f ? 0.0f : (reading - it->inMin) / span;
    t = std::clamp(t, 0.0f, 1.0f);
    float value = it->outMin + (it->outMax - it->outMin) * t;
    applyControllerValue(it->parameterId, value, "hub.osc.input", "OSC", it->label);
}

void ControlHubHarness::writeArtifact(const std::filesystem::path& path) const {
    if (!path.empty()) {
        auto dir = path.parent_path();
        if (!dir.empty()) {
            std::filesystem::create_directories(dir);
        }
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open artifact path: " + path.string());
    }
    out << "{\n";
    out << "  \"scenario\": \"browser_flow\",\n";
    out << "  \"parameters\": {\n";
    for (std::size_t i = 0; i < parameterOrder_.size(); ++i) {
        const auto& id = parameterOrder_[i];
        const auto& slot = floatSignals_.at(id);
        out << "    \"" << escape_json(id) << "\": {\n";
        out << "      \"label\": \"" << escape_json(slot->label) << "\",\n";
        out << "      \"group\": \"" << escape_json(slot->group) << "\",\n";
        out << "      \"history\": [";
        for (std::size_t h = 0; h < slot->history.size(); ++h) {
            out << std::fixed << std::setprecision(4) << slot->history[h];
            if (h + 1 < slot->history.size()) {
                out << ", ";
            }
        }
        out << "]\n";
        out << "    }" << (i + 1 < parameterOrder_.size() ? "," : "") << "\n";
    }
    out << "  },\n";
    out << "  \"events\": [\n";
    for (std::size_t i = 0; i < events_.size(); ++i) {
        const auto& ev = events_[i];
        out << "    { \"type\": \"" << escape_json(ev.type) << "\", \"parameterId\": \""
            << escape_json(ev.parameterId) << "\", \"source\": \"" << escape_json(ev.source)
            << "\", \"detail\": \"" << escape_json(ev.detail) << "\", \"value\": "
            << std::fixed << std::setprecision(4) << ev.value << ", \"timestampMs\": "
            << ev.timestampMs << " }" << (i + 1 < events_.size() ? "," : "") << "\n";
    }
    out << "  ]\n";
    out << "}\n";
}

float ControlHubHarness::valueOf(const std::string& id) const {
    return registry_.getFloatBase(id);
}

int ControlHubHarness::countEvents(const std::string& type) const {
    return static_cast<int>(std::count_if(events_.begin(), events_.end(), [&](const ControlHubEvent& ev) {
        return ev.type == type;
    }));
}

ParameterRegistry::FloatParam& ControlHubHarness::requireFloat(const std::string& id) {
    auto* ptr = registry_.findFloat(id);
    if (!ptr) {
        throw std::runtime_error("Unknown float parameter: " + id);
    }
    return *ptr;
}

const ParameterRegistry::FloatParam& ControlHubHarness::requireFloat(const std::string& id) const {
    auto* ptr = registry_.findFloat(id);
    if (!ptr) {
        throw std::runtime_error("Unknown float parameter: " + id);
    }
    return *ptr;
}

void ControlHubHarness::refreshMenuView() {
    if (!menuState_) {
        return;
    }
    MenuController::StateView view;
    view.entries.reserve(parameterOrder_.size());
    for (const auto& id : parameterOrder_) {
        const auto& slot = floatSignals_.at(id);
        MenuController::EntryView entry;
        entry.id = slot->id;
        entry.label = slot->label;
        entry.description = slot->group;
        entry.selectable = true;
        view.entries.push_back(entry);
    }
    view.selectedIndex = view.entries.empty() ? -1 : 0;
    menuState_->setView(std::move(view));
    controller_.requestViewModelRefresh();
}

void ControlHubHarness::recordSample(const std::string& id, float value) {
    auto it = floatSignals_.find(id);
    if (it == floatSignals_.end()) {
        return;
    }
    it->second->value = value;
    it->second->history.push_back(value);
}

float ControlHubHarness::clampToRange(const ParameterRegistry::Range& range, float value) const {
    if (!std::isfinite(range.min) || !std::isfinite(range.max)) {
        return value;
    }
    float lo = std::min(range.min, range.max);
    float hi = std::max(range.min, range.max);
    return std::clamp(value, lo, hi);
}

void ControlHubHarness::logEvent(std::string type,
                                 const std::string& parameterId,
                                 std::string source,
                                 std::string detail,
                                 float value) {
    ControlHubEvent event;
    event.type = std::move(type);
    event.parameterId = parameterId;
    event.source = std::move(source);
    event.detail = std::move(detail);
    event.value = value;
    auto now = std::chrono::steady_clock::now();
    event.timestampMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count());
    events_.push_back(std::move(event));
}

void ControlHubHarness::applyKey(const KeyBinding& binding) {
    const auto& param = requireFloat(binding.parameterId);
    float candidate = param.baseValue + binding.delta;
    float clamped = clampToRange(param.meta.range, candidate);
    registry_.setFloatBase(binding.parameterId, clamped, true);
    recordSample(binding.parameterId, clamped);
    logEvent("hub.key.triggered", binding.parameterId, "KEY", binding.label, clamped);
}

void ControlHubHarness::applyControllerValue(const std::string& id,
                                             float value,
                                             std::string type,
                                             std::string source,
                                             std::string detail) {
    const auto& param = requireFloat(id);
    float clamped = clampToRange(param.meta.range, value);
    registry_.setFloatBase(id, clamped, true);
    recordSample(id, clamped);
    logEvent(std::move(type), id, std::move(source), std::move(detail), clamped);
}

bool RunScenario(const std::filesystem::path& artifactPath) {
    ControlHubHarness harness;
    harness.addFloatParameter("shaders.wave.amplitude", "Wave Amplitude", "Shaders/Wave", 0.25f, 0.0f, 1.0f);
    harness.addFloatParameter("console.layer.opacity", "Layer Opacity", "Console", 0.80f, 0.0f, 1.0f);

    harness.bindKey("shaders.wave.amplitude", 'a', 0.10f, "+0.10");
    harness.bindKey("shaders.wave.amplitude", 'z', -0.10f, "-0.10");
    harness.assignMidi("shaders.wave.amplitude", 14, 0.0f, 1.0f, "Wave Amp CC14");
    harness.assignMidi("console.layer.opacity", 42, 0.0f, 1.0f, "Layer Opacity CC42");
    harness.assignOsc("shaders.wave.amplitude", "/wave/amp", 0.0f, 1.0f, 0.0f, 1.0f, "Wave Amp OSC");

    harness.simulateKeyPress('a');
    harness.simulateMidi(14, 0.75f);
    harness.simulateOsc("/wave/amp", 0.42f);
    harness.simulateMidi(42, 0.33f);

    harness.writeArtifact(artifactPath);

    const float amp = harness.valueOf("shaders.wave.amplitude");
    if (std::fabs(amp - 0.42f) > 1e-4f) {
        std::ostringstream err;
        err << "Wave amplitude expected 0.42 after OSC input, got " << amp;
        throw std::runtime_error(err.str());
    }
    const float consoleOpacity = harness.valueOf("console.layer.opacity");
    if (std::fabs(consoleOpacity - 0.33f) > 1e-4f) {
        std::ostringstream err;
        err << "Console opacity expected 0.33 after MIDI input, got " << consoleOpacity;
        throw std::runtime_error(err.str());
    }
    if (harness.countEvents("hub.key.triggered") != 1) {
        throw std::runtime_error("Expected one key trigger event");
    }
    if (harness.countEvents("hub.midi.input") != 2) {
        throw std::runtime_error("Expected two MIDI input events");
    }
    if (harness.countEvents("hub.osc.input") != 1) {
        throw std::runtime_error("Expected one OSC input event");
    }
    if (!std::filesystem::exists(artifactPath)) {
        throw std::runtime_error("Artifact not written: " + artifactPath.string());
    }

    // HUD registry synthetic test: verify widget visibility follows toggle changes (headless stubs)
    {
        HudRegistry hud;
        OverlayManager overlay;
        hud.setOverlayManager(&overlay);

        bool hudShowControls = false;
        HudRegistry::Toggle toggle;
        toggle.id = "hud.controls";
        toggle.label = "Control Hints";
        toggle.description = "Show keyboard and mouse hint banner";
        toggle.defaultValue = false;
        toggle.valuePtr = &hudShowControls;
        if (!hud.registerToggle(toggle)) {
            throw std::runtime_error("Failed to register HUD toggle");
        }

        OverlayWidget::Metadata meta;
        meta.id = "hud.controls";
        meta.label = "Control Hints";
        HudRegistry::WidgetDescriptor widgetDesc;
        widgetDesc.metadata = meta;
    widgetDesc.factory = [meta]() -> std::unique_ptr<OverlayWidget> { return std::make_unique<SimpleOverlayWidget>(meta); };
        widgetDesc.toggleId = "hud.controls";
        if (!hud.registerWidget(std::move(widgetDesc))) {
            throw std::runtime_error("Failed to register HUD widget");
        }

        // After registration, overlay should have the widget and its visibility should match the toggle (false)
        auto state = overlay.captureState();
        bool found = false;
        for (const auto& w : state.widgets) {
            if (w.id == "hud.controls") {
                found = true;
                if (w.visible != hudShowControls) {
                    throw std::runtime_error("HUD widget visibility did not match toggle (expected false)");
                }
            }
        }
        if (!found) {
            throw std::runtime_error("HUD widget not found in overlay manager after registration");
        }

        // Toggle it on and verify overlay reflects the change
        if (!hud.setValue("hud.controls", true)) {
            throw std::runtime_error("Failed to set HUD toggle value");
        }
        state = overlay.captureState();
        for (const auto& w : state.widgets) {
            if (w.id == "hud.controls") {
                if (!w.visible) {
                    throw std::runtime_error("HUD widget visibility did not update after toggle (expected true)");
                }
            }
        }
    }
    return true;
}

bool RunMidiMappingFlowScenario(const std::filesystem::path& artifactPath) {
    ParameterRegistry registry;
    float mappedValue = 0.25f;
    ParameterRegistry::Descriptor meta;
    meta.id = "tests.midi.depth";
    meta.label = "MIDI Depth";
    meta.group = "Tests";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    registry.addFloat(meta.id, &mappedValue, mappedValue, meta);

    MidiRouter router;
    router.setTestPortList({"Deck Surface", "Grid Controller"});
    router.bindFloat(meta.id, &mappedValue, meta.range.min, meta.range.max, false, 0.0f);

    ControlMappingHubState hub;
    auto tempRoot = std::filesystem::temp_directory_path() / "cmh_midi_mapping_flow";
    std::filesystem::create_directories(tempRoot);
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    hub.setPreferencesPath((tempRoot / "prefs.json").string());
    hub.setSlotAssignmentsPath((tempRoot / "slots.json").string());

    std::vector<ofJson> eventLog;
    hub.setEventCallback([&](const std::string& payload) {
        try {
            eventLog.push_back(ofJson::parse(payload));
        } catch (...) {
            ofJson fallback = ofJson::object();
            fallback["raw"] = payload;
            eventLog.push_back(std::move(fallback));
        }
    });

    MenuController controller;
    hub.onEnter(controller);
    hub.view();

    if (!hub.debugBeginMidiLearn(meta.id)) {
        throw std::runtime_error("Failed to arm MIDI learn for tests.midi.depth");
    }

    DevicesPanel devices;
    devices.setMidiRouter(&router);
    devices.setDeviceMapsDirectory((tempRoot / "device_maps").string());
    MenuController devicesController;
    devices.onEnter(devicesController);
    auto deviceView = devices.view();
    devices.onExit(devicesController);

    bool deckEnumerated = false;
    ofJson deviceEntries = ofJson::array();
    for (const auto& entry : deviceView.entries) {
        if (entry.id.rfind("device.", 0) != 0) {
            continue;
        }
        ofJson node = ofJson::object();
        node["id"] = entry.id;
        node["label"] = entry.label;
        node["description"] = entry.description;
        if (entry.label.find("Deck Surface") != std::string::npos &&
            entry.label.find("(online)") != std::string::npos) {
            deckEnumerated = true;
        }
        deviceEntries.push_back(std::move(node));
    }
    if (!deckEnumerated) {
        throw std::runtime_error("DevicesPanel did not surface Deck Surface as an online controller");
    }

    std::vector<float> history;
    history.push_back(mappedValue);

    constexpr int kCcNumber = 21;
    ofxMidiMessage learnMsg;
    learnMsg.status = MIDI_CONTROL_CHANGE;
    learnMsg.control = kCcNumber;
    learnMsg.channel = 0;
    learnMsg.value = 96;
    router.newMidiMessage(learnMsg);
    history.push_back(mappedValue);

    const auto& ccMaps = router.getCcMaps();
    auto mapping = std::find_if(ccMaps.begin(), ccMaps.end(), [&](const MidiRouter::CcMap& map) {
        return map.target == meta.id;
    });
    if (mapping == ccMaps.end()) {
        throw std::runtime_error("MidiRouter did not record a CC map for tests.midi.depth");
    }
    const float expected = learnMsg.value / 127.0f;
    if (std::fabs(history.back() - expected) > 0.05f) {
        throw std::runtime_error("Mapped parameter value did not track MIDI hardware input");
    }

    if (!artifactPath.empty()) {
        std::filesystem::create_directories(artifactPath.parent_path());
        ofJson artifact = ofJson::object();
        artifact["scenario"] = "midi_mapping_flow";
        artifact["ports"] = router.availableInputPorts();
        artifact["devices"] = std::move(deviceEntries);
        ofJson mappingNode = ofJson::object();
        mappingNode["parameter"] = mapping->target;
        mappingNode["cc"] = mapping->cc;
        mappingNode["channel"] = mapping->channel;
        mappingNode["outMin"] = mapping->outMin;
        mappingNode["outMax"] = mapping->outMax;
        mappingNode["finalValue"] = history.back();
        mappingNode["samples"] = history;
        artifact["mapping"] = std::move(mappingNode);
        ofJson eventsNode = ofJson::array();
        for (const auto& evt : eventLog) {
            eventsNode.push_back(evt);
        }
        artifact["events"] = std::move(eventsNode);
        std::ofstream out(artifactPath, std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Failed to write MIDI mapping flow artifact");
        }
        out << std::setw(2) << artifact << "\n";
    }

    hub.onExit(controller);
    router.clearTestPortList();
    return true;
}

bool RunSlotDropdownFocusScenario() {
    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    const auto deviceDir = synaptome_test_paths::deviceMapsRoot();
    hub.setDeviceMapsDirectory(deviceDir.string());

    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "cmh_slot_dropdown_focus";
    const auto layersDir = tempRoot / "layers";
    std::filesystem::create_directories(layersDir);
    const std::filesystem::path assetPath = layersDir / "tests.asset.dropdown.json";
    if (!std::filesystem::exists(assetPath)) {
        std::ofstream out(assetPath);
        if (!out) {
            throw std::runtime_error("Failed to create slot dropdown focus asset");
        }
        out << R"JSON({
    "id":"tests.asset.dropdown",
    "label":"Dropdown Asset",
    "category":"Tests",
    "type":"generative.perlin",
    "registryPrefix":"tests.asset.dropdown"
})JSON";
    }

    LayerLibrary library;
    if (!library.reload(layersDir.string())) {
        throw std::runtime_error("Failed to load slot dropdown asset library");
    }
    hub.setLayerLibrary(&library);
    const auto* entry = library.find("tests.asset.dropdown");
    if (!entry) {
        throw std::runtime_error("Dropdown asset missing from catalog");
    }
    hub.setConsoleAssetResolver([entry](const std::string& prefix) -> const LayerLibrary::Entry* {
        if (!entry) {
            return nullptr;
        }
        if (prefix.rfind("console.layer", 0) == 0) {
            return entry;
        }
        if (prefix == entry->registryPrefix || prefix == entry->id) {
            return entry;
        }
        return nullptr;
    });
    hub.setConsoleSlotInventoryCallback([entry]() {
        std::vector<ConsoleLayerInfo> slots;
        ConsoleLayerInfo info;
        info.index = 1;
        info.assetId = entry->id;
        info.active = true;
        info.label = entry->label;
        slots.push_back(info);
        return slots;
    });

    ParameterRegistry::Descriptor meta;
    meta.label = "Dropdown Opacity";
    meta.group = "Console";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    float opacity = 0.5f;
    registry.addFloat("console.layer1.opacity", &opacity, opacity, meta);

    MenuController controller;
    hub.onEnter(controller);
    hub.view();

    hub.rebuildView();
    const auto& rows = hub.activeRowIndices();
    if (rows.empty()) {
        throw std::runtime_error("No parameter rows available for slot dropdown focus scenario");
    }
    const int slotColumn = static_cast<int>(ControlMappingHubState::Column::kSlot);
    if (!hub.debugSetGridSelection(0, slotColumn)) {
        throw std::runtime_error("Failed to focus slot column in dropdown focus scenario");
    }
    if (!hub.handleInput(controller, OF_KEY_RETURN)) {
        throw std::runtime_error("Slot picker hotkey was not handled in dropdown focus scenario");
    }
    if (!hub.debugSlotPickerVisible()) {
        throw std::runtime_error("Slot picker did not open after pressing Enter in dropdown focus scenario");
    }
    hub.cancelSlotPicker();
    hub.onExit(controller);
    return true;
}

bool RunSlotBindingRefreshScenario() {
    const auto deviceDir = synaptome_test_paths::deviceMapsRoot();

    ParameterRegistry registry;
    ParameterRegistry::Descriptor meta;
    meta.label = "Tests Asset Opacity";
    meta.group = "Console";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    float col1Value = 0.2f;
    meta.id = "console.layer1.tests.asset.opacity";
    registry.addFloat(meta.id, &col1Value, col1Value, meta);
    float col5Value = 0.8f;
    meta.id = "console.layer5.tests.asset.opacity";
    registry.addFloat(meta.id, &col5Value, col5Value, meta);

    MidiRouter router;
    router.bindFloat("console.layer1.tests.asset.opacity", &col1Value, 0.0f, 1.0f, false, 0.0f);
    router.bindFloat("console.layer5.tests.asset.opacity", &col5Value, 0.0f, 1.0f, false, 0.0f);

    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    hub.setMenuSkin(MenuSkin::ConsoleHub());
    hub.setDeviceMapsDirectory(deviceDir.string());
    hub.slotAssignmentsLoaded_ = true;
    ControlMappingHubState::LogicalSlotBinding binding;
    binding.deviceId = "MIDI Mix 0";
    binding.deviceName = "MIDI Mix 0";
    binding.slotId = "K1";
    binding.slotLabel = "Knob 1";
    binding.analog = true;
    hub.slotAssignments_["tests.asset::tests.asset.opacity"] = binding;

    MenuController controller;
    hub.onEnter(controller);
    hub.view();

    auto findMapForTarget = [&](const std::string& target) -> const MidiRouter::CcMap* {
        const auto& ccMaps = router.getCcMaps();
        for (const auto& map : ccMaps) {
            if (map.target == target) {
                return &map;
            }
        }
        return nullptr;
    };

    ControlMappingHubState::ParameterRow row;
    row.isAsset = true;
    row.assetKey = "tests.asset";
    row.assetLabel = "Tests Asset";
    row.familyLabel = "Tests";
    row.isFloat = true;
    row.category = "Console";
    row.subcategory = "Tests Asset";

    auto applyBindingForColumn = [&](int column) -> const MidiRouter::CcMap* {
        if (column == 1) {
            row.id = "console.layer1.tests.asset.opacity";
            row.floatParam = registry.findFloat(row.id);
        } else {
            row.id = "console.layer5.tests.asset.opacity";
            row.floatParam = registry.findFloat(row.id);
        }
        row.consoleSlots.clear();
        row.consoleSlots.push_back(column);
        if (!hub.applySlotAssignmentToRow(row)) {
            throw std::runtime_error("Failed to apply slot assignment to row");
        }
        return findMapForTarget(row.id);
    };

    const auto* map1 = applyBindingForColumn(1);
    if (!map1 || map1->columnId != "column1") {
        throw std::runtime_error("Column 1 binding metadata incorrect");
    }
    ofxMidiMessage msg;
    msg.status = MIDI_CONTROL_CHANGE;
    msg.channel = map1->channel < 0 ? 0 : map1->channel;
    msg.control = map1->cc;
    msg.value = 100;
    router.newMidiMessage(msg);
    float normalized = msg.value / 127.0f;
    if (std::fabs(col1Value - normalized) > 0.1f) {
        throw std::runtime_error("Column 1 parameter did not follow MIDI input");
    }

    router.removeMidiMappingsForTarget("console.layer1.tests.asset.opacity");

    const auto* map5 = applyBindingForColumn(5);
    if (!map5 || map5->columnId != "column5") {
        throw std::runtime_error("Column 5 binding metadata incorrect");
    }
    msg.channel = map5->channel < 0 ? 0 : map5->channel;
    msg.control = map5->cc;
    msg.value = 64;
    router.newMidiMessage(msg);
    normalized = msg.value / 127.0f;
    if (std::fabs(col5Value - normalized) > 0.1f) {
        throw std::runtime_error("Column 5 parameter did not follow MIDI input after rebinding");
    }

    hub.onExit(controller);
    return true;
}

struct SensorSample {
    std::string parameterId;
    float value = 0.0f;
    uint64_t timestampMs = 0;
};

bool RunOscIngestFlowScenario(const std::filesystem::path& artifactPath) {
    HudRegistry hud;
    OverlayManager overlay;
    hud.setOverlayManager(&overlay);

    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);

    std::vector<ofJson> telemetryEvents;
    hub.setEventCallback([&](const std::string& payload) {
        telemetryEvents.push_back(ofJson::parse(payload));
    });

    std::vector<SensorSample> samples;

    auto parseBioDetail = [&](const std::string& detail,
                              std::string& parameterId,
                              std::string& metricName,
                              float& value) -> bool {
        auto pos = detail.find('=');
        if (pos == std::string::npos) {
            return false;
        }
        std::string metric = detail.substr(0, pos);
        std::string number = detail.substr(pos + 1);
        auto trim = [](std::string& text) {
            auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            text.erase(text.begin(), std::find_if(text.begin(), text.end(), [&](unsigned char c) { return !isSpace(c); }));
            text.erase(std::find_if(text.rbegin(), text.rend(), [&](unsigned char c) { return !isSpace(c); }).base(), text.end());
        };
        trim(metric);
        trim(number);
        std::string normalized = ofToLower(metric);
        if (normalized == "bioamp-raw") {
            parameterId = "sensors.bioamp.raw";
            metricName = "bioamp-raw";
        } else if (normalized == "bioamp-signal") {
            parameterId = "sensors.bioamp.signal";
            metricName = "bioamp-signal";
        } else if (normalized == "bioamp-mean") {
            parameterId = "sensors.bioamp.mean";
            metricName = "bioamp-mean";
        } else if (normalized == "bioamp-rms") {
            parameterId = "sensors.bioamp.rms";
            metricName = "bioamp-rms";
        } else if (normalized == "bioamp-dom-hz") {
            parameterId = "sensors.bioamp.dom_hz";
            metricName = "bioamp-dom-hz";
        } else if (normalized == "bioamp-sample-rate" || normalized == "sample_rate") {
            parameterId = "sensors.bioamp.sample_rate";
            metricName = "bioamp-sample-rate";
        } else if (normalized == "bioamp-window" || normalized == "window") {
            parameterId = "sensors.bioamp.window";
            metricName = "bioamp-window";
        } else {
            return false;
        }
        char* endPtr = nullptr;
        value = std::strtof(number.c_str(), &endPtr);
        if (endPtr == number.c_str()) {
            return false;
        }
        return true;
    };

    const std::vector<std::string> events = {
        R"({"type":"sensor.bioamp","detail":"bioamp-raw=0.125","timestampMs":101})",
        R"({"type":"sensor.bioamp","detail":"bioamp-signal=0.082","timestampMs":135})",
        R"({"type":"sensor.bioamp","detail":"bioamp-rms=0.211","timestampMs":150})",
        R"({"type":"sensor.bioamp","detail":"bioamp-sample-rate=512","timestampMs":188})",
        R"({"type":"sensor.bioamp","detail":"bioamp-window=256","timestampMs":222})"
    };

    struct DeviceCoverage {
        bool matrixBioamp = false;
        bool matrixMic = false;
        bool deck = false;
        bool hostMic = false;
    } coverage;

    auto trackCoverage = [&](const std::string& address) {
        auto tokens = ofSplitString(address, "/", true, true);
        if (tokens.size() < 4 || tokens.front() != "sensor") {
            return;
        }
        std::string device = ofToLower(tokens[1]);
        std::string metric = ofToLower(tokens.back());
        std::string scope = tokens.size() >= 3 ? ofToLower(tokens[2]) : std::string();
        auto startsWith = [](const std::string& value, const std::string& needle) {
            return value.rfind(needle, 0) == 0;
        };
        if (device == "matrix" || device == "matrixportal") {
            if (startsWith(metric, "bioamp")) {
                coverage.matrixBioamp = true;
            }
            if (startsWith(metric, "mic")) {
                coverage.matrixMic = true;
            }
        } else if (device == "deck" || device == "cyberdeck") {
            coverage.deck = true;
        } else if (device == "host" && scope == "localmic") {
            coverage.hostMic = true;
        }
    };

    for (const auto& event : events) {
        auto json = ofJson::parse(event);
        const std::string detail = json.value("detail", std::string());
        std::string parameterId;
        std::string metricName;
        float value = 0.0f;
        if (!parseBioDetail(detail, parameterId, metricName, value)) {
            throw std::runtime_error("Failed to parse sensor.bioamp detail: " + detail);
        }
        uint64_t timestampMs = json.value("timestampMs", static_cast<uint64_t>(0));
        samples.push_back(SensorSample{parameterId, value, timestampMs});
        hub.setBioAmpMetric(metricName, value, timestampMs);
        coverage.matrixBioamp = true;
    }

    const std::vector<std::pair<std::string, float>> oscMessages = {
        {"/sensor/matrix/0x0101/mic-level", 0.42f},
        {"/sensor/deck/0x0201/deck-intensity", 0.77f},
        {"/sensor/host/localmic/mic-level", 0.58f}
    };
    for (const auto& msg : oscMessages) {
        trackCoverage(msg.first);
    }

    if (samples.size() != events.size()) {
        throw std::runtime_error("Sensor sink did not capture each OSC event");
    }

    const auto& bioState = hub.bioAmpState();
    auto expectValid = [&](bool valid, float value, float expected, const char* label) {
        if (!valid || std::fabs(value - expected) > 1e-4f) {
            std::ostringstream oss;
            oss << "BioAmp metric '" << label << "' mismatch (value=" << value << ")";
            throw std::runtime_error(oss.str());
        }
    };
    expectValid(bioState.raw.valid, bioState.raw.value, 0.125f, "raw");
    expectValid(bioState.signal.valid, bioState.signal.value, 0.082f, "signal");
    expectValid(bioState.rms.valid, bioState.rms.value, 0.211f, "rms");
    if (std::fabs(static_cast<float>(bioState.sampleRate) - 512.0f) > 1e-4f) {
        throw std::runtime_error("BioAmp sample_rate mismatch");
    }
    if (std::fabs(static_cast<float>(bioState.windowSize) - 256.0f) > 1e-4f) {
        throw std::runtime_error("BioAmp window mismatch");
    }
    if (telemetryEvents.size() < samples.size()) {
        throw std::runtime_error("ControlMappingHubState telemetry did not emit enough sensor events");
    }

    if (!coverage.matrixBioamp || !coverage.matrixMic || !coverage.deck || !coverage.hostMic) {
        throw std::runtime_error("OSC coverage did not mark matrix/deck/host sensors as present");
    }

    if (!artifactPath.empty()) {
        std::filesystem::create_directories(artifactPath.parent_path());
        ofJson artifact = ofJson::object();
        artifact["scenario"] = "osc_ingest_flow";
        ofJson eventArray = ofJson::array();
        for (const auto& entry : events) {
            eventArray.push_back(entry);
        }
        artifact["events"] = std::move(eventArray);
        ofJson samplesArray = ofJson::array();
        for (const auto& sample : samples) {
            ofJson node = ofJson::object();
            node["parameterId"] = sample.parameterId;
            node["value"] = sample.value;
            node["timestampMs"] = sample.timestampMs;
            samplesArray.push_back(std::move(node));
        }
        artifact["samples"] = std::move(samplesArray);
        ofJson oscArray = ofJson::array();
        for (const auto& msg : oscMessages) {
            ofJson node = ofJson::object();
            node["address"] = msg.first;
            node["value"] = msg.second;
            oscArray.push_back(std::move(node));
        }
        artifact["oscMessages"] = std::move(oscArray);
        artifact["coverage"] = {
            {"matrixBioamp", coverage.matrixBioamp},
            {"matrixMic", coverage.matrixMic},
            {"deck", coverage.deck},
            {"hostMic", coverage.hostMic}
        };
        std::ofstream out(artifactPath, std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Failed to write OSC ingest artifact");
        }
        out << std::setw(2) << artifact << "\n";
    }

    return true;
}

bool RunWebcamReplayScenario(const std::filesystem::path& artifactPath) {
    auto grabber = std::make_shared<FakeVideoGrabber>();
    ofVideoDevice integrated;
    integrated.id = 0;
    integrated.deviceName = "Integrated Webcam";
    integrated.hardwareName = "integrated";
    integrated.bAvailable = true;
    ofVideoDevice deckCam;
    deckCam.id = 4;
    deckCam.deviceName = "Deck Capture";
    deckCam.hardwareName = "deckcapture";
    deckCam.bAvailable = false;
    grabber->devices = {integrated, deckCam};

    VideoGrabberLayer layer;
    layer.setGrabberForTesting(grabber);
    ParameterRegistry registry;
    ofJson config = ofJson::object();
    config["defaults"] = {
        {"deviceIndex", 0},
        {"gain", 1.25f},
        {"width", 640},
        {"height", 360},
        {"fps", 30}
    };
    config["resolutions"] = ofJson::array(
        {ofJson::object({{"width", 640}, {"height", 360}}),
         ofJson::object({{"width", 960}, {"height", 540}})});
    layer.setRegistryPrefix("layer.webcam");
    layer.configure(config);
    layer.setup(registry);
    auto* deviceParam = registry.findFloat("layer.webcam.device");
    if (!deviceParam) {
        throw std::runtime_error("Webcam device parameter not registered");
    }
    if (deviceParam->meta.description.find("Integrated Webcam") == std::string::npos) {
        throw std::runtime_error("Webcam device metadata did not include device labels");
    }
    auto* overlayParam = registry.findBool("layer.webcam.deviceInfoOverlay");
    if (!overlayParam) {
        throw std::runtime_error("Webcam device overlay toggle not registered");
    }

    if (grabber->setupEvents.empty() || !grabber->setupEvents.back().success) {
        throw std::runtime_error("Webcam layer did not attempt initial setup");
    }

    auto lazyGrabber = std::make_shared<FakeVideoGrabber>();
    lazyGrabber->devices = {integrated, deckCam};
    VideoGrabberLayer lazyLayer;
    lazyLayer.setGrabberForTesting(lazyGrabber);
    ParameterRegistry lazyRegistry;
    ofJson lazyConfig = config;
    lazyConfig["defaults"]["deferOpen"] = true;
    lazyConfig["defaults"]["deferredOpenDelayMs"] = 0;
    lazyConfig["defaults"]["deferredOpenFrames"] = 1;
    lazyLayer.setRegistryPrefix("layer.webcam.lazy");
    lazyLayer.configure(lazyConfig);
    lazyLayer.setup(lazyRegistry);
    if (!lazyGrabber->setupEvents.empty()) {
        throw std::runtime_error("Lazy webcam layer opened during setup");
    }

    LayerUpdateParams updateParams;
    updateParams.dt = 1.0f / 60.0f;

    lazyLayer.update(updateParams);
    if (!lazyGrabber->setupEvents.empty()) {
        throw std::runtime_error("Lazy webcam layer ignored frame delay");
    }
    lazyLayer.update(updateParams);
    if (lazyGrabber->setupEvents.empty() || !lazyGrabber->setupEvents.back().success) {
        throw std::runtime_error("Lazy webcam layer did not open after deferred updates");
    }

    auto lazyFailureGrabber = std::make_shared<FakeVideoGrabber>();
    lazyFailureGrabber->devices = {integrated};
    lazyFailureGrabber->setupShouldSucceed = false;
    VideoGrabberLayer lazyFailureLayer;
    lazyFailureLayer.setGrabberForTesting(lazyFailureGrabber);
    ParameterRegistry lazyFailureRegistry;
    lazyFailureLayer.setRegistryPrefix("layer.webcam.lazyFailure");
    lazyFailureLayer.configure(lazyConfig);
    lazyFailureLayer.setup(lazyFailureRegistry);
    lazyFailureLayer.update(updateParams);
    lazyFailureLayer.update(updateParams);
    if (lazyFailureGrabber->setupEvents.empty() || lazyFailureGrabber->setupEvents.back().success) {
        throw std::runtime_error("Lazy webcam setup failure was not handled");
    }

    grabber->queueFrame(true);
    layer.update(updateParams);

    grabber->setupShouldSucceed = false;
    grabber->close();
    const std::size_t failureStart = grabber->setupEvents.size();
    layer.forceDeviceRefresh();
    if (grabber->setupEvents.size() == failureStart) {
        throw std::runtime_error("Webcam refresh did not trigger a setup attempt");
    }
    bool failureLogged = false;
    for (std::size_t i = failureStart; i < grabber->setupEvents.size(); ++i) {
        if (!grabber->setupEvents[i].success) {
            failureLogged = true;
            break;
        }
    }
    if (!failureLogged) {
        throw std::runtime_error("Webcam refresh failure was not detected");
    }

    grabber->setupShouldSucceed = true;
    layer.forceDeviceRefresh();
    if (!grabber->setupEvents.back().success) {
        throw std::runtime_error("Webcam did not recover after failure");
    }

    grabber->queueFrame(true);
    layer.update(updateParams);

    ofJson artifact = ofJson::object();
    artifact["scenario"] = "webcam_replay_flow";
    ofJson deviceArray = ofJson::array();
    for (const auto& dev : grabber->devices) {
        deviceArray.push_back({
            {"id", dev.id},
            {"label", dev.deviceName},
            {"available", dev.bAvailable}
        });
    }
    artifact["devices"] = std::move(deviceArray);

    ofJson setupArray = ofJson::array();
    for (const auto& evt : grabber->setupEvents) {
        setupArray.push_back({
            {"deviceId", evt.deviceId},
            {"width", evt.width},
            {"height", evt.height},
            {"success", evt.success}
        });
    }
    artifact["setups"] = std::move(setupArray);
    artifact["lazySetups"] = lazyGrabber->setupEvents.size();
    artifact["lazyFailureSetups"] = lazyFailureGrabber->setupEvents.size();
    artifact["closeCount"] = grabber->closeCount;
    artifact["frames"] = grabber->frameHistory;

    std::filesystem::create_directories(artifactPath.parent_path());
    std::ofstream out(artifactPath, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to write webcam_replay_flow artifact");
    }
    out << std::setw(2) << artifact << "\n";
    return true;
}

bool RunConsoleSlotHotkeyScenario() {
    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    const auto deviceDir = synaptome_test_paths::deviceMapsRoot();
    hub.setDeviceMapsDirectory(deviceDir.string());

    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "cmh_slot_hotkeys";
    std::filesystem::create_directories(tempRoot);
    const auto layersDir = tempRoot / "layers";
    std::filesystem::create_directories(layersDir);
    const std::filesystem::path assetPath = layersDir / "tests.asset.json";
    if (!std::filesystem::exists(assetPath)) {
        std::ofstream out(assetPath);
        out << R"JSON({
    "id":"tests.asset.simple",
    "label":"Test Asset",
    "category":"Tests",
    "type":"fx.dither",
    "registryPrefix":"tests.asset.simple"
})JSON";
    }

    LayerLibrary library;
    if (!library.reload(assetPath.parent_path().string())) {
        throw std::runtime_error("Failed to load test asset library");
    }
    hub.setLayerLibrary(&library);
    hub.setConsoleAssetResolver([entry = library.find("tests.asset.simple")](const std::string& prefix) -> const LayerLibrary::Entry* {
        if (!entry) {
            return nullptr;
        }
        if (prefix.rfind("console.layer", 0) == 0) {
            return entry;
        }
        if (prefix == entry->registryPrefix || prefix == entry->id) {
            return entry;
        }
        return nullptr;
    });
    hub.setPreferencesPath((tempRoot / "prefs.json").string());
    hub.setSlotAssignmentsPath((tempRoot / "slots.json").string());
    ParameterRegistry::Descriptor meta;
    meta.label = "Layer Opacity";
    meta.group = "Console";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    float slotOpacity = 0.64f;
    registry.addFloat("console.layer1.opacity", &slotOpacity, slotOpacity, meta);

    float fxCoverage = 0.0f;
    ParameterRegistry::Descriptor coverageMeta;
    coverageMeta.label = "Dither Coverage";
    coverageMeta.group = "Effects";
    coverageMeta.range.min = 0.0f;
    coverageMeta.range.max = 8.0f;
    coverageMeta.range.step = 1.0f;
    registry.addFloat("effects.dither.coverage", &fxCoverage, fxCoverage, coverageMeta);

    float assetOpacity = 0.5f;
    ParameterRegistry::Descriptor assetMeta;
    assetMeta.label = "Test Asset Opacity";
    assetMeta.group = "Tests";
    assetMeta.range.min = 0.0f;
    assetMeta.range.max = 1.0f;
    registry.addFloat("tests.asset.simple.opacity", &assetOpacity, assetOpacity, assetMeta);

    std::vector<ConsoleLayerInfo> inventory;
    hub.setConsoleSlotInventoryCallback([&]() {
        return inventory;
    });

    std::vector<std::pair<int, std::string>> loadRequests;
    std::vector<int> unloadRequests;
    ControlMappingHubState* hubPtr = &hub;
    hub.setConsoleSlotLoadCallback([&, hubPtr](int slotIndex, const std::string& assetId) {
        loadRequests.emplace_back(slotIndex, assetId);
        inventory.clear();
        ConsoleLayerInfo info;
        info.index = slotIndex;
        info.assetId = assetId;
        info.active = true;
        inventory.push_back(info);
        hubPtr->markConsoleSlotsDirty();
        return true;
    });
    hub.setConsoleSlotUnloadCallback([&, hubPtr](int slotIndex) {
        unloadRequests.push_back(slotIndex);
        inventory.clear();
        hubPtr->markConsoleSlotsDirty();
        return true;
    });

    MenuController controller;
    hub.onEnter(controller);
    hub.view();
    hub.rebuildView();
    int assetRowIndex = -1;
    for (std::size_t i = 0; i < hub.tableModel_.rows.size(); ++i) {
        const auto& row = hub.tableModel_.rows[i];
        if (row.assetKey == "tests.asset.simple") {
            assetRowIndex = static_cast<int>(i);
            break;
        }
    }
    if (assetRowIndex < 0) {
        throw std::runtime_error("Failed to select test asset row for console slot hotkey scenario");
    }
    hub.selectedRow_ = assetRowIndex;
    hub.selectedColumn_ = ControlMappingHubState::Column::kName;
    hub.focusPane_ = ControlMappingHubState::FocusPane::kGrid;

    int loadKey = MenuController::HOTKEY_MOD_CTRL | '1';
    hub.handleInput(controller, loadKey);
    if (loadRequests.size() != 1) {
        throw std::runtime_error("Expected one console slot load request");
    }
    if (loadRequests.front().second != "tests.asset.simple") {
        throw std::runtime_error("Unexpected asset id in load request: " + loadRequests.front().second);
    }

    hub.rebuildView();
    const auto& rows = hub.activeRowIndices();
    if (rows.empty()) {
        throw std::runtime_error("No parameter rows available after loading console slot");
    }
    const int slotColumn = static_cast<int>(ControlMappingHubState::Column::kSlot);
    int targetRowIndex = -1;
    for (std::size_t i = 0; i < hub.tableModel_.rows.size(); ++i) {
        const auto& row = hub.tableModel_.rows[i];
        if (row.id == "console.layer1.opacity") {
            targetRowIndex = static_cast<int>(i);
            break;
        }
    }
    if (targetRowIndex < 0) {
        throw std::runtime_error("Console layer opacity row not available for slot picker scenario");
    }
    hub.selectedRow_ = targetRowIndex;
    hub.selectedColumn_ = static_cast<ControlMappingHubState::Column>(slotColumn);
    hub.focusPane_ = ControlMappingHubState::FocusPane::kGrid;
    if (!hub.handleInput(controller, OF_KEY_RETURN)) {
        throw std::runtime_error("Slot picker hotkey was not handled in console slot scenario");
    }
    if (!hub.debugSlotPickerVisible()) {
        throw std::runtime_error("Slot picker did not open after pressing Enter in console slot scenario");
    }
    hub.cancelSlotPicker();

    int unloadKey = MenuController::HOTKEY_MOD_CTRL | MenuController::HOTKEY_MOD_SHIFT | '1';
    hub.handleInput(controller, unloadKey);
    if (unloadRequests.size() != 1 || unloadRequests.front() != 1) {
        throw std::runtime_error("Expected one console slot unload request for slot 1");
    }

    hub.onExit(controller);
    if (!registry.findFloat("effects.dither.coverage")) {
        throw std::runtime_error("effects.dither.coverage parameter not registered for coverage test");
    }
    if (registry.findFloat("console.layer1.coverage")) {
        throw std::runtime_error("console.layer1.coverage parameter should no longer be user-facing");
    }
    return true;
}

bool RunCoverageWindowLogicScenario() {
    PostEffectChain chain;
    auto expectWindow = [&](int effectColumn,
                            float coverage,
                            int expectedFirst,
                            int expectedLast,
                            bool expectedAll) {
        auto window = chain.resolveCoverageWindow(effectColumn, coverage);
        if (window.firstColumn != expectedFirst || window.lastColumn != expectedLast) {
            std::ostringstream oss;
            oss << "Coverage window mismatch for column " << effectColumn << " value " << coverage
                << " (first=" << window.firstColumn << ", last=" << window.lastColumn
                << ", expected " << expectedFirst << "-" << expectedLast << ")";
            throw std::runtime_error(oss.str());
        }
        if (window.includesAll != expectedAll) {
            std::ostringstream oss;
            oss << "Coverage includesAll mismatch for column " << effectColumn << " value " << coverage;
            throw std::runtime_error(oss.str());
        }
    };

    expectWindow(4, 0.0f, 1, 3, true);
    expectWindow(4, 1.0f, 3, 3, false);
    expectWindow(5, 2.0f, 3, 4, false);
    expectWindow(5, 10.0f, 1, 4, true);

    auto window = chain.resolveCoverageWindow(5, 2.0f);
    auto columnInWindow = [&](int columnIndex) -> bool {
        if (window.lastColumn == 0) return false;
        if (window.includesAll) return columnIndex < 5;
        return columnIndex >= window.firstColumn && columnIndex <= window.lastColumn;
    };
    if (!columnInWindow(3) || !columnInWindow(4)) {
        throw std::runtime_error("Coverage window failed to include the two nearest upstream columns");
    }
    if (columnInWindow(2) || columnInWindow(5)) {
        throw std::runtime_error("Coverage window incorrectly included columns outside the requested range");
    }

    return true;
}

bool RunViewportPersistenceScenario() {
    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    LayerLibrary library;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    hub.setLayerLibrary(&library);

    // Default tree selection lands on the alphabetically first asset (Effects), so
    // allocate enough rows under that asset to force grid scrolling.
    const int kActiveAssetRowCount = 18;
    const int kSecondaryAssetRowCount = 6;
    const int kParamCount = kActiveAssetRowCount + kSecondaryAssetRowCount;
    std::vector<float> values(static_cast<std::size_t>(kParamCount), 0.0f);
    int valueIndex = 0;
    auto appendRowsForAsset = [&](int count, const std::string& group, const std::string& assetPrefix) {
        for (int i = 0; i < count; ++i) {
            ParameterRegistry::Descriptor meta;
            meta.group = group;
            meta.label = "Param " + std::to_string(valueIndex);
            std::string id = assetPrefix + ".control" + std::to_string(valueIndex);
            registry.addFloat(id, &values[static_cast<std::size_t>(valueIndex)], 0.0f, meta);
            ++valueIndex;
        }
    };
    appendRowsForAsset(kActiveAssetRowCount, "Effects", "deckA.effects.asset0");
    appendRowsForAsset(kSecondaryAssetRowCount, "Meshes", "deckA.meshes.asset0");

    MenuController controller;
    hub.onEnter(controller);
    hub.view();

    auto baseline = hub.snapshotViewport(640.0f, 320.0f);
    if (baseline.treeNodeCount <= 0) {
        throw std::runtime_error("Control hub tree did not produce any nodes");
    }

    for (int i = 0; i < kActiveAssetRowCount - 1; ++i) {
        hub.handleInput(controller, OF_KEY_DOWN);
    }
    auto scrolled = hub.snapshotViewport(640.0f, 180.0f);
    if (scrolled.gridScrollOffset <= 0) {
        std::ostringstream oss;
        oss << "Grid did not auto-scroll when moving to the last rows (rows=" << scrolled.gridRowCount
            << ", visible=" << scrolled.gridVisibleRows << ", offset=" << scrolled.gridScrollOffset << ")";
        throw std::runtime_error(oss.str());
    }

    hub.handleInput(controller, OF_KEY_TAB);
    hub.handleInput(controller, OF_KEY_LEFT);
    hub.handleInput(controller, OF_KEY_LEFT);
    auto collapsed = hub.snapshotViewport(640.0f, 180.0f);
    if (collapsed.treeNodeCount >= baseline.treeNodeCount) {
        throw std::runtime_error("Collapsing a category did not reduce the visible tree node count");
    }

    hub.onExit(controller);
    hub.onEnter(controller);
    hub.view();

    auto afterPop = hub.snapshotViewport(640.0f, 180.0f);
    if (afterPop.treeNodeCount != collapsed.treeNodeCount) {
        throw std::runtime_error("Collapsed tree state did not survive push/pop");
    }

    hub.handleInput(controller, OF_KEY_TAB);
    hub.handleInput(controller, OF_KEY_RIGHT);
    auto expandedAgain = hub.snapshotViewport(640.0f, 180.0f);
    if (expandedAgain.treeNodeCount != baseline.treeNodeCount) {
        throw std::runtime_error("Expanding a category after push/pop did not restore the tree nodes");
    }

    hub.onExit(controller);
    return true;
}

bool RunConsoleStorePersistenceScenario() {
    std::vector<ConsoleLayerInfo> layers;
    ConsoleLayerInfo slot1;
    slot1.index = 1;
    slot1.assetId = "geometry.grid";
    slot1.active = true;
    slot1.opacity = 0.75f;
    slot1.label = "Grid";
    slot1.coverage.defined = true;
    slot1.coverage.mode = "upstream";
    slot1.coverage.columns = 0;
    layers.push_back(slot1);

    ConsoleLayerInfo slot2;
    slot2.index = 4;
    slot2.assetId = "generative.perlin";
    slot2.active = false;
    slot2.opacity = 0.5f;
    slot2.displayName = "Perlin Noise";
    slot2.coverage.defined = true;
    slot2.coverage.mode = "upstream";
    slot2.coverage.columns = 3;
    layers.push_back(slot2);

    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "cmh_console_store";
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path storePath = tempRoot / "console.json";
    ConsolePresentationState state;
    state.layers = layers;
    state.overlays.hudVisible = false;
    state.overlays.consoleVisible = true;
    state.overlays.controlHubVisible = false;
    state.overlays.menuVisible = true;
    state.dualDisplay.mode = "dual";
    state.secondaryDisplay.enabled = true;
    state.secondaryDisplay.monitorId = "Controller";
    state.secondaryDisplay.x = 200;
    state.secondaryDisplay.y = 120;
    state.secondaryDisplay.width = 1440;
    state.secondaryDisplay.height = 900;
    state.secondaryDisplay.vsync = false;
    state.secondaryDisplay.dpiScale = 1.25f;
    state.secondaryDisplay.background = "#101010";
    state.secondaryDisplay.followPrimary = false;
    state.controllerFocus.consolePreferred = false;
    state.controllerFocusDefined = true;
    state.overlayLayoutsDefined = true;
    state.overlayLayouts.activeTarget = "controller";
    state.overlayLayouts.lastSyncMs = 4242;
    state.overlayLayouts.projector.capturedAtMs = 128;
    ConsoleOverlayWidgetPlacement projectorWidget;
    projectorWidget.id = "hud.controls";
    projectorWidget.columnIndex = 1;
    projectorWidget.visible = true;
    projectorWidget.collapsed = false;
    projectorWidget.bandId = "hud";
    state.overlayLayouts.projector.widgets.push_back(projectorWidget);
    state.overlayLayouts.controller.capturedAtMs = 256;
    ConsoleOverlayWidgetPlacement controllerWidget;
    controllerWidget.id = "hud.menu";
    controllerWidget.columnIndex = 3;
    controllerWidget.visible = false;
    controllerWidget.collapsed = true;
    controllerWidget.bandId = "hud.bottom";
    state.overlayLayouts.controller.widgets.push_back(controllerWidget);

    state.sensorsDefined = true;
    state.sensors.bioAmpDefined = true;
    state.sensors.bioAmp.hasRaw = true;
    state.sensors.bioAmp.raw = 1.23f;
    state.sensors.bioAmp.rawTimestampMs = 111;
    state.sensors.bioAmp.hasSampleRate = true;
    state.sensors.bioAmp.sampleRate = 256;
    state.sensors.bioAmp.sampleRateTimestampMs = 222;

    if (!ConsoleStore::saveState(storePath.string(), state)) {
        throw std::runtime_error("ConsoleStore::saveState failed");
    }

    auto loadedState = ConsoleStore::loadState(storePath.string());
    if (loadedState.layers.size() != layers.size()) {
        throw std::runtime_error("ConsoleStore::load returned unexpected slot count");
    }
    for (std::size_t i = 0; i < layers.size(); ++i) {
        const auto& expected = layers[i];
        const auto& actual = loadedState.layers[i];
        if (expected.index != actual.index ||
            expected.assetId != actual.assetId ||
            expected.active != actual.active) {
            throw std::runtime_error("ConsoleStore round-trip mismatch on slot metadata");
        }
        if (std::abs(expected.opacity - actual.opacity) > 0.0001f) {
            throw std::runtime_error("ConsoleStore round-trip mismatch on opacity");
        }
        if (!expected.label.empty() && expected.label != actual.label) {
            throw std::runtime_error("ConsoleStore round-trip mismatch on label");
        }
        if (expected.label.empty() && !expected.displayName.empty() && actual.label != expected.displayName) {
            throw std::runtime_error("ConsoleStore did not preserve legacy displayName");
        }
        if (expected.coverage.defined != actual.coverage.defined) {
            throw std::runtime_error("ConsoleStore did not persist coverage defined flag");
        }
        if (expected.coverage.defined) {
            if (expected.coverage.mode != actual.coverage.mode ||
                expected.coverage.columns != actual.coverage.columns) {
                throw std::runtime_error("ConsoleStore did not round-trip coverage fields");
            }
        }
    }
    if (loadedState.overlays.hudVisible != state.overlays.hudVisible ||
        loadedState.overlays.consoleVisible != state.overlays.consoleVisible ||
        loadedState.overlays.controlHubVisible != state.overlays.controlHubVisible ||
        loadedState.overlays.menuVisible != state.overlays.menuVisible) {
        throw std::runtime_error("ConsoleStore did not persist overlay visibility flags");
    }
    if (loadedState.dualDisplay.mode != state.dualDisplay.mode) {
        throw std::runtime_error("ConsoleStore did not persist dual-display mode");
    }
    if (loadedState.secondaryDisplay.enabled != state.secondaryDisplay.enabled ||
        loadedState.secondaryDisplay.monitorId != state.secondaryDisplay.monitorId ||
        loadedState.secondaryDisplay.x != state.secondaryDisplay.x ||
        loadedState.secondaryDisplay.y != state.secondaryDisplay.y ||
        loadedState.secondaryDisplay.width != state.secondaryDisplay.width ||
        loadedState.secondaryDisplay.height != state.secondaryDisplay.height ||
        loadedState.secondaryDisplay.vsync != state.secondaryDisplay.vsync ||
        std::abs(loadedState.secondaryDisplay.dpiScale - state.secondaryDisplay.dpiScale) > 0.0001f ||
        loadedState.secondaryDisplay.background != state.secondaryDisplay.background ||
        loadedState.secondaryDisplay.followPrimary != state.secondaryDisplay.followPrimary ||
        loadedState.controllerFocus.consolePreferred != state.controllerFocus.consolePreferred ||
        !loadedState.controllerFocusDefined) {
        throw std::runtime_error("ConsoleStore did not persist secondary display/controller focus state");
    }
    if (!loadedState.overlayLayoutsDefined) {
        throw std::runtime_error("ConsoleStore did not persist overlay layout snapshots");
    }
    if (loadedState.overlayLayouts.activeTarget != state.overlayLayouts.activeTarget ||
        loadedState.overlayLayouts.lastSyncMs != state.overlayLayouts.lastSyncMs ||
        loadedState.overlayLayouts.projector.capturedAtMs != state.overlayLayouts.projector.capturedAtMs ||
        loadedState.overlayLayouts.controller.capturedAtMs != state.overlayLayouts.controller.capturedAtMs) {
        throw std::runtime_error("ConsoleStore overlay layout metadata mismatch");
    }
    if (loadedState.overlayLayouts.projector.widgets.size() != state.overlayLayouts.projector.widgets.size() ||
        loadedState.overlayLayouts.controller.widgets.size() != state.overlayLayouts.controller.widgets.size()) {
        throw std::runtime_error("ConsoleStore overlay layout widget count mismatch");
    }
    if (!state.overlayLayouts.projector.widgets.empty()) {
        const auto& expected = state.overlayLayouts.projector.widgets.front();
        const auto& actual = loadedState.overlayLayouts.projector.widgets.front();
        if (expected.id != actual.id ||
            expected.columnIndex != actual.columnIndex ||
            expected.visible != actual.visible ||
            expected.collapsed != actual.collapsed ||
            expected.bandId != actual.bandId) {
            throw std::runtime_error("ConsoleStore projector overlay snapshot mismatch");
        }
    }
    if (!state.overlayLayouts.controller.widgets.empty()) {
        const auto& expected = state.overlayLayouts.controller.widgets.front();
        const auto& actual = loadedState.overlayLayouts.controller.widgets.front();
        if (expected.id != actual.id ||
            expected.columnIndex != actual.columnIndex ||
            expected.visible != actual.visible ||
            expected.collapsed != actual.collapsed ||
            expected.bandId != actual.bandId) {
            throw std::runtime_error("ConsoleStore controller overlay snapshot mismatch");
        }
    }

    if (!loadedState.sensorsDefined || !loadedState.sensors.bioAmpDefined) {
        throw std::runtime_error("ConsoleStore did not persist sensor snapshots");
    }
    if (!loadedState.sensors.bioAmp.hasRaw || std::fabs(loadedState.sensors.bioAmp.raw - 1.23f) > 1e-4f) {
        throw std::runtime_error("ConsoleStore bioamp raw mismatch");
    }
    if (loadedState.sensors.bioAmp.rawTimestampMs != 111) {
        throw std::runtime_error("ConsoleStore bioamp timestamp mismatch");
    }
    if (!loadedState.sensors.bioAmp.hasSampleRate || loadedState.sensors.bioAmp.sampleRate != 256) {
        throw std::runtime_error("ConsoleStore bioamp sample rate mismatch");
    }
    if (loadedState.sensors.bioAmp.sampleRateTimestampMs != 222) {
        throw std::runtime_error("ConsoleStore bioamp sample rate timestamp mismatch");
    }

    auto legacyLoaded = ConsoleStore::load(storePath.string());
    if (legacyLoaded.size() != layers.size()) {
        throw std::runtime_error("ConsoleStore::load legacy wrapper mismatch");
    }
    return true;
}

bool RunLayerOpacityParameterScenario() {
    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);

    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "cmh_opacity_param";
    std::filesystem::create_directories(tempRoot);
    const auto layersDir = tempRoot / "layers";
    std::filesystem::create_directories(layersDir);
    const std::filesystem::path assetPath = layersDir / "tests.asset.opacity.json";
    std::ofstream out(assetPath);
    out << R"JSON({
    "id":"tests.asset.opacity",
    "label":"Opacity Asset",
    "category":"Tests",
    "type":"generative.perlin",
    "registryPrefix":"tests.asset.opacity",
    "opacity":0.64
})JSON";
    out.close();

    LayerLibrary library;
    if (!library.reload(layersDir.string())) {
        throw std::runtime_error("Failed to load opacity test asset");
    }
    hub.setLayerLibrary(&library);
    const auto* entry = library.find("tests.asset.opacity");
    if (!entry) {
        throw std::runtime_error("Opacity test asset missing from catalog");
    }
    hub.setConsoleAssetResolver([entry](const std::string& prefix) -> const LayerLibrary::Entry* {
        if (prefix.rfind("console.layer", 0) != 0) {
            return nullptr;
        }
        return entry;
    });
    hub.setConsoleSlotInventoryCallback([entry]() {
        std::vector<ConsoleLayerInfo> slots;
        ConsoleLayerInfo info;
        info.index = 1;
        info.assetId = entry->id;
        info.active = true;
        slots.push_back(info);
        return slots;
    });

    ParameterRegistry::Descriptor meta;
    meta.label = "Layer Opacity";
    meta.group = "Visibility";
    meta.description = "Base layer opacity";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    float slotOpacity = 0.64f;
    registry.addFloat("console.layer1.opacity", &slotOpacity, slotOpacity, meta);

    MenuController controller;
    hub.onEnter(controller);
    hub.view();
    hub.rebuildView();
    hub.onExit(controller);

    bool found = false;
    for (const auto& row : hub.tableModel_.rows) {
        if (row.id == "console.layer1.opacity" || row.assetKey == entry->id) {
            if (row.label == "Layer Opacity") {
                found = true;
                break;
            }
        }
    }
    if (!found) {
        throw std::runtime_error("Layer opacity parameter row not surfaced in CMH view");
    }
    return true;
}

bool RunHudAssetPlacementScenario() {
    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);

    ParameterRegistry::Descriptor hudMeta;
    hudMeta.label = "Control Hints";
    hudMeta.group = "HUD";
    hudMeta.description = "Toggle control hints widget";
    bool controlsVisible = true;
    registry.addBool("hud.controls", &controlsVisible, controlsVisible, hudMeta);

    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "cmh_hud_assets";
    const auto hudDir = tempRoot / "hud";
    std::filesystem::create_directories(hudDir);
    const std::filesystem::path assetPath = hudDir / "controls.json";
    std::ofstream out(assetPath);
    out << R"JSON({
    "id":"hud.controls",
    "label":"Control Hints",
    "category":"HUD",
    "type":"ui.hud.widget",
    "registryPrefix":"hud.controls",
    "hudWidget":{
        "module":"controls",
        "toggleId":"hud.controls",
        "defaultBand":"console",
        "defaultColumn":1
    }
})JSON";
    out.close();

    LayerLibrary library;
    if (!library.reload(tempRoot.string())) {
        throw std::runtime_error("Failed to load HUD asset templates");
    }
    hub.setLayerLibrary(&library);

    hub.setHudPlacementProvider([]() {
        ControlMappingHubState::HudPlacementSnapshot snapshot;
        snapshot.id = "hud.controls";
        snapshot.bandLabel = "Console";
        snapshot.columnLabel = "Column 2";
        snapshot.columnIndex = 1;
        snapshot.visible = true;
        snapshot.target = "controller";
        return std::vector<ControlMappingHubState::HudPlacementSnapshot>{snapshot};
    });
    hub.setHudPlacementCallback([](const std::string&, int) {});

    MenuController controller;
    hub.onEnter(controller);
    hub.view();
    hub.onExit(controller);

    if (!hub.debugRowIsAsset("hud.controls")) {
        throw std::runtime_error("HUD asset row missing metadata");
    }

    const std::string summary = hub.debugValueForRow("hud.controls");
    if (summary.empty()) {
        throw std::runtime_error("HUD asset row not surfaced in CMH view");
    }
    if (summary.find("Band: Console") == std::string::npos) {
        throw std::runtime_error("HUD placement summary missing band label");
    }
    if (summary.find("Column: Column 2") == std::string::npos) {
        throw std::runtime_error("HUD placement summary missing column label");
    }
    if (summary.find("Route: Controller") == std::string::npos) {
        throw std::runtime_error("HUD placement summary missing route label");
    }

    std::error_code cleanupEc;
    std::filesystem::remove_all(tempRoot, cleanupEc);

    return true;
}

bool RunHudInlinePickerScenario() {
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "cmh_hud_inline_picker";
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path prefsPath = tempRoot / "hub_prefs.json";

    {
        const auto hudDir = tempRoot / "hud";
        std::filesystem::create_directories(hudDir);
        const auto hudAsset = hudDir / "controls.json";
        std::ofstream assetOut(hudAsset);
        assetOut << R"JSON({
    "id":"hud.controls",
    "label":"Control Hints",
    "category":"HUD",
    "type":"ui.hud.widget",
    "registryPrefix":"hud.controls",
    "hudWidget":{
        "module":"controls",
        "toggleId":"hud.controls",
        "defaultBand":"hud",
        "defaultColumn":1
    }
})JSON";
        assetOut.close();

        ControlMappingHubState hub;
        ParameterRegistry registry;
        MidiRouter router;
        hub.setParameterRegistry(&registry);
        hub.setMidiRouter(&router);
        hub.setPreferencesPath(prefsPath.string());

        LayerLibrary library;
        if (!library.reload(tempRoot.string())) {
            throw std::runtime_error("Failed to load HUD templates for inline picker scenario");
        }
        hub.setLayerLibrary(&library);

        ParameterRegistry::Descriptor hudMeta;
        hudMeta.label = "Control Hints";
        hudMeta.group = "HUD";
        hudMeta.description = "Toggle control hints widget";
        bool controlsVisible = false;
        registry.addBool("hud.controls", &controlsVisible, controlsVisible, hudMeta);

        bool toggleState = controlsVisible;
        int columnState = 0;
        std::vector<int> columnHistory;
        std::vector<bool> visibilityHistory;

        hub.setHudToggleCallback([&](const std::string& id, bool enabled) {
            if (id == "hud.controls") {
                toggleState = enabled;
                visibilityHistory.push_back(enabled);
            }
        });
        hub.setHudPlacementCallback([&](const std::string& id, int columnIndex) {
            if (id == "hud.controls") {
                columnState = columnIndex;
                columnHistory.push_back(columnIndex);
            }
        });
        hub.setHudPlacementProvider([&]() {
            ControlMappingHubState::HudPlacementSnapshot snapshot;
            snapshot.id = "hud.controls";
            snapshot.bandId = "hud";
            snapshot.bandLabel = "HUD";
            snapshot.columnIndex = columnState;
            snapshot.columnLabel = "Column " + std::to_string(columnState + 1);
            snapshot.visible = toggleState;
            snapshot.collapsed = false;
            return std::vector<ControlMappingHubState::HudPlacementSnapshot>{snapshot};
        });

        MenuController controller;
        hub.onEnter(controller);
        hub.view();

        if (!hub.debugSetHudColumnSelection("hud.controls", 3)) {
            throw std::runtime_error("Failed to assign HUD column selection");
        }
        if (columnHistory.empty() || columnHistory.back() != 2) {
            throw std::runtime_error("HUD column picker did not emit column index 2");
        }
        if (visibilityHistory.empty() || !visibilityHistory.back()) {
            throw std::runtime_error("HUD toggle callback not invoked for activation");
        }

        if (!hub.debugSetHudColumnSelection("hud.controls", 0)) {
            throw std::runtime_error("Failed to deactivate HUD widget via picker");
        }
        if (visibilityHistory.back()) {
            throw std::runtime_error("HUD picker failed to mark widget inactive");
        }
        const std::size_t columnEventCount = columnHistory.size();

        if (!hub.debugSetHudColumnSelection("hud.controls", 4)) {
            throw std::runtime_error("Failed to assign HUD column 4");
        }
        if (columnHistory.size() != columnEventCount + 1 || columnHistory.back() != 3) {
            throw std::runtime_error("HUD picker failed to emit column index 3");
        }

        const std::string summary = hub.debugValueForRow("hud.controls");
        if (summary.find("Column: Column 4") == std::string::npos) {
            throw std::runtime_error("HUD summary missing updated column label after picker change");
        }

        hub.debugFlushPreferences();
        hub.onExit(controller);
    }

    bool replayToggle = false;
    int replayColumn = -1;
    {
        ControlMappingHubState hub;
        ParameterRegistry registry;
        MidiRouter router;
        hub.setParameterRegistry(&registry);
        hub.setMidiRouter(&router);
        hub.setPreferencesPath(prefsPath.string());

        LayerLibrary library;
        if (!library.reload(tempRoot.string())) {
            throw std::runtime_error("Failed to reload HUD templates for inline picker replay");
        }
        hub.setLayerLibrary(&library);

        ParameterRegistry::Descriptor hudMeta;
        hudMeta.label = "Control Hints";
        hudMeta.group = "HUD";
        hudMeta.description = "Toggle control hints widget";
        registry.addBool("hud.controls", &replayToggle, replayToggle, hudMeta);

        hub.setHudToggleCallback([&](const std::string& id, bool enabled) {
            if (id == "hud.controls") {
                replayToggle = enabled;
            }
        });
        hub.setHudPlacementCallback([&](const std::string& id, int columnIndex) {
            if (id == "hud.controls") {
                replayColumn = columnIndex;
            }
        });
        hub.setHudPlacementProvider([&]() {
            ControlMappingHubState::HudPlacementSnapshot snapshot;
            snapshot.id = "hud.controls";
            snapshot.bandId = "hud";
            snapshot.bandLabel = "HUD";
            snapshot.columnIndex = replayColumn < 0 ? 0 : replayColumn;
            snapshot.columnLabel = "Column " + std::to_string(snapshot.columnIndex + 1);
            snapshot.visible = replayToggle;
            snapshot.collapsed = false;
            return std::vector<ControlMappingHubState::HudPlacementSnapshot>{snapshot};
        });

        MenuController controller;
        hub.onEnter(controller);
        hub.view();
        if (replayColumn != 3 || !replayToggle) {
            throw std::runtime_error("HUD state did not replay persisted picker selection");
        }
        const std::string replaySummary = hub.debugValueForRow("hud.controls");
        if (replaySummary.find("Column: Column 4") == std::string::npos) {
            throw std::runtime_error("HUD summary missing persisted column label after reload");
        }
        hub.onExit(controller);
    }

    std::error_code cleanupEc;
    std::filesystem::remove_all(tempRoot, cleanupEc);
    return true;
}

bool RunHudFeedTelemetryScenario() {
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "cmh_hud_feed";
    std::filesystem::create_directories(tempRoot);
    const auto prefsPath = tempRoot / "hub_prefs.json";
    const auto hudDir = tempRoot / "hud";
    std::filesystem::create_directories(hudDir);
    const auto hudAsset = hudDir / "controls.json";
    {
        std::ofstream assetOut(hudAsset);
        assetOut << R"JSON({
    "id":"hud.controls",
    "label":"Control Hints",
    "category":"HUD",
    "type":"ui.hud.widget",
    "registryPrefix":"hud.controls",
    "hudWidget":{
        "module":"controls",
        "toggleId":"hud.controls",
        "defaultBand":"hud",
        "defaultColumn":0
    }
})JSON";
    }

    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    HudFeedRegistry feedRegistry;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    hub.setPreferencesPath(prefsPath.string());
    hub.setHudFeedRegistry(&feedRegistry);

    LayerLibrary library;
    if (!library.reload(tempRoot.string())) {
        throw std::runtime_error("Failed to load HUD assets for feed telemetry scenario");
    }
    hub.setLayerLibrary(&library);

    ParameterRegistry::Descriptor hudMeta;
    hudMeta.label = "Control Hints";
    hudMeta.group = "HUD";
    hudMeta.description = "Toggle control hints widget";
    bool controlsVisible = false;
    registry.addBool("hud.controls", &controlsVisible, controlsVisible, hudMeta);

    int columnIndex = 0;
    hub.setHudToggleCallback([&](const std::string& id, bool enabled) {
        if (id == "hud.controls") {
            controlsVisible = enabled;
        }
    });
    hub.setHudPlacementCallback([&](const std::string& id, int column) {
        if (id == "hud.controls") {
            columnIndex = column;
        }
    });
    hub.setHudPlacementProvider([&]() {
        ControlMappingHubState::HudPlacementSnapshot snapshot;
        snapshot.id = "hud.controls";
        snapshot.bandId = "hud";
        snapshot.bandLabel = "HUD";
        snapshot.columnIndex = columnIndex;
        snapshot.columnLabel = "Column " + std::to_string(columnIndex + 1);
        snapshot.visible = controlsVisible;
        snapshot.collapsed = false;
        return std::vector<ControlMappingHubState::HudPlacementSnapshot>{snapshot};
    });

    std::vector<std::string> events;
    hub.setEventCallback([&](const std::string& payload) {
        events.push_back(payload);
    });

    MenuController controller;
    hub.onEnter(controller);
    hub.view();

    ofJson feedPayload = ofJson::object();
    ofJson slots = ofJson::object();
    slots["active"] = 1;
    slots["assigned"] = 2;
    slots["capacity"] = 8;
    feedPayload["slots"] = std::move(slots);
    ofJson fxRoutes = ofJson::object();
    fxRoutes["dither"] = { { "state", "Console" } };
    feedPayload["fxRoutes"] = std::move(fxRoutes);
    feedPayload["activeBank"] = "test";
    feedRegistry.publish("hud.controls", feedPayload);

    if (events.empty()) {
        throw std::runtime_error("HUD feed publish did not emit any events");
    }
    ofJson parsed = ofJson::parse(events.back());
    if (parsed.value("type", std::string()) != "hud.feed.updated") {
        throw std::runtime_error("Expected hud.feed.updated event");
    }
    if (parsed.value("widgetId", std::string()) != "hud.controls") {
        throw std::runtime_error("HUD feed event emitted incorrect widgetId");
    }

    events.clear();
    if (!hub.debugSetHudColumnSelection("hud.controls", 2)) {
        throw std::runtime_error("Failed to change HUD column during feed telemetry scenario");
    }
    bool mappingEventSeen = false;
    for (const auto& evt : events) {
        if (evt.find("\"type\":\"hud.mapping.changed\"") != std::string::npos) {
            mappingEventSeen = true;
            break;
        }
    }
    if (!mappingEventSeen) {
        throw std::runtime_error("HUD mapping change did not emit telemetry");
    }

    hub.onExit(controller);
    std::error_code cleanupEc;
    std::filesystem::remove_all(tempRoot, cleanupEc);
    return true;
}

bool RunHudRoutingManifestScenario() {
    ControlMappingHubState hub;
    std::vector<std::string> events;
    hub.setEventCallback([&](const std::string& payload) {
        events.push_back(payload);
    });
    hub.setHudPlacementProvider([]() {
        ControlMappingHubState::HudPlacementSnapshot snapshot;
        snapshot.id = "hud.controls";
        snapshot.bandId = "hud";
        snapshot.bandLabel = "HUD";
        snapshot.columnIndex = 1;
        snapshot.columnLabel = "Column 2";
        snapshot.visible = true;
        snapshot.collapsed = false;
        snapshot.target = "controller";
        return std::vector<ControlMappingHubState::HudPlacementSnapshot>{snapshot};
    });
    auto snapshot = hub.exportHudLayoutSnapshot(ControlMappingHubState::HudLayoutTarget::Projector);
    if (snapshot.empty()) {
        throw std::runtime_error("HUD layout snapshot export returned no widgets");
    }
    if (snapshot.front().target != "controller") {
        throw std::runtime_error("HUD layout snapshot did not retain widget target metadata");
    }
    events.clear();
    hub.emitHudLayoutSnapshot(ControlMappingHubState::HudLayoutTarget::Projector, snapshot, "unittest");
    bool snapshotEventSeen = false;
    for (const auto& evt : events) {
        if (evt.find("\"type\":\"hud.layout.snapshot\"") == std::string::npos) {
            continue;
        }
        snapshotEventSeen = evt.find("\"target\":\"controller\"") != std::string::npos;
        if (snapshotEventSeen) {
            break;
        }
    }
    if (!snapshotEventSeen) {
        throw std::runtime_error("HUD layout snapshot event missing target metadata");
    }
    events.clear();
    ControlMappingHubState::HudRoutingEntry entry;
    entry.id = "hud.controls";
    entry.label = "Controls";
    entry.category = "HUD";
    entry.target = "controller";
    hub.emitHudRoutingManifest({entry});
    bool manifestEventSeen = false;
    for (const auto& evt : events) {
        if (evt.find("\"type\":\"overlay.routing.manifest\"") == std::string::npos) {
            continue;
        }
        manifestEventSeen = evt.find("\"target\":\"controller\"") != std::string::npos;
        if (manifestEventSeen) {
            break;
        }
    }
    if (!manifestEventSeen) {
        throw std::runtime_error("Overlay routing manifest event missing target metadata");
    }
    return true;
}

bool RunDualScreenPhase2Scenario() {
    ControlMappingHubState hub;
    std::unordered_map<std::string, int> routeEvents;
    bool eventParseFailed = false;
    std::string eventParseError;
    hub.setEventCallback([&](const std::string& payload) {
        if (eventParseFailed) {
            return;
        }
        try {
            const auto event = ofJson::parse(payload);
            if (event.value("type", "") == "overlay.route.changed") {
                const std::string target = event.value("target", "");
                if (!target.empty()) {
                    routeEvents[target]++;
                }
            }
        } catch (const std::exception& ex) {
            eventParseFailed = true;
            eventParseError = ex.what();
        }
    });
    ControlMappingHubState::HudPlacementSnapshot projectorPlacement;
    projectorPlacement.id = "hud.controls";
    projectorPlacement.bandId = "hud";
    projectorPlacement.bandLabel = "HUD";
    projectorPlacement.columnIndex = 0;
    projectorPlacement.columnLabel = "Column 1";
    projectorPlacement.visible = true;
    projectorPlacement.collapsed = false;
    projectorPlacement.target = "projector";

    ControlMappingHubState::HudPlacementSnapshot controllerPlacement = projectorPlacement;
    controllerPlacement.columnIndex = 3;
    controllerPlacement.columnLabel = "Column 4";
    controllerPlacement.target = "controller";

    hub.setHudPlacementCallback([](const std::string&, int) {});
    hub.setHudPlacementProvider([&]() {
        if (hub.hudLayoutTarget() == ControlMappingHubState::HudLayoutTarget::Controller) {
            return std::vector<ControlMappingHubState::HudPlacementSnapshot>{controllerPlacement};
        }
        return std::vector<ControlMappingHubState::HudPlacementSnapshot>{projectorPlacement};
    });

    auto captureSnapshot = [&](ControlMappingHubState::HudLayoutTarget target)
        -> ControlMappingHubState::HudPlacementSnapshot {
        if (hub.hudLayoutTarget() != target) {
            hub.setHudLayoutTarget(target);
        }
        hub.notifyHudLayoutChanged();
        hub.emitOverlayRouteEvent(ControlMappingHubState::hudLayoutTargetName(target),
                                  target == ControlMappingHubState::HudLayoutTarget::Projector
                                      ? "test.projector"
                                      : "test.controller",
                                  target == ControlMappingHubState::HudLayoutTarget::Projector);
        auto snapshot = hub.exportHudLayoutSnapshot(target);
        if (snapshot.empty()) {
            throw std::runtime_error("HUD snapshot capture returned no widgets");
        }
        return snapshot.front();
    };

    const int shuttleIterations = 200;
    int projectorCaptures = 0;
    int controllerCaptures = 0;
    ControlMappingHubState::HudPlacementSnapshot lastProjectorSnapshot;
    ControlMappingHubState::HudPlacementSnapshot lastControllerSnapshot;

    for (int i = 0; i < shuttleIterations; ++i) {
        ControlMappingHubState::HudLayoutTarget target =
            (i % 2 == 0) ? ControlMappingHubState::HudLayoutTarget::Controller
                         : ControlMappingHubState::HudLayoutTarget::Projector;
        auto snapshot = captureSnapshot(target);
        if (target == ControlMappingHubState::HudLayoutTarget::Projector) {
            ++projectorCaptures;
            lastProjectorSnapshot = snapshot;
            if (snapshot.columnIndex != projectorPlacement.columnIndex) {
                throw std::runtime_error("Projector snapshot column mismatch during follow cycle");
            }
            if (snapshot.target != "projector") {
                throw std::runtime_error("Projector snapshot missing correct route label");
            }
        } else {
            ++controllerCaptures;
            lastControllerSnapshot = snapshot;
            if (snapshot.columnIndex != controllerPlacement.columnIndex) {
                throw std::runtime_error("Controller snapshot column mismatch during freeform cycle");
            }
            if (snapshot.target != "controller") {
                throw std::runtime_error("Controller snapshot missing correct route label");
            }
        }
    }

    if (projectorCaptures == 0 || controllerCaptures == 0) {
        throw std::runtime_error("Follow/freeform cycle did not capture both layout targets");
    }
    if (eventParseFailed) {
        throw std::runtime_error("Overlay route event parsing failed: " + eventParseError);
    }

    const auto routeCaptureCount = [&](const std::string& key) -> int {
        const auto it = routeEvents.find(key);
        return it == routeEvents.end() ? 0 : it->second;
    };
    const int projectorRouteEvents = routeCaptureCount("projector");
    const int controllerRouteEvents = routeCaptureCount("controller");
    if (projectorRouteEvents == 0 || controllerRouteEvents == 0) {
        throw std::runtime_error("Overlay route telemetry missing controller/projector coverage");
    }

    const std::filesystem::path artifactPath = "tests/artifacts/dual_screen_phase2.json";
    std::filesystem::create_directories(artifactPath.parent_path());
    ofJson artifact = ofJson::object();
    artifact["scenario"] = "dual_screen_phase2";
    artifact["iterations"] = shuttleIterations;
    artifact["widgetId"] = projectorPlacement.id;
    artifact["lastTarget"] = ControlMappingHubState::hudLayoutTargetName(hub.hudLayoutTarget());
    ofJson projectorNode = ofJson::object();
    projectorNode["target"] = lastProjectorSnapshot.target;
    projectorNode["column"] = lastProjectorSnapshot.columnIndex;
    projectorNode["captures"] = projectorCaptures;
    artifact["projector"] = std::move(projectorNode);
    ofJson controllerNode = ofJson::object();
    controllerNode["target"] = lastControllerSnapshot.target;
    controllerNode["column"] = lastControllerSnapshot.columnIndex;
    controllerNode["captures"] = controllerCaptures;
    artifact["controller"] = std::move(controllerNode);
    ofJson routeNode = ofJson::object();
    routeNode["projector"] = projectorRouteEvents;
    routeNode["controller"] = controllerRouteEvents;
    artifact["routeEvents"] = std::move(routeNode);

    std::ofstream out(artifactPath, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to write dual-screen Phase 2 artifact");
    }
    out << std::setw(2) << artifact << "\n";
    return true;
}

}  // namespace browser_flow

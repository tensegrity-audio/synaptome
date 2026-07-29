#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <cctype>
#include <cstring>
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
#include "../synaptome/src/core/PackageControlTransactions.h"
#include "../synaptome/src/runtime/BuiltinElementHostBindings.h"
#include "../synaptome/src/visuals/TextLayerState.h"
#include "../synaptome/src/ui/MenuController.h"
#include "../synaptome/src/ui/MenuController.cpp"
#include "../synaptome/src/ui/HotkeyManager.cpp"
#define private public
#define protected public
#include "../synaptome/src/ui/ControlMappingHubState.h"
#undef private
#undef protected
#ifndef OF_SDK_AVAILABLE
inline constexpr int OF_SERIAL_ERROR = -1;
inline constexpr int OF_SERIAL_NO_DATA = -2;
class ofSerial {
public:
    int readByte() { return OF_SERIAL_NO_DATA; }
};
#endif
#define private public
#include "../synaptome/src/io/SerialSlipOsc.h"
#undef private
#include "../synaptome/src/ui/ColumnControls.h"
#include "../synaptome/src/ui/ColumnControls.cpp"
#include "../synaptome/src/ui/DevicesPanel.h"
#include "../synaptome/src/ui/DevicesPanel.cpp"
#include "../synaptome/src/ui/WindowMonitorPlacement.h"
#include "../synaptome/src/io/MidiRouter.h"
#include "../synaptome/src/io/MachineProfileDocument.cpp"
#include "../synaptome/src/io/MappingBankDocument.cpp"
#include "../synaptome/src/io/PreferencesDocument.cpp"
#include "../synaptome/src/io/BankDefinitionsDocument.cpp"
#include "../synaptome/src/io/MidiRouter.cpp"
#include "../synaptome/src/io/OscIngressMessage.h"
#include "../synaptome/src/io/SceneStateDocument.cpp"
#include "../synaptome/src/visuals/LayerLibrary.cpp"

// The BrowserFlow harness uses deliberately small openFrameworks stubs. These
// adapters are sufficient to instantiate the real CircuitTraceLayer and run
// its production parameter registration without requiring a GL context.
namespace glm {
inline constexpr ivec2 operator+(ivec2 left, ivec2 right) {
    return { left.x + right.x, left.y + right.y };
}
inline constexpr ivec2 operator-(ivec2 left, ivec2 right) {
    return { left.x - right.x, left.y - right.y };
}
inline constexpr ivec2 operator*(ivec2 value, int scalar) {
    return { value.x * scalar, value.y * scalar };
}
inline constexpr ivec2 operator/(ivec2 value, int scalar) {
    return { value.x / scalar, value.y / scalar };
}
}

struct ofFloatColor {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;

    ofFloatColor() = default;
    ofFloatColor(float red, float green, float blue, float alpha)
        : r(red), g(green), b(blue), a(alpha) {}
};

class ofFloatPixels : public ofPixels {
public:
    void allocate(int width, int height, int channels) {
        ofPixels::allocate(
            width, height,
            channels == 4 ? OF_PIXELS_RGBA : OF_PIXELS_RGB);
    }
    bool isAllocated() const { return size() > 0; }
    void setColor(int, int, const ofFloatColor&) {}
};

inline constexpr int GL_RGBA32F = 0;
inline constexpr int GL_NEAREST = 0;

#include "../synaptome/src/visuals/CircuitTraceLayer.cpp"
#include "../synaptome/src/visuals/LeniaLayer.cpp"
#include "../synaptome/src/ui/AssetBrowser.cpp"
#include "../synaptome/src/ui/ConsoleState.h"
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

struct CapturingTelemetrySink final
    : synaptome::element::TelemetrySink {
    void add(
        synaptome::element::TelemetryEntry entry) override {
        entries.push_back(std::move(entry));
    }

    const synaptome::element::TelemetryEntry* find(
        const std::string& id) const {
        for (const auto& entry : entries) {
            if (entry.id == id) {
                return &entry;
            }
        }
        return nullptr;
    }

    std::vector<synaptome::element::TelemetryEntry> entries;
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
        toggle.label = "Hotkey Guide";
        toggle.description = "Show keyboard and mouse hint banner";
        toggle.defaultValue = false;
        toggle.valuePtr = &hudShowControls;
        if (!hud.registerToggle(toggle)) {
            throw std::runtime_error("Failed to register HUD toggle");
        }

        OverlayWidget::Metadata meta;
        meta.id = "hud.controls";
        meta.label = "Hotkey Guide";
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

bool RunMidiNamespaceCleanupScenario() {
    MidiRouter router;
    float layer1Value = 0.5f;
    router.bindFloat(
        "console.layer1.opacity",
        &layer1Value,
        0.0f,
        1.0f,
        false,
        0.0f);
    router.setOrUpdateCc("console.layer1.opacity", 11);
    router.setOrUpdateCc("console.layer10.opacity", 12);
    router.setOrUpdateCc("console.layer1extra.opacity", 13);

    router.unbindTargetsByPrefix("console.layer1");
    if (router.getCcMaps().size() != 3) {
        throw std::runtime_error(
            "target-only MIDI cleanup removed persisted mappings");
    }

    router.unbindByPrefix("console.layer1");

    const auto& mappings = router.getCcMaps();
    auto hasTarget = [&](const std::string& target) {
        return std::any_of(
            mappings.begin(),
            mappings.end(),
            [&](const MidiRouter::CcMap& mapping) {
                return mapping.target == target;
            });
    };
    if (hasTarget("console.layer1.opacity")) {
        throw std::runtime_error("MIDI namespace cleanup retained the target layer");
    }
    if (!hasTarget("console.layer10.opacity")) {
        throw std::runtime_error("MIDI namespace cleanup removed layer10");
    }
    if (!hasTarget("console.layer1extra.opacity")) {
        throw std::runtime_error("MIDI namespace cleanup removed a textual sibling");
    }
    return true;
}

bool RunParameterRegistryStorageInvalidationScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) throw std::runtime_error(message);
    };
    ParameterRegistry registry;
    float originalValue = 0.25f;
    ParameterRegistry::Descriptor descriptor;
    descriptor.label = "Unrelated Runtime Value";
    registry.addFloat(
        "runtime.unrelated",
        &originalValue,
        originalValue,
        descriptor);

    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.rebuildModel();
    const auto originalRow = std::find_if(
        hub.tableModel_.rows.begin(),
        hub.tableModel_.rows.end(),
        [](const ControlMappingHubState::ParameterRow& row) {
            return row.id == "runtime.unrelated";
        });
    require(originalRow != hub.tableModel_.rows.end(),
            "prebuilt model omitted the unrelated registry row");
    require(originalRow->floatParam &&
                originalRow->floatParam->value == &originalValue,
            "prebuilt model did not cache the original registry entry");

    hub.selectedRow_ = static_cast<int>(
        std::distance(hub.tableModel_.rows.begin(), originalRow));
    hub.activeRowSet_ = &hub.tableModel_.allRowIndices;
    hub.routingPopoverVisible_ = true;

    ParameterRegistry replacement;
    float replacementValue = 0.75f;
    replacement.addFloat(
        "runtime.unrelated",
        &replacementValue,
        replacementValue,
        descriptor);
    registry.swap(replacement);
    hub.invalidateParameterRegistryStorage();

    require(hub.tableModel_.rows.empty(),
            "registry invalidation retained pointer-bearing rows");
    require(hub.activeRowSet_ == nullptr && hub.selectedRow_ == -1,
            "registry invalidation retained a pointer-bearing selection");
    require(!hub.routingPopoverVisible_,
            "registry invalidation retained a row-backed popover");
    require(hub.tableModel_.dirty,
            "registry invalidation did not request a safe rebuild");

    hub.rebuildModel();
    const auto rebuiltRow = std::find_if(
        hub.tableModel_.rows.begin(),
        hub.tableModel_.rows.end(),
        [](const ControlMappingHubState::ParameterRow& row) {
            return row.id == "runtime.unrelated";
        });
    require(rebuiltRow != hub.tableModel_.rows.end(),
            "model did not rebuild the unrelated registry row");
    require(rebuiltRow->floatParam &&
                rebuiltRow->floatParam->value == &replacementValue,
            "rebuilt model retained the replaced registry entry");
    require(
        hub.setRowFloatValue(*rebuiltRow, 0.5f) &&
            std::fabs(replacementValue - 0.5f) < 0.0001f &&
            std::fabs(originalValue - 0.25f) < 0.0001f,
        "rebuilt row did not edit only the live registry storage");
    return true;
}

bool RunBuiltinElementHostParametersWithoutTextElementScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) throw std::runtime_error(message);
    };
    auto nearlyEqual = [](float left, float right) {
        return std::fabs(left - right) < 0.0001f;
    };

    constexpr const char* stringIds[] = {
        "overlay.text.content",
        "overlay.text.topLeft",
        "overlay.text.topRight",
        "overlay.text.bottomLeft",
        "overlay.text.bottomRight",
        "overlay.text.font",
    };
    constexpr const char* floatIds[] = {
        "overlay.text.fontIndex",
        "overlay.text.size",
        "overlay.text.corner.size",
        "overlay.text.color.r",
        "overlay.text.color.g",
        "overlay.text.color.b",
    };

    auto& textState = TextLayerState::instance();
    struct ScopedTextStateRestore {
        TextLayerState& state;
        std::string content;
        std::string font;
        float fontIndex = 0.0f;

        ~ScopedTextStateRestore() noexcept {
            try {
                state.content = content;
                state.font = font;
                state.fontIndex = fontIndex;
                state.syncFontSelection();
                state.content = content;
                state.font = font;
                state.fontIndex = fontIndex;
            } catch (...) {
                // Test cleanup must not mask the original scenario failure.
            }
        }
    } restore{
        textState,
        textState.content,
        textState.font,
        textState.fontIndex,
    };
    textState.refreshAvailableFonts();

    ParameterRegistry registry;
    synaptome::runtime::registerBuiltinElementHostParameters(registry);
    require(
        registry.strings().size() == std::size(stringIds) &&
            registry.floats().size() == std::size(floatIds) &&
            registry.bools().empty(),
        "zero-text host binding did not register exactly 6 strings and 6 floats");

    for (const char* id : stringIds) {
        const auto* parameter = registry.findString(id);
        require(
            parameter && parameter->value && parameter->meta.group == "Overlay",
            std::string("zero-text host binding omitted string metadata: ") + id);
    }
    for (const char* id : floatIds) {
        const auto* parameter = registry.findFloat(id);
        require(
            parameter && parameter->value && parameter->meta.group == "Overlay",
            std::string("zero-text host binding omitted float metadata: ") + id);
    }
    struct ExpectedMetadata {
        const char* id;
        const char* label;
        const char* description;
    };
    constexpr ExpectedMetadata expectedStringMetadata[] = {
        {"overlay.text.content", "Motion: Center Text",
         "Text displayed in the center text slot"},
        {"overlay.text.topLeft", "Motion: Top Left Text",
         "Text displayed in the top-left text slot"},
        {"overlay.text.topRight", "Motion: Top Right Text",
         "Text displayed in the top-right text slot"},
        {"overlay.text.bottomLeft", "Motion: Bottom Left Text",
         "Text displayed in the bottom-left text slot"},
        {"overlay.text.bottomRight", "Motion: Bottom Right Text",
         "Text displayed in the bottom-right text slot"},
        {"overlay.text.font", "Motion: Font File",
         "TrueType font filename under data/fonts"},
    };
    constexpr ExpectedMetadata expectedFloatMetadata[] = {
        {"overlay.text.fontIndex", "Motion: Font Index",
         "Select discovered font by index"},
        {"overlay.text.size", "Scale: Center Text Size",
         "Center text font size in pixels"},
        {"overlay.text.corner.size", "Scale: Corner Text Size",
         "Corner text font size in pixels"},
        {"overlay.text.color.r", "Color: Text Color R", ""},
        {"overlay.text.color.g", "Color: Text Color G", ""},
        {"overlay.text.color.b", "Color: Text Color B", ""},
    };
    for (const auto& expected : expectedStringMetadata) {
        const auto* parameter = registry.findString(expected.id);
        require(
            parameter &&
                parameter->meta.label == expected.label &&
                parameter->meta.description == expected.description,
            std::string("zero-text host binding changed string presentation metadata: ") +
                expected.id);
    }
    for (const auto& expected : expectedFloatMetadata) {
        const auto* parameter = registry.findFloat(expected.id);
        require(
            parameter &&
                parameter->meta.label == expected.label &&
                parameter->meta.description == expected.description,
            std::string("zero-text host binding changed float presentation metadata: ") +
                expected.id);
    }

    const auto requireRange =
        [&](const char* id,
            float min,
            float max,
            float step,
            const std::string& units = std::string()) {
            const auto* parameter = registry.findFloat(id);
            require(
                parameter &&
                    nearlyEqual(parameter->meta.range.min, min) &&
                    nearlyEqual(parameter->meta.range.max, max) &&
                    nearlyEqual(parameter->meta.range.step, step) &&
                    parameter->meta.units == units,
                std::string("zero-text host binding changed range metadata: ") +
                    id);
        };
    const auto* fontIndex = registry.findFloat("overlay.text.fontIndex");
    require(fontIndex, "zero-text host binding omitted font index");
    requireRange(
        "overlay.text.fontIndex",
        0.0f,
        fontIndex->meta.range.max,
        1.0f);
    require(fontIndex->meta.range.max >= 0.0f,
            "zero-text font index exposed a negative maximum");
    requireRange("overlay.text.size", 12.0f, 256.0f, 1.0f, "px");
    requireRange("overlay.text.corner.size", 8.0f, 256.0f, 1.0f, "px");
    requireRange("overlay.text.color.r", 0.0f, 255.0f, 1.0f);
    requireRange("overlay.text.color.g", 0.0f, 255.0f, 1.0f);
    requireRange("overlay.text.color.b", 0.0f, 255.0f, 1.0f);

    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.setConsoleSlotInventoryCallback(
        [] { return std::vector<ConsoleLayerInfo>{}; });
    hub.rebuildModel();

    const auto findRow = [&](const std::string& id) {
        return std::find_if(
            hub.tableModel_.rows.begin(),
            hub.tableModel_.rows.end(),
            [&](const ControlMappingHubState::ParameterRow& row) {
                return row.id == id;
            });
    };
    for (const char* id : stringIds) {
        require(
            findRow(id) != hub.tableModel_.rows.end(),
            std::string("Browser omitted zero-text string row: ") + id);
    }
    for (const char* id : floatIds) {
        require(
            findRow(id) != hub.tableModel_.rows.end(),
            std::string("Browser omitted zero-text float row: ") + id);
    }

    const auto contentRow = findRow("overlay.text.content");
    require(
        contentRow != hub.tableModel_.rows.end() &&
            hub.setRowStringValue(*contentRow, "Browser without TextLayer") &&
            registry.getStringBase("overlay.text.content") ==
                "Browser without TextLayer" &&
            registry.findString("overlay.text.content")->valueForPersistence() ==
                "Browser without TextLayer",
        "Browser could not edit live and base text content without a text element");

    fontIndex = registry.findFloat("overlay.text.fontIndex");
    *fontIndex->value = fontIndex->meta.range.max + 10.0f;
    synaptome::runtime::updateBuiltinElementHostParameters();
    require(
        *fontIndex->value >= fontIndex->meta.range.min &&
            *fontIndex->value <= fontIndex->meta.range.max &&
            nearlyEqual(*fontIndex->value, std::round(*fontIndex->value)),
        "zero-text font synchronization did not clamp and snap font index");

    bool duplicateRejected = false;
    try {
        synaptome::runtime::registerBuiltinElementHostParameters(registry);
    } catch (const std::logic_error&) {
        duplicateRejected = true;
    }
    require(
        duplicateRejected,
        "built-in host binding did not reject duplicate registration");
    return true;
}

bool RunTextLayerStateTransactionScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) throw std::runtime_error(message);
    };
    auto same = [](const TextLayerState::Snapshot& left,
                   const TextLayerState::Snapshot& right) {
        return left.content == right.content &&
               left.topLeft == right.topLeft &&
               left.topRight == right.topRight &&
               left.bottomLeft == right.bottomLeft &&
               left.bottomRight == right.bottomRight &&
               left.font == right.font &&
               left.fontIndex == right.fontIndex &&
               left.fontSize == right.fontSize &&
               left.cornerFontSize == right.cornerFontSize &&
               left.colorR == right.colorR &&
               left.colorG == right.colorG &&
               left.colorB == right.colorB;
    };

    auto& live = TextLayerState::instance();
    struct ScopedSnapshotRestore {
        TextLayerState& state;
        TextLayerState::Snapshot saved;
        ~ScopedSnapshotRestore() noexcept {
            state.adoptSnapshot(std::move(saved));
        }
    } restore{live, live.snapshot()};

    const auto baseline = live.snapshot();
    {
        auto abandoned = live.snapshot();
        abandoned.content = "prepared but abandoned";
        abandoned.topLeft = "must not leak";
        abandoned.fontSize = 201.0f;
        abandoned.colorR = 91.0f;
    }
    require(
        same(live.snapshot(), baseline),
        "abandoned Text configuration changed shared live state");

    auto prepared = live.snapshot();
    prepared.content = "adopted text";
    prepared.topLeft = "committed";
    prepared.topRight.clear();
    prepared.bottomLeft = "left";
    prepared.bottomRight = "right";
    prepared.fontSize = 72.0f;
    prepared.cornerFontSize = 24.0f;
    prepared.colorR = 12.0f;
    prepared.colorG = 34.0f;
    prepared.colorB = 56.0f;
    const auto expected = prepared;

    require(
        same(live.snapshot(), baseline),
        "prepared Text configuration published before adoption");
    live.adoptSnapshot(std::move(prepared));
    require(
        same(live.snapshot(), expected),
        "Text adoption did not atomically publish the staged values");
    return true;
}

bool RunElementParameterRegistryContractScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) throw std::runtime_error(message);
    };
    auto nearlyEqual = [](float left, float right) {
        return std::fabs(left - right) < 0.0001f;
    };

    ParameterRegistry registry;
    float liveScale = 2.0f;
    bool liveEnabled = true;

    ParameterRegistry::Descriptor scaleDescriptor;
    scaleDescriptor.label = "Element Scale";
    scaleDescriptor.group = "Tests";
    scaleDescriptor.range.min = 0.1f;
    scaleDescriptor.range.max = 10.0f;
    scaleDescriptor.range.step = 0.1f;
    registry.addFloat(
        "console.layer1.scale",
        &liveScale,
        liveScale,
        scaleDescriptor);

    ParameterRegistry::Descriptor enabledDescriptor;
    enabledDescriptor.label = "Element Enabled";
    enabledDescriptor.group = "Tests";
    registry.addBool(
        "console.layer1.enabled",
        &liveEnabled,
        liveEnabled,
        enabledDescriptor);

    modifier::Modifier scaleMod;
    scaleMod.type = modifier::Type::kAutomation;
    scaleMod.blend = modifier::BlendMode::kAbsolute;
    scaleMod.inputRange = {0.0f, 1.0f, false};
    scaleMod.outputRange = {0.0f, 1.0f, false};
    auto& scaleRuntimeModifier =
        registry.addFloatModifier("console.layer1.scale", scaleMod);
    scaleRuntimeModifier.ownerTag = "tests.automation.scale";
    registry.setFloatModifierInput(
        "console.layer1.scale",
        0,
        0.75f,
        true);

    modifier::Modifier enabledMod;
    enabledMod.type = modifier::Type::kAutomation;
    enabledMod.blend = modifier::BlendMode::kToggle;
    enabledMod.inputRange = {0.0f, 1.0f, false};
    enabledMod.outputRange = {0.0f, 1.0f, false};
    auto& enabledRuntimeModifier =
        registry.addBoolModifier("console.layer1.enabled", enabledMod);
    enabledRuntimeModifier.ownerTag = "tests.automation.enabled";
    registry.setBoolModifierInput(
        "console.layer1.enabled",
        0,
        0.0f,
        true);
    registry.evaluateAllModifiers();

    const auto* scale = registry.findFloat("console.layer1.scale");
    const auto* enabled = registry.findBool("console.layer1.enabled");
    require(
        scale &&
            scale->value == &liveScale &&
            nearlyEqual(scale->baseValue, 2.0f) &&
            nearlyEqual(*scale->value, 0.75f),
        "typed float lookup did not preserve distinct base and live values");
    require(
        enabled &&
            enabled->value == &liveEnabled &&
            enabled->baseValue &&
            !*enabled->value,
        "typed bool lookup did not preserve distinct base and live values");
    const auto initialSnapshots = registry.snapshotValues();
    const auto scaleSnapshot = std::find_if(
        initialSnapshots.begin(),
        initialSnapshots.end(),
        [](const auto& snapshot) {
            return snapshot.id == "console.layer1.scale";
        });
    require(
        scaleSnapshot != initialSnapshots.end() &&
            scaleSnapshot->baseOrigin.kind ==
                synaptome::state::ParameterBaseOriginKind::
                    ElementDefault &&
            scaleSnapshot->modifiers.size() == 1 &&
            scaleSnapshot->modifiers.front().ownerTag ==
                "tests.automation.scale",
        "read-only parameter snapshot lost default or modifier origin");

    const ParameterRegistry::Range copiedRange = scale->meta.range;
    require(
        nearlyEqual(copiedRange.min, 0.1f) &&
            nearlyEqual(copiedRange.max, 10.0f) &&
            nearlyEqual(copiedRange.step, 0.1f),
        "float lookup did not expose complete range metadata");

    const synaptome::state::ParameterBaseOrigin operatorOrigin{
        synaptome::state::ParameterBaseOriginKind::OperatorEdit,
        "tests.operator",
    };
    registry.setFloatBase(
        "console.layer1.scale",
        3.25f,
        operatorOrigin,
        true);
    registry.setBoolBase(
        "console.layer1.enabled",
        false,
        operatorOrigin,
        true);
    require(
        nearlyEqual(registry.getFloatBase("console.layer1.scale"), 3.25f) &&
            nearlyEqual(liveScale, 3.25f) &&
            !registry.getBoolBase("console.layer1.enabled") &&
            !liveEnabled &&
            registry.findFloat("console.layer1.scale")
                    ->baseOrigin.kind ==
                synaptome::state::ParameterBaseOriginKind::
                    OperatorEdit &&
            registry.findBool("console.layer1.enabled")
                    ->baseOrigin.kind ==
                synaptome::state::ParameterBaseOriginKind::
                    OperatorEdit,
        "base-plus-live writes did not update both registry and live storage");

    ParameterRegistry replacement;
    float replacementScale = 4.0f;
    ParameterRegistry::Descriptor replacementDescriptor = scaleDescriptor;
    replacementDescriptor.range.min = 0.0f;
    replacementDescriptor.range.max = 5.0f;
    replacementDescriptor.range.step = 0.5f;
    replacement.addFloat(
        "console.layer1.scale",
        &replacementScale,
        replacementScale,
        replacementDescriptor);
    registry.swap(replacement);

    const auto* replacementParam =
        registry.findFloat("console.layer1.scale");
    require(
        replacementParam &&
            replacementParam->value == &replacementScale &&
            nearlyEqual(replacementParam->meta.range.min, 0.0f) &&
            nearlyEqual(replacementParam->meta.range.max, 5.0f) &&
            nearlyEqual(replacementParam->meta.range.step, 0.5f),
        "registry replacement did not publish the replacement storage and range");
    require(
        nearlyEqual(copiedRange.min, 0.1f) &&
            nearlyEqual(copiedRange.max, 10.0f) &&
            nearlyEqual(copiedRange.step, 0.1f),
        "by-value range metadata changed after registry storage replacement");
    require(
        registry.findBool("console.layer1.enabled") == nullptr,
        "registry replacement retained a removed bool parameter");
    return true;
}

bool RunElementParameterMidiRebindScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) throw std::runtime_error(message);
    };
    auto nearlyEqual = [](float left, float right) {
        return std::fabs(left - right) < 0.0001f;
    };

    constexpr const char* kPrefix = "console.layer1";
    constexpr const char* kContinuousId = "console.layer1.scale";
    constexpr const char* kSteppedId = "console.layer1.octaves";
    constexpr int kContinuousCc = 21;
    constexpr int kSteppedCc = 22;

    ParameterRegistry::Range continuousRange;
    continuousRange.min = 0.1f;
    continuousRange.max = 10.0f;
    continuousRange.step = 0.1f;
    ParameterRegistry::Range steppedRange;
    steppedRange.min = 1.0f;
    steppedRange.max = 8.0f;
    steppedRange.step = 1.0f;

    float retiredContinuous = 2.0f;
    float retiredStepped = 3.0f;
    MidiRouter router;
    router.bindFloat(
        kContinuousId,
        &retiredContinuous,
        continuousRange.min,
        continuousRange.max,
        false,
        0.0f);
    router.bindFloat(
        kSteppedId,
        &retiredStepped,
        steppedRange.min,
        steppedRange.max,
        true,
        steppedRange.step);
    router.setOrUpdateCc(kContinuousId, kContinuousCc);
    router.setOrUpdateCc(kSteppedId, kSteppedCc);

    auto mappingFor = [&](const std::string& target)
        -> const MidiRouter::CcMap* {
        const auto& mappings = router.getCcMaps();
        const auto found = std::find_if(
            mappings.begin(),
            mappings.end(),
            [&](const MidiRouter::CcMap& mapping) {
                return mapping.target == target;
            });
        return found != mappings.end() ? &*found : nullptr;
    };

    const auto* continuousMapping = mappingFor(kContinuousId);
    const auto* steppedMapping = mappingFor(kSteppedId);
    require(
        continuousMapping &&
            nearlyEqual(continuousMapping->outMin, continuousRange.min) &&
            nearlyEqual(continuousMapping->outMax, continuousRange.max) &&
            !continuousMapping->snapInt &&
            nearlyEqual(continuousMapping->step, 0.0f),
        "continuous MIDI binding adopted descriptor quantization");
    require(
        steppedMapping &&
            nearlyEqual(steppedMapping->outMin, steppedRange.min) &&
            nearlyEqual(steppedMapping->outMax, steppedRange.max) &&
            steppedMapping->snapInt &&
            nearlyEqual(steppedMapping->step, steppedRange.step),
        "stepped MIDI binding lost its explicit snap/step policy");

    ofxMidiMessage continuousMessage;
    continuousMessage.status = MIDI_CONTROL_CHANGE;
    continuousMessage.control = kContinuousCc;
    continuousMessage.channel = 0;
    continuousMessage.value = 64;
    router.newMidiMessage(continuousMessage);
    const float expectedContinuous =
        continuousRange.min +
        (continuousRange.max - continuousRange.min) *
            (static_cast<float>(continuousMessage.value) / 127.0f);
    require(
        std::fabs(retiredContinuous - expectedContinuous) < 0.001f,
        "continuous MIDI target was quantized by descriptor step");

    ofxMidiMessage steppedMessage;
    steppedMessage.status = MIDI_CONTROL_CHANGE;
    steppedMessage.control = kSteppedCc;
    steppedMessage.channel = 0;
    steppedMessage.value = 64;
    router.newMidiMessage(steppedMessage);
    const float expectedStepped = std::round(
        steppedRange.min +
        (steppedRange.max - steppedRange.min) *
            (static_cast<float>(steppedMessage.value) / 127.0f));
    require(
        nearlyEqual(retiredStepped, expectedStepped),
        "stepped MIDI target did not honor integer snapping");

    router.unbindTargetsByPrefix(kPrefix);
    const float retiredContinuousAtUnbind = retiredContinuous;
    const float retiredSteppedAtUnbind = retiredStepped;
    continuousMessage.value = 100;
    steppedMessage.value = 100;
    router.newMidiMessage(continuousMessage);
    router.newMidiMessage(steppedMessage);
    require(
        nearlyEqual(retiredContinuous, retiredContinuousAtUnbind) &&
            nearlyEqual(retiredStepped, retiredSteppedAtUnbind),
        "retired element storage received MIDI after target invalidation");

    float replacementContinuous = 1.0f;
    float replacementStepped = 1.0f;
    router.bindFloat(
        kContinuousId,
        &replacementContinuous,
        continuousRange.min,
        continuousRange.max,
        false,
        0.0f);
    router.bindFloat(
        kSteppedId,
        &replacementStepped,
        steppedRange.min,
        steppedRange.max,
        true,
        steppedRange.step);
    continuousMessage.value = 96;
    steppedMessage.value = 96;
    router.newMidiMessage(continuousMessage);
    router.newMidiMessage(steppedMessage);
    require(
        !nearlyEqual(replacementContinuous, 1.0f) &&
            !nearlyEqual(replacementStepped, 1.0f) &&
            nearlyEqual(retiredContinuous, retiredContinuousAtUnbind) &&
            nearlyEqual(retiredStepped, retiredSteppedAtUnbind),
        "MIDI replacement rebind retained or failed to replace a live target");

    continuousMapping = mappingFor(kContinuousId);
    steppedMapping = mappingFor(kSteppedId);
    require(
        continuousMapping &&
            !continuousMapping->snapInt &&
            nearlyEqual(continuousMapping->step, 0.0f) &&
            steppedMapping &&
            steppedMapping->snapInt &&
            nearlyEqual(steppedMapping->step, 1.0f),
        "MIDI replacement rebind changed explicit snap/step policy");

    router.unbindTargetsByPrefix(kPrefix);
    const float replacementContinuousAtClear = replacementContinuous;
    const float replacementSteppedAtClear = replacementStepped;
    continuousMessage.value = 12;
    steppedMessage.value = 12;
    router.newMidiMessage(continuousMessage);
    router.newMidiMessage(steppedMessage);
    require(
        nearlyEqual(
            replacementContinuous,
            replacementContinuousAtClear) &&
            nearlyEqual(replacementStepped, replacementSteppedAtClear),
        "cleared element storage remained reachable through MIDI targets");
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

    if (!hub.focusAssetById("tests.asset.dropdown")) {
        throw std::runtime_error("Could not explicitly focus dropdown asset");
    }
    hub.rebuildView();
    const auto& rows = hub.activeRowIndices();
    if (rows.empty()) {
        throw std::runtime_error("No parameter rows available for slot dropdown focus scenario");
    }
    const int slotColumn = static_cast<int>(ControlMappingHubState::Column::kSlot);
    const auto& gridItems = hub.activeGridItems();
    auto rowItemIt = std::find_if(gridItems.begin(), gridItems.end(), [](const ControlMappingHubState::GridItem& item) {
        return !item.sectionHeader;
    });
    if (rowItemIt == gridItems.end()) {
        throw std::runtime_error("No selectable parameter row available for slot dropdown focus scenario");
    }
    int rowItemIndex = static_cast<int>(std::distance(gridItems.begin(), rowItemIt));
    if (!hub.debugSetGridSelection(rowItemIndex, slotColumn)) {
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

bool RunSlotAssignmentTransactionScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const auto uniqueSuffix = std::to_string(
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count());
    const auto tempRoot =
        std::filesystem::temp_directory_path() /
        ("cmh_slot_assignment_transaction_" + uniqueSuffix);
    std::filesystem::create_directories(tempRoot);
    const auto mappingPath = tempRoot / "midi-map.json";

    ParameterRegistry registry;
    ParameterRegistry::Descriptor depthMeta;
    depthMeta.id = "tests.slot.depth";
    depthMeta.label = "Slot Depth";
    depthMeta.group = "Tests";
    depthMeta.range.min = 0.0f;
    depthMeta.range.max = 1.0f;
    float depth = 0.25f;
    registry.addFloat(
        depthMeta.id,
        &depth,
        depth,
        depthMeta);

    ParameterRegistry::Descriptor keepMeta = depthMeta;
    keepMeta.id = "tests.slot.keep";
    keepMeta.label = "Unrelated Route";
    float keep = 0.75f;
    registry.addFloat(
        keepMeta.id,
        &keep,
        keep,
        keepMeta);

    MidiRouter router;
    router.bindFloat(
        depthMeta.id,
        &depth,
        0.0f,
        1.0f,
        false,
        0.0f);
    router.bindFloat(
        keepMeta.id,
        &keep,
        0.0f,
        1.0f,
        false,
        0.0f);
    require(
        !router.load(mappingPath.string()),
        "Fresh transaction fixture unexpectedly loaded a MIDI map");
    const ofJson initialRoutes = {
        {"schemaVersion", 1},
        {"cc",
         ofJson::array({
             {{"num", 7}, {"target", depthMeta.id}},
             {{"num", 99}, {"target", keepMeta.id}},
         })},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array()},
        {"osc", ofJson::array()},
    };
    require(
        router.importMappingSnapshot(initialRoutes, true) &&
            router.save(""),
        "Could not seed transactional slot routing fixture");

    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    hub.setMenuSkin(MenuSkin::ConsoleHub());
    hub.setDeviceMapsDirectory(
        synaptome_test_paths::deviceMapsRoot().string());
    hub.setSlotAssignmentsPath(
        (tempRoot / "slot-assignments.json").string());

    int persistenceCalls = 0;
    bool persistenceSucceeds = true;
    ofJson lastPublishedSnapshot;
    hub.setSlotAssignmentPersistenceCallback(
        [&](const ofJson& snapshot) {
            ++persistenceCalls;
            lastPublishedSnapshot = snapshot;
            return persistenceSucceeds;
        });

    MenuController controller;
    hub.onEnter(controller);
    hub.view();

    const ofJson canonical = {
        {"assignments",
         ofJson::array({
             {
                 {"assignmentKey", depthMeta.id},
                 {"deviceProfileId", "MIDI Mix 0"},
                 {"slotId", "K1"},
                 {"analog", true},
             },
         })},
    };
    const auto imported =
        hub.importSlotAssignmentSnapshotTransactional(canonical);
    require(
        imported.ok &&
            imported.rollbackSucceeded &&
            imported.error.empty(),
        "Canonical slot-assignment transaction was rejected: " +
            imported.error);
    require(
        persistenceCalls == 1 &&
            lastPublishedSnapshot == canonical,
        "Successful slot-assignment transaction did not publish exactly "
        "one canonical snapshot");
    require(
        hub.exportSlotAssignmentSnapshot() == canonical,
        "Canonical slot-assignment import/export changed the shipped "
        "spaced device profile ID");
    require(
        std::count_if(
            router.getCcMaps().begin(),
            router.getCcMaps().end(),
            [&](const MidiRouter::CcMap& route) {
                return route.target == depthMeta.id;
            }) == 1,
        "Canonical slot-assignment transaction published duplicate "
        "MIDI routes");
    require(
        std::any_of(
            router.getCcMaps().begin(),
            router.getCcMaps().end(),
            [&](const MidiRouter::CcMap& route) {
                return route.target == depthMeta.id &&
                    route.cc == 16;
            }),
        "Canonical spaced-device assignment did not resolve shipped "
        "MIDI Mix 0.K1 routing");
    require(
        std::any_of(
            router.getCcMaps().begin(),
            router.getCcMaps().end(),
            [&](const MidiRouter::CcMap& route) {
                return route.target == keepMeta.id &&
                    route.cc == 99;
            }),
        "Canonical slot transaction replaced an unrelated route");

    const ofJson acceptedAssignments =
        hub.exportSlotAssignmentSnapshot();
    const ofJson acceptedRoutes =
        router.exportMappingSnapshot();
    const ofJson acceptedPersistedRoutes =
        ofLoadJson(mappingPath.string());

    const ofJson duplicate = {
        {"assignments",
         ofJson::array({
             canonical["assignments"][0],
             canonical["assignments"][0],
         })},
    };
    persistenceCalls = 0;
    const auto duplicateRejected =
        hub.importSlotAssignmentSnapshotTransactional(duplicate);
    require(
        !duplicateRejected.ok &&
            !duplicateRejected.error.empty() &&
            persistenceCalls == 0,
        "Duplicate slot assignment was accepted or reached persistence");
    require(
        hub.exportSlotAssignmentSnapshot() ==
                acceptedAssignments &&
            router.exportMappingSnapshot() == acceptedRoutes &&
            ofLoadJson(mappingPath.string()) ==
                acceptedPersistedRoutes,
        "Duplicate rejection changed assignments or routes");

    const ofJson malformed = {
        {"assignments",
         ofJson::array({
             {
                 {"assignmentKey", depthMeta.id},
                 {"deviceProfileId", "MIDI Mix 0"},
                 {"slotId", "K2"},
             },
         })},
    };
    const auto malformedRejected =
        hub.importSlotAssignmentSnapshotTransactional(malformed);
    require(
        !malformedRejected.ok &&
            !malformedRejected.error.empty() &&
            persistenceCalls == 0,
        "Malformed slot assignment was accepted or reached persistence");
    require(
        hub.exportSlotAssignmentSnapshot() ==
                acceptedAssignments &&
            router.exportMappingSnapshot() == acceptedRoutes &&
            ofLoadJson(mappingPath.string()) ==
                acceptedPersistedRoutes,
        "Malformed rejection changed assignments or routes");

    const ofJson replacement = {
        {"assignments",
         ofJson::array({
             {
                 {"assignmentKey", depthMeta.id},
                 {"deviceProfileId", "MIDI Mix 0"},
                 {"slotId", "K2"},
                 {"analog", true},
             },
         })},
    };
    persistenceSucceeds = false;
    persistenceCalls = 0;
    const auto persistenceRejected =
        hub.importSlotAssignmentSnapshotTransactional(replacement);
    require(
        !persistenceRejected.ok &&
            persistenceRejected.rollbackSucceeded &&
            !persistenceRejected.error.empty() &&
            persistenceCalls == 1,
        "Persistence rejection did not report a successful rollback");
    require(
        hub.exportSlotAssignmentSnapshot() ==
                acceptedAssignments &&
            router.exportMappingSnapshot() == acceptedRoutes &&
            ofLoadJson(mappingPath.string()) ==
                acceptedPersistedRoutes,
        "Persistence callback failure did not roll back assignment and "
        "MIDI snapshots");

    persistenceSucceeds = true;
    persistenceCalls = 0;
    const ofJson explicitlyEmpty = {
        {"assignments", ofJson::array()},
    };
    const auto cleared =
        hub.importSlotAssignmentSnapshotTransactional(explicitlyEmpty);
    require(
        cleared.ok &&
            cleared.rollbackSucceeded &&
            persistenceCalls == 1 &&
            lastPublishedSnapshot == explicitlyEmpty &&
            hub.exportSlotAssignmentSnapshot() == explicitlyEmpty,
        "Explicitly empty slot-assignment transaction did not clear and "
        "publish exactly once");
    require(
        std::none_of(
            router.getCcMaps().begin(),
            router.getCcMaps().end(),
            [&](const MidiRouter::CcMap& route) {
                return route.target == depthMeta.id;
            }),
        "Explicitly empty slot assignment left its MIDI route active");
    require(
        std::any_of(
            router.getCcMaps().begin(),
            router.getCcMaps().end(),
            [&](const MidiRouter::CcMap& route) {
                return route.target == keepMeta.id &&
                    route.cc == 99;
            }),
        "Explicitly empty slot assignment removed an unrelated route");

    hub.onExit(controller);
    return true;
}

bool RunSlotAssignmentUiRollbackScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const auto uniqueSuffix = std::to_string(
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count());
    const auto tempRoot =
        std::filesystem::temp_directory_path() /
        ("cmh_slot_assignment_ui_rollback_" + uniqueSuffix);
    const auto layersDir = tempRoot / "layers";
    std::filesystem::create_directories(layersDir);
    const auto assetPath = layersDir / "tests.asset.ui.json";
    {
        std::ofstream asset(assetPath);
        require(
            static_cast<bool>(asset),
            "Could not create UI rollback asset fixture");
        asset << R"JSON({
  "id": "tests.asset.ui",
  "label": "Slot UI Asset",
  "category": "Tests",
  "type": "generative.perlin",
  "registryPrefix": "tests.asset.ui"
})JSON";
    }

    LayerLibrary library;
    require(
        library.reload(layersDir.string()),
        "Could not load UI rollback asset fixture");
    const auto* entry = library.find("tests.asset.ui");
    require(entry != nullptr, "UI rollback asset was not indexed");

    ParameterRegistry registry;
    ParameterRegistry::Descriptor meta;
    meta.id = "console.layer1.opacity";
    meta.label = "UI Slot Opacity";
    meta.group = "Console";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    float opacity = 0.5f;
    registry.addFloat(meta.id, &opacity, opacity, meta);

    const auto mappingPath = tempRoot / "midi-map.json";
    MidiRouter router;
    router.bindFloat(
        meta.id,
        &opacity,
        0.0f,
        1.0f,
        false,
        0.0f);
    require(
        !router.load(mappingPath.string()),
        "Fresh UI rollback fixture unexpectedly loaded a MIDI map");

    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    hub.setLayerLibrary(&library);
    hub.setMenuSkin(MenuSkin::ConsoleHub());
    hub.setDeviceMapsDirectory(
        synaptome_test_paths::deviceMapsRoot().string());
    hub.setConsoleAssetResolver(
        [entry](const std::string& prefix)
            -> const LayerLibrary::Entry* {
            if (prefix.rfind("console.layer", 0) == 0 ||
                prefix == entry->registryPrefix ||
                prefix == entry->id) {
                return entry;
            }
            return nullptr;
        });
    hub.setConsoleSlotInventoryCallback([entry] {
        ConsoleLayerInfo info;
        info.index = 1;
        info.assetId = entry->id;
        info.active = true;
        info.label = entry->label;
        return std::vector<ConsoleLayerInfo>{info};
    });
    hub.setSlotAssignmentsPath(
        (tempRoot / "slot-assignments.json").string());

    int persistenceCalls = 0;
    bool persistenceSucceeds = true;
    hub.setSlotAssignmentPersistenceCallback(
        [&](const ofJson&) {
            ++persistenceCalls;
            return persistenceSucceeds;
        });

    MenuController controller;
    hub.onEnter(controller);
    hub.view();
    require(
        hub.focusAssetById(entry->id),
        "Could not focus UI rollback asset");
    hub.rebuildView();

    constexpr const char* kCanonicalAssignmentKey =
        "tests.asset.ui::opacity";
    const ofJson initialAssignment = {
        {"assignments",
         ofJson::array({
             {
                 {"assignmentKey", kCanonicalAssignmentKey},
                 {"deviceProfileId", "MIDI Mix 0"},
                 {"slotId", "K1"},
                 {"analog", true},
             },
         })},
    };
    const auto seeded =
        hub.importSlotAssignmentSnapshotTransactional(
            initialAssignment);
    require(
        seeded.ok && persistenceCalls == 1,
        "Could not seed UI slot assignment transaction: " +
            seeded.error);

    hub.rebuildView();
    const auto* row = hub.rowForId(meta.id);
    require(
        row != nullptr && row->isFloat &&
            row->floatParam != nullptr,
        "UI rollback parameter row was not available");

    persistenceSucceeds = false;
    persistenceCalls = 0;
    const ofJson beforeAssignmentFailure =
        hub.exportSlotAssignmentSnapshot();
    const ofJson beforeAssignmentRoutes =
        router.exportMappingSnapshot();
    const ofJson beforeAssignmentFile =
        ofLoadJson(mappingPath.string());

    require(
        hub.beginSlotPicker(*row),
        "Could not open UI slot picker for rollback test");
    bool selectedK2 = false;
    for (std::size_t pickerIndex = 0;
         pickerIndex < hub.slotPickerIndices_.size();
         ++pickerIndex) {
        const int slotIndex =
            hub.slotPickerIndices_[pickerIndex];
        if (slotIndex < 0 ||
            slotIndex >=
                static_cast<int>(hub.slotCatalog_.size())) {
            continue;
        }
        const auto& slot =
            hub.slotCatalog_[static_cast<std::size_t>(
                slotIndex)];
        if (slot.deviceId == "MIDI Mix 0" &&
            slot.slotId == "K2" &&
            slot.analog) {
            hub.slotPickerSelection_ =
                static_cast<int>(pickerIndex);
            selectedK2 = true;
            break;
        }
    }
    require(selectedK2, "Shipped MIDI Mix 0.K2 slot was absent");

    const bool uiAssigned = hub.applySelectedSlot();
    require(
        !uiAssigned && persistenceCalls == 1,
        "UI slot assignment did not report one rejected persistence "
        "attempt");
    require(
        hub.exportSlotAssignmentSnapshot() ==
                beforeAssignmentFailure &&
            router.exportMappingSnapshot() ==
                beforeAssignmentRoutes &&
            ofLoadJson(mappingPath.string()) ==
                beforeAssignmentFile,
        "UI slot assignment persistence failure left partial "
        "assignment or route state");

    hub.cancelSlotPicker();
    hub.rebuildView();
    const auto& gridItems = hub.activeGridItems();
    int gridItemIndex = -1;
    for (std::size_t index = 0;
         index < gridItems.size();
         ++index) {
        const auto& item = gridItems[index];
        if (item.sectionHeader ||
            item.rowIndex < 0 ||
            item.rowIndex >=
                static_cast<int>(
                    hub.tableModel_.rows.size())) {
            continue;
        }
        if (hub.tableModel_.rows[
                static_cast<std::size_t>(
                    item.rowIndex)].id == meta.id) {
            gridItemIndex = static_cast<int>(index);
            break;
        }
    }
    require(
        gridItemIndex >= 0 &&
            hub.debugSetGridSelection(
                gridItemIndex,
                static_cast<int>(
                    ControlMappingHubState::Column::kSlot)),
        "Could not select the UI slot cell for unmap rollback");

    persistenceCalls = 0;
    const ofJson beforeUnmapAssignments =
        hub.exportSlotAssignmentSnapshot();
    const ofJson beforeUnmapRoutes =
        router.exportMappingSnapshot();
    const ofJson beforeUnmapFile =
        ofLoadJson(mappingPath.string());
    require(
        hub.handleInput(controller, 'u'),
        "UI slot unmap input was not handled");
    require(
        persistenceCalls == 1 &&
            hub.exportSlotAssignmentSnapshot() ==
                beforeUnmapAssignments &&
            router.exportMappingSnapshot() ==
                beforeUnmapRoutes &&
            ofLoadJson(mappingPath.string()) ==
                beforeUnmapFile,
        "UI unmap persistence failure left partial assignment or "
        "route state");

    persistenceCalls = 0;
    hub.slotAssignments_.clear();
    ControlMappingHubState::LogicalSlotBinding legacyAlias;
    legacyAlias.deviceId = "MIDI Mix 0";
    legacyAlias.deviceName = "MIDI Mix 0";
    legacyAlias.slotId = "K1";
    legacyAlias.slotLabel = "Knob 1";
    legacyAlias.analog = true;
    hub.slotAssignments_[meta.id] =
        std::move(legacyAlias);
    hub.slotAssignmentsLoaded_ = true;
    hub.slotAssignmentsDirty_ = false;
    const ofJson beforeAliasRenderRoutes =
        router.exportMappingSnapshot();
    hub.tableModel_.dirty = true;
    hub.invalidateRowCache();
    hub.view();
    require(
        persistenceCalls == 0,
        "Rendering a legacy slot-assignment alias invoked "
        "persistence");
    require(
        router.exportMappingSnapshot() ==
            beforeAliasRenderRoutes,
        "Rendering a legacy slot-assignment alias changed routes");

    hub.slotAssignmentsDirty_ = false;
    hub.onExit(controller);
    return true;
}

#include "midi_input_binding_flow.inc"

struct SensorSample {
    std::string parameterId;
    float value = 0.0f;
    uint64_t timestampMs = 0;
};

bool RunSynaptomeMeshOscCompatibilityScenario() {
    auto numericMessage = [](const std::string& address,
                             float value,
                             uint64_t timestampMs) {
        OscIngressMessage message;
        message.rawAddress = address;
        message.typeTags = ",f";
        message.transport = "udp";
        message.endpoint = "127.0.0.1:9002";
        message.timestampMs = timestampMs;
        message.arguments.push_back(
            OscIngressAtom::numeric(OscIngressAtomType::Float32, value));
        OscIngressCompatibility::normalizeSynaptomeMeshV1(message);
        return message;
    };

    auto legacy = numericMessage(
        "/sensor/hr/0x0301/heart-bpm",
        72.0f,
        100);
    auto namespaced = numericMessage(
        "/synaptome_mesh/sensor/hr/0x0301/heart-bpm",
        72.0f,
        101);
    if (legacy.canonicalAddress != "/sensor/hr/0x0301/bpm"
        || namespaced.canonicalAddress != legacy.canonicalAddress
        || legacy.meshNamespaceAlias
        || !namespaced.meshNamespaceAlias
        || !legacy.meshRouteAliasApplied
        || !namespaced.meshRouteAliasApplied) {
        throw std::runtime_error("Mesh heart-bpm compatibility normalization failed");
    }

    OscIngressCompatibility::MeshDualEmissionDeduper deduper(25);
    if (deduper.shouldSuppress(legacy)) {
        throw std::runtime_error("Mesh legacy route was suppressed before its paired alias");
    }
    if (!deduper.shouldSuppress(namespaced)
        || !namespaced.duplicateSuppressed) {
        throw std::runtime_error("Mesh dual-emitted namespace alias was not suppressed");
    }

    auto repeatedLegacy = numericMessage(
        "/sensor/hr/0x0301/heart-bpm",
        72.0f,
        140);
    auto repeatedAlias = numericMessage(
        "/synaptome_mesh/sensor/hr/0x0301/heart-bpm",
        72.0f,
        141);
    if (deduper.shouldSuppress(repeatedLegacy)
        || !deduper.shouldSuppress(repeatedAlias)) {
        throw std::runtime_error("Repeated Mesh event pairs were not independently accepted");
    }

    auto matrixAlias = numericMessage(
        "/synaptome_mesh/sensor/matrix/0x0101/mic-level",
        0.42f,
        200);
    if (matrixAlias.canonicalAddress != "/sensor/matrix/0x0101/mic-level"
        || matrixAlias.meshRouteAliasApplied) {
        throw std::runtime_error("Mesh identity route normalization changed the metric");
    }
    float scalarValue = 0.0f;
    if (!matrixAlias.finiteNumericScalar(scalarValue)
        || std::fabs(scalarValue - 0.42f) > 0.0001f) {
        throw std::runtime_error("Mesh numeric payload was not retained as a scalar");
    }

    OscIngressMessage stringMessage;
    stringMessage.rawAddress =
        "/synaptome_mesh/system/matrix/0x0101/device-type-name";
    stringMessage.typeTags = ",s";
    stringMessage.transport = "serial-slip";
    stringMessage.endpoint = "COM7";
    stringMessage.timestampMs = 210;
    stringMessage.arguments.push_back(
        OscIngressAtom::text(OscIngressAtomType::String, "matrix"));
    OscIngressCompatibility::normalizeSynaptomeMeshV1(stringMessage);
    if (stringMessage.canonicalAddress
            != "/system/matrix/0x0101/device-type-name"
        || stringMessage.finiteNumericScalar(scalarValue)
        || stringMessage.payloadSummary() != "\"matrix\"") {
        throw std::runtime_error("Mesh string payload was not preserved as diagnostics");
    }

    OscIngressMessage multiArgument;
    multiArgument.rawAddress = "/vendor/device/pose";
    multiArgument.canonicalAddress = multiArgument.rawAddress;
    multiArgument.typeTags = ",fff";
    multiArgument.arguments = {
        OscIngressAtom::numeric(OscIngressAtomType::Float32, 0.1),
        OscIngressAtom::numeric(OscIngressAtomType::Float32, 0.2),
        OscIngressAtom::numeric(OscIngressAtomType::Float32, 0.3)
    };
    if (multiArgument.finiteNumericScalar(scalarValue)
        || multiArgument.payloadSummary().empty()) {
        throw std::runtime_error("Generic multi-argument OSC was treated as a scalar");
    }

    OscIngressMessage nonFinite = numericMessage(
        "/vendor/device/nonfinite",
        std::numeric_limits<float>::infinity(),
        220);
    if (nonFinite.finiteNumericScalar(scalarValue)) {
        throw std::runtime_error("Non-finite OSC value entered scalar routing");
    }

    OscIngressMessage boolMessage;
    boolMessage.rawAddress = "/vendor/device/enabled";
    boolMessage.canonicalAddress = boolMessage.rawAddress;
    boolMessage.typeTags = ",T";
    OscIngressAtom boolAtom;
    boolAtom.type = OscIngressAtomType::Bool;
    boolAtom.numericValue = 1.0;
    boolMessage.arguments.push_back(boolAtom);
    if (boolMessage.finiteNumericScalar(scalarValue)
        || boolMessage.payloadSummary() != "true") {
        throw std::runtime_error("OSC bool observation was coerced into numeric routing");
    }

    OscIngressMessage int64Message;
    int64Message.rawAddress = "/vendor/device/counter";
    int64Message.canonicalAddress = int64Message.rawAddress;
    int64Message.typeTags = ",h";
    int64Message.arguments.push_back(OscIngressAtom::integer(
        OscIngressAtomType::Int64,
        std::numeric_limits<std::int64_t>::max()));
    if (int64Message.payloadSummary() != "9223372036854775807") {
        throw std::runtime_error("OSC int64 diagnostic precision was not preserved");
    }

    auto appendPaddedString = [](std::vector<std::uint8_t>& packet,
                                 const std::string& value) {
        packet.insert(packet.end(), value.begin(), value.end());
        packet.push_back(0);
        while ((packet.size() % 4) != 0) {
            packet.push_back(0);
        }
    };
    auto appendU32 = [](std::vector<std::uint8_t>& packet,
                        std::uint32_t value) {
        packet.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
        packet.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
        packet.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
        packet.push_back(static_cast<std::uint8_t>(value & 0xff));
    };
    auto makeSerialPacket = [&](const std::string& address,
                                const std::string& tags) {
        std::vector<std::uint8_t> packet;
        appendPaddedString(packet, address);
        appendPaddedString(packet, tags);
        return packet;
    };

    SerialSlipOsc serialParser;
    serialParser.activePort = "COM7";
    std::vector<OscIngressMessage> serialEvents;
    auto captureSerial = [&](const OscIngressMessage& message) {
        serialEvents.push_back(message);
    };

    auto serialFloat = makeSerialPacket("/sensor/matrix/0x0101/mic-level", ",f");
    float serialFloatValue = 0.42f;
    std::uint32_t serialFloatBits = 0;
    std::memcpy(&serialFloatBits, &serialFloatValue, sizeof(serialFloatBits));
    appendU32(serialFloat, serialFloatBits);
    serialParser.parseFrame(
        serialFloat.data(),
        static_cast<int>(serialFloat.size()),
        captureSerial);

    auto serialInt = makeSerialPacket("/param/rx/deck/0x0201/page", ",i");
    appendU32(serialInt, 3);
    serialParser.parseFrame(
        serialInt.data(),
        static_cast<int>(serialInt.size()),
        captureSerial);

    auto serialString = makeSerialPacket(
        "/system/matrix/0x0101/device-type-name",
        ",s");
    appendPaddedString(serialString, "matrix");
    serialParser.parseFrame(
        serialString.data(),
        static_cast<int>(serialString.size()),
        captureSerial);

    if (serialEvents.size() != 3
        || serialEvents[0].transport != "serial-slip"
        || serialEvents[0].endpoint != "COM7"
        || serialEvents[0].payloadSummary() != "0.42"
        || serialEvents[1].payloadSummary() != "3"
        || serialEvents[2].payloadSummary() != "\"matrix\"") {
        throw std::runtime_error("Serial SLIP OSC did not preserve Mesh f/i/s payloads");
    }

    auto truncatedString = makeSerialPacket("/system/matrix/0x0101/role-name", ",s");
    truncatedString.push_back('x');
    const auto eventsBeforeTruncation = serialEvents.size();
    serialParser.parseFrame(
        truncatedString.data(),
        static_cast<int>(truncatedString.size()),
        captureSerial);
    if (serialEvents.size() != eventsBeforeTruncation) {
        throw std::runtime_error("Truncated serial OSC string mutated ingress state");
    }
    return true;
}

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
    {
        CapturingTelemetrySink telemetry;
        layer.collectTelemetry(telemetry);
        const auto* labelEntry =
            telemetry.find("media.sourceLabel");
        const auto* initializedEntry =
            telemetry.find("media.captureInitialized");
        const auto* label = labelEntry
            ? std::get_if<std::string>(&labelEntry->value)
            : nullptr;
        const auto* initialized = initializedEntry
            ? std::get_if<bool>(&initializedEntry->value)
            : nullptr;
        if (!label || *label != "Integrated Webcam" ||
            !initialized || !*initialized) {
            throw std::runtime_error(
                "Webcam telemetry did not expose the live source and capture state");
        }
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
    {
        CapturingTelemetrySink telemetry;
        layer.collectTelemetry(telemetry);
        const auto* labelEntry =
            telemetry.find("media.sourceLabel");
        const auto* initializedEntry =
            telemetry.find("media.captureInitialized");
        const auto* label = labelEntry
            ? std::get_if<std::string>(&labelEntry->value)
            : nullptr;
        const auto* initialized = initializedEntry
            ? std::get_if<bool>(&initializedEntry->value)
            : nullptr;
        if (!label || *label != "(none)" ||
            !initialized || *initialized) {
            throw std::runtime_error(
                "Webcam telemetry did not refresh after capture failure");
        }
    }

    grabber->setupShouldSucceed = true;
    layer.forceDeviceRefresh();
    if (!grabber->setupEvents.back().success) {
        throw std::runtime_error("Webcam did not recover after failure");
    }
    {
        CapturingTelemetrySink telemetry;
        layer.collectTelemetry(telemetry);
        const auto* labelEntry =
            telemetry.find("media.sourceLabel");
        const auto* initializedEntry =
            telemetry.find("media.captureInitialized");
        const auto* label = labelEntry
            ? std::get_if<std::string>(&labelEntry->value)
            : nullptr;
        const auto* initialized = initializedEntry
            ? std::get_if<bool>(&initializedEntry->value)
            : nullptr;
        if (!label || *label != "Integrated Webcam" ||
            !initialized || !*initialized) {
            throw std::runtime_error(
                "Webcam telemetry did not refresh after capture recovery");
        }
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
    if (!hub.focusAssetById("tests.asset.simple")) {
        throw std::runtime_error("Could not explicitly focus console hotkey asset");
    }
    hub.view();
    hub.rebuildView();
    const auto& groupedItems = hub.activeGridItems();
    if (groupedItems.size() < 2 || !groupedItems.front().sectionHeader) {
        throw std::runtime_error("Grouped asset view did not expose a section header before asset rows");
    }
    if (!hub.debugSetGridSelection(0, static_cast<int>(ControlMappingHubState::Column::kName))) {
        throw std::runtime_error("Failed to select grouped asset section header for console slot hotkey scenario");
    }

    int loadKey = MenuController::HOTKEY_MOD_CTRL | '1';
    hub.handleInput(controller, loadKey);
    if (loadRequests.size() != 1) {
        throw std::runtime_error("Expected one console slot load request");
    }
    if (loadRequests.front().second != "tests.asset.simple") {
        throw std::runtime_error("Unexpected asset id in load request: " + loadRequests.front().second);
    }

    hub.rebuildView();
    if (!hub.focusAssetById("tests.asset.simple")) {
        throw std::runtime_error(
            "Could not restore asset focus after loading console slot");
    }
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
    const auto& targetRow =
        hub.tableModel_.rows[static_cast<std::size_t>(targetRowIndex)];
    hub.setParameterSectionExpanded(
        hub.parameterSectionExpansionKey(targetRow.section), true);
    hub.invalidateRowCache();
    const auto& slotItems = hub.activeGridItems();
    auto targetItemIt = std::find_if(
        slotItems.begin(), slotItems.end(),
        [&](const ControlMappingHubState::GridItem& item) {
            return !item.sectionHeader && item.rowIndex == targetRowIndex;
        });
    if (targetItemIt == slotItems.end() ||
        !hub.debugSetGridSelection(
            static_cast<int>(std::distance(slotItems.begin(), targetItemIt)),
            slotColumn)) {
        throw std::runtime_error(
            "Could not focus console layer opacity slot cell");
    }
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

bool RunSceneParameterPersistenceScenario() {
    ParameterRegistry registry;
    ParameterRegistry::Descriptor meta;

    float liveFloat = 0.25f;
    auto& floatParam = registry.addFloat("test.scene.float", &liveFloat, liveFloat, meta);
    liveFloat = 0.75f;
    if (std::abs(floatParam.valueForPersistence() - 0.75f) > 0.0001f) {
        throw std::runtime_error("Unmodulated float persistence did not capture the live value");
    }

    modifier::Modifier floatModifier;
    floatModifier.type = modifier::Type::kAutomation;
    registry.addFloatModifier("test.scene.float", floatModifier);
    liveFloat = 0.9f;
    if (std::abs(floatParam.valueForPersistence() - 0.25f) > 0.0001f) {
        throw std::runtime_error("Modulated float persistence did not preserve the base value");
    }

    bool liveBool = false;
    auto& boolParam = registry.addBool("test.scene.bool", &liveBool, liveBool, meta);
    liveBool = true;
    if (!boolParam.valueForPersistence()) {
        throw std::runtime_error("Unmodulated bool persistence did not capture the live value");
    }

    std::string liveString = "before";
    auto& stringParam =
        registry.addString("test.scene.string", &liveString, liveString, meta);
    liveString = "after";
    if (stringParam.valueForPersistence() != "after") {
        throw std::runtime_error("String persistence did not capture the live value");
    }

    const synaptome::state::ParameterBaseOrigin sceneOrigin{
        synaptome::state::ParameterBaseOriginKind::Scene,
        "layers/scenes/legacy-origin.json",
        1,
        {"scene-v1-to-v2"},
    };
    registry.setFloatBase(
        "test.scene.float",
        0.4f,
        sceneOrigin,
        true);
    registry.setBoolBase(
        "test.scene.bool",
        false,
        sceneOrigin,
        true);
    registry.setStringBase(
        "test.scene.string",
        "scene",
        sceneOrigin,
        true);
    if (floatParam.baseOrigin != sceneOrigin ||
        boolParam.baseOrigin != sceneOrigin ||
        stringParam.baseOrigin != sceneOrigin) {
        throw std::runtime_error(
            "Scene value application did not retain source version or migration origin");
    }
    registry.setStringBase(
        "test.scene.string",
        "operator",
        {synaptome::state::ParameterBaseOriginKind::OperatorEdit,
         "tests.operator"},
        true);
    if (stringParam.baseOrigin.kind !=
        synaptome::state::ParameterBaseOriginKind::OperatorEdit) {
        throw std::runtime_error(
            "Operator edit did not replace the prior Scene base origin");
    }

    return true;
}

bool RunSceneStateDocumentVersionScenario() {
    using synaptome::state::SceneDocumentError;
    using synaptome::state::SceneDocumentKind;
    using synaptome::state::normalizeSceneDocument;
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const ofJson legacy = {
        {"globals", {{"transport.bpm", 120.0}}},
        {"mappings", {{"activeBank", "home"}}},
    };
    const ofJson legacyBefore = legacy;
    const auto normalizedLegacy = normalizeSceneDocument(legacy);
    require(normalizedLegacy.ok,
            "Unversioned legacy scene was rejected");
    require(normalizedLegacy.kind == SceneDocumentKind::LegacyV1 &&
                normalizedLegacy.sourceVersion == 1 &&
                normalizedLegacy.migratedInMemory,
            "Unversioned scene was not classified as legacy v1");
    require(normalizedLegacy.document["scene"]["schemaVersion"] == 2,
            "Legacy scene was not normalized to current v2 in memory");
    require(normalizedLegacy.document["mappings"] == legacy["mappings"],
            "Legacy normalization changed mapping presence/value semantics");
    require(legacy == legacyBefore && !legacy.contains("scene"),
            "Legacy normalization mutated its source document");

    const ofJson explicitV1 = {
        {"scene", {{"schemaVersion", 1}}},
        {"mappings", {{"router", ofJson::object()}}},
    };
    const auto normalizedV1 = normalizeSceneDocument(explicitV1);
    require(normalizedV1.ok &&
                normalizedV1.kind == SceneDocumentKind::LegacyV1 &&
                normalizedV1.sourceVersion == 1 &&
                normalizedV1.document["mappings"].contains("router") &&
                normalizedV1.document["mappings"]["router"].empty(),
            "Explicit v1 scene did not retain authoritative empty mappings");

    const ofJson currentV2 = {
        {"scene", {{"schemaVersion", 2}}},
        {"console", {{"slots", ofJson::array()}}},
    };
    const auto normalizedV2 = normalizeSceneDocument(currentV2);
    require(normalizedV2.ok &&
                normalizedV2.kind == SceneDocumentKind::CurrentV2 &&
                normalizedV2.sourceVersion == 2 &&
                !normalizedV2.migratedInMemory &&
                normalizedV2.document == currentV2,
            "Current v2 scene was not accepted unchanged");

    for (const auto& invalid : std::vector<ofJson>{
             ofJson::array(),
             ofJson{{"scene", "invalid"}},
             ofJson{{"scene", {{"schemaVersion", "2"}}}},
             ofJson{{"scene", {{"schemaVersion", 0}}}},
             ofJson{{"scene", {{"schemaVersion", 3}}}},
         }) {
        const auto rejected = normalizeSceneDocument(invalid);
        require(!rejected.ok && !rejected.error.empty(),
                "Invalid/future scene version was accepted");
    }
    const auto future = normalizeSceneDocument(
        ofJson{{"scene", {{"schemaVersion", 3}}}});
    require(
        future.errorCode == SceneDocumentError::UnsupportedFutureVersion,
        "Future scene version did not remain distinguishable from corruption");

    const ofJson omittedMappings = {
        {"scene", {{"schemaVersion", 2}}},
    };
    const auto normalizedOmitted = normalizeSceneDocument(omittedMappings);
    require(normalizedOmitted.ok &&
                !normalizedOmitted.document.contains("mappings"),
            "Scene normalization invented an omitted mappings owner");
    return true;
}

bool RunSceneSlotAssignmentOwnershipScenario() {
    using synaptome::state::normalizeSceneDocument;
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const auto fixturePath =
        synaptome_test_paths::appRoot().parent_path() /
        "tools" / "testdata" / "scene_persistence" /
        "slot_assignment_ownership_cases.json";
    const ofJson fixture = ofLoadJson(fixturePath.string());
    require(
        fixture.is_object() &&
            fixture.value("policyVersion", 0) == 1 &&
            fixture.contains("writerCases") &&
            fixture["writerCases"].is_array() &&
            fixture.contains("legacyReadCases") &&
            fixture["legacyReadCases"].is_array(),
        "Malformed Scene slot-assignment ownership fixture");

    bool namedWriterCovered = false;
    bool autosaveWriterCovered = false;
    for (const auto& writerCase : fixture["writerCases"]) {
        require(
            writerCase.is_object() &&
                writerCase.contains("id") &&
                writerCase["id"].is_string() &&
                writerCase.contains("path") &&
                writerCase["path"].is_string() &&
                writerCase.contains("storageKind") &&
                writerCase["storageKind"].is_string() &&
                writerCase.contains("document"),
            "Malformed Scene slot-assignment writer case");
        const std::string caseId = writerCase["id"].get<std::string>();
        const std::string storageKind =
            writerCase["storageKind"].get<std::string>();
        const ofJson source = writerCase["document"];
        const ofJson sourceBefore = source;
        const auto normalized = normalizeSceneDocument(source);
        require(
            normalized.ok &&
                normalized.document == source &&
                source == sourceBefore,
            "Canonical Scene writer case was rejected or mutated: " +
                caseId);
        require(
            normalized.document.contains("scene") &&
                normalized.document["scene"].is_object() &&
                normalized.document["scene"].contains("storage") &&
                normalized.document["scene"]["storage"].is_object() &&
                normalized.document["scene"]["storage"].value(
                    "kind",
                    std::string()) == storageKind,
            "Scene writer case storage kind drifted: " + caseId);
        require(
            !normalized.document.contains("mappings") ||
                !normalized.document["mappings"].contains(
                    "slotAssignments"),
            "Canonical Scene writer case absorbed machine-local "
            "slot assignments: " +
                caseId);
        namedWriterCovered |= storageKind == "named";
        autosaveWriterCovered |= storageKind == "autosave";
    }
    require(
        namedWriterCovered && autosaveWriterCovered,
        "Scene ownership fixture must pin named and recovery-autosave "
        "writer policy");

    bool nonemptyLegacyCovered = false;
    bool emptyLegacyCovered = false;
    for (const auto& readCase : fixture["legacyReadCases"]) {
        require(
            readCase.is_object() &&
                readCase.contains("id") &&
                readCase["id"].is_string() &&
                readCase.value(
                    "expectedCompatibility",
                    std::string()) == "accepted" &&
                readCase.value(
                    "expectedNormalLoadAction",
                    std::string()) == "preserve-machine" &&
                readCase.value("explicitImportRequired", false) &&
                readCase.contains("document"),
            "Malformed legacy Scene slot-assignment compatibility case");
        const std::string caseId = readCase["id"].get<std::string>();
        const ofJson source = readCase["document"];
        const ofJson sourceBefore = source;
        const auto normalized = normalizeSceneDocument(source);
        require(
            normalized.ok &&
                normalized.document == source &&
                source == sourceBefore,
            "Legacy Scene slot-assignment input was rejected or mutated: " +
                caseId);
        require(
            normalized.document.contains("mappings") &&
                normalized.document["mappings"].is_object() &&
                normalized.document["mappings"].contains(
                    "slotAssignments") &&
                normalized.document["mappings"]["slotAssignments"].
                    is_object() &&
                normalized.document["mappings"]["slotAssignments"].
                    contains("assignments") &&
                normalized.document["mappings"]["slotAssignments"]
                    ["assignments"].is_array(),
            "Legacy Scene slot-assignment compatibility shape drifted: " +
                caseId);
        const bool empty =
            normalized.document["mappings"]["slotAssignments"]
                ["assignments"].empty();
        emptyLegacyCovered |= empty;
        nonemptyLegacyCovered |= !empty;
    }
    require(
        nonemptyLegacyCovered && emptyLegacyCovered,
        "Legacy Scene compatibility must cover non-empty and explicitly "
        "empty slot assignments");

    return true;
}

bool RunMachineProfileDocumentVersionScenario() {
    using synaptome::state::MachineProfileDocumentError;
    using synaptome::state::MachineProfileDocumentKind;
    using synaptome::state::validateMachineProfileDocument;
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const auto fixtureRoot =
        synaptome_test_paths::appRoot().parent_path() /
        "tools" / "testdata" / "machine_profile";

    const ofJson canonical =
        ofLoadJson((fixtureRoot / "canonical_v1.json").string());
    const ofJson canonicalBefore = canonical;
    const auto accepted = validateMachineProfileDocument(canonical);
    require(
        accepted.ok &&
            accepted.kind == MachineProfileDocumentKind::CurrentV1 &&
            accepted.errorCode == MachineProfileDocumentError::None &&
            accepted.sourceVersion == 1 &&
            accepted.document == canonical,
        "Canonical machine profile v1 was not accepted unchanged");
    require(canonical == canonicalBefore,
            "Machine profile validation mutated its source document");

    const ofJson empty =
        ofLoadJson((fixtureRoot / "canonical_empty_v1.json").string());
    const ofJson emptyBefore = empty;
    const auto acceptedEmpty = validateMachineProfileDocument(empty);
    require(
        acceptedEmpty.ok &&
            acceptedEmpty.document["osc"]["inputs"].empty() &&
            !acceptedEmpty.document["osc"].contains("activeInputId"),
        "Canonical empty machine profile v1 was not accepted");
    require(empty == emptyBefore,
            "Empty machine profile validation mutated its source document");

    const ofJson invalidFixture =
        ofLoadJson((fixtureRoot / "invalid_cases.json").string());
    require(invalidFixture.is_object() &&
                invalidFixture.contains("cases") &&
                invalidFixture["cases"].is_array() &&
                !invalidFixture["cases"].empty(),
            "Machine profile invalid-case fixture is empty");
    for (const auto& invalidCase : invalidFixture["cases"]) {
        require(invalidCase.is_object() &&
                    invalidCase.contains("id") &&
                    invalidCase["id"].is_string() &&
                    invalidCase.contains("document"),
                "Malformed machine profile invalid-case fixture");
        const std::string caseId = invalidCase["id"].get<std::string>();
        const ofJson source = invalidCase["document"];
        const ofJson sourceBefore = source;
        const auto rejected = validateMachineProfileDocument(source);
        require(!rejected.ok && !rejected.error.empty(),
                "Invalid machine profile was accepted: " + caseId);
        require(source == sourceBefore,
                "Rejected machine profile source was mutated: " + caseId);
        const auto expectedError =
            caseId == "future-version"
                ? MachineProfileDocumentError::UnsupportedFutureVersion
                : MachineProfileDocumentError::InvalidDocument;
        require(rejected.errorCode == expectedError,
                "Machine profile rejection used the wrong error class: " +
                    caseId);
    }

    const ofJson midiFixture =
        ofLoadJson(
            (fixtureRoot / "midi_binding_cases.json").string());
    require(
        midiFixture.is_object() &&
            midiFixture.value("policyVersion", 0) == 1 &&
            midiFixture.contains("acceptedDocuments") &&
            midiFixture["acceptedDocuments"].is_array() &&
            midiFixture.contains("invalidDocuments") &&
            midiFixture["invalidDocuments"].is_array(),
        "Malformed machine-profile physical-MIDI fixture");
    for (const auto& acceptedCase :
         midiFixture["acceptedDocuments"]) {
        require(
            acceptedCase.is_object() &&
                acceptedCase.contains("id") &&
                acceptedCase["id"].is_string() &&
                acceptedCase.contains("expectedApplyAction") &&
                acceptedCase["expectedApplyAction"].is_string() &&
                acceptedCase.contains("document"),
            "Malformed accepted physical-MIDI document case");
        const std::string caseId =
            acceptedCase["id"].get<std::string>();
        const ofJson source = acceptedCase["document"];
        const ofJson sourceBefore = source;
        const auto validated =
            validateMachineProfileDocument(source);
        require(
            validated.ok &&
                validated.document == source &&
                source == sourceBefore,
            "Valid physical-MIDI machine-profile document was rejected "
            "or mutated: " +
                caseId);
    }
    for (const auto& invalidCase :
         midiFixture["invalidDocuments"]) {
        require(
            invalidCase.is_object() &&
                invalidCase.contains("id") &&
                invalidCase["id"].is_string() &&
                invalidCase.contains("document"),
            "Malformed rejected physical-MIDI document case");
        const std::string caseId =
            invalidCase["id"].get<std::string>();
        const ofJson source = invalidCase["document"];
        const ofJson sourceBefore = source;
        const auto rejected =
            validateMachineProfileDocument(source);
        require(
            !rejected.ok &&
                rejected.errorCode ==
                    MachineProfileDocumentError::InvalidDocument &&
                !rejected.error.empty(),
            "Malformed or duplicate physical-MIDI profile was accepted: " +
                caseId);
        require(
            source == sourceBefore,
            "Rejected physical-MIDI machine-profile source was mutated: " +
                caseId);
    }

    return true;
}

bool RunMappingBankDocumentVersionScenario() {
    using synaptome::state::MappingBankDocumentError;
    using synaptome::state::MappingBankDocumentKind;
    using synaptome::state::normalizeMappingBankDocument;
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const ofJson legacy = {
        {"cc", ofJson::array({
            {{"num", 12}, {"target", "test.mapping.depth"}}
        })},
    };
    const ofJson legacyBefore = legacy;
    const auto normalizedLegacy =
        normalizeMappingBankDocument(legacy);
    require(
        normalizedLegacy.ok &&
            normalizedLegacy.kind ==
                MappingBankDocumentKind::LegacyUnversioned &&
            normalizedLegacy.sourceVersion == 0 &&
            normalizedLegacy.migratedInMemory &&
            normalizedLegacy.document["schemaVersion"] == 1,
        "Unversioned mapping snapshot was not normalized to v1");
    require(
        normalizedLegacy.document["cc"] == legacy["cc"] &&
            legacy == legacyBefore &&
            !legacy.contains("schemaVersion"),
        "Legacy mapping normalization changed or mutated route state");

    const ofJson current = {
        {"schemaVersion", 1},
        {"cc", ofJson::array()},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array()},
        {"osc", ofJson::array()},
    };
    const auto normalizedCurrent =
        normalizeMappingBankDocument(current);
    require(
        normalizedCurrent.ok &&
            normalizedCurrent.kind ==
                MappingBankDocumentKind::CurrentV1 &&
            normalizedCurrent.sourceVersion == 1 &&
            !normalizedCurrent.migratedInMemory &&
            normalizedCurrent.document == current,
        "Current mapping-bank v1 was not accepted unchanged");

    for (const auto& invalid : std::vector<ofJson>{
             ofJson::array(),
             ofJson{{"schemaVersion", "1"}},
             ofJson{{"schemaVersion", 0}},
             ofJson{{"schemaVersion", -1}},
             ofJson{{"schemaVersion", 2}},
             ofJson{
                 {"version", 1},
                 {"bank", "default"},
                 {"mappings", ofJson::array()},
             },
         }) {
        const auto rejected =
            normalizeMappingBankDocument(invalid);
        require(
            !rejected.ok && !rejected.error.empty(),
            "Invalid, future, or interchange mapping document was accepted");
    }
    const auto future = normalizeMappingBankDocument(
        ofJson{
            {"schemaVersion", 2},
            {"mappings", ofJson::object()},
        });
    require(
        future.errorCode ==
            MappingBankDocumentError::UnsupportedFutureVersion,
        "Future mapping-bank version was not distinguishable from corruption");

    const auto normalizedNull =
        normalizeMappingBankDocument(ofJson());
    require(
        normalizedNull.ok &&
            normalizedNull.kind ==
                MappingBankDocumentKind::LegacyUnversioned &&
            normalizedNull.document["schemaVersion"] == 1,
        "Legacy null explicit-empty snapshot lost compatibility");
    return true;
}

bool RunPreferencesDocumentVersionScenario() {
    using synaptome::state::PreferencesDocumentError;
    using synaptome::state::PreferencesDocumentKind;
    using synaptome::state::PreferencesPublisher;
    using synaptome::state::normalizePreferencesDocument;
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const ofJson legacy = {
        {"treeWidthRatio", 0.16},
        {"selectedCategory", "Scenes"},
        {"selectedSubcategory", "Saved"},
        {"selectedAsset", ""},
        {"selectedColumn", "value"},
        {"visibleColumns", {{"name", true}, {"value", true}}},
        {"collapsedCategories", ofJson::array({"Examples"})},
        {"hudVisible", true},
        {"hudLayoutTarget", "controller"},
        {"hudWidgets", ofJson::array({
            {
                {"id", "hud.status"},
                {"column", 0},
                {"band", "hud"},
                {"visible", true},
                {"collapsed", false},
            }
        })},
        {"hudControllerWidgets", ofJson::array({
            {
                {"id", "hud.status"},
                {"column", 1},
                {"band", "hud"},
                {"collapsed", true},
            }
        })},
    };
    const ofJson legacyBefore = legacy;
    const auto migrated = normalizePreferencesDocument(legacy);
    require(
        migrated.ok &&
            migrated.kind ==
                PreferencesDocumentKind::LegacyControlHub &&
            migrated.migratedInMemory &&
            migrated.sourceVersion == 0 &&
            migrated.document["schemaVersion"] == 1,
        "Legacy Control Hub preferences were not migrated to v1");
    require(
        migrated.document["browser"]["selection"]["category"] ==
                "Scenes" &&
            migrated.document["hud"]["widgets"].size() == 2 &&
            migrated.document["hud"]["widgets"][0]["target"] ==
                "projector" &&
            migrated.document["hud"]["widgets"][1]["target"] ==
                "controller" &&
            legacy == legacyBefore &&
            !legacy.contains("schemaVersion"),
        "Legacy preference migration lost fields or mutated its source");

    const ofJson current = {
        {"schemaVersion", 1},
        {"browser", {
            {"treeWidthRatio", 0.20},
            {"selection", {
                {"category", "SDK Inspection (Read-only)"},
                {"subcategory", ""},
                {"asset", "Signal Bloom"},
            }},
            {"selectedColumn", "midi"},
            {"visibleColumns", {{"name", true}, {"midi", true}}},
            {"collapsedCategories", ofJson::array()},
            {"collapsedParameterSections", ofJson::array()},
        }},
        {"hud", {
            {"visible", false},
            {"layoutTarget", "projector"},
            {"stateMigrated", true},
            {"widgets", ofJson::array()},
        }},
        {"hotkeys", {
            {"bindings", ofJson::array({
                {{"id", "menu.console"}, {"key", 96}},
                {{"id", "app.quit"}, {"key", 0}},
            })}
        }},
        {"packages", {
            {"activations", ofJson::array({
                {
                    {"packageId", "examples.signal_bloom"},
                    {"enabled", true},
                    {"selectedPreset", {
                        {"bankId", "performance"},
                        {"presetId", "default"},
                    }},
                }
            })}
        }},
        {"mappings", {{"activeBank", "home"}}},
    };
    const auto accepted = normalizePreferencesDocument(current);
    require(
        accepted.ok &&
            accepted.kind == PreferencesDocumentKind::CurrentV1 &&
            accepted.sourceVersion == 1 &&
            !accepted.migratedInMemory &&
            accepted.document == current,
        "Canonical preferences v1 was not accepted unchanged");

    for (const auto& invalid : std::vector<ofJson>{
             ofJson::array(),
             ofJson{{"schemaVersion", "1"}},
             ofJson{{"schemaVersion", 0}},
             ofJson{{"schemaVersion", 1}, {"machine", ofJson::object()}},
             ofJson{
                 {"schemaVersion", 1},
                 {"browser", {{"treeWidthRatio", 0.01}}},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"browser", {{"visibleColumns", {{"name", false}}}}},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"hud", {{"layoutTarget", "both"}}},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"hotkeys", {{"bindings", ofJson::array({
                     {{"id", "menu.console"}, {"key", 1}},
                     {{"id", "menu.console"}, {"key", 2}},
                 })}}},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"packages", {{"activations", ofJson::array({
                     {{"packageId", "bad package"}, {"enabled", true}},
                 })}}},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"mappings", {{"activeBank", "bad bank"}}},
             },
         }) {
        const auto rejected = normalizePreferencesDocument(invalid);
        require(
            !rejected.ok && !rejected.error.empty(),
            "Invalid preferences document was accepted");
    }
    const auto future = normalizePreferencesDocument({
        {"schemaVersion", 2},
        {"browser", ofJson::object()},
    });
    require(
        !future.ok &&
            future.errorCode ==
                PreferencesDocumentError::UnsupportedFutureVersion,
        "Future preferences version was not distinguishable from corruption");

    PreferencesPublisher publisher;
    std::string error;
    require(
        publisher.adoptInitial(current, &error),
        "Could not seed preferences publisher: " + error);
    std::vector<ofJson> persisted;
    std::vector<ofJson> adopted;
    publisher.setPersistCallback([&](const ofJson& snapshot) {
        persisted.push_back(snapshot);
        return true;
    });
    publisher.setAdoptCallback([&](const ofJson& snapshot) {
        adopted.push_back(snapshot);
        return snapshot["mappings"].value(
                   "activeBank",
                   std::string()) != "reject";
    });

    const auto sectionPublished = publisher.publishSection(
        "mappings",
        {{"activeBank", "performance"}});
    require(
        sectionPublished.ok &&
            publisher.snapshot()["mappings"]["activeBank"] ==
                "performance" &&
            publisher.snapshot()["browser"] == current["browser"] &&
            publisher.snapshot()["hotkeys"] == current["hotkeys"],
        "Section publication did not preserve unrelated preferences");

    const ofJson beforeFailure = publisher.snapshot();
    const auto rejectedPublication = publisher.publishSection(
        "mappings",
        {{"activeBank", "reject"}});
    require(
        !rejectedPublication.ok &&
            rejectedPublication.rollbackSucceeded &&
            publisher.snapshot() == beforeFailure &&
            persisted.size() == 3 &&
            persisted.back() == beforeFailure &&
            adopted.size() == 3 &&
            adopted.back() == beforeFailure,
        "Failed preference adoption did not restore persisted and live state");

    publisher.setPersistCallback([](const ofJson&) {
        return false;
    });
    const auto persistFailure = publisher.publishSection(
        "mappings",
        {{"activeBank", "home"}});
    require(
        !persistFailure.ok &&
            publisher.snapshot() == beforeFailure,
        "Failed preference persistence changed the authoritative snapshot");

    PreferencesPublisher throwingPublisher;
    require(
        throwingPublisher.adoptInitial(current),
        "Could not seed throwing preferences publisher");
    throwingPublisher.setPersistCallback(
        [](const ofJson&) -> bool {
            throw std::runtime_error("injected persistence exception");
        });
    bool escaped = false;
    try {
        const auto result = throwingPublisher.publishSection(
            "mappings",
            {{"activeBank", "performance"}});
        require(
            !result.ok &&
                throwingPublisher.snapshot() == current,
            "Thrown persistence callback changed preferences");
    } catch (...) {
        escaped = true;
    }
    require(
        !escaped,
        "Persistence callback exception escaped publication");

    int adoptionCalls = 0;
    throwingPublisher.setPersistCallback(
        [](const ofJson&) { return true; });
    throwingPublisher.setAdoptCallback(
        [&](const ofJson&) -> bool {
            ++adoptionCalls;
            throw std::runtime_error("injected adoption exception");
        });
    try {
        const auto result = throwingPublisher.publishSection(
            "mappings",
            {{"activeBank", "performance"}});
        require(
            !result.ok &&
                !result.rollbackSucceeded &&
                adoptionCalls == 2 &&
                throwingPublisher.snapshot() == current,
            "Thrown adoption/rollback callback was not contained");
    } catch (...) {
        escaped = true;
    }
    require(
        !escaped,
        "Adoption callback exception escaped publication");

    const auto unknownSection =
        publisher.publishSection("machine", ofJson::object());
    require(
        !unknownSection.ok &&
            publisher.snapshot() == beforeFailure,
        "Unknown preference section was published");
    return true;
}

bool RunBankDefinitionsDocumentScenario() {
    using synaptome::state::BankDefinitionsDocumentError;
    using synaptome::state::BankDefinitionsPublisher;
    using synaptome::state::validateBankDefinitionsDocument;
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };
    const ofJson current = {
        {"schemaVersion", 1},
        {"globalBanks", ofJson::array({
            {
                {"id", "home"},
                {"label", "Home"},
                {"controls", ofJson::array({
                    {
                        {"id", "speed"},
                        {"label", "Speed"},
                        {"target", "globals.speed"},
                        {"softTakeover", true},
                    }
                })},
            },
            {
                {"id", "performance"},
                {"parent", "home"},
                {"controls", ofJson::array({
                    {
                        {"id", "masterFx"},
                        {"target", "fx.master"},
                    }
                })},
            },
        })}
    };
    const ofJson before = current;
    const auto accepted =
        validateBankDefinitionsDocument(current);
    require(
        accepted.ok &&
            accepted.sourceVersion == 1 &&
            accepted.document == current &&
            current == before,
        "Canonical bank definitions were not accepted unchanged");

    for (const auto& invalid : std::vector<ofJson>{
             ofJson::array(),
             ofJson{{"schemaVersion", 1}},
             ofJson{
                 {"schemaVersion", 0},
                 {"globalBanks", ofJson::array()},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"globalBanks", ofJson::array()},
                 {"activeBank", "home"},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"globalBanks", ofJson::array({
                     {{"id", "home"}},
                     {{"id", "home"}},
                 })},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"globalBanks", ofJson::array({
                     {{"id", "child"}, {"parent", "missing"}},
                 })},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"globalBanks", ofJson::array({
                     {{"id", "a"}, {"parent", "b"}},
                     {{"id", "b"}, {"parent", "a"}},
                 })},
             },
             ofJson{
                 {"schemaVersion", 1},
                 {"globalBanks", ofJson::array({
                     {
                         {"id", "home"},
                         {"controls", ofJson::array({
                             {{"id", "orphan"}},
                         })},
                     },
                 })},
             },
         }) {
        const auto rejected =
            validateBankDefinitionsDocument(invalid);
        require(
            !rejected.ok && !rejected.error.empty(),
            "Invalid bank-definitions document was accepted");
    }
    const auto future =
        validateBankDefinitionsDocument({
            {"schemaVersion", 2},
            {"globalBanks", ofJson::array()},
        });
    require(
        !future.ok &&
            future.errorCode ==
                BankDefinitionsDocumentError::
                    UnsupportedFutureVersion,
        "Future bank-definitions version was not distinguishable");

    BankDefinitionsPublisher publisher;
    require(
        publisher.adoptInitial(current),
        "Could not seed bank-definitions publisher");
    std::vector<ofJson> persisted;
    std::vector<ofJson> adopted;
    publisher.setPersistCallback([&](const ofJson& value) {
        persisted.push_back(value);
        return true;
    });
    publisher.setAdoptCallback([&](const ofJson& value) {
        adopted.push_back(value);
        return value["globalBanks"].size() != 1;
    });
    ofJson candidate = current;
    candidate["globalBanks"][0]["label"] = "Operator Home";
    require(
        publisher.publish(candidate).ok &&
            publisher.snapshot() == candidate,
        "Valid bank definitions were not published");

    const ofJson published = publisher.snapshot();
    ofJson rejectedCandidate = current;
    rejectedCandidate["globalBanks"].erase(
        rejectedCandidate["globalBanks"].begin() + 1);
    const auto rolledBack =
        publisher.publish(rejectedCandidate);
    require(
        !rolledBack.ok &&
            rolledBack.rollbackSucceeded &&
            publisher.snapshot() == published &&
            persisted.size() == 3 &&
            persisted.back() == published &&
            adopted.size() == 3 &&
            adopted.back() == published,
        "Failed bank adoption did not restore persisted/live definitions");

    BankDefinitionsPublisher throwingPublisher;
    require(
        throwingPublisher.adoptInitial(current),
        "Could not seed throwing bank publisher");
    throwingPublisher.setPersistCallback(
        [](const ofJson&) -> bool {
            throw std::runtime_error("injected write failure");
        });
    bool escaped = false;
    try {
        const auto result =
            throwingPublisher.publish(candidate);
        require(
            !result.ok &&
                throwingPublisher.snapshot() == current,
            "Thrown persistence changed bank definitions");
    } catch (...) {
        escaped = true;
    }
    require(
        !escaped,
        "Bank persistence exception escaped publication");
    return true;
}

bool RunMappingSnapshotRoundTripScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    float depth = 0.35f;
    bool enabled = false;
    std::string timingMode = "halftime";
    ParameterRegistry registry;
    ParameterRegistry::Descriptor depthMeta;
    depthMeta.id = "test.scene.depth";
    depthMeta.label = "Depth";
    depthMeta.group = "Scene Test";
    depthMeta.range.min = -2.0f;
    depthMeta.range.max = 3.0f;
    registry.addFloat(depthMeta.id, &depth, depth, depthMeta);
    ParameterRegistry::Descriptor enabledMeta;
    enabledMeta.id = "test.scene.enabled";
    enabledMeta.label = "Enabled";
    enabledMeta.group = "Scene Test";
    registry.addBool(enabledMeta.id, &enabled, enabled, enabledMeta);
    ParameterRegistry::Descriptor timingMeta;
    timingMeta.id = "test.scene.timing";
    timingMeta.label = "Timing";
    timingMeta.group = "Scene Test";
    registry.addString(timingMeta.id, &timingMode, timingMode, timingMeta);

    MidiRouter router;
    router.bindFloat("test.scene.depth",
                     &depth,
                     -2.0f,
                     3.0f,
                     true,
                     0.25f,
                     "performance",
                     "depth-knob");
    router.bindBool("test.scene.enabled",
                    &enabled,
                    MidiRouter::BoolMode::Toggle,
                    "performance",
                    "enable-button");
    router.setTestPortList({});

    const ofJson savedSceneMappings = {
        {"cc", ofJson::array({
            {
                {"num", 74},
                {"channel", 3},
                {"target", "test.scene.depth"},
                {"bank", "performance"},
                {"control", "depth-knob"},
                {"device", "show-controller"},
                {"column", "console.column.2"},
                {"slot", "encoder.4"},
                {"out", ofJson::array({-1.5f, 2.75f})},
                {"snapInt", true},
                {"step", 0.25f}
            }
        })},
        {"buttons", ofJson::array({
            {
                {"num", 42},
                {"channel", 9},
                {"type", "toggle"},
                {"target", "test.scene.enabled"},
                {"bank", "performance"},
                {"control", "enable-button"},
                {"device", "show-controller"},
                {"column", "console.column.2"},
                {"slot", "pad.2"},
                {"setValue", 0.625f}
            }
        })},
        {"oscSources", ofJson::array({
            {
                {"pattern", "/sensor/deck/*/intensity"},
                {"in", ofJson::array({0.1f, 0.9f})},
                {"out", ofJson::array({-0.75f, 1.8f})},
                {"smooth", 0.63f},
                {"deadband", 0.075f},
                {"blend", "additive"},
                {"relative", false}
            }
        })},
        {"osc", ofJson::array({
            {
                {"pattern", "/sensor/deck/*/intensity"},
                {"target", "test.scene.depth"},
                {"bank", "performance"},
                {"control", "deck-intensity"}
            }
        })}
    };

    require(router.importMappingSnapshot(savedSceneMappings, true),
            "Complete scene mapping snapshot was rejected");
    const ofJson canonicalBaseline = router.exportMappingSnapshot();
    require(
        canonicalBaseline["schemaVersion"] == 1 &&
            canonicalBaseline.contains("cc") &&
            canonicalBaseline.contains("buttons") &&
            canonicalBaseline.contains("oscSources") &&
            canonicalBaseline.contains("osc"),
        "Canonical mapping export did not emit the complete v1 shape");
    ofJson combinedSceneState = {
        {"parameters", {
            {"floats", {{"test.scene.depth", registry.getFloatBase("test.scene.depth")}}},
            {"bools", {{"test.scene.enabled", registry.getBoolBase("test.scene.enabled")}}},
            {"strings", {{"test.scene.timing", registry.getStringBase("test.scene.timing")}}}
        }},
        {"mappings", {{"router", canonicalBaseline}}}
    };
    require(canonicalBaseline.contains("cc") && canonicalBaseline["cc"].size() == 1,
            "CC mapping was not captured in the canonical scene snapshot");
    require(canonicalBaseline.contains("buttons") &&
                canonicalBaseline["buttons"].size() == 1,
            "Button mapping was not captured in the canonical scene snapshot");
    require(canonicalBaseline.contains("osc") && canonicalBaseline["osc"].size() == 1 &&
                canonicalBaseline.contains("oscSources") &&
                canonicalBaseline["oscSources"].size() == 1,
            "OSC route and source profile were not captured together");

    const auto& cc = router.getCcMaps().front();
    require(cc.cc == 74 && cc.channel == 3 && cc.target == "test.scene.depth",
            "CC identity did not survive import");
    require(cc.bankId == "performance" && cc.controlId == "depth-knob" &&
                cc.deviceId == "show-controller" &&
                cc.columnId == "console.column.2" && cc.slotId == "encoder.4",
            "CC control metadata did not survive import");
    require(std::fabs(cc.outMin - -1.5f) < 0.0001f &&
                std::fabs(cc.outMax - 2.75f) < 0.0001f &&
                cc.snapInt && std::fabs(cc.step - 0.25f) < 0.0001f,
            "CC output range or stepping metadata did not survive import");

    const auto& button = router.getBtnMaps().front();
    require(button.num == 42 && button.channel == 9 &&
                button.type == "toggle" &&
                std::fabs(button.setValue - 0.625f) < 0.0001f,
            "Button mapping behavior did not survive import");
    require(button.target == "test.scene.enabled" &&
                button.bankId == "performance" &&
                button.controlId == "enable-button" &&
                button.deviceId == "show-controller" &&
                button.columnId == "console.column.2" &&
                button.slotId == "pad.2",
            "Button mapping metadata did not survive import");

    const auto* profile =
        router.findOscSourceProfile("/sensor/deck/*/intensity");
    require(profile != nullptr, "OSC source profile was not restored");
    require(std::fabs(profile->inMin - 0.1f) < 0.0001f &&
                std::fabs(profile->inMax - 0.9f) < 0.0001f &&
                std::fabs(profile->outMin - -0.75f) < 0.0001f &&
                std::fabs(profile->outMax - 1.8f) < 0.0001f &&
                std::fabs(profile->smooth - 0.63f) < 0.0001f &&
                std::fabs(profile->deadband - 0.075f) < 0.0001f &&
                profile->blend == modifier::BlendMode::kAdditive &&
                !profile->relativeToBase,
            "OSC range, smoothing, deadband, blend, or relative mode was lost");
    const auto* osc = router.findOscMap("test.scene.depth");
    require(osc != nullptr && osc->pattern == "/sensor/deck/*/intensity" &&
                osc->bankId == "performance" &&
                osc->controlId == "deck-intensity",
            "OSC route identity or control metadata was lost");

    router.setActiveBank("performance");
    const ofJson beforeFutureImport =
        router.exportMappingSnapshot();
    ofJson futureSnapshot = beforeFutureImport;
    futureSnapshot["schemaVersion"] = 2;
    require(
        !router.importMappingSnapshot(futureSnapshot, true) &&
            router.exportMappingSnapshot() ==
                beforeFutureImport &&
            router.activeBank() == "performance",
        "Future mapping import changed routes or the active bank");

    const ofJson publicInterchange = {
        {"version", 1},
        {"bank", "performance"},
        {"mappings", ofJson::array()},
    };
    require(
        !router.importMappingSnapshot(
            publicInterchange,
            true) &&
            router.exportMappingSnapshot() ==
                beforeFutureImport,
        "Public MIDI interchange was mistaken for a runtime snapshot");

    const ofJson explicitEmptyV1 = {
        {"schemaVersion", 1},
        {"cc", ofJson::array()},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array()},
        {"osc", ofJson::array()},
    };
    require(
        router.importMappingSnapshot(
            explicitEmptyV1,
            true) &&
            router.getCcMaps().empty() &&
            router.getBtnMaps().empty() &&
            router.getOscMaps().empty() &&
            router.getOscSourceProfiles().empty() &&
            router.activeBank() == "performance",
        "Present empty mapping-bank v1 was not authoritative");
    require(
        router.importMappingSnapshot(
            canonicalBaseline,
            true),
        "Canonical mappings could not be restored after explicit clear");
    const ofJson beforePublicationFailure =
        router.exportMappingSnapshot();
    ofJson publicationCandidate =
        beforePublicationFailure;
    publicationCandidate["cc"][0]["num"] = 75;
    router.onOscRoutesChanged = [] {
        throw std::runtime_error(
            "injected route publication failure");
    };
    bool publicationAccepted = false;
    try {
        publicationAccepted =
            router.importMappingSnapshot(
                publicationCandidate,
                true);
    } catch (...) {
        throw std::runtime_error(
            "Route publication failure escaped import");
    }
    router.onOscRoutesChanged = nullptr;
    require(
        !publicationAccepted &&
            router.exportMappingSnapshot() ==
                beforePublicationFailure &&
            router.activeBank() == "performance",
        "Route publication failure did not restore prior mappings");

    const ofJson mutatedMappings = {
        {"cc", ofJson::array({
            {
                {"num", 7},
                {"target", "test.scene.depth"},
                {"out", ofJson::array({0.0f, 1.0f})}
            }
        })}
    };
    require(router.importMappingSnapshot(mutatedMappings, true),
            "Mutated mapping state was rejected");
    registry.setFloatBase("test.scene.depth", 2.2f, true);
    registry.setBoolBase("test.scene.enabled", true, true);
    registry.setStringBase("test.scene.timing", "doubletime", true);
    require(router.getCcMaps().size() == 1 && router.getCcMaps().front().cc == 7 &&
                router.getBtnMaps().empty() && router.getOscMaps().empty(),
            "Replace import did not remove the prior scene mapping state");
    const auto& savedParameters = combinedSceneState["parameters"];
    registry.setFloatBase(
        "test.scene.depth",
        savedParameters["floats"]["test.scene.depth"].get<float>(),
        true);
    registry.setBoolBase(
        "test.scene.enabled",
        savedParameters["bools"]["test.scene.enabled"].get<bool>(),
        true);
    registry.setStringBase(
        "test.scene.timing",
        savedParameters["strings"]["test.scene.timing"].get<std::string>(),
        true);
    require(router.importMappingSnapshot(
                combinedSceneState["mappings"]["router"], true),
            "Saved mapping state could not be restored after mutation");
    require(std::fabs(depth - 0.35f) < 0.0001f && !enabled &&
                timingMode == "halftime",
            "Combined scene parameter state did not restore with its mappings");
    require(router.exportMappingSnapshot() == canonicalBaseline,
            "Save, mutate, and restore did not reproduce the exact mapping snapshot");

    const ofJson additionalMapping = {
        {"cc", ofJson::array({
            {
                {"num", 91},
                {"target", "test.scene.secondary"},
                {"out", ofJson::array({0.2f, 0.8f})}
            }
        })}
    };
    require(router.importMappingSnapshot(additionalMapping, false),
            "Non-replacing mapping import was rejected");
    require(router.getCcMaps().size() == 2,
            "Non-replacing import did not preserve the active scene mapping");
    require(std::any_of(router.getCcMaps().begin(),
                        router.getCcMaps().end(),
                        [](const MidiRouter::CcMap& map) {
                            return map.target == "test.scene.depth" && map.cc == 74;
                        }),
            "Non-replacing import erased the prior CC mapping");
    require(router.importMappingSnapshot(canonicalBaseline, true),
            "Replacing import could not restore the canonical mapping set");
    require(router.getCcMaps().size() == 1 &&
                router.getCcMaps().front().target == "test.scene.depth",
            "Replacing import retained mappings absent from the scene snapshot");

    const ofJson beforeMalformedImport = router.exportMappingSnapshot();
    const ofJson malformedSnapshot = {
        {"cc", "not-an-array"},
        {"buttons", ofJson::array({
            {{"num", "not-a-number"}, {"target", "test.scene.enabled"}}
        })}
    };
    bool malformedAccepted = false;
    try {
        malformedAccepted = router.importMappingSnapshot(malformedSnapshot, true);
    } catch (...) {
        throw std::runtime_error(
            "Malformed mapping snapshot escaped validation as an exception");
    }
    require(!malformedAccepted,
            "Malformed mapping snapshot was reported as successfully imported");
    require(router.exportMappingSnapshot() == beforeMalformedImport,
            "Malformed mapping import damaged the last-known-good mapping set");

    float restartedDepth = 0.0f;
    bool restartedEnabled = false;
    MidiRouter restartedRouter;
    restartedRouter.bindFloat("test.scene.depth",
                              &restartedDepth,
                              -2.0f,
                              3.0f,
                              true,
                              0.25f,
                              "performance",
                              "depth-knob");
    restartedRouter.bindBool("test.scene.enabled",
                             &restartedEnabled,
                             MidiRouter::BoolMode::Toggle,
                             "performance",
                             "enable-button");
    restartedRouter.setTestPortList({});
    require(restartedRouter.availableInputPorts().empty(),
            "Missing-device test unexpectedly exposed a MIDI port");
    require(restartedRouter.importMappingSnapshot(canonicalBaseline, true),
            "Fresh router could not restore mappings while the device was missing");
    require(restartedRouter.exportMappingSnapshot() == canonicalBaseline,
            "Unavailable MIDI hardware caused saved mappings to be discarded");
    restartedRouter.setActiveBank("performance");

    ofxMidiMessage ccMessage;
    ccMessage.status = MIDI_CONTROL_CHANGE;
    ccMessage.control = 74;
    ccMessage.channel = 3;
    ccMessage.value = 45;
    restartedRouter.newMidiMessage(ccMessage);
    ccMessage.value = 127;
    restartedRouter.newMidiMessage(ccMessage);
    require(std::fabs(restartedDepth - 3.0f) < 0.0001f,
            "Restored CC mapping did not become usable after soft-takeover catch");

    ofxMidiMessage buttonMessage;
    buttonMessage.status = MIDI_NOTE_ON;
    buttonMessage.pitch = 42;
    buttonMessage.channel = 9;
    buttonMessage.velocity = 127;
    restartedRouter.newMidiMessage(buttonMessage);
    require(restartedEnabled,
            "Restored button mapping did not become usable after restart");

    router.clearTestPortList();
    restartedRouter.clearTestPortList();
    return true;
}

bool RunMappingStoreRecoveryScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() /
        "synaptome_mapping_bank_v1";
    std::error_code cleanupError;
    std::filesystem::remove_all(tempRoot, cleanupError);
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path mappingPath =
        tempRoot / "midi-map.json";
    const std::filesystem::path backupPath =
        tempRoot / "midi-map.json.bak";

    float writerValue = 0.0f;
    MidiRouter writer;
    writer.bindFloat(
        "test.mapping.store",
        &writerValue,
        0.0f,
        1.0f);
    writer.setTestPortList({});
    const ofJson legacy = {
        {"cc", ofJson::array({
            {
                {"num", 23},
                {"channel", 2},
                {"target", "test.mapping.store"},
                {"out", ofJson::array({0.0f, 1.0f})},
            }
        })},
    };
    require(
        writer.importMappingSnapshot(legacy, true) &&
            writer.save(mappingPath.string()),
        "Canonical mapping-bank save failed");
    const ofJson firstSave =
        ofLoadJson(mappingPath.string());
    require(
        firstSave["schemaVersion"] == 1 &&
            firstSave.contains("cc") &&
            firstSave.contains("buttons") &&
            firstSave.contains("oscSources") &&
            firstSave.contains("osc"),
        "Standalone writer did not emit canonical mapping-bank v1");

    float loadedValue = 0.0f;
    MidiRouter loaded;
    loaded.bindFloat(
        "test.mapping.store",
        &loadedValue,
        0.0f,
        1.0f);
    loaded.setTestPortList({});
    require(
        loaded.load(mappingPath.string()) &&
            loaded.exportMappingSnapshot() ==
                writer.exportMappingSnapshot() &&
            ofLoadJson(mappingPath.string()) == firstSave,
        "Canonical mapping-bank reload changed routes or rewrote its source");

    const ofJson secondRoutes = {
        {"schemaVersion", 1},
        {"cc", ofJson::array({
            {
                {"num", 64},
                {"target", "test.mapping.store"},
                {"out", ofJson::array({0.0f, 1.0f})},
            }
        })},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array()},
        {"osc", ofJson::array()},
    };
    require(
        writer.importMappingSnapshot(
            secondRoutes,
            true) &&
            writer.save(mappingPath.string()) &&
            std::filesystem::exists(backupPath),
        "Second mapping save did not preserve a recovery copy");

    const ofJson liveBeforeFuture =
        loaded.exportMappingSnapshot();
    ofJson futurePrimary = secondRoutes;
    futurePrimary["schemaVersion"] = 2;
    futurePrimary["mappings"] = ofJson::object();
    require(
        ofSavePrettyJson(
            mappingPath.string(),
            futurePrimary) &&
            ofSavePrettyJson(
                backupPath.string(),
                firstSave),
        "Could not prepare future-version recovery fixture");
    require(
        !loaded.load(mappingPath.string()) &&
            loaded.exportMappingSnapshot() ==
                liveBeforeFuture &&
            ofLoadJson(mappingPath.string()) ==
                futurePrimary,
        "Future primary downgraded through backup or changed live routes");

    const ofJson malformedPrimary = {
        {"schemaVersion", 1},
        {"cc", "not-an-array"},
    };
    const ofJson malformedBackup = {
        {"schemaVersion", 1},
        {"buttons", "not-an-array"},
    };
    require(
        ofSavePrettyJson(
            mappingPath.string(),
            malformedPrimary) &&
            ofSavePrettyJson(
                backupPath.string(),
                malformedBackup),
        "Could not prepare malformed recovery fixtures");
    require(
        !loaded.load(mappingPath.string()) &&
            loaded.exportMappingSnapshot() ==
                liveBeforeFuture,
        "Malformed primary and backup damaged live mappings");

    writer.clearTestPortList();
    loaded.clearTestPortList();
    std::filesystem::remove_all(tempRoot, cleanupError);
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

    // Explicitly open Effects for this within-session persistence scenario. Browser
    // startup itself is intentionally collapsed and is covered separately below.
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

    hub.setCategoryExpanded("Effects", true);
    hub.tableModel_.dirty = true;
    hub.rebuildView();
    int effectsNodeIndex = -1;
    for (std::size_t i = 0; i < hub.tableModel_.tree.size(); ++i) {
        const auto& node = hub.tableModel_.tree[i];
        if (node.categoryName == "Effects" && node.depth == 1) {
            effectsNodeIndex = static_cast<int>(i);
            break;
        }
    }
    if (effectsNodeIndex < 0) {
        throw std::runtime_error("Expanded Effects category did not expose a child node");
    }
    hub.applyTreeSelection(effectsNodeIndex, false);

    std::vector<std::string> sectionKeys;
    for (const auto& item : hub.activeGridItems()) {
        if (item.sectionHeader && !item.sectionKey.empty()) {
            sectionKeys.push_back(item.sectionKey);
        }
    }
    for (const auto& sectionKey : sectionKeys) {
        hub.setParameterSectionExpanded(sectionKey, true);
    }
    hub.invalidateRowCache();
    const auto& activeRows = hub.activeRowIndices();
    hub.selectedGridSectionKey_.clear();
    hub.selectedRow_ = activeRows.empty() ? -1 : activeRows.front();
    hub.clampSelection();

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

    hub.setCategoryExpanded("Effects", false);
    hub.tableModel_.dirty = true;
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

    hub.setCategoryExpanded("Effects", true);
    hub.tableModel_.dirty = true;
    auto expandedAgain = hub.snapshotViewport(640.0f, 180.0f);
    if (expandedAgain.treeNodeCount != baseline.treeNodeCount) {
        throw std::runtime_error("Expanding a category after push/pop did not restore the tree nodes");
    }

    hub.onExit(controller);
    return true;
}

bool RunCollapsedBrowserStartupScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    ControlMappingHubState hub;
    ParameterRegistry registry;
    MidiRouter router;
    LayerLibrary library;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&router);
    hub.setLayerLibrary(&library);

    float generativeIntensity = 0.5f;
    float generativeSpeed = 1.0f;
    bool effectEnabled = true;

    ParameterRegistry::Descriptor intensityMeta;
    intensityMeta.group = "Generative";
    intensityMeta.label = "Appearance: Intensity";
    registry.addFloat(
        "generative.startupFixture.intensity",
        &generativeIntensity,
        generativeIntensity,
        intensityMeta);

    ParameterRegistry::Descriptor speedMeta;
    speedMeta.group = "Generative";
    speedMeta.label = "Motion: Speed";
    registry.addFloat(
        "generative.startupFixture.speed",
        &generativeSpeed,
        generativeSpeed,
        speedMeta);

    ParameterRegistry::Descriptor enabledMeta;
    enabledMeta.group = "Post FX";
    enabledMeta.label = "Behavior: Enabled";
    registry.addBool(
        "effects.startupFixture.enabled",
        &effectEnabled,
        effectEnabled,
        enabledMeta);

    hub.setSavedSceneListCallback([] {
        ControlMappingHubState::SavedSceneInfo scene;
        scene.id = "startup-fixture";
        scene.label = "Startup Fixture";
        return std::vector<ControlMappingHubState::SavedSceneInfo>{scene};
    });

    MenuController controller;
    hub.onEnter(controller);
    hub.view();

    require(!hub.tableModel_.categories.empty(),
            "Fresh browser did not build any categories");
    require(hub.tableModel_.categories.front().name == "Scenes",
            "Scenes was not the first category on fresh browser startup");

    int expandableCategoryCount = 0;
    for (const auto& node : hub.tableModel_.tree) {
        if (node.depth != 0 || !node.expandable) {
            continue;
        }
        ++expandableCategoryCount;
        require(!node.expanded,
                "Expandable category opened on fresh browser startup: " +
                    node.categoryName);
    }
    require(expandableCategoryCount > 0,
            "Startup fixture did not exercise an expandable category");

    int parameterSectionCount = 0;
    auto inspectParameterScope =
        [&](int categoryIndex, int subcategoryIndex, int assetGroupIndex) {
        ControlMappingHubState::TreeNode scopeNode;
        scopeNode.categoryIndex = categoryIndex;
        scopeNode.subcategoryIndex = subcategoryIndex;
        scopeNode.assetGroupIndex = assetGroupIndex;
        hub.tableModel_.tree.push_back(scopeNode);
        hub.selectedTreeNodeIndex_ =
            static_cast<int>(hub.tableModel_.tree.size() - 1);
        hub.selectedRow_ = -1;
        hub.selectedGridSectionKey_.clear();
        hub.invalidateRowCache();
        const auto& items = hub.activeGridItems();
        for (const auto& item : items) {
            require(item.sectionHeader,
                    "Parameter row was visible beneath a collapsed startup section");
            ++parameterSectionCount;
            require(!item.expanded,
                    "Parameter section opened on fresh browser startup: " +
                        item.sectionName);
        }
        hub.tableModel_.tree.pop_back();
    };
    for (std::size_t categoryIndex = 0;
         categoryIndex < hub.tableModel_.categories.size();
         ++categoryIndex) {
        const auto& category = hub.tableModel_.categories[categoryIndex];
        for (std::size_t subcategoryIndex = 0;
             subcategoryIndex < category.subcategories.size();
             ++subcategoryIndex) {
            const auto& subcategory =
                category.subcategories[subcategoryIndex];
            if (subcategory.assetGroups.empty()) {
                inspectParameterScope(
                    static_cast<int>(categoryIndex),
                    static_cast<int>(subcategoryIndex),
                    -1);
                continue;
            }
            for (std::size_t assetGroupIndex = 0;
                 assetGroupIndex < subcategory.assetGroups.size();
                 ++assetGroupIndex) {
                inspectParameterScope(
                    static_cast<int>(categoryIndex),
                    static_cast<int>(subcategoryIndex),
                    static_cast<int>(assetGroupIndex));
            }
        }
    }
    require(parameterSectionCount > 0,
            "Startup fixture did not exercise a parameter-section scope");

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
    controllerWidget.id = "hud.debug.terminal";
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
    hudMeta.label = "Hotkey Guide";
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
    "label":"Hotkey Guide",
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
    "label":"Hotkey Guide",
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
        hudMeta.label = "Hotkey Guide";
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
        hudMeta.label = "Hotkey Guide";
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
    "label":"Hotkey Guide",
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
    hudMeta.label = "Hotkey Guide";
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

bool RunWindowMonitorPlacementScenario() {
    using window_monitor_placement::Rect;
    const std::vector<Rect> sideBySide = {
        {0, 0, 1920, 1080},
        {1920, 0, 1920, 1080},
    };

    // Reproduces the show bug: the top-left is still on the laptop, but most
    // of the dragged window is already on the projector.
    const Rect mostlyProjector{1750, 120, 1280, 720};
    if (window_monitor_placement::selectMonitorForWindow(
            mostlyProjector, sideBySide) != 1) {
        throw std::runtime_error(
            "Fullscreen monitor selection followed the top-left pixel "
            "instead of the largest window overlap");
    }

    const std::vector<Rect> projectorLeft = {
        {0, 0, 1920, 1080},
        {-1920, 0, 1920, 1080},
    };
    const Rect leftProjectorWindow{-1700, 80, 1280, 720};
    if (window_monitor_placement::selectMonitorForWindow(
            leftProjectorWindow, projectorLeft) != 1) {
        throw std::runtime_error(
            "Fullscreen monitor selection failed for a negative-coordinate projector");
    }

    const Rect disconnectedPosition{5000, 5000, 1280, 720};
    if (window_monitor_placement::selectMonitorForWindow(
            disconnectedPosition, sideBySide) != 1) {
        throw std::runtime_error(
            "Off-screen window did not select the nearest available monitor");
    }
    return true;
}

namespace {
class OfflineElementCreatorProbe final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor meta;
        meta.label = "Probe: Amount";
        meta.group = "Probe";
        meta.description = "BrowserFlow probe for offline element hydration";
        meta.range.min = 0.0f;
        meta.range.max = 1.0f;
        meta.range.step = 0.01f;
        registry.addFloat(
            registryPrefix() + ".amount", &amount_, amount_, meta);
    }

    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float amount_ = 0.25f;
};

void verifyOfflineElementCreatorHydration() {
    const auto uniqueSuffix = std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const auto layersDir =
        std::filesystem::temp_directory_path() /
        ("cmh_offline_element_creator_" + uniqueSuffix);
    std::filesystem::create_directories(layersDir);
    const auto assetPath = layersDir / "tests.offline.probe.json";
    std::ofstream out(assetPath, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to create offline creator probe asset");
    }
    out << R"JSON({
    "id":"tests.offline.probe",
    "label":"Offline Creator Probe",
    "category":"Tests",
    "type":"tests.offline.probe",
    "registryPrefix":"tests.offline.probe"
})JSON";
    out.close();

    LayerLibrary library;
    if (!library.reload(layersDir.string()) ||
        !library.find("tests.offline.probe")) {
        throw std::runtime_error("Failed to load offline creator probe asset");
    }

    ParameterRegistry liveRegistry;
    ControlMappingHubState hydratedHub;
    hydratedHub.setParameterRegistry(&liveRegistry);
    hydratedHub.setLayerLibrary(&library);
    int creatorInvocations = 0;
    hydratedHub.setOfflineElementCreator(
        [&creatorInvocations](const std::string& type) -> std::unique_ptr<Layer> {
            ++creatorInvocations;
            if (type == "tests.offline.probe") {
                return std::make_unique<OfflineElementCreatorProbe>();
            }
            return nullptr;
        });
    hydratedHub.rebuildModel();

    if (creatorInvocations != 1) {
        throw std::runtime_error(
            "Offline element creator was not invoked exactly once for the probe");
    }
    if (hydratedHub.offlineLayers_.size() != 1 ||
        !hydratedHub.offlineRegistry_.findFloat("tests.offline.probe.amount")) {
        throw std::runtime_error(
            "Offline element creator did not hydrate the probe layer registry");
    }
    const auto hydratedRow = std::find_if(
        hydratedHub.tableModel_.rows.begin(),
        hydratedHub.tableModel_.rows.end(),
        [](const ControlMappingHubState::ParameterRow& row) {
            return row.id == "tests.offline.probe.amount" &&
                   row.offline && row.isFloat && row.floatParam;
        });
    if (hydratedRow == hydratedHub.tableModel_.rows.end()) {
        throw std::runtime_error(
            "Hydrated probe parameter did not appear as an offline Browser row");
    }

    ParameterRegistry unhydratedRegistry;
    ControlMappingHubState unhydratedHub;
    unhydratedHub.setParameterRegistry(&unhydratedRegistry);
    unhydratedHub.setLayerLibrary(&library);
    unhydratedHub.rebuildModel();
    const auto unexpectedRow = std::find_if(
        unhydratedHub.tableModel_.rows.begin(),
        unhydratedHub.tableModel_.rows.end(),
        [](const ControlMappingHubState::ParameterRow& row) {
            return row.id == "tests.offline.probe.amount";
        });
    if (!unhydratedHub.offlineLayers_.empty() ||
        !unhydratedHub.offlineRegistry_.floats().empty() ||
        !unhydratedHub.offlineRegistry_.bools().empty() ||
        !unhydratedHub.offlineRegistry_.strings().empty() ||
        unexpectedRow != unhydratedHub.tableModel_.rows.end()) {
        throw std::runtime_error(
            "Hub without an offline creator unexpectedly hydrated the probe");
    }
}
}

bool RunLayerPackageReadOnlyInspectionScenario() {
    verifyOfflineElementCreatorHydration();

    ParameterRegistry registry;
    OptionProviderRegistry optionProviders;
    if (!optionProviders.setProvider(
            "transport.bpmMultipliers",
            ofJson::array({
                {{"multiplier", 0.5}, {"label", "Half Time"}},
                {{"multiplier", 1.0}, {"label", "Normal"}},
                {{"multiplier", 2.0}, {"label", "Double Time"}}
            }))) {
        throw std::runtime_error("Failed to register transport BPM option provider");
    }
    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.setOptionProviderRegistry(&optionProviders);
    const auto payloadPath =
        (synaptome_test_paths::dataRoot() / "config" / "layer-package-inspection.json").string();
    hub.setLayerPackageInspectionPath(payloadPath);
    const std::string inspectionPayloadBefore = hub.layerPackageInspection_.dump();
    hub.rebuildModel();

    int inspectionRows = 0;
    bool foundSignalBloom = false;
    bool foundStaticChoices = false;
    bool foundDynamicSource = false;
    bool foundInertPresetBank = false;
    for (const auto& row : hub.tableModel_.rows) {
        if (!row.isInspectionRow) {
            continue;
        }
        ++inspectionRows;
        if (row.isAsset || row.floatParam || row.boolParam || row.stringParam) {
            throw std::runtime_error("Inspection row acquired a runtime/editable binding");
        }
        if (row.assetKey == "examples.signal_bloom") {
            foundSignalBloom = true;
        }
        if (row.id == "inspection.examples.signal_bloom.scale") {
            foundStaticChoices =
                row.inspectionValue.find("Choices: Compact=0.500") != std::string::npos &&
                row.inspectionValue.find("Default=0.820") != std::string::npos &&
                row.inspectionValue.find("Full=1.000") != std::string::npos;
        }
        if (row.id == "inspection.examples.signal_bloom.bpmMultiplier") {
            foundDynamicSource =
                row.inspectionValue.find("Default: 1.000") != std::string::npos &&
                row.inspectionValue.find(
                    "Options source: transport.bpmMultipliers (resolved; 3 available)") !=
                    std::string::npos &&
                row.inspectionValue.find("Half Time=0.500") != std::string::npos &&
                row.inspectionValue.find("Normal=1.000") != std::string::npos &&
                row.inspectionValue.find("Double Time=2.000") != std::string::npos;
        }
        if (row.id == "inspection.examples.signal_bloom.presetBank.performance") {
            foundInertPresetBank =
                !row.isPackagePresetBankRow &&
                row.inspectionValue.find("Choices: Default, Bright, Calm") !=
                    std::string::npos &&
                row.inspectionValue.find("Activate package to choose") !=
                    std::string::npos;
        }
    }
    if (!foundSignalBloom || inspectionRows < 2) {
        throw std::runtime_error("Read-only inspection payload did not populate Browser rows");
    }
    if (!foundStaticChoices) {
        throw std::runtime_error("Read-only inspection did not render package named choices");
    }
    if (!foundDynamicSource) {
        throw std::runtime_error(
            "Read-only inspection did not resolve the registered option provider");
    }
    if (!foundInertPresetBank) {
        throw std::runtime_error(
            "Inspection-only package preset bank became editable or lost labels");
    }
    if (hub.layerPackageInspection_.dump() != inspectionPayloadBefore) {
        throw std::runtime_error("Read-only inspection rewrote package metadata or stored values");
    }
    if (!hub.offlineLayers_.empty() || !hub.offlineRegistry_.floats().empty() ||
        !hub.offlineRegistry_.bools().empty() || !hub.offlineRegistry_.strings().empty()) {
        throw std::runtime_error("Read-only inspection instantiated or hydrated a layer");
    }

    if (!optionProviders.setProvider(
            "transport.bpmMultipliers",
            ofJson::array({
                {{"multiplier", 0.5}, {"label", "Half Time"}},
                {{"multiplier", 2.0}, {"label", "Double Time"}}
            }))) {
        throw std::runtime_error("Failed to replace transport BPM option provider");
    }
    hub.rebuildModel();
    bool foundPreservedUnavailableDefault = false;
    for (const auto& row : hub.tableModel_.rows) {
        if (row.id == "inspection.examples.signal_bloom.bpmMultiplier") {
            foundPreservedUnavailableDefault =
                row.inspectionValue.find(
                    "Options source: transport.bpmMultipliers (resolved; 2 available)") !=
                    std::string::npos &&
                row.inspectionValue.find("Unavailable default: 1.000 (preserved)") !=
                    std::string::npos;
            break;
        }
    }
    if (!foundPreservedUnavailableDefault) {
        throw std::runtime_error(
            "Provider resolution did not preserve and mark an unavailable default");
    }
    if (hub.layerPackageInspection_.dump() != inspectionPayloadBefore) {
        throw std::runtime_error(
            "Unavailable provider value handling rewrote package metadata or stored values");
    }
    return true;
}

bool RunLabeledParameterSelectionScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const auto appRoot = synaptome_test_paths::appRoot();
    LayerLibrary library;
    require(library.reload((appRoot / "bin" / "data" / "layers").string()),
            "canonical catalog did not load for labeled-parameter scenario");
    const auto activationPath =
        std::filesystem::temp_directory_path() /
        "synaptome-labeled-parameter-selection-test.json";
    const ofJson activation = {
        {"schemaVersion", 1},
        {"enabled", true},
        {"packages", ofJson::array({{
            {"id", "examples.signal_bloom"},
            {"enabled", true},
            {"catalogPath", (appRoot / "bin" / "data" / "layers-optional" /
                             "examples.signal_bloom.json").string()},
            {"presetBank", "performance"},
            {"preset", "default"},
            {"mappingPreset", ""}
        }})}
    };
    {
        std::ofstream out(activationPath, std::ios::trunc);
        out << std::setw(2) << activation << "\n";
    }
    require(library.loadOptInPackages(activationPath.string()),
            "active package catalog did not load for labeled-parameter scenario");
    const auto* entry = library.find("examples.signal_bloom");
    require(entry != nullptr,
            "Signal Bloom was not active for labeled-parameter scenario");

    ParameterRegistry registry;
    ParameterRegistry::Descriptor bpmMeta;
    bpmMeta.label = "Time: BPM Multiplier";
    bpmMeta.group = "Console";
    bpmMeta.range = {0.25f, 8.0f, 0.25f};
    float bpmMultiplier = 1.0f;
    registry.addFloat(
        "console.layer1.bpmMultiplier",
        &bpmMultiplier,
        bpmMultiplier,
        bpmMeta);
    ParameterRegistry::Descriptor scaleMeta;
    scaleMeta.label = "Scale: Size";
    scaleMeta.group = "Console";
    scaleMeta.range = {0.1f, 2.0f, 0.01f};
    float scale = 0.82f;
    registry.addFloat("console.layer1.scale", &scale, scale, scaleMeta);

    OptionProviderRegistry optionProviders;
    require(optionProviders.setProvider(
                "transport.bpmMultipliers",
                ofJson::array({
                    {{"multiplier", 0.5}, {"label", "Half Time"}},
                    {{"multiplier", 1.0}, {"label", "Normal"}},
                    {{"multiplier", 2.0}, {"label", "Double Time"}}
                })),
            "transport choices were not registered");

    MidiRouter midi;
    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.setOptionProviderRegistry(&optionProviders);
    hub.setMidiRouter(&midi);
    hub.setLayerLibrary(&library);
    hub.setConsoleAssetResolver(
        [entry](const std::string& prefix) -> const LayerLibrary::Entry* {
            return prefix == "console.layer1" ? entry : nullptr;
        });
    hub.setConsoleSlotInventoryCallback([entry]() {
        ConsoleLayerInfo info;
        info.index = 1;
        info.assetId = entry->id;
        info.label = entry->label;
        info.active = true;
        return std::vector<ConsoleLayerInfo>{info};
    });
    const auto payloadPath =
        (synaptome_test_paths::dataRoot() / "config" /
         "layer-package-inspection.json").string();
    hub.setLayerPackageInspectionPath(payloadPath);
    const std::string payloadBefore = hub.layerPackageInspection_.dump();
    int selectionEvents = 0;
    hub.setEventCallback([&](const std::string& payload) {
        const auto event = ofJson::parse(payload, nullptr, false);
        if (event.is_object() &&
            event.value("type", std::string()) == "value.option.select") {
            ++selectionEvents;
        }
    });
    hub.rebuildModel();

    const std::string bpmRowId = "console.layer1.bpmMultiplier";
    const std::string scaleRowId = "console.layer1.scale";
    const auto* bpmRow = hub.rowForId(bpmRowId);
    const auto* scaleRow = hub.rowForId(scaleRowId);
    require(bpmRow && bpmRow->hasLabeledValueOptions &&
                bpmRow->labeledValueOptions.size() == 3,
            "runtime-provider choices did not decorate the live BPM row");
    require(scaleRow && scaleRow->hasLabeledValueOptions &&
                scaleRow->labeledValueOptions.size() == 3,
            "static package choices did not decorate the live scale row");
    require(hub.formatValue(*bpmRow) == "Normal",
            "live BPM value did not render its label");
    require(hub.formatValue(*scaleRow) == "Default",
            "live static value did not render its label");

    require(hub.debugSelectLabeledValue(bpmRowId, 2.0),
            "Double Time selection failed");
    require(std::abs(bpmMultiplier - 2.0f) < 0.0001f &&
                std::abs(registry.getFloatBase(bpmRowId) - 2.0f) < 0.0001f &&
                registry.findFloat(bpmRowId)->baseOrigin.kind ==
                    synaptome::state::ParameterBaseOriginKind::
                        OperatorEdit &&
                registry.findFloat(bpmRowId)->baseOrigin.originId ==
                    "browser",
            "labeled BPM selection did not update live and base values");
    hub.rebuildModel();
    bpmRow = hub.rowForId(bpmRowId);
    require(bpmRow && hub.formatValue(*bpmRow) == "Double Time",
            "committed BPM choice did not replay as a label");

    require(hub.debugSelectLabeledValue(scaleRowId, 0.5),
            "Compact static choice selection failed");
    require(std::abs(scale - 0.5f) < 0.0001f &&
                std::abs(registry.getFloatBase(scaleRowId) - 0.5f) < 0.0001f,
            "static labeled choice did not update the live parameter");

    hub.rebuildModel();
    bpmRow = hub.rowForId(bpmRowId);
    require(bpmRow && hub.beginLabeledValuePicker(*bpmRow) &&
                hub.labeledValuePickerVisible(),
            "labeled picker did not open before provider refresh");
    require(optionProviders.setProvider(
                "transport.bpmMultipliers",
                ofJson::array({
                    {{"multiplier", 0.5}, {"label", "Half Time"}},
                    {{"multiplier", 1.0}, {"label", "Normal"}}
                })),
            "transport provider could not remove the current choice");
    hub.rebuildModel();
    require(!hub.labeledValuePickerVisible(),
            "provider revision left a stale labeled picker open");
    bpmRow = hub.rowForId(bpmRowId);
    require(bpmRow && hub.formatValue(*bpmRow) == "2.000 (unavailable)" &&
                std::abs(bpmMultiplier - 2.0f) < 0.0001f,
            "provider refresh rewrote or hid the unavailable live value");
    require(!hub.debugSelectLabeledValue(bpmRowId, 4.0) &&
                std::abs(bpmMultiplier - 2.0f) < 0.0001f,
            "unknown labeled choice changed the current value");
    require(hub.debugSelectLabeledValue(bpmRowId, 0.5),
            "explicit replacement of unavailable value failed");
    require(std::abs(bpmMultiplier - 0.5f) < 0.0001f,
            "explicit replacement did not update the live value");

    require(selectionEvents == 3,
            "labeled choices did not emit exactly one event per successful selection");
    require(hub.layerPackageInspection_.dump() == payloadBefore,
            "labeled selection mutated the inspection payload");
    require(midi.getCcMaps().empty() && midi.getBtnMaps().empty() &&
                midi.getOscMaps().empty(),
            "labeled selection changed MIDI or OSC mappings");

    std::error_code cleanupError;
    std::filesystem::remove(activationPath, cleanupError);
    return true;
}

bool RunLayerPackagePresetBankSelectionScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const auto appRoot = synaptome_test_paths::appRoot();
    LayerLibrary library;
    require(library.reload((appRoot / "bin" / "data" / "layers").string()),
            "canonical catalog did not load for preset-bank scenario");
    const auto activationPath =
        std::filesystem::temp_directory_path() /
        "synaptome-layer-package-preset-bank-test.json";
    ofJson activation = {
        {"schemaVersion", 1},
        {"enabled", true},
        {"packages", ofJson::array({{
            {"id", "examples.signal_bloom"},
            {"enabled", true},
            {"catalogPath", (appRoot / "bin" / "data" / "layers-optional" /
                             "examples.signal_bloom.json").string()},
            {"preset", "default"},
            {"mappingPreset", ""}
        }})}
    };
    {
        std::ofstream out(activationPath, std::ios::trunc);
        out << std::setw(2) << activation << "\n";
    }
    require(library.loadOptInPackages(activationPath.string()),
            "active package catalog did not load for preset-bank scenario");
    const auto* activeEntry = library.find("examples.signal_bloom");
    require(activeEntry != nullptr, "Signal Bloom was not active for preset-bank scenario");
    const std::string catalogBefore = activeEntry->config.dump();

    ParameterRegistry registry;
    MidiRouter midi;
    ControlMappingHubState hub;
    hub.setParameterRegistry(&registry);
    hub.setMidiRouter(&midi);
    hub.setLayerLibrary(&library);
    std::string selectedPreset = "default";
    int applyCalls = 0;
    int previewCalls = 0;
    int cancelCalls = 0;
    std::string previewedPreset;
    std::string appliedAsset;
    std::string appliedBank;
    std::string appliedPreset;
    hub.setPackagePresetSelectionProvider(
        [&](const std::string& assetId, const std::string& bankId) {
            return assetId == "examples.signal_bloom" && bankId == "performance"
                ? selectedPreset
                : std::string();
        });
    hub.setPackagePresetApplyCallback(
        [&](const std::string& assetId,
            const std::string& bankId,
            const std::string& presetId) {
            ++applyCalls;
            appliedAsset = assetId;
            appliedBank = bankId;
            appliedPreset = presetId;
            selectedPreset = presetId;
            return true;
        });
    hub.setPackagePresetPreviewCallback(
        [&](const std::string& assetId,
            const std::string& bankId,
            const std::string& presetId) {
            require(
                assetId == "examples.signal_bloom" &&
                    bankId == "performance",
                "preset preview lost stable package/bank identity");
            ++previewCalls;
            previewedPreset = presetId;
            return true;
        });
    hub.setPackagePresetCancelCallback(
        [&]() { ++cancelCalls; });
    const auto payloadPath =
        (synaptome_test_paths::dataRoot() / "config" /
         "layer-package-inspection.json").string();
    hub.setLayerPackageInspectionPath(payloadPath);
    const std::string payloadBefore = hub.layerPackageInspection_.dump();
    hub.rebuildModel();

    const std::string rowId =
        "inspection.examples.signal_bloom.presetBank.performance";
    const auto* bankRow = hub.rowForId(rowId);
    require(bankRow != nullptr && bankRow->isPackagePresetBankRow,
            "active package did not expose an editable preset-bank row");
    require(bankRow->inspectionValue.find("Selected: Default") != std::string::npos &&
                bankRow->inspectionValue.find("Choices: Default, Bright, Calm") !=
                    std::string::npos &&
                bankRow->inspectionValue.find("live preview") != std::string::npos,
            "preset-bank row did not render ordered labels and transactional controls");

    require(hub.debugSelectPackagePreset(rowId, "calm"),
            "explicit Calm preset selection failed");
    require(applyCalls == 1 &&
                appliedAsset == "examples.signal_bloom" &&
                appliedBank == "performance" &&
                appliedPreset == "calm",
            "preset picker did not commit exact stable IDs once");
    require(previewCalls >= 2 &&
                previewedPreset == "calm",
            "preset picker did not live-preview the selected stable preset "
            "(calls=" + std::to_string(previewCalls) +
            ", last=" + previewedPreset + ")");
    hub.rebuildModel();
    bankRow = hub.rowForId(rowId);
    require(bankRow &&
                bankRow->inspectionValue.find("Selected: Calm") != std::string::npos,
            "committed preset provider state did not replay in Browser");

    hub.setPackagePresetApplyCallback(
        [&](const std::string&, const std::string&, const std::string&) {
            ++applyCalls;
            return false;
        });
    require(!hub.debugSelectPackagePreset(rowId, "bright"),
            "failed host preset commit was reported as successful");
    require(selectedPreset == "calm" && applyCalls == 2,
            "failed preset commit changed the committed selection");
    hub.cancelPackagePresetPicker();
    require(cancelCalls >= 1,
            "canceling a failed preset apply did not restore the preview");

    require(hub.layerPackageInspection_.dump() == payloadBefore,
            "preset selection rewrote the inspection payload");
    require(activeEntry->config.dump() == catalogBefore,
            "Browser preset selection mutated the shared catalog entry");
    require(registry.floats().empty() && registry.bools().empty() &&
                registry.strings().empty(),
            "preset selection mutated live parameter registries");
    require(midi.getCcMaps().empty() && midi.getBtnMaps().empty() &&
                midi.getOscMaps().empty(),
            "preset selection implicitly changed mappings");

    std::error_code cleanupError;
    std::filesystem::remove(activationPath, cleanupError);
    return true;
}

bool RunPackageControlTransactionScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };
    using namespace synaptome::controls;

    const auto inspectionPath =
        synaptome_test_paths::dataRoot() / "config" /
        "layer-package-inspection.json";
    const ofJson inspection = ofLoadJson(inspectionPath.string());
    require(inspection.contains("entries") &&
                inspection["entries"].is_array() &&
                !inspection["entries"].empty(),
            "package transaction fixture has no inspection entries");
    const ofJson& packageEntry = inspection["entries"][0];
    require(
        packageEntry.value("assetId", std::string()) ==
            "examples.signal_bloom",
        "package transaction fixture identity drifted");

    ofJson current = {
        {"schemaVersion", 1},
        {"cc", ofJson::array()},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array({{
            {"pattern", "/operator/level"},
            {"in", {0.0, 1.0}},
            {"out", {0.0, 1.0}},
            {"blend", "absolute"},
            {"relative", false}
        }})},
        {"osc", ofJson::array({{
            {"pattern", "/operator/level"},
            {"target", "console.layer2.gain"},
            {"provenance", {{"owner", "operator"}}}
        }})}
    };
    const std::vector<PackageLayerTarget> layers = {{
        2,
        "examples.signal_bloom",
        "console.layer2",
        true,
    }};
    auto preview = previewMappingSuggestion(
        packageEntry,
        "hostMicMotion",
        layers,
        current);
    require(preview.ok && preview.routes.size() == 2,
            "package mapping preview did not expand both routes to the assigned layer");
    require(preview.routes[0].expandedTargetId ==
                "console.layer2.gain" &&
                preview.routes[1].expandedTargetId ==
                "console.layer2.speedInput",
            "package mapping preview expanded unstable target IDs");
    require(preview.conflictCount() == 1 &&
                preview.routes[0].conflicts[0].kind == "target",
            "package mapping preview did not compare the operator target conflict");

    auto rejected = buildMappingCandidate(current, preview, false);
    require(!rejected.ok && rejected.document == current,
            "target conflict applied without explicit replacement");
    auto candidate = buildMappingCandidate(current, preview, true);
    require(candidate.ok && candidate.document["osc"].size() == 2,
            "explicit conflict replacement did not build a complete candidate");
    require(
        candidate.document["osc"][0]["provenance"].value(
            "owner", std::string()) == "package-suggestion",
        "package mapping candidate lost route provenance");

    ofJson sharedSourceBank = current;
    sharedSourceBank["osc"] = ofJson::array({{
        {"pattern", "/sensor/host/localmic/mic-level"},
        {"target", "console.layer1.opacity"},
        {"provenance", {{"owner", "operator"}}}
    }});
    auto sharedSourcePreview = previewMappingSuggestion(
        packageEntry,
        "hostMicMotion",
        layers,
        sharedSourceBank);
    require(
        sharedSourcePreview.ok &&
            sharedSourcePreview.routes[0].conflicts[0].kind ==
                "shared-source" &&
            !buildMappingCandidate(
                 sharedSourceBank,
                 sharedSourcePreview,
                 false)
                 .ok,
        "shared operator source was not protected by explicit approval");

    ofJson exactOperatorBank = current;
    exactOperatorBank["osc"][0]["pattern"] =
        "/sensor/host/localmic/mic-level";
    auto exactOperatorPreview = previewMappingSuggestion(
        packageEntry,
        "hostMicMotion",
        layers,
        exactOperatorBank);
    require(
        exactOperatorPreview.ok &&
            exactOperatorPreview.routes[0].conflicts[0].kind ==
                "exact-route" &&
            !buildMappingCandidate(
                 exactOperatorBank,
                 exactOperatorPreview,
                 false)
                 .ok,
        "exact operator route was silently claimed by the package");

    MidiRouter router;
    require(router.importMappingSnapshot(current, true),
            "operator mapping baseline did not import");
    const ofJson baseline = router.exportMappingSnapshot();
    auto failedPublish = router.publishMappingSnapshot(
        candidate.document,
        [](const ofJson&) { return false; });
    require(!failedPublish.ok &&
                failedPublish.rollbackSucceeded &&
                router.exportMappingSnapshot() == baseline,
            "failed mapping write did not restore the prior live routes");
    auto threwPublish = router.publishMappingSnapshot(
        candidate.document,
        [](const ofJson&) -> bool {
            throw std::runtime_error("injected mapping write exception");
        });
    require(!threwPublish.ok &&
                threwPublish.rollbackSucceeded &&
                router.exportMappingSnapshot() == baseline,
            "throwing mapping write did not restore the prior live routes");
    ofJson persisted;
    auto published = router.publishMappingSnapshot(
        candidate.document,
        [&](const ofJson& document) {
            persisted = document;
            return true;
        });
    require(published.ok && persisted == router.exportMappingSnapshot(),
            "mapping publication did not persist the canonical live candidate");
    require(
        persisted["osc"][0]["provenance"].value(
            "routeId", std::string()) ==
            "examples.signal_bloom/hostMicMotion/0/layer2",
        "mapping-bank round trip lost stable package route identity");

    const std::string routeId =
        "examples.signal_bloom/hostMicMotion/0/layer2";
    auto disabled =
        setPackageRouteEnabled(persisted, routeId, false);
    require(disabled.ok &&
                !disabled.document["osc"][0].value("enabled", true),
            "explicit package route disable did not retain the route");
    ofJson editedSource = {
        {"kind", "osc"},
        {"pattern", "/operator/edited"},
        {"in", {0.0, 1.0}},
        {"out", {0.0, 1.0}},
        {"smooth", 0.1},
        {"deadband", 0.0},
        {"blend", "set"},
        {"relative", false},
    };
    auto edited =
        editPackageRoute(disabled.document, routeId, editedSource);
    require(edited.ok &&
                edited.document["osc"][0].value(
                    "pattern", std::string()) ==
                    "/operator/edited" &&
                edited.document["osc"][0].value("enabled", false),
            "package route edit did not update source and re-enable");
    auto removed = removePackageRoute(edited.document, routeId);
    require(removed.ok && removed.document["osc"].size() == 1,
            "explicit package route removal affected the wrong routes");

    ofJson liveDocument = current;
    MappingSuggestionTransaction transaction;
    auto committed = transaction.publish(
        current,
        candidate.document,
        [&](const ofJson& document) {
            liveDocument = document;
            return true;
        });
    require(committed.ok && transaction.rollbackAvailable() &&
                liveDocument == candidate.document,
            "mapping transaction did not expose rollback after commit");
    auto rolledBack = transaction.rollback();
    require(rolledBack.ok && liveDocument == current,
            "mapping rollback did not restore the prior working document");

    ofJson actionPackage = packageEntry;
    actionPackage["controls"]["actions"] = ofJson::array({{
        {"id", "pulse"},
        {"label", "Pulse"},
        {"groupId", "performance"}
    }});
    actionPackage["mappingPresets"].push_back({
        {"id", "hostMicPulse"},
        {"label", "Host Mic Pulse"},
        {"description", "Trigger pulse on a rising mic edge."},
        {"applyMode", "suggestion-only"},
        {"mappings", ofJson::array({{
            {"target", {{"kind", "action"}, {"id", "pulse"}}},
            {"source", {
                {"kind", "osc"},
                {"pattern", "/sensor/host/localmic/mic-peak"},
                {"trigger", {
                    {"edge", "rising"},
                    {"threshold", 0.6}
                }}
            }}
        }})}
    });
    const ofJson emptyBank = {
        {"schemaVersion", 1},
        {"cc", ofJson::array()},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array()},
        {"osc", ofJson::array()},
    };
    auto actionPreview = previewMappingSuggestion(
        actionPackage, "hostMicPulse", layers, emptyBank);
    require(actionPreview.ok &&
                actionPreview.routes[0].expandedTargetId ==
                    "console.layer2.actions.pulse",
            "action suggestion did not expand a stable layer action target");
    auto actionCandidate =
        buildMappingCandidate(emptyBank, actionPreview, false);
    require(actionCandidate.ok &&
                actionCandidate.document["osc"][0]["trigger"].value(
                    "edge", std::string()) == "rising",
            "action suggestion lost trigger edge semantics");
    MidiRouter actionRouter;
    int actionCalls = 0;
    actionRouter.setActionTargetCallback(
        [&](int layerIndex, const std::string& actionId) {
            if (layerIndex == 2 && actionId == "pulse") {
                ++actionCalls;
                return true;
            }
            return false;
        });
    require(actionRouter.importMappingSnapshot(
                actionCandidate.document, true),
            "action mapping candidate did not import");
    actionRouter.onOscMessage(
        "/sensor/host/localmic/mic-peak", 0.2f);
    actionRouter.onOscMessage(
        "/sensor/host/localmic/mic-peak", 0.8f);
    actionRouter.onOscMessage(
        "/sensor/host/localmic/mic-peak", 0.9f);
    require(actionCalls == 1,
            "rising-edge action mapping did not fire exactly once");

    float gain = 0.25f;
    float speed = 0.5f;
    ParameterRegistry registry;
    ParameterRegistry::Descriptor gainMeta;
    gainMeta.range = {0.0f, 2.0f, 0.01f};
    registry.addFloat(
        "console.layer2.gain", &gain, gain, gainMeta);
    ParameterRegistry::Descriptor speedMeta;
    speedMeta.range = {0.0f, 4.0f, 0.01f};
    registry.addFloat(
        "console.layer2.speed", &speed, speed, speedMeta);
    PackagePresetTransaction presetTransaction;
    auto presetPreview = presetTransaction.preview(
        registry,
        "examples.signal_bloom",
        "bright",
        "console.layer2",
        {{"gain", 1.05}, {"speed", 1.15}});
    require(presetPreview.ok && gain == 1.05f && speed == 1.15f &&
                registry.getFloatBase("console.layer2.gain") == 0.25f,
            "live preset preview changed base ownership or missed live values");
    require(!presetTransaction.apply(
                []() { return false; }, 1, "0.1.0") &&
                gain == 0.25f && speed == 0.5f,
            "failed preset publication did not restore live/base values");
    presetPreview = presetTransaction.preview(
        registry,
        "examples.signal_bloom",
        "bright",
        "console.layer2",
        {{"gain", 1.05}, {"speed", 1.15}});
    require(presetTransaction.apply(
                []() { return true; }, 1, "0.1.0") &&
                registry.getFloatBase("console.layer2.gain") == 1.05f &&
                registry.findFloat("console.layer2.gain")
                        ->baseOrigin.originId ==
                    "examples.signal_bloom/bright",
            "preset apply did not publish values with package provenance");
    require(!presetTransaction.rollback(
                []() { return false; }) &&
                gain == 1.05f && speed == 1.15f &&
                presetTransaction.rollbackAvailable(),
            "failed preset rollback did not preserve the applied state");
    require(presetTransaction.rollback(
                []() { return true; }) &&
                gain == 0.25f && speed == 0.5f &&
                registry.getFloatBase("console.layer2.gain") == 0.25f,
            "preset rollback did not restore prior values and provenance");

    const auto appRoot = synaptome_test_paths::appRoot();
    LayerLibrary library;
    require(library.reload(
                (appRoot / "bin" / "data" / "layers").string()),
            "package transaction Browser fixture catalog did not load");
    const auto activationPath =
        std::filesystem::temp_directory_path() /
        "synaptome-package-control-transaction-activation.json";
    const ofJson activation = {
        {"schemaVersion", 1},
        {"enabled", true},
        {"packages", ofJson::array({{
            {"id", "examples.signal_bloom"},
            {"enabled", true},
            {"catalogPath",
             (appRoot / "bin" / "data" / "layers-optional" /
              "examples.signal_bloom.json").string()},
            {"presetBank", "performance"},
            {"preset", "default"},
            {"mappingPreset", ""}
        }})}
    };
    {
        std::ofstream out(activationPath, std::ios::trunc);
        out << std::setw(2) << activation << "\n";
    }
    require(library.loadOptInPackages(activationPath.string()),
            "package transaction Browser fixture did not activate");

    MidiRouter uiRouter;
    ParameterRegistry uiRegistry;
    ControlMappingHubState hub;
    hub.setParameterRegistry(&uiRegistry);
    hub.setMidiRouter(&uiRouter);
    hub.setLayerLibrary(&library);
    hub.setLayerPackageInspectionPath(
        inspectionPath.string());
    hub.setConsoleSlotInventoryCallback([]() {
        ConsoleLayerInfo info;
        info.index = 2;
        info.assetId = "examples.signal_bloom";
        info.label = "Signal Bloom";
        info.active = true;
        return std::vector<ConsoleLayerInfo>{info};
    });
    ofJson uiPersisted;
    int uiWrites = 0;
    hub.setMappingSnapshotPersistenceCallback(
        [&](const ofJson& document) {
            uiPersisted = document;
            ++uiWrites;
            return true;
        });
    hub.rebuildModel();
    const std::string mappingRowId =
        "inspection.examples.signal_bloom.mappingPreset.hostMicMotion";
    const auto* mappingRow = hub.rowForId(mappingRowId);
    require(mappingRow &&
                mappingRow->isPackageMappingPresetRow &&
                mappingRow->inspectionValue.find(
                    "Enter=preview/apply") != std::string::npos,
            "Browser did not expose the package mapping transaction controls");
    const std::string inspectionBefore =
        hub.layerPackageInspection_.dump();
    require(hub.debugPreviewPackageMapping(mappingRowId) &&
                uiRouter.getOscMaps().empty() &&
                uiWrites == 0,
            "Browser preview mutated live or persisted mappings");
    require(hub.debugApplyPackageMapping(mappingRowId) &&
                uiRouter.getOscMaps().size() == 2 &&
                uiWrites == 1,
            "Browser explicit apply did not publish both expanded routes");
    require(hub.debugDisablePackageMapping(mappingRowId) &&
                uiWrites == 2 &&
                std::all_of(
                    uiRouter.getOscMaps().begin(),
                    uiRouter.getOscMaps().end(),
                    [](const MidiRouter::OscMap& map) {
                        return !map.enabled;
                    }),
            "Browser explicit disable did not retain disabled routes");
    require(hub.debugRollbackPackageMapping() &&
                uiWrites == 3 &&
                std::all_of(
                    uiRouter.getOscMaps().begin(),
                    uiRouter.getOscMaps().end(),
                    [](const MidiRouter::OscMap& map) {
                        return map.enabled;
                    }),
            "Browser mapping rollback did not restore enabled routes");
    require(hub.debugRemovePackageMapping(mappingRowId) &&
                uiRouter.getOscMaps().empty() &&
                uiWrites == 4,
            "Browser explicit remove did not publish the route deletion");
    require(hub.debugRollbackPackageMapping() &&
                uiRouter.getOscMaps().size() == 2 &&
                uiWrites == 5,
            "Browser remove rollback did not restore package routes");
    require(hub.layerPackageInspection_.dump() ==
                inspectionBefore,
            "Browser package controls rewrote inspection metadata");
    std::error_code cleanupError;
    std::filesystem::remove(activationPath, cleanupError);
    return true;
}

bool RunOptInLayerPackageActivationScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };
    const auto appRoot = synaptome_test_paths::appRoot();
    LayerLibrary library;
    require(library.reload((appRoot / "bin" / "data" / "layers").string()),
            "canonical layer catalog did not load");
    const std::size_t baselineCount = library.entries().size();
    require(library.loadOptInPackages(
                (appRoot / "bin" / "data" / "config" / "layer-packages.json").string()),
            "disabled package config should be a successful no-op");
    require(library.entries().size() == baselineCount,
            "disabled package activation changed the canonical catalog");

    const auto activationPath =
        std::filesystem::temp_directory_path() / "synaptome-layer-package-activation-test.json";
    ofJson activation = {
        {"schemaVersion", 1},
        {"enabled", true},
        {"packages", ofJson::array({{
            {"id", "examples.signal_bloom"},
            {"enabled", true},
            {"catalogPath", (appRoot / "bin" / "data" / "layers-optional" /
                             "examples.signal_bloom.json").string()},
            {"preset", "bright"},
            {"mappingPreset", "hostMicMotion"},
            {"parameters", {{"speed", 2.25}}}
        }})}
    };
    {
        std::ofstream out(activationPath, std::ios::trunc);
        out << std::setw(2) << activation << "\n";
    }
    require(library.loadOptInPackages(activationPath.string()),
            "explicit package activation failed");
    const auto* entry = library.find("examples.signal_bloom");
    require(entry != nullptr, "activated package did not enter the layer catalog");
    require(entry->config["defaults"].value("speed", 0.0) == 2.25,
            "explicit activation value did not win over the preset");
    require(entry->config["defaults"].value("alpha", 0.0) == 0.95,
            "selected package preset was not merged");
    require(entry->config["packageActivation"].value("mappingApplied", true) == false,
            "mapping suggestion was applied implicitly");
    require(entry->config["packageActivation"].value(
                "presetBank",
                std::string()) == "performance",
            "legacy activation preset did not infer its package bank");
    const auto brightOrigins =
        library.parameterOriginsForConfig(
            "examples.signal_bloom",
            entry->config);
    require(
        brightOrigins.at("visible").kind ==
                synaptome::state::ParameterBaseOriginKind::
                    DefinitionDefault &&
            brightOrigins.at("alpha").kind ==
                synaptome::state::ParameterBaseOriginKind::Preset &&
            brightOrigins.at("alpha").originId ==
                "examples.signal_bloom/bright" &&
            brightOrigins.at("alpha").artifactVersion == 1 &&
            brightOrigins.at("alpha").artifactRevision == "0.1.0" &&
            brightOrigins.at("speed").kind ==
                synaptome::state::ParameterBaseOriginKind::
                    ActivationOverride &&
            brightOrigins.at("speed").originId ==
                "examples.signal_bloom" &&
            brightOrigins.at("speed").artifactVersion == 1,
        "package merge lost definition, preset, or activation origin");

    ofJson calmConfig;
    std::string presetError;
    require(library.configForPackagePreset(
                "examples.signal_bloom",
                "performance",
                "calm",
                calmConfig,
                &presetError),
            "valid package preset-bank selection did not resolve: " + presetError);
    require(calmConfig["defaults"].value("speed", 0.0) == 2.25,
            "explicit activation override did not remain above selected preset");
    require(calmConfig["defaults"].value("bpmMultiplier", 0.0) == 0.5,
            "selected Calm preset did not replace the package BPM default");
    require(calmConfig["defaults"].value("alpha", 0.0) == 0.62,
            "selected Calm preset did not replace the prior preset value");
    require(calmConfig["packageActivation"].value("mappingApplied", true) == false,
            "preset resolution implicitly applied a mapping preset");
    const auto calmOrigins =
        library.parameterOriginsForConfig(
            "examples.signal_bloom",
            calmConfig);
    require(
        calmOrigins.at("bpmMultiplier").kind ==
                synaptome::state::ParameterBaseOriginKind::Preset &&
            calmOrigins.at("bpmMultiplier").originId ==
                "examples.signal_bloom/calm" &&
            calmOrigins.at("speed").kind ==
                synaptome::state::ParameterBaseOriginKind::
                    ActivationOverride,
        "next-load preset resolution lost per-parameter provenance");
    ofJson rejectedConfig = {{"sentinel", true}};
    require(!library.configForPackagePreset(
                "examples.signal_bloom",
                "missingBank",
                "calm",
                rejectedConfig,
                &presetError) &&
                rejectedConfig == ofJson({{"sentinel", true}}),
            "unknown preset bank was not rejected atomically");
    require(!library.configForPackagePreset(
                "examples.signal_bloom",
                "performance",
                "missingPreset",
                rejectedConfig,
                &presetError),
            "unknown package preset was not rejected");

    activation["packages"][0]["mappingPreset"] = "missingPreset";
    {
        std::ofstream out(activationPath, std::ios::trunc);
        out << std::setw(2) << activation << "\n";
    }
    LayerLibrary rejectedLibrary;
    require(rejectedLibrary.reload((appRoot / "bin" / "data" / "layers").string()),
            "canonical catalog did not load for rejection case");
    require(!rejectedLibrary.loadOptInPackages(activationPath.string()),
            "unknown mapping preset was not rejected");
    require(rejectedLibrary.find("examples.signal_bloom") == nullptr,
            "invalid package activation leaked into the runtime catalog");
    std::error_code cleanupError;
    std::filesystem::remove(activationPath, cleanupError);
    return true;
}

bool RunCollapsedAssetBrowserStartupScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() /
        "synaptome-collapsed-asset-browser";
    std::filesystem::create_directories(tempRoot);
    {
        std::ofstream out(tempRoot / "generative.json", std::ios::trunc);
        out << R"JSON({
  "id": "tests.browser.generative",
  "label": "Generative Fixture",
  "category": "Generative",
  "layerGroup": "Elements",
  "type": "generative.perlin"
})JSON";
    }
    {
        std::ofstream out(tempRoot / "scene.json", std::ios::trunc);
        out << R"JSON({
  "id": "tests.browser.scene",
  "label": "Scene Fixture",
  "category": "Scenes",
  "layerGroup": "Saved Scenes",
  "type": "generative.perlin"
})JSON";
    }

    LayerLibrary library;
    require(library.reload(tempRoot.string()),
            "asset-browser startup fixture did not load");

    auto browser = std::make_shared<AssetBrowser>();
    browser->setLibrary(&library);
    MenuController controller;
    controller.pushState(browser);

    auto startup = controller.viewModel();
    require(startup.state.entries.size() == 2,
            "collapsed asset browser exposed groups or assets on startup");
    require(startup.state.entries.front().id == "category:Scenes",
            "Scenes was not the first asset-browser category");
    require(startup.state.entries[1].id == "category:Generative",
            "asset-browser startup did not contain only collapsed categories");

    require(controller.handleInput(OF_KEY_RIGHT),
            "asset browser did not expand the selected Scenes category");
    auto categoryOpen = controller.viewModel();
    require(categoryOpen.state.entries.size() == 3 &&
                categoryOpen.state.entries[1].id ==
                    "group:Scenes/Saved Scenes",
            "opening a category did not reveal its collapsed element group");
    require(std::none_of(
                categoryOpen.state.entries.begin(),
                categoryOpen.state.entries.end(),
                [](const MenuController::EntryView& entry) {
                    return entry.id == "tests.browser.scene";
                }),
            "asset appeared before its element group was explicitly opened");

    require(controller.handleInput(OF_KEY_DOWN) &&
                controller.handleInput(OF_KEY_RIGHT),
            "asset browser did not expand the selected element group");
    auto groupOpen = controller.viewModel();
    require(std::any_of(
                groupOpen.state.entries.begin(),
                groupOpen.state.entries.end(),
                [](const MenuController::EntryView& entry) {
                    return entry.id == "tests.browser.scene";
                }),
            "opening an element group did not reveal its asset");
    return true;
}

bool RunAssetBrowserSearchScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    LayerLibrary library;
    require(library.reload(
                (synaptome_test_paths::dataRoot() / "layers").string()),
            "canonical layer catalog did not load for asset search");

    auto browser = std::make_shared<AssetBrowser>();
    browser->setLibrary(&library);
    browser->setAllowEntryPredicate([](const LayerLibrary::Entry& entry) {
        return !entry.isHudWidget();
    });

    MenuController controller;
    controller.pushState(browser);
    for (const char ch : std::string("circuit river")) {
        require(controller.handleInput(static_cast<int>(ch)),
                "type-to-search did not consume printable input");
    }

    const auto filtered = controller.viewModel();
    require(browser->searchQuery() == "circuit river",
            "asset search query did not preserve typed text");
    require(filtered.state.entries.size() == 1,
            "multi-token asset search did not narrow to one result");
    require(filtered.state.entries.front().id == "generative.circuitRiver",
            "asset search selected the wrong circuit profile");
    require(filtered.state.selectedIndex == 0 &&
                filtered.state.entries.front().selected,
            "filtered asset result did not receive deterministic focus");

    require(controller.handleInput(OF_KEY_ESC),
            "first Escape did not clear active asset search");
    require(controller.isCurrent(browser->id()) && browser->searchQuery().empty(),
            "first Escape closed the picker instead of clearing search");
    require(controller.handleInput(OF_KEY_ESC),
            "second Escape did not close the asset picker");
    require(!controller.isCurrent(browser->id()),
            "asset picker remained open after cleared Escape");
    return true;
}

bool RunFocusedLayerEditScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    ParameterRegistry registry;
    CircuitTraceLayer circuitRiver(CircuitTraceLayer::Model::CircuitRiver);
    circuitRiver.setRegistryPrefix("generative.circuitRiver");
    circuitRiver.setup(registry);

    const auto quickParameters = registry.orderedQuickFloat();
    require(quickParameters.size() >= 2,
            "Circuit River did not register its deliberate quick-access set");
    require(quickParameters[0]->meta.id == "generative.circuitRiver.speed" &&
                quickParameters[0]->meta.quickAccessOrder == 10,
            "Circuit River growth speed is not its primary quick-access parameter");
    require(quickParameters[1]->meta.id == "generative.circuitRiver.behavior" &&
                quickParameters[1]->meta.quickAccessOrder == 20,
            "Circuit River growth behavior is not its secondary quick-access parameter");
    const std::string expectedQuickParameter =
        "generative.circuitRiver.speed";

    LayerLibrary library;
    require(library.reload(
                (synaptome_test_paths::dataRoot() / "layers").string()),
            "canonical catalog did not load for focused-layer edit");

    ControlMappingHubState hub;
    hub.setLayerLibrary(&library);
    hub.setParameterRegistry(&registry);

    auto console = std::make_shared<ConsoleState>();
    int editRequest = 0;
    bool focusedLayer = false;
    console->setRequestEditLayerCallback([&](int layerIndex) {
        editRequest = layerIndex;
        focusedLayer = hub.focusAssetById("generative.circuitRiver");
    });
    MenuController controller;
    controller.pushState(console);
    require(controller.handleInput(MenuController::HOTKEY_MOD_CTRL | 'e'),
            "Console Ctrl+E was not handled");
    require(editRequest == 1,
            "Console Ctrl+E did not target the focused slot");
    require(focusedLayer,
            "Browser could not focus the active layer by public asset ID");

    const auto focusedView = hub.view();
    require(focusedView.selectedIndex >= 0 &&
                focusedView.selectedIndex <
                    static_cast<int>(focusedView.entries.size()),
            "focused layer did not expose a selected parameter");
    require(focusedView.entries[
                static_cast<std::size_t>(focusedView.selectedIndex)].id ==
                expectedQuickParameter,
            "focused layer did not select its lowest-order quick-access parameter");
    return true;
}

bool RunCircuitVariantLifecycleScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    require(
        CircuitTraceLayer::modelFromId("circuitAntTunnels") ==
            CircuitTraceLayer::Model::CircuitAntTunnels,
        "Circuit Ant Tunnels model identity did not resolve");
    require(
        CircuitTraceLayer::modelFromId("circuitFlowField") ==
            CircuitTraceLayer::Model::CircuitFlowField,
        "Circuit Flow Field model identity did not resolve");

    auto runModel = [&](CircuitTraceLayer::Model model,
                        const std::string& prefix) {
        CircuitTraceLayer first(model);
        CircuitTraceLayer second(model);
        ParameterRegistry firstRegistry;
        ParameterRegistry secondRegistry;
        first.setRegistryPrefix(prefix + ".first");
        second.setRegistryPrefix(prefix + ".second");
        first.setup(firstRegistry);
        second.setup(secondRegistry);

        LayerUpdateParams params;
        params.dt = 1.0f / 30.0f;
        params.bpm = 120.0f;
        params.speed = 1.0f;
        for (int frame = 0; frame < 90; ++frame) {
            params.time += params.dt;
            first.update(params);
            second.update(params);
        }
        require(first.debugStateSignature() == second.debugStateSignature(),
                prefix + " did not reproduce from its owned seed");
        for (int heading : first.debugAgentHeadings()) {
            require(heading >= 0 && heading < 8,
                    prefix + " emitted a heading outside the eight-direction lattice");
        }
        return first.debugStateSignature();
    };

    const std::uint64_t antSignature = runModel(
        CircuitTraceLayer::Model::CircuitAntTunnels,
        "generative.circuitAntTunnels");
    const std::uint64_t flowSignature = runModel(
        CircuitTraceLayer::Model::CircuitFlowField,
        "generative.circuitFlowField");
    require(antSignature != flowSignature,
            "new circuit variants collapsed to the same routing lifecycle");
    return true;
}

bool RunCircuitLeniaLifecycleScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    ofJson config;
    config["presentation"] = "circuit";
    config["textureSize"] = { 64, 36 };
    config["defaults"]["seed"] = 2112.0f;
    config["defaults"]["injectionRate"] = 0.0f;
    config["defaults"]["circuitThreshold"] = 0.12f;
    config["defaults"]["circuitLevels"] = 4.0f;
    config["defaults"]["circuitTraceWidth"] = 1.0f;

    LeniaLayer first;
    LeniaLayer second;
    ParameterRegistry firstRegistry;
    ParameterRegistry secondRegistry;
    first.setRegistryPrefix("generative.circuitLenia.first");
    second.setRegistryPrefix("generative.circuitLenia.second");
    first.configure(config);
    second.configure(config);
    first.setup(firstRegistry);
    second.setup(secondRegistry);

    require(first.debugUsesCircuitPresentation() &&
                second.debugUsesCircuitPresentation(),
            "Circuit Lenia did not retain its catalog-selected presentation");
    const auto* threshold = firstRegistry.findFloat(
        "generative.circuitLenia.first.circuitThreshold");
    const auto* traceWidth = firstRegistry.findFloat(
        "generative.circuitLenia.first.circuitTraceWidth");
    require(threshold && threshold->meta.group == "Circuit Appearance" &&
                threshold->meta.quickAccess &&
                threshold->meta.quickAccessOrder == 20,
            "Circuit Lenia threshold is not a labeled quick-access control");
    require(traceWidth && traceWidth->meta.quickAccess &&
                traceWidth->meta.quickAccessOrder == 30,
            "Circuit Lenia trace width is not a quick-access control");

    LayerUpdateParams params;
    params.dt = 1.0f / 30.0f;
    params.bpm = 120.0f;
    params.speed = 1.0f;
    for (int frame = 0; frame < 60; ++frame) {
        params.time += params.dt;
        first.update(params);
        second.update(params);
    }
    require(first.debugStateSignature() == second.debugStateSignature(),
            "Circuit Lenia did not reproduce from the same seed and parameters");

    LeniaLayer organic;
    ParameterRegistry organicRegistry;
    organic.setRegistryPrefix("generative.lenia.test");
    organic.setup(organicRegistry);
    require(!organic.debugUsesCircuitPresentation(),
            "established Lenia unexpectedly switched to circuit presentation");
    const auto* organicThreshold = organicRegistry.findFloat(
        "generative.lenia.test.circuitThreshold");
    require(
        organicThreshold &&
            organicThreshold->meta.group == "Circuit Appearance",
        "Lenia variants no longer expose one stable parameter surface");
    return true;
}

bool RunCircuitLeniaOscDefaultsScenario() {
    auto require = [](bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    };

    struct ExpectedRoute {
        const char* pattern;
        const char* target;
        float outMin;
        float outMax;
    };
    const ExpectedRoute routes[] = {
        { "/control/circuit-lenia/threshold",
          "generative.circuitLenia.circuitThreshold", 0.0f, 0.55f },
        { "/control/circuit-lenia/levels",
          "generative.circuitLenia.circuitLevels", 2.0f, 8.0f },
        { "/control/circuit-lenia/trace-width",
          "generative.circuitLenia.circuitTraceWidth", 1.0f, 4.0f },
        { "/control/circuit-lenia/growth-center",
          "generative.circuitLenia.growthCenter", 0.20f, 0.55f },
        { "/control/circuit-lenia/growth-width",
          "generative.circuitLenia.growthWidth", 0.02f, 0.18f },
        { "/control/circuit-lenia/injection-rate",
          "generative.circuitLenia.injectionRate", 0.0f, 0.12f },
        { "/control/circuit-lenia/field-scale",
          "generative.circuitLenia.fieldScale", 0.5f, 4.0f },
    };

    ofJson fixture = {
        {"cc", ofJson::array()},
        {"buttons", ofJson::array()},
        {"oscSources", ofJson::array()},
        {"osc", ofJson::array()},
    };
    for (const auto& expected : routes) {
        fixture["osc"].push_back({
            {"bank", "home"},
            {"pattern", expected.pattern},
            {"target", expected.target},
            {"in", {0.0f, 1.0f}},
            {"out", {expected.outMin, expected.outMax}},
            {"blend", "absolute"},
            {"relative", false},
        });
    }

    MidiRouter router;
    require(
        router.importMappingSnapshot(fixture, true),
        "Circuit Lenia editable OSC fixture did not import");

    for (const auto& expected : routes) {
        const auto* route = router.findOscMap(expected.target);
        require(route != nullptr,
                std::string("missing Circuit Lenia OSC route: ") + expected.target);
        require(route->pattern == expected.pattern && route->bankId == "home",
                std::string("Circuit Lenia OSC route identity drifted: ") +
                    expected.target);
        require(route->blend == modifier::BlendMode::kAbsolute &&
                    !route->relativeToBase &&
                    std::abs(route->outMin - expected.outMin) < 0.0001f &&
                    std::abs(route->outMax - expected.outMax) < 0.0001f,
                std::string("Circuit Lenia OSC profile drifted: ") +
                    expected.target);
    }

    const std::string editedTarget =
        "generative.circuitLenia.circuitThreshold";
    require(router.adjustOscMap(
                editedTarget, 0.0f, 0.0f, 0.01f, -0.02f, 0.03f, 0.001f),
            "normal mapping flow could not edit a Circuit Lenia OSC default");
    const auto* edited = router.findOscMap(editedTarget);
    require(edited != nullptr &&
                std::abs(edited->outMin - 0.01f) < 0.0001f &&
                std::abs(edited->outMax - 0.53f) < 0.0001f,
            "Circuit Lenia OSC range edit was not applied");

    const ofJson snapshot = router.exportMappingSnapshot();
    MidiRouter restored;
    require(restored.importMappingSnapshot(snapshot, true),
            "Circuit Lenia OSC edit did not import from a scene mapping snapshot");
    const auto* restoredRoute = restored.findOscMap(editedTarget);
    require(restoredRoute != nullptr &&
                restoredRoute->pattern == "/control/circuit-lenia/threshold" &&
                std::abs(restoredRoute->outMin - 0.01f) < 0.0001f &&
                std::abs(restoredRoute->outMax - 0.53f) < 0.0001f,
            "Circuit Lenia OSC edit did not survive scene mapping persistence");
    return true;
}

}  // namespace browser_flow

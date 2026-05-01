#pragma once
#include "ofMain.h"
#include "ofxMidi.h"
#include <functional>
#include <cstdint>
#include <vector>
#include "../common/modifier_binding.h"
#include <unordered_map>

// Runtime MIDI router that supports live mapping/learning of CC controls.
class MidiRouter : public ofxMidiListener {
public:
    enum class BoolMode { Assign, Toggle };

    struct CcMap {
        int cc = -1;
        int channel = -1;
        std::string target;
        std::string bankId;
        std::string controlId;
        std::string deviceId;
        std::string columnId;
        std::string slotId;
        float outMin = 0.0f;
        float outMax = 1.0f;
        bool snapInt = false;
        float step = 0.0f;
        float lastHardwareNorm = 0.0f;
        bool hardwareKnown = false;
        float catchValue = 0.0f;
        float pendingDelta = 0.0f;
        float lastPendingDelta = 0.0f;
        bool pending = false;
        uint64_t pendingSinceMs = 0;
    };

    struct BtnMap {
        int num = -1;
        std::string type = "toggle";
        int channel = -1;
        std::string target;
        std::string bankId;
        std::string controlId;
        std::string deviceId;
        std::string columnId;
        std::string slotId;
        float setValue = 1.0f;
    };

    struct OscMap {
        std::string pattern;
        std::string target;
        std::string bankId;
        std::string controlId;
        float inMin = 0.0f;
        float inMax = 1.0f;
        float outMin = 0.0f;
        float outMax = 1.0f;
        float smooth = 0.2f;
        float deadband = 0.0f;
        modifier::BlendMode blend = modifier::BlendMode::kScale;
        bool relativeToBase = true;
    };

    struct OscSourceProfile {
        std::string pattern;
        float inMin = 0.0f;
        float inMax = 1.0f;
        float outMin = 0.0f;
        float outMax = 1.0f;
        float smooth = 0.2f;
        float deadband = 0.0f;
        modifier::BlendMode blend = modifier::BlendMode::kScale;
        bool relativeToBase = true;
    };

    struct OscSourceInfo {
        std::string address;
        float lastValue = 0.0f;
        uint64_t lastSeenMs = 0;
        bool seen = false;
    };

    struct TakeoverState {
        std::string bankId;
        std::string controlId;
        std::string targetId;
        float delta = 0.0f;
        float hardwareValue = 0.0f;
        float catchValue = 0.0f;
        uint64_t pendingSinceMs = 0;
    };

    struct CapturedMidiControl {
        std::string type;
        int channel = -1;
        int number = -1;
        int value = 0;
    };

    struct BindingMetadata {
        BindingMetadata();
        int channel;
        std::string controlId;
        std::string slotId;
        std::string deviceId;
        std::string columnId;
    };

    void bindFloat(const std::string& name, float* ptr,
                   float defMin = 0.0f, float defMax = 1.0f,
                   bool snapInt = false, float step = 0.0f,
                   const std::string& bankId = std::string(),
                   const std::string& controlId = std::string());
    void bindBool(const std::string& name, bool* ptr, BoolMode mode = BoolMode::Assign,
                  const std::string& bankId = std::string(),
                  const std::string& controlId = std::string());

    bool load(const std::string& jsonPath);
    bool save(const std::string& jsonPath = "");
    ofJson exportMappingSnapshot() const;
    bool importMappingSnapshot(const ofJson& snapshot, bool replaceExisting = true);
    void update();
    void close();
    modifier::BindingList snapshotModifierBindings() const;
    void setFloatTargetTouchedCallback(std::function<void(const std::string&)> cb);

    bool isConnected() const { return isOpen; }
    std::string connectedPortName() const { return currentPortLabel; }

    void listPortsToLog() const;
    void newMidiMessage(ofxMidiMessage& msg) override;

    void beginLearn(const std::string& targetName);
    // Begin learn; if oscLearn is true, the next incoming OSC address will be recorded
    void beginLearn(const std::string& targetName, bool oscLearn);
    bool isLearning() const { return !learningTarget.empty(); }
    bool isLearningOsc() const { return learningIsOsc; }
    std::string learningTargetName() const { return learningTarget; }

    // Called by ofApp when OSC messages arrive so the router can capture learn events
    void onOscMessage(const std::string& address, float value);

    // Capture the most-recent OSC address/value seen by onOscMessage
    // and create a mapping for 'target' using that address. Returns true on success.
    bool setOscMapFromLast(const std::string& target);

    void seedOscSources(const std::vector<std::string>& addresses);
    const std::vector<OscSourceInfo>& getOscSources() const { return oscSources; }
    const std::vector<OscSourceProfile>& getOscSourceProfiles() const { return oscSourceProfiles; }
    bool setOscMapFromAddress(const std::string& target, const std::string& address);

    void setActiveBank(const std::string& bankId);
    const std::string& activeBank() const { return activeBankId; }
    std::vector<TakeoverState> pendingTakeovers() const;

    // Accessors for the last seen OSC address/value (useful for UI capture)
    std::string lastOscAddress() const { return lastOscAddr; }
    float lastOscValue() const { return lastOscVal; }

    // Enumerate available MIDI input port names.
    std::vector<std::string> availableInputPorts() const;
    void setTestPortList(const std::vector<std::string>& ports);
    void clearTestPortList();
    void captureNextMidiControl(std::function<void(const CapturedMidiControl&)> callback);
    void cancelMidiControlCapture();

    void setOrUpdateCc(const std::string& target,
                       int ccNum,
                       float outMin = 0.0f,
                       float outMax = 1.0f,
                       bool snapInt = false,
                       float step = 0.0f,
                       const BindingMetadata& binding = BindingMetadata());
    void setOrUpdateBtn(const std::string& target,
                        int noteNum,
                        const std::string& type,
                        float setValue,
                        const BindingMetadata& binding = BindingMetadata());
    void adjustCcRange(const std::string& target, float dMin, float dMax);
    void unbindByPrefix(const std::string& prefix);

    const std::vector<CcMap>& getCcMaps() const { return ccMaps; }
    const std::vector<BtnMap>& getBtnMaps() const { return btnMaps; }
    const std::vector<OscMap>& getOscMaps() const { return oscMaps; }

    const OscMap* findOscMap(const std::string& target) const;
    OscMap* findOscMap(const std::string& target);
    const OscSourceProfile* findOscSourceProfile(const std::string& pattern) const;
    OscSourceProfile* findOscSourceProfile(const std::string& pattern);
    bool adjustOscMap(const std::string& target,
                      float dInMin,
                      float dInMax,
                      float dOutMin,
                      float dOutMax,
                      float dSmooth,
                      float dDeadband);
    bool adjustOscSourceProfile(const std::string& pattern,
                                float dInMin,
                                float dInMax,
                                float dOutMin,
                                float dOutMax,
                                float dSmooth,
                                float dDeadband);
    bool removeMidiMappingsForTarget(const std::string& target);
    bool removeOscMappingsForTarget(const std::string& target);

    // Optional callback invoked when an OSC map is added at runtime (learned)
    std::function<void(const OscMap&)> onOscMapAdded;
    std::function<void()> onOscRoutesChanged;
    // Optional callback invoked when a CC map is added or updated at runtime (learned)
    std::function<void(const CcMap&)> onCcMapAdded;

private:
    void applyCc(CcMap& map, int value0127);
    void applyBtn(const BtnMap& map, int value0127);
    OscSourceInfo* findOscSource(const std::string& address);
    const OscSourceInfo* findOscSource(const std::string& address) const;
    OscSourceProfile* ensureOscSourceProfile(const std::string& pattern);
    void ensureOscDeviceChannels(const std::string& deviceType, const std::string& deviceId);

    bool isMapActive(const std::string& bankId) const;
    float computeOutputValue(const CcMap& map, float normalized) const;
    float computeTolerance(const CcMap& map) const;

    bool openPreferredPort();
    bool openByName(const std::string& name, bool allowSubstring);
    void markClosed();
    uint64_t lastRetryMs = 0;
    uint64_t retryIntervalMs = 1000;


    struct FloatTarget {
        float* ptr = nullptr;
        float defMin = 0.0f;
        float defMax = 1.0f;
        bool snapInt = false;
        float step = 0.0f;
        std::string defaultBankId;
        std::string defaultControlId;
    };
    struct BoolTarget {
        bool* ptr = nullptr;
        BoolMode mode = BoolMode::Assign;
        bool lastHigh = false;
        std::string defaultBankId;
        std::string defaultControlId;
    };

    std::string deviceName;
    int deviceIndex = -1;
    std::string currentPortLabel;

    std::vector<CcMap> ccMaps;
    std::vector<BtnMap> btnMaps;
    std::vector<OscMap> oscMaps;
    std::vector<OscSourceProfile> oscSourceProfiles;
    std::vector<OscSourceInfo> oscSources;
    mutable std::vector<std::string> testPortListOverride_;
    mutable bool useTestPortListOverride_ = false;

    std::unordered_map<std::string, FloatTarget> floatTargets;
    std::unordered_map<std::string, BoolTarget> boolTargets;
    std::function<void(const std::string&)> floatTargetTouchedCallback_;

    std::function<void(const CapturedMidiControl&)> pendingMidiCapture_;
    ofxMidiIn midiIn;
    std::string activeBankId;
    float softTakeoverTolerance = 0.02f;

    bool isOpen = false;

    std::string mappingPath;
    std::string learningTarget;
    bool learningIsOsc = false;
    std::string lastOscAddr;
    float lastOscVal = 0.0f;
};

inline MidiRouter::BindingMetadata::BindingMetadata()
    : channel(-1) {}

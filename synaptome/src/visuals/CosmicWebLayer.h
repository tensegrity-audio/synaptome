#pragma once

#include "Layer.h"

#include <cstdint>
#include <cstddef>
#include <vector>

class CosmicWebLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Node {
        glm::vec2 pos{ 0.0f, 0.0f };
        glm::vec2 vel{ 0.0f, 0.0f };
        float seed = 0.0f;
    };

    struct Pulse {
        glm::vec2 origin{ 0.5f, 0.5f };
        float age = 0.0f;
        float strength = 1.0f;
    };

    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const std::string& label,
                       float min,
                       float max,
                       float step,
                       const std::string& description = std::string());
    void readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b);
    void clampParams();
    void resetNodes();
    void triggerPulse(float strength, const glm::vec2& origin);
    void updateAudioState();
    void updateNodes(float dt, float timeSeconds);
    void updatePulses(float dt);
    glm::vec2 visualPositionFor(std::size_t index, const Node& node) const;
    float waveformSampleFor(std::size_t index) const;
    float pulseForPoint(const glm::vec2& point) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramStarCount_ = 360.0f;
    float paramConnectionDistance_ = 0.22f;
    float paramMaxConnections_ = 4.0f;
    float paramFilamentAlpha_ = 0.78f;
    float paramFilamentThickness_ = 1.8f;
    float paramNodeSize_ = 3.6f;
    float paramTwinkle_ = 0.9f;
    float paramPulseSpeed_ = 0.68f;
    float paramPulseWidth_ = 0.11f;
    float paramPulseDecay_ = 1.15f;
    float paramFieldScale_ = 3.2f;
    float paramFieldStrength_ = 0.16f;
    float paramFlowSpeed_ = 0.11f;
    float paramTurbulence_ = 0.36f;
    float paramCenterPull_ = 0.06f;
    float paramDriftX_ = 0.0f;
    float paramDriftY_ = -0.015f;
    float paramWaveformDisplacement_ = 0.075f;
    float paramAudioAmount_ = 1.0f;
    float paramBassPull_ = 0.28f;
    float paramMidsTurbulence_ = 0.42f;
    float paramHighsTwinkle_ = 0.75f;
    float paramPeakPulseThreshold_ = 0.58f;
    float paramPulseCooldown_ = 0.35f;
    float paramAudioSmoothing_ = 0.35f;
    float paramXInput_ = 0.5f;
    float paramYInput_ = 0.5f;
    bool paramReseedRequested_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 64.0f;
    float paramSeed_ = 1337.0f;
    float paramBgAlpha_ = 0.32f;
    float paramBgR_ = 0.005f;
    float paramBgG_ = 0.008f;
    float paramBgB_ = 0.018f;
    float paramColorR_ = 0.15f;
    float paramColorG_ = 0.82f;
    float paramColorB_ = 1.0f;
    float paramPulseR_ = 1.0f;
    float paramPulseG_ = 0.42f;
    float paramPulseB_ = 0.95f;
    float paramNodeR_ = 0.8f;
    float paramNodeG_ = 0.96f;
    float paramNodeB_ = 1.0f;

    bool enabled_ = true;
    bool hasAudio_ = false;
    bool hasWaveform_ = false;
    uint64_t lastAudioFrame_ = 0;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    std::vector<float> waveform_;
    std::vector<Node> nodes_;
    std::vector<Pulse> pulses_;
    float lastPulseTime_ = -1000.0f;
    float nextAutoReseedBeat_ = -1.0f;
    std::uint32_t seedState_ = 1337;
    bool loggedFirstDraw_ = false;
    bool loggedEmptyDraw_ = false;
};

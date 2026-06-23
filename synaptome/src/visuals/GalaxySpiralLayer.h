#pragma once

#include "Layer.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class GalaxySpiralLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Star {
        float radius = 0.0f;
        float armIndex = 0.0f;
        float armOffset = 0.0f;
        float angleOffset = 0.0f;
        float size = 1.0f;
        float brightness = 1.0f;
        float heat = 0.0f;
        float dust = 0.0f;
        float seed = 0.0f;
    };

    struct Pulse {
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
    void resetStars();
    void updateAudioState();
    void updatePulses(float dt);
    void triggerPulse(float strength);
    glm::vec2 starPosition(std::size_t index, const Star& star, const glm::vec2& center, float radiusPx) const;
    float waveformSampleFor(std::size_t index) const;
    float pulseForRadius(float radius) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramStarCount_ = 1400.0f;
    float paramArms_ = 4.0f;
    float paramRadius_ = 0.92f;
    float paramCoreRadius_ = 0.16f;
    float paramArmTightness_ = 3.4f;
    float paramArmWidth_ = 0.42f;
    float paramInclination_ = 0.72f;
    float paramRotationSpeed_ = 0.018f;
    float paramStarSize_ = 1.45f;
    float paramStarBrightness_ = 1.2f;
    float paramDustAlpha_ = 0.32f;
    float paramCoreGlow_ = 0.72f;
    float paramTwinkle_ = 0.8f;
    float paramAudioAmount_ = 1.0f;
    float paramBassExpansion_ = 0.42f;
    float paramMidsTwist_ = 0.9f;
    float paramHighsSparkle_ = 1.35f;
    float paramWaveformWarp_ = 0.075f;
    float paramPeakPulseThreshold_ = 0.62f;
    float paramPulseCooldown_ = 0.32f;
    float paramPulseSpeed_ = 0.86f;
    float paramPulseWidth_ = 0.12f;
    float paramPulseStrength_ = 1.0f;
    float paramAudioSmoothing_ = 0.36f;
    bool paramReseedRequested_ = false;
    float paramSeed_ = 4242.0f;
    float paramBgAlpha_ = 0.55f;
    float paramBgR_ = 0.004f;
    float paramBgG_ = 0.006f;
    float paramBgB_ = 0.018f;
    float paramStarR_ = 0.46f;
    float paramStarG_ = 0.78f;
    float paramStarB_ = 1.0f;
    float paramCoreR_ = 1.0f;
    float paramCoreG_ = 0.72f;
    float paramCoreB_ = 0.34f;
    float paramDustR_ = 0.58f;
    float paramDustG_ = 0.24f;
    float paramDustB_ = 0.88f;
    float paramPulseR_ = 0.92f;
    float paramPulseG_ = 0.38f;
    float paramPulseB_ = 1.0f;

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
    std::vector<Star> stars_;
    std::vector<Pulse> pulses_;
    float rotationPhase_ = 0.0f;
    float lastPulseTime_ = -1000.0f;
    std::uint32_t seedState_ = 4242;
    int armState_ = 4;
};

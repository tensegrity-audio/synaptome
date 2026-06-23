#pragma once

#include "Layer.h"

#include <cstdint>
#include <cstddef>
#include <vector>

class BigBangLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Particle {
        glm::vec2 pos{ 0.0f, 0.0f };
        glm::vec2 prev{ 0.0f, 0.0f };
        float angle = 0.0f;
        float radius = 0.0f;
        float speed = 0.0f;
        float spin = 0.0f;
        float seed = 0.0f;
        float size = 1.0f;
        float brightness = 1.0f;
        std::size_t shell = 0;
        std::size_t shellIndex = 0;
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
    void resetParticles();
    void updateAudioState();
    void updateBeatState(float timeSeconds, float bpm);
    void updateParticles(float dt, float timeSeconds);
    void drawBackground(float width, float height, float alpha) const;
    void drawShells(float width, float height, float minDim, float alpha, float beat) const;
    void drawFilaments(float width, float height, float minDim, float alpha) const;
    void drawParticles(float width, float height, float minDim, float alpha) const;
    glm::vec2 particlePosition(const Particle& particle, float timeSeconds) const;
    float waveformSampleFor(std::size_t index) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramParticleCount_ = 720.0f;
    float paramShellCount_ = 7.0f;
    float paramExpansionRate_ = 0.18f;
    float paramExpansionPower_ = 0.58f;
    float paramCollapse_ = 0.42f;
    float paramRadius_ = 0.93f;
    float paramCoreRadius_ = 0.055f;
    float paramGalaxySpin_ = 1.15f;
    float paramSwirl_ = 0.64f;
    float paramTurbulence_ = 0.24f;
    float paramGravity_ = 0.18f;
    float paramDustSize_ = 2.1f;
    float paramDustGlow_ = 2.8f;
    float paramTrailAlpha_ = 0.42f;
    float paramTrailThickness_ = 1.1f;
    float paramFilamentAlpha_ = 0.34f;
    float paramFilamentStride_ = 5.0f;
    float paramShellAlpha_ = 0.58f;
    float paramShellThickness_ = 2.2f;
    float paramShockwaveCount_ = 4.0f;
    float paramShockwaveWidth_ = 0.045f;
    float paramAudioAmount_ = 1.0f;
    float paramBassExpansion_ = 0.34f;
    float paramMidsTurbulence_ = 0.45f;
    float paramHighsSparkle_ = 0.9f;
    float paramWaveformRipple_ = 0.08f;
    float paramPeakBurstThreshold_ = 0.62f;
    float paramPeakImpulse_ = 0.75f;
    float paramBeatImpulse_ = 0.22f;
    float paramAudioSmoothing_ = 0.35f;
    bool paramReseedRequested_ = false;
    float paramSeed_ = 2026.0f;
    float paramBgAlpha_ = 0.26f;
    float paramBgR_ = 0.006f;
    float paramBgG_ = 0.005f;
    float paramBgB_ = 0.014f;
    float paramCoreR_ = 1.0f;
    float paramCoreG_ = 0.76f;
    float paramCoreB_ = 0.36f;
    float paramDustR_ = 0.5f;
    float paramDustG_ = 0.82f;
    float paramDustB_ = 1.0f;
    float paramShellR_ = 1.0f;
    float paramShellG_ = 0.32f;
    float paramShellB_ = 0.92f;
    float paramFilamentR_ = 0.32f;
    float paramFilamentG_ = 1.0f;
    float paramFilamentB_ = 0.72f;

    bool enabled_ = true;
    bool hasAudio_ = false;
    bool hasWaveform_ = false;
    std::uint64_t lastAudioFrame_ = 0;
    std::vector<float> waveform_;
    std::vector<Particle> particles_;
    std::vector<std::size_t> shellOffsets_;
    std::vector<std::size_t> shellCounts_;
    float phase_ = 0.0f;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    float burst_ = 0.0f;
    float beatPulse_ = 0.0f;
    float lastPeakTime_ = -1000.0f;
    int lastBeatIndex_ = -1;
    std::uint32_t seedState_ = 2026;
};

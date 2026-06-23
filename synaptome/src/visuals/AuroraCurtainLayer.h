#pragma once

#include "Layer.h"

#include <cstdint>
#include <deque>
#include <vector>

class AuroraCurtainLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const std::string& label,
                       float min,
                       float max,
                       float step,
                       const std::string& units = std::string(),
                       const std::string& description = std::string());
    void readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b);
    void clampParams();
    void ensureWaveformSize(int sampleCount);
    void updateAudioState(float dt, float timeSeconds);
    void pushHistory();
    float sampleBuffer(const std::vector<float>& buffer, float normalizedIndex) const;
    float targetSampleFor(float normalizedIndex, float timeSeconds) const;
    glm::vec2 curtainPoint(const std::vector<float>& samples,
                           int curtainIndex,
                           int curtainCount,
                           float t,
                           float historyAge,
                           float timeSeconds,
                           float width,
                           float height,
                           bool mirror) const;
    void drawCurtain(const std::vector<float>& samples,
                     int curtainIndex,
                     int curtainCount,
                     int historyIndex,
                     int historyCount,
                     float width,
                     float height,
                     float timeSeconds,
                     float alpha,
                     bool mirror) const;
    void drawHorizonGlow(float width, float height, float alpha) const;
    void drawFlash(float width, float height, float alpha) const;

    bool paramEnabled_ = true;
    bool paramMirror_ = false;
    float paramAlpha_ = 1.0f;
    float paramCurtainCount_ = 3.0f;
    float paramSampleDensity_ = 220.0f;
    float paramWaveformGain_ = 0.38f;
    float paramVerticalScale_ = 0.34f;
    float paramCurtainHeight_ = 0.48f;
    float paramFoldStrength_ = 1.15f;
    float paramFlowSpeed_ = 0.12f;
    float paramNoiseScale_ = 2.8f;
    float paramTrailDecay_ = 0.20f;
    float paramLineThickness_ = 2.0f;
    float paramGlowAmount_ = 1.35f;
    float paramShimmerAmount_ = 0.65f;
    float paramMagneticTilt_ = -0.12f;
    float paramBassLift_ = 0.56f;
    float paramMidsFold_ = 1.05f;
    float paramHighsSparkle_ = 1.10f;
    float paramPeakFlash_ = 0.38f;
    float paramAudioAmount_ = 0.82f;
    float paramAudioSmoothing_ = 0.56f;
    float paramBgAlpha_ = 0.18f;
    float paramBgR_ = 0.003f;
    float paramBgG_ = 0.007f;
    float paramBgB_ = 0.018f;
    float paramColorR_ = 0.06f;
    float paramColorG_ = 0.95f;
    float paramColorB_ = 0.42f;
    float paramColor2R_ = 0.42f;
    float paramColor2G_ = 0.22f;
    float paramColor2B_ = 0.85f;

    bool enabled_ = true;
    bool hasAudio_ = false;
    bool hasWaveform_ = false;
    std::uint64_t lastAudioFrame_ = 0;
    std::vector<float> waveform_;
    std::vector<float> targetWaveform_;
    std::deque<std::vector<float>> history_;
    float historyAccumulator_ = 0.0f;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    float flash_ = 0.0f;
    float flashPhase_ = 0.5f;
    float lastPeakTime_ = -1000.0f;
};

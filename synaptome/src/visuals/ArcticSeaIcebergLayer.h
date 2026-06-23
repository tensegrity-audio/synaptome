#pragma once

#include "Layer.h"

#include <cstdint>
#include <vector>

class ArcticSeaIcebergLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Iceberg {
        float x = 0.0f;
        float depth = 0.0f;
        float width = 1.0f;
        float height = 1.0f;
        float speed = 0.0f;
        float seed = 0.0f;
        float lean = 0.0f;
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
    void resetIcebergs();
    void updateAudioState(float dt);
    void drawWater(float width, float height, float alpha, float timeSeconds) const;
    void drawHorizon(float width, float height, float alpha, float timeSeconds) const;
    void drawIceberg(const Iceberg& iceberg, float width, float height, float alpha, float timeSeconds) const;
    void drawPeakRipple(float width, float height, float alpha) const;
    float seaY(float height) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramSeaLevel_ = 0.66f;
    float paramWaterDepth_ = 0.34f;
    float paramHorizonGlow_ = 0.58f;
    float paramReflectionStrength_ = 0.72f;
    float paramWaveAmplitude_ = 0.44f;
    float paramWaveFrequency_ = 7.5f;
    float paramPerspectiveLines_ = 0.46f;
    float paramIcebergCount_ = 8.0f;
    float paramIcebergScale_ = 1.0f;
    float paramDriftSpeed_ = 0.030f;
    float paramRimLight_ = 0.78f;
    float paramMagentaAccent_ = 0.34f;
    float paramSparkleAmount_ = 0.48f;
    float paramPeakBloom_ = 0.50f;
    float paramBassWaves_ = 0.85f;
    float paramMidsDrift_ = 0.52f;
    float paramHighsSparkle_ = 0.90f;
    float paramAudioAmount_ = 0.82f;
    float paramAudioSmoothing_ = 0.48f;
    float paramSeed_ = 7331.0f;
    bool paramReseedRequested_ = false;
    float paramWaterR_ = 0.012f;
    float paramWaterG_ = 0.045f;
    float paramWaterB_ = 0.110f;
    float paramRimR_ = 0.10f;
    float paramRimG_ = 0.86f;
    float paramRimB_ = 1.0f;
    float paramAccentR_ = 0.88f;
    float paramAccentG_ = 0.18f;
    float paramAccentB_ = 0.86f;
    float paramAuroraR_ = 0.08f;
    float paramAuroraG_ = 1.0f;
    float paramAuroraB_ = 0.48f;

    bool enabled_ = true;
    bool hasAudio_ = false;
    std::uint32_t seedState_ = 7331;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    float ripple_ = 0.0f;
    float driftPhase_ = 0.0f;
    std::vector<Iceberg> icebergs_;
};

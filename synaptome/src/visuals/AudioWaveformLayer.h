#pragma once

#include "Layer.h"

#include <cstdint>
#include <vector>

class AudioWaveformLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    void clampParams();
    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const std::string& label,
                       float min,
                       float max,
                       float step,
                       const std::string& units = std::string());

    bool paramEnabled_ = true;
    bool paramShowBands_ = true;
    float paramGain_ = 3.0f;
    float paramVerticalScale_ = 0.85f;
    float paramLineThickness_ = 3.0f;
    float paramAlpha_ = 0.9f;
    float paramSmoothing_ = 0.35f;
    float paramBandHeight_ = 0.16f;
    float paramBandAlpha_ = 0.75f;
    float paramBgAlpha_ = 0.12f;
    float paramColorR_ = 0.1f;
    float paramColorG_ = 0.9f;
    float paramColorB_ = 0.72f;
    float paramBgColorR_ = 0.01f;
    float paramBgColorG_ = 0.02f;
    float paramBgColorB_ = 0.03f;

    bool enabled_ = true;
    bool hasSample_ = false;
    uint64_t lastFrame_ = 0;
    std::vector<float> waveform_;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
};

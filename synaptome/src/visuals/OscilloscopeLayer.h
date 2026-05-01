#pragma once

#include "Layer.h"
#include <deque>

class OscilloscopeLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    glm::vec2 basePatternPoint(float timeSeconds) const;
    void drawBackground(float radius, float baseAlpha) const;
    glm::vec2 applyModulation(const glm::vec2& basePoint) const;
    void clampParams();
    void appendInterpolatedSamples(const glm::vec2& targetPoint);
    void appendSample(const glm::vec2& point);

    bool paramEnabled_ = true;
    float paramPattern_ = 1.0f;
    float paramModMode_ = 1.0f;
    float paramXInput_ = 0.0f;
    float paramYInput_ = 0.0f;
    float paramSpeedInput_ = 0.0f;
    float paramBaseAmount_ = 1.0f;
    float paramModAmount_ = 0.5f;
    float paramRadialAmount_ = 0.4f;
    float paramWiggleAmount_ = 0.2f;
    float paramAmplitude_ = 0.8f;
    float paramSpeed_ = 1.0f;
    float paramSpeedModAmount_ = 0.75f;
    float paramFreqX_ = 1.0f;
    float paramFreqY_ = 1.0f;
    float paramPhaseOffsetDeg_ = 90.0f;
    float paramMorph_ = 0.0f;
    bool paramShowGrid_ = true;
    bool paramShowCrosshair_ = true;
    bool paramShowGlow_ = true;
    float paramGridDivisions_ = 4.0f;
    float paramGridAlpha_ = 0.2f;
    float paramGlowAlpha_ = 0.18f;
    float paramGlowRadius_ = 1.05f;
    float paramGlowFalloff_ = 0.65f;
    float paramBgColorR_ = 0.02f;
    float paramBgColorG_ = 0.08f;
    float paramBgColorB_ = 0.05f;
    float paramXScale_ = 1.0f;
    float paramYScale_ = 1.0f;
    float paramXBias_ = 0.0f;
    float paramYBias_ = 0.0f;
    float paramRotationDeg_ = 0.0f;
    float paramHistorySize_ = 256.0f;
    float paramSampleDensity_ = 4.0f;
    float paramThickness_ = 2.0f;
    float paramAlpha_ = 0.9f;
    float paramDecay_ = 0.65f;
    float paramIntensity_ = 1.0f;
    float paramPointSize_ = 3.0f;
    float paramColorR_ = 0.25f;
    float paramColorG_ = 1.0f;
    float paramColorB_ = 0.55f;

    bool enabled_ = true;
    bool hasLastPoint_ = false;
    glm::vec2 lastPoint_{ 0.0f, 0.0f };
    float phaseTime_ = 0.0f;
    std::deque<glm::vec2> history_;
};

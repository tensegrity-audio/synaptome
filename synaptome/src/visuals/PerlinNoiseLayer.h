#pragma once

#include "Layer.h"

class PerlinNoiseLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    float* scaleParamPtr() { return &paramScale_; }
    const float* scaleParamPtr() const { return &paramScale_; }
    float* speedParamPtr() { return &paramSpeed_; }
    const float* speedParamPtr() const { return &paramSpeed_; }
    float* brightnessParamPtr() { return &paramBrightness_; }
    const float* brightnessParamPtr() const { return &paramBrightness_; }
    float* contrastParamPtr() { return &paramContrast_; }
    const float* contrastParamPtr() const { return &paramContrast_; }
    float* alphaParamPtr() { return &paramAlpha_; }
    const float* alphaParamPtr() const { return &paramAlpha_; }
    float* texelZoomParamPtr() { return &paramTexelZoom_; }
    const float* texelZoomParamPtr() const { return &paramTexelZoom_; }
    float* colorRParamPtr() { return &paramColorR_; }
    const float* colorRParamPtr() const { return &paramColorR_; }
    float* colorGParamPtr() { return &paramColorG_; }
    const float* colorGParamPtr() const { return &paramColorG_; }
    float* colorBParamPtr() { return &paramColorB_; }
    const float* colorBParamPtr() const { return &paramColorB_; }
    float* octavesParamPtr() { return &paramOctaves_; }
    const float* octavesParamPtr() const { return &paramOctaves_; }
    float* lacunarityParamPtr() { return &paramLacunarity_; }
    const float* lacunarityParamPtr() const { return &paramLacunarity_; }
    float* persistenceParamPtr() { return &paramPersistence_; }
    const float* persistenceParamPtr() const { return &paramPersistence_; }
    float* paletteIndexParamPtr() { return &paramPaletteIndex_; }
    const float* paletteIndexParamPtr() const { return &paramPaletteIndex_; }
    float* paletteRateParamPtr() { return &paramPaletteRate_; }
    const float* paletteRateParamPtr() const { return &paramPaletteRate_; }
    int paletteCount() const;

private:
    void allocateTexture();
    void refreshPixels(float timeOffset);
    void applyTexelZoom();

    bool paramEnabled_ = true;
    float paramScale_ = 3.0f;
    float paramSpeed_ = 0.25f;
    float paramBrightness_ = 0.8f;
    float paramContrast_ = 1.0f;
    float paramAlpha_ = 1.0f;
    float paramTexelZoom_ = 1.0f;
    float paramColorR_ = 0.1f;
    float paramColorG_ = 0.6f;
    float paramColorB_ = 1.0f;
    float paramOctaves_ = 3.0f;
    float paramLacunarity_ = 2.0f;
    float paramPersistence_ = 0.5f;
    float paramPaletteIndex_ = 0.0f;
    float paramPaletteRate_ = 0.0f;

    bool enabled_ = true;
    glm::ivec2 textureSize_{ 256, 256 };
    glm::ivec2 baseTextureSize_{ 256, 256 };
    float lastTexelZoom_ = 1.0f;
    ofFloatPixels pixels_;
    ofTexture texture_;
    float noiseZ_ = 0.0f;
    float palettePhase_ = 0.0f;
};

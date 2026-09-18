#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "WaveTankModel.h"
#include <cstdint>

class WaveTankLayer final : public Layer, public synaptome::element::ParameterBindable {
public:
    void configure(const ofJson& config) override;
    void bindParameters(synaptome::element::ParameterBinder& binder) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
private:
    float speed_ = 0.8f;
    bool paused_ = false;
    bool bpmSync_ = true;
    float bpmMultiplier_ = 1.0f;
    float seed_ = 1001.0f;
    bool reseed_ = false;
    float scale_ = 0.94f;
    float rotationDeg_ = 0.0f;
    float colorR_ = 0.08f;
    float colorG_ = 0.72f;
    float colorB_ = 1.0f;
    float waveSpeed_ = 16.0f;
    float damping_ = 0.55f;
    float driveStrength_ = 5.0f;
    float driveFrequency_ = 0.8f;
    float sourceSpread_ = 0.6f;
    float radiance_ = 1.15f;
    float contourCount_ = 12.0f;
    float contourOpacity_ = 0.55f;
    synaptome_show_wave_tank::WaveTankModel model_;
    ofMesh mesh_;
    std::uint32_t activeSeed_ = 0;
};

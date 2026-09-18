#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "PhaseLatticeModel.h"
#include <cstdint>

class PhaseLatticeLayer final : public Layer, public synaptome::element::ParameterBindable {
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
    float colorR_ = 0.52f;
    float colorG_ = 0.32f;
    float colorB_ = 1.0f;
    float coupling_ = 2.4f;
    float frequency_ = 0.28f;
    float dispersion_ = 0.5f;
    float phaseLagDeg_ = 30.0f;
    float radiance_ = 1.1f;
    float contourOpacity_ = 0.5f;
    synaptome_show_phase_lattice::PhaseLatticeModel model_;
    ofMesh mesh_;
    std::uint32_t activeSeed_ = 0;
};

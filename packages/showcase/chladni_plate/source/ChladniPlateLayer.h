#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "ChladniPlateModel.h"
#include <cstdint>

class ChladniPlateLayer final : public Layer, public synaptome::element::ParameterBindable {
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
    float colorR_ = 0.98f;
    float colorG_ = 0.72f;
    float colorB_ = 0.24f;
    float modeX_ = 3.0f;
    float modeY_ = 5.0f;
    float attraction_ = 2.5f;
    float agitation_ = 0.16f;
    float phaseRate_ = 0.12f;
    float particleRadius_ = 0.003f;
    float radiance_ = 1.1f;
    synaptome_show_chladni_plate::ChladniPlateModel model_;
    ofMesh mesh_;
    std::uint32_t activeSeed_ = 0;
};

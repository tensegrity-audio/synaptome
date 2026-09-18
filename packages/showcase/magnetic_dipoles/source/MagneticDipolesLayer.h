#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "MagneticDipolesModel.h"

class MagneticDipolesLayer final : public Layer, public synaptome::element::ParameterBindable {
public:
    void configure(const ofJson& config) override;
    void bindParameters(synaptome::element::ParameterBinder& binder) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override { enabled_ = enabled; }
    std::uint64_t debugStateSignature() const { return model_.stateSignature(); }
private:
    float speed_ = 1.0f;
    bool paused_ = false;
    bool bpmSync_ = false;
    float bpmMultiplier_ = 1.0f;
    float seed_ = 155921.0f;
    bool reseed_ = false;
    float scale_ = 0.96f;
    float rotationDeg_ = 0.0f;
    float lineWidth_ = 1.6f;
    float glow_ = 1.0f;
    float colorR_ = 0.24f;
    float colorG_ = 0.78f;
    float colorB_ = 0.98f;
    float accentR_ = 0.95f;
    float accentG_ = 0.45f;
    float accentB_ = 0.64f;
    bool enabled_ = true;
    std::uint32_t appliedSeed_ = 0;
    synaptome_show_magnetic::MagneticDipolesModel model_;
    synaptome_show_magnetic::MagneticDipolesModel::Controls controls_;
    ofMesh mesh_;
};

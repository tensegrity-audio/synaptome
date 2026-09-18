#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "StrangeAttractorModel.h"

class StrangeAttractorLayer final : public Layer, public synaptome::element::ParameterBindable {
public:
    using Model = synaptome::showcase::StrangeAttractorModel;
    void configure(const ofJson&) override;
    void bindParameters(synaptome::element::ParameterBinder&) override;
    void setup(ParameterRegistry&) override;
    void update(const LayerUpdateParams&) override;
    void draw(const LayerDrawParams&) override;
    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool value) override { enabled_ = value; }
    const Model& model() const { return model_; }
    std::uint64_t stateSignature() const { return model_.stateSignature(); }
private:
    Model::Controls controls() const;
    Model model_;
    ofMesh mesh_, edges_;
    bool enabled_ = true;
    std::uint32_t lastSeed_ = 1001;
    double spinAngle_ = 0;
    float speed_ = 0.65f;
    bool bpmSync_ = true;
    float bpmMultiplier_ = 1.0f;
    float scale_ = 1.0f;
    float yawDeg_ = -24.0f;
    float tiltDeg_ = 38.0f;
    float spinRate_ = 3.0f;
    float seed_ = 1001.0f;
    bool reseed_ = false;
    float sigma_ = 10.0f;
    float rho_ = 28.0f;
    float beta_ = 2.666666667f;
    float trailSeconds_ = 5.5f;
    float ribbonWidth_ = 0.006f;
    float colorR_ = 0.08f;
    float colorG_ = 0.38f;
    float colorB_ = 0.8f;
    float highlightR_ = 0.65f;
    float highlightG_ = 0.98f;
    float highlightB_ = 0.9f;
};

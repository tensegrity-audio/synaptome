#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "ofMain.h"
#include "VoronoiFoamModel.h"

class VoronoiFoamLayer final : public Layer, public synaptome::element::ParameterBindable {
public:
    void configure(const ofJson& definition) override;
    void bindParameters(synaptome::element::ParameterBinder& binder) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
private:
    float speed_ = 0.8f;
    bool bpmSync_ = true;
    float bpmMultiplier_ = 1.0f;
    float scale_ = 0.92f;
    float rotationDeg_ = 0.0f;
    float driftSpeed_ = 0.35f;
    float relaxationRate_ = 0.7f;
    float repulsionWeight_ = 0.5f;
    float shear_ = 0.15f;
    float wallWidth_ = 0.024f;
    float cellFill_ = 0.14f;
    float edgeRadiance_ = 1.0f;
    float colorR_ = 0.15f;
    float colorG_ = 0.94f;
    float colorB_ = 0.68f;
    float accentR_ = 0.35f;
    float accentG_ = 0.2f;
    float accentB_ = 1.0f;
    float seed_ = 1703.0f;
    std::uint32_t activeSeed_ = 1703;
    VoronoiFoamModel model_;
    ofMesh mesh_;
};

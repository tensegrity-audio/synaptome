#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "ofMain.h"
#include "SandRipplesModel.h"

class SandRipplesLayer final : public Layer, public synaptome::element::ParameterBindable {
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
    float transportRate_ = 1.0f;
    float windDirectionDeg_ = 12.0f;
    float hopLength_ = 0.12f;
    float reposeSlope_ = 0.025f;
    float relaxationRate_ = 1.5f;
    float reliefScale_ = 1.0f;
    float tiltDeg_ = 48.0f;
    float ridgeContrast_ = 1.0f;
    float colorR_ = 0.8f;
    float colorG_ = 0.37f;
    float colorB_ = 0.12f;
    float accentR_ = 1.0f;
    float accentG_ = 0.86f;
    float accentB_ = 0.46f;
    float seed_ = 1702.0f;
    std::uint32_t activeSeed_ = 1702;
    SandRipplesModel model_;
    ofMesh mesh_;
};

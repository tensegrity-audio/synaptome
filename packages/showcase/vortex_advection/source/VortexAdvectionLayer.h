#pragma once
#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include "ofMain.h"
#include "VortexAdvectionModel.h"

class VortexAdvectionLayer final : public Layer, public synaptome::element::ParameterBindable {
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
    float circulation_ = 1.0f;
    float coreRadius_ = 0.085f;
    float strain_ = 0.18f;
    float vortexDrift_ = 0.4f;
    float trailTimeSec_ = 1.1f;
    float lineWidth_ = 1.3f;
    float colorR_ = 0.05f;
    float colorG_ = 0.8f;
    float colorB_ = 1.0f;
    float accentR_ = 0.95f;
    float accentG_ = 0.18f;
    float accentB_ = 0.52f;
    float seed_ = 1701.0f;
    std::uint32_t activeSeed_ = 1701;
    VortexAdvectionModel model_;
    ofMesh mesh_;
};

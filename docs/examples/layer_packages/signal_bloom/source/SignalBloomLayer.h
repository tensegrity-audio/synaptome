#pragma once

#include <synaptome/element/ParameterBinding.h>
#include <synaptome/element/compat/Layer.h>
#include <vector>

class SignalBloomLayer final
    : public Layer,
      public synaptome::element::ParameterBindable {
public:
    void configure(const ofJson& config) override;
    void bindParameters(
        synaptome::element::ParameterBinder& binder) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return visible_; }
    void setExternalEnabled(bool enabled) override;

private:
    bool visible_ = true;
    bool bpmSync_ = true;
    float speed_ = 0.65f;
    float bpmMultiplier_ = 1.0f;
    float scale_ = 0.82f;
    float rotationDeg_ = 0.0f;
    float alpha_ = 0.86f;
    float gain_ = 0.62f;
    float colorR_ = 0.12f;
    float colorG_ = 0.78f;
    float colorB_ = 1.0f;
    float bgColorR_ = 0.02f;
    float bgColorG_ = 0.04f;
    float bgColorB_ = 0.08f;
    float lineOpacity_ = 0.72f;
    float xInput_ = 0.5f;
    float yInput_ = 0.5f;
    float speedInput_ = 0.0f;

    float phase_ = 0.0f;
    std::vector<glm::vec2> points_;
    ofMesh lineMesh_;
    ofMesh dotMesh_;
};

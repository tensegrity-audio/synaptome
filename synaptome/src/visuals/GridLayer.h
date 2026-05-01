#pragma once

#include "Layer.h"
#include <vector>

class GridLayer : public Layer {
public:
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    void cycleSegments();

    bool* enabledParamPtr() { return &paramEnabled_; }
    float* segmentsParamPtr() { return &paramSegments_; }
    float* faceOpacityParamPtr() { return &paramFaceOpacity_; }

private:
    bool paramEnabled_ = true;
    float paramSegments_ = 100.0f;
    float paramFaceOpacity_ = 0.0f;

    bool enabled_ = true;
    int segments_ = 100;
    float faceOpacity_ = 0.0f;
    float gridSize_ = 800.0f;

    std::vector<glm::vec3> waveVerts_;

    int indexFor(int x, int z) const;
    void ensureVertexCapacity();
};

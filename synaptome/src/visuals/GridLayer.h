#pragma once

#include "Layer.h"
#include <string>
#include <vector>

class GridLayer : public Layer {
public:
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;
    std::string deformationSummary() const;

    void cycleSegments();

    bool* enabledParamPtr() { return &paramEnabled_; }
    float* segmentsParamPtr() { return &paramSegments_; }
    bool* waveParamPtr() { return &paramWave_; }
    float* waveAmountParamPtr() { return &paramWaveAmount_; }
    float* waveFrequencyParamPtr() { return &paramWaveFrequency_; }
    float* waveSpeedParamPtr() { return &paramWaveSpeed_; }
    bool* bendParamPtr() { return &paramBend_; }
    float* bendAmountParamPtr() { return &paramBendAmount_; }
    float* bendFrequencyParamPtr() { return &paramBendFrequency_; }
    float* bendSpeedParamPtr() { return &paramBendSpeed_; }
    bool* deformParamPtr() { return &paramDeform_; }
    float* deformAmountParamPtr() { return &paramDeformAmount_; }
    float* deformScaleParamPtr() { return &paramDeformScale_; }
    float* deformSpeedParamPtr() { return &paramDeformSpeed_; }
    bool* twistParamPtr() { return &paramTwist_; }
    float* twistAmountParamPtr() { return &paramTwistAmount_; }
    float* twistSpeedParamPtr() { return &paramTwistSpeed_; }
    bool* bulgeParamPtr() { return &paramBulge_; }
    float* bulgeAmountParamPtr() { return &paramBulgeAmount_; }
    float* bulgeRadiusParamPtr() { return &paramBulgeRadius_; }
    float* bulgeSpeedParamPtr() { return &paramBulgeSpeed_; }
    float* faceOpacityParamPtr() { return &paramFaceOpacity_; }

private:
    bool paramEnabled_ = true;
    float paramSegments_ = 100.0f;
    bool paramWave_ = true;
    float paramWaveAmount_ = 10.0f;
    float paramWaveFrequency_ = 3.0f;
    float paramWaveSpeed_ = 2.0f;
    bool paramBend_ = true;
    float paramBendAmount_ = 8.0f;
    float paramBendFrequency_ = 0.01f;
    float paramBendSpeed_ = 1.2f;
    bool paramDeform_ = false;
    float paramDeformAmount_ = 24.0f;
    float paramDeformScale_ = 1.4f;
    float paramDeformSpeed_ = 0.35f;
    bool paramTwist_ = false;
    float paramTwistAmount_ = 32.0f;
    float paramTwistSpeed_ = 0.35f;
    bool paramBulge_ = false;
    float paramBulgeAmount_ = 32.0f;
    float paramBulgeRadius_ = 0.85f;
    float paramBulgeSpeed_ = 0.25f;
    float paramFaceOpacity_ = 0.0f;

    bool enabled_ = true;
    int segments_ = 100;
    bool wave_ = true;
    float waveAmount_ = 10.0f;
    float waveFrequency_ = 3.0f;
    float waveSpeed_ = 2.0f;
    bool bend_ = true;
    float bendAmount_ = 8.0f;
    float bendFrequency_ = 0.01f;
    float bendSpeed_ = 1.2f;
    bool deform_ = false;
    float deformAmount_ = 24.0f;
    float deformScale_ = 1.4f;
    float deformSpeed_ = 0.35f;
    bool twist_ = false;
    float twistAmount_ = 32.0f;
    float twistSpeed_ = 0.35f;
    bool bulge_ = false;
    float bulgeAmount_ = 32.0f;
    float bulgeRadius_ = 0.85f;
    float bulgeSpeed_ = 0.25f;
    float faceOpacity_ = 0.0f;
    float gridSize_ = 800.0f;

    std::vector<glm::vec3> waveVerts_;

    int indexFor(int x, int z) const;
    void ensureVertexCapacity();
};

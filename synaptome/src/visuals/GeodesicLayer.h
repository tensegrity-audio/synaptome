#pragma once

#include "Layer.h"

class GeodesicLayer : public Layer {
public:
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    void incrementSubdivision();
    void decrementSubdivision();

    bool* enabledParamPtr() { return &paramEnabled_; }
    float* spinParamPtr() { return &paramSpinDeg_; }
    float* hoverParamPtr() { return &paramHoverAmp_; }
    float* baseHeightParamPtr() { return &paramBaseHeight_; }
    float* orbitRadiusParamPtr() { return &paramOrbitRadius_; }
    float* orbitSpeedParamPtr() { return &paramOrbitSpeedDeg_; }
    float* rotateXParamPtr() { return &paramRotateXDeg_; }
    float* rotateYParamPtr() { return &paramRotateYDeg_; }
    float* rotateZParamPtr() { return &paramRotateZDeg_; }
    float* radiusParamPtr() { return &paramRadius_; }
    float* lineOpacityParamPtr() { return &paramLineOpacity_; }
    float* faceOpacityParamPtr() { return &paramFaceOpacity_; }

    int subdivisions() const { return subdivisions_; }

private:
    void rebuildGeodesic();

    bool paramEnabled_ = true;
    float paramSpinDeg_ = 25.0f;
    float paramHoverAmp_ = 20.0f;
    float paramBaseHeight_ = 140.0f;
    float paramOrbitRadius_ = 0.0f;
    float paramOrbitSpeedDeg_ = 0.0f;
    float paramRotateXDeg_ = 0.0f;
    float paramRotateYDeg_ = 0.0f;
    float paramRotateZDeg_ = 0.0f;
    float paramRadius_ = 140.0f;
    float paramLineOpacity_ = 1.0f;
    float paramFaceOpacity_ = 0.0f;

    bool enabled_ = true;
    float spinDeg_ = 25.0f;
    float hoverAmp_ = 20.0f;
    float baseHeight_ = 140.0f;
    float orbitRadius_ = 0.0f;
    float orbitSpeedDeg_ = 0.0f;
    float rotateXDeg_ = 0.0f;
    float rotateYDeg_ = 0.0f;
    float rotateZDeg_ = 0.0f;
    float radius_ = 140.0f;
    float lineOpacity_ = 1.0f;
    float faceOpacity_ = 0.0f;
    int subdivisions_ = 2;

    ofIcoSpherePrimitive ico_;
    ofVboMesh mesh_;
};

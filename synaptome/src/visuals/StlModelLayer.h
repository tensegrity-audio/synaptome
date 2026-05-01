#pragma once

#include "Layer.h"

class StlModelLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    bool loadMesh();
    bool loadBinaryStl(const std::string& path, ofMesh& mesh);
    bool loadAsciiStl(const std::string& path, ofMesh& mesh);
    void normalizeMesh(ofMesh& mesh);
    void appendTriangle(ofMesh& mesh,
                        const glm::vec3& a,
                        const glm::vec3& b,
                        const glm::vec3& c);

    std::string assetPath_ = "models/lowpoly_tetra.stl";

    bool paramEnabled_ = true;
    float paramSpinDeg_ = 18.0f;
    float paramHoverAmp_ = 16.0f;
    float paramBaseHeight_ = 140.0f;
    float paramOrbitRadius_ = 0.0f;
    float paramOrbitSpeedDeg_ = 0.0f;
    float paramRotateXDeg_ = 0.0f;
    float paramRotateYDeg_ = 0.0f;
    float paramRotateZDeg_ = 0.0f;
    float paramColorR_ = 255.0f;
    float paramColorG_ = 140.0f;
    float paramColorB_ = 0.0f;
    float paramScale_ = 180.0f;
    float paramLineOpacity_ = 1.0f;
    float paramFaceOpacity_ = 0.18f;

    bool enabled_ = true;
    float spinDeg_ = 18.0f;
    float hoverAmp_ = 16.0f;
    float baseHeight_ = 140.0f;
    float orbitRadius_ = 0.0f;
    float orbitSpeedDeg_ = 0.0f;
    float rotateXDeg_ = 0.0f;
    float rotateYDeg_ = 0.0f;
    float rotateZDeg_ = 0.0f;
    ofColor liveColor_ = ofColor(255, 140, 0);
    float scale_ = 180.0f;
    float lineOpacity_ = 1.0f;
    float faceOpacity_ = 0.18f;

    bool meshLoaded_ = false;
    ofVboMesh mesh_;
};

#pragma once

#include "Layer.h"

#include <cstdint>
#include <vector>

class MountainIslandLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Peak {
        glm::vec2 position;
        float radius = 1.0f;
        float height = 1.0f;
    };

    struct CloudCluster {
        glm::vec3 position;
        glm::vec2 velocity;
        float baseY = 0.0f;
        float yaw = 0.0f;
        float seed = 0.0f;
        float scale = 1.0f;
        float bobPhase = 0.0f;
        ofMesh mesh;
    };

    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const std::string& label,
                       float min,
                       float max,
                       float step,
                       const std::string& units = std::string(),
                       const std::string& description = std::string());
    void readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b);
    void clampParams();
    void rebuildIsland();
    void resetClouds();
    void updateAudioState(float dt);
    void updateClouds(float dt, float timeSeconds);
    float growthAmount() const;
    float meshSignature() const;
    float cloudSignature() const;
    float shelfTopY() const;
    float sphereRadiusAtY(float y) const;
    float landLayerRadius() const;
    float hemisphereYForRadius(float radius) const;
    float seaFloorBaseYFor(const glm::vec2& point) const;
    glm::vec3 grownTerrainPoint(const glm::vec3& basePoint, const glm::vec3& finalPoint, float growth) const;

    float terrainHeightFor(const glm::vec2& point, float normalizedRadius, float baseY) const;
    float terrainHeightAt(const glm::vec2& point) const;
    glm::vec2 terrainGradientAt(const glm::vec2& point) const;
    ofFloatColor terrainColorForPoint(const glm::vec3& point,
                                      const glm::vec3& normal,
                                      float alpha) const;
    ofFloatColor terrainColorFor(const glm::vec3& a,
                                 const glm::vec3& b,
                                 const glm::vec3& c,
                                 float alpha) const;
    void buildCloudMesh(CloudCluster& cloud, std::mt19937& rng) const;

    void drawSky(const LayerDrawParams& params, float alpha) const;
    void drawWaterDisk(float alpha, float timeSeconds) const;
    void drawWaterHighlights(float alpha, float timeSeconds) const;
    void drawSolidWorld(float alpha) const;
    void drawIsland(float alpha) const;
    void drawShore(float alpha) const;
    void drawCloudShadows(float alpha, float timeSeconds) const;
    void drawClouds(float alpha, float timeSeconds) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramSceneScale_ = 1.0f;
    float paramSceneOffsetY_ = -28.0f;
    float paramSceneOffsetZ_ = 0.0f;
    float paramSpinAngle_ = 0.0f;
    float paramSpinSpeed_ = 3.2f;

    float paramWaterRadius_ = 820.0f;
    float paramWaterLevel_ = 0.0f;
    float paramWaterRimDepth_ = 76.0f;
    float paramWaterHighlight_ = 0.72f;
    float paramWaterWaveAmount_ = 3.5f;
    float paramShoreGlow_ = 0.78f;
    float paramWorldDepth_ = 820.0f;
    float paramSubmergedLandDepth_ = 76.0f;
    float paramSolidWorldAlpha_ = 0.92f;

    float paramIslandRadius_ = 430.0f;
    float paramPointCount_ = 720.0f;
    float paramBoundaryPoints_ = 144.0f;
    float paramTriangleTargetLength_ = 90.0f;
    float paramMountainHeight_ = 310.0f;
    float paramRoughness_ = 0.58f;
    float paramShorelineJitter_ = 0.22f;
    float paramPeakCount_ = 5.0f;
    float paramSnowLine_ = 0.76f;
    float paramTreeLine_ = 0.42f;
    float paramSandHeight_ = 115.0f;
    float paramUpliftRadius_ = 0.88f;
    float paramUpliftScatter_ = 0.82f;
    float paramUpliftFalloff_ = 0.95f;
    float paramSeabedUndulation_ = 22.0f;
    float paramEdgeUndulation_ = 34.0f;
    float paramWireAlpha_ = 0.18f;
    float paramGrowth_ = 0.55f;
    bool paramAutoGrow_ = true;
    bool paramGrowthReseedRequested_ = false;
    float paramGrowthRate_ = 0.035f;
    float paramGrowthCurve_ = 1.15f;

    float paramCloudCount_ = 7.0f;
    float paramCloudPuffCount_ = 11.0f;
    float paramCloudFacetSegments_ = 12.0f;
    float paramCloudFacetRings_ = 7.0f;
    float paramCloudScale_ = 1.0f;
    float paramCloudDensity_ = 0.92f;
    float paramCloudBaseHeight_ = 520.0f;
    float paramCloudLayerDepth_ = 220.0f;
    float paramCloudClearance_ = 150.0f;
    float paramCloudWindAngle_ = -28.0f;
    float paramCloudWindSpeed_ = 24.0f;
    float paramCloudTurbulence_ = 0.62f;
    float paramCloudMountainAvoidance_ = 1.65f;
    float paramCloudUpdraft_ = 0.0f;
    float paramCloudShadowAlpha_ = 0.06f;

    float paramAudioAmount_ = 0.55f;
    float paramAudioSmoothing_ = 0.42f;
    float paramBassLift_ = 0.42f;
    float paramHighsGlint_ = 0.86f;

    bool paramReseedRequested_ = false;
    float paramSeed_ = 20260623.0f;

    float paramSkyTopR_ = 0.015f;
    float paramSkyTopG_ = 0.026f;
    float paramSkyTopB_ = 0.045f;
    float paramSkyHorizonR_ = 0.18f;
    float paramSkyHorizonG_ = 0.28f;
    float paramSkyHorizonB_ = 0.32f;
    float paramWaterR_ = 0.030f;
    float paramWaterG_ = 0.240f;
    float paramWaterB_ = 0.300f;
    float paramShallowR_ = 0.160f;
    float paramShallowG_ = 0.620f;
    float paramShallowB_ = 0.560f;
    float paramShoreR_ = 0.720f;
    float paramShoreG_ = 0.610f;
    float paramShoreB_ = 0.360f;
    float paramLowlandR_ = 0.160f;
    float paramLowlandG_ = 0.440f;
    float paramLowlandB_ = 0.240f;
    float paramRockR_ = 0.360f;
    float paramRockG_ = 0.330f;
    float paramRockB_ = 0.310f;
    float paramSnowR_ = 0.860f;
    float paramSnowG_ = 0.910f;
    float paramSnowB_ = 0.920f;
    float paramWireR_ = 0.660f;
    float paramWireG_ = 0.920f;
    float paramWireB_ = 0.880f;
    float paramCloudR_ = 1.5f;
    float paramCloudG_ = 1.5f;
    float paramCloudB_ = 1.5f;
    float paramCloudShadeR_ = 1.5f;
    float paramCloudShadeG_ = 1.5f;
    float paramCloudShadeB_ = 1.5f;

    bool enabled_ = true;
    bool hasAudio_ = false;
    std::uint32_t seedState_ = 0;
    int pointCountState_ = -1;
    int boundaryPointState_ = -1;
    int peakCountState_ = -1;
    float meshSignatureState_ = -1.0f;
    int cloudCountState_ = -1;
    int cloudPuffCountState_ = -1;
    float cloudSignatureState_ = -1.0f;
    float spinPhase_ = 0.0f;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;

    std::vector<Peak> peaks_;
    std::vector<glm::vec2> shoreline2d_;
    std::vector<glm::vec3> shoreline3d_;
    ofMesh seaFloorBaseMesh_;
    ofMesh islandMesh_;
    ofMesh islandSkirt_;
    ofMesh ridgeLines_;
    ofMesh shoreLine_;
    std::vector<CloudCluster> clouds_;
};

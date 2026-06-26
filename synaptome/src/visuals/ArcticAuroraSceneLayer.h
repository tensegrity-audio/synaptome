#pragma once

#include "Layer.h"

#include <cstdint>
#include <string>
#include <vector>

class ArcticAuroraSceneLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Iceberg {
        glm::vec3 position;
        float orbitRadius = 0.0f;
        float orbitAngle = 0.0f;
        float driftSpeed = 0.0f;
        float bobPhase = 0.0f;
        float bobAmount = 0.0f;
        float yaw = 0.0f;
        float scale = 1.0f;
        float seed = 0.0f;
        ofMesh aboveWater;
        ofMesh belowWater;
        ofMesh rimLines;
    };

    struct Star {
        glm::vec2 position;
        float size = 1.0f;
        float alpha = 1.0f;
        float twinkle = 0.0f;
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
    void resetLayout();
    void updateAudioState(float dt);

    void drawSky(const LayerDrawParams& params, float alpha) const;
    void drawAuroraVolume(const LayerDrawParams& params, float alpha) const;
    void drawWaterPlane(const LayerDrawParams& params, float alpha) const;
    void drawWaterReflection(const LayerDrawParams& params, float alpha) const;
    void drawWaterHorizonMist(float alpha) const;
    float waterSurfaceYAt(float x, float z, float time, float waveScale = 1.0f) const;
    void buildIcebergMeshes(Iceberg& iceberg, const glm::vec3& dimensions, std::mt19937& rng);
    glm::vec3 icebergWorldPosition(const Iceberg& iceberg) const;
    float icebergWorldYaw(const Iceberg& iceberg) const;
    void drawIceberg(const Iceberg& iceberg, float alpha) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramSceneScale_ = 1.0f;
    float paramSceneOffsetY_ = 0.0f;
    float paramSceneOffsetZ_ = 0.0f;

    float paramWaterWidth_ = 4200.0f;
    float paramWaterNearZ_ = 260.0f;
    float paramWaterFarZ_ = -1450.0f;
    float paramWaterLevel_ = 0.0f;
    float paramWaterWaveIdle_ = 3.2f;
    float paramWaterHighlight_ = 1.15f;
    float paramWaterReflection_ = 0.0f;
    float paramWaterHorizonFog_ = 0.0f;
    float paramWaterAlpha_ = 0.94f;
    float paramWaterBrightness_ = 1.18f;
    float paramWaterTranslucency_ = 0.72f;
    float paramWaterCurvature_ = 1.0f;
    float paramWaterHemisphereDepth_ = 2100.0f;
    float paramWaterNoiseAmount_ = 2.8f;
    float paramWaterNoiseScale_ = 0.0038f;
    float paramWaterRippleAmount_ = 4.0f;
    float paramWaterRippleRadius_ = 420.0f;
    float paramWaterAuroraLight_ = 0.55f;

    float paramIcebergCount_ = 6.0f;
    float paramIcebergScale_ = 1.0f;
    float paramIcebergSpread_ = 3400.0f;
    float paramIcebergRimLight_ = 0.0f;

    float paramAuroraWidth_ = 2600.0f;
    float paramAuroraBaseY_ = 220.0f;
    float paramAuroraHeight_ = 560.0f;
    float paramAuroraDepthNear_ = -780.0f;
    float paramAuroraDepthFar_ = -1260.0f;
    float paramAuroraGlow_ = 2.2f;
    float paramAuroraBloom_ = 2.6f;
    float paramAuroraFoldStrength_ = 0.92f;
    float paramAuroraRayDensity_ = 0.0f;
    float paramAuroraCurtainCount_ = 4.0f;

    float paramAudioAmount_ = 1.35f;
    float paramAudioSmoothing_ = 0.32f;

    bool paramReseedRequested_ = false;
    float paramSeed_ = 20260621.0f;

    float paramSkyTopR_ = 0.0f;
    float paramSkyTopG_ = 0.004f;
    float paramSkyTopB_ = 0.015f;
    float paramSkyHorizonR_ = 0.010f;
    float paramSkyHorizonG_ = 0.028f;
    float paramSkyHorizonB_ = 0.055f;
    float paramWaterR_ = 0.055f;
    float paramWaterG_ = 0.155f;
    float paramWaterB_ = 0.265f;
    float paramAuroraR_ = 0.08f;
    float paramAuroraG_ = 1.0f;
    float paramAuroraB_ = 0.52f;
    float paramAurora2R_ = 0.48f;
    float paramAurora2G_ = 0.18f;
    float paramAurora2B_ = 0.92f;
    float paramIceRimR_ = 0.12f;
    float paramIceRimG_ = 0.88f;
    float paramIceRimB_ = 1.0f;
    float paramIceAccentR_ = 0.20f;
    float paramIceAccentG_ = 0.42f;
    float paramIceAccentB_ = 0.62f;

    bool enabled_ = true;
    std::uint32_t seedState_ = 0;
    int icebergCountState_ = -1;
    float sceneTime_ = 0.0f;

    bool hasAudio_ = false;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    float auroraEnergy_ = 0.16f;
    float auroraPulse_ = 0.0f;
    std::vector<float> energyField_;
    std::vector<float> targetEnergyField_;

    std::vector<Iceberg> icebergs_;
    std::vector<Star> stars_;
};

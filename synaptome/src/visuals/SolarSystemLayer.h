#pragma once

#include "Layer.h"

#include <cstdint>
#include <string>
#include <vector>

class SolarSystemLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
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
    void resetSystem();
    void initializeLifeStates();
    void updateAudioState(float dt, float timeSeconds);
    float waveformSampleFor(float normalizedIndex) const;
    std::uint32_t nextRuntimeSeed() const;

    struct Moon {
        float radius = 1.0f;
        float distance = 1.0f;
        float speed = 1.0f;
        float phase = 0.0f;
        float inclination = 0.0f;
        float seed = 0.0f;
        ofFloatColor color;
    };

    struct Body {
        std::string sourceName;
        float observedRadiusEarth = 1.0f;
        float observedMassEarth = 1.0f;
        float observedOrbitAu = 1.0f;
        float observedPeriodDays = 365.0f;
        float orbit = 0.0f;
        float radius = 1.0f;
        float speed = 1.0f;
        float phase = 0.0f;
        float eccentricity = 0.0f;
        float inclination = 0.0f;
        float orbitYaw = 0.0f;
        float axialTilt = 0.0f;
        float spin = 1.0f;
        float seed = 0.0f;
        bool rings = false;
        float ringInner = 1.6f;
        float ringOuter = 2.5f;
        float ringTilt = 0.0f;
        int ringBands = 0;
        float atmosphere = 0.0f;
        float banding = 0.0f;
        float storm = 0.0f;
        ofFloatColor color;
        ofFloatColor accentColor;
        std::vector<Moon> moons;
    };

    struct Asteroid {
        float orbit = 0.0f;
        float angle = 0.0f;
        float speed = 0.0f;
        float eccentricity = 0.0f;
        float inclination = 0.0f;
        float orbitYaw = 0.0f;
        float radius = 1.0f;
        float seed = 0.0f;
        ofFloatColor color;
    };

    struct Comet {
        float orbit = 0.0f;
        float angle = 0.0f;
        float speed = 0.0f;
        float eccentricity = 0.78f;
        float inclination = 0.0f;
        float orbitYaw = 0.0f;
        float radius = 1.0f;
        float tail = 1.0f;
        float seed = 0.0f;
        ofFloatColor color;
    };

    struct BackgroundStar {
        glm::vec2 position;
        float size = 1.0f;
        float twinkle = 1.0f;
        float depth = 1.0f;
        float drift = 0.0f;
        ofFloatColor color;
    };

    struct Visitor {
        int type = 0;
        float cycle = 80.0f;
        float phase = 0.0f;
        float duration = 7.0f;
        float angle = 0.0f;
        float inclination = 0.0f;
        float yaw = 0.0f;
        float impact = 0.0f;
        float size = 1.0f;
        float seed = 0.0f;
        ofFloatColor color;
    };

    struct Artifact {
        int type = 0;
        int bodyIndex = 0;
        float delay = 30.0f;
        float phase = 0.0f;
        float orbitDistance = 2.8f;
        float speed = 1.0f;
        float inclination = 0.0f;
        float size = 1.0f;
        float seed = 0.0f;
        ofFloatColor color;
    };

    struct LifeState {
        float bandCenter = 0.5f;
        float bandEnergy = 0.0f;
        float threshold = 0.48f;
        float affinity = 0.65f;
        float biosphereEnergy = 0.0f;
        float civilizationEnergy = 0.0f;
        float stability = 0.0f;
        float changeEnergy = 0.0f;
        float satellitePhase = 0.0f;
        float satelliteSpeed = 0.22f;
        float satelliteDistance = 3.3f;
        float satelliteInclination = 0.0f;
        float satelliteSize = 0.05f;
        float satelliteSeed = 0.0f;
        ofFloatColor biosphereColor;
    };

    glm::vec3 rotateX(const glm::vec3& value, float radians) const;
    glm::vec3 rotateY(const glm::vec3& value, float radians) const;
    glm::vec3 rotateZ(const glm::vec3& value, float radians) const;
    glm::vec3 orbitPointFor(const Body& body, float angle, float radiusScale, float waveformPhase, float extraRadius = 0.0f) const;
    glm::vec3 orbitPointFor(const Asteroid& asteroid, float angle, float radiusScale) const;
    glm::vec3 orbitPointFor(const Comet& comet, float angle, float radiusScale) const;
    glm::vec3 moonPointFor(const Body& body, const Moon& moon, const glm::vec3& bodyPosition, float angle, float planetRadius) const;
    float planetBandEnergyFor(std::size_t bodyIndex) const;
    void drawBackground(float width, float height, float alpha, float timeSeconds) const;
    void drawLowPolySphere(float radius,
                           const ofFloatColor& color,
                           float alpha,
                           float seed,
                           int rings,
                           int segments,
                           bool wireframe) const;
    void drawLowPolySphereLit(float radius,
                              const ofFloatColor& color,
                              float alpha,
                              float seed,
                              int rings,
                              int segments,
                              bool wireframe,
                              const glm::vec3& lightDirLocal) const;
    void drawLivingStarSurface(float starRadius, float alpha, float timeSeconds) const;
    void drawStarEmissionOverlay(float width, float height, float starRadius, float alpha, float timeSeconds) const;
    void drawStarGlow(float starRadius, float alpha, float timeSeconds) const;
    void drawStarRadiance(float starRadius, float alpha, float timeSeconds) const;
    void drawPlanetBands(const Body& body, float planetRadius, float alpha) const;
    void drawOrbitLine(const Body& body, float radiusScale, float alpha) const;
    void drawBodyTrail(std::size_t bodyIndex, const Body& body, float radiusScale, float alpha) const;
    void drawMoonOrbit(const glm::vec3& bodyPosition, const Moon& moon, float planetRadius, float alpha) const;
    void drawRingSet(const Body& body, float planetRadius, float alpha) const;
    void drawVisitors(float radiusScale, float alpha, float timeSeconds) const;
    void drawArtificialArtifacts(std::size_t bodyIndex,
                                 const Body& body,
                                 const glm::vec3& bodyPosition,
                                 float planetRadius,
                                 float alpha,
                                 float timeSeconds) const;
    void drawLifeSatellite(std::size_t bodyIndex,
                           const Body& body,
                           const glm::vec3& bodyPosition,
                           float planetRadius,
                           float alpha,
                           float timeSeconds) const;
    void drawPlanetCallouts(const LayerDrawParams& params,
                            float width,
                            float height,
                            float radiusScale,
                            float alpha) const;
    void drawHeliosphereField(float radiusScale, float alpha, float timeSeconds) const;
    void drawAsteroids(float radiusScale, float alpha, float timeSeconds) const;
    void drawComets(float radiusScale, float alpha, float timeSeconds) const;

    bool paramEnabled_ = true;
    bool paramShowOrbits_ = true;
    bool paramShowTrails_ = true;
    bool paramShowMoons_ = true;
    bool paramShowRings_ = true;
    bool paramShowAsteroids_ = true;
    bool paramShowComets_ = true;
    bool paramShowWaveformBelt_ = true;
    bool paramShowCallouts_ = true;
    bool paramCalloutCompact_ = false;
    float paramAlpha_ = 1.0f;
    float paramCalloutAlpha_ = 0.88f;
    float paramCalloutBackgroundAlpha_ = 0.82f;
    float paramCalloutScale_ = 0.92f;
    float paramCalloutFocusMode_ = 2.0f;
    float paramCalloutMaxVisible_ = 3.0f;
    float paramCalloutCycleSeconds_ = 4.5f;
    float paramScale_ = 0.92f;
    float paramSceneZoom_ = 1.0f;
    float paramOrbitSpread_ = 1.32f;
    float paramOrbitSpeed_ = 0.012f;
    float paramOrbitTilt_ = 38.0f;
    float paramOrbitRotation_ = -11.0f;
    float paramOrbitPlaneVariation_ = 0.90f;
    float paramEccentricity_ = 0.12f;
    float paramDepth_ = 1.0f;
    float paramStarSize_ = 0.070f;
    float paramStarGlow_ = 3.0f;
    float paramStarRadiance_ = 2.8f;
    float paramStarEmissionAudio_ = 1.35f;
    float paramStarSurfaceTurbulence_ = 0.82f;
    float paramSolarBurstIntensity_ = 0.90f;
    float paramSideFillLight_ = 0.34f;
    float paramVisitorEvents_ = 0.55f;
    float paramArtifactActivity_ = 0.65f;
    float paramPlanetSize_ = 0.62f;
    float paramObservedDiversity_ = 0.0f;
    float paramPlanetVariation_ = 0.28f;
    float paramAsteroidDensity_ = 0.65f;
    float paramCometDensity_ = 0.25f;
    float paramOrbitAlpha_ = 0.12f;
    float paramOrbitThickness_ = 0.85f;
    float paramTrailAlpha_ = 0.36f;
    float paramTrailLength_ = 0.30f;
    float paramTrailSteps_ = 56.0f;
    float paramTrailStampGain_ = 0.85f;
    float paramTrailStampLife_ = 5.5f;
    float paramAtmosphereGrowth_ = 1.25f;
    float paramLifeReactivity_ = 1.45f;
    float paramBiosphereThreshold_ = 0.34f;
    float paramCivilizationGrowth_ = 0.85f;
    float paramMoonSize_ = 0.45f;
    float paramMoonSpeed_ = 0.35f;
    float paramAudioAmount_ = 0.65f;
    float paramAudioSmoothing_ = 0.32f;
    float paramBassScale_ = 0.75f;
    float paramMidsSpeed_ = 0.36f;
    float paramHighsSparkle_ = 0.70f;
    float paramWaveformAmount_ = 0.045f;
    float paramBgAlpha_ = 0.28f;
    float paramBgR_ = 0.004f;
    float paramBgG_ = 0.006f;
    float paramBgB_ = 0.018f;
    float paramStarR_ = 1.0f;
    float paramStarG_ = 0.72f;
    float paramStarB_ = 0.24f;
    float paramOrbitR_ = 1.0f;
    float paramOrbitG_ = 1.0f;
    float paramOrbitB_ = 1.0f;
    float paramTrailR_ = 1.0f;
    float paramTrailG_ = 1.0f;
    float paramTrailB_ = 1.0f;
    bool paramReseedRequested_ = false;
    float paramSeed_ = 0.0f;

    bool enabled_ = true;
    bool systemReady_ = false;
    bool hasAudio_ = false;
    bool hasWaveform_ = false;
    uint64_t lastAudioFrame_ = 0;
    float orbitTime_ = 0.0f;
    float pulseEnvelope_ = 0.0f;
    float lastPulseTime_ = -1000.0f;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    float seedParamState_ = -1.0f;
    float diversityState_ = -1.0f;
    float planetVariationState_ = -1.0f;
    float orbitPlaneVariationState_ = -1.0f;
    float asteroidDensityState_ = -1.0f;
    float cometDensityState_ = -1.0f;
    std::uint32_t seedState_ = 0;
    std::string sourceSystem_;
    float sourceStarTemp_ = 5778.0f;
    float sourceStarRadiusSolar_ = 1.0f;
    ofFloatColor sourceStarColor_;
    std::vector<Body> bodies_;
    std::vector<Asteroid> asteroids_;
    std::vector<Asteroid> fieldAsteroids_;
    std::vector<Comet> comets_;
    std::vector<Visitor> visitors_;
    std::vector<Artifact> artifacts_;
    std::vector<LifeState> lifeStates_;
    std::vector<float> atmosphereEnergy_;
    std::vector<BackgroundStar> backgroundStars_;
    std::vector<float> waveform_;
};

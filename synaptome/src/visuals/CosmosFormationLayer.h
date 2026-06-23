#pragma once

#include "Layer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class CosmosFormationLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct MatterNode {
        glm::vec2 pos{ 0.0f, 0.0f };
        glm::vec2 prev{ 0.0f, 0.0f };
        glm::vec2 vel{ 0.0f, 0.0f };
        std::size_t cluster = 0;
        float angle = 0.0f;
        float orbitRadius = 0.0f;
        float speed = 1.0f;
        float size = 1.0f;
        float brightness = 1.0f;
        float heat = 1.0f;
        float seed = 0.0f;
        float spin = 1.0f;
        float birthDelay = 0.0f;
        float mass = 1.0f;
    };

    struct Cluster {
        glm::vec2 basePos{ 0.0f, 0.0f };
        float radius = 0.12f;
        float strength = 1.0f;
        float spin = 1.0f;
        float seed = 0.0f;
        float heat = 0.2f;
    };

    struct PressureWave {
        glm::vec2 origin{ 0.0f, 0.0f };
        float age = 0.0f;
        float strength = 1.0f;
    };

    struct GrowthCell {
        float darkDensity = 0.0f;
        float gasDensity = 0.0f;
        float stellarDensity = 0.0f;
        float temperature = 0.0f;
        float spin = 0.0f;
        float bloom = 0.0f;
        float compression = 0.0f;
        float luminosity = 0.0f;
        float audioEnergy = 0.0f;
        float ridge = 0.0f;
        float node = 0.0f;
        float voidness = 0.0f;
        glm::vec2 filamentDir{ 0.0f, 0.0f };
    };

    struct GrowthSample {
        float darkDensity = 0.0f;
        float gasDensity = 0.0f;
        float stellarDensity = 0.0f;
        float temperature = 0.0f;
        float spin = 0.0f;
        float bloom = 0.0f;
        float compression = 0.0f;
        float luminosity = 0.0f;
        float audioEnergy = 0.0f;
        float ridge = 0.0f;
        float node = 0.0f;
        float voidness = 0.0f;
        glm::vec2 gradient{ 0.0f, 0.0f };
        glm::vec2 filamentDir{ 0.0f, 0.0f };
        glm::vec2 flow{ 0.0f, 0.0f };
    };

    struct WebNode {
        int id = 0;
        glm::vec2 pos{ 0.0f, 0.0f };
        glm::vec2 vel{ 0.0f, 0.0f };
        float mass = 0.0f;
        float radius = 0.10f;
        float heat = 0.0f;
        float age = 0.0f;
        float activation = 0.0f;
        float audioCharge = 0.0f;
        float effectiveMass = 0.0f;
    };

    struct FilamentEdge {
        int a = -1;
        int b = -1;
        float strength = 1.0f;
        float age = 0.0f;
        float activation = 0.0f;
        float audioCharge = 0.0f;
        float conductivity = 0.0f;
        float massFlow = 0.0f;
        glm::vec2 bend{ 0.0f, 0.0f };
    };

    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const std::string& label,
                       float min,
                       float max,
                       float step,
                       const std::string& description = std::string());
    void readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b);
    void clampParams();
    void resetMatter();
    void triggerBang(float strength, bool collapseMatter);
    void triggerIgnition(float strength, const glm::vec2& origin);
    void triggerPressureWave(float strength, const glm::vec2& origin);
    void updateAudioState();
    void updateBeatState(float timeSeconds, float bpm);
    void resetGrowthField();
    void seedWebNodesFromClusters();
    void buildFilamentEdges();
    void updateCosmicWeb(float dt, float timeSeconds);
    void paintCosmicWebIntoField(float dt, float timeSeconds);
    void updateGrowthField(float dt, float timeSeconds, float transportSpeed);
    void updateMatter(float dt, float timeSeconds, float transportSpeed);
    void updatePressureWaves(float dt);
    void drawBackground(float width, float height, float alpha) const;
    void drawGrowthField(float width, float height, float minDim, float alpha, float timeSeconds) const;
    void drawPressureWaves(float width, float height, float minDim, float alpha) const;
    void drawDensityHalos(float width, float height, float minDim, float alpha, float timeSeconds) const;
    void drawCore(float width, float height, float minDim, float alpha) const;
    void drawTrails(float width, float height, float minDim, float alpha) const;
    void drawMatter(float width, float height, float minDim, float alpha, float timeSeconds) const;
    glm::vec2 growthFieldPoint(int x, int y) const;
    glm::vec2 screenScale(float width, float height) const;
    glm::vec2 screenPoint(const glm::vec2& point, float width, float height) const;
    GrowthSample sampleGrowthField(const glm::vec2& point) const;
    float distanceToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float& t) const;
    float waveformSampleFor(std::size_t index) const;
    float pressureForPoint(const glm::vec2& point) const;
    float formationAmount(const MatterNode& node) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramParticleCount_ = 2400.0f;
    float paramClusterCount_ = 64.0f;
    float paramRadius_ = 1.28f;
    float paramOriginRadius_ = 0.018f;
    float paramExpansionRate_ = 0.42f;
    float paramExpansionForce_ = 0.45f;
    bool paramAutoAdvance_ = false;
    float paramFormationAge_ = 1.0f;
    float paramFormationTime_ = 90.0f;
    float paramEvolutionSpeed_ = 0.16f;
    float paramGravity_ = 0.10f;
    float paramGravityDelay_ = 0.90f;
    float paramGravityEmergence_ = 0.18f;
    float paramClusterSwirl_ = 0.10f;
    float paramClusterSpread_ = 1.20f;
    float paramClusterDrift_ = 0.04f;
    float paramClusterSoftness_ = 1.08f;
    float paramShear_ = 0.12f;
    float paramVoidPressure_ = 0.42f;
    float paramTurbulence_ = 0.46f;
    float paramCoolingRate_ = 0.34f;
    float paramFieldLuminosity_ = 2.30f;
    float paramGlowPersistence_ = 0.945f;
    float paramFilamentMemory_ = 0.955f;
    float paramMatterSize_ = 0.55f;
    float paramMatterGlow_ = 1.65f;
    float paramTrailAlpha_ = 0.018f;
    float paramTrailThickness_ = 0.42f;
    float paramHaloAlpha_ = 0.62f;
    float paramHaloRadius_ = 0.145f;
    float paramShockwaveCount_ = 0.0f;
    float paramShockwaveAlpha_ = 0.0f;
    float paramShockwaveWidth_ = 0.075f;
    float paramShockwaveSpeed_ = 0.74f;
    float paramPressureAmount_ = 0.0f;
    float paramAudioAmount_ = 1.0f;
    float paramAudioAccretion_ = 0.78f;
    float paramAudioGlow_ = 1.15f;
    float paramAudioTwinkle_ = 1.10f;
    float paramBassExpansion_ = 0.22f;
    float paramMidsTurbulence_ = 0.36f;
    float paramHighsSparkle_ = 0.70f;
    float paramWaveformWarp_ = 0.030f;
    float paramPeakBangThreshold_ = 0.74f;
    float paramPeakImpulse_ = 0.14f;
    float paramBeatImpulse_ = 0.025f;
    float paramAudioSmoothing_ = 0.35f;
    bool paramBangRequested_ = false;
    bool paramReseedRequested_ = false;
    float paramSeed_ = 2026.0f;
    float paramBgAlpha_ = 0.72f;
    float paramBgR_ = 0.004f;
    float paramBgG_ = 0.006f;
    float paramBgB_ = 0.018f;
    float paramHotR_ = 1.0f;
    float paramHotG_ = 0.70f;
    float paramHotB_ = 0.30f;
    float paramMatterR_ = 0.48f;
    float paramMatterG_ = 0.82f;
    float paramMatterB_ = 1.0f;
    float paramCoolR_ = 0.13f;
    float paramCoolG_ = 0.38f;
    float paramCoolB_ = 0.95f;
    float paramClusterR_ = 0.95f;
    float paramClusterG_ = 0.36f;
    float paramClusterB_ = 1.0f;
    float paramWaveR_ = 0.72f;
    float paramWaveG_ = 1.0f;
    float paramWaveB_ = 0.86f;

    bool enabled_ = true;
    bool hasAudio_ = false;
    bool hasWaveform_ = false;
    std::uint64_t lastAudioFrame_ = 0;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    std::vector<float> waveform_;
    std::vector<MatterNode> matter_;
    std::vector<Cluster> clusters_;
    std::vector<WebNode> webNodes_;
    std::vector<FilamentEdge> filamentEdges_;
    std::vector<PressureWave> waves_;
    std::vector<GrowthCell> growthField_;
    std::vector<GrowthCell> growthScratch_;
    float age_ = 0.0f;
    float bangEnergy_ = 1.0f;
    float beatPulse_ = 0.0f;
    float scaleFactor_ = 0.18f;
    float cosmicTemperature_ = 1.0f;
    float simTime_ = 0.0f;
    float lastPeakTime_ = -1000.0f;
    int lastBeatIndex_ = -1;
    std::uint32_t seedState_ = 2026;
    int clusterState_ = 5;
    int nextWebNodeId_ = 1;
    float webRebuildAccumulator_ = 0.0f;
};

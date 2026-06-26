#pragma once

#include "Layer.h"

#include <cstdint>
#include <random>
#include <vector>

class RiverFormationLayer : public Layer {
public:
    struct RiverCell {
        float elevation = 0.0f;
        float sediment = 0.0f;
        float wetness = 0.0f;
        float activeWater = 0.0f;
        float abandonedWater = 0.0f;
        float bankResistance = 0.0f;
    };

    struct RiverNode {
        glm::vec2 fieldPosition{ 0.0f, 0.0f };
        glm::vec2 flowDirection{ 1.0f, 0.0f };
        float curvature = 0.0f;
        float width = 1.0f;
    };

    struct RiverPath {
        std::vector<RiverNode> nodes;
        float widthScale = 1.0f;
        float age = 0.0f;
        int attachStartIndex = -1;
        int attachEndIndex = -1;
        bool mainStem = false;
    };

    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    const std::vector<RiverNode>& riverCenterline() const { return centerline_; }
    const std::vector<RiverPath>& riverPaths() const { return paths_; }
    const std::vector<RiverCell>& riverCells() const { return cells_; }
    glm::ivec2 riverFieldSize() const { return fieldSize_; }
    float normalizedElevationAt(const glm::vec2& uv) const;
    float wetnessAt(const glm::vec2& uv) const;

private:
    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const char* label,
                       float minValue,
                       float maxValue,
                       float step,
                       const char* description = "");
    void readColorArray(const ofJson& def, const char* key, float& r, float& g, float& b);
    void clampParams();
    void allocateFields();
    void resetSimulation();
    void initializeFloodplain();
    void buildInitialMainPath(std::mt19937& rng);
    RiverPath buildBranchFrom(const RiverPath& parent, int parentIndex, std::mt19937& rng) const;
    RiverPath buildBranchBetween(const RiverPath& parent, int startIndex, int endIndex, std::mt19937& rng) const;
    bool findNeckCandidate(const RiverPath& path, int& startIndex, int& endIndex) const;
    void simulateStep();
    void fadeFields();
    void maybeSpawnBranch();
    void migratePath(RiverPath& path);
    void smoothPath(RiverPath& path);
    void constrainPathProgress(RiverPath& path);
    void resamplePath(RiverPath& path, int targetCount);
    void updatePathGeometry(RiverPath& path);
    void maybeApplyCutoff(RiverPath& path);
    void paintRiverEffects();
    void paintPath(const RiverPath& path, bool active);
    void drawPathPolyline(const RiverPath& path,
                          const LayerDrawParams& params,
                          float alpha,
                          float lineWidth) const;
    void paintDisk(const glm::vec2& center,
                   float radius,
                   float elevationDelta,
                   float sedimentDelta,
                   float wetnessAmount,
                   float activeWaterAmount,
                   float abandonedWaterAmount,
                   float bankResistanceScale);
    void refreshTexture();
    void triggerReset();
    void syncMainCenterline();

    float stepRateFor(const LayerUpdateParams& params) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;
    float renderSignature() const;
    int indexFor(int x, int y) const;
    RiverCell& cellAt(int x, int y);
    const RiverCell& cellAt(int x, int y) const;
    RiverCell sampleCell(const glm::vec2& fieldPos) const;
    float noiseSample(float x, float y, float z) const;
    float signedNoise(float x, float y, float z) const;

    bool paramEnabled_ = true;
    float paramAlpha_ = 1.0f;
    float paramSpeed_ = 96.0f;
    bool paramBpmSync_ = false;
    float paramBpmMultiplier_ = 2.0f;
    bool paramReseedRequested_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 128.0f;
    float paramSeed_ = 20260626.0f;
    float paramWarmupSteps_ = 0.0f;

    float paramPathPoints_ = 220.0f;
    float paramRiverWidth_ = 2.8f;
    float paramWidthVariation_ = 0.18f;
    float paramWidthPulse_ = 0.0f;
    float paramMigrationRate_ = 1.20f;
    float paramErosionStrength_ = 1.10f;
    float paramDepositionStrength_ = 0.65f;
    float paramChannelDepth_ = 0.0f;
    float paramBankHardness_ = 0.14f;
    float paramTrailDecay_ = 0.0025f;
    float paramOxbowDecay_ = 0.0010f;
    float paramCutoffFactor_ = 3.4f;
    float paramBranchChance_ = 0.0f;
    float paramMaxBranches_ = 0.0f;
    float paramBranchLength_ = 0.58f;
    float paramBranchAngle_ = 0.34f;
    float paramBranchWidth_ = 0.36f;
    float paramNoiseAmount_ = 0.20f;
    float paramValleyConfinement_ = 0.02f;
    float paramMeanderSmoothing_ = 0.035f;
    float paramCurvatureMemory_ = 10.0f;
    float paramStabilityClamp_ = 0.36f;

    float paramTrailBoost_ = 4.5f;
    float paramTrailAlpha_ = 1.0f;
    float paramOxbowAlpha_ = 0.40f;
    float paramGlowAmount_ = 0.0f;
    float paramMaskThreshold_ = 0.015f;
    float paramColorR_ = 1.0f;
    float paramColorG_ = 0.48f;
    float paramColorB_ = 0.0f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 fieldSize_{ 512, 288 };
    std::vector<RiverCell> cells_;
    std::vector<RiverNode> centerline_;
    std::vector<RiverPath> paths_;
    std::vector<RiverPath> abandonedPaths_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    float stepAccumulator_ = 0.0f;
    float nextAutoReseedBeat_ = -1.0f;
    float simAge_ = 0.0f;
    float cutoffPulse_ = 0.0f;
    int cutoffCooldown_ = 0;
    std::uint32_t seedState_ = 0;
    int pathPointState_ = 0;
    float fieldSignatureState_ = -1.0f;
};

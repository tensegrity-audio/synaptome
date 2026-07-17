#pragma once

#include "Layer.h"

#include <cstdint>
#include <random>
#include <vector>

class ReactionDiffusionLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Cell {
        float a = 1.0f;
        float b = 0.0f;
    };

    void allocateField();
    void resetField();
    void seedPatch(int centerX, int centerY, int radius, float amount);
    void stepSimulation();
    void injectChemical();
    void syncTexture();
    void clampParams();
    float laplacianA(int x, int y) const;
    float laplacianB(int x, int y) const;
    float stepRateFor(const LayerUpdateParams& params) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;
    std::uint32_t activeSeed() const;
    float randomUnit();
    float randomRange(float minValue, float maxValue);
    int randomInt(int minValue, int maxValue);
    int indexFor(int x, int y) const;
    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const char* label,
                       float minValue,
                       float maxValue,
                       float step,
                       const char* description = "");

    bool paramEnabled_ = true;
    float paramSpeed_ = 24.0f;
    bool paramBpmSync_ = true;
    float paramBpmMultiplier_ = 6.0f;
    float paramAlpha_ = 1.0f;
    bool paramPaused_ = false;
    bool paramReseedRequested_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 32.0f;
    float paramFeedRate_ = 0.037f;
    float paramKillRate_ = 0.061f;
    float paramDiffusionA_ = 1.0f;
    float paramDiffusionB_ = 0.50f;
    float paramInjectionRate_ = 0.025f;
    float paramInjectionAmount_ = 0.72f;
    float paramInjectionRadius_ = 7.0f;
    float paramSeed_ = 1337.0f;
    float paramSeedDensity_ = 0.12f;
    float paramContourThreshold_ = 0.24f;
    float paramContourWidth_ = 0.12f;
    float paramFieldScale_ = 1.65f;
    float paramBackgroundAlpha_ = 0.0f;
    float paramFieldAlpha_ = 1.0f;
    float paramContourOpacity_ = 0.76f;
    float paramBgR_ = 0.01f;
    float paramBgG_ = 0.012f;
    float paramBgB_ = 0.018f;
    float paramFieldR_ = 0.10f;
    float paramFieldG_ = 0.82f;
    float paramFieldB_ = 0.62f;
    float paramContourR_ = 0.95f;
    float paramContourG_ = 0.96f;
    float paramContourB_ = 0.86f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 textureSize_{ 192, 108 };
    std::vector<Cell> field_;
    std::vector<Cell> next_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    std::mt19937 rng_{ 1337u };
    float stepAccumulator_ = 0.0f;
    float nextAutoReseedBeat_ = -1.0f;
};

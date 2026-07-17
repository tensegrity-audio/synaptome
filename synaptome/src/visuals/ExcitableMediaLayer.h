#pragma once

#include "Layer.h"

#include <cstdint>
#include <random>
#include <vector>

class ExcitableMediaLayer : public Layer {
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
        float excitation = 0.0f;
        float refractory = 0.0f;
    };

    void allocateField();
    void resetField();
    void seedPatch(int centerX, int centerY, int radius, float amount);
    void stepSimulation();
    void injectSpark();
    void syncTexture();
    void clampParams();
    Cell sampleCell(int x, int y) const;
    float neighborExcitation(int x, int y) const;
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
    float paramSpeed_ = 30.0f;
    bool paramBpmSync_ = true;
    float paramBpmMultiplier_ = 8.0f;
    float paramAlpha_ = 1.0f;
    bool paramPaused_ = false;
    bool paramReseedRequested_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 32.0f;
    float paramSeed_ = 3773.0f;
    float paramSeedDensity_ = 0.06f;
    float paramPropagationRate_ = 1.35f;
    float paramExcitationThreshold_ = 0.38f;
    float paramRefractoryTime_ = 16.0f;
    float paramSeedRate_ = 0.028f;
    float paramWavefrontWidth_ = 0.12f;
    float paramSparkleAmount_ = 0.38f;
    float paramFieldDiffusion_ = 0.08f;
    float paramDecayRate_ = 0.18f;
    float paramInjectionRadius_ = 4.0f;
    float paramFieldScale_ = 1.4f;
    float paramBackgroundAlpha_ = 0.0f;
    float paramExcitationAlpha_ = 1.0f;
    float paramRefractoryAlpha_ = 0.42f;
    float paramWavefrontOpacity_ = 0.86f;
    float paramBgR_ = 0.006f;
    float paramBgG_ = 0.012f;
    float paramBgB_ = 0.026f;
    float paramExciteR_ = 0.18f;
    float paramExciteG_ = 0.78f;
    float paramExciteB_ = 1.0f;
    float paramRefractoryR_ = 0.22f;
    float paramRefractoryG_ = 0.16f;
    float paramRefractoryB_ = 0.46f;
    float paramWaveR_ = 1.0f;
    float paramWaveG_ = 0.94f;
    float paramWaveB_ = 0.62f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 textureSize_{ 192, 108 };
    std::vector<Cell> field_;
    std::vector<Cell> next_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    std::mt19937 rng_{ 3773u };
    float stepAccumulator_ = 0.0f;
    float nextAutoReseedBeat_ = -1.0f;
};

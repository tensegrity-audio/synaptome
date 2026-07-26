#pragma once

#include "Layer.h"

#include <cstdint>
#include <random>
#include <vector>

class LeniaLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    std::uint64_t debugStateSignature() const;
    bool debugUsesCircuitPresentation() const {
        return presentation_ == Presentation::Circuit;
    }

private:
    enum class Presentation {
        Organic,
        Circuit
    };

    void allocateField();
    void resetField();
    void seedPatch(int centerX, int centerY, int radius, float amount);
    void stepSimulation();
    void injectPatch();
    void syncTexture();
    int circuitBandAt(int x, int y) const;
    bool circuitContourAt(int x, int y) const;
    void clampParams();
    float sampleField(int x, int y) const;
    float kernelWeight(float normalizedDistance) const;
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
                       const char* description = "",
                       const char* group = "Generative",
                       bool quickAccess = false,
                       int quickAccessOrder = 0);

    Presentation presentation_ = Presentation::Organic;
    bool paramEnabled_ = true;
    float paramSpeed_ = 18.0f;
    bool paramBpmSync_ = true;
    float paramBpmMultiplier_ = 4.0f;
    float paramAlpha_ = 1.0f;
    bool paramPaused_ = false;
    bool paramReseedRequested_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 48.0f;
    float paramSeed_ = 2112.0f;
    float paramSeedDensity_ = 0.16f;
    float paramKernelRadius_ = 8.0f;
    float paramGrowthCenter_ = 0.34f;
    float paramGrowthWidth_ = 0.065f;
    float paramGrowthAmplitude_ = 0.105f;
    float paramMutationAmount_ = 0.002f;
    float paramInjectionRate_ = 0.018f;
    float paramInjectionAmount_ = 0.78f;
    float paramInjectionRadius_ = 8.0f;
    float paramDecayRate_ = 0.006f;
    float paramFieldScale_ = 1.8f;
    float paramEdgeGlow_ = 1.15f;
    float paramBackgroundAlpha_ = 0.0f;
    float paramFieldAlpha_ = 1.0f;
    float paramEdgeOpacity_ = 0.72f;
    float paramBgR_ = 0.012f;
    float paramBgG_ = 0.010f;
    float paramBgB_ = 0.018f;
    float paramFieldR_ = 0.88f;
    float paramFieldG_ = 0.64f;
    float paramFieldB_ = 0.32f;
    float paramEdgeR_ = 0.35f;
    float paramEdgeG_ = 0.95f;
    float paramEdgeB_ = 0.72f;
    float paramCircuitThreshold_ = 0.18f;
    float paramCircuitLevels_ = 4.0f;
    float paramCircuitTraceWidth_ = 1.0f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 textureSize_{ 160, 90 };
    std::vector<float> field_;
    std::vector<float> next_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    std::mt19937 rng_{ 2112u };
    float stepAccumulator_ = 0.0f;
    float nextAutoReseedBeat_ = -1.0f;
};

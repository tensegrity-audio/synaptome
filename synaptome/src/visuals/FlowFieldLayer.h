#pragma once

#include "Layer.h"
#include <vector>

class FlowFieldLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Particle {
        glm::vec2 pos{ 0.0f, 0.0f };
        glm::vec2 prev{ 0.0f, 0.0f };
        glm::vec2 vel{ 0.0f, 0.0f };
        float age = 0.0f;
        float life = 240.0f;
    };

    void allocateTrail();
    void resetParticles();
    void spawnParticle(Particle& particle);
    void fadeTrail();
    void stepParticles(float timeSeconds);
    void depositTrail(const glm::vec2& from, const glm::vec2& to, float amount, float colorMix);
    void depositPoint(const glm::vec2& pos, float amount, float colorMix);
    void syncTexture();
    void drawVectorOverlay(const LayerDrawParams& params) const;
    void clampParams();
    float stepRateFor(const LayerUpdateParams& params) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;
    glm::vec2 flowAt(const glm::vec2& pos, float timeSeconds) const;
    float noiseSample(float x, float y, float z) const;
    int indexFor(int x, int y) const;
    bool outOfBounds(const glm::vec2& pos) const;
    glm::vec2 wrapPosition(glm::vec2 pos) const;
    void triggerReset();

    bool paramEnabled_ = true;
    float paramSpeed_ = 18.0f;
    bool paramBpmSync_ = true;
    float paramBpmMultiplier_ = 6.0f;
    float paramAlpha_ = 1.0f;
    bool paramReseedRequested_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 32.0f;
    float paramParticleCount_ = 900.0f;
    float paramParticleLife_ = 420.0f;
    float paramRespawnRate_ = 0.01f;
    float paramSpawnRadius_ = 1.0f;
    bool paramEdgeWrap_ = true;
    bool paramMirrorX_ = false;
    bool paramMirrorY_ = false;
    float paramFieldScale_ = 2.8f;
    float paramFieldStrength_ = 1.0f;
    float paramFlowSpeed_ = 0.18f;
    float paramCurlAmount_ = 0.65f;
    float paramTurbulence_ = 0.35f;
    float paramStepSize_ = 1.0f;
    float paramInertia_ = 0.55f;
    float paramCenterPull_ = 0.0f;
    float paramDriftX_ = 0.0f;
    float paramDriftY_ = 0.0f;
    float paramTrailFade_ = 0.035f;
    float paramTrailDeposit_ = 0.16f;
    float paramTrailBoost_ = 1.3f;
    float paramBackgroundAlpha_ = 0.0f;
    float paramTrailAlpha_ = 0.9f;
    float paramPointSize_ = 1.0f;
    bool paramVectorOverlay_ = false;
    float paramVectorSpacing_ = 18.0f;
    float paramVectorScale_ = 0.45f;
    float paramVectorAlpha_ = 0.28f;
    float paramColorBias_ = 0.5f;
    float paramPaletteRate_ = 0.0f;
    float paramBgR_ = 0.01f;
    float paramBgG_ = 0.015f;
    float paramBgB_ = 0.03f;
    float paramColorAR_ = 0.1f;
    float paramColorAG_ = 0.75f;
    float paramColorAB_ = 1.0f;
    float paramColorBR_ = 1.0f;
    float paramColorBG_ = 0.35f;
    float paramColorBB_ = 0.85f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 textureSize_{ 256, 144 };
    std::vector<float> trailA_;
    std::vector<float> trailB_;
    std::vector<Particle> particles_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    float stepAccumulator_ = 0.0f;
    float palettePhase_ = 0.0f;
    float nextAutoReseedBeat_ = -1.0f;
};

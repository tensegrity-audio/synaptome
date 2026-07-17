#pragma once

#include "Layer.h"
#include <vector>

class FlockingLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Boid {
        glm::vec2 pos{ 0.0f, 0.0f };
        glm::vec2 vel{ 0.0f, 0.0f };
        float fear = 0.0f;
    };

    enum FlockModel {
        Schooling = 0,
        Murmuration = 1
    };

    enum BehaviorMode {
        Converge = 0,
        Diverge = 1,
        Stressed = 2
    };

    void allocateTrail();
    void resetSimulation();
    void fadeTrail();
    void stepSchooling(float dtScale, float time);
    void stepMurmuration(float dtScale, float time);
    void stepPredators(float dtScale);
    void depositTrail(const glm::vec2& pos, float amount);
    void depositFear(const glm::vec2& pos, float amount);
    void propagateFear(float neighborRadius, float threatRadius);
    glm::vec2 fearSteeringFor(const Boid& boid, float threatRadius, float time) const;
    void syncTexture();
    float stepRateFor(const LayerUpdateParams& params) const;
    glm::vec2 wrapPosition(glm::vec2 pos) const;
    void clampSpeed(glm::vec2& vel, float target, float minScale, float maxScale) const;
    void stampMarker(ofFloatPixels& pixels, const glm::vec2& pos, const ofFloatColor& color, int radius) const;
    void stampPredator(ofFloatPixels& pixels, const Boid& predator, const ofFloatColor& color, int radius) const;
    int behaviorMode() const;
    bool predatorsActive() const;

    FlockModel model_ = Murmuration;
    bool paramEnabled_ = true;
    float paramSpeed_ = 12.0f;
    bool paramBpmSync_ = false;
    float paramBpmMultiplier_ = 2.0f;
    float paramAlpha_ = 1.0f;
    bool paramReseedRequested_ = false;
    float paramMode_ = 0.0f;
    float paramBoidCount_ = 42.0f;
    bool paramPredatorEnabled_ = false;
    float paramPredatorCount_ = 6.0f;
    float paramPredatorPressure_ = 1.0f;
    bool paramFearWaveEnabled_ = false;
    float paramFearPropagation_ = 0.38f;
    float paramFearDecay_ = 0.08f;
    float paramFearRadius_ = 24.0f;
    float paramFearForce_ = 1.0f;
    float paramFearTrailAlpha_ = 0.42f;
    float paramNeighborCount_ = 7.0f;
    float paramCohesion_ = 0.008f;
    float paramAlignment_ = 0.025f;
    float paramSeparation_ = 0.002f;
    float paramPredatorSeparation_ = 0.018f;
    float paramChase_ = 0.008f;
    float paramEvade_ = 0.012f;
    float paramNoise_ = 0.006f;
    float paramTrailFade_ = 0.04f;
    float paramTrailDeposit_ = 0.18f;
    float paramPointSize_ = 2.0f;
    float paramBackgroundAlpha_ = 0.0f;
    float paramTrailAlpha_ = 0.7f;
    float paramPreyAlpha_ = 1.0f;
    float paramPredAlpha_ = 1.0f;
    float paramBgR_ = 0.01f;
    float paramBgG_ = 0.03f;
    float paramBgB_ = 0.06f;
    float paramTrailR_ = 0.15f;
    float paramTrailG_ = 0.55f;
    float paramTrailB_ = 1.0f;
    float paramPreyR_ = 0.3f;
    float paramPreyG_ = 0.9f;
    float paramPreyB_ = 0.5f;
    float paramPredR_ = 1.0f;
    float paramPredG_ = 0.35f;
    float paramPredB_ = 0.25f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 textureSize_{ 192, 108 };
    std::vector<float> trail_;
    std::vector<float> fearTrail_;
    std::vector<float> fearScratch_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    std::vector<Boid> boids_;
    std::vector<Boid> predators_;
    float stepAccumulator_ = 0.0f;
};

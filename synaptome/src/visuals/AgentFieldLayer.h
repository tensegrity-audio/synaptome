#pragma once

#include "Layer.h"
#include <vector>

class AgentFieldLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

private:
    struct Agent {
        float x = 0.0f;
        float y = 0.0f;
        float angle = 0.0f;
        float energy = 0.0f;
    };

    enum Mode {
        AntTunnels = 0,
        SlimeMold = 1,
        Physarum = 2
    };

    void allocateField();
    void resetAgents();
    void diffuseAndDecay();
    void stepAgents(float amount);
    void stepAnt(Agent& agent, float jitterScale);
    void stepSlime(Agent& agent, float jitterScale);
    void stepPhysarum(Agent& agent, float jitterScale);
    void deposit(int x, int y, float amount);
    float sample(float x, float y) const;
    void syncTexture();
    float stepRateFor(const LayerUpdateParams& params) const;
    int indexFor(int x, int y) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;
    float fieldCoverage(float threshold) const;
    void triggerReset();

    bool paramEnabled_ = true;
    float paramSpeed_ = 10.0f;
    bool paramBpmSync_ = false;
    float paramBpmMultiplier_ = 2.0f;
    float paramAlpha_ = 1.0f;
    bool paramReseedRequested_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 16.0f;
    float paramMode_ = 1.0f;
    float paramAgentCount_ = 48.0f;
    float paramStepSize_ = 1.0f;
    float paramTurnRate_ = 0.35f;
    float paramSensorAngle_ = 0.65f;
    float paramSensorDistance_ = 2.5f;
    float paramDeposit_ = 0.15f;
    float paramDecay_ = 0.02f;
    float paramDiffuse_ = 0.18f;
    float paramTrailBoost_ = 1.0f;
    float paramBackgroundAlpha_ = 0.0f;
    float paramTrailAlpha_ = 1.0f;
    float paramResetCoverage_ = 0.62f;
    float paramBgR_ = 0.02f;
    float paramBgG_ = 0.04f;
    float paramBgB_ = 0.02f;
    float paramTrailR_ = 0.4f;
    float paramTrailG_ = 1.0f;
    float paramTrailB_ = 0.5f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 textureSize_{ 192, 108 };
    std::vector<float> field_;
    std::vector<float> scratch_;
    std::vector<Agent> agents_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    float stepAccumulator_ = 0.0f;
    float nextAutoReseedBeat_ = -1.0f;
};

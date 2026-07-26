#pragma once

#include "EightDirectionMotion.h"
#include "Layer.h"
#include <cstdint>
#include <random>
#include <string>
#include <vector>

// A shared agent/pheromone simulation for the circuit-trace family. Every
// position change is an integer multiple of one of EightDirectionMotion's
// compass steps; model profiles only change how an agent chooses its next
// heading.
class CircuitTraceLayer : public Layer {
public:
    enum class Model {
        CircuitSlime,
        CircuitMycelium,
        CircuitRiver,
        CircuitAntTunnels,
        CircuitFlowField
    };

    explicit CircuitTraceLayer(Model model = Model::CircuitSlime);

    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    static Model modelFromId(const std::string& id, Model fallback = Model::CircuitSlime);
    static const char* modelId(Model model);

    // Small read-only hooks keep the movement and deterministic-reseed
    // contracts testable without exposing mutable simulation storage.
    std::uint64_t debugStateSignature() const;
    std::vector<int> debugAgentHeadings() const;
    void reseedForTest();

private:
    struct Agent {
        glm::ivec2 position{ 0, 0 };
        int heading = 0;
        float energy = 1.0f;
    };

    enum Behavior {
        Balanced = 0,
        Explore = 1,
        Exploit = 2
    };

    void applyModelDefaults(Model model);
    void allocateField();
    void resetSimulation();
    void diffuseAndDecay();
    void stepAgents();
    void stepSlime(Agent& agent);
    void stepMycelium(Agent& agent);
    void stepRiver(Agent& agent);
    void stepAntTunnels(Agent& agent);
    void stepFlowField(Agent& agent);
    void advance(Agent& agent, int nextHeading, bool wrap);
    void depositSegment(glm::ivec2 from, glm::ivec2 to, float amount);
    void deposit(glm::ivec2 point, float amount, bool hardTrace = true);
    void markVia(glm::ivec2 point);
    float sense(const Agent& agent, int heading) const;
    float sample(glm::ivec2 point) const;
    int chooseWeightedHeading(const Agent& agent, float followWeight, float exploreWeight);
    void syncTexture();
    float stepRateFor(const LayerUpdateParams& params) const;
    float currentBeatPosition(float timeSeconds, float bpm) const;
    float randomUnit();
    int randomInt(int minInclusive, int maxInclusive);
    std::uint32_t requestedSeed() const;
    int behavior() const;
    int indexFor(glm::ivec2 point) const;

    Model model_ = Model::CircuitSlime;

    bool paramVisible_ = true;
    float paramSpeed_ = 10.0f;
    bool paramBpmSync_ = false;
    float paramBpmMultiplier_ = 2.0f;
    float paramAlpha_ = 1.0f;
    float paramSeed_ = 4242.0f;
    bool paramReseed_ = false;
    bool paramAutoReseed_ = false;
    float paramAutoReseedEveryBeats_ = 16.0f;
    float paramBehavior_ = 0.0f;
    float paramAgentCount_ = 64.0f;
    float paramStepSize_ = 1.0f;
    float paramSensorDistance_ = 4.0f;
    float paramTurnChance_ = 0.28f;
    float paramBranchChance_ = 0.06f;
    float paramDeposit_ = 0.24f;
    float paramDecay_ = 0.008f;
    float paramDiffuse_ = 0.12f;
    float paramTracePersistence_ = 0.995f;
    float paramTraceWidth_ = 1.0f;
    float paramGlow_ = 1.2f;
    float paramViaChance_ = 0.025f;
    float paramBackgroundAlpha_ = 0.0f;
    float paramTrailAlpha_ = 1.0f;
    float paramBgR_ = 0.008f;
    float paramBgG_ = 0.015f;
    float paramBgB_ = 0.012f;
    float paramTraceR_ = 0.15f;
    float paramTraceG_ = 1.0f;
    float paramTraceB_ = 0.45f;

    bool enabled_ = true;
    bool dirty_ = true;
    glm::ivec2 textureSize_{ 256, 144 };
    std::vector<float> chemical_;
    std::vector<float> scratch_;
    std::vector<float> trace_;
    std::vector<std::uint8_t> vias_;
    std::vector<Agent> agents_;
    ofFloatPixels pixels_;
    ofTexture texture_;
    std::mt19937 rng_;
    std::uint32_t appliedSeed_ = 0;
    int appliedAgentCount_ = 0;
    float stepAccumulator_ = 0.0f;
    float nextAutoReseedBeat_ = -1.0f;
};

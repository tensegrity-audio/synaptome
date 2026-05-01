#pragma once

#include "Layer.h"
#include <vector>

class GameOfLifeLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    void randomize(float density = -1.0f);
    void setPaused(bool paused);
    bool isPaused() const { return paramPaused_; }
    void applyPreset(int presetIndex);
    int presetCount() const;

    float* speedParamPtr() { return &paramSpeed_; }
    const float* speedParamPtr() const { return &paramSpeed_; }
    bool* wrapParamPtr() { return &paramWrap_; }
    const bool* wrapParamPtr() const { return &paramWrap_; }
    float* alphaParamPtr() { return &paramAlpha_; }
    const float* alphaParamPtr() const { return &paramAlpha_; }
    float* aliveAlphaParamPtr() { return &paramAliveAlpha_; }
    const float* aliveAlphaParamPtr() const { return &paramAliveAlpha_; }
    float* deadAlphaParamPtr() { return &paramDeadAlpha_; }
    const float* deadAlphaParamPtr() const { return &paramDeadAlpha_; }
    float* aliveRParamPtr() { return &paramAliveR_; }
    const float* aliveRParamPtr() const { return &paramAliveR_; }
    float* aliveGParamPtr() { return &paramAliveG_; }
    const float* aliveGParamPtr() const { return &paramAliveG_; }
    float* aliveBParamPtr() { return &paramAliveB_; }
    const float* aliveBParamPtr() const { return &paramAliveB_; }
    float* deadRParamPtr() { return &paramDeadR_; }
    const float* deadRParamPtr() const { return &paramDeadR_; }
    float* deadGParamPtr() { return &paramDeadG_; }
    const float* deadGParamPtr() const { return &paramDeadG_; }
    float* deadBParamPtr() { return &paramDeadB_; }
    const float* deadBParamPtr() const { return &paramDeadB_; }
    float* densityParamPtr() { return &paramSeedDensity_; }
    const float* densityParamPtr() const { return &paramSeedDensity_; }
    float* presetParamPtr() { return &paramPresetIndex_; }
    const float* presetParamPtr() const { return &paramPresetIndex_; }
    bool* pausedParamPtr() { return &paramPaused_; }
    const bool* pausedParamPtr() const { return &paramPaused_; }
    bool* reseedParamPtr() { return &paramReseedRequested_; }
    const bool* reseedParamPtr() const { return &paramReseedRequested_; }
    float* fadeFramesParamPtr() { return &paramFadeFrames_; }
    const float* fadeFramesParamPtr() const { return &paramFadeFrames_; }
    bool* bpmSyncParamPtr() { return &paramBpmSync_; }
    const bool* bpmSyncParamPtr() const { return &paramBpmSync_; }
    float* bpmMultiplierParamPtr() { return &paramBpmMultiplier_; }
    const float* bpmMultiplierParamPtr() const { return &paramBpmMultiplier_; }
    bool* autoReseedParamPtr() { return &paramAutoReseed_; }
    const bool* autoReseedParamPtr() const { return &paramAutoReseed_; }
    float* reseedQuantizeBeatsParamPtr() { return &paramReseedQuantizeBeats_; }
    const float* reseedQuantizeBeatsParamPtr() const { return &paramReseedQuantizeBeats_; }
    float* autoReseedEveryBeatsParamPtr() { return &paramAutoReseedEveryBeats_; }
    const float* autoReseedEveryBeatsParamPtr() const { return &paramAutoReseedEveryBeats_; }

private:
    void stepGeneration();
    int index(int x, int y) const;
    void syncTexture();
    void clearCells();
    void seedGlider(int x, int y);
    void seedBlinker(int x, int y);
    void seedCross(int x, int y, int radius);
    void applyPresetInternal(int presetIndex);
    float currentBeatPosition(float timeSeconds, float bpm) const;
    float stepRateFor(const LayerUpdateParams& params) const;
    void schedulePendingReseed(float beatPosition);
    void triggerReseed(float density = -1.0f);

    bool paramEnabled_ = true;
    float paramSpeed_ = 6.0f;
    bool paramWrap_ = true;
    float paramAliveR_ = 0.9f;
    float paramAliveG_ = 0.9f;
    float paramAliveB_ = 0.9f;
    float paramDeadR_ = 0.05f;
    float paramDeadG_ = 0.05f;
    float paramDeadB_ = 0.05f;
    float paramAlpha_ = 1.0f;
    float paramAliveAlpha_ = 1.0f;
    float paramDeadAlpha_ = 1.0f;
    bool paramPaused_ = false;
    bool paramReseedRequested_ = false;
    float paramSeedDensity_ = 0.35f;
    float paramPresetIndex_ = 0.0f;
    float paramFadeFrames_ = 1.0f;
    bool paramBpmSync_ = false;
    float paramBpmMultiplier_ = 1.0f;
    bool paramAutoReseed_ = false;
    float paramReseedQuantizeBeats_ = 4.0f;
    float paramAutoReseedEveryBeats_ = 16.0f;

    bool dirty_ = true;

    bool enabled_ = true;
    int gridW_ = 128;
    int gridH_ = 128;
    std::vector<uint8_t> cells_;
    std::vector<uint8_t> next_;
    std::vector<uint16_t> fadeFramesRemaining_;
    float accumulator_ = 0.0f;
    float stepInterval_ = 0.1f;
    int activePreset_ = 0;
    bool reseedPending_ = false;
    float pendingReseedBeat_ = -1.0f;
    float nextAutoReseedBeat_ = -1.0f;

    ofFloatPixels pixels_;
    ofTexture texture_;
};

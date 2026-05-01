#pragma once

#include "Layer.h"
#include "ofVideoPlayer.h"
#include "../media/VideoCatalog.h"
#include <string>

class VideoClipLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    void cycleClip(int delta);
    void setClipIndex(int index);
    std::string currentClipLabel() const;
    float gain() const { return paramGain_; }
    bool mirror() const { return paramMirror_; }
    bool loop() const { return paramLoop_; }

    void adjustGain(float delta);
    void toggleMirror();
    void toggleLoop();

    float* gainParamPtr() { return &paramGain_; }
    float* clipParamPtr() { return &paramClipIndex_; }
    bool* mirrorParamPtr() { return &paramMirror_; }
    bool* enabledParamPtr() { return &paramEnabled_; }
    bool* loopParamPtr() { return &paramLoop_; }

private:
    void applyClipSelection();

    bool paramEnabled_ = true;
    float paramGain_ = 1.0f;
    float paramClipIndex_ = 0.0f;
    bool paramMirror_ = false;
    bool paramLoop_ = true;

    bool enabled_ = true;
    int selectedClipIndex_ = -1;
    bool clipDirty_ = true;

    std::string configClipId_;

    ofVideoPlayer player_;
};
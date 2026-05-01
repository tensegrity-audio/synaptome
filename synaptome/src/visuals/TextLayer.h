#pragma once

#include "Layer.h"
#include "TextLayerState.h"
#include "ofTrueTypeFont.h"

class TextLayer : public Layer {
public:
    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void setExternalEnabled(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

private:
    bool ensureFontLoaded();

    TextLayerState* state_ = nullptr;
    bool enabled_ = true;
    ofTrueTypeFont font_;
    bool fontLoaded_ = false;
    std::string loadedFontName_;
    int loadedFontSize_ = 0;
    ofColor textColor_ = ofColor::white;
};

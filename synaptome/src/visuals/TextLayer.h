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
    bool ensureCornerFontLoaded();
    bool loadFontAtSize(ofTrueTypeFont& font,
                        bool& fontLoaded,
                        std::string& loadedFontName,
                        int& loadedFontSize,
                        int size);

    TextLayerState* state_ = nullptr;
    bool enabled_ = true;
    ofTrueTypeFont font_;
    ofTrueTypeFont cornerFont_;
    bool fontLoaded_ = false;
    bool cornerFontLoaded_ = false;
    std::string loadedFontName_;
    std::string loadedCornerFontName_;
    int loadedFontSize_ = 0;
    int loadedCornerFontSize_ = 0;
    ofColor textColor_ = ofColor::white;
};

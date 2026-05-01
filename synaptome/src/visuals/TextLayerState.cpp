#include "TextLayerState.h"

#include "ofFileUtils.h"
#include "ofMath.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr float kFontSizeMin = 8.0f;
    constexpr float kFontSizeMax = 256.0f;
}

TextLayerState& TextLayerState::instance() {
    static TextLayerState state;
    return state;
}

void TextLayerState::refreshAvailableFonts() {
    availableFonts_.clear();
    ofDirectory dir(ofToDataPath("fonts", true));
    if (dir.exists()) {
        dir.allowExt("ttf");
        dir.allowExt("otf");
        dir.listDir();
        for (const auto& file : dir.getFiles()) {
            availableFonts_.push_back(file.getFileName());
        }
    }
    if (availableFonts_.empty()) {
        availableFonts_.push_back(font.empty() ? std::string("VCR_OSD_MONO_1.001.ttf") : font);
    }
    std::sort(availableFonts_.begin(), availableFonts_.end());
    auto it = std::find(availableFonts_.begin(), availableFonts_.end(), font);
    if (it == availableFonts_.end()) {
        font = availableFonts_.front();
        it = availableFonts_.begin();
    }
    fontIndex = static_cast<float>(std::distance(availableFonts_.begin(), it));
    fontIndexSnapshot_ = fontIndex;
    fontNameSnapshot_ = font;
}

void TextLayerState::syncFontSelection() {
    if (availableFonts_.empty()) {
        refreshAvailableFonts();
    }
    if (availableFonts_.empty()) {
        return;
    }

    int sliderIndex = static_cast<int>(std::round(fontIndex));
    sliderIndex = ofClamp(sliderIndex, 0, static_cast<int>(availableFonts_.size() - 1));
    float snapped = static_cast<float>(sliderIndex);
    bool sliderChanged = std::fabs(snapped - fontIndexSnapshot_) > 0.01f;
    bool nameChanged = (font != fontNameSnapshot_);

    if (nameChanged) {
        auto it = std::find(availableFonts_.begin(), availableFonts_.end(), font);
        if (it != availableFonts_.end()) {
            sliderIndex = static_cast<int>(std::distance(availableFonts_.begin(), it));
            snapped = static_cast<float>(sliderIndex);
        } else if (!availableFonts_.empty()) {
            sliderIndex = 0;
            snapped = 0.0f;
            font = availableFonts_.front();
        }
    } else if (!sliderChanged) {
        fontIndexSnapshot_ = snapped;
        fontNameSnapshot_ = font;
        fontSize = ofClamp(fontSize, kFontSizeMin, kFontSizeMax);
        return;
    }

    if (sliderChanged && !nameChanged) {
        font = availableFonts_[sliderIndex];
    }

    fontIndex = snapped;
    fontIndexSnapshot_ = snapped;
    fontNameSnapshot_ = font;
    fontSize = ofClamp(fontSize, kFontSizeMin, kFontSizeMax);
}

float TextLayerState::fontIndexMax() const {
    return availableFonts_.empty()
               ? 0.0f
               : static_cast<float>(std::max(0, static_cast<int>(availableFonts_.size()) - 1));
}

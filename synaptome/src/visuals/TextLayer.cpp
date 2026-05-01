#include "TextLayer.h"

#include "ofGraphics.h"
#include "ofLog.h"
#include "ofUtils.h"
#include <cmath>

namespace {
    constexpr float kColorMin = 0.0f;
    constexpr float kColorMax = 255.0f;
    constexpr float kFontSizeMin = 8.0f;
    constexpr float kFontSizeMax = 256.0f;
}

void TextLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }
    auto& state = TextLayerState::instance();
    const auto& defaults = config["defaults"];
    state.content = defaults.value("content", state.content);
    state.font = defaults.value("font", state.font);
    state.fontSize = defaults.value("size", state.fontSize);
    if (defaults.contains("color") && defaults["color"].is_array()) {
        const auto& arr = defaults["color"];
        if (arr.size() >= 3) {
            state.colorR = static_cast<float>(arr[0].get<double>());
            state.colorG = static_cast<float>(arr[1].get<double>());
            state.colorB = static_cast<float>(arr[2].get<double>());
        }
    }
}

void TextLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    state_ = &TextLayerState::instance();
    if (state_) {
        state_->refreshAvailableFonts();
        state_->syncFontSelection();
    }
    ensureFontLoaded();
}

void TextLayer::update(const LayerUpdateParams& params) {
    (void)params;
    if (!state_) {
        state_ = &TextLayerState::instance();
    }
    if (!state_) {
        return;
    }

    state_->syncFontSelection();
    state_->fontSize = ofClamp(state_->fontSize, kFontSizeMin, kFontSizeMax);
    state_->colorR = ofClamp(state_->colorR, kColorMin, kColorMax);
    state_->colorG = ofClamp(state_->colorG, kColorMin, kColorMax);
    state_->colorB = ofClamp(state_->colorB, kColorMin, kColorMax);

    textColor_.r = static_cast<unsigned char>(state_->colorR);
    textColor_.g = static_cast<unsigned char>(state_->colorG);
    textColor_.b = static_cast<unsigned char>(state_->colorB);
    ensureFontLoaded();
}

void TextLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (!state_ || state_->content.empty()) {
        return;
    }

    bool fontReady = ensureFontLoaded();

    ofPushStyle();
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    ofDisableDepthTest();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofColor color = textColor_;
    color.a = static_cast<unsigned char>(ofClamp(params.slotOpacity, 0.0f, 1.0f) * 255.0f);
    ofSetColor(color);

    glm::ivec2 viewport = params.viewport;
    if (viewport.x <= 0 || viewport.y <= 0) {
        viewport.x = ofGetWidth();
        viewport.y = ofGetHeight();
    }

    if (fontReady && font_.isLoaded()) {
        ofRectangle bounds = font_.getStringBoundingBox(state_->content, 0.0f, 0.0f);
        float x = (static_cast<float>(viewport.x) - bounds.width) * 0.5f;
        float y = (static_cast<float>(viewport.y) - bounds.height) * 0.5f;
        font_.drawString(state_->content, x - bounds.x, y - bounds.y);
    } else {
        ofDrawBitmapStringHighlight(state_->content,
                                    20.0f,
                                    static_cast<float>(viewport.y) - 40.0f);
    }

    if (depthWasEnabled) {
        ofEnableDepthTest();
    } else {
        ofDisableDepthTest();
    }
    ofDisableBlendMode();
    ofPopStyle();
}

bool TextLayer::ensureFontLoaded() {
    if (!state_) {
        return false;
    }
    std::string fontName = state_->font.empty() ? std::string("VCR_OSD_MONO_1.001.ttf") : state_->font;
    int size = static_cast<int>(std::round(ofClamp(state_->fontSize, kFontSizeMin, kFontSizeMax)));
    if (fontLoaded_ && loadedFontName_ == fontName && loadedFontSize_ == size && font_.isLoaded()) {
        return true;
    }

    std::string fontPath = ofToDataPath("fonts/" + fontName, true);
    if (!ofFile::doesFileExist(fontPath)) {
        if (ofFile::doesFileExist(fontName)) {
            fontPath = fontName;
        } else {
            ofLogWarning("TextLayer") << "Font not found: " << fontName;
            fontLoaded_ = false;
            return false;
        }
    }

    ofTrueTypeFontSettings settings(fontPath, size);
    settings.addRanges(ofAlphabet::Latin);
    settings.dpi = 96;
    settings.antialiased = true;
    settings.contours = false;

    if (!font_.load(settings)) {
        ofLogWarning("TextLayer") << "Failed to load font: " << fontPath;
        fontLoaded_ = false;
        return false;
    }

    fontLoaded_ = true;
    loadedFontName_ = fontName;
    loadedFontSize_ = size;
    return true;
}

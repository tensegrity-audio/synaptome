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
    constexpr float kTextMargin = 24.0f;

    enum class TextAnchor {
        Center,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };
}

void TextLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }
    auto& state = TextLayerState::instance();
    const auto& defaults = config["defaults"];
    state.content = defaults.value("content", state.content);
    state.topLeft = defaults.value("topLeft", state.topLeft);
    state.topRight = defaults.value("topRight", state.topRight);
    state.bottomLeft = defaults.value("bottomLeft", state.bottomLeft);
    state.bottomRight = defaults.value("bottomRight", state.bottomRight);
    state.font = defaults.value("font", state.font);
    state.fontSize = defaults.value("size", state.fontSize);
    state.cornerFontSize = defaults.value("cornerSize", state.cornerFontSize);
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
    ensureCornerFontLoaded();
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
    state_->cornerFontSize = ofClamp(state_->cornerFontSize, kFontSizeMin, kFontSizeMax);
    state_->colorR = ofClamp(state_->colorR, kColorMin, kColorMax);
    state_->colorG = ofClamp(state_->colorG, kColorMin, kColorMax);
    state_->colorB = ofClamp(state_->colorB, kColorMin, kColorMax);

    textColor_.r = static_cast<unsigned char>(state_->colorR);
    textColor_.g = static_cast<unsigned char>(state_->colorG);
    textColor_.b = static_cast<unsigned char>(state_->colorB);
    ensureFontLoaded();
    ensureCornerFontLoaded();
}

void TextLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (!state_) {
        return;
    }

    const bool hasText = !state_->content.empty() ||
                         !state_->topLeft.empty() ||
                         !state_->topRight.empty() ||
                         !state_->bottomLeft.empty() ||
                         !state_->bottomRight.empty();
    if (!hasText) {
        return;
    }

    bool fontReady = ensureFontLoaded();
    bool cornerFontReady = ensureCornerFontLoaded();

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

    auto drawFontSlot = [&](ofTrueTypeFont& font, const std::string& text, TextAnchor anchor) {
        if (text.empty()) {
            return;
        }

        ofRectangle bounds = font.getStringBoundingBox(text, 0.0f, 0.0f);
        float x = 0.0f;
        float y = 0.0f;

        switch (anchor) {
        case TextAnchor::Center:
            x = (static_cast<float>(viewport.x) - bounds.width) * 0.5f;
            y = (static_cast<float>(viewport.y) - bounds.height) * 0.5f;
            break;
        case TextAnchor::TopLeft:
            x = kTextMargin;
            y = kTextMargin;
            break;
        case TextAnchor::TopRight:
            x = static_cast<float>(viewport.x) - kTextMargin - bounds.width;
            y = kTextMargin;
            break;
        case TextAnchor::BottomLeft:
            x = kTextMargin;
            y = static_cast<float>(viewport.y) - kTextMargin - bounds.height;
            break;
        case TextAnchor::BottomRight:
            x = static_cast<float>(viewport.x) - kTextMargin - bounds.width;
            y = static_cast<float>(viewport.y) - kTextMargin - bounds.height;
            break;
        }

        font.drawString(text, x - bounds.x, y - bounds.y);
    };

    auto drawBitmapSlot = [&](const std::string& text, TextAnchor anchor) {
        if (text.empty()) {
            return;
        }

        constexpr float kBitmapCharWidth = 8.0f;
        constexpr float kBitmapLineHeight = 12.0f;
        float width = static_cast<float>(text.size()) * kBitmapCharWidth;
        float x = kTextMargin;
        float y = kTextMargin + kBitmapLineHeight;

        switch (anchor) {
        case TextAnchor::Center:
            x = (static_cast<float>(viewport.x) - width) * 0.5f;
            y = static_cast<float>(viewport.y) * 0.5f;
            break;
        case TextAnchor::TopLeft:
            break;
        case TextAnchor::TopRight:
            x = static_cast<float>(viewport.x) - kTextMargin - width;
            break;
        case TextAnchor::BottomLeft:
            y = static_cast<float>(viewport.y) - kTextMargin;
            break;
        case TextAnchor::BottomRight:
            x = static_cast<float>(viewport.x) - kTextMargin - width;
            y = static_cast<float>(viewport.y) - kTextMargin;
            break;
        }

        ofDrawBitmapString(text, x, y);
    };

    if (fontReady && font_.isLoaded()) {
        drawFontSlot(font_, state_->content, TextAnchor::Center);
    } else {
        drawBitmapSlot(state_->content, TextAnchor::Center);
    }

    if (cornerFontReady && cornerFont_.isLoaded()) {
        drawFontSlot(cornerFont_, state_->topLeft, TextAnchor::TopLeft);
        drawFontSlot(cornerFont_, state_->topRight, TextAnchor::TopRight);
        drawFontSlot(cornerFont_, state_->bottomLeft, TextAnchor::BottomLeft);
        drawFontSlot(cornerFont_, state_->bottomRight, TextAnchor::BottomRight);
    } else {
        drawBitmapSlot(state_->topLeft, TextAnchor::TopLeft);
        drawBitmapSlot(state_->topRight, TextAnchor::TopRight);
        drawBitmapSlot(state_->bottomLeft, TextAnchor::BottomLeft);
        drawBitmapSlot(state_->bottomRight, TextAnchor::BottomRight);
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
    int size = static_cast<int>(std::round(ofClamp(state_->fontSize, kFontSizeMin, kFontSizeMax)));
    return loadFontAtSize(font_, fontLoaded_, loadedFontName_, loadedFontSize_, size);
}

bool TextLayer::ensureCornerFontLoaded() {
    if (!state_) {
        return false;
    }
    int size = static_cast<int>(std::round(ofClamp(state_->cornerFontSize, kFontSizeMin, kFontSizeMax)));
    return loadFontAtSize(cornerFont_, cornerFontLoaded_, loadedCornerFontName_, loadedCornerFontSize_, size);
}

bool TextLayer::loadFontAtSize(ofTrueTypeFont& font,
                               bool& fontLoaded,
                               std::string& loadedFontName,
                               int& loadedFontSize,
                               int size) {
    if (!state_) {
        return false;
    }
    std::string fontName = state_->font.empty() ? std::string("VCR_OSD_MONO_1.001.ttf") : state_->font;
    if (fontLoaded && loadedFontName == fontName && loadedFontSize == size && font.isLoaded()) {
        return true;
    }

    std::string fontPath = ofToDataPath("fonts/" + fontName, true);
    if (!ofFile::doesFileExist(fontPath)) {
        if (ofFile::doesFileExist(fontName)) {
            fontPath = fontName;
        } else {
            ofLogWarning("TextLayer") << "Font not found: " << fontName;
            fontLoaded = false;
            return false;
        }
    }

    ofTrueTypeFontSettings settings(fontPath, size);
    settings.addRanges(ofAlphabet::Latin);
    settings.dpi = 96;
    settings.antialiased = true;
    settings.contours = false;

    if (!font.load(settings)) {
        ofLogWarning("TextLayer") << "Failed to load font: " << fontPath;
        fontLoaded = false;
        return false;
    }

    fontLoaded = true;
    loadedFontName = fontName;
    loadedFontSize = size;
    return true;
}

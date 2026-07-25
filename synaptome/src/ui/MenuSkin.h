#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>

#include "ofBitmapFont.h"
#include "ofFileUtils.h"
#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMesh.h"
#include "ofTrueTypeFont.h"

struct HudSkin {
    ofColor overlayBackground = ofColor(8, 8, 8, 230);
    ofColor overlayChrome = ofColor(0, 255, 120, 140);
    ofColor textPrimary = ofColor(235, 255, 245);
    ofColor textMuted = ofColor(150, 165, 160);
    ofColor accent = ofColor(0, 255, 140);
    ofColor warning = ofColor(255, 110, 110);
    ofColor badgeBackground = ofColor(20, 20, 20, 235);
    ofColor badgeText = ofColor(235, 255, 245);
    ofColor badgeStroke = ofColor(0, 255, 140, 150);
    float typographyScale = 1.0f;
    float lineHeight = 18.0f;
    float blockPadding = 14.0f;
    float badgeCornerRadius = 4.0f;
    float badgePadding = 6.0f;
    float badgeHeight = 22.0f;
};

class UiFontRenderer {
public:
    void draw(const std::string& text, float x, float y, float scale, bool bold) {
        if (ofTrueTypeFont* font = fontForScale(scale)) {
            font->drawString(text, x, y);
            if (bold) {
                font->drawString(text, x + 1.0f, y);
            }
            return;
        }
        drawBitmapFallback(text, x, y, scale, bold);
    }

    ofRectangle bounds(const std::string& text, float x, float y, float scale) {
        if (ofTrueTypeFont* font = fontForScale(scale)) {
            return font->getStringBoundingBox(text, x, y);
        }

        const float safeScale = std::max(0.01f, scale);
        const ofRectangle raw = bitmapFont_.getBoundingBox(text, 0, 0);
        return ofRectangle(x + raw.getX() * safeScale,
                           y + raw.getY() * safeScale,
                           raw.getWidth() * safeScale,
                           raw.getHeight() * safeScale);
    }

private:
    static constexpr float kBasePixelSize = 14.0f;
    static constexpr int kMinPixelSize = 8;
    static constexpr int kMaxPixelSize = 64;
    static constexpr const char* kFontDataPath = "fonts/unifont-17.0.05.otf";

    ofTrueTypeFont* fontForScale(float scale) {
        const float safeScale = std::max(0.01f, scale);
        const int pixelSize = std::clamp(
            static_cast<int>(std::lround(kBasePixelSize * safeScale)),
            kMinPixelSize,
            kMaxPixelSize);
        if (font_ && loadedPixelSize_ == pixelSize) {
            return font_.get();
        }
        if (failedPixelSize_ == pixelSize) {
            return nullptr;
        }

        auto candidate = std::make_unique<ofTrueTypeFont>();
        ofTrueTypeFontSettings settings(ofToDataPath(kFontDataPath, true), pixelSize);
        settings.antialiased = true;
        settings.contours = false;
        settings.addRange(ofUnicode::Latin);
        settings.addRange(ofUnicode::Latin1Supplement);
        settings.addRange(ofUnicode::GeneralPunctuation);
        settings.addRange(ofUnicode::CurrencySymbols);
        settings.addRange(ofUnicode::Arrows);
        settings.addRange(ofUnicode::MathOperators);
        settings.addRange(ofUnicode::BoxDrawing);
        settings.addRange(ofUnicode::BlockElement);
        settings.addRange(ofUnicode::GeometricShapes);
        settings.addRange(ofUnicode::MiscSymbols);

        if (!candidate->load(settings)) {
            failedPixelSize_ = pixelSize;
            if (!fontFailureLogged_) {
                ofLogWarning("UiFontRenderer")
                    << "Failed to load " << kFontDataPath
                    << "; falling back to the built-in bitmap font.";
                fontFailureLogged_ = true;
            }
            return nullptr;
        }

        font_ = std::move(candidate);
        loadedPixelSize_ = pixelSize;
        failedPixelSize_ = -1;
        return font_.get();
    }

    void drawBitmapFallback(const std::string& text,
                            float x,
                            float y,
                            float scale,
                            bool bold) {
        const float safeScale = std::max(0.01f, scale);
        ofPushMatrix();
        ofTranslate(x, y);
        ofScale(safeScale, safeScale);
        const ofTexture& texture = bitmapFont_.getTexture();
        texture.bind();
        bitmapFont_.getMesh(text, 0, 0).draw();
        if (bold) {
            ofPushMatrix();
            ofTranslate(1.0f / safeScale, 0.0f);
            bitmapFont_.getMesh(text, 0, 0).draw();
            ofPopMatrix();
        }
        texture.unbind();
        ofPopMatrix();
    }

    std::unique_ptr<ofTrueTypeFont> font_;
    ofBitmapFont bitmapFont_;
    int loadedPixelSize_ = -1;
    int failedPixelSize_ = -1;
    bool fontFailureLogged_ = false;
};

inline UiFontRenderer& uiFontRenderer() {
    static UiFontRenderer renderer;
    return renderer;
}

inline void drawBitmapStringScaled(const std::string& text,
                                   float x,
                                   float y,
                                   float scale,
                                   bool bold = false) {
    uiFontRenderer().draw(text, x, y, scale, bold);
}

inline float measureUiStringWidth(const std::string& text, float scale) {
    return uiFontRenderer().bounds(text, 0.0f, 0.0f, scale).getWidth();
}

inline void drawBitmapStringHighlightScaled(const std::string& text,
                                            float x,
                                            float y,
                                            float scale) {
    const float safeScale = std::max(0.01f, scale);
    const float padding = 4.0f * safeScale;
    const ofRectangle textBounds = uiFontRenderer().bounds(text, x, y, safeScale);
    ofPushStyle();
    ofFill();
    ofSetColor(ofColor::black);
    ofDrawRectangle(textBounds.getX() - padding,
                    textBounds.getY() - padding,
                    textBounds.getWidth() + padding * 2.0f,
                    textBounds.getHeight() + padding * 2.0f);
    ofSetColor(ofColor::white);
    uiFontRenderer().draw(text, x, y, safeScale, false);
    ofPopStyle();
}

struct MenuSkin {
    struct ColumnDescriptor {
        std::string id;
        std::string label;
        float weight = 0.0f;
    };

    struct OscPickerLayout {
        std::array<ColumnDescriptor, 2> sourceColumns;
        std::array<ColumnDescriptor, 4> editorColumns;
    };

    struct Palette {
        ofColor background;
        ofColor surface;
        ofColor surfaceAlternate;
        ofColor border;
        ofColor headerText;
        ofColor bodyText;
        ofColor mutedText;
        ofColor treeSelection;
        ofColor treeFocus;
        ofColor gridSelection;
        ofColor gridSelectionFill;
        ofColor gridDivider;
        ofColor accent;
        ofColor warning;
        ofColor slotActive;
        ofColor slotInactive;
        ofColor badgeBackground;
    };

    struct Metrics {
        float margin = 12.0f;
        float padding = 12.0f;
        float columnHeaderHeight = 22.0f;
        float rowHeight = 20.0f;
        float treeRowHeight = 20.0f;
        float typographyScale = 1.0f;
        float treeIndent = 18.0f;
        float panelSpacing = 12.0f;
        float borderRadius = 6.0f;
        float focusStroke = 2.0f;
        float treeMinWidth = 180.0f;
        float gridMinWidth = 360.0f;
        float treeMaxWidthRatio = 0.45f;
    };

    Palette palette;
    Metrics metrics;
    HudSkin hud;
    OscPickerLayout oscPicker;

    static MenuSkin ConsoleHub();
};

inline MenuSkin MenuSkin::ConsoleHub() {
    MenuSkin skin;
    skin.palette.background = ofColor(0, 0, 0, 255); // Global backdrop (raise RGB for brighter CRT glow)
    skin.palette.surface = ofColor(12, 12, 12, 235); // Primary panels (increase alpha for less translucency)
    skin.palette.surfaceAlternate = ofColor(18, 18, 18, 220); // Secondary strips/cards (tweak RGB for contrast bands)
    skin.palette.border = ofColor(90, 90, 90, 160); // Widget outlines (raise RGB for lighter bezel)
    skin.palette.headerText = ofColor(255, 255, 255); // Column headers & labels (tint slightly if you want off-white)
    skin.palette.bodyText = ofColor(245, 245, 245); // Main row text (push toward grey for dimmer text)
    skin.palette.mutedText = ofColor(150, 150, 150); // Disabled/hint text (adjust RGB for lighter/darker hints)
    skin.palette.treeSelection = ofColor(0, 255, 120); // Tree highlight stroke (lower G to desaturate the neon)
    skin.palette.treeFocus = ofColor(80, 255, 160); // Tree focus caret (edit RGB to change focus accent)
    skin.palette.gridSelection = ofColor(0, 255, 120); // Grid outline color (match to preferred neon accent)
    skin.palette.gridSelectionFill = ofColor(180, 180, 180, 40); // Grid selection fill (keep RGB equal for greyscale haze)
    skin.palette.gridDivider = ofColor(80, 80, 80, 140); // Column separators (raise values for brighter scanlines)
    skin.palette.accent = ofColor(0, 255, 120); // Buttons/badges accent (tame saturation by lowering green channel)
    skin.palette.warning = ofColor(255, 90, 90); // Warning text/icons (shift RGB for different alert hue)
    skin.palette.slotActive = ofColor(0, 255, 120); // Active console slot indicator (adjust alpha for glow strength)
    skin.palette.slotInactive = ofColor(100, 100, 100); // Inactive slot outlines (raise RGB for lighter inactive state)
    skin.palette.badgeBackground = ofColor(26, 26, 26, 230); // Popover badges (tweak alpha for more/less translucency)
    skin.metrics.margin = 10.0f;
    skin.metrics.padding = 12.0f;
    skin.metrics.columnHeaderHeight = 22.0f;
    skin.metrics.rowHeight = 20.0f;
    skin.metrics.treeRowHeight = 20.0f;
    skin.metrics.typographyScale = 1.0f;
    skin.metrics.treeIndent = 18.0f;
    skin.metrics.panelSpacing = 12.0f;
    skin.metrics.borderRadius = 6.0f;
    skin.metrics.focusStroke = 2.0f;
    skin.metrics.treeMinWidth = 200.0f;
    skin.metrics.gridMinWidth = 320.0f;
    skin.metrics.treeMaxWidthRatio = 0.4f;
    skin.hud.overlayBackground = ofColor(6, 6, 6, 228);
    skin.hud.overlayChrome = ofColor(0, 255, 140, 120);
    skin.hud.textPrimary = ofColor(235, 255, 245);
    skin.hud.textMuted = ofColor(150, 165, 160);
    skin.hud.accent = ofColor(0, 255, 140);
    skin.hud.warning = ofColor(255, 120, 120);
    skin.hud.badgeBackground = ofColor(18, 18, 18, 240);
    skin.hud.badgeText = ofColor(235, 255, 245);
    skin.hud.badgeStroke = ofColor(0, 255, 120, 150);
    skin.hud.typographyScale = 1.0f;
    skin.hud.lineHeight = 18.0f;
    skin.hud.blockPadding = 14.0f;
    skin.hud.badgeCornerRadius = 4.0f;
    skin.hud.badgePadding = 6.0f;
    skin.hud.badgeHeight = 22.0f;
    skin.oscPicker.sourceColumns = {{
        { "address", "Source Address", 0.7f },
        { "value", "Live Value", 0.3f }
    }};
    skin.oscPicker.editorColumns = {{
        { "inMin", "Input Min", 0.25f },
        { "inMax", "Input Max", 0.25f },
        { "outMinFactor", "Output Min x", 0.25f },
        { "outMaxFactor", "Output Max x", 0.25f }
    }};
    return skin;
}

#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "ofBitmapFont.h"
#include "ofGraphics.h"
#include "ofMesh.h"

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

inline void drawBitmapStringScaled(const std::string& text,
                                   float x,
                                   float y,
                                   float scale,
                                   bool bold = false) {
    float safeScale = std::max(0.01f, scale);
    static ofBitmapFont bitmapFont;
    ofPushMatrix();
    ofTranslate(x, y);
    ofScale(safeScale, safeScale);
    const ofTexture& texture = bitmapFont.getTexture();
    texture.bind();
    if (bold) {
        bitmapFont.getMesh(text, 0, 0).draw();
        ofPushMatrix();
        ofTranslate(1.0f / safeScale, 0.0f);
        bitmapFont.getMesh(text, 0, 0).draw();
        ofPopMatrix();
    } else {
        bitmapFont.getMesh(text, 0, 0).draw();
    }
    texture.unbind();
    ofPopMatrix();
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

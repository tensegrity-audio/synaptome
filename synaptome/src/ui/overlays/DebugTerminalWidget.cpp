#include "DebugTerminalWidget.h"

#include "ofGraphics.h"

#include "HudThemeUtils.h"
#include "../../ofApp.h"

DebugTerminalWidget::DebugTerminalWidget() {
    metadata_.id = "hud.debug.terminal";
    metadata_.label = "Debug Terminal";
    metadata_.category = "HUD";
    metadata_.description = "Compact runtime log and diagnostics context.";
    metadata_.defaultColumn = 0;
    metadata_.defaultHeight = 280.0f;
    metadata_.minHeight = 180.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
}

void DebugTerminalWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void DebugTerminalWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void DebugTerminalWidget::draw(const DrawParams& params) {
    if (!app_) {
        return;
    }
    std::string text = app_->composeHudDebugTerminal();
    text = hudEllipsizeText(text, params.bounds.width, hudSkin_, hudMaxVisibleLines(params.bounds.height, hudSkin_));
    if (text.empty()) {
        return;
    }

    ofPushStyle();
    drawHudPanelBackground(params.bounds, hudSkin_);
    ofSetColor(hudAccentColor(hudSkin_));
    float textX = params.bounds.x + hudBlockPadding(hudSkin_);
    float textY = params.bounds.y + hudBlockPadding(hudSkin_) + hudLineHeight(hudSkin_);
    drawBitmapStringScaled(text, textX, textY, hudTypographyScale(hudSkin_));
    ofPopStyle();
}

float DebugTerminalWidget::preferredHeight(float width) const {
    if (!app_) {
        return metadata_.defaultHeight;
    }
    std::string text = app_->composeHudDebugTerminal();
    text = hudEllipsizeText(text, width, hudSkin_);
    return computeHudTextHeight(text, metadata_.minHeight, hudSkin_);
}

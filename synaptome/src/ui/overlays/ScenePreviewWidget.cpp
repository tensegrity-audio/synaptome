#include "ScenePreviewWidget.h"

#include "../../ofApp.h"
#include "HudThemeUtils.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>

namespace {
    ofColor withAlpha(ofColor color, int alpha) {
        color.a = static_cast<unsigned char>(ofClamp(alpha, 0, 255));
        return color;
    }

    ofRectangle fitAspect(const ofRectangle& bounds, float aspect) {
        if (bounds.width <= 0.0f || bounds.height <= 0.0f || aspect <= 0.0f) {
            return ofRectangle(bounds.x, bounds.y, 0.0f, 0.0f);
        }
        float width = bounds.width;
        float height = width / aspect;
        if (height > bounds.height) {
            height = bounds.height;
            width = height * aspect;
        }
        return ofRectangle(bounds.x + (bounds.width - width) * 0.5f,
                           bounds.y + (bounds.height - height) * 0.5f,
                           width,
                           height);
    }
}

ScenePreviewWidget::ScenePreviewWidget() {
    metadata_.id = "hud.scene.preview";
    metadata_.label = "Scene Preview";
    metadata_.category = "HUD";
    metadata_.description = "Shows a live preview of the current scene composite.";
    metadata_.defaultColumn = 2;
    metadata_.defaultHeight = 220.0f;
    metadata_.minHeight = 150.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
    metadata_.target = OverlayWidget::Target::Controller;
}

void ScenePreviewWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void ScenePreviewWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void ScenePreviewWidget::draw(const DrawParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }

    const float padding = hudBlockPadding(hudSkin_);
    const float lineH = hudLineHeight(hudSkin_);
    const float scale = hudTypographyScale(hudSkin_);
    const float contentX = params.bounds.x + padding;
    const float contentY = params.bounds.y + padding;
    const float contentW = std::max(1.0f, params.bounds.width - padding * 2.0f);
    const float contentBottom = params.bounds.y + params.bounds.height - padding;
    const ofRectangle previewBounds(contentX,
                                    contentY + lineH * 1.55f,
                                    contentW,
                                    std::max(1.0f, contentBottom - (contentY + lineH * 1.55f)));
    const ofRectangle previewRect = fitAspect(previewBounds, 16.0f / 9.0f);
    const bool available = app_ && app_->hasScenePreview();

    ofPushStyle();
    drawHudPanelBackground(params.bounds, hudSkin_);

    ofSetColor(available ? hudAccentColor(hudSkin_) : hudMutedColor(hudSkin_));
    drawBitmapStringScaled("Scene Preview", contentX, contentY + lineH, scale, true);

    ofSetColor(withAlpha(hudBadgeFill(hudSkin_), 220));
    ofDrawRectangle(previewBounds.x, previewBounds.y, previewBounds.width, previewBounds.height);
    ofSetColor(withAlpha(hudChromeColor(hudSkin_), available ? 120 : 60));
    ofNoFill();
    ofDrawRectangle(previewBounds.x, previewBounds.y, previewBounds.width, previewBounds.height);
    ofFill();

    if (available && previewRect.width > 0.0f && previewRect.height > 0.0f) {
        ofSetColor(255);
        app_->drawScenePreview(previewRect);
        ofSetColor(withAlpha(hudChromeColor(hudSkin_), 170));
        ofNoFill();
        ofDrawRectangle(previewRect.x, previewRect.y, previewRect.width, previewRect.height);
        ofFill();
    } else {
        ofSetColor(hudMutedColor(hudSkin_));
        drawBitmapStringScaled("Waiting for scene frame",
                               previewBounds.x + padding * 0.5f,
                               previewBounds.y + previewBounds.height * 0.5f,
                               scale);
    }

    ofPopStyle();
}

float ScenePreviewWidget::preferredHeight(float width) const {
    const float padding = hudBlockPadding(hudSkin_);
    const float titleH = hudLineHeight(hudSkin_) * 1.55f;
    const float contentW = std::max(1.0f, width - padding * 2.0f);
    const float previewH = contentW * 9.0f / 16.0f;
    return std::max(metadata_.minHeight, padding * 2.0f + titleH + previewH);
}

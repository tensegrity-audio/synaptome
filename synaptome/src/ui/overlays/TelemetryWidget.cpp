#include "TelemetryWidget.h"
#include "../../ofApp.h"
#include "ofGraphics.h"

#include "HudThemeUtils.h"

TelemetryWidget::TelemetryWidget() {
    meta_.id = "telemetry";
    meta_.label = "Telemetry";
    meta_.category = "HUD";
    meta_.description = "Shows fps, rssi, battery, latency";
    meta_.defaultColumn = 0;
    meta_.defaultHeight = 120.0f;
    meta_.band = OverlayWidget::Band::Hud;
}

void TelemetryWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void TelemetryWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
    float fallbackFps = ofGetFrameRate();
    fps_ = app_ ? app_->hudTelemetryValueOr(meta_.id, "fps", fallbackFps, 1500) : fallbackFps;
    gpuPercent_ = app_ ? app_->hudTelemetryValueOr(meta_.id, "gpu", -1.0f, 2000) : -1.0f;
    if (app_) {
        if (app_->hudDeckActivity.hr.hasValue) {
            lastLine_ = "HR: " + std::to_string(app_->hudDeckActivity.hr.lastValue);
        } else if (app_->hudMatrixActivity.mic.hasValue) {
            lastLine_ = "MIC: " + std::to_string(app_->hudMatrixActivity.mic.lastValue);
        } else {
            lastLine_.clear();
        }
    }
}

void TelemetryWidget::draw(const DrawParams& params) {
    ofPushStyle();
    drawHudPanelBackground(params.bounds, hudSkin_);
    float padding = hudBlockPadding(hudSkin_);
    float lineH = hudLineHeight(hudSkin_);
    float x = params.bounds.x + padding;
    float y = params.bounds.y + padding + lineH;
    ofSetColor(hudTextColor(hudSkin_));
    drawBitmapStringScaled("FPS: " + ofToString(fps_, 1), x, y, hudTypographyScale(hudSkin_));
    y += lineH;
    std::string gpuText;
    ofColor gpuColor;
    if (gpuPercent_ >= 0.0f) {
        gpuText = "GPU: " + ofToString(gpuPercent_, 1) + " %";
        gpuColor = hudTextColor(hudSkin_);
    } else {
        gpuText = "GPU: n/a (counter unavailable)";
        gpuColor = hudMutedColor(hudSkin_);
    }
    ofSetColor(gpuColor);
    drawBitmapStringScaled(gpuText, x, y, hudTypographyScale(hudSkin_));
    y += lineH;
    if (!lastLine_.empty()) {
        ofSetColor(hudMutedColor(hudSkin_));
        drawBitmapStringScaled(lastLine_, x, y, hudTypographyScale(hudSkin_));
    }
    ofPopStyle();
}

#include "SensorsHudWidget.h"

#include <algorithm>
#include <sstream>

#include "ofGraphics.h"
#include "ofJson.h"

#include "HudThemeUtils.h"
#include "../../ofApp.h"

namespace {
std::string composeSensorsFromFeed(const ofJson& payload) {
    std::ostringstream out;
    out << "Connected Devices:";
    if (payload.contains("devices") && payload["devices"].is_array()) {
        for (const auto& entry : payload["devices"]) {
            if (!entry.is_object()) continue;
            const bool connected = entry.value("connected", false);
            const std::string label = entry.value("label", std::string("Device"));
            const std::string kind = entry.value("kind", std::string());
            const std::string detail = entry.value("detail", std::string());
            out << "\n  " << (connected ? "[*] " : "[ ] ");
            if (!kind.empty()) {
                out << kind << ": ";
            }
            out << label;
            if (!detail.empty()) {
                out << "  " << detail;
            }
        }
    }
    if (payload.contains("oscHistory") && payload["oscHistory"].is_array()) {
        out << "\nRecent OSC:";
        for (const auto& entry : payload["oscHistory"]) {
            if (!entry.is_object()) continue;
            const std::string address = entry.value("address", std::string());
            const float value = entry.value("value", 0.0f);
            out << "\n  " << address << " -> " << ofToString(value, 4);
        }
    }
    return out.str();
}
}

SensorsHudWidget::SensorsHudWidget() {
    metadata_.id = "hud.sensors";
    metadata_.label = "Connected Devices";
    metadata_.category = "HUD";
    metadata_.description = "Runtime connection summary for cameras, MIDI, audio, OSC, and sensor buses.";
    metadata_.defaultColumn = 1;
    metadata_.defaultHeight = 280.0f;
    metadata_.minHeight = 180.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
}

void SensorsHudWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void SensorsHudWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void SensorsHudWidget::draw(const DrawParams& params) {
    if (!app_) {
        return;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.sensors")) {
        text = composeSensorsFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudSensors();
    }
    text = hudEllipsizeText(text, params.bounds.width, hudSkin_, hudMaxVisibleLines(params.bounds.height, hudSkin_));
    if (text.empty()) {
        return;
    }

    ofPushStyle();
    drawHudPanelBackground(params.bounds, hudSkin_);
    ofSetColor(hudTextColor(hudSkin_));
    float textX = params.bounds.x + hudBlockPadding(hudSkin_);
    float textY = params.bounds.y + hudBlockPadding(hudSkin_) + hudLineHeight(hudSkin_);
    drawBitmapStringScaled(text, textX, textY, hudTypographyScale(hudSkin_));
    ofPopStyle();
}

float SensorsHudWidget::preferredHeight(float width) const {
    if (!app_) {
        return metadata_.defaultHeight;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.sensors")) {
        text = composeSensorsFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudSensors();
    }
    text = hudEllipsizeText(text, width, hudSkin_);
    return computeHudTextHeight(text, metadata_.minHeight, hudSkin_);
}

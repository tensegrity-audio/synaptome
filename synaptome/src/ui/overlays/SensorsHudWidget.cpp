#include "SensorsHudWidget.h"

#include <algorithm>
#include <sstream>

#include "ofGraphics.h"
#include "ofJson.h"

#include "HudThemeUtils.h"
#include "../../ofApp.h"

namespace {
std::string composeSensorsFromFeed(const ofJson& payload) {
    auto indicator = [](const ofJson& node) -> std::string {
        return node.value("indicator", std::string("[ ]"));
    };
    auto categoryLine = [&](const char* label, const ofJson& node) -> std::string {
        std::ostringstream line;
        line << label << " " << indicator(node);
        if (node.value("hasValue", false)) {
            line << " " << ofToString(node.value("lastValue", 0.0f), 3);
        }
        std::string metric = node.value("lastMetric", std::string());
        if (!metric.empty()) {
            line << " (" << metric << ")";
        }
        float ageMs = node.value("ageMs", 0.0f);
        if (ageMs > 0.0f) {
            line << "  (" << ofToString(ageMs / 1000.0f, 1) << "s ago)";
        }
        return line.str();
    };

    std::ostringstream out;
    out << "\n\nSensor HUD:";
    if (payload.contains("deck") && payload["deck"].is_object()) {
        const auto& deck = payload["deck"];
        out << "\n  Cyberdeck TLVs " << indicator(deck);
        if (deck.contains("categories") && deck["categories"].is_object()) {
            const auto& cats = deck["categories"];
            if (cats.contains("hr")) {
                out << "\n    HR " << categoryLine("HR", cats["hr"]);
            }
            if (cats.contains("imu")) {
                out << "\n    IMU " << categoryLine("IMU", cats["imu"]);
            }
            if (cats.contains("aux")) {
                out << "\n    AUX " << categoryLine("AUX", cats["aux"]);
            }
        }
    }
    if (payload.contains("matrix") && payload["matrix"].is_object()) {
        const auto& matrix = payload["matrix"];
        out << "\n  Matrix TLVs " << indicator(matrix);
        if (matrix.contains("categories") && matrix["categories"].is_object()) {
            const auto& cats = matrix["categories"];
            if (cats.contains("mic")) {
                out << "\n    MIC " << categoryLine("MIC", cats["mic"]);
            }
            if (cats.contains("bio")) {
                out << "\n    BIO " << categoryLine("BIO", cats["bio"]);
            }
            if (cats.contains("imu")) {
                out << "\n    IMU " << categoryLine("IMU", cats["imu"]);
            }
            if (cats.contains("aux")) {
                out << "\n    AUX " << categoryLine("AUX", cats["aux"]);
            }
        }
    }
    out << "\n\nRecent OSC:";
    if (payload.contains("oscHistory") && payload["oscHistory"].is_array()) {
        for (const auto& entry : payload["oscHistory"]) {
            if (!entry.is_object()) continue;
            std::string address = entry.value("address", std::string());
            float value = entry.value("value", 0.0f);
            out << "\n  " << address << " -> " << ofToString(value, 4);
        }
    }
    return out.str();
}
}

SensorsHudWidget::SensorsHudWidget() {
    metadata_.id = "hud.sensors";
    metadata_.label = "Sensor Activity";
    metadata_.category = "HUD";
    metadata_.description = "Cyberdeck and matrix telemetry plus recent OSC.";
    metadata_.defaultColumn = 1;
    metadata_.defaultHeight = 360.0f;
    metadata_.minHeight = 220.0f;
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

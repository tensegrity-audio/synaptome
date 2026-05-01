#include "ControlsHudWidget.h"

#include <algorithm>
#include <sstream>

#include "ofGraphics.h"
#include "ofJson.h"

#include "HudThemeUtils.h"
#include "../../ofApp.h"

namespace {
std::string composeControlsFromFeed(const ofJson& payload) {
    if (!payload.contains("lines") || !payload["lines"].is_array()) {
        return std::string();
    }
    auto composeLine = [](const ofJson& lineNode) -> std::string {
        std::ostringstream line;
        std::string prefix = lineNode.value("prefix", std::string());
        if (!prefix.empty()) {
            line << prefix;
        }
        if (lineNode.contains("actions") && lineNode["actions"].is_array()) {
            bool first = true;
            for (const auto& action : lineNode["actions"]) {
                if (!first) {
                    line << "   ";
                }
                first = false;
                std::string keys = action.value("keys", std::string("Key"));
                std::string desc = action.value("description", std::string());
                line << "[" << keys << "]";
                if (!desc.empty()) {
                    line << " " << desc;
                }
            }
        }
        return line.str();
    };
    std::ostringstream out;
    bool firstLine = true;
    for (const auto& lineNode : payload["lines"]) {
        if (!lineNode.is_object()) {
            continue;
        }
        std::string composed = composeLine(lineNode);
        if (composed.empty()) {
            continue;
        }
        if (!firstLine) {
            out << "\n";
        }
        firstLine = false;
        out << composed;
    }
    return out.str();
}
}

ControlsHudWidget::ControlsHudWidget() {
    metadata_.id = "hud.controls";
    metadata_.label = "Control Hints";
    metadata_.category = "HUD";
    metadata_.description = "Keyboard and mouse quick reference.";
    metadata_.defaultColumn = 0;
    metadata_.defaultHeight = 240.0f;
    metadata_.minHeight = 160.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
}

void ControlsHudWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void ControlsHudWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void ControlsHudWidget::draw(const DrawParams& params) {
    if (!app_) {
        return;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.controls")) {
        text = composeControlsFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudControls();
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

float ControlsHudWidget::preferredHeight(float width) const {
    if (!app_) {
        return metadata_.defaultHeight;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.controls")) {
        text = composeControlsFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudControls();
    }
    text = hudEllipsizeText(text, width, hudSkin_);
    return computeHudTextHeight(text, metadata_.minHeight, hudSkin_);
}
#include "HudThemeUtils.h"
#include "../../ofApp.h"

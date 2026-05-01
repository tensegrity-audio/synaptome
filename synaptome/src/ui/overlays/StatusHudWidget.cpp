#include "StatusHudWidget.h"

#include <algorithm>
#include <sstream>

#include "ofGraphics.h"
#include "ofJson.h"

#include "HudThemeUtils.h"
#include "../../ofApp.h"

namespace {
std::string composeStatusFromFeed(const ofJson& payload) {
    std::ostringstream out;
    if (payload.contains("slots") && payload["slots"].is_object()) {
        const auto& slots = payload["slots"];
        int active = slots.value("active", 0);
        int assigned = slots.value("assigned", 0);
        int capacity = std::max(slots.value("capacity", 0), 1);
        out << "\nConsole slots: " << active << " active / " << capacity;
        out << "   Assigned: " << assigned;
    }
    if (payload.contains("fxRoutes") && payload["fxRoutes"].is_object()) {
        const auto& routes = payload["fxRoutes"];
        auto routeLine = [&](const char* label, const ofJson& node) {
            std::string state = node.value("state", std::string("Off"));
            std::ostringstream route;
            route << label << "=" << state;
            return route.str();
        };
        out << "\nFX routes: ";
        bool first = true;
        for (auto& kv : routes.items()) {
            if (!kv.value().is_object()) continue;
            if (!first) {
                out << "  ";
            }
            first = false;
            out << routeLine(kv.key().c_str(), kv.value());
        }
    }
    if (payload.contains("connections") && payload["connections"].is_object()) {
        const auto& connections = payload["connections"];
        if (connections.contains("midi")) {
            const auto& midi = connections["midi"];
            out << "\nMIDI: " << (midi.value("connected", false) ? "connected" : "not connected");
            std::string label = midi.value("label", std::string());
            if (!label.empty()) {
                out << " (" << label << ")";
            }
        }
        if (connections.contains("collector")) {
            const auto& collector = connections["collector"];
            out << "\nCollector: " << (collector.value("connected", false) ? "connected" : "searching...");
            std::string label = collector.value("label", std::string());
            if (!label.empty()) {
                out << " (" << label << ")";
            }
        }
    }
    if (payload.contains("activeBank")) {
        out << "\nActive MIDI bank: " << payload["activeBank"].get<std::string>();
    }
    if (payload.contains("takeovers") && payload["takeovers"].is_array()) {
        const auto& takeovers = payload["takeovers"];
        if (!takeovers.empty()) {
            out << "\nSoft takeover pending:";
            for (const auto& takeover : takeovers) {
                if (!takeover.is_object()) continue;
                std::string bank = takeover.value("bank", std::string("global"));
                std::string control = takeover.value("controlId", std::string());
                float delta = takeover.value("delta", 0.0f);
                out << "\n  [" << bank << "] " << control << " delta=" << ofToString(delta, 3);
            }
        }
    }
    if (payload.contains("oscSources") && payload["oscSources"].is_array()) {
        out << "\nOSC sources learned: " << ofToString(static_cast<int>(payload["oscSources"].size()));
    }
    return out.str();
}
}

StatusHudWidget::StatusHudWidget() {
    metadata_.id = "hud.status";
    metadata_.label = "System Status";
    metadata_.category = "HUD";
    metadata_.description = "MIDI devices, collector link, bank, and takeover state.";
    metadata_.defaultColumn = 0;
    metadata_.defaultHeight = 260.0f;
    metadata_.minHeight = 160.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
}

void StatusHudWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void StatusHudWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void StatusHudWidget::draw(const DrawParams& params) {
    if (!app_) {
        return;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.status")) {
        text = composeStatusFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudStatus();
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

float StatusHudWidget::preferredHeight(float width) const {
    if (!app_) {
        return metadata_.defaultHeight;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.status")) {
        text = composeStatusFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudStatus();
    }
    text = hudEllipsizeText(text, width, hudSkin_);
    return computeHudTextHeight(text, metadata_.minHeight, hudSkin_);
}

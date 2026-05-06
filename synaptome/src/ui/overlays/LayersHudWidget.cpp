#include "LayersHudWidget.h"

#include <algorithm>
#include <sstream>

#include "ofGraphics.h"
#include "ofJson.h"

#include "HudThemeUtils.h"
#include "../../ofApp.h"

namespace {
std::string composeLayersFromFeed(const ofJson& payload) {
    std::ostringstream out;
    if (payload.contains("summary") && payload["summary"].is_object()) {
        const auto& summary = payload["summary"];
        if (summary.contains("grid")) {
            const auto& grid = summary["grid"];
            out << "\nGrid segments: " << grid.value("segments", 0)
                << "   modes: " << grid.value("deformationSummary", std::string("flat"))
                << "   faces: " << ofToString(grid.value("faceOpacity", 0.0f), 2)
                << "   visible: " << (grid.value("visible", false) ? "yes" : "no");
        }
        if (summary.contains("geodesic")) {
            const auto& geo = summary["geodesic"];
            out << "\nSphere spin: " << ofToString(geo.value("spin", 0.0f), 1)
                << "   hover: " << ofToString(geo.value("hover", 0.0f), 1)
                << "   radius: " << ofToString(geo.value("radius", 0.0f), 1)
                << "   deform: " << (geo.value("deform", false) ? ofToString(geo.value("deformAmount", 0.0f), 1) : "off")
                << "   faces: " << ofToString(geo.value("faceOpacity", 0.0f), 2)
                << "   visible: " << (geo.value("visible", false) ? "yes" : "no");
        }
    }
    if (payload.contains("slots") && payload["slots"].is_array()) {
        out << "\nConsole slots:";
        const auto& slots = payload["slots"];
        if (slots.empty()) {
            out << " (none)";
        } else {
            for (const auto& slot : slots) {
                if (!slot.is_object()) continue;
                int index = slot.value("index", 0);
                out << "\n  [" << index << "] ";
                std::string label = slot.value("label", std::string());
                bool empty = slot.value("empty", false);
                if (empty || label.empty()) {
                    out << "(empty)";
                    continue;
                }
                bool active = slot.value("active", false);
                out << label << (active ? " *" : " (off)");
                if (slot.contains("metadata") && slot["metadata"].is_object()) {
                    const auto& meta = slot["metadata"];
                    std::string module = slot.value("module", std::string());
                    if (module == "video.grabber") {
                        out << "  [" << meta.value("deviceLabel", std::string()) << "]";
                        out << " gain=" << ofToString(meta.value("gain", 0.0f), 2);
                        if (meta.value("mirror", false)) out << " mirror";
                    } else if (module == "video.clip") {
                        out << "  [" << meta.value("clipLabel", std::string()) << "]";
                        out << " gain=" << ofToString(meta.value("gain", 0.0f), 2);
                        if (meta.value("mirror", false)) out << " mirror";
                        if (!meta.value("loop", true)) out << " loop=off";
                    } else if (module == "perlin") {
                        out << "  scale=" << ofToString(meta.value("scale", 0.0f), 2);
                        out << " texZoom=" << ofToString(meta.value("texelZoom", 0.0f), 2);
                        out << " oct=" << ofToString(meta.value("octaves", 0), 0);
                        out << " palette=" << ofToString(meta.value("paletteIndex", 0), 0);
                    } else if (module == "gameOfLife") {
                        out << (meta.value("paused", false) ? " paused" : " running");
                        out << " preset=" << ofToString(meta.value("preset", 0), 0);
                        out << " dens=" << ofToString(meta.value("density", 0.0f), 2);
                        out << " aAlpha=" << ofToString(meta.value("aliveAlpha", 0.0f), 2);
                        out << " dAlpha=" << ofToString(meta.value("deadAlpha", 0.0f), 2);
                    }
                    out << " opacity=" << ofToString(slot.value("opacity", 1.0f), 2);
                }
            }
        }
    }
    if (payload.contains("effects") && payload["effects"].is_object()) {
        auto routeName = [](const ofJson& node) -> std::string {
            return node.value("route", std::string("Off"));
        };
        const auto& fx = payload["effects"];
        out << "\nEffects:";
        if (fx.contains("dither")) {
            const auto& dither = fx["dither"];
            out << " Dither=" << routeName(dither)
                << " (cell " << ofToString(dither.value("cellSize", 0)) << ")";
        }
        if (fx.contains("ascii")) {
            const auto& ascii = fx["ascii"];
            out << " ASCII=" << routeName(ascii)
                << " (mode " << ofToString(ascii.value("colorMode", 0))
                << ", block " << ofToString(ascii.value("blockSize", 0)) << ")";
        }
        if (fx.contains("crt")) {
            const auto& crt = fx["crt"];
            out << " CRT=" << routeName(crt)
                << " (scan " << ofToString(crt.value("scanline", 0.0f), 2)
                << " vig " << ofToString(crt.value("vignette", 0.0f), 2)
                << " bleed " << ofToString(crt.value("bleed", 0.0f), 2) << ")";
        }
    }
    return out.str();
}
}

LayersHudWidget::LayersHudWidget() {
    metadata_.id = "hud.layers";
    metadata_.label = "Console Layers";
    metadata_.category = "HUD";
    metadata_.description = "Console layer roster and effects routing.";
    metadata_.defaultColumn = 0;
    metadata_.defaultHeight = 360.0f;
    metadata_.minHeight = 200.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
}

void LayersHudWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void LayersHudWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void LayersHudWidget::draw(const DrawParams& params) {
    if (!app_) {
        return;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.layers")) {
        text = composeLayersFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudLayers();
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

float LayersHudWidget::preferredHeight(float width) const {
    if (!app_) {
        return metadata_.defaultHeight;
    }
    std::string text;
    if (auto feed = app_->latestHudFeed("hud.layers")) {
        text = composeLayersFromFeed(feed->payload);
    }
    if (text.empty()) {
        text = app_->composeHudLayers();
    }
    text = hudEllipsizeText(text, width, hudSkin_);
    return computeHudTextHeight(text, metadata_.minHeight, hudSkin_);
}

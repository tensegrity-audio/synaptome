#pragma once

#include "OverlayWidget.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

class OverlayManager {
public:
    struct ColumnSpec { std::string id; float width = 360.0f; };
    struct LayoutState {
        std::vector<ColumnSpec> columns;
        struct WidgetState { std::string id; int columnIndex = 0; bool visible = true; bool collapsed = false; std::string bandId; };
        std::vector<WidgetState> widgets;
    };

    OverlayManager() {}
    void setLayoutDirtyCallback(std::function<void()> cb) { layoutDirtyCallback_ = cb; }
    void setHost(void*) {}
    void setColumnSpecs(const std::vector<ColumnSpec>& specs) { columnSpecs_ = specs; }
    void setHudSkin(const HudSkin& hud) { hudSkin_ = hud; }
    void setHudVisible(bool visible) { hudVisible_ = visible; }
    bool hudVisible() const { return hudVisible_; }

    bool registerWidget(std::unique_ptr<OverlayWidget> widget) {
        if (!widget) return false;
        entries_.push_back(std::move(widget));
        return true;
    }
    bool hasWidget(const std::string& id) const {
        for (const auto& w : entries_) {
            if (w->metadata().id == id) return true;
        }
        return false;
    }
    void setWidgetVisible(const std::string& id, bool visible) {
        for (auto& w : entries_) {
            if (w->metadata().id == id) {
                // store visibility map
                vis_[id] = visible;
            }
        }
    }
    void setWidgetBand(const std::string& id, OverlayWidget::Band band) {
        bands_[id] = band;
    }
    OverlayWidget::Band widgetBand(const std::string& id) const {
        auto it = bands_.find(id);
        if (it != bands_.end()) {
            return it->second;
        }
        for (const auto& w : entries_) {
            if (w->metadata().id == id) {
                return w->metadata().band;
            }
        }
        return OverlayWidget::Band::Hud;
    }

    LayoutState captureState() const {
        LayoutState s;
        s.columns = columnSpecs_;
        for (const auto& w : entries_) {
            LayoutState::WidgetState ws;
            ws.id = w->metadata().id;
            auto it = vis_.find(ws.id);
            ws.visible = (it != vis_.end()) ? it->second : true;
            OverlayWidget::Band band = widgetBand(ws.id);
            ws.bandId = overlayBandToString(band);
            s.widgets.push_back(ws);
        }
        return s;
    }

private:
    std::vector<ColumnSpec> columnSpecs_;
    std::vector<std::unique_ptr<OverlayWidget>> entries_;
    std::unordered_map<std::string,bool> vis_;
    std::function<void()> layoutDirtyCallback_;
    HudSkin hudSkin_;
    bool hudVisible_ = true;
    std::unordered_map<std::string, OverlayWidget::Band> bands_;
};

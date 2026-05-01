#pragma once

#include "OverlayWidget.h"
#include "OverlayManager.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

class HudRegistry {
public:
    struct Toggle {
        std::string id;
        std::string label;
        std::string description;
        bool defaultValue = false;
        bool* valuePtr = nullptr;
    };

    struct ViewEntry {
        std::string id; std::string label; std::string description; bool enabled=false; bool dirty=false;
    };

    struct WidgetDescriptor {
        OverlayWidget::Metadata metadata;
        std::function<std::unique_ptr<OverlayWidget>()> factory;
        std::string toggleId;
    };

    struct WidgetInfo {
        OverlayWidget::Metadata metadata;
        std::string toggleId;
        bool registered = false;
        int columnIndex = 0;
        std::string columnId;
        OverlayWidget::Band band = OverlayWidget::Band::Hud;
        bool visible = true;
        bool collapsed = false;
    };

    void setOverlayManager(OverlayManager* m) { overlayManager_ = m; }
    void setLayoutChangedCallback(std::function<void()>) {}
    bool registerToggle(const Toggle& t) {
        if (t.id.empty() || !t.valuePtr) return false;
        toggles_[t.id] = t;
        savedValues_[t.id] = t.defaultValue;
        // initialize pointer
        *(t.valuePtr) = t.defaultValue;
        return true;
    }
    bool registerWidget(WidgetDescriptor wd) {
        widgets_[wd.metadata.id] = wd;
        widgetOrder_.push_back(wd.metadata.id);
        if (overlayManager_ && wd.factory) {
            overlayManager_->registerWidget(wd.factory());
            // set visibility based on toggle default
            bool vis = true;
            auto it = savedValues_.find(wd.toggleId);
            if (it != savedValues_.end()) vis = it->second;
            overlayManager_->setWidgetVisible(wd.metadata.id, vis);
        }
        return true;
    }

    bool setValue(const std::string& id, bool enabled) {
        auto it = savedValues_.find(id);
        if (it == savedValues_.end()) return false;
        it->second = enabled;
        auto tIt = toggles_.find(id);
        if (tIt != toggles_.end() && tIt->second.valuePtr) {
            *(tIt->second.valuePtr) = enabled;
        }
        if (overlayManager_) overlayManager_->setWidgetVisible(id, enabled);
        return true;
    }

    bool setWidgetColumn(const std::string& id, int columnIndex) {
        columnOverrides_[id] = columnIndex;
        return true;
    }

    std::vector<WidgetInfo> widgets() const {
        std::vector<WidgetInfo> out;
        for (auto const& id : widgetOrder_) {
            WidgetInfo w;
            w.metadata = widgets_.at(id).metadata;
            w.toggleId = widgets_.at(id).toggleId;
            w.registered = true;
            w.band = w.metadata.band;
            auto columnIt = columnOverrides_.find(id);
            if (columnIt != columnOverrides_.end()) {
                w.columnIndex = columnIt->second;
            } else {
                w.columnIndex = w.metadata.defaultColumn;
            }
            w.columnId = "Column " + std::to_string(w.columnIndex + 1);
            auto toggleIt = savedValues_.find(w.toggleId);
            w.visible = (toggleIt == savedValues_.end()) ? true : toggleIt->second;
            out.push_back(w);
        }
        return out;
    }

private:
    OverlayManager* overlayManager_ = nullptr;
    std::unordered_map<std::string, Toggle> toggles_;
    std::unordered_map<std::string, bool> savedValues_;
    std::unordered_map<std::string, WidgetDescriptor> widgets_;
    std::vector<std::string> widgetOrder_;
    std::unordered_map<std::string, int> columnOverrides_;
};

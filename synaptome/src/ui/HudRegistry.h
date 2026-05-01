#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

#include "ofMain.h"
#include "overlays/OverlayWidget.h"

class OverlayManager;

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
        std::string id;
        std::string label;
        std::string description;
        bool enabled = false;
        bool dirty = false;
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

    void setOverlayManager(class OverlayManager* manager);
    void setLayoutChangedCallback(std::function<void()> cb);
    void setLayoutStoragePath(std::string path);
    void setHudVisible(bool visible);
    bool hudVisible() const { return hudVisible_; }

    bool registerWidget(WidgetDescriptor widget);
    std::vector<WidgetInfo> widgets() const;

    void setStoragePath(std::string path);

    bool loadFromDisk();
    bool saveToDisk();
    bool saveIfDirty();

    bool loadLayoutFromDisk();
    bool saveLayoutToDisk();
    bool saveLayoutIfDirty();

    bool registerToggle(const Toggle& toggle);
    bool toggle(const std::string& id);
    bool setValue(const std::string& id, bool enabled);
    bool resetToDefault(const std::string& id);
    bool setWidgetColumn(const std::string& id, int columnIndex);

    bool isEnabled(const std::string& id) const;

    std::vector<ViewEntry> entries() const;

    bool isDirty() const;
    bool entryDirty(const std::string& id) const;
    bool isLayoutDirty() const;

private:
    struct InternalToggle {
        std::string id;
        std::string label;
        std::string description;
        bool defaultValue = false;
        bool* valuePtr = nullptr;
    };

    std::string storagePath_;
    std::unordered_map<std::string, InternalToggle> toggles_;
    std::vector<std::string> order_;
    std::unordered_map<std::string, bool> savedValues_;

    std::string layoutStoragePath_;
    bool layoutDirty_ = false;
    bool layoutTrackingActive_ = false;
    bool layoutLoaded_ = false;

    struct WidgetEntry {
        OverlayWidget::Metadata metadata;
        std::function<std::unique_ptr<OverlayWidget>()> factory;
        std::string toggleId;
        bool registered = false;
    };
    std::unordered_map<std::string, WidgetEntry> widgets_;
    std::vector<std::string> widgetOrder_;
    std::unordered_map<std::string, std::vector<std::string>> toggleToWidgets_;
    class OverlayManager* overlayManager_ = nullptr;
    bool hudVisible_ = true;
    std::function<void()> layoutChangedCallback_;

    bool readValue(const InternalToggle& toggle) const;
    void writeValue(const InternalToggle& toggle, bool value) const;
    void registerWidgetWithOverlay(const std::string& id, WidgetEntry& entry);
    void applyToggleToWidgets(const std::string& toggleId, bool value);
    void markLayoutDirty();
};

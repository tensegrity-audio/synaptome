#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <array>

#include "OverlayWidget.h"
#include "../MenuSkin.h"
#include "../ThreeBandLayout.h"

class OverlayManager {
public:
    struct ColumnSpec {
        std::string id;
        float width = 360.0f;
    };

    struct UpdateParams {
        float deltaTime = 0.0f;
        class ofApp* app = nullptr;
    };

    struct DrawParams {
        ofRectangle bounds;
        class ofApp* app = nullptr;
        ThreeBandLayout layout;
        bool useThreeBandLayout = false;
    };

    struct LayoutState {
        std::vector<ColumnSpec> columns;
        struct WidgetState {
            std::string id;
            int columnIndex = 0;
            bool visible = true;
            bool collapsed = false;
            std::string bandId;
        };
        std::vector<WidgetState> widgets;
    };

    OverlayManager();

    void setLayoutDirtyCallback(std::function<void()> callback);

    void setHost(class ofApp* app);
    void setColumnSpecs(std::vector<ColumnSpec> specs);
    void setHudSkin(const HudSkin& hudSkin);
    const HudSkin& hudSkin() const { return hudSkin_; }
    const std::vector<ColumnSpec>& columnSpecs() const { return columnSpecs_; }
    void setHudVisible(bool visible);
    bool hudVisible() const { return hudVisible_; }

    bool registerWidget(std::unique_ptr<OverlayWidget> widget);
    bool hasWidget(const std::string& id) const;
    void setWidgetVisible(const std::string& id, bool visible);
    void setWidgetCollapsed(const std::string& id, bool collapsed);
    void setWidgetColumn(const std::string& id, int columnIndex);
    void setWidgetBand(const std::string& id, OverlayWidget::Band band);
    OverlayWidget::Band widgetBand(const std::string& id) const;
    int widgetColumn(const std::string& id) const;
    bool widgetCollapsed(const std::string& id) const;

    LayoutState captureState() const;
    bool applyState(const LayoutState& state, bool notify = true);

    void update(const UpdateParams& params);
    void draw(const DrawParams& params);

private:
    struct WidgetEntry {
        std::unique_ptr<OverlayWidget> widget;
        OverlayWidget::Metadata metadata;
        int columnIndex = 0;
        bool visible = true;
        bool collapsed = false;
        bool setupComplete = false;
    };

    class ofApp* hostApp_ = nullptr;
    std::vector<ColumnSpec> columnSpecs_;
    std::vector<WidgetEntry> entries_;
    HudSkin hudSkin_;
    std::function<void()> layoutDirtyCallback_;
    bool suppressLayoutCallbacks_ = false;
    bool hudVisible_ = true;

    WidgetEntry* findEntry(const std::string& id);
    const WidgetEntry* findEntry(const std::string& id) const;
    void ensureDefaultColumns();
    void notifyLayoutChanged();
};

#pragma once

#include "MenuController.h"
#include "HudRegistry.h"
#include "overlays/OverlayManager.h"

#include <array>
#include <string>
#include <vector>

class HudLayoutEditor : public MenuController::State {
public:
    HudLayoutEditor(HudRegistry* registry, OverlayManager* overlayManager);

    const std::string& id() const override { return stateId_; }
    const std::string& label() const override { return label_; }
    const std::string& scope() const override { return scope_; }

    MenuController::StateView view() const override;
    bool handleInput(MenuController& controller, int key) override;
    void onEnter(MenuController& controller) override;
    void onExit(MenuController& controller) override;
    void draw() const;

private:
    struct WidgetItem {
        std::string id;
        std::string label;
        std::string columnId;
        int columnIndex = 0;
        bool visible = true;
        bool collapsed = false;
        std::string toggleId;
        bool registered = false;
        OverlayWidget::Band band = OverlayWidget::Band::Hud;
    };

    struct Preset {
        OverlayManager::LayoutState state;
        bool valid = false;
    };

    HudRegistry* registry_ = nullptr;
    OverlayManager* overlay_ = nullptr;
    MenuController* controller_ = nullptr;

    mutable MenuController::StateView cachedView_;
    mutable std::vector<WidgetItem> items_;
    mutable int selectedIndex_ = 0;
    mutable std::string selectedId_;
    bool active_ = false;

    std::array<Preset, 3> presets_{};

    const std::string stateId_ = "ui.hud.layout";
    const std::string label_ = "HUD Layout Editor";
    const std::string scope_ = "HUD";

    void rebuildView() const;
    void rebuildItems() const;
    void clampSelection() const;
    void moveSelection(int delta);
    void cycleBand(int delta);
    WidgetItem* currentItem();
    const WidgetItem* currentItem() const;
    std::string columnLabel(int index) const;

    void toggleVisibility();
    void toggleCollapsed();
    void moveColumn(int delta);
    void storePreset(int slot);
    void recallPreset(int slot);
    void refreshSelectionId() const;
    bool shiftDown() const;
};

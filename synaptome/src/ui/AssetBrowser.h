#pragma once

#include "ofMain.h"
#include "MenuController.h"
#include "MenuSkin.h"
#include "../visuals/LayerLibrary.h"
#include <functional>
#include <set>
#include <string>
#include <vector>

class AssetBrowser : public MenuController::State {
public:
    void setLibrary(const LayerLibrary* library);
    void setPresenceQuery(std::function<bool(const std::string& id)> query);
    void setActiveQuery(std::function<bool(const std::string& id)> query);
    void setCommandHandler(std::function<void(const LayerLibrary::Entry& entry, int key)> handler);
    void setAllowEntryPredicate(std::function<bool(const LayerLibrary::Entry&)> predicate);
    void setMenuSkin(const MenuSkin& skin) { skin_ = skin; }

    bool isActive() const { return active_; }

    void draw() const;

    const std::string& id() const override { return stateId; }
    const std::string& label() const override { return stateLabel; }
    const std::string& scope() const override { return scopeId; }
    MenuController::StateView view() const override;
    bool handleInput(MenuController& controller, int key) override;
    void onEnter(MenuController& controller) override;
    void onExit(MenuController& controller) override;

private:
    struct Row {
        enum Kind {
            Category,
            Group,
            Asset
        };
        Kind kind = Asset;
        std::string key;
        std::string label;
        std::string description;
        int depth = 0;
        bool expandable = false;
        bool expanded = false;
        const LayerLibrary::Entry* entry = nullptr;
    };

    const LayerLibrary* library_ = nullptr;
    std::function<bool(const std::string&)> presenceQuery_;
    std::function<bool(const std::string&)> activeQuery_;
    std::function<void(const LayerLibrary::Entry&, int)> commandHandler_;
    std::function<bool(const LayerLibrary::Entry&)> allowEntryPredicate_;

    MenuController* controller_ = nullptr;
    bool active_ = false;
    int selected_ = 0;
    std::set<std::string> collapsedKeys_;
    MenuSkin skin_ = MenuSkin::ConsoleHub();

    const std::string stateId = "ui.assets";
    const std::string stateLabel = "Asset Browser";
    const std::string scopeId = "Home";

    void clampSelection();
    const LayerLibrary::Entry* currentEntry() const;
    std::vector<std::reference_wrapper<const LayerLibrary::Entry>> visibleEntries() const;
    std::vector<Row> visibleRows() const;
    bool isExpanded(const std::string& key) const;
    void setExpanded(const std::string& key, bool expanded);
    void notifyViewModel();
};

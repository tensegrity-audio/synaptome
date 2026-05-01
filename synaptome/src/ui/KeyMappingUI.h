#pragma once

#include "MenuController.h"

#include <string>
#include <vector>

class HotkeyManager;

class KeyMappingUI : public MenuController::State {
public:
    explicit KeyMappingUI(HotkeyManager* manager);

    void draw() const;

    const std::string& id() const override { return stateId_; }
    const std::string& label() const override { return label_; }
    const std::string& scope() const override { return scope_; }
    MenuController::StateView view() const override;
    bool handleInput(MenuController& controller, int key) override;
    void onEnter(MenuController& controller) override;
    void onExit(MenuController& controller) override;
    bool capturesInputBeforeHotkeys() const override { return true; }
    bool wantsRawKeyInput() const override { return learning_; }

private:
    struct RowInfo {
        bool header = false;
        std::string scope;
        std::string bindingId;
    };

    HotkeyManager* manager_ = nullptr;
    MenuController* controller_ = nullptr;
    mutable std::vector<RowInfo> rows_;
    mutable MenuController::StateView cachedView_;
    mutable int selectedIndex_ = 0;
    bool active_ = false;
    bool learning_ = false;
    std::string learningBindingId_;
    // transient UI toast for success/errors
    mutable std::string toastMessage_;
    mutable uint64_t toastExpiryMs_ = 0;

    void showToast(const std::string& msg, uint64_t durationMs = 2000) const;

    const std::string stateId_ = "ui.keymap";
    const std::string label_ = "Key Mapping";
    const std::string scope_ = "Key Map";

    void rebuildView() const;
    void clampSelection() const;
    void moveSelection(int delta);
    void beginLearning();
    void cancelLearning();
    void finishLearning(int key);
    bool saveBindings();
    const RowInfo* currentRow() const;
    static std::string conflictSummary(const std::vector<std::string>& conflicts);
};

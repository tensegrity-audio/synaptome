#include "KeyMappingUI.h"

#include "HotkeyManager.h"

#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMain.h"
#include "ofUtils.h"

#include <algorithm>
#include <sstream>

namespace {
    std::string scopeLabel(const std::string& scope) {
        return scope.empty() ? std::string("Global") : scope;
    }
}

KeyMappingUI::KeyMappingUI(HotkeyManager* manager)
    : manager_(manager) {
}

void KeyMappingUI::draw() const {
    if (!active_ || !manager_) {
        return;
    }

    rebuildView();

    float y = 40.0f;
    std::string header = learning_ ? "Key Mapping � learning (press a key or Esc to cancel)"
                                   : "Key Mapping � Enter: learn   S: save   R: reset   Esc: close";
    ofDrawBitmapStringHighlight(header, 20.0f, y);
    y += 22.0f;

    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const auto& row = rows_[i];
        std::string line;
        if (row.header) {
            line = "[" + scopeLabel(row.scope) + "]";
        } else {
            const auto* binding = manager_->findBinding(row.bindingId);
            if (!binding) {
                continue;
            }
            std::string keyText = HotkeyManager::keyLabel(binding->currentKey);
            line = (static_cast<int>(i) == selectedIndex_ ? "> " : "  ");
            line += binding->displayName + "  [" + keyText + "]";

            if (learning_ && binding->id == learningBindingId_) {
                line += "  <-- waiting for key";
            } else {
                auto conflicts = manager_->bindingConflicts(binding->id);
                if (!conflicts.empty()) {
                    line += "  !! conflicts: " + conflictSummary(conflicts);
                } else if (manager_->bindingDirty(binding->id)) {
                    line += "  * unsaved";
                }
                if (!binding->description.empty()) {
                    line += "  - " + binding->description;
                }
            }
        }
        ofDrawBitmapStringHighlight(line, 20.0f, y);
        y += 18.0f;
    }

    // Draw transient toast if active
    uint64_t now = ofGetElapsedTimeMillis();
    if (!toastMessage_.empty() && toastExpiryMs_ > now) {
        float tx = ofGetWidth() - 20.0f;
        float ty = ofGetHeight() - 30.0f;
        ofDrawBitmapStringHighlight(toastMessage_, tx - 300.0f, ty);
    }
}

MenuController::StateView KeyMappingUI::view() const {
    rebuildView();
    return cachedView_;
}

bool KeyMappingUI::handleInput(MenuController& controller, int key) {
    if (!manager_) {
        return false;
    }

    const int baseKey = key & 0xFFFF;

    if (learning_) {
        if (baseKey == OF_KEY_ESC) {
            cancelLearning();
            controller.requestViewModelRefresh();
            return true;
        }
        auto isModifierOnly = [&](int base) {
            if (base == OF_KEY_CONTROL || base == OF_KEY_SHIFT || base == OF_KEY_ALT) {
                return true;
            }
            if (base == 0) {
                return (key & MenuController::HOTKEY_MOD_MASK) != 0;
            }
            return false;
        };
        if (isModifierOnly(baseKey)) {
            return true;
        }
        finishLearning(key);
        controller.requestViewModelRefresh();
        return true;
    }

    switch (baseKey) {
    case OF_KEY_UP:
        moveSelection(-1);
        controller.requestViewModelRefresh();
        return true;
    case OF_KEY_DOWN:
        moveSelection(1);
        controller.requestViewModelRefresh();
        return true;
    case OF_KEY_RETURN:
        beginLearning();
        controller.requestViewModelRefresh();
        return true;
    case 's':
    case 'S':
        if (saveBindings()) {
            controller.requestViewModelRefresh();
        }
        return true;
    case 'u':
    case 'U':
        // undo unsaved changes by reloading from disk
        if (manager_ && manager_->loadFromDisk()) {
            showToast("Reverted unsaved changes", 1500);
            controller.requestViewModelRefresh();
        } else {
            showToast("Failed to revert bindings from disk", 3000);
        }
        return true;
    case 'r':
    case 'R': {
        const auto* row = currentRow();
        if (row && manager_->resetBindingToDefault(row->bindingId)) {
            controller.requestViewModelRefresh();
        }
        return true;
    }
    case OF_KEY_ESC:
        controller.popState();
        return true;
    default:
        break;
    }

    return false;
}

void KeyMappingUI::onEnter(MenuController& controller) {
    controller_ = &controller;
    active_ = true;
    learning_ = false;
    controller.requestViewModelRefresh();
}

void KeyMappingUI::onExit(MenuController& controller) {
    (void)controller;
    controller_ = nullptr;
    active_ = false;
    learning_ = false;
    learningBindingId_.clear();
}

void KeyMappingUI::rebuildView() const {
    cachedView_ = MenuController::StateView{};
    rows_.clear();

    if (!manager_) {
        cachedView_.selectedIndex = -1;
        return;
    }

    auto ordered = manager_->orderedBindings();
    std::string currentScope;
    for (const auto* binding : ordered) {
        if (!binding) continue;
        if (currentScope != binding->scope) {
            currentScope = binding->scope;
            RowInfo header;
            header.header = true;
            header.scope = binding->scope;
            rows_.push_back(header);

            MenuController::EntryView headerEntry;
            headerEntry.id = "scope:" + binding->scope;
            headerEntry.label = scopeLabel(binding->scope);
            headerEntry.selectable = false;
            cachedView_.entries.push_back(std::move(headerEntry));
        }

        RowInfo row;
        row.header = false;
        row.scope = binding->scope;
        row.bindingId = binding->id;
        rows_.push_back(row);

        MenuController::EntryView entry;
        entry.id = binding->id;
        entry.label = binding->displayName;
        entry.selectable = binding->learnable;
        cachedView_.entries.push_back(std::move(entry));
    }

    clampSelection();

    cachedView_.selectedIndex = -1;
    for (std::size_t i = 0; i < cachedView_.entries.size(); ++i) {
        cachedView_.entries[i].selected = false;
    }
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(cachedView_.entries.size())) {
        cachedView_.selectedIndex = selectedIndex_;
        cachedView_.entries[static_cast<std::size_t>(selectedIndex_)].selected = true;
    }

    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].header) {
            cachedView_.entries[i].description.clear();
            continue;
        }
        const auto* binding = manager_->findBinding(rows_[i].bindingId);
        if (!binding) continue;
        auto& entry = cachedView_.entries[i];
        std::string base = "Key: " + HotkeyManager::keyLabel(binding->currentKey);
        if (!binding->description.empty()) {
            base += "  � " + binding->description;
        }
        if (learning_ && binding->id == learningBindingId_) {
            entry.description = "Press new key   (Esc cancels)";
            entry.pendingChanges = true;
        } else {
            auto conflicts = manager_->bindingConflicts(binding->id);
            if (!conflicts.empty()) {
                entry.description = base + "  ? " + conflictSummary(conflicts);
                entry.pendingChanges = true;
            } else {
                entry.description = base;
                entry.pendingChanges = manager_->bindingDirty(binding->id);
            }
        }
    }

    cachedView_.hotkeys.clear();
    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_UP, "Up", "Previous binding"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_DOWN, "Down", "Next binding"});
    if (learning_) {
        cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_ESC, "Esc", "Cancel learn"});
    } else {
        cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_RETURN, "Enter", "Learn binding"});
        cachedView_.hotkeys.push_back(MenuController::KeyHint{'S', "S", "Save bindings"});
        cachedView_.hotkeys.push_back(MenuController::KeyHint{'R', "R", "Reset to default"});
        cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_ESC, "Esc", "Close"});
    }
}

void KeyMappingUI::clampSelection() const {
    if (rows_.empty()) {
        selectedIndex_ = -1;
        return;
    }
    if (selectedIndex_ < 0) {
        selectedIndex_ = 0;
    }
    int maxIndex = static_cast<int>(rows_.size()) - 1;
    if (selectedIndex_ > maxIndex) {
        selectedIndex_ = maxIndex;
    }
    if (selectedIndex_ < 0) {
        return;
    }
    if (rows_[selectedIndex_].header) {
        int forward = selectedIndex_;
        while (forward < static_cast<int>(rows_.size()) && rows_[forward].header) {
            ++forward;
        }
        if (forward < static_cast<int>(rows_.size())) {
            selectedIndex_ = forward;
        } else {
            int backward = selectedIndex_;
            while (backward >= 0 && rows_[backward].header) {
                --backward;
            }
            selectedIndex_ = backward;
        }
    }
}

void KeyMappingUI::moveSelection(int delta) {
    if (rows_.empty()) {
        selectedIndex_ = -1;
        return;
    }
    int count = static_cast<int>(rows_.size());
    int idx = selectedIndex_;
    for (int i = 0; i < count; ++i) {
        idx = (idx + delta + count) % count;
        if (!rows_[idx].header) {
            selectedIndex_ = idx;
            return;
        }
    }
}

void KeyMappingUI::beginLearning() {
    const auto* row = currentRow();
    if (!row) {
        return;
    }
    const auto* binding = manager_->findBinding(row->bindingId);
    if (!binding || !binding->learnable) {
        return;
    }
    learning_ = true;
    learningBindingId_ = binding->id;
}

void KeyMappingUI::cancelLearning() {
    learning_ = false;
    learningBindingId_.clear();
}

void KeyMappingUI::finishLearning(int key) {
    if (!learning_ || learningBindingId_.empty()) {
        return;
    }
    if (manager_->setBindingKey(learningBindingId_, key)) {
        ofLogNotice("KeyMappingUI") << "Updated hotkey '" << learningBindingId_ << "' to key " << HotkeyManager::keyLabel(key);
    }
    learning_ = false;
    learningBindingId_.clear();
}

bool KeyMappingUI::saveBindings() {
    if (!manager_) {
        return false;
    }
    if (!manager_->isDirty()) {
        return true;
    }
    bool ok = manager_->saveToDisk();
    if (!ok) {
        ofLogWarning("KeyMappingUI") << "Failed to save hotkey bindings";
        showToast("Failed to save hotkey bindings", 3000);
        return false;
    }
    showToast("Hotkeys saved", 1200);
    return true;
}

void KeyMappingUI::showToast(const std::string& msg, uint64_t durationMs) const {
    toastMessage_ = msg;
    toastExpiryMs_ = ofGetElapsedTimeMillis() + durationMs;
}

const KeyMappingUI::RowInfo* KeyMappingUI::currentRow() const {
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(rows_.size())) {
        return nullptr;
    }
    if (rows_[selectedIndex_].header) {
        return nullptr;
    }
    return &rows_[selectedIndex_];
}

std::string KeyMappingUI::conflictSummary(const std::vector<std::string>& conflicts) {
    if (conflicts.empty()) {
        return std::string();
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < conflicts.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << conflicts[i];
    }
    return oss.str();
}

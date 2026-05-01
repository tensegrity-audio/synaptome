#include "HudLayoutEditor.h"

#include "ofLog.h"
#include "ofGraphics.h"
#include "ofUtils.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <sstream>

namespace {
    std::string formatColumnSummary(const std::vector<OverlayManager::ColumnSpec>& columns) {
        if (columns.empty()) {
            return "No columns configured";
        }
        std::ostringstream out;
        for (std::size_t i = 0; i < columns.size(); ++i) {
            const auto& column = columns[i];
            if (i > 0) {
                out << "  |  ";
            }
            std::string name = column.id.empty() ? ("Column " + std::to_string(i + 1)) : column.id;
            out << name << " (" << static_cast<int>(column.width) << ")";
        }
        return out.str();
    }
}

HudLayoutEditor::HudLayoutEditor(HudRegistry* registry, OverlayManager* overlayManager)
    : registry_(registry)
    , overlay_(overlayManager) {
}

MenuController::StateView HudLayoutEditor::view() const {
    rebuildView();
    return cachedView_;
}
void HudLayoutEditor::draw() const {
    if (!active_ || !controller_ || !controller_->isCurrent(id())) {
        return;
    }

    rebuildView();

    ofPushStyle();
    float y = 40.0f;
    std::string header = "HUD Layout Editor - Space: visibility   C: collapse   [ / ]: move column   1-3: presets (Shift stores)   S: save   R: reload   Ctrl+H: toggle HUD";
    ofDrawBitmapStringHighlight(header, 20.0f, y);
    y += 22.0f;

    for (std::size_t i = 0; i < cachedView_.entries.size(); ++i) {
        const auto& entry = cachedView_.entries[i];
        bool selected = (static_cast<int>(i) == cachedView_.selectedIndex);
        std::string line = selected ? "> " : "  ";
        line += entry.label;
        if (!entry.description.empty()) {
            line += "  - " + entry.description;
        }
        if (entry.pendingChanges) {
            line += "  * unsaved";
        }

        if (!entry.selectable) {
            ofSetColor(190, 220, 255);
        } else if (selected) {
            ofSetColor(255, 255, 0);
        } else {
            ofSetColor(255);
        }
        ofDrawBitmapStringHighlight(line, 20.0f, y);
        y += 18.0f;
    }

    if (!cachedView_.hotkeys.empty()) {
        y += 10.0f;
        std::ostringstream hint;
        hint << "Keys: ";
        for (std::size_t i = 0; i < cachedView_.hotkeys.size(); ++i) {
            const auto& hotkey = cachedView_.hotkeys[i];
            if (i > 0) {
                hint << "   ";
            }
            hint << "[" << hotkey.label << "] " << hotkey.description;
        }
        ofSetColor(180, 255, 180);
        ofDrawBitmapStringHighlight(hint.str(), 20.0f, y);
    }
    ofPopStyle();
}


bool HudLayoutEditor::handleInput(MenuController& controller, int key) {
    if (!active_) {
        return false;
    }
    switch (key) {
    case OF_KEY_UP:
        moveSelection(-1);
        controller.requestViewModelRefresh();
        return true;
    case OF_KEY_DOWN:
        moveSelection(1);
        controller.requestViewModelRefresh();
        return true;
    case ' ':
        toggleVisibility();
        controller.requestViewModelRefresh();
        return true;
    case 'c':
    case 'C':
        toggleCollapsed();
        controller.requestViewModelRefresh();
        return true;
    case 'b':
        cycleBand(1);
        controller.requestViewModelRefresh();
        return true;
    case 'B':
        cycleBand(-1);
        controller.requestViewModelRefresh();
        return true;
    case '[':
        moveColumn(-1);
        controller.requestViewModelRefresh();
        return true;
    case ']':
        moveColumn(1);
        controller.requestViewModelRefresh();
        return true;
    case 's':
    case 'S': {
        if (registry_) {
            if (registry_->saveLayoutToDisk()) {
                ofLogNotice("HudLayoutEditor") << "Layout saved to disk";
            } else {
                ofLogWarning("HudLayoutEditor") << "Failed to save layout";
            }
            controller.requestViewModelRefresh();
        }
        return true;
    }
    case 'r':
    case 'R': {
        if (registry_) {
            if (registry_->loadLayoutFromDisk()) {
                ofLogNotice("HudLayoutEditor") << "Layout reloaded";
            } else {
                ofLogWarning("HudLayoutEditor") << "Layout reload failed";
            }
            controller.requestViewModelRefresh();
        }
        return true;
    }
    case '1':
    case '2':
    case '3': {
        int slot = key - '1';
        if (slot >= 0 && slot < 3) {
            if (shiftDown()) {
                storePreset(slot);
            } else {
                recallPreset(slot);
            }
            controller.requestViewModelRefresh();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void HudLayoutEditor::onEnter(MenuController& controller) {
    controller_ = &controller;
    active_ = true;
    clampSelection();
    if (!items_.empty() && selectedId_.empty()) {
        selectedId_ = items_.front().id;
    }
    controller.requestViewModelRefresh();
}

void HudLayoutEditor::onExit(MenuController& controller) {
    (void)controller;
    controller_ = nullptr;
    active_ = false;
}

void HudLayoutEditor::rebuildView() const {
    cachedView_ = MenuController::StateView{};
    rebuildItems();

    MenuController::EntryView columnsEntry;
    columnsEntry.id = "layout.columns";
    columnsEntry.label = "Columns";
    columnsEntry.description = formatColumnSummary(overlay_ ? overlay_->columnSpecs() : std::vector<OverlayManager::ColumnSpec>{});
    columnsEntry.selectable = false;
    columnsEntry.pendingChanges = registry_ ? registry_->isLayoutDirty() : false;
    cachedView_.entries.push_back(std::move(columnsEntry));

    if (!presets_.empty()) {
        MenuController::EntryView presetEntry;
        presetEntry.id = "layout.presets";
        presetEntry.label = "Quick Presets";
        presetEntry.description = "1-3 recall (Shift+number stores)";
        presetEntry.selectable = false;
        cachedView_.entries.push_back(std::move(presetEntry));
    }

    int widgetOffset = static_cast<int>(cachedView_.entries.size());
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        MenuController::EntryView entry;
        entry.id = item.id;
        entry.label = item.label.empty() ? item.id : item.label;
        std::ostringstream desc;
        desc << "Column: " << columnLabel(item.columnIndex)
             << "  |  Band: " << overlayBandToString(item.band)
             << "  |  Visible: " << (item.visible ? "yes" : "no")
             << "  |  Collapsed: " << (item.collapsed ? "yes" : "no");
        if (!item.toggleId.empty()) {
            desc << "  |  Toggle: " << item.toggleId;
        }
        entry.description = desc.str();
        entry.selectable = true;
        entry.selected = (static_cast<int>(i) == selectedIndex_);
        entry.pendingChanges = registry_ ? registry_->isLayoutDirty() : false;
        cachedView_.entries.push_back(std::move(entry));
    }

    cachedView_.selectedIndex = items_.empty() ? -1 : widgetOffset + selectedIndex_;

    cachedView_.hotkeys.clear();
    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_UP, "Up", "Previous widget"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{OF_KEY_DOWN, "Down", "Next widget"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{' ', "Space", "Toggle visibility"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'C', "C", "Collapse/expand"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'[', "[", "Move to previous column"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{']', "]", "Move to next column"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'S', "S", "Save layout"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'R', "R", "Reload layout"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'1', "1", "Preset 1 (Shift stores)"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'2', "2", "Preset 2 (Shift stores)"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'3', "3", "Preset 3 (Shift stores)"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{'B', "B", "Cycle widget band (Shift reverses)"});
    cachedView_.hotkeys.push_back(MenuController::KeyHint{MenuController::HOTKEY_MOD_CTRL | 'h', "Ctrl+H", "Toggle HUD + tools"});
}

void HudLayoutEditor::rebuildItems() const {
    items_.clear();
    if (!overlay_) {
        return;
    }

    auto state = overlay_->captureState();
    std::vector<OverlayManager::ColumnSpec> columns = overlay_->columnSpecs();

    std::unordered_map<std::string, OverlayManager::LayoutState::WidgetState> widgetState;
    widgetState.reserve(state.widgets.size());
    for (const auto& widget : state.widgets) {
        widgetState[widget.id] = widget;
    }

    if (registry_) {
        for (const auto& info : registry_->widgets()) {
            WidgetItem item;
            item.id = info.metadata.id;
            item.label = info.metadata.label.empty() ? info.metadata.id : info.metadata.label;
            item.toggleId = info.toggleId;
            item.registered = info.registered;
            item.band = info.metadata.band;
            auto it = widgetState.find(item.id);
            if (it != widgetState.end()) {
                item.columnIndex = it->second.columnIndex;
                item.visible = it->second.visible;
                item.collapsed = it->second.collapsed;
                if (!it->second.bandId.empty()) {
                    item.band = overlayBandFromString(it->second.bandId, item.band);
                }
            } else {
                item.columnIndex = info.metadata.defaultColumn;
                item.visible = true;
                item.collapsed = false;
            }
            if (item.columnIndex < 0) {
                item.columnIndex = 0;
            }
            if (!columns.empty()) {
                int last = static_cast<int>(columns.size()) - 1;
                if (item.columnIndex > last) {
                    item.columnIndex = last;
                }
                item.columnId = columns[item.columnIndex].id;
            }
            if (item.columnId.empty()) {
                item.columnId = "Column " + std::to_string(item.columnIndex + 1);
            }
            items_.push_back(std::move(item));
        }
    }

    // Fallback for widgets unknown to registry
    for (const auto& widget : state.widgets) {
        auto it = std::find_if(items_.begin(), items_.end(), [&](const WidgetItem& item) {
            return item.id == widget.id;
        });
        if (it != items_.end()) {
            continue;
        }
        WidgetItem item;
        item.id = widget.id;
        item.label = widget.id;
        item.columnIndex = widget.columnIndex;
        item.band = overlayBandFromString(widget.bandId, OverlayWidget::Band::Hud);
        if (!columns.empty()) {
            int last = static_cast<int>(columns.size()) - 1;
            if (item.columnIndex < 0) item.columnIndex = 0;
            if (item.columnIndex > last) item.columnIndex = last;
            item.columnId = columns[item.columnIndex].id;
        }
        if (item.columnId.empty()) {
            item.columnId = "Column " + std::to_string(item.columnIndex + 1);
        }
        item.visible = widget.visible;
        item.collapsed = widget.collapsed;
        item.registered = overlay_->hasWidget(widget.id);
        items_.push_back(std::move(item));
    }

    clampSelection();
    refreshSelectionId();
}

void HudLayoutEditor::clampSelection() const {
    if (items_.empty()) {
        selectedIndex_ = -1;
        selectedId_.clear();
        return;
    }
    int maxIndex = static_cast<int>(items_.size()) - 1;
    if (selectedIndex_ < 0) {
        selectedIndex_ = 0;
    }
    if (selectedIndex_ > maxIndex) {
        selectedIndex_ = maxIndex;
    }
    if (selectedId_.empty() && selectedIndex_ >= 0) {
        selectedId_ = items_[selectedIndex_].id;
    }
}

void HudLayoutEditor::moveSelection(int delta) {
    if (items_.empty()) {
        selectedIndex_ = -1;
        selectedId_.clear();
        return;
    }
    int count = static_cast<int>(items_.size());
    selectedIndex_ = (selectedIndex_ + delta + count) % count;
    selectedId_ = items_[selectedIndex_].id;
}

void HudLayoutEditor::cycleBand(int delta) {
    if (delta == 0) {
        return;
    }
    auto* item = currentItem();
    if (!item || !overlay_) {
        return;
    }
    if (!overlay_->hasWidget(item->id)) {
        return;
    }
    constexpr int kBandCount = 3;
    int current = static_cast<int>(item->band);
    current = (current + delta + kBandCount * 4) % kBandCount;
    OverlayWidget::Band newBand = static_cast<OverlayWidget::Band>(current);
    if (newBand == item->band) {
        return;
    }
    overlay_->setWidgetBand(item->id, newBand);
    item->band = newBand;
}

HudLayoutEditor::WidgetItem* HudLayoutEditor::currentItem() {
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(items_.size())) {
        return nullptr;
    }
    return &items_[selectedIndex_];
}

const HudLayoutEditor::WidgetItem* HudLayoutEditor::currentItem() const {
    if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(items_.size())) {
        return nullptr;
    }
    return &items_[selectedIndex_];
}

std::string HudLayoutEditor::columnLabel(int index) const {
    if (!overlay_) {
        return "Column " + std::to_string(index + 1);
    }
    const auto& columns = overlay_->columnSpecs();
    if (index >= 0 && index < static_cast<int>(columns.size())) {
        if (!columns[index].id.empty()) {
            return columns[index].id;
        }
    }
    return "Column " + std::to_string(index + 1);
}

void HudLayoutEditor::toggleVisibility() {
    auto* item = currentItem();
    if (!item) {
        return;
    }
    bool newVisibility = !item->visible;
    bool handled = false;
    if (registry_ && !item->toggleId.empty()) {
        handled = registry_->setValue(item->toggleId, newVisibility);
    }
    if (!handled && overlay_) {
        overlay_->setWidgetVisible(item->id, newVisibility);
    }
}

void HudLayoutEditor::toggleCollapsed() {
    auto* item = currentItem();
    if (!overlay_ || !item) {
        return;
    }
    overlay_->setWidgetCollapsed(item->id, !item->collapsed);
}

void HudLayoutEditor::moveColumn(int delta) {
    auto* item = currentItem();
    if (!overlay_ || !item) {
        return;
    }
    overlay_->setWidgetColumn(item->id, item->columnIndex + delta);
}

void HudLayoutEditor::storePreset(int slot) {
    if (!overlay_) {
        return;
    }
    presets_[slot].state = overlay_->captureState();
    presets_[slot].valid = true;
    ofLogNotice("HudLayoutEditor") << "Stored preset " << (slot + 1);
}

void HudLayoutEditor::recallPreset(int slot) {
    if (!overlay_ || !presets_[slot].valid) {
        ofLogWarning("HudLayoutEditor") << "Preset " << (slot + 1) << " not set";
        return;
    }
    overlay_->applyState(presets_[slot].state);
    ofLogNotice("HudLayoutEditor") << "Recalled preset " << (slot + 1);
}

void HudLayoutEditor::refreshSelectionId() const {
    if (items_.empty()) {
        selectedIndex_ = -1;
        selectedId_.clear();
        return;
    }
    if (!selectedId_.empty()) {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            if (items_[i].id == selectedId_) {
                selectedIndex_ = static_cast<int>(i);
                return;
            }
        }
    }
    selectedIndex_ = 0;
    selectedId_ = items_.front().id;
}

bool HudLayoutEditor::shiftDown() const {
    return ofGetKeyPressed(OF_KEY_SHIFT) != 0;
}

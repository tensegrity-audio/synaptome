#pragma once

#include "ofMain.h"
#include "MenuController.h"
#include "MenuSkin.h"
#include "io/ConsoleStore.h"
#include "../core/ParameterRegistry.h"
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <cmath>

class ConsoleState : public MenuController::State {
public:
    ConsoleState();

    struct ParameterPreview {
        std::string parameterId;
        std::string label;
        std::string detail;
        bool valid = false;
    };

    const std::string& id() const override { return stateId; }
    const std::string& label() const override { return stateLabel; }
    const std::string& scope() const override { return scopeId; }

    MenuController::StateView view() const override;
    bool handleInput(MenuController& controller, int key) override;
    void onEnter(MenuController& controller) override;
    void onExit(MenuController& controller) override;

    // Receive raw key input so we can detect Ctrl+number combos
    bool wantsRawKeyInput() const override { return true; }

    // Callback used by the host (`ofApp`) to open an asset browser for a
    // specific console layer index (1..8). ConsoleState will call this when
    // the operator presses Enter or Ctrl+1..Ctrl+8.
    void setRequestAssetBrowserCallback(std::function<void(int)> cb);
    void setRequestClearLayerCallback(std::function<void(int)> cb);
    void setQueryLayerCallback(std::function<ConsoleLayerInfo(int)> cb);
    void setQueryParameterPreviewCallback(std::function<ParameterPreview(int,int)> cb);
    void setParameterRegistry(ParameterRegistry* registry);
    void setLayoutBand(const ofRectangle& bounds);
    void clearLayoutBand();
    void setMenuSkin(const MenuSkin& skin);

    void draw() const;

private:
    mutable int selectedColumn_ = 0;
    bool active_ = false;
    std::function<void(int)> requestAssetBrowserCallback_;
    std::function<void(int)> requestClearLayerCallback_;
    std::function<ConsoleLayerInfo(int)> queryLayerCallback_;
    std::function<ParameterPreview(int,int)> queryParameterPreviewCallback_;
    ParameterRegistry* registry_ = nullptr;
    mutable bool layoutBandOverride_ = false;
    mutable ofRectangle layoutBandBounds_;
    MenuSkin skin_ = MenuSkin::ConsoleHub();

    const std::string stateId = "ui.console";
    const std::string stateLabel = "Console";
    const std::string scopeId = "Home";
    static constexpr int kSlotCount = 8;
    static constexpr int kRowCount = 6;

    mutable std::unordered_map<std::string, float> lastValueSnapshot_;
    mutable std::unordered_map<std::string, std::string> lastStringSnapshot_;
    mutable std::unordered_map<std::string, uint64_t> lastActivityMs_;

    bool isParameterActive(const std::string& parameterId) const;
    std::string ellipsize(const std::string& text, float maxWidth) const;
};

// Inline implementations to avoid needing a separate TU in the user's project
inline ConsoleState::ConsoleState() {
}

inline void ConsoleState::setRequestAssetBrowserCallback(std::function<void(int)> cb) {
    requestAssetBrowserCallback_ = std::move(cb);
}

inline void ConsoleState::setRequestClearLayerCallback(std::function<void(int)> cb) {
    requestClearLayerCallback_ = std::move(cb);
}

inline void ConsoleState::setQueryLayerCallback(std::function<ConsoleLayerInfo(int)> cb) {
    queryLayerCallback_ = std::move(cb);
}

inline void ConsoleState::setQueryParameterPreviewCallback(std::function<ParameterPreview(int,int)> cb) {
    // rowIndex is 0-based
    queryParameterPreviewCallback_ = std::move(cb);
}

inline void ConsoleState::setParameterRegistry(ParameterRegistry* registry) {
    registry_ = registry;
    lastValueSnapshot_.clear();
    lastStringSnapshot_.clear();
    lastActivityMs_.clear();
}

inline void ConsoleState::setLayoutBand(const ofRectangle& bounds) {
    layoutBandOverride_ = bounds.getWidth() > 0.0f && bounds.getHeight() > 0.0f;
    if (layoutBandOverride_) {
        layoutBandBounds_ = bounds;
    }
}

inline void ConsoleState::clearLayoutBand() {
    layoutBandOverride_ = false;
    layoutBandBounds_.set(0.0f, 0.0f, 0.0f, 0.0f);
}

inline void ConsoleState::setMenuSkin(const MenuSkin& skin) {
    skin_ = skin;
}

inline MenuController::StateView ConsoleState::view() const {
    MenuController::StateView view;
    view.entries.reserve(kSlotCount);
    for (int i = 0; i < kSlotCount; ++i) {
        const int slotIndex = i + 1;
        ConsoleLayerInfo info;
        if (queryLayerCallback_) {
            info = queryLayerCallback_(slotIndex);
        }
        MenuController::EntryView entry;
        entry.id = "console.slot." + std::to_string(slotIndex);
        entry.label = "Slot " + std::to_string(slotIndex);
        if (info.assetId.empty()) {
            entry.label += " \u2022 (empty)";
            entry.description = "Load an asset into this slot";
        } else {
            std::string resolvedLabel = info.label.empty() ? info.assetId : info.label;
            entry.label += " \u2022 " + resolvedLabel;
            std::string status = info.active ? "Active" : "Assigned";
            status += " \u2022 opacity " + ofToString(ofClamp(info.opacity, 0.0f, 1.0f), 2);
            entry.description = status;
        }
        entry.selectable = true;
        entry.selected = (selectedColumn_ == i);
        view.entries.push_back(std::move(entry));
    }
    view.selectedIndex = ofClamp(selectedColumn_, 0, kSlotCount - 1);

    auto appendHint = [&](int key, const std::string& label, const std::string& description) {
        MenuController::KeyHint hint;
        hint.key = key;
        hint.label = label;
        hint.description = description;
        view.hotkeys.push_back(std::move(hint));
    };
    appendHint(OF_KEY_LEFT, "Left/Right", "Change focused console slot");
    appendHint(OF_KEY_RETURN, "Enter", "Open the asset picker for the focused slot");
    appendHint(MenuController::HOTKEY_MOD_CTRL | '1', "Ctrl+1..8", "Open a specific slot's asset picker");
    appendHint(MenuController::HOTKEY_MOD_CTRL | 'u', "Ctrl+U", "Unload the focused slot");

    return view;
}

inline bool ConsoleState::handleInput(MenuController& controller, int key) {
    const int baseKey = key & 0xFFFF;

    bool handled = false;
    bool refreshView = false;

    auto clampSelection = [&]() {
        int clamped = ofClamp(selectedColumn_, 0, kSlotCount - 1);
        if (clamped != selectedColumn_) {
            selectedColumn_ = clamped;
            refreshView = true;
        }
    };

    // Navigation: arrows and tab
    if (baseKey == OF_KEY_LEFT) {
        if (selectedColumn_ > 0) {
            --selectedColumn_;
            refreshView = true;
        }
        handled = true;
    } else if (baseKey == OF_KEY_RIGHT) {
        if (selectedColumn_ < kSlotCount - 1) {
            ++selectedColumn_;
            refreshView = true;
        }
        handled = true;
    } else if (baseKey == OF_KEY_TAB) {
        selectedColumn_ = (selectedColumn_ + 1) % kSlotCount;
        refreshView = true;
        handled = true;
    }

    // Enter: open asset browser for the selected column
    if (baseKey == OF_KEY_RETURN || baseKey == '\r') {
        if (requestAssetBrowserCallback_) {
            requestAssetBrowserCallback_(selectedColumn_ + 1);
            handled = true;
        } else {
            ofLogWarning("ConsoleState") << "Enter pressed but no requestAssetBrowserCallback_ set";
        }
    }

    // Handle raw keys for Ctrl+1..Ctrl+8 to open the asset browser for that column.
    const int mods = key & MenuController::HOTKEY_MOD_MASK;
    if (!handled && (mods & MenuController::HOTKEY_MOD_CTRL) != 0) {
        if (baseKey >= '1' && baseKey <= '8') {
            int layer = baseKey - '0';
            selectedColumn_ = layer - 1;
            refreshView = true;
            clampSelection();
            if (requestAssetBrowserCallback_) {
                requestAssetBrowserCallback_(layer);
            } else {
                ofLogWarning("ConsoleState") << "Ctrl+digit detected but no requestAssetBrowserCallback_ set!";
            }
            handled = true;
        } else if (baseKey == 'u' || baseKey == 'U') {
            if (requestClearLayerCallback_) {
                requestClearLayerCallback_(selectedColumn_ + 1);
                handled = true;
            } else {
                ofLogWarning("ConsoleState") << "Ctrl+U pressed but no requestClearLayerCallback_ set!";
            }
        }
    }

    if (refreshView) {
        clampSelection();
        controller.requestViewModelRefresh();
    }

    return handled;
}

inline void ConsoleState::onEnter(MenuController& controller) {
    active_ = true;
}

inline void ConsoleState::onExit(MenuController& controller) {
    active_ = false;
}

inline void ConsoleState::draw() const {
    const bool useLayoutBand = layoutBandOverride_ && layoutBandBounds_.getWidth() > 1.0f && layoutBandBounds_.getHeight() > 1.0f;
    float viewportWidth = useLayoutBand ? layoutBandBounds_.getWidth() : static_cast<float>(ofGetWidth());
    float viewportHeight = useLayoutBand ? layoutBandBounds_.getHeight() : static_cast<float>(ofGetHeight());
    if (viewportWidth <= 1.0f || viewportHeight <= 1.0f) {
        return;
    }
    if (useLayoutBand) {
        ofPushMatrix();
        ofTranslate(layoutBandBounds_.x, layoutBandBounds_.y);
    }

    const float margin = skin_.metrics.margin;
    const float padding = skin_.metrics.padding;
    float width = std::max(60.0f, viewportWidth - margin * 2.0f);
    float height = std::max(60.0f, viewportHeight - margin * 2.0f);
    float outerX = margin;
    float outerY = margin;

    const float slotHeaderHeight = skin_.metrics.columnHeaderHeight + 24.0f;
    float availableGridHeight = height - slotHeaderHeight - padding;
    float rowHeight = std::max(skin_.metrics.rowHeight, availableGridHeight / static_cast<float>(kRowCount));
    float gridHeight = rowHeight * static_cast<float>(kRowCount);
    float panelHeight = slotHeaderHeight + gridHeight;
    if (panelHeight < height) {
        outerY += (height - panelHeight) * 0.5f;
    }

    float slotSpacing = skin_.metrics.panelSpacing;
    float slotWidth = (width - slotSpacing * static_cast<float>(kSlotCount - 1)) / static_cast<float>(kSlotCount);
    slotWidth = std::max(90.0f, slotWidth);
    if (slotWidth * kSlotCount + slotSpacing * (kSlotCount - 1) > width) {
        slotSpacing = std::max(4.0f, (width - slotWidth * kSlotCount) / std::max(1, kSlotCount - 1));
    }
    float totalWidth = slotWidth * kSlotCount + slotSpacing * (kSlotCount - 1);
    float startX = outerX + std::max(0.0f, (width - totalWidth) * 0.5f);

    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    auto drawTextStyled = [textScale](const std::string& text, float x, float y, const ofColor& color, bool bold) {
        ofSetColor(color);
        drawBitmapStringScaled(text, x, y, textScale, bold);
    };

    ofPushStyle();
    ofSetColor(skin_.palette.background);
    ofDrawRectangle(0.0f, 0.0f, viewportWidth, viewportHeight);

    ofSetColor(skin_.palette.surface);
    ofDrawRectRounded(outerX, outerY, width, panelHeight, skin_.metrics.borderRadius);

    float headerLabelWidth = slotWidth - padding * 1.4f;
    for (int column = 0; column < kSlotCount; ++column) {
        float columnX = startX + column * (slotWidth + slotSpacing);
        ConsoleLayerInfo info;
        if (queryLayerCallback_) {
            info = queryLayerCallback_(column + 1);
        }
        std::string slotLabel = "Slot " + std::to_string(column + 1);
        std::string assetLabel;
        if (info.assetId.empty()) {
            assetLabel = "(empty)";
        } else if (!info.label.empty()) {
            assetLabel = info.label;
        } else {
            assetLabel = info.assetId;
        }
        assetLabel = ellipsize(assetLabel, headerLabelWidth);

        drawTextStyled(slotLabel,
                       columnX + padding,
                       outerY + padding + 12.0f * textScale,
                       skin_.palette.headerText,
                       true);
        drawTextStyled(assetLabel,
                       columnX + padding,
                       outerY + padding + 28.0f * textScale,
                       skin_.palette.bodyText,
                       false);
    }

    ofSetColor(skin_.palette.gridDivider);
    ofDrawLine(outerX + padding * 0.5f,
               outerY + slotHeaderHeight,
               outerX + width - padding * 0.5f,
               outerY + slotHeaderHeight);

    for (int column = 0; column < kSlotCount; ++column) {
        float columnX = startX + column * (slotWidth + slotSpacing);
        float cellWidth = slotWidth - padding;
        for (int row = 0; row < kRowCount; ++row) {
            float cellY = outerY + slotHeaderHeight + row * rowHeight;
            ParameterPreview preview;
            if (queryParameterPreviewCallback_) {
                preview = queryParameterPreviewCallback_(column + 1, row);
            }
            std::string left = preview.valid ? preview.label : std::string();
            if (left.empty() && preview.valid) {
                left = preview.detail;
            }
            if (left.empty()) {
                left = "-";
            }
            std::string detail = preview.detail;
            left = ellipsize(left, cellWidth - padding * 0.2f);
            if (!detail.empty()) {
                detail = ellipsize(detail, cellWidth - padding * 0.2f);
            }

            bool highlightActive = preview.valid && isParameterActive(preview.parameterId);
            if (highlightActive) {
                ofSetColor(skin_.palette.gridSelectionFill);
                ofDrawRectRounded(columnX + padding * 0.5f,
                                  cellY + 2.0f,
                                  cellWidth,
                                  rowHeight - 4.0f,
                                  skin_.metrics.borderRadius * 0.5f);
                ofNoFill();
                ofSetColor(skin_.palette.gridSelection);
                ofSetLineWidth(skin_.metrics.focusStroke);
                ofDrawRectRounded(columnX + padding * 0.5f,
                                  cellY + 2.0f,
                                  cellWidth,
                                  rowHeight - 4.0f,
                                  skin_.metrics.borderRadius * 0.5f);
                ofSetLineWidth(1.0f);
                ofFill();
            }

            ofColor textColor = highlightActive ? skin_.palette.gridSelection : skin_.palette.bodyText;
            bool bold = highlightActive;
            drawTextStyled(left, columnX + padding, cellY + 14.0f * textScale, textColor, bold);
            if (!detail.empty()) {
                drawTextStyled(detail, columnX + padding, cellY + 30.0f * textScale, textColor, bold);
            }
        }
    }

    ofPopStyle();
    if (useLayoutBand) {
        ofPopMatrix();
    }
}

inline bool ConsoleState::isParameterActive(const std::string& parameterId) const {
    if (!registry_ || parameterId.empty()) {
        return false;
    }
    uint64_t nowMs = ofGetElapsedTimeMillis();
    bool observed = false;
    auto trackNumeric = [&](float value) {
        observed = true;
        auto it = lastValueSnapshot_.find(parameterId);
        if (it == lastValueSnapshot_.end() || std::fabs(it->second - value) > 1e-3f) {
            lastValueSnapshot_[parameterId] = value;
            lastActivityMs_[parameterId] = nowMs;
        }
    };
    auto trackString = [&](const std::string& value) {
        observed = true;
        auto it = lastStringSnapshot_.find(parameterId);
        if (it == lastStringSnapshot_.end() || it->second != value) {
            lastStringSnapshot_[parameterId] = value;
            lastActivityMs_[parameterId] = nowMs;
        }
    };

    if (auto* fp = registry_->findFloat(parameterId)) {
        float current = fp->value ? *fp->value : fp->baseValue;
        trackNumeric(current);
    } else if (auto* bp = registry_->findBool(parameterId)) {
        float current = bp->value ? (*bp->value ? 1.0f : 0.0f) : (bp->baseValue ? 1.0f : 0.0f);
        trackNumeric(current);
    } else if (auto* sp = registry_->findString(parameterId)) {
        const std::string& current = sp->value ? *sp->value : sp->baseValue;
        trackString(current);
    }

    if (!observed) {
        return false;
    }

    auto it = lastActivityMs_.find(parameterId);
    if (it == lastActivityMs_.end()) {
        return false;
    }
    constexpr uint64_t kActivityWindowMs = 1500;
    if (nowMs < it->second) {
        return false;
    }
    return (nowMs - it->second) <= kActivityWindowMs;
}

inline std::string ConsoleState::ellipsize(const std::string& text, float maxWidth) const {
    if (maxWidth <= 0.0f) {
        return text;
    }
    float textScale = std::max(0.01f, skin_.metrics.typographyScale);
    static ofBitmapFont bitmapFont;
    auto bbox = bitmapFont.getBoundingBox(text, 0.0f, 0.0f);
    if (bbox.getWidth() * textScale <= maxWidth) {
        return text;
    }
    std::string trimmed = text;
    while (!trimmed.empty()) {
        trimmed.pop_back();
        std::string candidate = trimmed + "...";
        bbox = bitmapFont.getBoundingBox(candidate, 0.0f, 0.0f);
        if (bbox.getWidth() * textScale <= maxWidth) {
            return candidate;
        }
    }
    return std::string("...");
}

#include "MenuMirrorHudWidget.h"

#include <algorithm>

#include "ofGraphics.h"
#include "ofJson.h"

#include "HudThemeUtils.h"
#include "../../ofApp.h"
#include "../MenuController.h"

namespace {
ofRectangle approximateBitmapBounds(const std::string& text, float lineHeight) {
    if (text.empty()) {
        return ofRectangle(0.0f, 0.0f, 0.0f, lineHeight);
    }
    float width = 0.0f;
    float maxWidth = 0.0f;
    int lines = 1;
    constexpr float kCharWidth = 8.0f;
    for (char c : text) {
        if (c == '\n') {
            maxWidth = std::max(maxWidth, width);
            width = 0.0f;
            ++lines;
        } else if (c == '\t') {
            width += kCharWidth * 4.0f;
        } else {
            width += kCharWidth;
        }
    }
    maxWidth = std::max(maxWidth, width);
    return ofRectangle(0.0f, 0.0f, maxWidth, static_cast<float>(lines) * lineHeight);
}

ofRectangle approximateBitmapBoundsScaled(const std::string& text, float lineHeight, float scale) {
    ofRectangle bounds = approximateBitmapBounds(text, lineHeight);
    bounds.width *= std::max(0.01f, scale);
    return bounds;
}

struct MenuFeedState {
    bool valid = false;
    bool hasState = false;
    std::vector<std::string> breadcrumbs;
    std::string scope;
    std::string selectedLabel;
    std::string selectedDescription;
    std::vector<MenuController::KeyHint> hotkeys;
    std::vector<std::string> conflicts;
};

MenuFeedState parseMenuFeed(const ofJson& payload) {
    MenuFeedState data;
    if (!payload.is_object()) {
        return data;
    }
    data.valid = true;
    data.hasState = payload.value("hasState", false);
    if (payload.contains("breadcrumbs") && payload["breadcrumbs"].is_array()) {
        for (const auto& crumb : payload["breadcrumbs"]) {
            if (crumb.is_string()) {
                data.breadcrumbs.push_back(crumb.get<std::string>());
            }
        }
    }
    data.scope = payload.value("scope", std::string());
    if (payload.contains("selected") && payload["selected"].is_object()) {
        const auto& selected = payload["selected"];
        data.selectedLabel = selected.value("label", std::string());
        data.selectedDescription = selected.value("description", std::string());
    }
    if (payload.contains("hotkeys") && payload["hotkeys"].is_array()) {
        for (const auto& node : payload["hotkeys"]) {
            if (!node.is_object()) continue;
            MenuController::KeyHint hint;
            hint.label = node.value("label", std::string());
            hint.description = node.value("description", std::string());
            data.hotkeys.push_back(std::move(hint));
        }
    }
    if (payload.contains("hotkeyConflicts") && payload["hotkeyConflicts"].is_array()) {
        for (const auto& entry : payload["hotkeyConflicts"]) {
            if (entry.is_string()) {
                data.conflicts.push_back(entry.get<std::string>());
            }
        }
    }
    return data;
}

MenuFeedState snapshotToMenuData(const ofApp::MenuHudSnapshot& snapshot,
                                 const std::vector<std::string>& conflicts) {
    MenuFeedState data;
    data.valid = true;
    data.hasState = snapshot.hasState;
    data.breadcrumbs = snapshot.breadcrumbs;
    data.scope = snapshot.scope;
    data.selectedLabel = snapshot.selectedLabel;
    data.selectedDescription = snapshot.selectedDescription;
    data.hotkeys = snapshot.hotkeys;
    data.conflicts = conflicts;
    return data;
}
}

MenuMirrorHudWidget::MenuMirrorHudWidget() {
    metadata_.id = "hud.menu";
    metadata_.label = "Menu Mirror";
    metadata_.category = "HUD";
    metadata_.description = "Active menu breadcrumb trail and hotkeys.";
    metadata_.defaultColumn = 1;
    metadata_.defaultHeight = 240.0f;
    metadata_.minHeight = 160.0f;
    metadata_.allowsDetach = false;
    metadata_.band = OverlayWidget::Band::Hud;
}

void MenuMirrorHudWidget::setup(const SetupParams& params) {
    app_ = params.app;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void MenuMirrorHudWidget::update(const UpdateParams& params) {
    if (params.app) {
        app_ = params.app;
    }
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void MenuMirrorHudWidget::draw(const DrawParams& params) {
    if (!app_) {
        return;
    }

    MenuFeedState state;
    bool usingFeed = false;
    if (auto feed = app_->latestHudFeed("hud.menu")) {
        state = parseMenuFeed(feed->payload);
        usingFeed = state.valid;
    }
    if (!usingFeed) {
        state = snapshotToMenuData(app_->menuHudSnapshot, app_->menuController.hotkeyConflicts());
    }
    const auto& snapshot = app_->menuHudSnapshot;
    ofPushStyle();
    drawHudPanelBackground(params.bounds, hudSkin_);
    float padding = hudBlockPadding(hudSkin_);
    float lineH = hudLineHeight(hudSkin_);
    float textScale = hudTypographyScale(hudSkin_);
    float x = params.bounds.x + padding;
    float y = params.bounds.y + padding + lineH;

    if (state.hasState && !state.breadcrumbs.empty()) {
        std::string breadcrumbs;
        for (std::size_t i = 0; i < state.breadcrumbs.size(); ++i) {
            breadcrumbs += state.breadcrumbs[i];
            if (i + 1 < state.breadcrumbs.size()) {
                breadcrumbs += " / ";
            }
        }
        ofSetColor(hudTextColor(hudSkin_));
        drawBitmapStringScaled(breadcrumbs, x, y, textScale);
        y += lineH;
    } else if (!state.hasState) {
        ofSetColor(hudMutedColor(hudSkin_));
        drawBitmapStringScaled("No active menu", x, y, textScale);
        y += lineH;
    }

    if (!state.scope.empty()) {
        ofSetColor(hudMutedColor(hudSkin_));
        drawBitmapStringScaled("Scope: " + state.scope, x, y, textScale);
        y += lineH;
    }

    if (!state.selectedLabel.empty()) {
        ofSetColor(hudTextColor(hudSkin_));
        std::string selected = "> " + state.selectedLabel;
        if (!state.selectedDescription.empty()) {
            selected += " :: " + state.selectedDescription;
        }
        drawBitmapStringScaled(selected, x, y, textScale);
        y += lineH;
    }

    if (!state.hotkeys.empty()) {
        y += lineH * 0.5f;
        float availableWidth = params.bounds.width - padding * 2.0f;
        float badgeHeight = hudBadgeHeight(hudSkin_);
        float badgePad = hudBadgePadding(hudSkin_);
        float rowHeight = badgeHeight + badgePad;
        float cursorX = x;
        float rowStartX = x;
        for (const auto& hint : state.hotkeys) {
            std::string label = hint.label.empty() ? "Key" : hint.label;
            std::string description = hint.description;
            auto labelBox = approximateBitmapBoundsScaled(label, lineH, textScale);
            auto descBox = approximateBitmapBoundsScaled(description, lineH, textScale);
            float badgeWidth = labelBox.width + badgePad * 2.0f;
            float entryWidth = badgeWidth;
            if (!description.empty()) {
                entryWidth += badgePad + descBox.width;
            }
            if (cursorX + entryWidth > rowStartX + availableWidth && cursorX > rowStartX) {
                cursorX = rowStartX;
                y += rowHeight;
            }
            ofRectangle badge(cursorX, y, badgeWidth, badgeHeight);
            ofSetColor(hudBadgeFill(hudSkin_));
            ofDrawRectRounded(badge, hudBadgeRadius(hudSkin_));
            ofNoFill();
            ofSetColor(hudBadgeStroke(hudSkin_));
            ofDrawRectRounded(badge, hudBadgeRadius(hudSkin_));
            ofFill();
            ofSetColor(hudBadgeText(hudSkin_));
            float textY = badge.y + badge.height * 0.65f;
            drawBitmapStringScaled(label, badge.x + badgePad, textY, textScale);

            if (!description.empty()) {
                ofSetColor(hudMutedColor(hudSkin_));
                drawBitmapStringScaled(description, badge.getRight() + badgePad, textY, textScale);
            }

            cursorX += entryWidth + badgePad;
        }
        y += rowHeight;
    }

    ofPopStyle();
}

float MenuMirrorHudWidget::preferredHeight(float width) const {
    if (!app_) {
        return metadata_.defaultHeight;
    }
    MenuFeedState state;
    bool usingFeed = false;
    if (auto feed = app_->latestHudFeed("hud.menu")) {
        state = parseMenuFeed(feed->payload);
        usingFeed = state.valid;
    }
    if (!usingFeed) {
        state = snapshotToMenuData(app_->menuHudSnapshot, app_->menuController.hotkeyConflicts());
    }
    float lineH = hudLineHeight(hudSkin_);
    float padding = hudBlockPadding(hudSkin_);
    float textScale = hudTypographyScale(hudSkin_);
    float height = padding * 2.0f;
    std::size_t textLines = 0;
    if (state.hasState && !state.breadcrumbs.empty()) {
        textLines++;
    } else if (!state.hasState) {
        textLines++;
    }
    if (!state.scope.empty()) {
        textLines++;
    }
    if (!state.selectedLabel.empty()) {
        textLines++;
    }
    height += static_cast<float>(textLines) * lineH;

    if (!state.hotkeys.empty()) {
        height += lineH * 0.5f;
        float availableWidth = (width > 0.0f ? width : 360.0f) - padding * 2.0f;
        if (availableWidth <= 0.0f) {
            availableWidth = 320.0f;
        }
        float badgeHeight = hudBadgeHeight(hudSkin_);
        float badgePad = hudBadgePadding(hudSkin_);
        float cursor = 0.0f;
        int rows = 1;
        for (const auto& hint : state.hotkeys) {
            std::string label = hint.label.empty() ? "Key" : hint.label;
            std::string description = hint.description;
            auto labelBox = approximateBitmapBoundsScaled(label, lineH, textScale);
            auto descBox = approximateBitmapBoundsScaled(description, lineH, textScale);
            float badgeWidth = labelBox.width + badgePad * 2.0f;
            float entryWidth = badgeWidth;
            if (!description.empty()) {
                entryWidth += badgePad + descBox.width;
            }
            if (cursor + entryWidth > availableWidth && cursor > 0.0f) {
                rows++;
                cursor = 0.0f;
            }
            cursor += entryWidth + badgePad;
        }
        height += static_cast<float>(rows) * (badgeHeight + badgePad);
    }

    return std::max(metadata_.minHeight, height);
}

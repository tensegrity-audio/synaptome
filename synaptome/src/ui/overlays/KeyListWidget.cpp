#include "KeyListWidget.h"
#include "../../ofApp.h"
#include "../HotkeyManager.h"
#include "ofGraphics.h"
#include "ofUtils.h"

#include "HudThemeUtils.h"

KeyListWidget::KeyListWidget() {
    meta_.id = "key_list";
    meta_.label = "Key List";
    meta_.category = "HUD";
    meta_.description = "Displays scoped hotkeys and conflicts";
    meta_.defaultColumn = 0;
    meta_.defaultHeight = 220.0f;
    meta_.band = OverlayWidget::Band::Hud;
}

void KeyListWidget::setup(const SetupParams& params) {
    if (!params.app) {
        return;
    }
    ofApp* app = params.app;
    hotkeyManager_ = &app->hotkeyManager;
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
}

void KeyListWidget::update(const UpdateParams& params) {
    if (params.hudSkin) {
        hudSkin_ = params.hudSkin;
    }
    if (!hotkeyManager_) {
        return;
    }
    cachedBindings_ = hotkeyManager_->orderedBindings();
}

void KeyListWidget::draw(const DrawParams& params) {
    ofPushStyle();
    drawHudPanelBackground(params.bounds, hudSkin_);
    float padding = hudBlockPadding(hudSkin_);
    float lineH = hudLineHeight(hudSkin_);
    float x = params.bounds.x + padding;
    float y = params.bounds.y + padding + lineH;

    std::string currentScope;
    for (const auto* b : cachedBindings_) {
        if (!b) {
            continue;
        }
        if (b->scope != currentScope) {
            currentScope = b->scope;
            std::string header = currentScope.empty() ? "Global" : currentScope;
            ofSetColor(hudAccentColor(hudSkin_));
            drawBitmapStringScaled(header, x, y, hudTypographyScale(hudSkin_));
            y += lineH;
            ofSetColor(hudTextColor(hudSkin_));
        }
        std::string name = b->displayName.empty() ? b->id : b->displayName;
        std::string key = HotkeyManager::keyLabel(b->currentKey);
        bool dirty = false;
        bool conflicts = false;
        if (hotkeyManager_) {
            dirty = hotkeyManager_->bindingDirty(b->id);
            auto conf = hotkeyManager_->bindingConflicts(b->id);
            conflicts = !conf.empty();
        }
        std::string line = name + "\t" + key;
        if (dirty) line += "  *";
        if (conflicts) line += "  !!";
        if (conflicts) {
            ofSetColor(hudWarningColor(hudSkin_));
        } else if (dirty) {
            ofSetColor(hudAccentColor(hudSkin_));
        } else {
            ofSetColor(hudTextColor(hudSkin_));
        }
        drawBitmapStringScaled(line, x, y, hudTypographyScale(hudSkin_));
        y += lineH;
    }

    ofPopStyle();
}

float KeyListWidget::preferredHeight(float width) const {
    float lineH = hudLineHeight(hudSkin_);
    std::size_t rows = cachedBindings_.size();
    std::string lastScope;
    std::size_t headers = 0;
    for (const auto* b : cachedBindings_) {
        if (!b) {
            continue;
        }
        if (b->scope != lastScope) {
            headers++;
            lastScope = b->scope;
        }
    }
    float total = static_cast<float>(rows + headers) * lineH + hudBlockPadding(hudSkin_) * 2.0f;
    lastPreferredWidth_ = width;
    return total;
}

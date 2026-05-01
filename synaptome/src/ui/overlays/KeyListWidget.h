#pragma once

#include "OverlayWidget.h"
#include "../HotkeyManager.h"
#include <memory>
#include <vector>

class KeyListWidget : public OverlayWidget {
public:
    KeyListWidget();

    const Metadata& metadata() const override { return meta_; }
    void setup(const SetupParams& params) override;
    void update(const UpdateParams& params) override;
    void draw(const DrawParams& params) override;
    float preferredHeight(float width) const override;

private:
    Metadata meta_;
    HotkeyManager* hotkeyManager_ = nullptr;
    std::vector<const HotkeyManager::Binding*> cachedBindings_;
    mutable float lastPreferredWidth_ = 0.0f;
    const HudSkin* hudSkin_ = nullptr;
};

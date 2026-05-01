#pragma once

#include "OverlayWidget.h"

class MenuMirrorHudWidget : public OverlayWidget {
public:
    MenuMirrorHudWidget();

    const Metadata& metadata() const override { return metadata_; }
    void setup(const SetupParams& params) override;
    void update(const UpdateParams& params) override;
    void draw(const DrawParams& params) override;
    float preferredHeight(float width) const override;

private:
    Metadata metadata_;
    class ofApp* app_ = nullptr;
    const HudSkin* hudSkin_ = nullptr;
};

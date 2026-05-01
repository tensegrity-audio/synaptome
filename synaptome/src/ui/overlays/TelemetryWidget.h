#pragma once

#include "OverlayWidget.h"

class TelemetryWidget : public OverlayWidget {
public:
    TelemetryWidget();

    const Metadata& metadata() const override { return meta_; }
    void setup(const SetupParams& params) override;
    void update(const UpdateParams& params) override;
    void draw(const DrawParams& params) override;

private:
    Metadata meta_;
    class ofApp* app_ = nullptr;
    float fps_ = 0.0f;
    float gpuPercent_ = -1.0f;
    std::string lastLine_;
    const HudSkin* hudSkin_ = nullptr;
};

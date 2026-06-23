#pragma once

#include "OverlayWidget.h"

#include <cstdint>
#include <string>
#include <vector>

class AudioMonitorWidget : public OverlayWidget {
public:
    AudioMonitorWidget();

    const Metadata& metadata() const override { return metadata_; }
    void setup(const SetupParams& params) override;
    void update(const UpdateParams& params) override;
    void draw(const DrawParams& params) override;
    float preferredHeight(float width) const override;

private:
    Metadata metadata_;
    const HudSkin* hudSkin_ = nullptr;
    std::vector<float> waveform_;
    std::string sourceLabel_;
    float level_ = 0.0f;
    float peak_ = 0.0f;
    float bass_ = 0.0f;
    float mids_ = 0.0f;
    float highs_ = 0.0f;
    uint64_t lastFrame_ = 0;
    uint64_t lastSampleMs_ = 0;
    bool hasSample_ = false;
};

#pragma once

#include <string>
#include <vector>

// Shared parameter storage for the text layer so Browser controls remain
// available even when no text slot is active.
struct TextLayerState {
    static TextLayerState& instance();

    void refreshAvailableFonts();
    void syncFontSelection();
    float fontIndexMax() const;

    std::string content = "Hello";
    std::string font = "VCR_OSD_MONO_1.001.ttf";
    float fontIndex = 0.0f;
    float fontSize = 48.0f;
    float colorR = 0.0f;
    float colorG = 255.0f;
    float colorB = 160.0f;

private:
    TextLayerState() = default;

    std::vector<std::string> availableFonts_;
    float fontIndexSnapshot_ = 0.0f;
    std::string fontNameSnapshot_ = "VCR_OSD_MONO_1.001.ttf";

    friend class TextLayer;
    friend class TextLayerStateAccess;
};

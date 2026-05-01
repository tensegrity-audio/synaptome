#pragma once

#ifdef OF_SDK_AVAILABLE
#include <graphics/ofTrueTypeFont.h>
#else

#include "ofGraphics.h"
#include <algorithm>
#include <string>
#include <utility>

class ofTrueTypeFontSettings {
public:
    ofTrueTypeFontSettings(std::string fontPath, int fontSize)
        : fontPath(std::move(fontPath)), fontSize(fontSize) {}

    std::string fontPath;
    int fontSize = 12;
    bool antialiased = true;
    int dpi = 96;
};

class ofTrueTypeFont {
public:
    bool load(const ofTrueTypeFontSettings& settings) {
        settings_ = settings;
        loaded_ = true;
        return true;
    }

    ofRectangle getStringBoundingBox(const std::string& text, float, float) const {
        const float size = static_cast<float>(settings_.fontSize > 0 ? settings_.fontSize : 12);
        const float width = std::max(1.0f, static_cast<float>(text.size()) * size * 0.5f);
        return ofRectangle(0.0f, -size, width, size);
    }

    void drawString(const std::string&, float, float) const {}
    bool isLoaded() const { return loaded_; }

private:
    ofTrueTypeFontSettings settings_{"", 12};
    bool loaded_ = false;
};

#endif

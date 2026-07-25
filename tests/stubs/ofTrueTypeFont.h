#pragma once

#ifdef OF_SDK_AVAILABLE
#include <graphics/ofTrueTypeFont.h>
#else

#include "ofGraphics.h"
#include <algorithm>
#include <string>
#include <utility>

class ofUnicode {
public:
    struct range {};

    static inline const range Latin{};
    static inline const range Latin1Supplement{};
    static inline const range GeneralPunctuation{};
    static inline const range CurrencySymbols{};
    static inline const range Arrows{};
    static inline const range MathOperators{};
    static inline const range BoxDrawing{};
    static inline const range BlockElement{};
    static inline const range GeometricShapes{};
    static inline const range MiscSymbols{};
};

class ofTrueTypeFontSettings {
public:
    ofTrueTypeFontSettings(std::string fontPath, int fontSize)
        : fontPath(std::move(fontPath)), fontSize(fontSize) {}

    std::string fontPath;
    int fontSize = 12;
    bool antialiased = true;
    bool contours = false;
    int dpi = 96;

    void addRange(const ofUnicode::range&) {}
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

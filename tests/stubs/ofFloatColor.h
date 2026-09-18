#pragma once

#ifdef OF_SDK_AVAILABLE
#include <types/ofColor.h>
#else

// Normalized color used by package mesh APIs. This is separate from the legacy
// unsigned-byte ofColor stub so ofMesh can include it without a header cycle.
struct ofFloatColor {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    ofFloatColor() = default;
    explicit ofFloatColor(float gray, float alpha = 1.0f)
        : r(gray), g(gray), b(gray), a(alpha) {}
    ofFloatColor(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

#endif

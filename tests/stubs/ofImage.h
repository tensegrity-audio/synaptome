#pragma once

#ifdef OF_SDK_AVAILABLE
#include <graphics/ofImage.h>
#else

#include "ofPixels.h"
#include <string>

inline bool ofLoadImage(ofPixels& pixels, const std::string&) {
    pixels.allocate(16, 16, OF_PIXELS_RGBA);
    pixels.set(255);
    return true;
}

#endif

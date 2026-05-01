#pragma once

#ifdef OF_SDK_AVAILABLE
#include <gl/ofTexture.h>
#else

#include "ofPixels.h"

struct ofTextureData {
    unsigned int textureID = 1;
    unsigned int textureTarget = 0;
    unsigned int glInternalFormat = 0;
};

class ofTexture {
public:
    bool isAllocated() const { return allocated_; }

    void allocate(int width, int height, int /*internalFormat*/ = 0) {
        width_ = width;
        height_ = height;
        allocated_ = true;
    }

    void clear() {
        allocated_ = false;
        width_ = 0;
        height_ = 0;
    }

    float getWidth() const { return static_cast<float>(width_); }
    float getHeight() const { return static_cast<float>(height_); }

    void bind() const {}
    void unbind() const {}
    void draw(float, float, float, float) const {}
    void drawSubsection(float, float, float, float, float, float, float, float) const {}
    void loadData(const ofPixels& pixels) {
        width_ = pixels.getWidth();
        height_ = pixels.getHeight();
        allocated_ = pixels.size() > 0;
    }
    void readToPixels(ofPixels& pixels) const {
        pixels.allocate(width_, height_, OF_PIXELS_RGBA);
    }
    void setTextureMinMagFilter(int, int) {}
    void setTextureWrap(int, int) {}
    const ofTextureData& getTextureData() const { return textureData_; }

private:
    int width_ = 0;
    int height_ = 0;
    bool allocated_ = false;
    ofTextureData textureData_;
};

#endif

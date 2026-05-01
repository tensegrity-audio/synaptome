#pragma once

#ifdef OF_SDK_AVAILABLE
#include <gl/ofFbo.h>
#else

#include "ofPixels.h"
#include "ofTexture.h"
#include "ofGLStub.h"

enum ofFboMode {
    OF_FBOMODE_STANDARD = 0,
};

class ofFbo {
public:
    struct Settings {
        int width = 0;
        int height = 0;
        bool useDepth = false;
        bool useStencil = false;
        int internalformat = 0;
        int textureTarget = 0;
        int minFilter = 0;
        int maxFilter = 0;
        int wrapModeHorizontal = 0;
        int wrapModeVertical = 0;
    };

    bool isAllocated() const { return allocated_; }

    void allocate(const Settings& settings) {
        allocate(settings.width, settings.height);
    }

    void allocate(int width, int height, int /*internalFormat*/ = 0) {
        width_ = width;
        height_ = height;
        texture_.allocate(width, height);
        allocated_ = true;
    }

    void begin(ofFboMode = OF_FBOMODE_STANDARD) {}
    void begin(ofFboMode = OF_FBOMODE_STANDARD) const {}

    void end() {}
    void end() const {}

    void clear() {
        allocated_ = false;
        width_ = 0;
        height_ = 0;
        texture_.clear();
    }

    float getWidth() const { return static_cast<float>(width_); }
    float getHeight() const { return static_cast<float>(height_); }

    ofTexture& getTexture() { return texture_; }
    const ofTexture& getTexture() const { return texture_; }
    void readToPixels(ofPixels& pixels) const {
        pixels.allocate(width_, height_, OF_PIXELS_RGBA);
    }

    void draw(float, float, float, float) const {}

private:
    int width_ = 0;
    int height_ = 0;
    bool allocated_ = false;
    mutable ofTexture texture_;
};

#endif

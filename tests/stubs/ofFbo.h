#pragma once

#ifdef OF_SDK_AVAILABLE
#include <gl/ofFbo.h>
#else

#include "ofPixels.h"
#include "ofTexture.h"
#include "ofGLStub.h"
#if defined(SYNAPTOME_OF_STUB_TRACE)
#include "ofStubTrace.h"
#endif

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
        allocateImpl(settings);
    }

    void allocate(int width, int height, int internalFormat = 0) {
        Settings settings;
        settings.width = width;
        settings.height = height;
        settings.internalformat = internalFormat;
        allocateImpl(settings);
    }

    void begin(ofFboMode = OF_FBOMODE_STANDARD) {
#if defined(SYNAPTOME_OF_STUB_TRACE)
        ofstub::beginTarget(traceId_, allocated_);
#endif
    }
    void begin(ofFboMode = OF_FBOMODE_STANDARD) const {
#if defined(SYNAPTOME_OF_STUB_TRACE)
        ofstub::beginTarget(traceId_, allocated_);
#endif
    }

    void end() {
#if defined(SYNAPTOME_OF_STUB_TRACE)
        ofstub::endTarget(traceId_, allocated_);
#endif
    }
    void end() const {
#if defined(SYNAPTOME_OF_STUB_TRACE)
        ofstub::endTarget(traceId_, allocated_);
#endif
    }

    void clear() {
#if defined(SYNAPTOME_OF_STUB_TRACE)
        ofstub::record({
            ofstub::EventKind::Release,
            traceId_,
            ofstub::activeTargetId(),
            {},
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0,
            allocated_,
        });
#endif
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

    void draw(float x, float y, float width, float height) const {
#if defined(SYNAPTOME_OF_STUB_TRACE)
        ofstub::record({
            ofstub::EventKind::Draw,
            traceId_,
            ofstub::activeTargetId(),
            {},
            x,
            y,
            width,
            height,
            0,
            allocated_,
        });
#endif
    }
    void draw(float x, float y) const {
        draw(
            x,
            y,
            static_cast<float>(width_),
            static_cast<float>(height_));
    }

#if defined(SYNAPTOME_OF_STUB_TRACE)
    std::uint64_t stubTraceId() const noexcept {
        return traceId_;
    }
#endif

private:
    void allocateImpl(const Settings& settings) {
#if defined(SYNAPTOME_OF_STUB_TRACE)
        const ofstub::FboSettings tracedSettings{
            settings.width,
            settings.height,
            settings.useDepth,
            settings.useStencil,
            settings.internalformat,
            settings.textureTarget,
            settings.minFilter,
            settings.maxFilter,
            settings.wrapModeHorizontal,
            settings.wrapModeVertical,
        };
        if (ofstub::beginAllocationAttempt()) {
            allocated_ = false;
            width_ = 0;
            height_ = 0;
            texture_.clear();
            ofstub::record({
                ofstub::EventKind::AllocateFailed,
                traceId_,
                ofstub::activeTargetId(),
                tracedSettings,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0,
                false,
            });
            return;
        }
#endif
        width_ = settings.width;
        height_ = settings.height;
        texture_.allocate(width_, height_);
        allocated_ = true;
#if defined(SYNAPTOME_OF_STUB_TRACE)
        ofstub::record({
            ofstub::EventKind::Allocate,
            traceId_,
            ofstub::activeTargetId(),
            tracedSettings,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0,
            true,
        });
#endif
    }

#if defined(SYNAPTOME_OF_STUB_TRACE)
    std::uint64_t traceId_ = ofstub::nextId();
#endif
    int width_ = 0;
    int height_ = 0;
    bool allocated_ = false;
    mutable ofTexture texture_;
};

#endif

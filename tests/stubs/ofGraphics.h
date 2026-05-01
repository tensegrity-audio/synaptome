#pragma once

#ifdef OF_SDK_AVAILABLE
#include <graphics/ofGraphics.h>
#else

#include "ofUtils.h"
#include "ofMesh.h"
#include "ofTexture.h"
#include <limits>
#include <string>

template <typename PixelType>
struct ofColor_ {
    PixelType r = static_cast<PixelType>(255);
    PixelType g = static_cast<PixelType>(255);
    PixelType b = static_cast<PixelType>(255);
    PixelType a = static_cast<PixelType>(255);

    ofColor_() = default;
    explicit ofColor_(PixelType gray, PixelType alpha = static_cast<PixelType>(255))
        : r(gray), g(gray), b(gray), a(alpha) {}
    ofColor_(PixelType red, PixelType green, PixelType blue, PixelType alpha = static_cast<PixelType>(255))
        : r(red), g(green), b(blue), a(alpha) {}

    void limit() {}

    static ofColor_ white;
    static ofColor_ black;
};

template <typename PixelType>
inline ofColor_<PixelType> ofColor_<PixelType>::white =
    ofColor_<PixelType>(static_cast<PixelType>(255),
                        static_cast<PixelType>(255),
                        static_cast<PixelType>(255),
                        static_cast<PixelType>(255));

template <typename PixelType>
inline ofColor_<PixelType> ofColor_<PixelType>::black = ofColor_<PixelType>(static_cast<PixelType>(0));

using ofColor = ofColor_<unsigned char>;

struct ofRectangle {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    ofRectangle() = default;
    ofRectangle(float px, float py, float w, float h) : x(px), y(py), width(w), height(h) {}

    void set(float px, float py, float w, float h) {
        x = px;
        y = py;
        width = w;
        height = h;
    }

    float getWidth() const { return width; }
    float getHeight() const { return height; }
};

inline float ofGetWidth() { return 1920.0f; }
inline float ofGetHeight() { return 1080.0f; }

struct ofStyle {};

inline ofStyle ofGetStyle() { return ofStyle(); }

inline void ofClear(float, float, float, float = 0.0f) {}
inline void ofPushStyle() {}
inline void ofPopStyle() {}
inline void ofPushMatrix() {}
inline void ofPopMatrix() {}
inline void ofPushView() {}
inline void ofPopView() {}
inline void ofTranslate(float, float) {}
inline void ofScale(float, float) {}
inline void ofSetColor(const ofColor&) {}
inline void ofSetColor(int, int, int, int = 255) {}
inline void ofSetColor(int gray) { ofSetColor(gray, gray, gray); }
inline void ofNoFill() {}
inline void ofFill() {}
inline void ofSetLineWidth(float) {}
inline void ofDrawRectangle(float, float, float, float) {}
inline void ofDrawRectRounded(float x, float y, float w, float h, float) { ofDrawRectangle(x, y, w, h); }
inline void ofViewport(float, float, float, float) {}
inline void ofSetupScreenOrtho(float, float, float, float) {}
inline void ofDrawLine(float, float, float, float) {}
inline void ofDrawBitmapString(const std::string&, float, float) {}
inline void ofDrawBitmapStringHighlight(const std::string& text, float x, float y) {
    ofDrawBitmapString(text, x, y);
}

inline void ofDrawBitmapStringHighlight(const std::string& text,
                                        float x,
                                        float y,
                                        const ofColor&,
                                        const ofColor&) {
    ofDrawBitmapString(text, x, y);
}

struct ofBitmapFont {
    const ofTexture& getTexture() const { return texture_; }
    const ofMesh& getMesh(const std::string&, float, float) const { return mesh_; }

    ofRectangle getBoundingBox(const std::string& text, float, float) const {
        float width = static_cast<float>(text.size()) * 8.0f;
        float height = 12.0f;
        return ofRectangle(0.0f, 0.0f, width, height);
    }

private:
    ofTexture texture_;
    ofMesh mesh_;
};

enum ofBlendMode {
    OF_BLENDMODE_ALPHA = 0,
    OF_BLENDMODE_ADD = 1,
};

inline void ofEnableBlendMode(ofBlendMode) {}
inline void ofDisableBlendMode() {}
inline void ofDisableDepthTest() {}
inline void ofEnableDepthTest() {}
inline bool ofIsVFlipped() { return false; }
inline bool ofGetUsingArbTex() { return false; }
inline void ofDisableArbTex() {}
inline void ofEnableArbTex() {}

#endif

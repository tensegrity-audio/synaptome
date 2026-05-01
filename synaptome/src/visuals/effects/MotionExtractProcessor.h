#pragma once

#include "ofFbo.h"
#include "ofShader.h"
#include "ofTexture.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include "ofAppRunner.h"
#include "ofUtils.h"
#include "ofLog.h"
#include <cmath>
#include <array>

class MotionExtractProcessor {
public:
    MotionExtractProcessor(float* thresholdParam,
                           float* boostParam,
                           float* mixParam,
                           float* softnessParam,
                           float* fadeBeatsParam,
                           float* blurParam,
                           float* alphaParam,
                           float* headColorRParam,
                           float* headColorGParam,
                           float* headColorBParam,
                           float* tailColorRParam,
                           float* tailColorGParam,
                           float* tailColorBParam,
                           float* tailOpacityParam,
                           const float* transportBpmParam);

    void resize(int width, int height);
    void apply(const ofFbo& src, ofFbo& dst);
    const ofTexture* processTexture(const ofTexture& texture);
    const ofTexture* compositeTexture() const;
    bool hasTrail() const { return trailReady_; }

private:
    static constexpr int kMaxTrailFrames = 6;

    void ensureHistory(int width, int height);
    void ensureTrail(int width, int height);
    void ensureScratch(int width, int height);
    void ensureOutput(int width, int height);
    void clearTrailBuffers();
    int frameCountForFade(float fadeFrames) const;
    void renderPass(ofFbo& target,
                    ofFbo& trailSample,
                    const ofFbo& src,
                    float threshold,
                    float boost,
                    float mix,
                    float softness,
                    float blur,
                    float alphaMix,
                    float persistence,
                    int mode);
    void blit(const ofFbo& src, ofFbo& dst);
    void copyToHistory(const ofFbo& src);
    void blitTexture(const ofTexture& texture, ofFbo& dst);

    ofShader shader_;
    ofFbo historyFbo_;
    std::array<ofFbo, kMaxTrailFrames> trailFrames_;
    ofFbo trailCompositeFbo_;
    ofFbo scratchFbo_;
    ofFbo outputFbo_;
    int trailIndex_ = 0;
    int availableTrailFrames_ = 0;
    bool historyReady_ = false;
    bool trailReady_ = false;

    float* thresholdParam_ = nullptr;
    float* boostParam_ = nullptr;
    float* mixParam_ = nullptr;
    float* softnessParam_ = nullptr;
    float* fadeBeatsParam_ = nullptr;
    float* blurParam_ = nullptr;
    float* alphaParam_ = nullptr;
    float* headColorRParam_ = nullptr;
    float* headColorGParam_ = nullptr;
    float* headColorBParam_ = nullptr;
    float* tailColorRParam_ = nullptr;
    float* tailColorGParam_ = nullptr;
    float* tailColorBParam_ = nullptr;
    float* tailOpacityParam_ = nullptr;
    const float* transportBpmParam_ = nullptr;
    float lastFadeInput_ = 0.0f;
};

namespace motion_extract_detail {
static const char* kMotionExtractFrag = R"(#version 150
uniform sampler2D tex0;
uniform sampler2D historyTex;
uniform sampler2D trailTex;
uniform vec2 resolution;
uniform float threshold;
uniform float boost;
uniform float mixAmount;
uniform float softness;
uniform float blurStrength;
uniform float alphaWeight;
uniform float trailPersistence;
uniform int mode; // 0 = update trail, 1 = final composite
in vec2 vTexCoord;
out vec4 fragColor;

float luminance(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

float motionEnergy(vec2 uv) {
    vec3 c = texture(tex0, uv).rgb;
    vec3 p = texture(historyTex, uv).rgb;
    vec3 d = abs(c - p);
    return luminance(d) * boost;
}

void main() {
    vec3 current = texture(tex0, vTexCoord).rgb;
    vec3 previous = texture(historyTex, vTexCoord).rgb;
    vec3 diff = abs(current - previous);
    float energy = motionEnergy(vTexCoord);

    if (blurStrength > 0.001) {
        vec2 texel = vec2(1.0) / max(resolution, vec2(1.0));
        float radius = mix(0.0, 2.0, clamp(blurStrength, 0.0, 1.0));
        vec2 offsets[4] = vec2[4](vec2(1.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, 1.0), vec2(0.0, -1.0));
        float accum = energy;
        float weight = 1.0;
        for (int i = 0; i < 4; ++i) {
            vec2 uv = vTexCoord + offsets[i] * texel * radius;
            accum += motionEnergy(uv);
            weight += 1.0;
        }
        float blurred = accum / max(weight, 0.0001);
        energy = mix(energy, blurred, clamp(blurStrength, 0.0, 1.0));
    }

    float edgeWidth = max(softness, 0.001);
    float edge0 = clamp(threshold - edgeWidth * 0.5, 0.0, 1.0);
    float edge1 = clamp(threshold + edgeWidth * 0.5, 0.0, 1.0);
    if (edge1 <= edge0) {
        edge1 = edge0 + 0.0001;
    }
    float mask = smoothstep(edge0, edge1, energy);
    vec3 boosted = clamp(diff * boost, 0.0, 1.0);
    float boostedEnergy = clamp(luminance(boosted), 0.0, 1.0);
    float highlight = clamp(mix(mask, boostedEnergy, 0.5), 0.0, 1.0);

    vec4 trailSample = texture(trailTex, vTexCoord);
    float prevMask = trailSample.a;
    float persistence = clamp(trailPersistence, 0.0, 0.999);

    if (mode == 0) {
        float accumMask = max(mask, prevMask * persistence);
        fragColor = vec4(vec3(highlight), accumMask);
        return;
    }

    vec3 accumulation = trailSample.rgb;
    float trailMask = clamp(prevMask, 0.0, 1.0);
    float mixAmt = clamp(mixAmount, 0.0, 1.0);
    float alphaBlend = clamp(alphaWeight, 0.0, 1.0);
    vec3 result = mix(current, accumulation, mixAmt);
    if (alphaBlend >= 0.999) {
        result = accumulation;
    }
    float outAlpha = mix(1.0, trailMask, alphaBlend);
    fragColor = vec4(result, outAlpha);
}
)";
} // namespace motion_extract_detail

inline MotionExtractProcessor::MotionExtractProcessor(float* thresholdParam,
                                                      float* boostParam,
                                                      float* mixParam,
                                                      float* softnessParam,
                                                      float* fadeBeatsParam,
                                                      float* blurParam,
                                                      float* alphaParam,
                                                      float* headColorRParam,
                                                      float* headColorGParam,
                                                      float* headColorBParam,
                                                      float* tailColorRParam,
                                                      float* tailColorGParam,
                                                      float* tailColorBParam,
                                                      float* tailOpacityParam,
                                                      const float* transportBpmParam)
    : thresholdParam_(thresholdParam)
    , boostParam_(boostParam)
    , mixParam_(mixParam)
    , softnessParam_(softnessParam)
    , fadeBeatsParam_(fadeBeatsParam)
    , blurParam_(blurParam)
    , alphaParam_(alphaParam)
    , headColorRParam_(headColorRParam)
    , headColorGParam_(headColorGParam)
    , headColorBParam_(headColorBParam)
    , tailColorRParam_(tailColorRParam)
    , tailColorGParam_(tailColorGParam)
    , tailColorBParam_(tailColorBParam)
    , tailOpacityParam_(tailOpacityParam)
    , transportBpmParam_(transportBpmParam) {
    shader_.setupShaderFromSource(GL_VERTEX_SHADER, R"(
#version 150
in vec4 position;
in vec2 texcoord;
uniform mat4 modelViewProjectionMatrix;
out vec2 vTexCoord;
void main() {
    vTexCoord = texcoord;
    gl_Position = modelViewProjectionMatrix * position;
}
)");
    shader_.setupShaderFromSource(GL_FRAGMENT_SHADER, motion_extract_detail::kMotionExtractFrag);
    shader_.bindDefaults();
    shader_.linkProgram();
}

inline void MotionExtractProcessor::resize(int width, int height) {
    ensureHistory(width, height);
    ensureTrail(width, height);
    ensureScratch(width, height);
    ensureOutput(width, height);
}

inline void MotionExtractProcessor::apply(const ofFbo& src, ofFbo& dst) {
    ensureHistory(src.getWidth(), src.getHeight());
    ensureTrail(src.getWidth(), src.getHeight());
    if (!historyReady_) {
        blit(src, dst);
        copyToHistory(src);
        clearTrailBuffers();
        trailReady_ = true;
        return;
    }
    if (!trailReady_) {
        clearTrailBuffers();
        trailReady_ = true;
    }

    float threshold = thresholdParam_ ? ofClamp(*thresholdParam_, 0.0f, 1.0f) : 0.15f;
    float boost = boostParam_ ? ofClamp(*boostParam_, 0.0f, 8.0f) : 2.5f;
    float mix = mixParam_ ? ofClamp(*mixParam_, 0.0f, 1.0f) : 0.85f;
    float softness = softnessParam_ ? ofClamp(*softnessParam_, 0.01f, 0.5f) : 0.2f;
    float fadeFrames = fadeBeatsParam_ ? std::max(0.0f, *fadeBeatsParam_) : 0.0f;
    lastFadeInput_ = fadeFrames;
    float blur = blurParam_ ? ofClamp(*blurParam_, 0.0f, 1.0f) : 0.0f;
    float alphaMix = alphaParam_ ? ofClamp(*alphaParam_, 0.0f, 1.0f) : 0.0f;
    const int sampleCount = frameCountForFade(fadeFrames);
    const float headR = headColorRParam_ ? ofClamp(*headColorRParam_, 0.0f, 255.0f) / 255.0f : 0.0f;
    const float headG = headColorGParam_ ? ofClamp(*headColorGParam_, 0.0f, 255.0f) / 255.0f : 0.95f;
    const float headB = headColorBParam_ ? ofClamp(*headColorBParam_, 0.0f, 255.0f) / 255.0f : 0.8f;
    const float tailR = tailColorRParam_ ? ofClamp(*tailColorRParam_, 0.0f, 255.0f) / 255.0f : headR;
    const float tailG = tailColorGParam_ ? ofClamp(*tailColorGParam_, 0.0f, 255.0f) / 255.0f : headG;
    const float tailB = tailColorBParam_ ? ofClamp(*tailColorBParam_, 0.0f, 255.0f) / 255.0f : headB;
    const float tailOpacity = tailOpacityParam_ ? ofClamp(*tailOpacityParam_, 0.0f, 100.0f) / 100.0f : 0.0f;

    static uint64_t lastLogMs = 0;
    uint64_t nowMs = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    if (nowMs - lastLogMs > 1000) {
        lastLogMs = nowMs;
        ofLogNotice("MotionExtractProcessor") << "processTexture historyReady=" << historyReady_
                                              << " trailFrames=" << availableTrailFrames_
                                              << " fadeFrames=" << fadeFrames
                                              << " samples=" << sampleCount
                                              << " alphaMix=" << alphaMix
                                              << " blur=" << blur;
    }

    trailCompositeFbo_.begin();
    ofClear(0, 0, 0, 0);
    trailCompositeFbo_.end();

    ofFbo& highlightTarget = trailFrames_[trailIndex_];
    renderPass(highlightTarget, trailCompositeFbo_, src, threshold, boost, mix, softness, blur, alphaMix, 0.0f, 0);
    trailIndex_ = (trailIndex_ + 1) % kMaxTrailFrames;
    availableTrailFrames_ = std::min(availableTrailFrames_ + 1, kMaxTrailFrames);

    const int framesToUse = std::min(sampleCount, availableTrailFrames_);
    trailCompositeFbo_.begin();
    ofPushView();
    ofViewport(0, 0, trailCompositeFbo_.getWidth(), trailCompositeFbo_.getHeight());
    ofSetupScreenOrtho(trailCompositeFbo_.getWidth(), trailCompositeFbo_.getHeight(), -1, 1);
    ofPushStyle();
    ofClear(0, 0, 0, 0);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (int i = 0; i < framesToUse; ++i) {
        int idx = (trailIndex_ - 1 - i + kMaxTrailFrames) % kMaxTrailFrames;
        float ageMix = framesToUse > 1 ? static_cast<float>(i) / static_cast<float>(framesToUse - 1) : 0.0f;
        float drawR = ofLerp(headR, tailR, ageMix);
        float drawG = ofLerp(headG, tailG, ageMix);
        float drawB = ofLerp(headB, tailB, ageMix);
        float drawAlpha = ofLerp(1.0f, tailOpacity, ageMix);
        ofSetColor(ofClamp(drawR * 255.0f, 0.0f, 255.0f),
                   ofClamp(drawG * 255.0f, 0.0f, 255.0f),
                   ofClamp(drawB * 255.0f, 0.0f, 255.0f),
                   ofClamp(drawAlpha * 255.0f, 0.0f, 255.0f));
        trailFrames_[idx].draw(0, 0,
                               trailCompositeFbo_.getWidth(),
                               trailCompositeFbo_.getHeight());
    }
    ofDisableBlendMode();
    ofPopStyle();
    ofPopView();
    trailCompositeFbo_.end();

    renderPass(dst, trailCompositeFbo_, src, threshold, boost, mix, softness, blur, alphaMix, 0.0f, 1);
    copyToHistory(src);
}

inline const ofTexture* MotionExtractProcessor::processTexture(const ofTexture& texture) {
    if (!texture.isAllocated()) return nullptr;
    const int width = static_cast<int>(texture.getWidth());
    const int height = static_cast<int>(texture.getHeight());
    ensureHistory(width, height);
    ensureTrail(width, height);
    ensureScratch(width, height);
    ensureOutput(width, height);

    blitTexture(texture, scratchFbo_);

    if (!historyReady_) {
        copyToHistory(scratchFbo_);
        clearTrailBuffers();
        trailReady_ = true;
        blit(scratchFbo_, outputFbo_);
        return &outputFbo_.getTexture();
    }
    if (!trailReady_) {
        clearTrailBuffers();
        trailReady_ = true;
    }

    float threshold = thresholdParam_ ? ofClamp(*thresholdParam_, 0.0f, 1.0f) : 0.15f;
    float boost = boostParam_ ? ofClamp(*boostParam_, 0.0f, 8.0f) : 2.5f;
    float mix = mixParam_ ? ofClamp(*mixParam_, 0.0f, 1.0f) : 0.85f;
    float softness = softnessParam_ ? ofClamp(*softnessParam_, 0.01f, 0.5f) : 0.2f;
    float fadeFrames = fadeBeatsParam_ ? std::max(0.0f, *fadeBeatsParam_) : 0.0f;
    lastFadeInput_ = fadeFrames;
    float blur = blurParam_ ? ofClamp(*blurParam_, 0.0f, 1.0f) : 0.0f;
    float alphaMix = alphaParam_ ? ofClamp(*alphaParam_, 0.0f, 1.0f) : 0.0f;
    const int sampleCount = frameCountForFade(fadeFrames);
    const float headR = headColorRParam_ ? ofClamp(*headColorRParam_, 0.0f, 255.0f) / 255.0f : 0.0f;
    const float headG = headColorGParam_ ? ofClamp(*headColorGParam_, 0.0f, 255.0f) / 255.0f : 0.95f;
    const float headB = headColorBParam_ ? ofClamp(*headColorBParam_, 0.0f, 255.0f) / 255.0f : 0.8f;
    const float tailR = tailColorRParam_ ? ofClamp(*tailColorRParam_, 0.0f, 255.0f) / 255.0f : headR;
    const float tailG = tailColorGParam_ ? ofClamp(*tailColorGParam_, 0.0f, 255.0f) / 255.0f : headG;
    const float tailB = tailColorBParam_ ? ofClamp(*tailColorBParam_, 0.0f, 255.0f) / 255.0f : headB;
    const float tailOpacity = tailOpacityParam_ ? ofClamp(*tailOpacityParam_, 0.0f, 100.0f) / 100.0f : 0.0f;

    trailCompositeFbo_.begin();
    ofClear(0, 0, 0, 0);
    trailCompositeFbo_.end();

    ofFbo& highlightTarget = trailFrames_[trailIndex_];
    renderPass(highlightTarget, trailCompositeFbo_, scratchFbo_, threshold, boost, mix, softness, blur, alphaMix, 0.0f, 0);
    trailIndex_ = (trailIndex_ + 1) % kMaxTrailFrames;
    availableTrailFrames_ = std::min(availableTrailFrames_ + 1, kMaxTrailFrames);

    const int framesToUse = std::min(sampleCount, availableTrailFrames_);
    trailCompositeFbo_.begin();
    ofPushView();
    ofViewport(0, 0, trailCompositeFbo_.getWidth(), trailCompositeFbo_.getHeight());
    ofSetupScreenOrtho(trailCompositeFbo_.getWidth(), trailCompositeFbo_.getHeight(), -1, 1);
    ofPushStyle();
    ofClear(0, 0, 0, 0);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for (int i = 0; i < framesToUse; ++i) {
        int idx = (trailIndex_ - 1 - i + kMaxTrailFrames) % kMaxTrailFrames;
        float ageMix = framesToUse > 1 ? static_cast<float>(i) / static_cast<float>(framesToUse - 1) : 0.0f;
        float drawR = ofLerp(headR, tailR, ageMix);
        float drawG = ofLerp(headG, tailG, ageMix);
        float drawB = ofLerp(headB, tailB, ageMix);
        float drawAlpha = ofLerp(1.0f, tailOpacity, ageMix);
        ofSetColor(ofClamp(drawR * 255.0f, 0.0f, 255.0f),
                   ofClamp(drawG * 255.0f, 0.0f, 255.0f),
                   ofClamp(drawB * 255.0f, 0.0f, 255.0f),
                   ofClamp(drawAlpha * 255.0f, 0.0f, 255.0f));
        trailFrames_[idx].draw(0, 0,
                               trailCompositeFbo_.getWidth(),
                               trailCompositeFbo_.getHeight());
    }
    ofDisableBlendMode();
    ofPopStyle();
    ofPopView();
    trailCompositeFbo_.end();

    renderPass(outputFbo_, trailCompositeFbo_, scratchFbo_, threshold, boost, mix, softness, blur, alphaMix, 0.0f, 1);
    copyToHistory(scratchFbo_);
    return &outputFbo_.getTexture();
}

inline const ofTexture* MotionExtractProcessor::compositeTexture() const {
    if (!outputFbo_.isAllocated()) return nullptr;
    return &outputFbo_.getTexture();
}

inline void MotionExtractProcessor::ensureHistory(int width, int height) {
    int w = std::max(1, width);
    int h = std::max(1, height);
    if (historyFbo_.isAllocated() && historyFbo_.getWidth() == w && historyFbo_.getHeight() == h) {
        return;
    }
    ofFbo::Settings settings;
    settings.width = w;
    settings.height = h;
    settings.useDepth = false;
    settings.useStencil = false;
    settings.internalformat = GL_RGBA;
    settings.textureTarget = GL_TEXTURE_2D;
    settings.minFilter = GL_LINEAR;
    settings.maxFilter = GL_LINEAR;
    settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
    historyFbo_.allocate(settings);
    historyFbo_.begin();
    ofClear(0, 0, 0, 255);
    historyFbo_.end();
    historyReady_ = false;
}

inline void MotionExtractProcessor::ensureTrail(int width, int height) {
    int w = std::max(1, width);
    int h = std::max(1, height);
    bool resized = false;
    for (auto& fbo : trailFrames_) {
        if (fbo.isAllocated() && fbo.getWidth() == w && fbo.getHeight() == h) {
            continue;
        }
        ofFbo::Settings settings;
        settings.width = w;
        settings.height = h;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.internalformat = GL_RGBA;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.minFilter = GL_LINEAR;
        settings.maxFilter = GL_LINEAR;
        settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
        settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
        fbo.allocate(settings);
        resized = true;
    }
    if (!trailCompositeFbo_.isAllocated() ||
        trailCompositeFbo_.getWidth() != w ||
        trailCompositeFbo_.getHeight() != h) {
        ofFbo::Settings settings;
        settings.width = w;
        settings.height = h;
        settings.useDepth = false;
        settings.useStencil = false;
        settings.internalformat = GL_RGBA;
        settings.textureTarget = GL_TEXTURE_2D;
        settings.minFilter = GL_LINEAR;
        settings.maxFilter = GL_LINEAR;
        settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
        settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
        trailCompositeFbo_.allocate(settings);
        resized = true;
    }
    if (resized) {
        clearTrailBuffers();
        trailReady_ = false;
    }
}

inline void MotionExtractProcessor::ensureScratch(int width, int height) {
    int w = std::max(1, width);
    int h = std::max(1, height);
    if (scratchFbo_.isAllocated() && scratchFbo_.getWidth() == w && scratchFbo_.getHeight() == h) {
        return;
    }
    ofFbo::Settings settings;
    settings.width = w;
    settings.height = h;
    settings.useDepth = false;
    settings.useStencil = false;
    settings.internalformat = GL_RGBA;
    settings.textureTarget = GL_TEXTURE_2D;
    settings.minFilter = GL_LINEAR;
    settings.maxFilter = GL_LINEAR;
    settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
    scratchFbo_.allocate(settings);
}

inline void MotionExtractProcessor::ensureOutput(int width, int height) {
    int w = std::max(1, width);
    int h = std::max(1, height);
    if (outputFbo_.isAllocated() && outputFbo_.getWidth() == w && outputFbo_.getHeight() == h) {
        return;
    }
    ofFbo::Settings settings;
    settings.width = w;
    settings.height = h;
    settings.useDepth = false;
    settings.useStencil = false;
    settings.internalformat = GL_RGBA;
    settings.textureTarget = GL_TEXTURE_2D;
    settings.minFilter = GL_LINEAR;
    settings.maxFilter = GL_LINEAR;
    settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
    outputFbo_.allocate(settings);
}

inline void MotionExtractProcessor::clearTrailBuffers() {
    for (auto& fbo : trailFrames_) {
        if (!fbo.isAllocated()) continue;
        fbo.begin();
        ofClear(0, 0, 0, 0);
        fbo.end();
    }
    if (trailCompositeFbo_.isAllocated()) {
        trailCompositeFbo_.begin();
        ofClear(0, 0, 0, 0);
        trailCompositeFbo_.end();
    }
    trailIndex_ = 0;
    availableTrailFrames_ = 0;
}

inline int MotionExtractProcessor::frameCountForFade(float fadeFrames) const {
    int samples = static_cast<int>(std::floor(fadeFrames)) + 1;
    samples = ofClamp(samples, 1, kMaxTrailFrames);
    return samples;
}

inline void MotionExtractProcessor::renderPass(ofFbo& target,
                                               ofFbo& trailSample,
                                               const ofFbo& src,
                                               float threshold,
                                               float boost,
                                               float mix,
                                               float softness,
                                               float blur,
                                               float alphaMix,
                                               float persistence,
                                               int mode) {
    target.begin();
    bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
    if (scissorWasEnabled) {
        glDisable(GL_SCISSOR_TEST);
    }
    ofPushView();
    ofViewport(0, 0, target.getWidth(), target.getHeight());
    ofSetupScreenOrtho(target.getWidth(), target.getHeight(), -1, 1);
    ofPushStyle();
    ofClear(0, 0, 0, 0);
    if (depthWasEnabled) {
        ofDisableDepthTest();
    }
    ofSetColor(255);
    shader_.begin();
    shader_.setUniformTexture("tex0", src.getTexture(), 0);
    shader_.setUniformTexture("historyTex", historyFbo_.getTexture(), 1);
    shader_.setUniformTexture("trailTex", trailSample.getTexture(), 2);
    shader_.setUniform2f("resolution", src.getWidth(), src.getHeight());
    shader_.setUniform1f("threshold", threshold);
    shader_.setUniform1f("boost", boost);
    shader_.setUniform1f("mixAmount", mix);
    shader_.setUniform1f("softness", softness);
    shader_.setUniform1f("blurStrength", blur);
    shader_.setUniform1f("alphaWeight", alphaMix);
    shader_.setUniform1f("trailPersistence", persistence);
    shader_.setUniform1i("mode", mode);
    src.draw(0, 0, target.getWidth(), target.getHeight());
    shader_.end();
    ofPopStyle();
    ofPopView();
    if (depthWasEnabled) {
        ofEnableDepthTest();
    }
    if (scissorWasEnabled) {
        glEnable(GL_SCISSOR_TEST);
    }
    target.end();
}

inline void MotionExtractProcessor::blit(const ofFbo& src, ofFbo& dst) {
    dst.begin();
    bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
    if (scissorWasEnabled) {
        glDisable(GL_SCISSOR_TEST);
    }
    ofPushView();
    ofViewport(0, 0, dst.getWidth(), dst.getHeight());
    ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
    ofPushStyle();
    ofClear(0, 0, 0, 0);
    if (depthWasEnabled) {
        ofDisableDepthTest();
    }
    ofSetColor(255);
    src.draw(0, 0, dst.getWidth(), dst.getHeight());
    ofPopStyle();
    ofPopView();
    if (depthWasEnabled) {
        ofEnableDepthTest();
    }
    if (scissorWasEnabled) {
        glEnable(GL_SCISSOR_TEST);
    }
    dst.end();
}

inline void MotionExtractProcessor::copyToHistory(const ofFbo& src) {
    blit(src, historyFbo_);
    historyReady_ = true;
}

inline void MotionExtractProcessor::blitTexture(const ofTexture& texture, ofFbo& dst) {
    dst.begin();
    bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
    if (scissorWasEnabled) {
        glDisable(GL_SCISSOR_TEST);
    }
    ofPushView();
    ofViewport(0, 0, dst.getWidth(), dst.getHeight());
    ofSetupScreenOrtho(dst.getWidth(), dst.getHeight(), -1, 1);
    ofPushStyle();
    ofClear(0, 0, 0, 0);
    if (depthWasEnabled) {
        ofDisableDepthTest();
    }
    ofSetColor(255);
    texture.draw(0, 0, dst.getWidth(), dst.getHeight());
    ofPopStyle();
    ofPopView();
    if (depthWasEnabled) {
        ofEnableDepthTest();
    }
    if (scissorWasEnabled) {
        glEnable(GL_SCISSOR_TEST);
    }
    dst.end();
}

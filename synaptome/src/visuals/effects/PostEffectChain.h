#pragma once

#include "MotionExtractProcessor.h"
#include "ofFbo.h"
#include "ofTexture.h"
#include "../../core/ParameterRegistry.h"
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct AsciiSupersampleAtlas {
    struct FontAtlas {
        std::string label;
        ofTexture texture;
        std::array<glm::vec4, 95> rects{};
        std::array<float, 95 * 6> descriptors{};
        std::array<float, 95> descriptorMeans{};
        bool ready = false;
    };

    bool ready = false;

    std::array<std::vector<int>, 4> glyphSets_{};

    bool build();
    int fontCount() const;
    int maxFontIndex() const;
    const FontAtlas* font(int index) const;
    const std::vector<int>& glyphSet(int index) const;

private:
    std::vector<std::shared_ptr<FontAtlas>> fonts_;
};

class PostEffectChain {
public:
    void setup(ParameterRegistry& registry);
    void resize(int width, int height);
    void applyConsole(ofFbo& fbo);
    void applyGlobal(ofFbo& fbo);
    void prepareLayerBuffers(int layerCount, int width, int height);
    void applyDither(const ofFbo& src, ofFbo& dst);
    void applyAscii(const ofFbo& src, ofFbo& dst);
    void applyAsciiSupersample(const ofFbo& src, ofFbo& dst);
    void applyCrt(const ofFbo& src, ofFbo& dst);
    void applyMotionExtract(const ofFbo& src, ofFbo& dst);

public:
    struct Effect {
        virtual ~Effect() = default;
        virtual void resize(int width, int height) {}
        virtual void apply(const ofFbo& src, ofFbo& dst) = 0;
    };

private:
    enum class Route : int {
        Off = 0,
        Console = 1,
        Global = 2
    };

    void ensureBuffers(int width, int height);
    void process(ofFbo& fbo, const std::vector<Effect*>& effects);
    Route routeFromValue(float value) const;

    std::unique_ptr<Effect> ditherEffect_;
    std::unique_ptr<Effect> asciiEffect_;
    std::unique_ptr<Effect> asciiSupersampleEffect_;
    std::unique_ptr<Effect> crtEffect_;
    std::unique_ptr<Effect> motionEffect_;
    std::unique_ptr<MotionExtractProcessor> motionProcessor_;
    std::unique_ptr<AsciiSupersampleAtlas> asciiSupersampleAtlas_;

    float ditherRoute_ = 0.0f;
    float ditherCoverage_ = 0.0f;
    bool ditherCoverageMask_ = false;
    float asciiRoute_ = 0.0f;
    float asciiBlockSize_ = 8.0f;
    float ditherCellSize_ = 3.0f;
    float ditherMode_ = 1.0f; // 0=2x2, 1=4x4, 2=8x8
    float asciiColorMode_ = 0.0f;
    float asciiCharacterSet_ = 0.0f;
    float asciiAspectMode_ = 0.0f;
    float asciiPadding_ = 0.0f;
    float asciiGamma_ = 1.0f;
    float asciiJitter_ = 0.0f;
    float asciiCoverage_ = 0.0f;
    bool asciiCoverageMask_ = false;
    glm::vec3 asciiGreenTint_{ 0.3f, 1.0f, 0.3f };
    float asciiSupersampleRoute_ = 0.0f;
    float asciiSupersampleBlockSize_ = 8.0f;
    float asciiSupersampleColorMode_ = 0.0f;
    float asciiSupersampleCharacterSet_ = 0.0f;
    float asciiSupersampleGamma_ = 1.0f;
    float asciiSupersampleJitter_ = 0.0f;
    float asciiSupersampleFontIndex_ = 0.0f;
    float asciiSupersampleDebugMode_ = 0.0f;
    float asciiSupersampleCoverage_ = 0.0f;
    bool asciiSupersampleCoverageMask_ = false;
    glm::vec3 asciiSupersampleGreenTint_{ 0.3f, 1.0f, 0.3f };
    float crtRoute_ = 0.0f;
    float crtScanlineIntensity_ = 0.4f;
    float crtVignetteIntensity_ = 0.25f;
    float crtBleed_ = 0.15f;
    float crtSoftness_ = 0.0f;
    float crtGlow_ = 0.0f;
    float crtPerChannelOffset_ = 0.0f;
    float crtScanlineJitter_ = 0.0f;
    float crtSubpixelDensity_ = 0.0f;
    float crtSubpixelAspect_ = 2.0f;
    float crtSubpixelPadding_ = 0.0f;
    float crtRgbMisalignment_ = 0.0f;
    float crtSyncTear_ = 0.0f;
    float crtTrackingWobble_ = 0.0f;
    float crtLumaNoise_ = 0.0f;
    float crtHeadSwitchFlicker_ = 0.0f;
    float crtCoverage_ = 0.0f;
    bool crtCoverageMask_ = true;
    float motionRoute_ = 0.0f;
    float motionThreshold_ = 0.15f;
    float motionBoost_ = 2.5f;
    float motionMix_ = 0.85f;
    float motionSoftness_ = 0.2f;
    float motionFadeBeats_ = 0.0f;
    float motionBlurStrength_ = 0.0f;
    float motionAlphaWeight_ = 0.0f;
    float motionHeadColorR_ = 0.0f;
    float motionHeadColorG_ = 242.0f;
    float motionHeadColorB_ = 204.0f;
    float motionTailColorR_ = 0.0f;
    float motionTailColorG_ = 242.0f;
    float motionTailColorB_ = 204.0f;
    float motionTailOpacity_ = 0.0f;
    float motionCoverage_ = 0.0f;
    bool motionCoverageMask_ = true;

    ofFbo pingFbo_;
    ofFbo pongFbo_;
    int bufferWidth_ = 0;
    int bufferHeight_ = 0;

    ParameterRegistry* registry_ = nullptr;
    std::vector<ofFbo> layerBuffers_;
    int layerBufferWidth_ = 0;
    int layerBufferHeight_ = 0;

public:
    struct CoverageWindow {
        int effectColumn = 0;
        int firstColumn = 1;
        int lastColumn = 0;
        int requestedColumns = 0;
        bool includesAll = true;
    };

public:
    float* ditherRouteParamPtr() { return &ditherRoute_; }
    const float* ditherRouteParamPtr() const { return &ditherRoute_; }
    float* ditherCoverageParamPtr() { return &ditherCoverage_; }
    const float* ditherCoverageParamPtr() const { return &ditherCoverage_; }
    float* asciiRouteParamPtr() { return &asciiRoute_; }
    const float* asciiRouteParamPtr() const { return &asciiRoute_; }
    float* asciiCoverageParamPtr() { return &asciiCoverage_; }
    const float* asciiCoverageParamPtr() const { return &asciiCoverage_; }
    float* asciiBlockParamPtr() { return &asciiBlockSize_; }
    const float* asciiBlockParamPtr() const { return &asciiBlockSize_; }
    float* asciiSupersampleRouteParamPtr() { return &asciiSupersampleRoute_; }
    const float* asciiSupersampleRouteParamPtr() const { return &asciiSupersampleRoute_; }
    float* asciiSupersampleCoverageParamPtr() { return &asciiSupersampleCoverage_; }
    const float* asciiSupersampleCoverageParamPtr() const { return &asciiSupersampleCoverage_; }
    float* asciiSupersampleBlockParamPtr() { return &asciiSupersampleBlockSize_; }
    const float* asciiSupersampleBlockParamPtr() const { return &asciiSupersampleBlockSize_; }
    float* crtRouteParamPtr() { return &crtRoute_; }
    const float* crtRouteParamPtr() const { return &crtRoute_; }
    float* crtCoverageParamPtr() { return &crtCoverage_; }
    const float* crtCoverageParamPtr() const { return &crtCoverage_; }
    float* crtScanlineParamPtr() { return &crtScanlineIntensity_; }
    const float* crtScanlineParamPtr() const { return &crtScanlineIntensity_; }
    float* crtVignetteParamPtr() { return &crtVignetteIntensity_; }
    const float* crtVignetteParamPtr() const { return &crtVignetteIntensity_; }
    float* crtBleedParamPtr() { return &crtBleed_; }
    const float* crtBleedParamPtr() const { return &crtBleed_; }
    float* crtSoftnessParamPtr() { return &crtSoftness_; }
    float* crtGlowParamPtr() { return &crtGlow_; }
    float* crtPerChannelOffsetParamPtr() { return &crtPerChannelOffset_; }
    float* crtScanlineJitterParamPtr() { return &crtScanlineJitter_; }
    float* crtSubpixelDensityParamPtr() { return &crtSubpixelDensity_; }
    float* crtSubpixelAspectParamPtr() { return &crtSubpixelAspect_; }
    float* crtSubpixelPaddingParamPtr() { return &crtSubpixelPadding_; }
    float* crtRgbMisalignmentParamPtr() { return &crtRgbMisalignment_; }
    float* crtSyncTearParamPtr() { return &crtSyncTear_; }
    float* crtTrackingWobbleParamPtr() { return &crtTrackingWobble_; }
    float* crtLumaNoiseParamPtr() { return &crtLumaNoise_; }
    float* crtHeadSwitchFlickerParamPtr() { return &crtHeadSwitchFlicker_; }
    float* motionRouteParamPtr() { return &motionRoute_; }
    const float* motionRouteParamPtr() const { return &motionRoute_; }
    float* motionCoverageParamPtr() { return &motionCoverage_; }
    const float* motionCoverageParamPtr() const { return &motionCoverage_; }

public:
    float* ditherCellSizeParamPtr() { return &ditherCellSize_; }
    const float* ditherCellSizeParamPtr() const { return &ditherCellSize_; }
    float* ditherModeParamPtr() { return &ditherMode_; }
    const float* ditherModeParamPtr() const { return &ditherMode_; }
    float* asciiColorModeParamPtr() { return &asciiColorMode_; }
    const float* asciiColorModeParamPtr() const { return &asciiColorMode_; }
    float* asciiCharacterSetParamPtr() { return &asciiCharacterSet_; }
    const float* asciiCharacterSetParamPtr() const { return &asciiCharacterSet_; }
    float* asciiAspectModeParamPtr() { return &asciiAspectMode_; }
    const float* asciiAspectModeParamPtr() const { return &asciiAspectMode_; }
    float* asciiPaddingParamPtr() { return &asciiPadding_; }
    const float* asciiPaddingParamPtr() const { return &asciiPadding_; }
    float* asciiGammaParamPtr() { return &asciiGamma_; }
    const float* asciiGammaParamPtr() const { return &asciiGamma_; }
    float* asciiJitterParamPtr() { return &asciiJitter_; }
    const float* asciiJitterParamPtr() const { return &asciiJitter_; }
    float* asciiSupersampleColorModeParamPtr() { return &asciiSupersampleColorMode_; }
    const float* asciiSupersampleColorModeParamPtr() const { return &asciiSupersampleColorMode_; }
    float* asciiSupersampleCharacterSetParamPtr() { return &asciiSupersampleCharacterSet_; }
    const float* asciiSupersampleCharacterSetParamPtr() const { return &asciiSupersampleCharacterSet_; }
    float* asciiSupersampleGammaParamPtr() { return &asciiSupersampleGamma_; }
    const float* asciiSupersampleGammaParamPtr() const { return &asciiSupersampleGamma_; }
    float* asciiSupersampleJitterParamPtr() { return &asciiSupersampleJitter_; }
    const float* asciiSupersampleJitterParamPtr() const { return &asciiSupersampleJitter_; }
    float* asciiSupersampleFontParamPtr() { return &asciiSupersampleFontIndex_; }
    const float* asciiSupersampleFontParamPtr() const { return &asciiSupersampleFontIndex_; }
    float* asciiSupersampleDebugModeParamPtr() { return &asciiSupersampleDebugMode_; }
    const float* asciiSupersampleDebugModeParamPtr() const { return &asciiSupersampleDebugMode_; }
    float* motionThresholdParamPtr() { return &motionThreshold_; }
    const float* motionThresholdParamPtr() const { return &motionThreshold_; }
    float* motionBoostParamPtr() { return &motionBoost_; }
    const float* motionBoostParamPtr() const { return &motionBoost_; }
    float* motionMixParamPtr() { return &motionMix_; }
    const float* motionMixParamPtr() const { return &motionMix_; }
    float* motionSoftnessParamPtr() { return &motionSoftness_; }
    const float* motionSoftnessParamPtr() const { return &motionSoftness_; }
    float ditherRouteValue() const { return ditherRoute_; }
    float asciiRouteValue() const { return asciiRoute_; }
    float asciiBlockSizeValue() const { return asciiBlockSize_; }
    float ditherCellSizeValue() const { return ditherCellSize_; }
    float asciiColorModeValue() const { return asciiColorMode_; }
    float asciiCharacterSetValue() const { return asciiCharacterSet_; }
    float asciiAspectModeValue() const { return asciiAspectMode_; }
    float asciiPaddingValue() const { return asciiPadding_; }
    float asciiGammaValue() const { return asciiGamma_; }
    float asciiJitterValue() const { return asciiJitter_; }
    float asciiCoverageValue() const { return asciiCoverage_; }
    float ditherCoverageValue() const { return ditherCoverage_; }
    float crtRouteValue() const { return crtRoute_; }
    float crtScanlineValue() const { return crtScanlineIntensity_; }
    float crtVignetteValue() const { return crtVignetteIntensity_; }
    float crtBleedValue() const { return crtBleed_; }
    float crtCoverageValue() const { return crtCoverage_; }
    float motionRouteValue() const { return motionRoute_; }
    float motionThresholdValue() const { return motionThreshold_; }
    float motionBoostValue() const { return motionBoost_; }
    float motionMixValue() const { return motionMix_; }
    float motionSoftnessValue() const { return motionSoftness_; }
    float motionCoverageValue() const { return motionCoverage_; }
    bool coverageMaskEnabled(const std::string& effectType) const;
    float defaultCoverageForType(const std::string& effectType) const;
    MotionExtractProcessor* motionProcessor() { return motionProcessor_.get(); }
    const MotionExtractProcessor* motionProcessor() const { return motionProcessor_.get(); }

    CoverageWindow resolveCoverageWindow(int effectColumnIndex, float coverageParamValue) const;
    const std::vector<ofFbo>& layerBuffers() const { return layerBuffers_; }
    ofFbo* layerBufferPtr(int index);
};

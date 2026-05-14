#include "AudioWaveformLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
    constexpr float kGainMin = 0.1f;
    constexpr float kGainMax = 12.0f;
    constexpr float kVerticalScaleMin = 0.1f;
    constexpr float kVerticalScaleMax = 1.5f;
    constexpr float kThicknessMin = 1.0f;
    constexpr float kThicknessMax = 16.0f;
    constexpr float kAlphaMin = 0.0f;
    constexpr float kAlphaMax = 1.0f;
    constexpr float kSmoothingMin = 0.0f;
    constexpr float kSmoothingMax = 0.95f;
    constexpr float kBandHeightMin = 0.0f;
    constexpr float kBandHeightMax = 0.35f;
    constexpr float kColorMin = 0.0f;
    constexpr float kColorMax = 1.0f;

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, kSmoothingMin, kSmoothingMax);
    }

    ofFloatColor colorFromParams(float r, float g, float b, float alpha) {
        return ofFloatColor(ofClamp(r, 0.0f, 1.0f),
                            ofClamp(g, 0.0f, 1.0f),
                            ofClamp(b, 0.0f, 1.0f),
                            ofClamp(alpha, 0.0f, 1.0f));
    }
}

void AudioWaveformLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramShowBands_ = def.value("showBands", paramShowBands_);
    paramGain_ = def.value("gain", paramGain_);
    paramVerticalScale_ = def.value("verticalScale", paramVerticalScale_);
    paramLineThickness_ = def.value("lineThickness", paramLineThickness_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramSmoothing_ = def.value("smoothing", paramSmoothing_);
    paramBandHeight_ = def.value("bandHeight", paramBandHeight_);
    paramBandAlpha_ = def.value("bandAlpha", paramBandAlpha_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    if (def.contains("color") && def["color"].is_array() && def["color"].size() >= 3) {
        paramColorR_ = def["color"][0].get<float>();
        paramColorG_ = def["color"][1].get<float>();
        paramColorB_ = def["color"][2].get<float>();
    }
    if (def.contains("backgroundColor") && def["backgroundColor"].is_array() && def["backgroundColor"].size() >= 3) {
        paramBgColorR_ = def["backgroundColor"][0].get<float>();
        paramBgColorG_ = def["backgroundColor"][1].get<float>();
        paramBgColorB_ = def["backgroundColor"][2].get<float>();
    }
    clampParams();
}

void AudioWaveformLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "sensors.audio.waveform" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Audio Waveform";

    meta.label = "Waveform Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "Waveform Show Bands";
    registry.addBool(prefix + ".showBands", &paramShowBands_, paramShowBands_, meta);

    registerFloat(registry, prefix + ".gain", &paramGain_, paramGain_, "Waveform Gain", kGainMin, kGainMax, 0.01f);
    registerFloat(registry, prefix + ".verticalScale", &paramVerticalScale_, paramVerticalScale_, "Waveform Scale", kVerticalScaleMin, kVerticalScaleMax, 0.01f);
    registerFloat(registry, prefix + ".lineThickness", &paramLineThickness_, paramLineThickness_, "Waveform Thickness", kThicknessMin, kThicknessMax, 0.1f, "px");
    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Waveform Alpha", kAlphaMin, kAlphaMax, 0.01f, "normalized");
    registerFloat(registry, prefix + ".smoothing", &paramSmoothing_, paramSmoothing_, "Waveform Smoothing", kSmoothingMin, kSmoothingMax, 0.01f, "normalized");
    registerFloat(registry, prefix + ".bandHeight", &paramBandHeight_, paramBandHeight_, "Waveform Band Height", kBandHeightMin, kBandHeightMax, 0.01f, "normalized");
    registerFloat(registry, prefix + ".bandAlpha", &paramBandAlpha_, paramBandAlpha_, "Waveform Band Alpha", kAlphaMin, kAlphaMax, 0.01f, "normalized");
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Waveform BG Alpha", kAlphaMin, kAlphaMax, 0.01f, "normalized");
    registerFloat(registry, prefix + ".colorR", &paramColorR_, paramColorR_, "Waveform Red", kColorMin, kColorMax, 0.01f);
    registerFloat(registry, prefix + ".colorG", &paramColorG_, paramColorG_, "Waveform Green", kColorMin, kColorMax, 0.01f);
    registerFloat(registry, prefix + ".colorB", &paramColorB_, paramColorB_, "Waveform Blue", kColorMin, kColorMax, 0.01f);
    registerFloat(registry, prefix + ".bgColorR", &paramBgColorR_, paramBgColorR_, "Waveform BG Red", kColorMin, kColorMax, 0.01f);
    registerFloat(registry, prefix + ".bgColorG", &paramBgColorG_, paramBgColorG_, "Waveform BG Green", kColorMin, kColorMax, 0.01f);
    registerFloat(registry, prefix + ".bgColorB", &paramBgColorB_, paramBgColorB_, "Waveform BG Blue", kColorMin, kColorMax, 0.01f);
}

void AudioWaveformLayer::update(const LayerUpdateParams& params) {
    (void)params;
    enabled_ = paramEnabled_;
    clampParams();
    if (!enabled_) {
        return;
    }

    auto snapshot = AudioAnalysisBus::instance().snapshot();
    if (!snapshot.valid || snapshot.waveform.empty() || snapshot.frame == lastFrame_) {
        return;
    }

    const float follow = followAmount(paramSmoothing_);
    if (!hasSample_ || waveform_.size() != snapshot.waveform.size()) {
        waveform_ = snapshot.waveform;
        level_ = snapshot.level;
        peak_ = snapshot.peak;
        bass_ = snapshot.bass;
        mids_ = snapshot.mids;
        highs_ = snapshot.highs;
        hasSample_ = true;
    } else {
        for (std::size_t i = 0; i < waveform_.size(); ++i) {
            waveform_[i] = ofLerp(waveform_[i], snapshot.waveform[i], follow);
        }
        level_ = ofLerp(level_, snapshot.level, follow);
        peak_ = ofLerp(peak_, snapshot.peak, follow);
        bass_ = ofLerp(bass_, snapshot.bass, follow);
        mids_ = ofLerp(mids_, snapshot.mids, follow);
        highs_ = ofLerp(highs_, snapshot.highs, follow);
    }
    lastFrame_ = snapshot.frame;
}

void AudioWaveformLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f || waveform_.empty()) {
        return;
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    const ofFloatColor traceColor = colorFromParams(paramColorR_, paramColorG_, paramColorB_, alpha);
    const ofFloatColor bgColor = colorFromParams(paramBgColorR_, paramBgColorG_, paramBgColorB_, paramBgAlpha_ * params.slotOpacity);

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    if (bgColor.a > 0.0f) {
        ofSetColor(static_cast<int>(bgColor.r * 255.0f),
                   static_cast<int>(bgColor.g * 255.0f),
                   static_cast<int>(bgColor.b * 255.0f),
                   static_cast<int>(bgColor.a * 255.0f));
        ofDrawRectangle(0.0f, 0.0f, width, height);
    }

    const float padX = width * 0.045f;
    const float scopeWidth = std::max(1.0f, width - padX * 2.0f);
    const float centerY = height * 0.5f;
    const float amplitude = height * 0.42f * paramVerticalScale_;

    ofSetColor(static_cast<int>(traceColor.r * 255.0f),
               static_cast<int>(traceColor.g * 255.0f),
               static_cast<int>(traceColor.b * 255.0f),
               static_cast<int>(alpha * 64.0f));
    ofDrawLine(padX, centerY, width - padX, centerY);

    ofMesh trace;
    trace.setMode(OF_PRIMITIVE_LINE_STRIP);
    const float denom = static_cast<float>(std::max<std::size_t>(1, waveform_.size() - 1));
    const float peakLift = ofClamp(0.55f + peak_ * 0.65f, 0.55f, 1.0f);
    for (std::size_t i = 0; i < waveform_.size(); ++i) {
        const float x = padX + (static_cast<float>(i) / denom) * scopeWidth;
        const float sample = ofClamp(waveform_[i] * paramGain_, -1.25f, 1.25f);
        const float y = centerY - sample * amplitude;
        trace.addVertex(glm::vec3(x, y, 0.0f));
        trace.addColor(ofFloatColor(traceColor.r,
                                    traceColor.g,
                                    traceColor.b,
                                    ofClamp(alpha * peakLift, 0.0f, 1.0f)));
    }

#ifndef TARGET_OPENGLES
    glLineWidth(ofClamp(paramLineThickness_, kThicknessMin, kThicknessMax));
#endif
    trace.draw();

    if (paramShowBands_ && paramBandHeight_ > 0.0f && paramBandAlpha_ > 0.0f) {
        const float bandH = height * paramBandHeight_;
        const float baseY = height - bandH;
        const float laneW = width / 3.0f;
        const std::array<float, 3> values = { bass_, mids_, highs_ };
        const std::array<ofFloatColor, 3> colors = {
            ofFloatColor(1.0f, 0.44f, 0.16f, paramBandAlpha_ * params.slotOpacity),
            ofFloatColor(0.2f, 0.92f, 0.55f, paramBandAlpha_ * params.slotOpacity),
            ofFloatColor(0.23f, 0.62f, 1.0f, paramBandAlpha_ * params.slotOpacity)
        };
        for (std::size_t i = 0; i < values.size(); ++i) {
            float value = ofClamp(values[i], 0.0f, 1.0f);
            float meterH = bandH * value;
            ofSetColor(static_cast<int>(colors[i].r * 255.0f),
                       static_cast<int>(colors[i].g * 255.0f),
                       static_cast<int>(colors[i].b * 255.0f),
                       static_cast<int>(colors[i].a * 255.0f));
            ofDrawRectangle(static_cast<float>(i) * laneW,
                            baseY + bandH - meterH,
                            laneW,
                            meterH);
        }
    }

    ofPopView();
    ofPopStyle();
}

void AudioWaveformLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void AudioWaveformLayer::clampParams() {
    paramGain_ = ofClamp(paramGain_, kGainMin, kGainMax);
    paramVerticalScale_ = ofClamp(paramVerticalScale_, kVerticalScaleMin, kVerticalScaleMax);
    paramLineThickness_ = ofClamp(paramLineThickness_, kThicknessMin, kThicknessMax);
    paramAlpha_ = ofClamp(paramAlpha_, kAlphaMin, kAlphaMax);
    paramSmoothing_ = ofClamp(paramSmoothing_, kSmoothingMin, kSmoothingMax);
    paramBandHeight_ = ofClamp(paramBandHeight_, kBandHeightMin, kBandHeightMax);
    paramBandAlpha_ = ofClamp(paramBandAlpha_, kAlphaMin, kAlphaMax);
    paramBgAlpha_ = ofClamp(paramBgAlpha_, kAlphaMin, kAlphaMax);
    paramColorR_ = ofClamp(paramColorR_, kColorMin, kColorMax);
    paramColorG_ = ofClamp(paramColorG_, kColorMin, kColorMax);
    paramColorB_ = ofClamp(paramColorB_, kColorMin, kColorMax);
    paramBgColorR_ = ofClamp(paramBgColorR_, kColorMin, kColorMax);
    paramBgColorG_ = ofClamp(paramBgColorG_, kColorMin, kColorMax);
    paramBgColorB_ = ofClamp(paramBgColorB_, kColorMin, kColorMax);
}

void AudioWaveformLayer::registerFloat(ParameterRegistry& registry,
                                       const std::string& id,
                                       float* target,
                                       float initial,
                                       const std::string& label,
                                       float min,
                                       float max,
                                       float step,
                                       const std::string& units) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Audio Waveform";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.units = units;
    registry.addFloat(id, target, initial, meta);
}

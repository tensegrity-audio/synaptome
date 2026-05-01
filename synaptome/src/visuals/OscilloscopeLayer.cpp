#include "OscilloscopeLayer.h"

#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float kPatternMin = 0.0f;
    constexpr float kPatternMax = 3.0f;
    constexpr float kModModeMin = 0.0f;
    constexpr float kModModeMax = 6.0f;
    constexpr float kInputMin = -1.0f;
    constexpr float kInputMax = 1.0f;
    constexpr float kAmountMin = 0.0f;
    constexpr float kAmountMax = 2.0f;
    constexpr float kSignedAmountMin = -2.0f;
    constexpr float kSignedAmountMax = 2.0f;
    constexpr float kAmplitudeMin = 0.0f;
    constexpr float kAmplitudeMax = 1.5f;
    constexpr float kSpeedMin = 0.0f;
    constexpr float kSpeedMax = 4.0f;
    constexpr float kFreqMin = 0.05f;
    constexpr float kFreqMax = 12.0f;
    constexpr float kMorphMin = 0.0f;
    constexpr float kMorphMax = 1.0f;
    constexpr float kGridDivMin = 1.0f;
    constexpr float kGridDivMax = 12.0f;
    constexpr float kGridAlphaMin = 0.0f;
    constexpr float kGridAlphaMax = 1.0f;
    constexpr float kGlowAlphaMin = 0.0f;
    constexpr float kGlowAlphaMax = 1.0f;
    constexpr float kGlowRadiusMin = 0.2f;
    constexpr float kGlowRadiusMax = 2.0f;
    constexpr float kGlowFalloffMin = 0.05f;
    constexpr float kGlowFalloffMax = 1.0f;
    constexpr float kScaleMin = 0.0f;
    constexpr float kScaleMax = 4.0f;
    constexpr float kBiasMin = -1.0f;
    constexpr float kBiasMax = 1.0f;
    constexpr float kHistoryMin = 16.0f;
    constexpr float kHistoryMax = 4096.0f;
    constexpr float kDensityMin = 1.0f;
    constexpr float kDensityMax = 32.0f;
    constexpr float kThicknessMin = 1.0f;
    constexpr float kThicknessMax = 24.0f;
    constexpr float kAlphaMin = 0.0f;
    constexpr float kAlphaMax = 1.0f;
    constexpr float kDecayMin = 0.05f;
    constexpr float kDecayMax = 1.0f;
    constexpr float kIntensityMin = 0.0f;
    constexpr float kIntensityMax = 2.0f;
    constexpr float kPointSizeMin = 0.0f;
    constexpr float kPointSizeMax = 12.0f;
    constexpr float kColorMin = 0.0f;
    constexpr float kColorMax = 1.5f;
}

void OscilloscopeLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramPattern_ = def.value("pattern", paramPattern_);
    paramModMode_ = def.value("modMode", paramModMode_);
    paramXInput_ = def.value("xInput", paramXInput_);
    paramYInput_ = def.value("yInput", paramYInput_);
    paramSpeedInput_ = def.value("speedInput", paramSpeedInput_);
    paramBaseAmount_ = def.value("baseAmount", paramBaseAmount_);
    paramModAmount_ = def.value("modAmount", paramModAmount_);
    paramRadialAmount_ = def.value("radialAmount", paramRadialAmount_);
    paramWiggleAmount_ = def.value("wiggleAmount", paramWiggleAmount_);
    paramAmplitude_ = def.value("amplitude", paramAmplitude_);
    paramSpeed_ = def.value("speed", paramSpeed_);
    paramSpeedModAmount_ = def.value("speedModAmount", paramSpeedModAmount_);
    paramFreqX_ = def.value("freqX", paramFreqX_);
    paramFreqY_ = def.value("freqY", paramFreqY_);
    paramPhaseOffsetDeg_ = def.value("phaseOffsetDeg", paramPhaseOffsetDeg_);
    paramMorph_ = def.value("morph", paramMorph_);
    paramShowGrid_ = def.value("showGrid", paramShowGrid_);
    paramShowCrosshair_ = def.value("showCrosshair", paramShowCrosshair_);
    paramShowGlow_ = def.value("showGlow", paramShowGlow_);
    paramGridDivisions_ = def.value("gridDivisions", paramGridDivisions_);
    paramGridAlpha_ = def.value("gridAlpha", paramGridAlpha_);
    paramGlowAlpha_ = def.value("glowAlpha", paramGlowAlpha_);
    paramGlowRadius_ = def.value("glowRadius", paramGlowRadius_);
    paramGlowFalloff_ = def.value("glowFalloff", paramGlowFalloff_);
    paramXScale_ = def.value("xScale", paramXScale_);
    paramYScale_ = def.value("yScale", paramYScale_);
    paramXBias_ = def.value("xBias", paramXBias_);
    paramYBias_ = def.value("yBias", paramYBias_);
    paramRotationDeg_ = def.value("rotationDeg", paramRotationDeg_);
    paramHistorySize_ = def.value("historySize", paramHistorySize_);
    paramSampleDensity_ = def.value("sampleDensity", paramSampleDensity_);
    paramThickness_ = def.value("thickness", paramThickness_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramDecay_ = def.value("decay", paramDecay_);
    paramIntensity_ = def.value("intensity", paramIntensity_);
    paramPointSize_ = def.value("pointSize", paramPointSize_);
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

void OscilloscopeLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.oscilloscope" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Oscilloscope";

    meta.label = "Scope Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "Scope Show Grid";
    meta.description = "Toggle oscilloscope-style XY coordinate grid.";
    registry.addBool(prefix + ".showGrid", &paramShowGrid_, paramShowGrid_, meta);

    meta.label = "Scope Show Crosshair";
    meta.description = "Toggle center horizontal/vertical axes.";
    registry.addBool(prefix + ".showCrosshair", &paramShowCrosshair_, paramShowCrosshair_, meta);

    meta.label = "Scope Show Glow";
    meta.description = "Toggle CRT-like phosphor background glow.";
    registry.addBool(prefix + ".showGlow", &paramShowGlow_, paramShowGlow_, meta);

    meta.label = "Scope Pattern";
    meta.range.min = kPatternMin;
    meta.range.max = kPatternMax;
    meta.range.step = 1.0f;
    meta.description = "0=Audio XY 1=Circle 2=Lissajous 3=Rose";
    registry.addFloat(prefix + ".pattern", &paramPattern_, paramPattern_, meta);

    meta.label = "Scope Mod Mode";
    meta.range.min = kModModeMin;
    meta.range.max = kModModeMax;
    meta.range.step = 1.0f;
    meta.description = "0=Cartesian 1=Radial+Wiggle 2=Wiggle Only 3=Radial Only 4=Orbit 5=Spiral Twist 6=Mirror Warp";
    registry.addFloat(prefix + ".modMode", &paramModMode_, paramModMode_, meta);

    meta.label = "Scope X Input";
    meta.range.min = kInputMin;
    meta.range.max = kInputMax;
    meta.range.step = 0.001f;
    meta.description = "Horizontal modulation input. Map mic or OSC here to bend the base pattern.";
    registry.addFloat(prefix + ".xInput", &paramXInput_, paramXInput_, meta);

    meta.label = "Scope Y Input";
    meta.description = "Vertical modulation input. Different X/Y sources create richer figures.";
    registry.addFloat(prefix + ".yInput", &paramYInput_, paramYInput_, meta);

    meta.label = "Scope Speed Input";
    meta.description = "Modulation input for movement speed.";
    registry.addFloat(prefix + ".speedInput", &paramSpeedInput_, paramSpeedInput_, meta);

    meta.label = "Scope Base Amount";
    meta.range.min = kAmountMin;
    meta.range.max = kAmountMax;
    meta.range.step = 0.01f;
    meta.description = "Strength of the internally generated test pattern.";
    registry.addFloat(prefix + ".baseAmount", &paramBaseAmount_, paramBaseAmount_, meta);

    meta.label = "Scope Mod Amount";
    meta.description = "How strongly xInput/yInput modulate the base pattern.";
    registry.addFloat(prefix + ".modAmount", &paramModAmount_, paramModAmount_, meta);

    meta.label = "Scope Radial Amount";
    meta.range.min = kSignedAmountMin;
    meta.range.max = kSignedAmountMax;
    meta.range.step = 0.01f;
    meta.description = "How much modulation changes radial distance from center.";
    registry.addFloat(prefix + ".radialAmount", &paramRadialAmount_, paramRadialAmount_, meta);

    meta.label = "Scope Wiggle Amount";
    meta.description = "How much modulation wiggles perpendicular to the current path.";
    registry.addFloat(prefix + ".wiggleAmount", &paramWiggleAmount_, paramWiggleAmount_, meta);

    meta.label = "Scope Amplitude";
    meta.range.min = kAmplitudeMin;
    meta.range.max = kAmplitudeMax;
    meta.range.step = 0.01f;
    meta.description = "Overall size of the generated pattern before scale/bias.";
    registry.addFloat(prefix + ".amplitude", &paramAmplitude_, paramAmplitude_, meta);

    meta.label = "Scope Speed";
    meta.range.min = kSpeedMin;
    meta.range.max = kSpeedMax;
    meta.range.step = 0.01f;
    meta.description = "Base movement speed for the internal oscillator.";
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_, meta);

    meta.label = "Scope Speed Mod Amount";
    meta.range.min = kSignedAmountMin;
    meta.range.max = kSignedAmountMax;
    meta.range.step = 0.01f;
    meta.description = "How much speedInput changes oscillator speed.";
    registry.addFloat(prefix + ".speedModAmount", &paramSpeedModAmount_, paramSpeedModAmount_, meta);

    meta.label = "Scope Freq X";
    meta.range.min = kFreqMin;
    meta.range.max = kFreqMax;
    meta.range.step = 0.01f;
    meta.description = "Horizontal oscillator rate.";
    registry.addFloat(prefix + ".freqX", &paramFreqX_, paramFreqX_, meta);

    meta.label = "Scope Freq Y";
    meta.description = "Vertical oscillator rate.";
    registry.addFloat(prefix + ".freqY", &paramFreqY_, paramFreqY_, meta);

    meta.label = "Scope Phase Offset";
    meta.range.min = -180.0f;
    meta.range.max = 180.0f;
    meta.range.step = 0.1f;
    meta.units = "deg";
    meta.description = "Phase offset between X and Y oscillators.";
    registry.addFloat(prefix + ".phaseOffsetDeg", &paramPhaseOffsetDeg_, paramPhaseOffsetDeg_, meta);

    meta.label = "Scope Morph";
    meta.range.min = kMorphMin;
    meta.range.max = kMorphMax;
    meta.range.step = 0.01f;
    meta.units.clear();
    meta.description = "Pattern-specific shape control.";
    registry.addFloat(prefix + ".morph", &paramMorph_, paramMorph_, meta);

    meta.label = "Scope Grid Divisions";
    meta.range.min = kGridDivMin;
    meta.range.max = kGridDivMax;
    meta.range.step = 1.0f;
    meta.description = "Number of grid subdivisions per axis quadrant.";
    registry.addFloat(prefix + ".gridDivisions", &paramGridDivisions_, paramGridDivisions_, meta);

    meta.label = "Scope Grid Alpha";
    meta.range.min = kGridAlphaMin;
    meta.range.max = kGridAlphaMax;
    meta.range.step = 0.01f;
    meta.description = "Opacity of the XY grid and crosshair overlay.";
    registry.addFloat(prefix + ".gridAlpha", &paramGridAlpha_, paramGridAlpha_, meta);

    meta.label = "Scope Glow Alpha";
    meta.range.min = kGlowAlphaMin;
    meta.range.max = kGlowAlphaMax;
    meta.range.step = 0.01f;
    meta.description = "Opacity of the CRT-style background glow.";
    registry.addFloat(prefix + ".glowAlpha", &paramGlowAlpha_, paramGlowAlpha_, meta);

    meta.label = "Scope Glow Radius";
    meta.range.min = kGlowRadiusMin;
    meta.range.max = kGlowRadiusMax;
    meta.range.step = 0.01f;
    meta.description = "Scale of the glow relative to the trace radius.";
    registry.addFloat(prefix + ".glowRadius", &paramGlowRadius_, paramGlowRadius_, meta);

    meta.label = "Scope Glow Falloff";
    meta.range.min = kGlowFalloffMin;
    meta.range.max = kGlowFalloffMax;
    meta.range.step = 0.01f;
    meta.description = "Controls how quickly the glow fades toward the center.";
    registry.addFloat(prefix + ".glowFalloff", &paramGlowFalloff_, paramGlowFalloff_, meta);

    meta.label = "Scope X Scale";
    meta.range.min = kScaleMin;
    meta.range.max = kScaleMax;
    meta.range.step = 0.01f;
    meta.description = "Horizontal gain applied after xInput.";
    registry.addFloat(prefix + ".xScale", &paramXScale_, paramXScale_, meta);

    meta.label = "Scope Y Scale";
    meta.description = "Vertical gain applied after yInput.";
    registry.addFloat(prefix + ".yScale", &paramYScale_, paramYScale_, meta);

    meta.label = "Scope X Bias";
    meta.range.min = kBiasMin;
    meta.range.max = kBiasMax;
    meta.range.step = 0.001f;
    meta.description = "Horizontal offset after gain.";
    registry.addFloat(prefix + ".xBias", &paramXBias_, paramXBias_, meta);

    meta.label = "Scope Y Bias";
    meta.description = "Vertical offset after gain.";
    registry.addFloat(prefix + ".yBias", &paramYBias_, paramYBias_, meta);

    meta.label = "Scope Rotation";
    meta.range.min = -180.0f;
    meta.range.max = 180.0f;
    meta.range.step = 0.1f;
    meta.units = "deg";
    meta.description = "Rotates the trace around screen center.";
    registry.addFloat(prefix + ".rotationDeg", &paramRotationDeg_, paramRotationDeg_, meta);

    meta.label = "Scope History";
    meta.range.min = kHistoryMin;
    meta.range.max = kHistoryMax;
    meta.range.step = 1.0f;
    meta.units.clear();
    meta.description = "Max retained points in the trail buffer.";
    registry.addFloat(prefix + ".historySize", &paramHistorySize_, paramHistorySize_, meta);

    meta.label = "Scope Sample Density";
    meta.range.min = kDensityMin;
    meta.range.max = kDensityMax;
    meta.range.step = 1.0f;
    meta.description = "Interpolated points generated each frame between samples.";
    registry.addFloat(prefix + ".sampleDensity", &paramSampleDensity_, paramSampleDensity_, meta);

    meta.label = "Scope Thickness";
    meta.range.min = kThicknessMin;
    meta.range.max = kThicknessMax;
    meta.range.step = 0.1f;
    meta.description = "OpenGL line width for the trace.";
    registry.addFloat(prefix + ".thickness", &paramThickness_, paramThickness_, meta);

    meta.label = "Scope Alpha";
    meta.range.min = kAlphaMin;
    meta.range.max = kAlphaMax;
    meta.range.step = 0.01f;
    meta.description = "Master alpha for the trace.";
    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_, meta);

    meta.label = "Scope Decay";
    meta.range.min = kDecayMin;
    meta.range.max = kDecayMax;
    meta.range.step = 0.01f;
    meta.description = "Higher values preserve more of the trail.";
    registry.addFloat(prefix + ".decay", &paramDecay_, paramDecay_, meta);

    meta.label = "Scope Intensity";
    meta.range.min = kIntensityMin;
    meta.range.max = kIntensityMax;
    meta.range.step = 0.01f;
    meta.description = "Brightness multiplier for the trace colors.";
    registry.addFloat(prefix + ".intensity", &paramIntensity_, paramIntensity_, meta);

    meta.label = "Scope Point Size";
    meta.range.min = kPointSizeMin;
    meta.range.max = kPointSizeMax;
    meta.range.step = 0.1f;
    meta.description = "Draws a dot on the newest sample when above zero.";
    registry.addFloat(prefix + ".pointSize", &paramPointSize_, paramPointSize_, meta);

    meta.range.min = kColorMin;
    meta.range.max = kColorMax;
    meta.range.step = 0.01f;
    meta.description = "RGB color channels for the trace.";
    meta.label = "Scope Red";
    registry.addFloat(prefix + ".colorR", &paramColorR_, paramColorR_, meta);
    meta.label = "Scope Green";
    registry.addFloat(prefix + ".colorG", &paramColorG_, paramColorG_, meta);
    meta.label = "Scope Blue";
    registry.addFloat(prefix + ".colorB", &paramColorB_, paramColorB_, meta);

    meta.label = "Scope BG Red";
    meta.description = "Background glow color red channel.";
    registry.addFloat(prefix + ".bgColorR", &paramBgColorR_, paramBgColorR_, meta);
    meta.label = "Scope BG Green";
    registry.addFloat(prefix + ".bgColorG", &paramBgColorG_, paramBgColorG_, meta);
    meta.label = "Scope BG Blue";
    registry.addFloat(prefix + ".bgColorB", &paramBgColorB_, paramBgColorB_, meta);

    history_.clear();
    hasLastPoint_ = false;
    phaseTime_ = 0.0f;
}

void OscilloscopeLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    clampParams();
    if (!enabled_) {
        return;
    }

    float speedFactor = std::max(0.0f, paramSpeed_ + paramSpeedInput_ * paramSpeedModAmount_);
    phaseTime_ += params.dt * speedFactor;

    glm::vec2 base = basePatternPoint(phaseTime_);
    glm::vec2 point = applyModulation(base * paramBaseAmount_);
    point.x = point.x * paramXScale_ + paramXBias_;
    point.y = point.y * paramYScale_ + paramYBias_;

    float radians = glm::radians(paramRotationDeg_);
    float s = std::sin(radians);
    float c = std::cos(radians);
    glm::vec2 rotated(point.x * c - point.y * s,
                      point.x * s + point.y * c);

    appendInterpolatedSamples(rotated);
}

void OscilloscopeLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f || history_.empty()) {
        return;
    }

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    ofTranslate(params.viewport.x * 0.5f, params.viewport.y * 0.5f);
    const float radius = std::min(params.viewport.x, params.viewport.y) * 0.42f;
    const float intensity = ofClamp(paramIntensity_, kIntensityMin, kIntensityMax);
    const float baseAlpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);

    drawBackground(radius, baseAlpha);

    ofMesh trace;
    trace.setMode(OF_PRIMITIVE_LINE_STRIP);

    const std::size_t count = history_.size();
    const float tailPower = ofMap(paramDecay_, kDecayMin, kDecayMax, 6.0f, 1.0f, true);
    for (std::size_t i = 0; i < count; ++i) {
        const glm::vec2& pt = history_[i];
        float normalizedAge = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 1.0f;
        float alpha = std::pow(ofClamp(normalizedAge, 0.0f, 1.0f), tailPower) * baseAlpha;

        trace.addVertex(glm::vec3(pt.x * radius, -pt.y * radius, 0.0f));
        trace.addColor(ofFloatColor(ofClamp(paramColorR_ * intensity, 0.0f, 1.0f),
                                    ofClamp(paramColorG_ * intensity, 0.0f, 1.0f),
                                    ofClamp(paramColorB_ * intensity, 0.0f, 1.0f),
                                    alpha));
    }

#ifndef TARGET_OPENGLES
    glLineWidth(ofClamp(paramThickness_, kThicknessMin, kThicknessMax));
#endif
    trace.draw();

    if (paramPointSize_ > 0.0f) {
        const glm::vec2& latest = history_.back();
        ofSetColor(static_cast<int>(ofClamp(paramColorR_ * intensity, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(paramColorG_ * intensity, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(paramColorB_ * intensity, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(baseAlpha * 255.0f));
        ofDrawCircle(latest.x * radius, -latest.y * radius, paramPointSize_);
    }

    ofPopView();
    ofPopStyle();
}

void OscilloscopeLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void OscilloscopeLayer::drawBackground(float radius, float baseAlpha) const {
    const float gridAlpha = ofClamp(paramGridAlpha_ * baseAlpha, 0.0f, 1.0f);
    const float glowAlpha = ofClamp(paramGlowAlpha_ * baseAlpha, 0.0f, 1.0f);
    const ofFloatColor bgColor(ofClamp(paramBgColorR_, 0.0f, 1.0f),
                               ofClamp(paramBgColorG_, 0.0f, 1.0f),
                               ofClamp(paramBgColorB_, 0.0f, 1.0f),
                               1.0f);

    if (paramShowGlow_ && glowAlpha > 0.0f) {
        const int rings = 24;
        const float glowRadius = radius * ofClamp(paramGlowRadius_, kGlowRadiusMin, kGlowRadiusMax);
        const float falloff = ofClamp(paramGlowFalloff_, kGlowFalloffMin, kGlowFalloffMax);
        for (int i = rings; i >= 1; --i) {
            float t = static_cast<float>(i) / static_cast<float>(rings);
            float alpha = std::pow(t, falloff) * glowAlpha;
            ofSetColor(static_cast<int>(bgColor.r * 255.0f),
                       static_cast<int>(bgColor.g * 255.0f),
                       static_cast<int>(bgColor.b * 255.0f),
                       static_cast<int>(alpha * 255.0f));
            ofDrawCircle(0.0f, 0.0f, glowRadius * t);
        }
    }

    if ((paramShowGrid_ || paramShowCrosshair_) && gridAlpha > 0.0f) {
        ofSetColor(static_cast<int>(ofClamp(paramColorR_, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(paramColorG_, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(paramColorB_, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(gridAlpha * 255.0f));

        if (paramShowGrid_) {
            const int divisions = std::max(1, static_cast<int>(std::round(paramGridDivisions_)));
            for (int i = 1; i <= divisions; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(divisions);
                float offset = radius * t;
                ofDrawLine(-radius, -offset, radius, -offset);
                ofDrawLine(-radius, offset, radius, offset);
                ofDrawLine(-offset, -radius, -offset, radius);
                ofDrawLine(offset, -radius, offset, radius);
            }
            ofNoFill();
            ofDrawCircle(0.0f, 0.0f, radius);
            ofFill();
        }

        if (paramShowCrosshair_) {
            ofDrawLine(-radius, 0.0f, radius, 0.0f);
            ofDrawLine(0.0f, -radius, 0.0f, radius);
        }
    }
}

glm::vec2 OscilloscopeLayer::applyModulation(const glm::vec2& basePoint) const {
    const int modMode = ofClamp(static_cast<int>(std::round(paramModMode_)),
                                static_cast<int>(kModModeMin),
                                static_cast<int>(kModModeMax));
    const glm::vec2 rawMod(paramXInput_, paramYInput_);

    if (modMode == 0) {
        return basePoint + rawMod * paramModAmount_;
    }

    glm::vec2 radial = basePoint;
    float radialLength = glm::length(radial);
    if (radialLength < 0.0001f) {
        radial = glm::vec2(1.0f, 0.0f);
        radialLength = 1.0f;
    } else {
        radial /= radialLength;
    }
    const glm::vec2 tangent(-radial.y, radial.x);

    float radialInput = paramXInput_ * paramModAmount_;
    float wiggleInput = paramYInput_ * paramModAmount_;

    glm::vec2 modulated = basePoint;
    if (modMode == 1 || modMode == 3) {
        modulated += radial * (radialInput * paramRadialAmount_);
    }
    if (modMode == 1 || modMode == 2) {
        modulated += tangent * (wiggleInput * paramWiggleAmount_);
    }

    if (modMode == 4) {
        float orbitAngle = (paramXInput_ + paramYInput_) * paramModAmount_ * paramRadialAmount_;
        float s = std::sin(orbitAngle);
        float c = std::cos(orbitAngle);
        modulated = glm::vec2(modulated.x * c - modulated.y * s,
                              modulated.x * s + modulated.y * c);
        modulated += radial * (paramXInput_ * paramRadialAmount_ * 0.25f);
    } else if (modMode == 5) {
        float twist = (paramXInput_ * paramRadialAmount_ + paramYInput_ * paramWiggleAmount_) * paramModAmount_;
        float localAngle = twist * std::max(0.25f, radialLength);
        float s = std::sin(localAngle);
        float c = std::cos(localAngle);
        modulated = glm::vec2(modulated.x * c - modulated.y * s,
                              modulated.x * s + modulated.y * c);
        modulated += radial * (paramXInput_ * paramRadialAmount_ * 0.2f);
        modulated += tangent * (paramYInput_ * paramWiggleAmount_ * 0.2f);
    } else if (modMode == 6) {
        float mirror = paramXInput_ * paramRadialAmount_ * paramModAmount_;
        float shear = paramYInput_ * paramWiggleAmount_ * paramModAmount_;
        modulated.x *= 1.0f + mirror;
        modulated.y *= 1.0f - mirror;
        modulated += glm::vec2(basePoint.y, basePoint.x) * shear * 0.5f;
    }

    return modulated;
}

glm::vec2 OscilloscopeLayer::basePatternPoint(float timeSeconds) const {
    const float amp = ofClamp(paramAmplitude_, kAmplitudeMin, kAmplitudeMax);
    const float fx = ofClamp(paramFreqX_, kFreqMin, kFreqMax);
    const float fy = ofClamp(paramFreqY_, kFreqMin, kFreqMax);
    const float phase = glm::radians(paramPhaseOffsetDeg_);
    const int pattern = ofClamp(static_cast<int>(std::round(paramPattern_)),
                                static_cast<int>(kPatternMin),
                                static_cast<int>(kPatternMax));
    const float morph = ofClamp(paramMorph_, kMorphMin, kMorphMax);

    const float tx = timeSeconds * fx * TWO_PI;
    const float ty = timeSeconds * fy * TWO_PI + phase;

    switch (pattern) {
    case 0:
        return glm::vec2(0.0f, 0.0f);
    case 1:
        return glm::vec2(std::sin(tx), std::cos(ty)) * amp;
    case 2: {
        float ratio = ofLerp(1.0f, 5.0f, morph);
        float x = std::sin(tx);
        float y = std::sin(ty * ratio);
        return glm::vec2(x, y) * amp;
    }
    case 3: {
        float petals = ofLerp(2.0f, 8.0f, morph);
        float theta = tx;
        float radius = std::cos(theta * petals);
        return glm::vec2(std::cos(theta), std::sin(theta)) * radius * amp;
    }
    default:
        return glm::vec2(std::sin(tx), std::cos(ty)) * amp;
    }
}

void OscilloscopeLayer::clampParams() {
    paramPattern_ = ofClamp(paramPattern_, kPatternMin, kPatternMax);
    paramModMode_ = ofClamp(paramModMode_, kModModeMin, kModModeMax);
    paramXInput_ = ofClamp(paramXInput_, kInputMin, kInputMax);
    paramYInput_ = ofClamp(paramYInput_, kInputMin, kInputMax);
    paramSpeedInput_ = ofClamp(paramSpeedInput_, kInputMin, kInputMax);
    paramBaseAmount_ = ofClamp(paramBaseAmount_, kAmountMin, kAmountMax);
    paramModAmount_ = ofClamp(paramModAmount_, kAmountMin, kAmountMax);
    paramRadialAmount_ = ofClamp(paramRadialAmount_, kSignedAmountMin, kSignedAmountMax);
    paramWiggleAmount_ = ofClamp(paramWiggleAmount_, kSignedAmountMin, kSignedAmountMax);
    paramAmplitude_ = ofClamp(paramAmplitude_, kAmplitudeMin, kAmplitudeMax);
    paramSpeed_ = ofClamp(paramSpeed_, kSpeedMin, kSpeedMax);
    paramSpeedModAmount_ = ofClamp(paramSpeedModAmount_, kSignedAmountMin, kSignedAmountMax);
    paramFreqX_ = ofClamp(paramFreqX_, kFreqMin, kFreqMax);
    paramFreqY_ = ofClamp(paramFreqY_, kFreqMin, kFreqMax);
    paramMorph_ = ofClamp(paramMorph_, kMorphMin, kMorphMax);
    paramGridDivisions_ = ofClamp(paramGridDivisions_, kGridDivMin, kGridDivMax);
    paramGridAlpha_ = ofClamp(paramGridAlpha_, kGridAlphaMin, kGridAlphaMax);
    paramGlowAlpha_ = ofClamp(paramGlowAlpha_, kGlowAlphaMin, kGlowAlphaMax);
    paramGlowRadius_ = ofClamp(paramGlowRadius_, kGlowRadiusMin, kGlowRadiusMax);
    paramGlowFalloff_ = ofClamp(paramGlowFalloff_, kGlowFalloffMin, kGlowFalloffMax);
    paramXScale_ = ofClamp(paramXScale_, kScaleMin, kScaleMax);
    paramYScale_ = ofClamp(paramYScale_, kScaleMin, kScaleMax);
    paramXBias_ = ofClamp(paramXBias_, kBiasMin, kBiasMax);
    paramYBias_ = ofClamp(paramYBias_, kBiasMin, kBiasMax);
    paramHistorySize_ = ofClamp(paramHistorySize_, kHistoryMin, kHistoryMax);
    paramSampleDensity_ = ofClamp(paramSampleDensity_, kDensityMin, kDensityMax);
    paramThickness_ = ofClamp(paramThickness_, kThicknessMin, kThicknessMax);
    paramAlpha_ = ofClamp(paramAlpha_, kAlphaMin, kAlphaMax);
    paramDecay_ = ofClamp(paramDecay_, kDecayMin, kDecayMax);
    paramIntensity_ = ofClamp(paramIntensity_, kIntensityMin, kIntensityMax);
    paramPointSize_ = ofClamp(paramPointSize_, kPointSizeMin, kPointSizeMax);
    paramColorR_ = ofClamp(paramColorR_, kColorMin, kColorMax);
    paramColorG_ = ofClamp(paramColorG_, kColorMin, kColorMax);
    paramColorB_ = ofClamp(paramColorB_, kColorMin, kColorMax);
    paramBgColorR_ = ofClamp(paramBgColorR_, kColorMin, kColorMax);
    paramBgColorG_ = ofClamp(paramBgColorG_, kColorMin, kColorMax);
    paramBgColorB_ = ofClamp(paramBgColorB_, kColorMin, kColorMax);

    const std::size_t maxPoints = static_cast<std::size_t>(std::round(paramHistorySize_));
    while (history_.size() > maxPoints) {
        history_.pop_front();
    }
}

void OscilloscopeLayer::appendInterpolatedSamples(const glm::vec2& targetPoint) {
    const int steps = std::max(1, static_cast<int>(std::round(paramSampleDensity_)));
    if (!hasLastPoint_) {
        appendSample(targetPoint);
        lastPoint_ = targetPoint;
        hasLastPoint_ = true;
        return;
    }

    for (int i = 1; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        appendSample(glm::mix(lastPoint_, targetPoint, t));
    }
    lastPoint_ = targetPoint;
}

void OscilloscopeLayer::appendSample(const glm::vec2& point) {
    history_.push_back(glm::clamp(point,
                                  glm::vec2(-1.5f, -1.5f),
                                  glm::vec2(1.5f, 1.5f)));
    const std::size_t maxPoints = static_cast<std::size_t>(std::round(paramHistorySize_));
    while (history_.size() > maxPoints) {
        history_.pop_front();
    }
}

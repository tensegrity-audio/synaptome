#include "OscilloscopeLayer.h"

#include "../io/AudioAnalysisBus.h"

#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
    constexpr float kPatternMin = 0.0f;
    constexpr float kPatternMax = 3.0f;
    constexpr float kSignalModeMin = 0.0f;
    constexpr float kSignalModeMax = 3.0f;
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
    constexpr float kWaveformGainMin = 0.1f;
    constexpr float kWaveformGainMax = 8.0f;
    constexpr float kWaveformMixMin = 0.0f;
    constexpr float kWaveformMixMax = 1.0f;
    constexpr float kWaveformDelayMin = 1.0f;
    constexpr float kWaveformDelayMax = 128.0f;
    constexpr float kWaveformSmoothingMin = 0.0f;
    constexpr float kWaveformSmoothingMax = 0.95f;
    constexpr float kWaveformPersistenceMin = 0.03f;
    constexpr float kWaveformPersistenceMax = 3.0f;

    float boostedAudioValue(float value, float gain, float curve) {
        return std::pow(ofClamp(value * gain, 0.0f, 1.0f), curve);
    }
}

void OscilloscopeLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramSignalMode_ = def.value("signalMode", paramSignalMode_);
    paramTriggerSync_ = def.value("triggerSync", paramTriggerSync_);
    paramWaveformGain_ = def.value("waveformGain", paramWaveformGain_);
    paramWaveformMix_ = def.value("waveformMix", paramWaveformMix_);
    paramWaveformDelay_ = def.value("waveformDelay", paramWaveformDelay_);
    paramWaveformSmoothing_ = def.value("waveformSmoothing", paramWaveformSmoothing_);
    paramWaveformPersistence_ = def.value("waveformPersistence", paramWaveformPersistence_);
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

    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "Action: Show Grid";
    meta.description = "Toggle oscilloscope-style XY coordinate grid.";
    registry.addBool(prefix + ".showGrid", &paramShowGrid_, paramShowGrid_, meta);

    meta.label = "Action: Show Crosshair";
    meta.description = "Toggle center horizontal/vertical axes.";
    registry.addBool(prefix + ".showCrosshair", &paramShowCrosshair_, paramShowCrosshair_, meta);

    meta.label = "Action: Show Glow";
    meta.description = "Toggle CRT-like phosphor background glow.";
    registry.addBool(prefix + ".showGlow", &paramShowGlow_, paramShowGlow_, meta);

    meta.label = "Action: Pattern";
    meta.range.min = kPatternMin;
    meta.range.max = kPatternMax;
    meta.range.step = 1.0f;
    meta.description = "0=Audio XY 1=Circle 2=Lissajous 3=Rose";
    registry.addFloat(prefix + ".pattern", &paramPattern_, paramPattern_, meta);

    meta.label = "Audio: Signal Mode";
    meta.range.min = kSignalModeMin;
    meta.range.max = kSignalModeMax;
    meta.range.step = 1.0f;
    meta.units.clear();
    meta.description = "0=Generated 1=Waveform Trace 2=Phase Portrait 3=Hybrid";
    registry.addFloat(prefix + ".signalMode", &paramSignalMode_, paramSignalMode_, meta);

    meta.label = "Audio: Trigger Sync";
    meta.range = ParameterRegistry::Range{};
    meta.description = "Align waveform traces on an upward zero crossing when available.";
    registry.addBool(prefix + ".triggerSync", &paramTriggerSync_, paramTriggerSync_, meta);

    meta.label = "Audio: Waveform Gain";
    meta.range.min = kWaveformGainMin;
    meta.range.max = kWaveformGainMax;
    meta.range.step = 0.01f;
    meta.description = "Gain applied to full waveform samples before drawing or hybrid modulation.";
    registry.addFloat(prefix + ".waveformGain", &paramWaveformGain_, paramWaveformGain_, meta);

    meta.label = "Audio: Waveform Mix";
    meta.range.min = kWaveformMixMin;
    meta.range.max = kWaveformMixMax;
    meta.range.step = 0.01f;
    meta.description = "Blend between mapped scalar inputs and full waveform modulation in Hybrid mode.";
    registry.addFloat(prefix + ".waveformMix", &paramWaveformMix_, paramWaveformMix_, meta);

    meta.label = "Time: Phase Delay";
    meta.range.min = kWaveformDelayMin;
    meta.range.max = kWaveformDelayMax;
    meta.range.step = 1.0f;
    meta.units = "samples";
    meta.description = "Sample delay for phase-portrait and hybrid waveform coordinates.";
    registry.addFloat(prefix + ".waveformDelay", &paramWaveformDelay_, paramWaveformDelay_, meta);

    meta.label = "Audio: Waveform Smoothing";
    meta.range.min = kWaveformSmoothingMin;
    meta.range.max = kWaveformSmoothingMax;
    meta.range.step = 0.01f;
    meta.units = "normalized";
    meta.description = "Higher values smooth incoming waveform snapshots.";
    registry.addFloat(prefix + ".waveformSmoothing", &paramWaveformSmoothing_, paramWaveformSmoothing_, meta);

    meta.label = "Time: Waveform Persistence";
    meta.range.min = kWaveformPersistenceMin;
    meta.range.max = kWaveformPersistenceMax;
    meta.range.step = 0.01f;
    meta.units = "s";
    meta.description = "How long complete waveform traces remain in phosphor history.";
    registry.addFloat(prefix + ".waveformPersistence", &paramWaveformPersistence_, paramWaveformPersistence_, meta);

    meta.units.clear();
    meta.label = "Action: Mod Mode";
    meta.range.min = kModModeMin;
    meta.range.max = kModModeMax;
    meta.range.step = 1.0f;
    meta.description = "0=Cartesian 1=Radial+Wiggle 2=Wiggle Only 3=Radial Only 4=Orbit 5=Spiral Twist 6=Mirror Warp";
    registry.addFloat(prefix + ".modMode", &paramModMode_, paramModMode_, meta);

    meta.label = "Action: X Input";
    meta.range.min = kInputMin;
    meta.range.max = kInputMax;
    meta.range.step = 0.001f;
    meta.description = "Horizontal modulation input. Map mic or OSC here to bend the base pattern.";
    registry.addFloat(prefix + ".xInput", &paramXInput_, paramXInput_, meta);

    meta.label = "Action: Y Input";
    meta.description = "Vertical modulation input. Different X/Y sources create richer figures.";
    registry.addFloat(prefix + ".yInput", &paramYInput_, paramYInput_, meta);

    meta.label = "Action: Speed Input";
    meta.description = "Modulation input for movement speed.";
    registry.addFloat(prefix + ".speedInput", &paramSpeedInput_, paramSpeedInput_, meta);

    meta.label = "Scale: Base Amount";
    meta.range.min = kAmountMin;
    meta.range.max = kAmountMax;
    meta.range.step = 0.01f;
    meta.description = "Strength of the internally generated test pattern.";
    registry.addFloat(prefix + ".baseAmount", &paramBaseAmount_, paramBaseAmount_, meta);

    meta.label = "Motion: Mod Amount";
    meta.description = "How strongly xInput/yInput modulate the base pattern.";
    registry.addFloat(prefix + ".modAmount", &paramModAmount_, paramModAmount_, meta);

    meta.label = "Scale: Radial Amount";
    meta.range.min = kSignedAmountMin;
    meta.range.max = kSignedAmountMax;
    meta.range.step = 0.01f;
    meta.description = "How much modulation changes radial distance from center.";
    registry.addFloat(prefix + ".radialAmount", &paramRadialAmount_, paramRadialAmount_, meta);

    meta.label = "Motion: Wiggle Amount";
    meta.description = "How much modulation wiggles perpendicular to the current path.";
    registry.addFloat(prefix + ".wiggleAmount", &paramWiggleAmount_, paramWiggleAmount_, meta);

    meta.label = "Scale: Amplitude";
    meta.range.min = kAmplitudeMin;
    meta.range.max = kAmplitudeMax;
    meta.range.step = 0.01f;
    meta.description = "Overall size of the generated pattern before scale/bias.";
    registry.addFloat(prefix + ".amplitude", &paramAmplitude_, paramAmplitude_, meta);

    meta.label = "Time: Speed";
    meta.range.min = kSpeedMin;
    meta.range.max = kSpeedMax;
    meta.range.step = 0.01f;
    meta.description = "Base movement speed for the internal oscillator.";
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_, meta);

    meta.label = "Time: Speed Mod Amount";
    meta.range.min = kSignedAmountMin;
    meta.range.max = kSignedAmountMax;
    meta.range.step = 0.01f;
    meta.description = "How much speedInput changes oscillator speed.";
    registry.addFloat(prefix + ".speedModAmount", &paramSpeedModAmount_, paramSpeedModAmount_, meta);

    meta.label = "Time: Freq X";
    meta.range.min = kFreqMin;
    meta.range.max = kFreqMax;
    meta.range.step = 0.01f;
    meta.description = "Horizontal oscillator rate.";
    registry.addFloat(prefix + ".freqX", &paramFreqX_, paramFreqX_, meta);

    meta.label = "Time: Freq Y";
    meta.description = "Vertical oscillator rate.";
    registry.addFloat(prefix + ".freqY", &paramFreqY_, paramFreqY_, meta);

    meta.label = "Time: Phase Offset";
    meta.range.min = -180.0f;
    meta.range.max = 180.0f;
    meta.range.step = 0.1f;
    meta.units = "deg";
    meta.description = "Phase offset between X and Y oscillators.";
    registry.addFloat(prefix + ".phaseOffsetDeg", &paramPhaseOffsetDeg_, paramPhaseOffsetDeg_, meta);

    meta.label = "Motion: Morph";
    meta.range.min = kMorphMin;
    meta.range.max = kMorphMax;
    meta.range.step = 0.01f;
    meta.units.clear();
    meta.description = "Pattern-specific shape control.";
    registry.addFloat(prefix + ".morph", &paramMorph_, paramMorph_, meta);

    meta.label = "Count: Grid Divisions";
    meta.range.min = kGridDivMin;
    meta.range.max = kGridDivMax;
    meta.range.step = 1.0f;
    meta.description = "Number of grid subdivisions per axis quadrant.";
    registry.addFloat(prefix + ".gridDivisions", &paramGridDivisions_, paramGridDivisions_, meta);

    meta.label = "Alpha: Grid";
    meta.range.min = kGridAlphaMin;
    meta.range.max = kGridAlphaMax;
    meta.range.step = 0.01f;
    meta.description = "Opacity of the XY grid and crosshair overlay.";
    registry.addFloat(prefix + ".gridAlpha", &paramGridAlpha_, paramGridAlpha_, meta);

    meta.label = "Glow: Alpha";
    meta.range.min = kGlowAlphaMin;
    meta.range.max = kGlowAlphaMax;
    meta.range.step = 0.01f;
    meta.description = "Opacity of the CRT-style background glow.";
    registry.addFloat(prefix + ".glowAlpha", &paramGlowAlpha_, paramGlowAlpha_, meta);

    meta.label = "Glow: Radius";
    meta.range.min = kGlowRadiusMin;
    meta.range.max = kGlowRadiusMax;
    meta.range.step = 0.01f;
    meta.description = "Scale of the glow relative to the trace radius.";
    registry.addFloat(prefix + ".glowRadius", &paramGlowRadius_, paramGlowRadius_, meta);

    meta.label = "Glow: Falloff";
    meta.range.min = kGlowFalloffMin;
    meta.range.max = kGlowFalloffMax;
    meta.range.step = 0.01f;
    meta.description = "Controls how quickly the glow fades toward the center.";
    registry.addFloat(prefix + ".glowFalloff", &paramGlowFalloff_, paramGlowFalloff_, meta);

    meta.label = "Scale: X";
    meta.range.min = kScaleMin;
    meta.range.max = kScaleMax;
    meta.range.step = 0.01f;
    meta.description = "Horizontal gain applied after xInput.";
    registry.addFloat(prefix + ".xScale", &paramXScale_, paramXScale_, meta);

    meta.label = "Scale: Y";
    meta.description = "Vertical gain applied after yInput.";
    registry.addFloat(prefix + ".yScale", &paramYScale_, paramYScale_, meta);

    meta.label = "Motion: X Bias";
    meta.range.min = kBiasMin;
    meta.range.max = kBiasMax;
    meta.range.step = 0.001f;
    meta.description = "Horizontal offset after gain.";
    registry.addFloat(prefix + ".xBias", &paramXBias_, paramXBias_, meta);

    meta.label = "Motion: Y Bias";
    meta.description = "Vertical offset after gain.";
    registry.addFloat(prefix + ".yBias", &paramYBias_, paramYBias_, meta);

    meta.label = "Motion: Rotation";
    meta.range.min = -180.0f;
    meta.range.max = 180.0f;
    meta.range.step = 0.1f;
    meta.units = "deg";
    meta.description = "Rotates the trace around screen center.";
    registry.addFloat(prefix + ".rotationDeg", &paramRotationDeg_, paramRotationDeg_, meta);

    meta.label = "Time: History";
    meta.range.min = kHistoryMin;
    meta.range.max = kHistoryMax;
    meta.range.step = 1.0f;
    meta.units.clear();
    meta.description = "Max retained points in the trail buffer.";
    registry.addFloat(prefix + ".historySize", &paramHistorySize_, paramHistorySize_, meta);

    meta.label = "Count: Sample Density";
    meta.range.min = kDensityMin;
    meta.range.max = kDensityMax;
    meta.range.step = 1.0f;
    meta.description = "Interpolated points generated each frame between samples.";
    registry.addFloat(prefix + ".sampleDensity", &paramSampleDensity_, paramSampleDensity_, meta);

    meta.label = "Scale: Thickness";
    meta.range.min = kThicknessMin;
    meta.range.max = kThicknessMax;
    meta.range.step = 0.1f;
    meta.description = "OpenGL line width for the trace.";
    registry.addFloat(prefix + ".thickness", &paramThickness_, paramThickness_, meta);

    meta.label = "Alpha: Scope";
    meta.range.min = kAlphaMin;
    meta.range.max = kAlphaMax;
    meta.range.step = 0.01f;
    meta.description = "Master alpha for the trace.";
    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_, meta);

    meta.label = "Time: Decay";
    meta.range.min = kDecayMin;
    meta.range.max = kDecayMax;
    meta.range.step = 0.01f;
    meta.description = "Higher values preserve more of the trail.";
    registry.addFloat(prefix + ".decay", &paramDecay_, paramDecay_, meta);

    meta.label = "Glow: Intensity";
    meta.range.min = kIntensityMin;
    meta.range.max = kIntensityMax;
    meta.range.step = 0.01f;
    meta.description = "Brightness multiplier for the trace colors.";
    registry.addFloat(prefix + ".intensity", &paramIntensity_, paramIntensity_, meta);

    meta.label = "Scale: Point Size";
    meta.range.min = kPointSizeMin;
    meta.range.max = kPointSizeMax;
    meta.range.step = 0.1f;
    meta.description = "Draws a dot on the newest sample when above zero.";
    registry.addFloat(prefix + ".pointSize", &paramPointSize_, paramPointSize_, meta);

    meta.range.min = kColorMin;
    meta.range.max = kColorMax;
    meta.range.step = 0.01f;
    meta.description = "RGB color channels for the trace.";
    meta.label = "Color: Scope R";
    registry.addFloat(prefix + ".colorR", &paramColorR_, paramColorR_, meta);
    meta.label = "Color: Scope G";
    registry.addFloat(prefix + ".colorG", &paramColorG_, paramColorG_, meta);
    meta.label = "Color: Scope B";
    registry.addFloat(prefix + ".colorB", &paramColorB_, paramColorB_, meta);

    meta.label = "Color: Background R";
    meta.description = "Background glow color red channel.";
    registry.addFloat(prefix + ".bgColorR", &paramBgColorR_, paramBgColorR_, meta);
    meta.label = "Color: Background G";
    registry.addFloat(prefix + ".bgColorG", &paramBgColorG_, paramBgColorG_, meta);
    meta.label = "Color: Background B";
    registry.addFloat(prefix + ".bgColorB", &paramBgColorB_, paramBgColorB_, meta);

    history_.clear();
    waveformFrames_.clear();
    waveform_.clear();
    hasLastPoint_ = false;
    hasWaveform_ = false;
    lastAudioFrame_ = 0;
    audioLevel_ = 0.0f;
    audioPeak_ = 0.0f;
    audioBass_ = 0.0f;
    audioMids_ = 0.0f;
    audioHighs_ = 0.0f;
    phaseTime_ = 0.0f;
}

void OscilloscopeLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    clampParams();
    if (!enabled_) {
        return;
    }

    const int mode = signalMode();
    ageWaveformFrames(params.dt);
    const bool newWaveformFrame = updateWaveformState(params.dt);
    if (mode == 1 || mode == 2) {
        if (newWaveformFrame) {
            appendWaveformTraceFrame(mode);
        }
        return;
    }

    float speedFactor = std::max(0.0f, paramSpeed_ + paramSpeedInput_ * paramSpeedModAmount_);
    phaseTime_ += params.dt * speedFactor;

    glm::vec2 base = basePatternPoint(phaseTime_);
    const glm::vec2 modInput = mode == 3
        ? waveformModulationPoint(phaseTime_)
        : glm::vec2(paramXInput_, paramYInput_);
    glm::vec2 point = transformPoint(applyModulation(base * paramBaseAmount_, modInput));

    appendInterpolatedSamples(point);
}

void OscilloscopeLayer::draw(const LayerDrawParams& params) {
    const int mode = signalMode();
    const bool waveformTraceMode = mode == 1 || mode == 2;
    if (!enabled_ || params.slotOpacity <= 0.0f ||
        (waveformTraceMode ? waveformFrames_.empty() : history_.empty())) {
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

    if (waveformTraceMode) {
        const float tailPower = ofMap(paramDecay_, kDecayMin, kDecayMax, 3.25f, 0.85f, true);
        const float maxAge = std::max(kWaveformPersistenceMin, paramWaveformPersistence_);
        for (const auto& frame : waveformFrames_) {
            if (frame.points.size() < 2) {
                continue;
            }

            const float ageNorm = ofClamp(1.0f - frame.ageSeconds / maxAge, 0.0f, 1.0f);
            const float alphaScale = std::pow(ageNorm, tailPower);
            if (alphaScale <= 0.0f) {
                continue;
            }

            ofMesh trace;
            trace.setMode(OF_PRIMITIVE_LINE_STRIP);
            const float energyLift = ofClamp(0.65f + frame.energy * 0.85f, 0.65f, 1.35f);
            for (std::size_t i = 0; i < frame.points.size(); ++i) {
                const glm::vec2& pt = frame.points[i];
                const float localAge = frame.points.size() > 1
                    ? static_cast<float>(i) / static_cast<float>(frame.points.size() - 1)
                    : 1.0f;
                const float localAlpha = ofClamp((0.55f + localAge * 0.45f) * alphaScale * baseAlpha * energyLift,
                                                 0.0f,
                                                 1.0f);
                trace.addVertex(glm::vec3(pt.x * radius, -pt.y * radius, 0.0f));
                trace.addColor(ofFloatColor(ofClamp(paramColorR_ * intensity, 0.0f, 1.0f),
                                            ofClamp(paramColorG_ * intensity, 0.0f, 1.0f),
                                            ofClamp(paramColorB_ * intensity, 0.0f, 1.0f),
                                            localAlpha));
            }

#ifndef TARGET_OPENGLES
            glLineWidth(ofClamp(paramThickness_, kThicknessMin, kThicknessMax));
#endif
            trace.draw();
        }

        if (paramPointSize_ > 0.0f && !waveformFrames_.empty() && !waveformFrames_.back().points.empty()) {
            const glm::vec2& latest = waveformFrames_.back().points.back();
            ofSetColor(static_cast<int>(ofClamp(paramColorR_ * intensity, 0.0f, 1.0f) * 255.0f),
                       static_cast<int>(ofClamp(paramColorG_ * intensity, 0.0f, 1.0f) * 255.0f),
                       static_cast<int>(ofClamp(paramColorB_ * intensity, 0.0f, 1.0f) * 255.0f),
                       static_cast<int>(baseAlpha * 255.0f));
            ofDrawCircle(latest.x * radius, -latest.y * radius, paramPointSize_);
        }

        ofPopView();
        ofPopStyle();
        return;
    }

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
    return applyModulation(basePoint, glm::vec2(paramXInput_, paramYInput_));
}

glm::vec2 OscilloscopeLayer::applyModulation(const glm::vec2& basePoint, const glm::vec2& rawMod) const {
    const int modMode = ofClamp(static_cast<int>(std::round(paramModMode_)),
                                static_cast<int>(kModModeMin),
                                static_cast<int>(kModModeMax));

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

    float radialInput = rawMod.x * paramModAmount_;
    float wiggleInput = rawMod.y * paramModAmount_;

    glm::vec2 modulated = basePoint;
    if (modMode == 1 || modMode == 3) {
        modulated += radial * (radialInput * paramRadialAmount_);
    }
    if (modMode == 1 || modMode == 2) {
        modulated += tangent * (wiggleInput * paramWiggleAmount_);
    }

    if (modMode == 4) {
        float orbitAngle = (rawMod.x + rawMod.y) * paramModAmount_ * paramRadialAmount_;
        float s = std::sin(orbitAngle);
        float c = std::cos(orbitAngle);
        modulated = glm::vec2(modulated.x * c - modulated.y * s,
                              modulated.x * s + modulated.y * c);
        modulated += radial * (rawMod.x * paramRadialAmount_ * 0.25f);
    } else if (modMode == 5) {
        float twist = (rawMod.x * paramRadialAmount_ + rawMod.y * paramWiggleAmount_) * paramModAmount_;
        float localAngle = twist * std::max(0.25f, radialLength);
        float s = std::sin(localAngle);
        float c = std::cos(localAngle);
        modulated = glm::vec2(modulated.x * c - modulated.y * s,
                              modulated.x * s + modulated.y * c);
        modulated += radial * (rawMod.x * paramRadialAmount_ * 0.2f);
        modulated += tangent * (rawMod.y * paramWiggleAmount_ * 0.2f);
    } else if (modMode == 6) {
        float mirror = rawMod.x * paramRadialAmount_ * paramModAmount_;
        float shear = rawMod.y * paramWiggleAmount_ * paramModAmount_;
        modulated.x *= 1.0f + mirror;
        modulated.y *= 1.0f - mirror;
        modulated += glm::vec2(basePoint.y, basePoint.x) * shear * 0.5f;
    }

    return modulated;
}

glm::vec2 OscilloscopeLayer::transformPoint(const glm::vec2& point) const {
    glm::vec2 scaled(point.x * paramXScale_ + paramXBias_,
                     point.y * paramYScale_ + paramYBias_);

    const float radians = glm::radians(paramRotationDeg_);
    const float s = std::sin(radians);
    const float c = std::cos(radians);
    return glm::vec2(scaled.x * c - scaled.y * s,
                     scaled.x * s + scaled.y * c);
}

int OscilloscopeLayer::signalMode() const {
    return ofClamp(static_cast<int>(std::round(paramSignalMode_)),
                   static_cast<int>(kSignalModeMin),
                   static_cast<int>(kSignalModeMax));
}

bool OscilloscopeLayer::updateWaveformState(float dt) {
    (void)dt;
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    if (!snapshot.valid || snapshot.waveform.empty()) {
        hasWaveform_ = false;
        return false;
    }

    hasWaveform_ = true;
    if (snapshot.frame == lastAudioFrame_) {
        return false;
    }

    const float follow = 1.0f - ofClamp(paramWaveformSmoothing_,
                                        kWaveformSmoothingMin,
                                        kWaveformSmoothingMax);
    const bool resetWaveform = waveform_.size() != snapshot.waveform.size() || lastAudioFrame_ == 0;
    if (resetWaveform) {
        waveform_.resize(snapshot.waveform.size());
        for (std::size_t i = 0; i < snapshot.waveform.size(); ++i) {
            waveform_[i] = ofClamp(snapshot.waveform[i], -1.0f, 1.0f);
        }
        audioLevel_ = boostedAudioValue(snapshot.level, 3.2f, 0.70f);
        audioPeak_ = boostedAudioValue(std::max(snapshot.peak, snapshot.level), 2.8f, 0.66f);
        audioBass_ = boostedAudioValue(snapshot.bass, 2.7f, 0.72f);
        audioMids_ = boostedAudioValue(snapshot.mids, 3.0f, 0.68f);
        audioHighs_ = boostedAudioValue(snapshot.highs, 3.2f, 0.66f);
    } else {
        for (std::size_t i = 0; i < waveform_.size(); ++i) {
            waveform_[i] = ofLerp(waveform_[i], ofClamp(snapshot.waveform[i], -1.0f, 1.0f), follow);
        }
        audioLevel_ = ofLerp(audioLevel_, boostedAudioValue(snapshot.level, 3.2f, 0.70f), follow);
        audioPeak_ = ofLerp(audioPeak_, boostedAudioValue(std::max(snapshot.peak, snapshot.level), 2.8f, 0.66f), follow);
        audioBass_ = ofLerp(audioBass_, boostedAudioValue(snapshot.bass, 2.7f, 0.72f), follow);
        audioMids_ = ofLerp(audioMids_, boostedAudioValue(snapshot.mids, 3.0f, 0.68f), follow);
        audioHighs_ = ofLerp(audioHighs_, boostedAudioValue(snapshot.highs, 3.2f, 0.66f), follow);
    }

    lastAudioFrame_ = snapshot.frame;
    return true;
}

void OscilloscopeLayer::ageWaveformFrames(float dt) {
    for (auto& frame : waveformFrames_) {
        frame.ageSeconds += std::max(0.0f, dt);
    }
    pruneWaveformFrames();
}

void OscilloscopeLayer::pruneWaveformFrames() {
    const float maxAge = std::max(kWaveformPersistenceMin, paramWaveformPersistence_);
    while (!waveformFrames_.empty() && waveformFrames_.front().ageSeconds > maxAge) {
        waveformFrames_.pop_front();
    }

    const std::size_t maxFrames = static_cast<std::size_t>(
        ofClamp(std::ceil(maxAge * 60.0f), 1.0f, 180.0f));
    while (waveformFrames_.size() > maxFrames) {
        waveformFrames_.pop_front();
    }
}

bool OscilloscopeLayer::appendWaveformTraceFrame(int mode) {
    if (!hasWaveform_ || waveform_.size() < 2) {
        return false;
    }

    const std::size_t start = waveformTriggerStart();
    const std::size_t delay = std::min<std::size_t>(
        std::max<std::size_t>(1, static_cast<std::size_t>(std::round(paramWaveformDelay_))),
        std::max<std::size_t>(1, waveform_.size() - 1));
    const float amp = ofClamp(paramAmplitude_, kAmplitudeMin, kAmplitudeMax);
    const float gain = ofClamp(paramWaveformGain_, kWaveformGainMin, kWaveformGainMax) * amp;

    WaveformTraceFrame frame;
    frame.points.reserve(waveform_.size());
    float absSum = 0.0f;
    const float denom = static_cast<float>(std::max<std::size_t>(1, waveform_.size() - 1));
    for (std::size_t i = 0; i < waveform_.size(); ++i) {
        const float sample = waveformSampleWrapped(start + i);
        absSum += std::abs(sample);

        glm::vec2 point;
        if (mode == 2) {
            const float delayed = waveformSampleWrapped(start + i + delay);
            point = glm::vec2(sample * gain, delayed * gain);
        } else {
            const float u = static_cast<float>(i) / denom;
            point = glm::vec2(ofLerp(-amp, amp, u), sample * gain);
        }

        frame.points.push_back(glm::clamp(transformPoint(point),
                                          glm::vec2(-1.5f, -1.5f),
                                          glm::vec2(1.5f, 1.5f)));
    }

    frame.energy = ofClamp((absSum / static_cast<float>(waveform_.size())) * paramWaveformGain_ * 0.75f +
                               audioPeak_ * 0.35f,
                           0.0f,
                           1.0f);
    waveformFrames_.push_back(std::move(frame));
    pruneWaveformFrames();
    return true;
}

float OscilloscopeLayer::waveformSampleWrapped(std::size_t index) const {
    if (waveform_.empty()) {
        return 0.0f;
    }
    return ofClamp(waveform_[index % waveform_.size()], -1.0f, 1.0f);
}

std::size_t OscilloscopeLayer::waveformTriggerStart() const {
    if (!paramTriggerSync_ || waveform_.size() < 3) {
        return 0;
    }

    std::size_t best = 0;
    float bestSlope = 0.0f;
    bool foundCrossing = false;
    for (std::size_t i = 1; i < waveform_.size(); ++i) {
        const float prev = waveform_[i - 1];
        const float curr = waveform_[i];
        if (prev <= 0.0f && curr > 0.0f) {
            const float slope = curr - prev;
            if (!foundCrossing || slope > bestSlope) {
                best = i;
                bestSlope = slope;
                foundCrossing = true;
            }
        }
    }

    if (foundCrossing) {
        return best;
    }

    float peak = 0.0f;
    for (std::size_t i = 0; i < waveform_.size(); ++i) {
        const float value = std::abs(waveform_[i]);
        if (value > peak) {
            peak = value;
            best = i;
        }
    }
    return best;
}

glm::vec2 OscilloscopeLayer::waveformModulationPoint(float timeSeconds) const {
    const glm::vec2 scalar(paramXInput_, paramYInput_);
    if (!hasWaveform_ || waveform_.empty()) {
        return scalar;
    }

    const std::size_t delay = std::min<std::size_t>(
        std::max<std::size_t>(1, static_cast<std::size_t>(std::round(paramWaveformDelay_))),
        std::max<std::size_t>(1, waveform_.size() - 1));
    const float scanRate = ofLerp(0.18f, 0.72f, ofClamp(audioMids_ * 0.55f + audioHighs_ * 0.45f, 0.0f, 1.0f));
    const float wrapped = std::fmod(std::max(0.0f, timeSeconds) *
                                        static_cast<float>(waveform_.size()) *
                                        scanRate,
                                    static_cast<float>(waveform_.size()));
    const std::size_t index = static_cast<std::size_t>(wrapped);
    const float lift = ofClamp(0.65f + audioLevel_ * 0.55f + audioPeak_ * 0.45f, 0.65f, 1.65f);
    const glm::vec2 wave(ofClamp(waveformSampleWrapped(index) * paramWaveformGain_ * lift, -1.0f, 1.0f),
                         ofClamp(waveformSampleWrapped(index + delay) * paramWaveformGain_ * lift, -1.0f, 1.0f));

    return glm::mix(scalar, wave, ofClamp(paramWaveformMix_, kWaveformMixMin, kWaveformMixMax));
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
    paramSignalMode_ = ofClamp(paramSignalMode_, kSignalModeMin, kSignalModeMax);
    paramWaveformGain_ = ofClamp(paramWaveformGain_, kWaveformGainMin, kWaveformGainMax);
    paramWaveformMix_ = ofClamp(paramWaveformMix_, kWaveformMixMin, kWaveformMixMax);
    paramWaveformDelay_ = ofClamp(paramWaveformDelay_, kWaveformDelayMin, kWaveformDelayMax);
    paramWaveformSmoothing_ = ofClamp(paramWaveformSmoothing_, kWaveformSmoothingMin, kWaveformSmoothingMax);
    paramWaveformPersistence_ = ofClamp(paramWaveformPersistence_, kWaveformPersistenceMin, kWaveformPersistenceMax);
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
    pruneWaveformFrames();
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

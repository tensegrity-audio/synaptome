#include "AuroraCurtainLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr int kMinSamples = 32;
    constexpr int kMaxSamples = 384;
    constexpr int kMaxHistory = 18;
    constexpr float kHistoryInterval = 1.0f / 30.0f;
    constexpr float kMinDt = 0.0f;
    constexpr float kMaxDt = 1.0f / 20.0f;

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, 0.0f, 0.98f);
    }

    ofFloatColor colorFrom(float r, float g, float b, float a) {
        return ofFloatColor(ofClamp(r, 0.0f, 1.5f),
                            ofClamp(g, 0.0f, 1.5f),
                            ofClamp(b, 0.0f, 1.5f),
                            ofClamp(a, 0.0f, 1.0f));
    }

    void setColor(const ofFloatColor& color) {
        ofSetColor(static_cast<int>(ofClamp(color.r, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.g, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.b, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.a, 0.0f, 1.0f) * 255.0f));
    }

    float signedNoise(float x, float y, float z) {
        return ofNoise(x, y, z) * 2.0f - 1.0f;
    }
}

void AuroraCurtainLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramMirror_ = def.value("mirror", paramMirror_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramCurtainCount_ = def.value("curtainCount", paramCurtainCount_);
    paramSampleDensity_ = def.value("sampleDensity", paramSampleDensity_);
    paramWaveformGain_ = def.value("waveformGain", paramWaveformGain_);
    paramVerticalScale_ = def.value("verticalScale", paramVerticalScale_);
    paramCurtainHeight_ = def.value("curtainHeight", paramCurtainHeight_);
    paramFoldStrength_ = def.value("foldStrength", paramFoldStrength_);
    paramFlowSpeed_ = def.value("flowSpeed", paramFlowSpeed_);
    paramNoiseScale_ = def.value("noiseScale", paramNoiseScale_);
    paramTrailDecay_ = def.value("trailDecay", paramTrailDecay_);
    paramLineThickness_ = def.value("lineThickness", paramLineThickness_);
    paramGlowAmount_ = def.value("glowAmount", paramGlowAmount_);
    paramShimmerAmount_ = def.value("shimmerAmount", paramShimmerAmount_);
    paramMagneticTilt_ = def.value("magneticTilt", paramMagneticTilt_);
    paramBassLift_ = def.value("bassLift", paramBassLift_);
    paramMidsFold_ = def.value("midsFold", paramMidsFold_);
    paramHighsSparkle_ = def.value("highsSparkle", paramHighsSparkle_);
    paramPeakFlash_ = def.value("peakFlash", paramPeakFlash_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    paramBgR_ = def.value("bgR", paramBgR_);
    paramBgG_ = def.value("bgG", paramBgG_);
    paramBgB_ = def.value("bgB", paramBgB_);
    paramColorR_ = def.value("colorR", paramColorR_);
    paramColorG_ = def.value("colorG", paramColorG_);
    paramColorB_ = def.value("colorB", paramColorB_);
    paramColor2R_ = def.value("color2R", paramColor2R_);
    paramColor2G_ = def.value("color2G", paramColor2G_);
    paramColor2B_ = def.value("color2B", paramColor2B_);
    readColor(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(def, "color", paramColorR_, paramColorG_, paramColorB_);
    readColor(def, "color2", paramColor2R_, paramColor2G_, paramColor2B_);
    clampParams();
}

void AuroraCurtainLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.auroraCurtains" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Aurora Curtains";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "Action: Mirror";
    meta.description = "Draw a reflected secondary curtain for denser stage looks.";
    registry.addBool(prefix + ".mirror", &paramMirror_, paramMirror_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Alpha: Aurora", 0.0f, 1.0f, 0.01f, "normalized");
    registerFloat(registry, prefix + ".curtainCount", &paramCurtainCount_, paramCurtainCount_, "Count: Curtains", 1.0f, 8.0f, 1.0f);
    registerFloat(registry, prefix + ".sampleDensity", &paramSampleDensity_, paramSampleDensity_, "Count: Sample Density", static_cast<float>(kMinSamples), static_cast<float>(kMaxSamples), 1.0f, "samples");
    registerFloat(registry, prefix + ".waveformGain", &paramWaveformGain_, paramWaveformGain_, "Audio: Waveform Gain", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".verticalScale", &paramVerticalScale_, paramVerticalScale_, "Scale: Vertical", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".curtainHeight", &paramCurtainHeight_, paramCurtainHeight_, "Scale: Curtain Height", 0.04f, 0.80f, 0.01f, "viewport");
    registerFloat(registry, prefix + ".foldStrength", &paramFoldStrength_, paramFoldStrength_, "Force: Fold Strength", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".flowSpeed", &paramFlowSpeed_, paramFlowSpeed_, "Time: Flow Speed", -2.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".noiseScale", &paramNoiseScale_, paramNoiseScale_, "Scale: Noise", 0.2f, 8.0f, 0.01f);
    registerFloat(registry, prefix + ".trailDecay", &paramTrailDecay_, paramTrailDecay_, "Time: Trail Decay", 0.02f, 0.85f, 0.01f);
    registerFloat(registry, prefix + ".lineThickness", &paramLineThickness_, paramLineThickness_, "Scale: Line Thickness", 0.5f, 12.0f, 0.1f, "px");
    registerFloat(registry, prefix + ".glowAmount", &paramGlowAmount_, paramGlowAmount_, "Glow: Amount", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".shimmerAmount", &paramShimmerAmount_, paramShimmerAmount_, "Glow: Shimmer", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".magneticTilt", &paramMagneticTilt_, paramMagneticTilt_, "Motion: Magnetic Tilt", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bassLift", &paramBassLift_, paramBassLift_, "Audio: Bass Lift", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".midsFold", &paramMidsFold_, paramMidsFold_, "Audio: Mids Fold", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".highsSparkle", &paramHighsSparkle_, paramHighsSparkle_, "Audio: Highs Sparkle", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".peakFlash", &paramPeakFlash_, paramPeakFlash_, "Audio: Peak Flash", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Audio: Amount", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Audio: Smoothing", 0.0f, 0.98f, 0.01f);
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Alpha: Background", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Color: Background R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Color: Background G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Color: Background B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".colorR", &paramColorR_, paramColorR_, "Color: Aurora R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorG", &paramColorG_, paramColorG_, "Color: Aurora G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorB", &paramColorB_, paramColorB_, "Color: Aurora B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".color2R", &paramColor2R_, paramColor2R_, "Color: Aurora 2 R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".color2G", &paramColor2G_, paramColor2G_, "Color: Aurora 2 G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".color2B", &paramColor2B_, paramColor2B_, "Color: Aurora 2 B", 0.0f, 1.5f, 0.01f);
}

void AuroraCurtainLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    const float dt = ofClamp(params.dt, kMinDt, kMaxDt);
    updateAudioState(dt, params.time);
}

void AuroraCurtainLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (history_.empty()) {
        if (waveform_.empty()) {
            const int sampleCount = static_cast<int>(std::round(paramSampleDensity_));
            ensureWaveformSize(sampleCount);
            const float denom = static_cast<float>(std::max(1, sampleCount - 1));
            for (int i = 0; i < sampleCount; ++i) {
                const float t = static_cast<float>(i) / denom;
                waveform_[static_cast<std::size_t>(i)] = targetSampleFor(t, params.time);
            }
        }
        pushHistory();
    }
    if (history_.empty()) {
        return;
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    const int curtainCount = static_cast<int>(std::round(paramCurtainCount_));
    const int historyCount = static_cast<int>(history_.size());

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    if (paramBgAlpha_ > 0.0f) {
        setColor(colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * params.slotOpacity));
        ofDrawRectangle(0.0f, 0.0f, width, height);
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    drawHorizonGlow(width, height, alpha);

    for (int historyIndex = historyCount - 1; historyIndex >= 0; --historyIndex) {
        const auto& samples = history_[static_cast<std::size_t>(historyIndex)];
        for (int curtainIndex = 0; curtainIndex < curtainCount; ++curtainIndex) {
            drawCurtain(samples,
                        curtainIndex,
                        curtainCount,
                        historyIndex,
                        historyCount,
                        width,
                        height,
                        params.time,
                        alpha,
                        false);
            if (paramMirror_) {
                drawCurtain(samples,
                            curtainIndex,
                            curtainCount,
                            historyIndex,
                            historyCount,
                            width,
                            height,
                            params.time,
                            alpha * 0.58f,
                            true);
            }
        }
    }
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    drawFlash(width, height, alpha);

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofPopView();
    ofPopStyle();
}

void AuroraCurtainLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void AuroraCurtainLayer::registerFloat(ParameterRegistry& registry,
                                       const std::string& id,
                                       float* target,
                                       float initial,
                                       const std::string& label,
                                       float min,
                                       float max,
                                       float step,
                                       const std::string& units,
                                       const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Aurora Curtains";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.units = units;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void AuroraCurtainLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void AuroraCurtainLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramCurtainCount_ = std::round(ofClamp(paramCurtainCount_, 1.0f, 8.0f));
    paramSampleDensity_ = std::round(ofClamp(paramSampleDensity_, static_cast<float>(kMinSamples), static_cast<float>(kMaxSamples)));
    paramWaveformGain_ = ofClamp(paramWaveformGain_, 0.0f, 4.0f);
    paramVerticalScale_ = ofClamp(paramVerticalScale_, 0.0f, 1.5f);
    paramCurtainHeight_ = ofClamp(paramCurtainHeight_, 0.04f, 0.80f);
    paramFoldStrength_ = ofClamp(paramFoldStrength_, 0.0f, 2.0f);
    paramFlowSpeed_ = ofClamp(paramFlowSpeed_, -2.0f, 2.0f);
    paramNoiseScale_ = ofClamp(paramNoiseScale_, 0.2f, 8.0f);
    paramTrailDecay_ = ofClamp(paramTrailDecay_, 0.02f, 0.85f);
    paramLineThickness_ = ofClamp(paramLineThickness_, 0.5f, 12.0f);
    paramGlowAmount_ = ofClamp(paramGlowAmount_, 0.0f, 4.0f);
    paramShimmerAmount_ = ofClamp(paramShimmerAmount_, 0.0f, 3.0f);
    paramMagneticTilt_ = ofClamp(paramMagneticTilt_, -1.0f, 1.0f);
    paramBassLift_ = ofClamp(paramBassLift_, 0.0f, 2.0f);
    paramMidsFold_ = ofClamp(paramMidsFold_, 0.0f, 2.0f);
    paramHighsSparkle_ = ofClamp(paramHighsSparkle_, 0.0f, 4.0f);
    paramPeakFlash_ = ofClamp(paramPeakFlash_, 0.0f, 2.0f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 3.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramBgAlpha_ = ofClamp(paramBgAlpha_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramColorR_ = ofClamp(paramColorR_, 0.0f, 1.5f);
    paramColorG_ = ofClamp(paramColorG_, 0.0f, 1.5f);
    paramColorB_ = ofClamp(paramColorB_, 0.0f, 1.5f);
    paramColor2R_ = ofClamp(paramColor2R_, 0.0f, 1.5f);
    paramColor2G_ = ofClamp(paramColor2G_, 0.0f, 1.5f);
    paramColor2B_ = ofClamp(paramColor2B_, 0.0f, 1.5f);
}

void AuroraCurtainLayer::ensureWaveformSize(int sampleCount) {
    sampleCount = std::max(kMinSamples, std::min(kMaxSamples, sampleCount));
    if (static_cast<int>(waveform_.size()) == sampleCount && static_cast<int>(targetWaveform_.size()) == sampleCount) {
        return;
    }
    waveform_.assign(static_cast<std::size_t>(sampleCount), 0.0f);
    targetWaveform_.assign(static_cast<std::size_t>(sampleCount), 0.0f);
    history_.clear();
}

void AuroraCurtainLayer::updateAudioState(float dt, float timeSeconds) {
    const int sampleCount = static_cast<int>(std::round(paramSampleDensity_));
    ensureWaveformSize(sampleCount);

    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    const float follow = followAmount(paramAudioSmoothing_);
    const bool snapshotHasWaveform = snapshot.valid && !snapshot.waveform.empty();
    const bool newAudioFrame = snapshot.valid && snapshot.frame != lastAudioFrame_;
    hasAudio_ = snapshot.valid;
    hasWaveform_ = snapshotHasWaveform;

    if (snapshot.valid) {
        level_ = ofLerp(level_, snapshot.level, follow);
        peak_ = ofLerp(peak_, snapshot.peak, follow);
        bass_ = ofLerp(bass_, snapshot.bass, follow);
        mids_ = ofLerp(mids_, snapshot.mids, follow);
        highs_ = ofLerp(highs_, snapshot.highs, follow);
    } else {
        const float release = ofClamp(follow * 0.18f + dt * 0.45f, 0.0f, 1.0f);
        level_ = ofLerp(level_, 0.0f, release);
        peak_ = ofLerp(peak_, 0.0f, release);
        bass_ = ofLerp(bass_, 0.0f, release);
        mids_ = ofLerp(mids_, 0.0f, release);
        highs_ = ofLerp(highs_, 0.0f, release);
    }

    const float denom = static_cast<float>(std::max(1, sampleCount - 1));
    for (int i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / denom;
        const float rawWave = snapshotHasWaveform
            ? std::abs(sampleBuffer(snapshot.waveform, t))
            : 0.0f;
        const float slowField =
            0.42f * ofNoise(t * 1.35f, timeSeconds * 0.035f, 1.0f) +
            0.34f * ofNoise(t * 3.80f, timeSeconds * 0.060f, 2.0f) +
            0.24f * ofNoise(t * 9.50f, timeSeconds * 0.110f, 3.0f);
        const float spectralBias = hasAudio_
            ? bass_ * 0.34f +
                  mids_ * (0.28f + 0.20f * ofNoise(t * 4.0f, timeSeconds * 0.080f, 4.0f)) +
                  highs_ * 0.14f
            : 0.18f;
        const float audioRipple = rawWave * 0.10f;
        const float fallbackEnergy = snapshotHasWaveform ? 0.0f : targetSampleFor(t, timeSeconds) * 0.35f;
        targetWaveform_[static_cast<std::size_t>(i)] = ofClamp(
            slowField * 0.78f + spectralBias * paramAudioAmount_ + audioRipple + fallbackEnergy,
            0.0f,
            1.0f);
    }

    if (waveform_.empty()) {
        waveform_ = targetWaveform_;
    } else {
        for (std::size_t i = 0; i < waveform_.size(); ++i) {
            waveform_[i] = ofLerp(waveform_[i], targetWaveform_[i], follow);
        }
    }

    if (snapshot.valid) {
        lastAudioFrame_ = snapshot.frame;
    }

    if (peak_ > 0.56f && timeSeconds - lastPeakTime_ > 0.18f) {
        flash_ = std::max(flash_, ofClamp(peak_ * paramPeakFlash_, 0.0f, 2.0f));
        flashPhase_ = ofNoise(timeSeconds * 0.19f, 6.7f);
        lastPeakTime_ = timeSeconds;
    }
    flash_ = ofLerp(flash_, 0.0f, ofClamp(dt * 3.2f, 0.0f, 1.0f));

    historyAccumulator_ += dt;
    if (newAudioFrame || historyAccumulator_ >= kHistoryInterval) {
        pushHistory();
        historyAccumulator_ = 0.0f;
    }
}

void AuroraCurtainLayer::pushHistory() {
    if (waveform_.empty()) {
        return;
    }
    history_.push_front(waveform_);
    while (history_.size() > static_cast<std::size_t>(kMaxHistory)) {
        history_.pop_back();
    }
}

float AuroraCurtainLayer::sampleBuffer(const std::vector<float>& buffer, float normalizedIndex) const {
    if (buffer.empty()) {
        return 0.0f;
    }
    const float wrapped = ofClamp(normalizedIndex, 0.0f, 1.0f);
    const float index = wrapped * static_cast<float>(buffer.size() - 1);
    const std::size_t i0 = static_cast<std::size_t>(std::floor(index));
    const std::size_t i1 = std::min<std::size_t>(i0 + 1, buffer.size() - 1);
    const float frac = index - static_cast<float>(i0);
    return ofClamp(ofLerp(buffer[i0], buffer[i1], frac), -1.0f, 1.0f);
}

float AuroraCurtainLayer::targetSampleFor(float normalizedIndex, float timeSeconds) const {
    const float t = normalizedIndex;
    const float scalarEnergy = hasAudio_
        ? ofClamp(level_ * 0.55f + bass_ * 0.30f + mids_ * 0.15f, 0.0f, 1.0f)
        : 0.35f;
    const float slowField =
        0.44f * ofNoise(t * 1.40f, timeSeconds * 0.030f, 11.0f) +
        0.36f * ofNoise(t * 4.20f, timeSeconds * 0.055f, 12.0f) +
        0.20f * ofNoise(t * 10.0f, timeSeconds * 0.090f, 13.0f);
    return ofClamp(slowField * (0.72f + scalarEnergy * 0.35f), 0.0f, 1.0f);
}

glm::vec2 AuroraCurtainLayer::curtainPoint(const std::vector<float>& samples,
                                           int curtainIndex,
                                           int curtainCount,
                                           float t,
                                           float historyAge,
                                           float timeSeconds,
                                           float width,
                                           float height,
                                           bool mirror) const {
    const float energy = ofClamp(sampleBuffer(samples, t), 0.0f, 1.0f);
    const float curtainNorm = (static_cast<float>(curtainIndex) + 0.5f) / static_cast<float>(std::max(1, curtainCount));
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float phase = timeSeconds * paramFlowSpeed_ + static_cast<float>(curtainIndex) * 0.83f + historyAge * 0.19f;
    const float foldAudio = 1.0f + mids_ * paramMidsFold_ * audio;
    const float broadFold = std::sin((t * (1.15f + curtainNorm * 0.65f) + phase * 0.18f) * TWO_PI);
    const float fineFold = signedNoise(t * paramNoiseScale_ * 2.4f + curtainNorm * 1.7f, phase, historyAge * 2.0f);
    const float fold = (broadFold * 0.50f + fineFold * 0.50f) * paramFoldStrength_ * foldAudio;
    const float energyLift =
        energy * paramWaveformGain_ * paramVerticalScale_ * height * 0.050f *
        (0.65f + level_ * audio * 0.28f);
    const float bassLift = bass_ * paramBassLift_ * audio * height * 0.13f;
    const float skyTop = height * 0.10f;
    const float skyBottom = height * 0.62f;
    const float baseY =
        ofLerp(skyBottom, skyTop, curtainNorm * 0.72f)
        - bassLift
        + historyAge * height * 0.012f;
    const float tilt = (t - 0.5f) * paramMagneticTilt_ * height * 0.24f;
    const float y = baseY - energyLift + fold * height * 0.050f + tilt;
    const float sideDrift = signedNoise(t * paramNoiseScale_, curtainNorm * 9.0f, phase * 0.45f) * width * 0.018f * paramFoldStrength_;
    const float x = width * (0.04f + t * 0.92f) + sideDrift;
    if (mirror) {
        return glm::vec2(x, height - y * 0.82f);
    }
    return glm::vec2(x, y);
}

void AuroraCurtainLayer::drawCurtain(const std::vector<float>& samples,
                                     int curtainIndex,
                                     int curtainCount,
                                     int historyIndex,
                                     int historyCount,
                                     float width,
                                     float height,
                                     float timeSeconds,
                                     float alpha,
                                     bool mirror) const {
    if (samples.size() < 2) {
        return;
    }

    const float historyAge = historyCount > 1
        ? static_cast<float>(historyIndex) / static_cast<float>(historyCount - 1)
        : 0.0f;
    const float trail = std::pow(1.0f - paramTrailDecay_, static_cast<float>(historyIndex));
    const float curtainNorm = (static_cast<float>(curtainIndex) + 0.5f) / static_cast<float>(std::max(1, curtainCount));
    const ofFloatColor colorA = colorFrom(paramColorR_, paramColorG_, paramColorB_, 1.0f);
    const ofFloatColor colorB = colorFrom(paramColor2R_, paramColor2G_, paramColor2B_, 1.0f);
    const ofFloatColor topColor = colorA.getLerped(colorB, ofClamp(curtainNorm * 0.8f + historyAge * 0.2f, 0.0f, 1.0f));
    const ofFloatColor bottomColor = colorB.getLerped(colorA, 0.24f + curtainNorm * 0.18f);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float idleLift = hasAudio_ ? 0.0f : 0.22f;
    const float veilAlpha = alpha * trail *
        (0.62f + idleLift * 0.22f + level_ * audio * 0.16f + bass_ * audio * 0.10f);
    const float lowerVeilAlpha = alpha * trail *
        (0.24f + idleLift * 0.12f + bass_ * audio * 0.10f + mids_ * audio * 0.05f);
    const float striationAlpha = alpha * trail *
        (0.24f + idleLift * 0.12f + highs_ * audio * 0.20f + flash_ * 0.04f);
    const float edgeAlpha = alpha * trail *
        (0.20f + idleLift * 0.16f + highs_ * audio * 0.10f + flash_ * 0.04f);
    const int steps = static_cast<int>(samples.size());
    const float denom = static_cast<float>(std::max(1, steps - 1));

    ofMesh veil;
    veil.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    ofMesh innerGlow;
    innerGlow.setMode(OF_PRIMITIVE_LINE_STRIP);
    ofMesh lowerGlow;
    lowerGlow.setMode(OF_PRIMITIVE_LINE_STRIP);
    ofMesh striations;
    striations.setMode(OF_PRIMITIVE_LINES);
    ofMesh edge;
    edge.setMode(OF_PRIMITIVE_LINE_STRIP);
    const int striationStride = std::max(2, steps / 72);

    for (int i = 0; i < steps; ++i) {
        const float t = static_cast<float>(i) / denom;
        const float energy = ofClamp(sampleBuffer(samples, t), 0.0f, 1.0f);
        const glm::vec2 topPoint = curtainPoint(samples, curtainIndex, curtainCount, t, historyAge, timeSeconds, width, height, mirror);
        const float depth = height * paramCurtainHeight_ * (0.50f + curtainNorm * 0.18f + energy * 0.22f + bass_ * paramBassLift_ * audio * 0.18f);
        const glm::vec2 bottomPoint = mirror ? glm::vec2(topPoint.x, topPoint.y - depth) : glm::vec2(topPoint.x, topPoint.y + depth);
        const float shimmer = ofClamp(highs_ * paramHighsSparkle_ * audio + energy * paramShimmerAmount_ * 0.22f, 0.0f, 2.0f);
        ofFloatColor topVertexColor = topColor;
        topVertexColor.a = ofClamp(veilAlpha * (0.74f + energy * 0.32f + shimmer * 0.12f), 0.0f, 0.82f);
        ofFloatColor bottomVertexColor = bottomColor;
        bottomVertexColor.a = ofClamp(lowerVeilAlpha * (0.45f + energy * 0.32f), 0.0f, 0.36f);
        veil.addVertex(glm::vec3(topPoint.x, topPoint.y, 0.0f));
        veil.addColor(topVertexColor);
        veil.addVertex(glm::vec3(bottomPoint.x, bottomPoint.y, 0.0f));
        veil.addColor(bottomVertexColor);

        const glm::vec2 midPoint = topPoint + (bottomPoint - topPoint) * 0.38f;
        ofFloatColor midColor = topColor.getLerped(bottomColor, 0.28f);
        midColor.a = ofClamp(veilAlpha * (0.24f + shimmer * 0.10f), 0.0f, 0.45f);
        innerGlow.addVertex(glm::vec3(midPoint.x, midPoint.y, 0.0f));
        innerGlow.addColor(midColor);

        const glm::vec2 lowerPoint = topPoint + (bottomPoint - topPoint) * 0.68f;
        ofFloatColor lowColor = bottomColor.getLerped(topColor, 0.22f);
        lowColor.a = ofClamp(lowerVeilAlpha * 0.35f, 0.0f, 0.30f);
        lowerGlow.addVertex(glm::vec3(lowerPoint.x, lowerPoint.y, 0.0f));
        lowerGlow.addColor(lowColor);

        if ((i % striationStride) == 0) {
            const float streakNoise = ofNoise(t * 49.0f + curtainNorm * 7.0f, timeSeconds * (0.22f + std::abs(paramFlowSpeed_) * 0.12f), historyAge * 3.1f);
            const float streak = ofClamp((streakNoise - 0.22f) / 0.78f, 0.0f, 1.0f);
            const float rayDepth = ofClamp(0.30f + streak * 0.56f + energy * 0.16f, 0.22f, 1.0f);
            const float verticalSway =
                signedNoise(t * 6.0f, timeSeconds * 0.080f, curtainNorm * 4.0f) *
                width * 0.018f;
            const glm::vec2 rayStart = topPoint;
            const glm::vec2 rayEnd = topPoint + glm::vec2(
                verticalSway,
                (mirror ? -1.0f : 1.0f) * depth * rayDepth * (0.74f + bass_ * audio * 0.24f));
            const ofFloatColor paleGreen = colorFrom(0.72f, 1.0f, 0.78f, 1.0f);
            ofFloatColor rayTop = topColor.getLerped(paleGreen, 0.10f + streak * 0.12f);
            rayTop.a = ofClamp(striationAlpha * (0.36f + streak * 0.48f + shimmer * 0.08f), 0.0f, 0.78f);
            ofFloatColor rayBottom = bottomColor;
            rayBottom.a = ofClamp(striationAlpha * (0.04f + streak * 0.16f), 0.0f, 0.22f);
            striations.addVertex(glm::vec3(rayStart.x, rayStart.y, 0.0f));
            striations.addColor(rayTop);
            striations.addVertex(glm::vec3(rayEnd.x, rayEnd.y, 0.0f));
            striations.addColor(rayBottom);
        }

        ofFloatColor edgeColor = topColor.getLerped(colorB, ofClamp(shimmer * 0.16f, 0.0f, 0.45f));
        edgeColor.a = ofClamp(edgeAlpha * (0.65f + shimmer * 0.28f), 0.0f, 1.0f);
        edge.addVertex(glm::vec3(topPoint.x, topPoint.y, 0.0f));
        edge.addColor(edgeColor);
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    veil.draw();

    ofEnableBlendMode(OF_BLENDMODE_ADD);
#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, paramLineThickness_ * (0.7f + paramGlowAmount_ * 0.25f)));
#endif
    innerGlow.draw();
    lowerGlow.draw();

#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, paramLineThickness_ * 0.58f));
#endif
    striations.draw();

#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, paramLineThickness_ * (0.70f + paramGlowAmount_ * 0.28f)));
#endif
    ofMesh glow = edge;
    const int glowColorCount = static_cast<int>(glow.getNumColors());
    for (int i = 0; i < glowColorCount; ++i) {
        ofFloatColor color = glow.getColor(i);
        color.a *= 0.16f * paramGlowAmount_;
        glow.setColor(i, color);
    }
    glow.draw();

#ifndef TARGET_OPENGLES
    glLineWidth(std::max(0.5f, paramLineThickness_ * 0.55f));
#endif
    if (historyIndex == 0) {
        edge.draw();
    }

    const float sparkle = ofClamp(highs_ * paramHighsSparkle_ * audio * paramShimmerAmount_, 0.0f, 3.0f);
    if (sparkle > 0.01f && historyIndex < 4) {
        const int stride = std::max(3, steps / 54);
        for (int i = 0; i < steps; i += stride) {
            const float t = static_cast<float>(i) / denom;
            const float n = ofNoise(t * 74.0f + static_cast<float>(curtainIndex) * 3.7f, timeSeconds * 5.7f + historyAge);
            if (n < 0.56f) {
                continue;
            }
            const glm::vec2 p = curtainPoint(samples, curtainIndex, curtainCount, t, historyAge, timeSeconds, width, height, mirror);
            ofFloatColor spark = colorB.getLerped(colorFrom(0.72f, 1.0f, 0.84f, 1.0f), 0.35f);
            spark.a = ofClamp(alpha * trail * sparkle * (n - 0.50f) * 0.26f, 0.0f, 0.9f);
            setColor(spark);
            ofDrawCircle(p.x, p.y, 0.7f + sparkle * 0.9f * n);
        }
    }
}

void AuroraCurtainLayer::drawHorizonGlow(float width, float height, float alpha) const {
    ofMesh glow;
    glow.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

    const float y0 = height * 0.52f;
    const float y1 = height * 0.80f;
    const float glowEnergy = hasAudio_
        ? ofClamp(0.30f + bass_ * 0.45f + level_ * 0.20f, 0.0f, 1.0f)
        : 0.34f;

    const ofFloatColor top = colorFrom(
        paramColorR_ * 0.45f,
        paramColorG_ * 0.78f,
        paramColorB_ * 0.55f,
        alpha * (0.08f + glowEnergy * 0.12f));
    const ofFloatColor bottom = colorFrom(paramBgR_, paramBgG_, paramBgB_, 0.0f);

    glow.addVertex(glm::vec3(0.0f, y0, 0.0f));
    glow.addColor(top);
    glow.addVertex(glm::vec3(0.0f, y1, 0.0f));
    glow.addColor(bottom);
    glow.addVertex(glm::vec3(width, y0, 0.0f));
    glow.addColor(top);
    glow.addVertex(glm::vec3(width, y1, 0.0f));
    glow.addColor(bottom);

    glow.draw();
}

void AuroraCurtainLayer::drawFlash(float width, float height, float alpha) const {
    if (flash_ <= 0.002f || paramPeakFlash_ <= 0.0f) {
        return;
    }
    const float x = ofClamp(flashPhase_, 0.0f, 1.0f) * width;
    const float bandWidth = width * (0.08f + flash_ * 0.035f);
    const float y0 = height * 0.10f;
    const float y1 = height * 0.66f;

    ofMesh bloom;
    bloom.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

    const ofFloatColor center = colorFrom(0.50f, 1.0f, 0.70f, ofClamp(alpha * flash_ * 0.13f, 0.0f, 0.30f));
    const ofFloatColor edge = colorFrom(0.30f, 0.75f, 0.55f, 0.0f);

    bloom.addVertex(glm::vec3(x - bandWidth, y0, 0.0f));
    bloom.addColor(edge);
    bloom.addVertex(glm::vec3(x - bandWidth, y1, 0.0f));
    bloom.addColor(edge);
    bloom.addVertex(glm::vec3(x, y0, 0.0f));
    bloom.addColor(center);
    bloom.addVertex(glm::vec3(x, y1, 0.0f));
    bloom.addColor(center);
    bloom.addVertex(glm::vec3(x + bandWidth, y0, 0.0f));
    bloom.addColor(edge);
    bloom.addVertex(glm::vec3(x + bandWidth, y1, 0.0f));
    bloom.addColor(edge);

    bloom.draw();
}

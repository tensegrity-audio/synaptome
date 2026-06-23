#include "ArcticSeaIcebergLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {
    constexpr float kMaxDt = 1.0f / 20.0f;

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, 0.0f, 0.98f);
    }

    float wrap01(float value) {
        while (value < 0.0f) value += 1.0f;
        while (value >= 1.0f) value -= 1.0f;
        return value;
    }

    ofFloatColor colorFrom(float r, float g, float b, float a) {
        return ofFloatColor(ofClamp(r, 0.0f, 1.0f),
                            ofClamp(g, 0.0f, 1.0f),
                            ofClamp(b, 0.0f, 1.0f),
                            ofClamp(a, 0.0f, 1.0f));
    }
}

void ArcticSeaIcebergLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramSeaLevel_ = def.value("seaLevel", paramSeaLevel_);
    paramWaterDepth_ = def.value("waterDepth", paramWaterDepth_);
    paramHorizonGlow_ = def.value("horizonGlow", paramHorizonGlow_);
    paramReflectionStrength_ = def.value("reflectionStrength", paramReflectionStrength_);
    paramWaveAmplitude_ = def.value("waveAmplitude", paramWaveAmplitude_);
    paramWaveFrequency_ = def.value("waveFrequency", paramWaveFrequency_);
    paramPerspectiveLines_ = def.value("perspectiveLines", paramPerspectiveLines_);
    paramIcebergCount_ = def.value("icebergCount", paramIcebergCount_);
    paramIcebergScale_ = def.value("icebergScale", paramIcebergScale_);
    paramDriftSpeed_ = def.value("driftSpeed", paramDriftSpeed_);
    paramRimLight_ = def.value("rimLight", paramRimLight_);
    paramMagentaAccent_ = def.value("magentaAccent", paramMagentaAccent_);
    paramSparkleAmount_ = def.value("sparkleAmount", paramSparkleAmount_);
    paramPeakBloom_ = def.value("peakBloom", paramPeakBloom_);
    paramBassWaves_ = def.value("bassWaves", paramBassWaves_);
    paramMidsDrift_ = def.value("midsDrift", paramMidsDrift_);
    paramHighsSparkle_ = def.value("highsSparkle", paramHighsSparkle_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramSeed_ = def.value("seed", paramSeed_);
    paramWaterR_ = def.value("waterR", paramWaterR_);
    paramWaterG_ = def.value("waterG", paramWaterG_);
    paramWaterB_ = def.value("waterB", paramWaterB_);
    paramRimR_ = def.value("rimR", paramRimR_);
    paramRimG_ = def.value("rimG", paramRimG_);
    paramRimB_ = def.value("rimB", paramRimB_);
    paramAccentR_ = def.value("accentR", paramAccentR_);
    paramAccentG_ = def.value("accentG", paramAccentG_);
    paramAccentB_ = def.value("accentB", paramAccentB_);
    paramAuroraR_ = def.value("auroraR", paramAuroraR_);
    paramAuroraG_ = def.value("auroraG", paramAuroraG_);
    paramAuroraB_ = def.value("auroraB", paramAuroraB_);
    readColor(def, "waterColor", paramWaterR_, paramWaterG_, paramWaterB_);
    readColor(def, "rimColor", paramRimR_, paramRimG_, paramRimB_);
    readColor(def, "accentColor", paramAccentR_, paramAccentG_, paramAccentB_);
    readColor(def, "auroraColor", paramAuroraR_, paramAuroraG_, paramAuroraB_);
    clampParams();
}

void ArcticSeaIcebergLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.arcticSeaIcebergs" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Arctic Sea Icebergs";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Alpha: Arctic Sea", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".seaLevel", &paramSeaLevel_, paramSeaLevel_, "Scale: Sea Level", 0.48f, 0.82f, 0.01f);
    registerFloat(registry, prefix + ".waterDepth", &paramWaterDepth_, paramWaterDepth_, "Scale: Water Depth", 0.12f, 0.52f, 0.01f);
    registerFloat(registry, prefix + ".horizonGlow", &paramHorizonGlow_, paramHorizonGlow_, "Glow: Horizon", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".reflectionStrength", &paramReflectionStrength_, paramReflectionStrength_, "Glow: Reflection Strength", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".waveAmplitude", &paramWaveAmplitude_, paramWaveAmplitude_, "Scale: Wave Amplitude", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".waveFrequency", &paramWaveFrequency_, paramWaveFrequency_, "Time: Wave Frequency", 1.0f, 18.0f, 0.1f);
    registerFloat(registry, prefix + ".perspectiveLines", &paramPerspectiveLines_, paramPerspectiveLines_, "Alpha: Perspective Lines", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".icebergCount", &paramIcebergCount_, paramIcebergCount_, "Count: Icebergs", 0.0f, 18.0f, 1.0f);
    registerFloat(registry, prefix + ".icebergScale", &paramIcebergScale_, paramIcebergScale_, "Scale: Icebergs", 0.2f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".driftSpeed", &paramDriftSpeed_, paramDriftSpeed_, "Motion: Iceberg Drift", -0.25f, 0.25f, 0.001f);
    registerFloat(registry, prefix + ".rimLight", &paramRimLight_, paramRimLight_, "Glow: Rim Light", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".magentaAccent", &paramMagentaAccent_, paramMagentaAccent_, "Color: Magenta Accent", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".sparkleAmount", &paramSparkleAmount_, paramSparkleAmount_, "Glow: Water Sparkle", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".peakBloom", &paramPeakBloom_, paramPeakBloom_, "Audio: Peak Bloom", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".bassWaves", &paramBassWaves_, paramBassWaves_, "Audio: Bass Waves", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".midsDrift", &paramMidsDrift_, paramMidsDrift_, "Audio: Mids Drift", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".highsSparkle", &paramHighsSparkle_, paramHighsSparkle_, "Audio: Highs Sparkle", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Audio: Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Audio: Smoothing", 0.0f, 0.98f, 0.01f);

    meta = {};
    meta.group = "Arctic Sea Icebergs";
    meta.label = "Action: Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: Icebergs", 0.0f, 99999.0f, 1.0f);
    registerFloat(registry, prefix + ".waterR", &paramWaterR_, paramWaterR_, "Color: Water R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waterG", &paramWaterG_, paramWaterG_, "Color: Water G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waterB", &paramWaterB_, paramWaterB_, "Color: Water B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".rimR", &paramRimR_, paramRimR_, "Color: Rim R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".rimG", &paramRimG_, paramRimG_, "Color: Rim G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".rimB", &paramRimB_, paramRimB_, "Color: Rim B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".accentR", &paramAccentR_, paramAccentR_, "Color: Accent R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".accentG", &paramAccentG_, paramAccentG_, "Color: Accent G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".accentB", &paramAccentB_, paramAccentB_, "Color: Accent B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".auroraR", &paramAuroraR_, paramAuroraR_, "Color: Aurora Reflection R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".auroraG", &paramAuroraG_, paramAuroraG_, "Color: Aurora Reflection G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".auroraB", &paramAuroraB_, paramAuroraB_, "Color: Aurora Reflection B", 0.0f, 1.5f, 0.01f);

    resetIcebergs();
}

void ArcticSeaIcebergLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    updateAudioState(ofClamp(params.dt, 0.0f, kMaxDt));
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    driftPhase_ += ofClamp(params.dt, 0.0f, kMaxDt) * (1.0f + mids_ * paramMidsDrift_ * audio);

    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    const int desiredCount = static_cast<int>(std::round(paramIcebergCount_));
    if (desiredSeed != seedState_ || desiredCount != static_cast<int>(icebergs_.size()) || paramReseedRequested_) {
        resetIcebergs();
        paramReseedRequested_ = false;
    }
}

void ArcticSeaIcebergLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (icebergs_.empty() && paramIcebergCount_ > 0.0f) {
        resetIcebergs();
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    drawHorizon(width, height, alpha, params.time);
    drawWater(width, height, alpha, params.time);

    std::vector<Iceberg> sorted = icebergs_;
    std::sort(sorted.begin(), sorted.end(), [](const Iceberg& a, const Iceberg& b) {
        return a.depth < b.depth;
    });
    for (const auto& iceberg : sorted) {
        drawIceberg(iceberg, width, height, alpha, params.time);
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    drawPeakRipple(width, height, alpha);

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofPopView();
    ofPopStyle();
}

void ArcticSeaIcebergLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void ArcticSeaIcebergLayer::registerFloat(ParameterRegistry& registry,
                                          const std::string& id,
                                          float* target,
                                          float initial,
                                          const std::string& label,
                                          float min,
                                          float max,
                                          float step,
                                          const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Arctic Sea Icebergs";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void ArcticSeaIcebergLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void ArcticSeaIcebergLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramSeaLevel_ = ofClamp(paramSeaLevel_, 0.48f, 0.82f);
    paramWaterDepth_ = ofClamp(paramWaterDepth_, 0.12f, 0.52f);
    paramHorizonGlow_ = ofClamp(paramHorizonGlow_, 0.0f, 2.0f);
    paramReflectionStrength_ = ofClamp(paramReflectionStrength_, 0.0f, 2.0f);
    paramWaveAmplitude_ = ofClamp(paramWaveAmplitude_, 0.0f, 2.0f);
    paramWaveFrequency_ = ofClamp(paramWaveFrequency_, 1.0f, 18.0f);
    paramPerspectiveLines_ = ofClamp(paramPerspectiveLines_, 0.0f, 1.0f);
    paramIcebergCount_ = std::round(ofClamp(paramIcebergCount_, 0.0f, 18.0f));
    paramIcebergScale_ = ofClamp(paramIcebergScale_, 0.2f, 2.0f);
    paramDriftSpeed_ = ofClamp(paramDriftSpeed_, -0.25f, 0.25f);
    paramRimLight_ = ofClamp(paramRimLight_, 0.0f, 2.0f);
    paramMagentaAccent_ = ofClamp(paramMagentaAccent_, 0.0f, 1.0f);
    paramSparkleAmount_ = ofClamp(paramSparkleAmount_, 0.0f, 2.0f);
    paramPeakBloom_ = ofClamp(paramPeakBloom_, 0.0f, 2.0f);
    paramBassWaves_ = ofClamp(paramBassWaves_, 0.0f, 2.0f);
    paramMidsDrift_ = ofClamp(paramMidsDrift_, 0.0f, 2.0f);
    paramHighsSparkle_ = ofClamp(paramHighsSparkle_, 0.0f, 3.0f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 2.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999.0f));
    paramWaterR_ = ofClamp(paramWaterR_, 0.0f, 1.0f);
    paramWaterG_ = ofClamp(paramWaterG_, 0.0f, 1.0f);
    paramWaterB_ = ofClamp(paramWaterB_, 0.0f, 1.0f);
    paramRimR_ = ofClamp(paramRimR_, 0.0f, 1.5f);
    paramRimG_ = ofClamp(paramRimG_, 0.0f, 1.5f);
    paramRimB_ = ofClamp(paramRimB_, 0.0f, 1.5f);
    paramAccentR_ = ofClamp(paramAccentR_, 0.0f, 1.5f);
    paramAccentG_ = ofClamp(paramAccentG_, 0.0f, 1.5f);
    paramAccentB_ = ofClamp(paramAccentB_, 0.0f, 1.5f);
    paramAuroraR_ = ofClamp(paramAuroraR_, 0.0f, 1.5f);
    paramAuroraG_ = ofClamp(paramAuroraG_, 0.0f, 1.5f);
    paramAuroraB_ = ofClamp(paramAuroraB_, 0.0f, 1.5f);
}

void ArcticSeaIcebergLayer::resetIcebergs() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    std::mt19937 rng(seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    const int count = static_cast<int>(std::round(paramIcebergCount_));
    icebergs_.clear();
    icebergs_.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        Iceberg iceberg;
        iceberg.x = unit(rng);
        iceberg.depth = std::pow(unit(rng), 0.72f);
        iceberg.width = 0.70f + unit(rng) * 0.95f;
        iceberg.height = 0.70f + unit(rng) * 1.10f;
        iceberg.speed = (0.35f + unit(rng) * 0.65f) * (unit(rng) > 0.5f ? 1.0f : -1.0f);
        iceberg.seed = unit(rng) * 1000.0f;
        iceberg.lean = (unit(rng) - 0.5f) * 0.34f;
        icebergs_.push_back(iceberg);
    }
}

void ArcticSeaIcebergLayer::updateAudioState(float dt) {
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    const float follow = followAmount(paramAudioSmoothing_);
    hasAudio_ = snapshot.valid;
    if (snapshot.valid) {
        level_ = ofLerp(level_, snapshot.level, follow);
        peak_ = ofLerp(peak_, snapshot.peak, follow);
        bass_ = ofLerp(bass_, snapshot.bass, follow);
        mids_ = ofLerp(mids_, snapshot.mids, follow);
        highs_ = ofLerp(highs_, snapshot.highs, follow);
    } else {
        const float release = ofClamp(dt * 1.6f + follow * 0.12f, 0.0f, 1.0f);
        level_ = ofLerp(level_, 0.0f, release);
        peak_ = ofLerp(peak_, 0.0f, release);
        bass_ = ofLerp(bass_, 0.0f, release);
        mids_ = ofLerp(mids_, 0.0f, release);
        highs_ = ofLerp(highs_, 0.0f, release);
    }

    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    ripple_ = std::max(ripple_, ofClamp(peak_ * paramPeakBloom_ * audio, 0.0f, 1.2f));
    ripple_ = ofLerp(ripple_, 0.0f, ofClamp(dt * 2.2f, 0.0f, 1.0f));
}

void ArcticSeaIcebergLayer::drawHorizon(float width, float height, float alpha, float timeSeconds) const {
    const float y = seaY(height);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float glow = paramHorizonGlow_ * (0.45f + level_ * 0.26f * audio + bass_ * 0.22f * audio);
    ofMesh haze;
    haze.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    const ofFloatColor aurora = colorFrom(paramAuroraR_, paramAuroraG_, paramAuroraB_, alpha * glow * 0.22f);
    const ofFloatColor clear = colorFrom(paramWaterR_, paramWaterG_, paramWaterB_, 0.0f);
    haze.addVertex(glm::vec3(0.0f, y - height * 0.10f, 0.0f));
    haze.addColor(clear);
    haze.addVertex(glm::vec3(0.0f, y + height * 0.08f, 0.0f));
    haze.addColor(aurora);
    haze.addVertex(glm::vec3(width, y - height * 0.10f, 0.0f));
    haze.addColor(clear);
    haze.addVertex(glm::vec3(width, y + height * 0.08f, 0.0f));
    haze.addColor(aurora);
    haze.draw();

    ofEnableBlendMode(OF_BLENDMODE_ADD);
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f + glow * 1.4f);
#endif
    ofSetColor(colorFrom(paramAuroraR_, paramAuroraG_, paramAuroraB_, alpha * glow * 0.25f));
    const float shimmer = std::sin(timeSeconds * 0.7f + mids_ * audio * 2.0f) * height * 0.002f;
    ofDrawLine(0.0f, y + shimmer, width, y - shimmer);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void ArcticSeaIcebergLayer::drawWater(float width, float height, float alpha, float timeSeconds) const {
    const float y = seaY(height);
    const float bottom = height;
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float wave = paramWaveAmplitude_ * (0.45f + bass_ * paramBassWaves_ * audio);

    ofMesh water;
    water.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    const ofFloatColor nearColor = colorFrom(paramWaterR_ * 0.55f, paramWaterG_ * 0.78f, paramWaterB_ * 1.18f, alpha * 0.92f);
    const ofFloatColor farColor = colorFrom(paramWaterR_ * 1.20f, paramWaterG_ * 1.22f, paramWaterB_ * 1.05f, alpha * 0.78f);
    water.addVertex(glm::vec3(0.0f, y, 0.0f));
    water.addColor(farColor);
    water.addVertex(glm::vec3(0.0f, bottom, 0.0f));
    water.addColor(nearColor);
    water.addVertex(glm::vec3(width, y, 0.0f));
    water.addColor(farColor);
    water.addVertex(glm::vec3(width, bottom, 0.0f));
    water.addColor(nearColor);
    water.draw();

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofMesh reflections;
    reflections.setMode(OF_PRIMITIVE_LINES);
    const int bands = 34;
    for (int i = 0; i < bands; ++i) {
        const float d = static_cast<float>(i) / static_cast<float>(std::max(1, bands - 1));
        const float perspective = d * d;
        const float yy = ofLerp(y + height * 0.012f, bottom, perspective);
        const float widthFactor = ofLerp(0.16f, 0.92f, perspective);
        const float cx = width * (0.5f + std::sin(timeSeconds * 0.09f + d * 7.0f) * 0.03f);
        const float segment = width * widthFactor * (0.52f + 0.18f * std::sin(d * paramWaveFrequency_ + timeSeconds * 0.9f));
        const float sway = std::sin(d * paramWaveFrequency_ * 2.1f + timeSeconds * (0.8f + wave * 0.12f)) * height * 0.004f * wave;
        ofFloatColor color = colorFrom(paramAuroraR_, paramAuroraG_, paramAuroraB_,
            alpha * paramReflectionStrength_ * (1.0f - d) * (0.08f + level_ * 0.08f * audio));
        reflections.addVertex(glm::vec3(cx - segment * 0.5f, yy + sway, 0.0f));
        reflections.addColor(color);
        reflections.addVertex(glm::vec3(cx + segment * 0.5f, yy - sway, 0.0f));
        reflections.addColor(color);
    }
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f);
#endif
    reflections.draw();

    if (paramPerspectiveLines_ > 0.0f) {
        ofMesh lines;
        lines.setMode(OF_PRIMITIVE_LINES);
        for (int i = 0; i < 14; ++i) {
            const float d = static_cast<float>(i) / 13.0f;
            const float yy = ofLerp(y + height * 0.02f, bottom, d * d);
            ofFloatColor color = colorFrom(paramRimR_, paramRimG_, paramRimB_, alpha * paramPerspectiveLines_ * (1.0f - d) * 0.10f);
            lines.addVertex(glm::vec3(0.0f, yy, 0.0f));
            lines.addColor(color);
            lines.addVertex(glm::vec3(width, yy, 0.0f));
            lines.addColor(color);
        }
        lines.draw();
    }

    const float sparkle = ofClamp(paramSparkleAmount_ * (0.10f + highs_ * paramHighsSparkle_ * audio), 0.0f, 2.0f);
    if (sparkle > 0.01f) {
        ofSetColor(colorFrom(paramRimR_, paramRimG_, paramRimB_, alpha * sparkle * 0.22f));
        for (int i = 0; i < 42; ++i) {
            const float n = ofNoise(static_cast<float>(i) * 8.7f, timeSeconds * 1.7f);
            if (n < 0.66f) {
                continue;
            }
            const float d = ofNoise(static_cast<float>(i) * 2.1f, 13.0f);
            const float x = wrap01(ofNoise(static_cast<float>(i) * 5.2f, 2.0f) + timeSeconds * 0.01f) * width;
            const float yy = ofLerp(y + height * 0.04f, bottom, d * d);
            ofDrawCircle(x, yy, 0.4f + n * 1.3f);
        }
    }
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void ArcticSeaIcebergLayer::drawIceberg(const Iceberg& iceberg, float width, float height, float alpha, float timeSeconds) const {
    const float ySea = seaY(height);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float depth = ofClamp(iceberg.depth, 0.0f, 1.0f);
    const float drift = driftPhase_ * paramDriftSpeed_ * iceberg.speed * (0.38f + depth * 0.90f);
    const float x = wrap01(iceberg.x + drift) * width;
    const float baseY = ofLerp(ySea + height * 0.035f, height * 0.94f, depth * depth);
    const float scale = paramIcebergScale_ * (0.38f + depth * 1.36f);
    const float bergHeight = height * 0.12f * iceberg.height * scale;
    const float bergWidth = width * 0.10f * iceberg.width * scale;
    const float wave = std::sin(timeSeconds * (0.7f + mids_ * audio) + iceberg.seed) * height * 0.006f * (1.0f + bass_ * audio);
    const float base = baseY + wave;
    const float tip = base - bergHeight;

    std::vector<glm::vec2> points;
    points.push_back(glm::vec2(x - bergWidth * 0.55f, base));
    points.push_back(glm::vec2(x - bergWidth * 0.40f, ofLerp(base, tip, 0.42f + 0.10f * ofNoise(iceberg.seed, 1.0f))));
    points.push_back(glm::vec2(x - bergWidth * 0.16f + iceberg.lean * bergWidth, tip + bergHeight * 0.08f * ofNoise(iceberg.seed, 2.0f)));
    points.push_back(glm::vec2(x + bergWidth * 0.10f + iceberg.lean * bergWidth, ofLerp(base, tip, 0.90f)));
    points.push_back(glm::vec2(x + bergWidth * 0.36f, ofLerp(base, tip, 0.48f + 0.10f * ofNoise(iceberg.seed, 3.0f))));
    points.push_back(glm::vec2(x + bergWidth * 0.58f, base));

    ofMesh body;
    body.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    body.addVertex(glm::vec3(x, base - bergHeight * 0.40f, 0.0f));
    body.addColor(colorFrom(0.018f, 0.048f, 0.082f, alpha * (0.72f + depth * 0.24f)));
    for (const auto& p : points) {
        const float side = p.x < x ? 0.0f : 1.0f;
        body.addVertex(glm::vec3(p.x, p.y, 0.0f));
        body.addColor(colorFrom(0.035f + side * 0.020f, 0.080f + side * 0.034f, 0.130f + depth * 0.050f, alpha * (0.78f + depth * 0.18f)));
    }
    body.addVertex(glm::vec3(points.front().x, points.front().y, 0.0f));
    body.addColor(colorFrom(0.035f, 0.080f, 0.130f + depth * 0.050f, alpha * (0.78f + depth * 0.18f)));
    body.draw();

    ofEnableBlendMode(OF_BLENDMODE_ADD);
#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, 0.8f + depth * 2.0f));
#endif
    ofSetColor(colorFrom(paramRimR_, paramRimG_, paramRimB_, alpha * paramRimLight_ * (0.30f + depth * 0.32f)));
    for (int i = 0; i + 1 < static_cast<int>(points.size()); ++i) {
        const glm::vec2 a = points[static_cast<std::size_t>(i)];
        const glm::vec2 b = points[static_cast<std::size_t>(i + 1)];
        ofDrawLine(a.x, a.y, b.x, b.y);
    }
    ofSetColor(colorFrom(paramAccentR_, paramAccentG_, paramAccentB_, alpha * paramMagentaAccent_ * (0.12f + depth * 0.18f)));
    ofDrawLine(points.front().x, base + height * 0.004f, points.back().x, base - height * 0.003f);

    ofMesh reflection;
    reflection.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    const int strips = 8;
    for (int i = 0; i < strips; ++i) {
        const float d = static_cast<float>(i) / static_cast<float>(std::max(1, strips - 1));
        const float yy = base + d * bergHeight * 1.25f * (0.75f - depth * 0.25f);
        const float spread = bergWidth * (0.54f + d * 0.22f);
        const float wobble = std::sin(d * 9.0f + timeSeconds * 1.1f + iceberg.seed) * bergWidth * 0.05f * (1.0f + bass_ * audio);
        const float a = alpha * paramReflectionStrength_ * (1.0f - d) * (0.08f + depth * 0.12f + level_ * audio * 0.05f);
        const ofFloatColor color = colorFrom(paramAuroraR_, paramAuroraG_, paramAuroraB_, a);
        reflection.addVertex(glm::vec3(x - spread * 0.5f + wobble, yy, 0.0f));
        reflection.addColor(color);
        reflection.addVertex(glm::vec3(x + spread * 0.5f + wobble, yy, 0.0f));
        reflection.addColor(color);
    }
    reflection.draw();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}

void ArcticSeaIcebergLayer::drawPeakRipple(float width, float height, float alpha) const {
    if (ripple_ <= 0.004f) {
        return;
    }
    const float y = seaY(height);
    const float radius = width * (0.12f + ripple_ * 0.16f);
    const float cx = width * (0.50f + std::sin(driftPhase_ * 0.7f) * 0.18f);
    ofSetColor(colorFrom(paramRimR_, paramRimG_, paramRimB_, alpha * ripple_ * 0.18f));
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f + ripple_ * 3.0f);
#endif
    ofNoFill();
    ofDrawEllipse(cx, y + height * 0.16f, radius, radius * 0.18f);
    ofFill();
}

float ArcticSeaIcebergLayer::seaY(float height) const {
    return height * paramSeaLevel_;
}

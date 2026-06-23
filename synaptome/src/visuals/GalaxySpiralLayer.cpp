#include "GalaxySpiralLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {
    constexpr float kMinDt = 0.0f;
    constexpr float kMaxDt = 1.0f / 20.0f;

    float gaussianish(std::mt19937& rng) {
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        return (unit(rng) + unit(rng) + unit(rng) + unit(rng) - 2.0f) * 0.5f;
    }

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, 0.0f, 0.98f);
    }

    ofFloatColor colorFrom(float r, float g, float b, float a) {
        return ofFloatColor(ofClamp(r, 0.0f, 1.0f),
                            ofClamp(g, 0.0f, 1.0f),
                            ofClamp(b, 0.0f, 1.0f),
                            ofClamp(a, 0.0f, 1.0f));
    }

    void setColor(const ofFloatColor& color) {
        ofSetColor(static_cast<int>(ofClamp(color.r, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.g, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.b, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.a, 0.0f, 1.0f) * 255.0f));
    }
}

void GalaxySpiralLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramStarCount_ = def.value("starCount", paramStarCount_);
    paramArms_ = def.value("arms", paramArms_);
    paramRadius_ = def.value("radius", paramRadius_);
    paramCoreRadius_ = def.value("coreRadius", paramCoreRadius_);
    paramArmTightness_ = def.value("armTightness", paramArmTightness_);
    paramArmWidth_ = def.value("armWidth", paramArmWidth_);
    paramInclination_ = def.value("inclination", paramInclination_);
    paramRotationSpeed_ = def.value("rotationSpeed", paramRotationSpeed_);
    paramStarSize_ = def.value("starSize", paramStarSize_);
    paramStarBrightness_ = def.value("starBrightness", paramStarBrightness_);
    paramDustAlpha_ = def.value("dustAlpha", paramDustAlpha_);
    paramCoreGlow_ = def.value("coreGlow", paramCoreGlow_);
    paramTwinkle_ = def.value("twinkle", paramTwinkle_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramBassExpansion_ = def.value("bassExpansion", paramBassExpansion_);
    paramMidsTwist_ = def.value("midsTwist", paramMidsTwist_);
    paramHighsSparkle_ = def.value("highsSparkle", paramHighsSparkle_);
    paramWaveformWarp_ = def.value("waveformWarp", paramWaveformWarp_);
    paramPeakPulseThreshold_ = def.value("peakPulseThreshold", paramPeakPulseThreshold_);
    paramPulseCooldown_ = def.value("pulseCooldown", paramPulseCooldown_);
    paramPulseSpeed_ = def.value("pulseSpeed", paramPulseSpeed_);
    paramPulseWidth_ = def.value("pulseWidth", paramPulseWidth_);
    paramPulseStrength_ = def.value("pulseStrength", paramPulseStrength_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramSeed_ = def.value("seed", paramSeed_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    paramBgR_ = def.value("bgR", paramBgR_);
    paramBgG_ = def.value("bgG", paramBgG_);
    paramBgB_ = def.value("bgB", paramBgB_);
    paramStarR_ = def.value("starR", paramStarR_);
    paramStarG_ = def.value("starG", paramStarG_);
    paramStarB_ = def.value("starB", paramStarB_);
    paramCoreR_ = def.value("coreR", paramCoreR_);
    paramCoreG_ = def.value("coreG", paramCoreG_);
    paramCoreB_ = def.value("coreB", paramCoreB_);
    paramDustR_ = def.value("dustR", paramDustR_);
    paramDustG_ = def.value("dustG", paramDustG_);
    paramDustB_ = def.value("dustB", paramDustB_);
    paramPulseR_ = def.value("pulseR", paramPulseR_);
    paramPulseG_ = def.value("pulseG", paramPulseG_);
    paramPulseB_ = def.value("pulseB", paramPulseB_);
    readColor(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(def, "starColor", paramStarR_, paramStarG_, paramStarB_);
    readColor(def, "coreColor", paramCoreR_, paramCoreG_, paramCoreB_);
    readColor(def, "dustColor", paramDustR_, paramDustG_, paramDustB_);
    readColor(def, "pulseColor", paramPulseR_, paramPulseG_, paramPulseB_);
    clampParams();
}

void GalaxySpiralLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.galaxySpiral" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Galaxy Spiral";
    meta.label = "Galaxy Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Galaxy Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starCount", &paramStarCount_, paramStarCount_, "Galaxy Stars", 96.0f, 3000.0f, 1.0f);
    registerFloat(registry, prefix + ".arms", &paramArms_, paramArms_, "Galaxy Arms", 1.0f, 8.0f, 1.0f);
    registerFloat(registry, prefix + ".radius", &paramRadius_, paramRadius_, "Galaxy Radius", 0.1f, 1.25f, 0.01f);
    registerFloat(registry, prefix + ".coreRadius", &paramCoreRadius_, paramCoreRadius_, "Galaxy Core Radius", 0.02f, 0.45f, 0.01f);
    registerFloat(registry, prefix + ".armTightness", &paramArmTightness_, paramArmTightness_, "Galaxy Arm Tightness", -8.0f, 8.0f, 0.01f);
    registerFloat(registry, prefix + ".armWidth", &paramArmWidth_, paramArmWidth_, "Galaxy Arm Width", 0.01f, 1.2f, 0.01f);
    registerFloat(registry, prefix + ".inclination", &paramInclination_, paramInclination_, "Galaxy Inclination", 0.2f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".rotationSpeed", &paramRotationSpeed_, paramRotationSpeed_, "Galaxy Rotation", -0.25f, 0.25f, 0.001f, "Revolutions per second.");
    registerFloat(registry, prefix + ".starSize", &paramStarSize_, paramStarSize_, "Galaxy Star Size", 0.4f, 6.0f, 0.05f);
    registerFloat(registry, prefix + ".starBrightness", &paramStarBrightness_, paramStarBrightness_, "Galaxy Star Brightness", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".dustAlpha", &paramDustAlpha_, paramDustAlpha_, "Galaxy Dust Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".coreGlow", &paramCoreGlow_, paramCoreGlow_, "Galaxy Core Glow", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".twinkle", &paramTwinkle_, paramTwinkle_, "Galaxy Twinkle", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Galaxy Audio Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".bassExpansion", &paramBassExpansion_, paramBassExpansion_, "Galaxy Bass Expansion", -0.5f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".midsTwist", &paramMidsTwist_, paramMidsTwist_, "Galaxy Mids Twist", -3.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".highsSparkle", &paramHighsSparkle_, paramHighsSparkle_, "Galaxy Highs Sparkle", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".waveformWarp", &paramWaveformWarp_, paramWaveformWarp_, "Galaxy Waveform Warp", 0.0f, 0.3f, 0.001f);
    registerFloat(registry, prefix + ".peakPulseThreshold", &paramPeakPulseThreshold_, paramPeakPulseThreshold_, "Galaxy Peak Threshold", 0.01f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".pulseCooldown", &paramPulseCooldown_, paramPulseCooldown_, "Galaxy Pulse Cooldown", 0.0f, 2.0f, 0.01f, "Seconds between audio peak pulses.");
    registerFloat(registry, prefix + ".pulseSpeed", &paramPulseSpeed_, paramPulseSpeed_, "Galaxy Pulse Speed", 0.1f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".pulseWidth", &paramPulseWidth_, paramPulseWidth_, "Galaxy Pulse Width", 0.01f, 0.35f, 0.001f);
    registerFloat(registry, prefix + ".pulseStrength", &paramPulseStrength_, paramPulseStrength_, "Galaxy Pulse Strength", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Galaxy Audio Smoothing", 0.0f, 0.98f, 0.01f);

    meta = {};
    meta.group = "Galaxy Spiral";
    meta.label = "Galaxy Reseed";
    meta.description = "Respawn galaxy stars using the current seed.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Galaxy Seed", 0.0f, 99999.0f, 1.0f);
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Galaxy Bg Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Galaxy Bg R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Galaxy Bg G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Galaxy Bg B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starR", &paramStarR_, paramStarR_, "Galaxy Star R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".starG", &paramStarG_, paramStarG_, "Galaxy Star G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".starB", &paramStarB_, paramStarB_, "Galaxy Star B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coreR", &paramCoreR_, paramCoreR_, "Galaxy Core R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coreG", &paramCoreG_, paramCoreG_, "Galaxy Core G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coreB", &paramCoreB_, paramCoreB_, "Galaxy Core B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".dustR", &paramDustR_, paramDustR_, "Galaxy Dust R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".dustG", &paramDustG_, paramDustG_, "Galaxy Dust G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".dustB", &paramDustB_, paramDustB_, "Galaxy Dust B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".pulseR", &paramPulseR_, paramPulseR_, "Galaxy Pulse R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".pulseG", &paramPulseG_, paramPulseG_, "Galaxy Pulse G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".pulseB", &paramPulseB_, paramPulseB_, "Galaxy Pulse B", 0.0f, 1.5f, 0.01f);

    resetStars();
}

void GalaxySpiralLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    updateAudioState();

    const int desiredCount = static_cast<int>(std::round(paramStarCount_));
    const int desiredArms = static_cast<int>(std::round(paramArms_));
    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    if (desiredCount != static_cast<int>(stars_.size()) || desiredArms != armState_ || desiredSeed != seedState_ || paramReseedRequested_) {
        resetStars();
        paramReseedRequested_ = false;
    }

    const float dt = ofClamp(params.dt, kMinDt, kMaxDt);
    const float speed = std::max(0.0f, params.speed);
    rotationPhase_ += dt * paramRotationSpeed_ * speed * TWO_PI * (1.0f + bass_ * paramAudioAmount_ * 0.65f);
    updatePulses(dt);

    if (hasAudio_ && peak_ >= paramPeakPulseThreshold_ && params.time - lastPulseTime_ >= paramPulseCooldown_) {
        triggerPulse(peak_ * (0.65f + paramAudioAmount_ * 0.7f));
        lastPulseTime_ = params.time;
    }
}

void GalaxySpiralLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (stars_.empty()) {
        resetStars();
    }
    if (stars_.empty()) {
        return;
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float minDim = std::min(width, height);
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const float radiusPx = minDim * 0.5f * paramRadius_;
    const ofFloatColor bgColor = colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * params.slotOpacity);
    const ofFloatColor starColor = colorFrom(paramStarR_, paramStarG_, paramStarB_, 1.0f);
    const ofFloatColor coreColor = colorFrom(paramCoreR_, paramCoreG_, paramCoreB_, 1.0f);
    const ofFloatColor dustColor = colorFrom(paramDustR_, paramDustG_, paramDustB_, 1.0f);
    const ofFloatColor pulseColor = colorFrom(paramPulseR_, paramPulseG_, paramPulseB_, 1.0f);

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    if (bgColor.a > 0.0f) {
        setColor(bgColor);
        ofDrawRectangle(0.0f, 0.0f, width, height);
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofSetCircleResolution(18);

    if (paramCoreGlow_ > 0.0f) {
        const float corePulse = 1.0f + level_ * paramAudioAmount_ * 0.42f + bass_ * paramAudioAmount_ * 0.25f;
        const float coreRadius = std::max(2.0f, radiusPx * paramCoreRadius_ * corePulse);
        for (int i = 3; i >= 1; --i) {
            const float t = static_cast<float>(i) / 3.0f;
            ofFloatColor glow = coreColor;
            glow.a = alpha * paramCoreGlow_ * (0.075f + 0.065f * t);
            setColor(glow);
            ofDrawCircle(center.x, center.y, coreRadius * (1.4f + t * 3.4f));
        }
        ofFloatColor core = coreColor;
        core.a = alpha * ofClamp(0.55f + level_ * paramAudioAmount_ * 0.35f, 0.0f, 1.0f);
        setColor(core);
        ofDrawCircle(center.x, center.y, coreRadius);
    }

    for (std::size_t i = 0; i < stars_.size(); ++i) {
        const Star& star = stars_[i];
        const glm::vec2 pos = starPosition(i, star, center, radiusPx);
        if (pos.x < -radiusPx || pos.x > width + radiusPx || pos.y < -radiusPx || pos.y > height + radiusPx) {
            continue;
        }

        const float pulse = pulseForRadius(star.radius);
        const float twinkle = ofNoise(star.seed, params.time * (0.25f + paramTwinkle_ * 0.55f));
        const float sparkle = highs_ * paramHighsSparkle_ * paramAudioAmount_;
        const float innerMix = 1.0f - ofClamp(star.radius / std::max(0.02f, paramCoreRadius_ * 2.2f), 0.0f, 1.0f);
        const float armMix = ofClamp(star.heat * 0.55f + sparkle * 0.08f, 0.0f, 1.0f);
        ofFloatColor color = dustColor.getLerped(starColor, armMix);
        color = color.getLerped(coreColor, innerMix);
        color = color.getLerped(pulseColor, ofClamp(pulse * 0.85f + sparkle * 0.06f, 0.0f, 1.0f));

        const float dustA = paramDustAlpha_ * star.dust * alpha * (0.18f + twinkle * 0.18f);
        if (dustA > 0.006f) {
            ofFloatColor dust = dustColor;
            dust.a = dustA;
            setColor(dust);
            ofDrawCircle(pos.x, pos.y, paramStarSize_ * (2.8f + star.size * 3.0f) * (0.75f + star.radius));
        }

        const float brightness = star.brightness * paramStarBrightness_ *
                                 (0.56f + twinkle * paramTwinkle_ * 0.42f + level_ * paramAudioAmount_ * 0.22f + sparkle * 0.18f + pulse * 0.75f);
        const float starAlpha = ofClamp(alpha * brightness, 0.0f, 1.0f);
        const float starRadius = std::max(0.35f, paramStarSize_ * (0.35f + star.size * 0.9f) * (1.0f + sparkle * 0.18f + pulse * 0.45f));
        if (starAlpha <= 0.002f) {
            continue;
        }

        if (starRadius > 1.1f || pulse > 0.05f || sparkle > 0.18f) {
            ofFloatColor halo = color;
            halo.a = ofClamp(starAlpha * (0.12f + pulse * 0.24f), 0.0f, 0.65f);
            setColor(halo);
            ofDrawCircle(pos.x, pos.y, starRadius * (2.1f + pulse * 2.0f));
        }

        color.a = starAlpha;
        setColor(color);
        ofDrawCircle(pos.x, pos.y, starRadius);
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofPopView();
    ofPopStyle();
}

void GalaxySpiralLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void GalaxySpiralLayer::registerFloat(ParameterRegistry& registry,
                                      const std::string& id,
                                      float* target,
                                      float initial,
                                      const std::string& label,
                                      float min,
                                      float max,
                                      float step,
                                      const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Galaxy Spiral";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void GalaxySpiralLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void GalaxySpiralLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramStarCount_ = std::round(ofClamp(paramStarCount_, 96.0f, 3000.0f));
    paramArms_ = std::round(ofClamp(paramArms_, 1.0f, 8.0f));
    paramRadius_ = ofClamp(paramRadius_, 0.1f, 1.25f);
    paramCoreRadius_ = ofClamp(paramCoreRadius_, 0.02f, 0.45f);
    paramArmTightness_ = ofClamp(paramArmTightness_, -8.0f, 8.0f);
    paramArmWidth_ = ofClamp(paramArmWidth_, 0.01f, 1.2f);
    paramInclination_ = ofClamp(paramInclination_, 0.2f, 1.0f);
    paramRotationSpeed_ = ofClamp(paramRotationSpeed_, -0.25f, 0.25f);
    paramStarSize_ = ofClamp(paramStarSize_, 0.4f, 6.0f);
    paramStarBrightness_ = ofClamp(paramStarBrightness_, 0.0f, 3.0f);
    paramDustAlpha_ = ofClamp(paramDustAlpha_, 0.0f, 1.0f);
    paramCoreGlow_ = ofClamp(paramCoreGlow_, 0.0f, 1.5f);
    paramTwinkle_ = ofClamp(paramTwinkle_, 0.0f, 2.0f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 2.0f);
    paramBassExpansion_ = ofClamp(paramBassExpansion_, -0.5f, 1.5f);
    paramMidsTwist_ = ofClamp(paramMidsTwist_, -3.0f, 3.0f);
    paramHighsSparkle_ = ofClamp(paramHighsSparkle_, 0.0f, 4.0f);
    paramWaveformWarp_ = ofClamp(paramWaveformWarp_, 0.0f, 0.3f);
    paramPeakPulseThreshold_ = ofClamp(paramPeakPulseThreshold_, 0.01f, 1.0f);
    paramPulseCooldown_ = ofClamp(paramPulseCooldown_, 0.0f, 2.0f);
    paramPulseSpeed_ = ofClamp(paramPulseSpeed_, 0.1f, 4.0f);
    paramPulseWidth_ = ofClamp(paramPulseWidth_, 0.01f, 0.35f);
    paramPulseStrength_ = ofClamp(paramPulseStrength_, 0.0f, 3.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999.0f));
    paramBgAlpha_ = ofClamp(paramBgAlpha_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramStarR_ = ofClamp(paramStarR_, 0.0f, 1.5f);
    paramStarG_ = ofClamp(paramStarG_, 0.0f, 1.5f);
    paramStarB_ = ofClamp(paramStarB_, 0.0f, 1.5f);
    paramCoreR_ = ofClamp(paramCoreR_, 0.0f, 1.5f);
    paramCoreG_ = ofClamp(paramCoreG_, 0.0f, 1.5f);
    paramCoreB_ = ofClamp(paramCoreB_, 0.0f, 1.5f);
    paramDustR_ = ofClamp(paramDustR_, 0.0f, 1.5f);
    paramDustG_ = ofClamp(paramDustG_, 0.0f, 1.5f);
    paramDustB_ = ofClamp(paramDustB_, 0.0f, 1.5f);
    paramPulseR_ = ofClamp(paramPulseR_, 0.0f, 1.5f);
    paramPulseG_ = ofClamp(paramPulseG_, 0.0f, 1.5f);
    paramPulseB_ = ofClamp(paramPulseB_, 0.0f, 1.5f);
}

void GalaxySpiralLayer::resetStars() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    armState_ = static_cast<int>(std::round(paramArms_));

    std::mt19937 rng(seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    const int count = static_cast<int>(std::round(paramStarCount_));
    stars_.assign(static_cast<std::size_t>(count), {});
    for (std::size_t i = 0; i < stars_.size(); ++i) {
        Star& star = stars_[i];
        const bool coreStar = unit(rng) < 0.16f;
        const float radial = coreStar
            ? std::pow(unit(rng), 2.4f) * paramCoreRadius_ * 1.45f
            : std::pow(unit(rng), 0.72f);
        star.radius = ofClamp(radial, 0.0f, 1.0f);
        star.armIndex = static_cast<float>(static_cast<int>(i % static_cast<std::size_t>(std::max(1, armState_))));
        star.armOffset = gaussianish(rng);
        star.angleOffset = gaussianish(rng) * 0.12f + unit(rng) * 0.03f;
        star.size = 0.35f + std::pow(unit(rng), 2.1f) * 1.65f;
        star.brightness = 0.25f + std::pow(unit(rng), 0.45f) * 0.9f;
        star.heat = ofClamp(0.32f + unit(rng) * 0.68f - star.radius * 0.18f, 0.0f, 1.0f);
        star.dust = ofClamp(std::pow(unit(rng), 0.55f) * (0.3f + star.radius * 0.9f), 0.0f, 1.0f);
        star.seed = unit(rng) * 10000.0f;
    }
    pulses_.clear();
}

void GalaxySpiralLayer::updateAudioState() {
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    const float follow = followAmount(paramAudioSmoothing_);
    hasAudio_ = snapshot.valid;
    if (!snapshot.valid) {
        level_ = ofLerp(level_, 0.0f, follow * 0.25f);
        peak_ = ofLerp(peak_, 0.0f, follow * 0.25f);
        bass_ = ofLerp(bass_, 0.0f, follow * 0.25f);
        mids_ = ofLerp(mids_, 0.0f, follow * 0.25f);
        highs_ = ofLerp(highs_, 0.0f, follow * 0.25f);
        hasWaveform_ = false;
        return;
    }

    level_ = ofLerp(level_, snapshot.level, follow);
    peak_ = ofLerp(peak_, snapshot.peak, follow);
    bass_ = ofLerp(bass_, snapshot.bass, follow);
    mids_ = ofLerp(mids_, snapshot.mids, follow);
    highs_ = ofLerp(highs_, snapshot.highs, follow);
    if (snapshot.frame != lastAudioFrame_) {
        waveform_ = snapshot.waveform;
        hasWaveform_ = !waveform_.empty();
        lastAudioFrame_ = snapshot.frame;
    }
}

void GalaxySpiralLayer::updatePulses(float dt) {
    for (auto& pulse : pulses_) {
        pulse.age += dt;
    }
    pulses_.erase(std::remove_if(pulses_.begin(), pulses_.end(), [&](const Pulse& pulse) {
        return pulse.age * paramPulseSpeed_ > 1.55f || pulse.strength <= 0.001f;
    }), pulses_.end());
}

void GalaxySpiralLayer::triggerPulse(float strength) {
    Pulse pulse;
    pulse.strength = ofClamp(strength, 0.0f, 3.0f);
    pulses_.push_back(pulse);
    if (pulses_.size() > 8) {
        pulses_.erase(pulses_.begin());
    }
}

glm::vec2 GalaxySpiralLayer::starPosition(std::size_t index, const Star& star, const glm::vec2& center, float radiusPx) const {
    const float arms = std::max(1.0f, paramArms_);
    const float armAngle = (star.armIndex / arms) * TWO_PI;
    const float audio = paramAudioAmount_;
    const float radiusWeight = 0.35f + (1.0f - star.radius) * 0.65f;
    const float bassScale = 1.0f + bass_ * paramBassExpansion_ * audio * radiusWeight;
    const float waveformWarp = hasWaveform_ ? waveformSampleFor(index) * paramWaveformWarp_ * audio * (0.35f + level_) : 0.0f;
    const float pulse = pulseForRadius(star.radius);
    const float warpedRadius = ofClamp(star.radius * bassScale + waveformWarp + pulse * paramPulseStrength_ * 0.035f, 0.0f, 1.35f);
    const float tightness = (paramArmTightness_ + mids_ * paramMidsTwist_ * audio) * TWO_PI;
    const float differential = rotationPhase_ * (0.22f + (1.0f - star.radius) * 0.78f);
    const float scatter = star.armOffset * paramArmWidth_ * (0.35f + star.radius * 0.85f);
    const float angle = armAngle + star.radius * tightness + scatter + star.angleOffset + differential;
    const float x = std::cos(angle) * warpedRadius;
    const float y = std::sin(angle) * warpedRadius * paramInclination_;
    return center + glm::vec2(x, y) * radiusPx;
}

float GalaxySpiralLayer::waveformSampleFor(std::size_t index) const {
    if (waveform_.empty()) {
        return 0.0f;
    }
    return ofClamp(waveform_[index % waveform_.size()], -1.0f, 1.0f);
}

float GalaxySpiralLayer::pulseForRadius(float radius) const {
    float value = 0.0f;
    for (const auto& pulse : pulses_) {
        const float pulseRadius = pulse.age * paramPulseSpeed_;
        const float ring = std::max(0.0f, 1.0f - std::abs(radius - pulseRadius) / std::max(0.001f, paramPulseWidth_));
        const float decay = std::exp(-pulse.age * 1.75f);
        value = std::max(value, ring * decay * pulse.strength * paramPulseStrength_);
    }
    return ofClamp(value, 0.0f, 1.0f);
}

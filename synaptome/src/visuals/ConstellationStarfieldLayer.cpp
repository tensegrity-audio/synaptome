#include "ConstellationStarfieldLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <random>

namespace {
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

void ConstellationStarfieldLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramStarCount_ = def.value("starCount", paramStarCount_);
    paramConstellationCount_ = def.value("constellationCount", paramConstellationCount_);
    paramConstellationAlpha_ = def.value("constellationAlpha", paramConstellationAlpha_);
    paramConstellationReach_ = def.value("constellationReach", paramConstellationReach_);
    paramStarSize_ = def.value("starSize", paramStarSize_);
    paramDepthParallax_ = def.value("depthParallax", paramDepthParallax_);
    paramDriftSpeed_ = def.value("driftSpeed", paramDriftSpeed_);
    paramTwinkle_ = def.value("twinkle", paramTwinkle_);
    paramHighsTwinkle_ = def.value("highsTwinkle", paramHighsTwinkle_);
    paramMidsLines_ = def.value("midsLines", paramMidsLines_);
    paramBassDimming_ = def.value("bassDimming", paramBassDimming_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramSeed_ = def.value("seed", paramSeed_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    paramBgR_ = def.value("bgR", paramBgR_);
    paramBgG_ = def.value("bgG", paramBgG_);
    paramBgB_ = def.value("bgB", paramBgB_);
    paramStarR_ = def.value("starR", paramStarR_);
    paramStarG_ = def.value("starG", paramStarG_);
    paramStarB_ = def.value("starB", paramStarB_);
    paramLineR_ = def.value("lineR", paramLineR_);
    paramLineG_ = def.value("lineG", paramLineG_);
    paramLineB_ = def.value("lineB", paramLineB_);
    readColor(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(def, "starColor", paramStarR_, paramStarG_, paramStarB_);
    readColor(def, "lineColor", paramLineR_, paramLineG_, paramLineB_);
    clampParams();
}

void ConstellationStarfieldLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.constellationStarfield" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Constellation Starfield";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Alpha: Starfield", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starCount", &paramStarCount_, paramStarCount_, "Count: Stars", 24.0f, 900.0f, 1.0f);
    registerFloat(registry, prefix + ".constellationCount", &paramConstellationCount_, paramConstellationCount_, "Count: Constellations", 0.0f, 18.0f, 1.0f);
    registerFloat(registry, prefix + ".constellationAlpha", &paramConstellationAlpha_, paramConstellationAlpha_, "Alpha: Constellations", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".constellationReach", &paramConstellationReach_, paramConstellationReach_, "Scale: Constellation Reach", 0.04f, 0.42f, 0.01f);
    registerFloat(registry, prefix + ".starSize", &paramStarSize_, paramStarSize_, "Scale: Star Size", 0.1f, 5.0f, 0.05f);
    registerFloat(registry, prefix + ".depthParallax", &paramDepthParallax_, paramDepthParallax_, "Motion: Depth Parallax", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".driftSpeed", &paramDriftSpeed_, paramDriftSpeed_, "Motion: Star Drift", -0.2f, 0.2f, 0.001f);
    registerFloat(registry, prefix + ".twinkle", &paramTwinkle_, paramTwinkle_, "Glow: Twinkle", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".highsTwinkle", &paramHighsTwinkle_, paramHighsTwinkle_, "Audio: Highs Twinkle", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".midsLines", &paramMidsLines_, paramMidsLines_, "Audio: Mids Lines", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".bassDimming", &paramBassDimming_, paramBassDimming_, "Audio: Bass Dimming", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Audio: Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Audio: Smoothing", 0.0f, 0.98f, 0.01f);

    meta = {};
    meta.group = "Constellation Starfield";
    meta.label = "Action: Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: Starfield", 0.0f, 99999.0f, 1.0f);
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Alpha: Background", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Color: Background R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Color: Background G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Color: Background B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starR", &paramStarR_, paramStarR_, "Color: Star R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".starG", &paramStarG_, paramStarG_, "Color: Star G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".starB", &paramStarB_, paramStarB_, "Color: Star B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".lineR", &paramLineR_, paramLineR_, "Color: Constellation R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".lineG", &paramLineG_, paramLineG_, "Color: Constellation G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".lineB", &paramLineB_, paramLineB_, "Color: Constellation B", 0.0f, 1.5f, 0.01f);

    resetStars();
}

void ConstellationStarfieldLayer::update(const LayerUpdateParams& params) {
    (void)params;
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    updateAudioState();

    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    const int desiredCount = static_cast<int>(std::round(paramStarCount_));
    if (desiredSeed != seedState_ || desiredCount != static_cast<int>(stars_.size()) || paramReseedRequested_) {
        resetStars();
        paramReseedRequested_ = false;
    }
}

void ConstellationStarfieldLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (stars_.empty()) {
        resetStars();
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float auroraDimming = ofClamp(1.0f - (bass_ * paramBassDimming_ + level_ * 0.18f) * audio, 0.35f, 1.0f);
    const ofFloatColor starBase = colorFrom(paramStarR_, paramStarG_, paramStarB_, 1.0f);
    const ofFloatColor violetTint = colorFrom(0.62f, 0.48f, 1.0f, 1.0f);
    const ofFloatColor lineBase = colorFrom(paramLineR_, paramLineG_, paramLineB_, 1.0f);

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    if (paramBgAlpha_ > 0.0f) {
        ofMesh sky;
        sky.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
        sky.addVertex(glm::vec3(0.0f, 0.0f, 0.0f));
        sky.addColor(colorFrom(paramBgR_ * 0.65f, paramBgG_ * 0.75f, paramBgB_ * 1.35f, paramBgAlpha_ * alpha));
        sky.addVertex(glm::vec3(0.0f, height, 0.0f));
        sky.addColor(colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * alpha * 0.52f));
        sky.addVertex(glm::vec3(width, 0.0f, 0.0f));
        sky.addColor(colorFrom(paramBgR_ * 0.65f, paramBgG_ * 0.75f, paramBgB_ * 1.35f, paramBgAlpha_ * alpha));
        sky.addVertex(glm::vec3(width, height, 0.0f));
        sky.addColor(colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * alpha * 0.52f));
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        sky.draw();
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);

    ofMesh lineMesh;
    lineMesh.setMode(OF_PRIMITIVE_LINES);
    const float lineAudio = ofClamp(0.52f + mids_ * paramMidsLines_ * audio + peak_ * 0.12f, 0.0f, 1.4f);
    for (const auto& link : links_) {
        if (link.a < 0 || link.b < 0 || link.a >= static_cast<int>(stars_.size()) || link.b >= static_cast<int>(stars_.size())) {
            continue;
        }
        const Star& a = stars_[static_cast<std::size_t>(link.a)];
        const Star& b = stars_[static_cast<std::size_t>(link.b)];
        const glm::vec2 pa = projectedStar(a, width, height, params.time);
        const glm::vec2 pb = projectedStar(b, width, height, params.time);
        ofFloatColor color = lineBase.getLerped(violetTint, 0.18f + 0.25f * b.tint);
        color.a = ofClamp(alpha * paramConstellationAlpha_ * link.strength * lineAudio * auroraDimming, 0.0f, 0.42f);
        lineMesh.addVertex(glm::vec3(pa.x, pa.y, 0.0f));
        lineMesh.addColor(color);
        lineMesh.addVertex(glm::vec3(pb.x, pb.y, 0.0f));
        lineMesh.addColor(color);
    }
#ifndef TARGET_OPENGLES
    glLineWidth(1.0f);
#endif
    lineMesh.draw();

    for (const auto& star : stars_) {
        const glm::vec2 p = projectedStar(star, width, height, params.time);
        if (p.y > height * 0.78f) {
            continue;
        }
        const float twinkleNoise = ofNoise(star.phase, params.time * (0.28f + highs_ * paramHighsTwinkle_ * audio), star.depth * 5.0f);
        const float twinkle = 0.72f + (twinkleNoise - 0.5f) * paramTwinkle_ * (0.45f + highs_ * audio);
        const float depthScale = 0.45f + star.depth * 0.95f;
        const float radius = std::max(0.35f, paramStarSize_ * star.size * depthScale);
        ofFloatColor color = starBase.getLerped(violetTint, star.tint * 0.28f);
        color.a = ofClamp(alpha * star.brightness * twinkle * auroraDimming, 0.0f, 0.92f);
        ofSetColor(color);
        ofDrawCircle(p.x, p.y, radius);
        if (star.brightness > 0.82f && radius > 0.75f) {
            color.a *= 0.22f;
            ofSetColor(color);
            ofDrawCircle(p.x, p.y, radius * (2.5f + star.depth));
        }
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofPopView();
    ofPopStyle();
}

void ConstellationStarfieldLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void ConstellationStarfieldLayer::registerFloat(ParameterRegistry& registry,
                                                const std::string& id,
                                                float* target,
                                                float initial,
                                                const std::string& label,
                                                float min,
                                                float max,
                                                float step,
                                                const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Constellation Starfield";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void ConstellationStarfieldLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void ConstellationStarfieldLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramStarCount_ = std::round(ofClamp(paramStarCount_, 24.0f, 900.0f));
    paramConstellationCount_ = std::round(ofClamp(paramConstellationCount_, 0.0f, 18.0f));
    paramConstellationAlpha_ = ofClamp(paramConstellationAlpha_, 0.0f, 1.0f);
    paramConstellationReach_ = ofClamp(paramConstellationReach_, 0.04f, 0.42f);
    paramStarSize_ = ofClamp(paramStarSize_, 0.1f, 5.0f);
    paramDepthParallax_ = ofClamp(paramDepthParallax_, 0.0f, 1.0f);
    paramDriftSpeed_ = ofClamp(paramDriftSpeed_, -0.2f, 0.2f);
    paramTwinkle_ = ofClamp(paramTwinkle_, 0.0f, 2.0f);
    paramHighsTwinkle_ = ofClamp(paramHighsTwinkle_, 0.0f, 3.0f);
    paramMidsLines_ = ofClamp(paramMidsLines_, 0.0f, 2.0f);
    paramBassDimming_ = ofClamp(paramBassDimming_, 0.0f, 1.0f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 2.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999.0f));
    paramBgAlpha_ = ofClamp(paramBgAlpha_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramStarR_ = ofClamp(paramStarR_, 0.0f, 1.5f);
    paramStarG_ = ofClamp(paramStarG_, 0.0f, 1.5f);
    paramStarB_ = ofClamp(paramStarB_, 0.0f, 1.5f);
    paramLineR_ = ofClamp(paramLineR_, 0.0f, 1.5f);
    paramLineG_ = ofClamp(paramLineG_, 0.0f, 1.5f);
    paramLineB_ = ofClamp(paramLineB_, 0.0f, 1.5f);
}

void ConstellationStarfieldLayer::resetStars() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    std::mt19937 rng(seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    const int count = static_cast<int>(std::round(paramStarCount_));
    stars_.clear();
    stars_.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        Star star;
        star.pos = glm::vec2(unit(rng), std::pow(unit(rng), 1.28f) * 0.78f);
        star.depth = std::pow(unit(rng), 0.72f);
        star.size = 0.45f + unit(rng) * 1.25f;
        star.brightness = 0.28f + std::pow(unit(rng), 2.0f) * 0.92f;
        star.tint = unit(rng);
        star.phase = unit(rng) * 1000.0f;
        stars_.push_back(star);
    }

    std::vector<int> bright;
    for (int i = 0; i < static_cast<int>(stars_.size()); ++i) {
        if (stars_[static_cast<std::size_t>(i)].brightness > 0.62f && stars_[static_cast<std::size_t>(i)].pos.y < 0.66f) {
            bright.push_back(i);
        }
    }
    std::shuffle(bright.begin(), bright.end(), rng);

    links_.clear();
    const int groups = std::min(static_cast<int>(std::round(paramConstellationCount_)), static_cast<int>(bright.size() / 3));
    std::size_t cursor = 0;
    for (int group = 0; group < groups; ++group) {
        const int groupSize = 3 + static_cast<int>(unit(rng) * 3.0f);
        if (cursor + static_cast<std::size_t>(groupSize) > bright.size()) {
            break;
        }
        std::vector<int> indices(bright.begin() + static_cast<std::ptrdiff_t>(cursor),
                                 bright.begin() + static_cast<std::ptrdiff_t>(cursor + static_cast<std::size_t>(groupSize)));
        cursor += static_cast<std::size_t>(groupSize);
        std::sort(indices.begin(), indices.end(), [this](int a, int b) {
            return stars_[static_cast<std::size_t>(a)].pos.x < stars_[static_cast<std::size_t>(b)].pos.x;
        });
        for (int i = 0; i + 1 < static_cast<int>(indices.size()); ++i) {
            const glm::vec2 pa = stars_[static_cast<std::size_t>(indices[static_cast<std::size_t>(i)])].pos;
            const glm::vec2 pb = stars_[static_cast<std::size_t>(indices[static_cast<std::size_t>(i + 1)])].pos;
            const float dist = glm::length(pa - pb);
            if (dist <= paramConstellationReach_) {
                Link link;
                link.a = indices[static_cast<std::size_t>(i)];
                link.b = indices[static_cast<std::size_t>(i + 1)];
                link.strength = ofClamp(1.0f - dist / std::max(0.001f, paramConstellationReach_), 0.16f, 1.0f);
                links_.push_back(link);
            }
        }
    }
}

void ConstellationStarfieldLayer::updateAudioState() {
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
        level_ = ofLerp(level_, 0.0f, 0.08f);
        peak_ = ofLerp(peak_, 0.0f, 0.08f);
        bass_ = ofLerp(bass_, 0.0f, 0.08f);
        mids_ = ofLerp(mids_, 0.0f, 0.08f);
        highs_ = ofLerp(highs_, 0.0f, 0.08f);
    }
}

glm::vec2 ConstellationStarfieldLayer::projectedStar(const Star& star, float width, float height, float timeSeconds) const {
    const float depthSpeed = ofLerp(0.25f, 1.0f, star.depth);
    const float x = wrap01(star.pos.x + timeSeconds * paramDriftSpeed_ * depthSpeed);
    const float horizonPull = (star.depth - 0.5f) * paramDepthParallax_ * 0.05f;
    const float y = ofClamp(star.pos.y + horizonPull + std::sin(timeSeconds * 0.018f + star.phase) * 0.006f * paramDepthParallax_, 0.0f, 0.82f);
    return glm::vec2(x * width, y * height);
}

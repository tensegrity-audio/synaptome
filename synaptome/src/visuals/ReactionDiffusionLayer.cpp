#include "ReactionDiffusionLayer.h"

#include "ofGraphics.h"
#include "ofUtils.h"

#include <algorithm>
#include <cmath>

namespace {
    void readColorArray(const ofJson& def, const char* key, float& r, float& g, float& b) {
        if (def.contains(key) && def[key].is_array() && def[key].size() >= 3) {
            r = def[key][0].get<float>();
            g = def[key][1].get<float>();
            b = def[key][2].get<float>();
        }
    }

    float wrapIndex(float value, float limit) {
        while (value < 0.0f) value += limit;
        while (value >= limit) value -= limit;
        return value;
    }
}

void ReactionDiffusionLayer::configure(const ofJson& config) {
    if (config.contains("defaults") && config["defaults"].is_object()) {
        const auto& def = config["defaults"];
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramPaused_ = def.value("paused", paramPaused_);
        paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
        paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
        paramFeedRate_ = def.value("feedRate", paramFeedRate_);
        paramKillRate_ = def.value("killRate", paramKillRate_);
        paramDiffusionA_ = def.value("diffusionA", paramDiffusionA_);
        paramDiffusionB_ = def.value("diffusionB", paramDiffusionB_);
        paramInjectionRate_ = def.value("injectionRate", paramInjectionRate_);
        paramInjectionAmount_ = def.value("injectionAmount", paramInjectionAmount_);
        paramInjectionRadius_ = def.value("injectionRadius", paramInjectionRadius_);
        paramSeed_ = def.value("seed", paramSeed_);
        paramSeedDensity_ = def.value("seedDensity", paramSeedDensity_);
        paramContourThreshold_ = def.value("contourThreshold", paramContourThreshold_);
        paramContourWidth_ = def.value("contourWidth", paramContourWidth_);
        paramFieldScale_ = def.value("fieldScale", paramFieldScale_);
        paramBackgroundAlpha_ = def.value("backgroundAlpha", paramBackgroundAlpha_);
        paramFieldAlpha_ = def.value("fieldAlpha", paramFieldAlpha_);
        paramContourOpacity_ = def.value("contourOpacity", paramContourOpacity_);
        paramBgR_ = def.value("bgR", paramBgR_);
        paramBgG_ = def.value("bgG", paramBgG_);
        paramBgB_ = def.value("bgB", paramBgB_);
        paramFieldR_ = def.value("fieldR", paramFieldR_);
        paramFieldG_ = def.value("fieldG", paramFieldG_);
        paramFieldB_ = def.value("fieldB", paramFieldB_);
        paramContourR_ = def.value("contourR", paramContourR_);
        paramContourG_ = def.value("contourG", paramContourG_);
        paramContourB_ = def.value("contourB", paramContourB_);
        readColorArray(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
        readColorArray(def, "fieldColor", paramFieldR_, paramFieldG_, paramFieldB_);
        readColorArray(def, "contourColor", paramContourR_, paramContourG_, paramContourB_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(32, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(32, config["textureSize"][1].get<int>());
    }
}

void ReactionDiffusionLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.reactionDiffusion" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".speed", &paramSpeed_, paramSpeed_, "Time: Simulation Speed", 0.0f, 80.0f, 0.1f);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: BPM Sync";
    meta.description = "Use transport BPM instead of free-running simulation speed.";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);

    registerFloat(registry, prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, "Time: BPM Mult", 0.25f, 16.0f, 0.25f);
    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Visibility: Layer Opacity", 0.0f, 1.0f, 0.01f);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: Paused";
    registry.addBool(prefix + ".paused", &paramPaused_, paramPaused_, meta);

    meta.label = "Action: Reseed";
    meta.description = "Reset the chemical field and seed fresh B chemical patches.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    meta.label = "Action: Auto Reseed";
    meta.description = "Reset on a transport-quantized cadence.";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, meta);

    registerFloat(registry, prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, "Time: Auto Reseed Beats", 1.0f, 128.0f, 1.0f);
    registerFloat(registry, prefix + ".feedRate", &paramFeedRate_, paramFeedRate_, "Growth: Feed Rate", 0.0f, 0.09f, 0.001f);
    registerFloat(registry, prefix + ".killRate", &paramKillRate_, paramKillRate_, "Growth: Kill Rate", 0.0f, 0.09f, 0.001f);
    registerFloat(registry, prefix + ".diffusionA", &paramDiffusionA_, paramDiffusionA_, "Force: Diffusion A", 0.0f, 1.2f, 0.01f);
    registerFloat(registry, prefix + ".diffusionB", &paramDiffusionB_, paramDiffusionB_, "Force: Diffusion B", 0.0f, 0.8f, 0.01f);
    registerFloat(registry, prefix + ".injectionRate", &paramInjectionRate_, paramInjectionRate_, "Growth: Injection Rate", 0.0f, 0.2f, 0.001f);
    registerFloat(registry, prefix + ".injectionAmount", &paramInjectionAmount_, paramInjectionAmount_, "Growth: Injection Amount", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".injectionRadius", &paramInjectionRadius_, paramInjectionRadius_, "Scale: Injection Radius", 1.0f, 24.0f, 1.0f);
    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: Pattern Seed", 0.0f, 999999.0f, 1.0f,
                  "Deterministic field seed used when the chemical simulation is reset.");
    registerFloat(registry, prefix + ".seedDensity", &paramSeedDensity_, paramSeedDensity_, "Seed: Patch Density", 0.0f, 0.5f, 0.01f);
    registerFloat(registry, prefix + ".contourThreshold", &paramContourThreshold_, paramContourThreshold_, "Scale: Contour Threshold", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".contourWidth", &paramContourWidth_, paramContourWidth_, "Scale: Contour Width", 0.01f, 0.4f, 0.01f);
    registerFloat(registry, prefix + ".fieldScale", &paramFieldScale_, paramFieldScale_, "Scale: Field Gain", 0.1f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".backgroundAlpha", &paramBackgroundAlpha_, paramBackgroundAlpha_, "Visibility: Background Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldAlpha", &paramFieldAlpha_, paramFieldAlpha_, "Visibility: Field Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".contourOpacity", &paramContourOpacity_, paramContourOpacity_, "Visibility: Contour Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Color: Background R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Color: Background G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Color: Background B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldR", &paramFieldR_, paramFieldR_, "Color: Field R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldG", &paramFieldG_, paramFieldG_, "Color: Field G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldB", &paramFieldB_, paramFieldB_, "Color: Field B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".contourR", &paramContourR_, paramContourR_, "Color: Contour R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".contourG", &paramContourG_, paramContourG_, "Color: Contour G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".contourB", &paramContourB_, paramContourB_, "Color: Contour B", 0.0f, 1.0f, 0.01f);

    allocateField();
    resetField();
    syncTexture();
}

void ReactionDiffusionLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    clampParams();

    if (paramReseedRequested_) {
        resetField();
        paramReseedRequested_ = false;
    }

    const float beatPosition = currentBeatPosition(params.time, params.bpm);
    if (paramAutoReseed_ && params.bpm > 0.0f) {
        if (nextAutoReseedBeat_ < 0.0f) {
            const float interval = std::max(1.0f, paramAutoReseedEveryBeats_);
            nextAutoReseedBeat_ = std::floor(beatPosition / interval) * interval + interval;
        }
        while (beatPosition >= nextAutoReseedBeat_) {
            resetField();
            nextAutoReseedBeat_ += std::max(1.0f, paramAutoReseedEveryBeats_);
        }
    } else {
        nextAutoReseedBeat_ = -1.0f;
    }

    const float stepRate = paramPaused_ ? 0.0f : stepRateFor(params);
    if (stepRate <= 0.0f) {
        if (dirty_) {
            syncTexture();
            dirty_ = false;
        }
        return;
    }

    stepAccumulator_ += params.dt * stepRate;
    int iterations = std::min(32, static_cast<int>(std::floor(stepAccumulator_)));
    if (iterations <= 0) {
        if (dirty_) {
            syncTexture();
            dirty_ = false;
        }
        return;
    }

    stepAccumulator_ -= static_cast<float>(iterations);
    for (int i = 0; i < iterations; ++i) {
        stepSimulation();
        if (paramInjectionRate_ > 0.0f && randomUnit() < paramInjectionRate_) {
            injectChemical();
        }
    }

    syncTexture();
    dirty_ = false;
}

void ReactionDiffusionLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || !texture_.isAllocated() || params.slotOpacity <= 0.0f) return;

    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    ofSetColor(255, 255, 255, static_cast<int>(alpha * 255.0f));
    texture_.draw(0, 0, params.viewport.x, params.viewport.y);
    ofPopView();
    ofPopStyle();
}

void ReactionDiffusionLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void ReactionDiffusionLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void ReactionDiffusionLayer::allocateField() {
    const std::size_t count = static_cast<std::size_t>(textureSize_.x * textureSize_.y);
    field_.assign(count, {});
    next_.assign(count, {});
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    dirty_ = true;
}

void ReactionDiffusionLayer::resetField() {
    if (field_.empty()) {
        allocateField();
    }
    rng_.seed(activeSeed());
    for (auto& cell : field_) {
        cell.a = 1.0f;
        cell.b = 0.0f;
    }

    const int centerRadius = std::max(2, static_cast<int>(std::round(paramInjectionRadius_ * 1.7f)));
    seedPatch(textureSize_.x / 2, textureSize_.y / 2, centerRadius, 1.0f);

    const int patchCount = std::max(1, static_cast<int>(std::round(paramSeedDensity_ * 32.0f)));
    for (int i = 0; i < patchCount; ++i) {
        seedPatch(randomInt(0, std::max(0, textureSize_.x - 1)),
                  randomInt(0, std::max(0, textureSize_.y - 1)),
                  std::max(2, static_cast<int>(std::round(randomRange(2.0f, paramInjectionRadius_ + 2.0f)))),
                  randomRange(0.45f, 1.0f));
    }
    next_ = field_;
    stepAccumulator_ = 0.0f;
    dirty_ = true;
}

void ReactionDiffusionLayer::seedPatch(int centerX, int centerY, int radius, float amount) {
    const int safeRadius = std::max(1, radius);
    const float radius2 = static_cast<float>(safeRadius * safeRadius);
    for (int dy = -safeRadius; dy <= safeRadius; ++dy) {
        for (int dx = -safeRadius; dx <= safeRadius; ++dx) {
            const float d2 = static_cast<float>(dx * dx + dy * dy);
            if (d2 > radius2) continue;
            const int x = static_cast<int>(wrapIndex(static_cast<float>(centerX + dx), static_cast<float>(textureSize_.x)));
            const int y = static_cast<int>(wrapIndex(static_cast<float>(centerY + dy), static_cast<float>(textureSize_.y)));
            const float falloff = 1.0f - std::sqrt(d2) / static_cast<float>(safeRadius);
            Cell& cell = field_[static_cast<std::size_t>(indexFor(x, y))];
            cell.b = ofClamp(cell.b + amount * (0.35f + falloff * 0.65f), 0.0f, 1.0f);
            cell.a = ofClamp(cell.a - amount * falloff * 0.25f, 0.0f, 1.0f);
        }
    }
}

void ReactionDiffusionLayer::stepSimulation() {
    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const int idx = indexFor(x, y);
            const Cell& cell = field_[static_cast<std::size_t>(idx)];
            const float reaction = cell.a * cell.b * cell.b;
            const float nextA = cell.a + paramDiffusionA_ * laplacianA(x, y) - reaction + paramFeedRate_ * (1.0f - cell.a);
            const float nextB = cell.b + paramDiffusionB_ * laplacianB(x, y) + reaction - (paramKillRate_ + paramFeedRate_) * cell.b;
            next_[static_cast<std::size_t>(idx)].a = ofClamp(nextA, 0.0f, 1.0f);
            next_[static_cast<std::size_t>(idx)].b = ofClamp(nextB, 0.0f, 1.0f);
        }
    }
    field_.swap(next_);
    dirty_ = true;
}

void ReactionDiffusionLayer::injectChemical() {
    seedPatch(randomInt(0, std::max(0, textureSize_.x - 1)),
              randomInt(0, std::max(0, textureSize_.y - 1)),
              static_cast<int>(std::round(paramInjectionRadius_)),
              paramInjectionAmount_);
}

void ReactionDiffusionLayer::syncTexture() {
    if (!pixels_.isAllocated()) return;

    const ofFloatColor bg(ofClamp(paramBgR_, 0.0f, 1.0f),
                          ofClamp(paramBgG_, 0.0f, 1.0f),
                          ofClamp(paramBgB_, 0.0f, 1.0f),
                          ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f));
    const ofFloatColor fieldColor(ofClamp(paramFieldR_, 0.0f, 1.0f),
                                  ofClamp(paramFieldG_, 0.0f, 1.0f),
                                  ofClamp(paramFieldB_, 0.0f, 1.0f),
                                  ofClamp(paramFieldAlpha_, 0.0f, 1.0f));
    const ofFloatColor contourColor(ofClamp(paramContourR_, 0.0f, 1.0f),
                                    ofClamp(paramContourG_, 0.0f, 1.0f),
                                    ofClamp(paramContourB_, 0.0f, 1.0f),
                                    ofClamp(paramContourOpacity_, 0.0f, 1.0f));

    const float threshold = ofClamp(paramContourThreshold_, 0.0f, 1.0f);
    const float width = std::max(0.01f, paramContourWidth_);
    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const Cell& cell = field_[static_cast<std::size_t>(indexFor(x, y))];
            const float concentration = ofClamp(cell.b * paramFieldScale_, 0.0f, 1.0f);
            const float contour = ofClamp(1.0f - std::abs(cell.b - threshold) / width, 0.0f, 1.0f);
            ofFloatColor color(ofLerp(bg.r, fieldColor.r, concentration),
                               ofLerp(bg.g, fieldColor.g, concentration),
                               ofLerp(bg.b, fieldColor.b, concentration),
                               ofLerp(bg.a, fieldColor.a, concentration));
            color.r = ofLerp(color.r, contourColor.r, contour * contourColor.a);
            color.g = ofLerp(color.g, contourColor.g, contour * contourColor.a);
            color.b = ofLerp(color.b, contourColor.b, contour * contourColor.a);
            color.a = ofClamp(std::max(color.a, contour * contourColor.a), 0.0f, 1.0f);
            pixels_.setColor(x, y, color);
        }
    }

    texture_.loadData(pixels_);
}

void ReactionDiffusionLayer::clampParams() {
    paramSpeed_ = ofClamp(paramSpeed_, 0.0f, 80.0f);
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 16.0f);
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 128.0f));
    paramFeedRate_ = ofClamp(paramFeedRate_, 0.0f, 0.09f);
    paramKillRate_ = ofClamp(paramKillRate_, 0.0f, 0.09f);
    paramDiffusionA_ = ofClamp(paramDiffusionA_, 0.0f, 1.2f);
    paramDiffusionB_ = ofClamp(paramDiffusionB_, 0.0f, 0.8f);
    paramInjectionRate_ = ofClamp(paramInjectionRate_, 0.0f, 0.2f);
    paramInjectionAmount_ = ofClamp(paramInjectionAmount_, 0.0f, 1.0f);
    paramInjectionRadius_ = std::round(ofClamp(paramInjectionRadius_, 1.0f, 24.0f));
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 999999.0f));
    paramSeedDensity_ = ofClamp(paramSeedDensity_, 0.0f, 0.5f);
    paramContourThreshold_ = ofClamp(paramContourThreshold_, 0.0f, 1.0f);
    paramContourWidth_ = ofClamp(paramContourWidth_, 0.01f, 0.4f);
    paramFieldScale_ = ofClamp(paramFieldScale_, 0.1f, 4.0f);
    paramBackgroundAlpha_ = ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f);
    paramFieldAlpha_ = ofClamp(paramFieldAlpha_, 0.0f, 1.0f);
    paramContourOpacity_ = ofClamp(paramContourOpacity_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramFieldR_ = ofClamp(paramFieldR_, 0.0f, 1.0f);
    paramFieldG_ = ofClamp(paramFieldG_, 0.0f, 1.0f);
    paramFieldB_ = ofClamp(paramFieldB_, 0.0f, 1.0f);
    paramContourR_ = ofClamp(paramContourR_, 0.0f, 1.0f);
    paramContourG_ = ofClamp(paramContourG_, 0.0f, 1.0f);
    paramContourB_ = ofClamp(paramContourB_, 0.0f, 1.0f);
}

float ReactionDiffusionLayer::laplacianA(int x, int y) const {
    const auto sample = [&](int sx, int sy) {
        sx = (sx + textureSize_.x) % textureSize_.x;
        sy = (sy + textureSize_.y) % textureSize_.y;
        return field_[static_cast<std::size_t>(indexFor(sx, sy))].a;
    };
    return sample(x, y) * -1.0f +
           (sample(x - 1, y) + sample(x + 1, y) + sample(x, y - 1) + sample(x, y + 1)) * 0.20f +
           (sample(x - 1, y - 1) + sample(x + 1, y - 1) + sample(x - 1, y + 1) + sample(x + 1, y + 1)) * 0.05f;
}

float ReactionDiffusionLayer::laplacianB(int x, int y) const {
    const auto sample = [&](int sx, int sy) {
        sx = (sx + textureSize_.x) % textureSize_.x;
        sy = (sy + textureSize_.y) % textureSize_.y;
        return field_[static_cast<std::size_t>(indexFor(sx, sy))].b;
    };
    return sample(x, y) * -1.0f +
           (sample(x - 1, y) + sample(x + 1, y) + sample(x, y - 1) + sample(x, y + 1)) * 0.20f +
           (sample(x - 1, y - 1) + sample(x + 1, y - 1) + sample(x - 1, y + 1) + sample(x + 1, y + 1)) * 0.05f;
}

float ReactionDiffusionLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

float ReactionDiffusionLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) return 0.0f;
    return std::max(0.0f, timeSeconds) * (bpm / 60.0f);
}

std::uint32_t ReactionDiffusionLayer::activeSeed() const {
    return static_cast<std::uint32_t>(std::round(ofClamp(paramSeed_, 0.0f, 999999.0f)));
}

float ReactionDiffusionLayer::randomUnit() {
    return randomRange(0.0f, 1.0f);
}

float ReactionDiffusionLayer::randomRange(float minValue, float maxValue) {
    std::uniform_real_distribution<float> dist(minValue, std::max(minValue, maxValue));
    return dist(rng_);
}

int ReactionDiffusionLayer::randomInt(int minValue, int maxValue) {
    if (maxValue <= minValue) return minValue;
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng_);
}

int ReactionDiffusionLayer::indexFor(int x, int y) const {
    return y * textureSize_.x + x;
}

void ReactionDiffusionLayer::registerFloat(ParameterRegistry& registry,
                                           const std::string& id,
                                           float* target,
                                           float initial,
                                           const char* label,
                                           float minValue,
                                           float maxValue,
                                           float step,
                                           const char* description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = label;
    meta.range.min = minValue;
    meta.range.max = maxValue;
    meta.range.step = step;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

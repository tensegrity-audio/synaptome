#include "LeniaLayer.h"

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
}

void LeniaLayer::configure(const ofJson& config) {
    presentation_ =
        config.value("presentation", std::string()) == "circuit"
            ? Presentation::Circuit
            : Presentation::Organic;

    if (config.contains("defaults") && config["defaults"].is_object()) {
        const auto& def = config["defaults"];
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramPaused_ = def.value("paused", paramPaused_);
        paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
        paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
        paramSeed_ = def.value("seed", paramSeed_);
        paramSeedDensity_ = def.value("seedDensity", paramSeedDensity_);
        paramKernelRadius_ = def.value("kernelRadius", paramKernelRadius_);
        paramGrowthCenter_ = def.value("growthCenter", paramGrowthCenter_);
        paramGrowthWidth_ = def.value("growthWidth", paramGrowthWidth_);
        paramGrowthAmplitude_ = def.value("growthAmplitude", paramGrowthAmplitude_);
        paramMutationAmount_ = def.value("mutationAmount", paramMutationAmount_);
        paramInjectionRate_ = def.value("injectionRate", paramInjectionRate_);
        paramInjectionAmount_ = def.value("injectionAmount", paramInjectionAmount_);
        paramInjectionRadius_ = def.value("injectionRadius", paramInjectionRadius_);
        paramDecayRate_ = def.value("decayRate", paramDecayRate_);
        paramFieldScale_ = def.value("fieldScale", paramFieldScale_);
        paramEdgeGlow_ = def.value("edgeGlow", paramEdgeGlow_);
        paramBackgroundAlpha_ = def.value("backgroundAlpha", paramBackgroundAlpha_);
        paramFieldAlpha_ = def.value("fieldAlpha", paramFieldAlpha_);
        paramEdgeOpacity_ = def.value("edgeOpacity", paramEdgeOpacity_);
        paramBgR_ = def.value("bgR", paramBgR_);
        paramBgG_ = def.value("bgG", paramBgG_);
        paramBgB_ = def.value("bgB", paramBgB_);
        paramFieldR_ = def.value("fieldR", paramFieldR_);
        paramFieldG_ = def.value("fieldG", paramFieldG_);
        paramFieldB_ = def.value("fieldB", paramFieldB_);
        paramEdgeR_ = def.value("edgeR", paramEdgeR_);
        paramEdgeG_ = def.value("edgeG", paramEdgeG_);
        paramEdgeB_ = def.value("edgeB", paramEdgeB_);
        paramCircuitThreshold_ =
            def.value("circuitThreshold", paramCircuitThreshold_);
        paramCircuitLevels_ =
            def.value("circuitLevels", paramCircuitLevels_);
        paramCircuitTraceWidth_ =
            def.value("circuitTraceWidth", paramCircuitTraceWidth_);
        readColorArray(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
        readColorArray(def, "fieldColor", paramFieldR_, paramFieldG_, paramFieldB_);
        readColorArray(def, "edgeColor", paramEdgeR_, paramEdgeG_, paramEdgeB_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(32, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(32, config["textureSize"][1].get<int>());
    }
}

void LeniaLayer::bindParameters(
    synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", paramSpeed_);
    binder.bind("bpmMultiplier", paramBpmMultiplier_);
    binder.bind("alpha", paramAlpha_);
    binder.bind("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
    binder.bind("seed", paramSeed_);
    binder.bind("seedDensity", paramSeedDensity_);
    binder.bind("kernelRadius", paramKernelRadius_);
    binder.bind("growthCenter", paramGrowthCenter_);
    binder.bind("growthWidth", paramGrowthWidth_);
    binder.bind("growthAmplitude", paramGrowthAmplitude_);
    binder.bind("mutationAmount", paramMutationAmount_);
    binder.bind("injectionRate", paramInjectionRate_);
    binder.bind("injectionAmount", paramInjectionAmount_);
    binder.bind("injectionRadius", paramInjectionRadius_);
    binder.bind("decayRate", paramDecayRate_);
    binder.bind("fieldScale", paramFieldScale_);
    binder.bind("edgeGlow", paramEdgeGlow_);
    binder.bind("backgroundAlpha", paramBackgroundAlpha_);
    binder.bind("fieldAlpha", paramFieldAlpha_);
    binder.bind("edgeOpacity", paramEdgeOpacity_);
    binder.bind("bgR", paramBgR_);
    binder.bind("bgG", paramBgG_);
    binder.bind("bgB", paramBgB_);
    binder.bind("fieldR", paramFieldR_);
    binder.bind("fieldG", paramFieldG_);
    binder.bind("fieldB", paramFieldB_);
    binder.bind("edgeR", paramEdgeR_);
    binder.bind("edgeG", paramEdgeG_);
    binder.bind("edgeB", paramEdgeB_);
    binder.bind("circuitThreshold", paramCircuitThreshold_);
    binder.bind("circuitLevels", paramCircuitLevels_);
    binder.bind("circuitTraceWidth", paramCircuitTraceWidth_);
    binder.bind("visible", paramEnabled_);
    binder.bind("bpmSync", paramBpmSync_);
    binder.bind("paused", paramPaused_);
    binder.bind("reseed", paramReseedRequested_);
    binder.bind("autoReseed", paramAutoReseed_);
}

void LeniaLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    clampParams();

    allocateField();
    resetField();
    syncTexture();
}

void LeniaLayer::update(const LayerUpdateParams& params) {
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
    int iterations = std::min(18, static_cast<int>(std::floor(stepAccumulator_)));
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
            injectPatch();
        }
    }

    syncTexture();
    dirty_ = false;
}

void LeniaLayer::draw(const LayerDrawParams& params) {
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

void LeniaLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void LeniaLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void LeniaLayer::allocateField() {
    const std::size_t count = static_cast<std::size_t>(textureSize_.x * textureSize_.y);
    field_.assign(count, 0.0f);
    next_.assign(count, 0.0f);
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    dirty_ = true;
}

void LeniaLayer::resetField() {
    if (field_.empty()) {
        allocateField();
    }
    rng_.seed(activeSeed());
    std::fill(field_.begin(), field_.end(), 0.0f);

    seedPatch(textureSize_.x / 2,
              textureSize_.y / 2,
              std::max(3, static_cast<int>(std::round(paramInjectionRadius_ * 1.5f))),
              1.0f);

    const int patchCount = std::max(1, static_cast<int>(std::round(paramSeedDensity_ * 36.0f)));
    for (int i = 0; i < patchCount; ++i) {
        seedPatch(randomInt(0, std::max(0, textureSize_.x - 1)),
                  randomInt(0, std::max(0, textureSize_.y - 1)),
                  std::max(2, static_cast<int>(std::round(randomRange(2.0f, paramInjectionRadius_ + 3.0f)))),
                  randomRange(0.45f, 1.0f));
    }

    next_ = field_;
    stepAccumulator_ = 0.0f;
    dirty_ = true;
}

void LeniaLayer::seedPatch(int centerX, int centerY, int radius, float amount) {
    const int safeRadius = std::max(1, radius);
    const float radius2 = static_cast<float>(safeRadius * safeRadius);
    for (int dy = -safeRadius; dy <= safeRadius; ++dy) {
        for (int dx = -safeRadius; dx <= safeRadius; ++dx) {
            const float d2 = static_cast<float>(dx * dx + dy * dy);
            if (d2 > radius2) continue;
            const int x = (centerX + dx + textureSize_.x) % textureSize_.x;
            const int y = (centerY + dy + textureSize_.y) % textureSize_.y;
            const float falloff = 1.0f - std::sqrt(d2) / static_cast<float>(safeRadius);
            float& cell = field_[static_cast<std::size_t>(indexFor(x, y))];
            cell = ofClamp(cell + amount * (0.25f + falloff * 0.75f), 0.0f, 1.0f);
        }
    }
}

void LeniaLayer::stepSimulation() {
    const int radius = std::max(1, static_cast<int>(std::round(paramKernelRadius_)));
    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            float weightedSum = 0.0f;
            float weightSum = 0.0f;
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (distance > static_cast<float>(radius)) continue;
                    const float weight = kernelWeight(distance / static_cast<float>(radius));
                    weightedSum += sampleField(x + dx, y + dy) * weight;
                    weightSum += weight;
                }
            }

            const float potential = weightSum > 0.0f ? weightedSum / weightSum : 0.0f;
            const float width = std::max(0.005f, paramGrowthWidth_);
            const float centered = (potential - paramGrowthCenter_) / width;
            const float growth = 2.0f * std::exp(-0.5f * centered * centered) - 1.0f;
            const float mutation = (randomUnit() * 2.0f - 1.0f) * paramMutationAmount_;
            const float current = field_[static_cast<std::size_t>(indexFor(x, y))];
            const float nextValue = current + growth * paramGrowthAmplitude_ - current * paramDecayRate_ + mutation;
            next_[static_cast<std::size_t>(indexFor(x, y))] = ofClamp(nextValue, 0.0f, 1.0f);
        }
    }
    field_.swap(next_);
    dirty_ = true;
}

void LeniaLayer::injectPatch() {
    seedPatch(randomInt(0, std::max(0, textureSize_.x - 1)),
              randomInt(0, std::max(0, textureSize_.y - 1)),
              static_cast<int>(std::round(paramInjectionRadius_)),
              paramInjectionAmount_);
}

void LeniaLayer::syncTexture() {
    if (!pixels_.isAllocated()) return;

    const ofFloatColor bg(ofClamp(paramBgR_, 0.0f, 1.0f),
                          ofClamp(paramBgG_, 0.0f, 1.0f),
                          ofClamp(paramBgB_, 0.0f, 1.0f),
                          ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f));
    const ofFloatColor fieldColor(ofClamp(paramFieldR_, 0.0f, 1.0f),
                                  ofClamp(paramFieldG_, 0.0f, 1.0f),
                                  ofClamp(paramFieldB_, 0.0f, 1.0f),
                                  ofClamp(paramFieldAlpha_, 0.0f, 1.0f));
    const ofFloatColor edgeColor(ofClamp(paramEdgeR_, 0.0f, 1.0f),
                                 ofClamp(paramEdgeG_, 0.0f, 1.0f),
                                 ofClamp(paramEdgeB_, 0.0f, 1.0f),
                                 ofClamp(paramEdgeOpacity_, 0.0f, 1.0f));

    if (presentation_ == Presentation::Circuit) {
        const int levels = static_cast<int>(std::round(paramCircuitLevels_));
        const int radius = std::max(
            0, static_cast<int>(std::round(paramCircuitTraceWidth_)) - 1);
        for (int y = 0; y < textureSize_.y; ++y) {
            for (int x = 0; x < textureSize_.x; ++x) {
                const int band = circuitBandAt(x, y);
                bool contour = false;
                for (int oy = -radius; oy <= radius && !contour; ++oy) {
                    for (int ox = -radius; ox <= radius; ++ox) {
                        if (ox * ox + oy * oy > radius * radius) {
                            continue;
                        }
                        if (circuitContourAt(x + ox, y + oy)) {
                            contour = true;
                            break;
                        }
                    }
                }

                const float platedFill = band > 0
                    ? (0.08f + 0.16f *
                        static_cast<float>(band) /
                        static_cast<float>(std::max(1, levels))) *
                        fieldColor.a
                    : 0.0f;
                const float trace =
                    contour ? ofClamp(edgeColor.a, 0.0f, 1.0f) : 0.0f;
                ofFloatColor color(
                    ofLerp(bg.r, fieldColor.r, platedFill),
                    ofLerp(bg.g, fieldColor.g, platedFill),
                    ofLerp(bg.b, fieldColor.b, platedFill),
                    ofLerp(bg.a, fieldColor.a, platedFill));
                color.r = ofLerp(color.r, edgeColor.r, trace);
                color.g = ofLerp(color.g, edgeColor.g, trace);
                color.b = ofLerp(color.b, edgeColor.b, trace);
                color.a = ofClamp(std::max(color.a, trace), 0.0f, 1.0f);
                pixels_.setColor(x, y, color);
            }
        }
        texture_.loadData(pixels_);
        return;
    }

    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const float value = ofClamp(sampleField(x, y) * paramFieldScale_, 0.0f, 1.0f);
            const float dx = std::abs(sampleField(x + 1, y) - sampleField(x - 1, y));
            const float dy = std::abs(sampleField(x, y + 1) - sampleField(x, y - 1));
            const float edge = ofClamp((dx + dy) * paramEdgeGlow_, 0.0f, 1.0f);
            ofFloatColor color(ofLerp(bg.r, fieldColor.r, value),
                               ofLerp(bg.g, fieldColor.g, value),
                               ofLerp(bg.b, fieldColor.b, value),
                               ofLerp(bg.a, fieldColor.a, value));
            color.r = ofLerp(color.r, edgeColor.r, edge * edgeColor.a);
            color.g = ofLerp(color.g, edgeColor.g, edge * edgeColor.a);
            color.b = ofLerp(color.b, edgeColor.b, edge * edgeColor.a);
            color.a = ofClamp(std::max(color.a, edge * edgeColor.a), 0.0f, 1.0f);
            pixels_.setColor(x, y, color);
        }
    }

    texture_.loadData(pixels_);
}

int LeniaLayer::circuitBandAt(int x, int y) const {
    const float threshold =
        ofClamp(paramCircuitThreshold_, 0.0f, 0.95f);
    const int levels = std::max(
        2, static_cast<int>(std::round(paramCircuitLevels_)));
    const float value =
        ofClamp(sampleField(x, y) * paramFieldScale_, 0.0f, 1.0f);
    if (value <= threshold) {
        return 0;
    }
    const float normalized =
        (value - threshold) / std::max(0.001f, 1.0f - threshold);
    return ofClamp(
        1 + static_cast<int>(std::floor(normalized * levels)),
        1, levels);
}

bool LeniaLayer::circuitContourAt(int x, int y) const {
    const int center = circuitBandAt(x, y);
    if (center == 0) {
        return false;
    }
    return circuitBandAt(x - 1, y) != center ||
        circuitBandAt(x + 1, y) != center ||
        circuitBandAt(x, y - 1) != center ||
        circuitBandAt(x, y + 1) != center;
}

void LeniaLayer::clampParams() {
    paramSpeed_ = ofClamp(paramSpeed_, 0.0f, 80.0f);
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 16.0f);
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 128.0f));
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 999999.0f));
    paramSeedDensity_ = ofClamp(paramSeedDensity_, 0.0f, 0.5f);
    paramKernelRadius_ = std::round(ofClamp(paramKernelRadius_, 2.0f, 18.0f));
    paramGrowthCenter_ = ofClamp(paramGrowthCenter_, 0.0f, 1.0f);
    paramGrowthWidth_ = ofClamp(paramGrowthWidth_, 0.005f, 0.25f);
    paramGrowthAmplitude_ = ofClamp(paramGrowthAmplitude_, 0.0f, 0.35f);
    paramMutationAmount_ = ofClamp(paramMutationAmount_, 0.0f, 0.05f);
    paramInjectionRate_ = ofClamp(paramInjectionRate_, 0.0f, 0.2f);
    paramInjectionAmount_ = ofClamp(paramInjectionAmount_, 0.0f, 1.0f);
    paramInjectionRadius_ = std::round(ofClamp(paramInjectionRadius_, 1.0f, 28.0f));
    paramDecayRate_ = ofClamp(paramDecayRate_, 0.0f, 0.05f);
    paramFieldScale_ = ofClamp(paramFieldScale_, 0.1f, 4.0f);
    paramEdgeGlow_ = ofClamp(paramEdgeGlow_, 0.0f, 5.0f);
    paramBackgroundAlpha_ = ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f);
    paramFieldAlpha_ = ofClamp(paramFieldAlpha_, 0.0f, 1.0f);
    paramEdgeOpacity_ = ofClamp(paramEdgeOpacity_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramFieldR_ = ofClamp(paramFieldR_, 0.0f, 1.0f);
    paramFieldG_ = ofClamp(paramFieldG_, 0.0f, 1.0f);
    paramFieldB_ = ofClamp(paramFieldB_, 0.0f, 1.0f);
    paramEdgeR_ = ofClamp(paramEdgeR_, 0.0f, 1.0f);
    paramEdgeG_ = ofClamp(paramEdgeG_, 0.0f, 1.0f);
    paramEdgeB_ = ofClamp(paramEdgeB_, 0.0f, 1.0f);
    paramCircuitThreshold_ =
        ofClamp(paramCircuitThreshold_, 0.0f, 0.95f);
    paramCircuitLevels_ =
        std::round(ofClamp(paramCircuitLevels_, 2.0f, 8.0f));
    paramCircuitTraceWidth_ =
        std::round(ofClamp(paramCircuitTraceWidth_, 1.0f, 4.0f));
}

float LeniaLayer::sampleField(int x, int y) const {
    const int sx = (x + textureSize_.x) % textureSize_.x;
    const int sy = (y + textureSize_.y) % textureSize_.y;
    return field_[static_cast<std::size_t>(indexFor(sx, sy))];
}

float LeniaLayer::kernelWeight(float normalizedDistance) const {
    const float centered = (normalizedDistance - 0.48f) / 0.18f;
    return std::exp(-0.5f * centered * centered);
}

float LeniaLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

float LeniaLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) return 0.0f;
    return std::max(0.0f, timeSeconds) * (bpm / 60.0f);
}

std::uint32_t LeniaLayer::activeSeed() const {
    return static_cast<std::uint32_t>(std::round(ofClamp(paramSeed_, 0.0f, 999999.0f)));
}

float LeniaLayer::randomUnit() {
    return randomRange(0.0f, 1.0f);
}

float LeniaLayer::randomRange(float minValue, float maxValue) {
    std::uniform_real_distribution<float> dist(minValue, std::max(minValue, maxValue));
    return dist(rng_);
}

int LeniaLayer::randomInt(int minValue, int maxValue) {
    if (maxValue <= minValue) return minValue;
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng_);
}

int LeniaLayer::indexFor(int x, int y) const {
    return y * textureSize_.x + x;
}

std::uint64_t LeniaLayer::debugStateSignature() const {
    std::uint64_t hash = 1469598103934665603ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    auto mix = [&](std::uint64_t value) {
        hash ^= value;
        hash *= prime;
    };
    mix(static_cast<std::uint64_t>(presentation_));
    mix(activeSeed());
    for (std::size_t i = 0; i < field_.size(); i += 7) {
        mix(static_cast<std::uint64_t>(
            std::round(ofClamp(field_[i], 0.0f, 1.0f) * 65535.0f)));
    }
    return hash;
}

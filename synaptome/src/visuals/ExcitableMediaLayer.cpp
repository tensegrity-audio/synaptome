#include "ExcitableMediaLayer.h"

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

void ExcitableMediaLayer::configure(const ofJson& config) {
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
        paramPropagationRate_ = def.value("propagationRate", paramPropagationRate_);
        paramExcitationThreshold_ = def.value("excitationThreshold", paramExcitationThreshold_);
        paramRefractoryTime_ = def.value("refractoryTime", paramRefractoryTime_);
        paramSeedRate_ = def.value("seedRate", paramSeedRate_);
        paramWavefrontWidth_ = def.value("wavefrontWidth", paramWavefrontWidth_);
        paramSparkleAmount_ = def.value("sparkleAmount", paramSparkleAmount_);
        paramFieldDiffusion_ = def.value("fieldDiffusion", paramFieldDiffusion_);
        paramDecayRate_ = def.value("decayRate", paramDecayRate_);
        paramInjectionRadius_ = def.value("injectionRadius", paramInjectionRadius_);
        paramFieldScale_ = def.value("fieldScale", paramFieldScale_);
        paramBackgroundAlpha_ = def.value("backgroundAlpha", paramBackgroundAlpha_);
        paramExcitationAlpha_ = def.value("excitationAlpha", paramExcitationAlpha_);
        paramRefractoryAlpha_ = def.value("refractoryAlpha", paramRefractoryAlpha_);
        paramWavefrontOpacity_ = def.value("wavefrontOpacity", paramWavefrontOpacity_);
        paramBgR_ = def.value("bgR", paramBgR_);
        paramBgG_ = def.value("bgG", paramBgG_);
        paramBgB_ = def.value("bgB", paramBgB_);
        paramExciteR_ = def.value("exciteR", paramExciteR_);
        paramExciteG_ = def.value("exciteG", paramExciteG_);
        paramExciteB_ = def.value("exciteB", paramExciteB_);
        paramRefractoryR_ = def.value("refractoryR", paramRefractoryR_);
        paramRefractoryG_ = def.value("refractoryG", paramRefractoryG_);
        paramRefractoryB_ = def.value("refractoryB", paramRefractoryB_);
        paramWaveR_ = def.value("waveR", paramWaveR_);
        paramWaveG_ = def.value("waveG", paramWaveG_);
        paramWaveB_ = def.value("waveB", paramWaveB_);
        readColorArray(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
        readColorArray(def, "excitationColor", paramExciteR_, paramExciteG_, paramExciteB_);
        readColorArray(def, "refractoryColor", paramRefractoryR_, paramRefractoryG_, paramRefractoryB_);
        readColorArray(def, "wavefrontColor", paramWaveR_, paramWaveG_, paramWaveB_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(32, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(32, config["textureSize"][1].get<int>());
    }
}

void ExcitableMediaLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.excitableMedia" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".speed", &paramSpeed_, paramSpeed_, "Time: Simulation Speed", 0.0f, 96.0f, 0.1f);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: BPM Sync";
    meta.description = "Use transport BPM instead of free-running excitation updates.";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);

    registerFloat(registry, prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, "Time: BPM Mult", 0.25f, 24.0f, 0.25f);
    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Visibility: Layer Opacity", 0.0f, 1.0f, 0.01f);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: Paused";
    registry.addBool(prefix + ".paused", &paramPaused_, paramPaused_, meta);

    meta.label = "Action: Reseed";
    meta.description = "Reset the resting, excited, and refractory fields.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    meta.label = "Action: Auto Reseed";
    meta.description = "Reset on a transport-quantized cadence.";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, meta);

    registerFloat(registry, prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, "Time: Auto Reseed Beats", 1.0f, 128.0f, 1.0f);
    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: Pattern Seed", 0.0f, 999999.0f, 1.0f);
    registerFloat(registry, prefix + ".seedDensity", &paramSeedDensity_, paramSeedDensity_, "Seed: Spark Density", 0.0f, 0.5f, 0.01f);
    registerFloat(registry, prefix + ".propagationRate", &paramPropagationRate_, paramPropagationRate_, "Force: Propagation Rate", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".excitationThreshold", &paramExcitationThreshold_, paramExcitationThreshold_, "Growth: Excitation Threshold", 0.01f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".refractoryTime", &paramRefractoryTime_, paramRefractoryTime_, "Time: Refractory Time", 1.0f, 64.0f, 1.0f);
    registerFloat(registry, prefix + ".seedRate", &paramSeedRate_, paramSeedRate_, "Growth: Spark Rate", 0.0f, 0.2f, 0.001f);
    registerFloat(registry, prefix + ".wavefrontWidth", &paramWavefrontWidth_, paramWavefrontWidth_, "Scale: Wavefront Width", 0.01f, 0.5f, 0.01f);
    registerFloat(registry, prefix + ".sparkleAmount", &paramSparkleAmount_, paramSparkleAmount_, "Glow: Sparkle Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldDiffusion", &paramFieldDiffusion_, paramFieldDiffusion_, "Force: Field Diffusion", 0.0f, 0.5f, 0.01f);
    registerFloat(registry, prefix + ".decayRate", &paramDecayRate_, paramDecayRate_, "Time: Excitation Decay", 0.0f, 0.5f, 0.01f);
    registerFloat(registry, prefix + ".injectionRadius", &paramInjectionRadius_, paramInjectionRadius_, "Scale: Spark Radius", 1.0f, 16.0f, 1.0f);
    registerFloat(registry, prefix + ".fieldScale", &paramFieldScale_, paramFieldScale_, "Scale: Field Gain", 0.1f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".backgroundAlpha", &paramBackgroundAlpha_, paramBackgroundAlpha_, "Visibility: Background Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".excitationAlpha", &paramExcitationAlpha_, paramExcitationAlpha_, "Visibility: Excitation Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".refractoryAlpha", &paramRefractoryAlpha_, paramRefractoryAlpha_, "Visibility: Refractory Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".wavefrontOpacity", &paramWavefrontOpacity_, paramWavefrontOpacity_, "Visibility: Wavefront Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Color: Background R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Color: Background G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Color: Background B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".exciteR", &paramExciteR_, paramExciteR_, "Color: Excitation R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".exciteG", &paramExciteG_, paramExciteG_, "Color: Excitation G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".exciteB", &paramExciteB_, paramExciteB_, "Color: Excitation B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".refractoryR", &paramRefractoryR_, paramRefractoryR_, "Color: Refractory R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".refractoryG", &paramRefractoryG_, paramRefractoryG_, "Color: Refractory G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".refractoryB", &paramRefractoryB_, paramRefractoryB_, "Color: Refractory B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waveR", &paramWaveR_, paramWaveR_, "Color: Wavefront R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waveG", &paramWaveG_, paramWaveG_, "Color: Wavefront G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waveB", &paramWaveB_, paramWaveB_, "Color: Wavefront B", 0.0f, 1.0f, 0.01f);

    allocateField();
    resetField();
    syncTexture();
}

void ExcitableMediaLayer::update(const LayerUpdateParams& params) {
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
        if (paramSeedRate_ > 0.0f && randomUnit() < paramSeedRate_) {
            injectSpark();
        }
    }

    syncTexture();
    dirty_ = false;
}

void ExcitableMediaLayer::draw(const LayerDrawParams& params) {
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

void ExcitableMediaLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void ExcitableMediaLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void ExcitableMediaLayer::allocateField() {
    const std::size_t count = static_cast<std::size_t>(textureSize_.x * textureSize_.y);
    field_.assign(count, {});
    next_.assign(count, {});
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    dirty_ = true;
}

void ExcitableMediaLayer::resetField() {
    if (field_.empty()) {
        allocateField();
    }
    rng_.seed(activeSeed());
    for (auto& cell : field_) {
        cell.excitation = 0.0f;
        cell.refractory = 0.0f;
    }

    seedPatch(textureSize_.x / 2, textureSize_.y / 2, static_cast<int>(std::round(paramInjectionRadius_)), 1.0f);
    const int patchCount = std::max(1, static_cast<int>(std::round(paramSeedDensity_ * 48.0f)));
    for (int i = 0; i < patchCount; ++i) {
        seedPatch(randomInt(0, std::max(0, textureSize_.x - 1)),
                  randomInt(0, std::max(0, textureSize_.y - 1)),
                  std::max(1, static_cast<int>(std::round(randomRange(1.0f, paramInjectionRadius_ + 2.0f)))),
                  randomRange(0.65f, 1.0f));
    }

    next_ = field_;
    stepAccumulator_ = 0.0f;
    dirty_ = true;
}

void ExcitableMediaLayer::seedPatch(int centerX, int centerY, int radius, float amount) {
    const int safeRadius = std::max(1, radius);
    const float radius2 = static_cast<float>(safeRadius * safeRadius);
    for (int dy = -safeRadius; dy <= safeRadius; ++dy) {
        for (int dx = -safeRadius; dx <= safeRadius; ++dx) {
            const float d2 = static_cast<float>(dx * dx + dy * dy);
            if (d2 > radius2) continue;
            const int x = (centerX + dx + textureSize_.x) % textureSize_.x;
            const int y = (centerY + dy + textureSize_.y) % textureSize_.y;
            const float falloff = 1.0f - std::sqrt(d2) / static_cast<float>(safeRadius);
            Cell& cell = field_[static_cast<std::size_t>(indexFor(x, y))];
            cell.excitation = ofClamp(std::max(cell.excitation, amount * (0.35f + falloff * 0.65f)), 0.0f, 1.0f);
            cell.refractory = 0.0f;
        }
    }
}

void ExcitableMediaLayer::stepSimulation() {
    const float refractoryRelease = 1.0f / std::max(1.0f, paramRefractoryTime_);
    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const Cell& cell = field_[static_cast<std::size_t>(indexFor(x, y))];
            const float localDrive = neighborExcitation(x, y) * paramPropagationRate_;
            Cell nextCell;

            if (cell.refractory > 0.0f) {
                nextCell.excitation = ofClamp(cell.excitation * (1.0f - paramDecayRate_), 0.0f, 1.0f);
                nextCell.refractory = ofClamp(cell.refractory - refractoryRelease, 0.0f, 1.0f);
            } else if (cell.excitation + localDrive >= paramExcitationThreshold_) {
                nextCell.excitation = 1.0f;
                nextCell.refractory = 1.0f;
            } else {
                const float carried = cell.excitation * (1.0f - paramDecayRate_) + localDrive * paramFieldDiffusion_;
                nextCell.excitation = ofClamp(carried, 0.0f, paramExcitationThreshold_);
                nextCell.refractory = 0.0f;
            }

            next_[static_cast<std::size_t>(indexFor(x, y))] = nextCell;
        }
    }
    field_.swap(next_);
    dirty_ = true;
}

void ExcitableMediaLayer::injectSpark() {
    seedPatch(randomInt(0, std::max(0, textureSize_.x - 1)),
              randomInt(0, std::max(0, textureSize_.y - 1)),
              static_cast<int>(std::round(paramInjectionRadius_)),
              1.0f);
}

void ExcitableMediaLayer::syncTexture() {
    if (!pixels_.isAllocated()) return;

    const ofFloatColor bg(ofClamp(paramBgR_, 0.0f, 1.0f),
                          ofClamp(paramBgG_, 0.0f, 1.0f),
                          ofClamp(paramBgB_, 0.0f, 1.0f),
                          ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f));
    const ofFloatColor excitationColor(ofClamp(paramExciteR_, 0.0f, 1.0f),
                                       ofClamp(paramExciteG_, 0.0f, 1.0f),
                                       ofClamp(paramExciteB_, 0.0f, 1.0f),
                                       ofClamp(paramExcitationAlpha_, 0.0f, 1.0f));
    const ofFloatColor refractoryColor(ofClamp(paramRefractoryR_, 0.0f, 1.0f),
                                       ofClamp(paramRefractoryG_, 0.0f, 1.0f),
                                       ofClamp(paramRefractoryB_, 0.0f, 1.0f),
                                       ofClamp(paramRefractoryAlpha_, 0.0f, 1.0f));
    const ofFloatColor waveColor(ofClamp(paramWaveR_, 0.0f, 1.0f),
                                 ofClamp(paramWaveG_, 0.0f, 1.0f),
                                 ofClamp(paramWaveB_, 0.0f, 1.0f),
                                 ofClamp(paramWavefrontOpacity_, 0.0f, 1.0f));

    const float width = std::max(0.01f, paramWavefrontWidth_);
    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const Cell cell = sampleCell(x, y);
            const float excitation = ofClamp(cell.excitation * paramFieldScale_, 0.0f, 1.0f);
            const float refractory = ofClamp(cell.refractory, 0.0f, 1.0f);
            const float wave = ofClamp(1.0f - std::abs(cell.excitation - paramExcitationThreshold_) / width, 0.0f, 1.0f);
            const float sparkle = ofClamp((neighborExcitation(x, y) + wave) * paramSparkleAmount_, 0.0f, 1.0f);

            ofFloatColor color(ofLerp(bg.r, refractoryColor.r, refractory * refractoryColor.a),
                               ofLerp(bg.g, refractoryColor.g, refractory * refractoryColor.a),
                               ofLerp(bg.b, refractoryColor.b, refractory * refractoryColor.a),
                               ofLerp(bg.a, refractoryColor.a, refractory * refractoryColor.a));
            color.r = ofLerp(color.r, excitationColor.r, excitation * excitationColor.a);
            color.g = ofLerp(color.g, excitationColor.g, excitation * excitationColor.a);
            color.b = ofLerp(color.b, excitationColor.b, excitation * excitationColor.a);
            color.a = ofClamp(std::max(color.a, excitation * excitationColor.a), 0.0f, 1.0f);
            color.r = ofLerp(color.r, waveColor.r, wave * waveColor.a);
            color.g = ofLerp(color.g, waveColor.g, wave * waveColor.a);
            color.b = ofLerp(color.b, waveColor.b, wave * waveColor.a);
            color.a = ofClamp(std::max(color.a, wave * waveColor.a), 0.0f, 1.0f);
            color.r = ofClamp(color.r + sparkle * 0.10f, 0.0f, 1.0f);
            color.g = ofClamp(color.g + sparkle * 0.10f, 0.0f, 1.0f);
            color.b = ofClamp(color.b + sparkle * 0.08f, 0.0f, 1.0f);
            pixels_.setColor(x, y, color);
        }
    }

    texture_.loadData(pixels_);
}

void ExcitableMediaLayer::clampParams() {
    paramSpeed_ = ofClamp(paramSpeed_, 0.0f, 96.0f);
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 24.0f);
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 128.0f));
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 999999.0f));
    paramSeedDensity_ = ofClamp(paramSeedDensity_, 0.0f, 0.5f);
    paramPropagationRate_ = ofClamp(paramPropagationRate_, 0.0f, 3.0f);
    paramExcitationThreshold_ = ofClamp(paramExcitationThreshold_, 0.01f, 1.0f);
    paramRefractoryTime_ = std::round(ofClamp(paramRefractoryTime_, 1.0f, 64.0f));
    paramSeedRate_ = ofClamp(paramSeedRate_, 0.0f, 0.2f);
    paramWavefrontWidth_ = ofClamp(paramWavefrontWidth_, 0.01f, 0.5f);
    paramSparkleAmount_ = ofClamp(paramSparkleAmount_, 0.0f, 2.0f);
    paramFieldDiffusion_ = ofClamp(paramFieldDiffusion_, 0.0f, 0.5f);
    paramDecayRate_ = ofClamp(paramDecayRate_, 0.0f, 0.5f);
    paramInjectionRadius_ = std::round(ofClamp(paramInjectionRadius_, 1.0f, 16.0f));
    paramFieldScale_ = ofClamp(paramFieldScale_, 0.1f, 4.0f);
    paramBackgroundAlpha_ = ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f);
    paramExcitationAlpha_ = ofClamp(paramExcitationAlpha_, 0.0f, 1.0f);
    paramRefractoryAlpha_ = ofClamp(paramRefractoryAlpha_, 0.0f, 1.0f);
    paramWavefrontOpacity_ = ofClamp(paramWavefrontOpacity_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramExciteR_ = ofClamp(paramExciteR_, 0.0f, 1.0f);
    paramExciteG_ = ofClamp(paramExciteG_, 0.0f, 1.0f);
    paramExciteB_ = ofClamp(paramExciteB_, 0.0f, 1.0f);
    paramRefractoryR_ = ofClamp(paramRefractoryR_, 0.0f, 1.0f);
    paramRefractoryG_ = ofClamp(paramRefractoryG_, 0.0f, 1.0f);
    paramRefractoryB_ = ofClamp(paramRefractoryB_, 0.0f, 1.0f);
    paramWaveR_ = ofClamp(paramWaveR_, 0.0f, 1.0f);
    paramWaveG_ = ofClamp(paramWaveG_, 0.0f, 1.0f);
    paramWaveB_ = ofClamp(paramWaveB_, 0.0f, 1.0f);
}

ExcitableMediaLayer::Cell ExcitableMediaLayer::sampleCell(int x, int y) const {
    const int sx = (x + textureSize_.x) % textureSize_.x;
    const int sy = (y + textureSize_.y) % textureSize_.y;
    return field_[static_cast<std::size_t>(indexFor(sx, sy))];
}

float ExcitableMediaLayer::neighborExcitation(int x, int y) const {
    float sum = 0.0f;
    sum += sampleCell(x - 1, y).excitation;
    sum += sampleCell(x + 1, y).excitation;
    sum += sampleCell(x, y - 1).excitation;
    sum += sampleCell(x, y + 1).excitation;
    sum += sampleCell(x - 1, y - 1).excitation * 0.5f;
    sum += sampleCell(x + 1, y - 1).excitation * 0.5f;
    sum += sampleCell(x - 1, y + 1).excitation * 0.5f;
    sum += sampleCell(x + 1, y + 1).excitation * 0.5f;
    return sum / 6.0f;
}

float ExcitableMediaLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

float ExcitableMediaLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) return 0.0f;
    return std::max(0.0f, timeSeconds) * (bpm / 60.0f);
}

std::uint32_t ExcitableMediaLayer::activeSeed() const {
    return static_cast<std::uint32_t>(std::round(ofClamp(paramSeed_, 0.0f, 999999.0f)));
}

float ExcitableMediaLayer::randomUnit() {
    return randomRange(0.0f, 1.0f);
}

float ExcitableMediaLayer::randomRange(float minValue, float maxValue) {
    std::uniform_real_distribution<float> dist(minValue, std::max(minValue, maxValue));
    return dist(rng_);
}

int ExcitableMediaLayer::randomInt(int minValue, int maxValue) {
    if (maxValue <= minValue) return minValue;
    std::uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng_);
}

int ExcitableMediaLayer::indexFor(int x, int y) const {
    return y * textureSize_.x + x;
}

void ExcitableMediaLayer::registerFloat(ParameterRegistry& registry,
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

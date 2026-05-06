#include "FlowFieldLayer.h"

#include "ofGraphics.h"
#include "ofMath.h"
#include "ofUtils.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float kNoiseEpsilon = 0.015f;

    float wrapCoord(float value, float limit) {
        while (value < 0.0f) value += limit;
        while (value >= limit) value -= limit;
        return value;
    }

    glm::vec2 safeNormalize(const glm::vec2& v, const glm::vec2& fallback = glm::vec2(1.0f, 0.0f)) {
        const float len = glm::length(v);
        if (len <= 0.0001f) {
            return fallback;
        }
        return v / len;
    }

    void readColorArray(const ofJson& def,
                        const char* key,
                        float& r,
                        float& g,
                        float& b) {
        if (def.contains(key) && def[key].is_array() && def[key].size() >= 3) {
            r = def[key][0].get<float>();
            g = def[key][1].get<float>();
            b = def[key][2].get<float>();
        }
    }

    void registerFloat(ParameterRegistry& registry,
                       const std::string& id,
                       float* target,
                       float initial,
                       const char* label,
                       float minValue,
                       float maxValue,
                       float step,
                       const char* description = "") {
        ParameterRegistry::Descriptor meta;
        meta.group = "Generative";
        meta.label = label;
        meta.range.min = minValue;
        meta.range.max = maxValue;
        meta.range.step = step;
        meta.description = description;
        registry.addFloat(id, target, initial, meta);
    }
}

void FlowFieldLayer::configure(const ofJson& config) {
    if (config.contains("defaults") && config["defaults"].is_object()) {
        const auto& def = config["defaults"];
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
        paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
        paramParticleCount_ = def.value("particleCount", paramParticleCount_);
        paramParticleLife_ = def.value("particleLife", paramParticleLife_);
        paramRespawnRate_ = def.value("respawnRate", paramRespawnRate_);
        paramSpawnRadius_ = def.value("spawnRadius", paramSpawnRadius_);
        paramEdgeWrap_ = def.value("edgeWrap", paramEdgeWrap_);
        paramMirrorX_ = def.value("mirrorX", paramMirrorX_);
        paramMirrorY_ = def.value("mirrorY", paramMirrorY_);
        paramFieldScale_ = def.value("fieldScale", paramFieldScale_);
        paramFieldStrength_ = def.value("fieldStrength", paramFieldStrength_);
        paramFlowSpeed_ = def.value("flowSpeed", paramFlowSpeed_);
        paramCurlAmount_ = def.value("curlAmount", paramCurlAmount_);
        paramTurbulence_ = def.value("turbulence", paramTurbulence_);
        paramStepSize_ = def.value("stepSize", paramStepSize_);
        paramInertia_ = def.value("inertia", paramInertia_);
        paramCenterPull_ = def.value("centerPull", paramCenterPull_);
        paramDriftX_ = def.value("driftX", paramDriftX_);
        paramDriftY_ = def.value("driftY", paramDriftY_);
        paramTrailFade_ = def.value("trailFade", paramTrailFade_);
        paramTrailDeposit_ = def.value("trailDeposit", paramTrailDeposit_);
        paramTrailBoost_ = def.value("trailBoost", paramTrailBoost_);
        paramBackgroundAlpha_ = def.value("backgroundAlpha", paramBackgroundAlpha_);
        paramTrailAlpha_ = def.value("trailAlpha", paramTrailAlpha_);
        paramPointSize_ = def.value("pointSize", paramPointSize_);
        paramVectorOverlay_ = def.value("vectorOverlay", paramVectorOverlay_);
        paramVectorSpacing_ = def.value("vectorSpacing", paramVectorSpacing_);
        paramVectorScale_ = def.value("vectorScale", paramVectorScale_);
        paramVectorAlpha_ = def.value("vectorAlpha", paramVectorAlpha_);
        paramColorBias_ = def.value("colorBias", paramColorBias_);
        paramPaletteRate_ = def.value("paletteRate", paramPaletteRate_);
        paramBgR_ = def.value("bgR", paramBgR_);
        paramBgG_ = def.value("bgG", paramBgG_);
        paramBgB_ = def.value("bgB", paramBgB_);
        paramColorAR_ = def.value("colorAR", paramColorAR_);
        paramColorAG_ = def.value("colorAG", paramColorAG_);
        paramColorAB_ = def.value("colorAB", paramColorAB_);
        paramColorBR_ = def.value("colorBR", paramColorBR_);
        paramColorBG_ = def.value("colorBG", paramColorBG_);
        paramColorBB_ = def.value("colorBB", paramColorBB_);
        readColorArray(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
        readColorArray(def, "colorA", paramColorAR_, paramColorAG_, paramColorAB_);
        readColorArray(def, "colorB", paramColorBR_, paramColorBG_, paramColorBB_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(32, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(32, config["textureSize"][1].get<int>());
    }
}

void FlowFieldLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.flowField" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = "Flow Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".speed", &paramSpeed_, paramSpeed_, "Flow Speed", 0.0f, 60.0f, 0.1f,
                  "Free-running simulation steps per second.");

    meta = {};
    meta.group = "Generative";
    meta.label = "Flow BPM Sync";
    meta.description = "Use transport BPM instead of free-running speed.";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);

    registerFloat(registry, prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, "Flow BPM Mult", 0.25f, 16.0f, 0.25f,
                  "Simulation step multiplier when BPM sync is enabled.");
    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Flow Alpha", 0.0f, 1.0f, 0.01f);

    meta = {};
    meta.group = "Generative";
    meta.label = "Flow Reseed";
    meta.description = "Respawn every particle and clear trails.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    meta.label = "Flow Auto Reseed";
    meta.description = "Respawn on a transport-quantized cadence.";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, meta);

    registerFloat(registry, prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, "Flow Auto Reseed Beats", 1.0f, 128.0f, 1.0f);
    registerFloat(registry, prefix + ".particleCount", &paramParticleCount_, paramParticleCount_, "Flow Particles", 16.0f, 4096.0f, 1.0f);
    registerFloat(registry, prefix + ".particleLife", &paramParticleLife_, paramParticleLife_, "Flow Particle Life", 16.0f, 2048.0f, 1.0f);
    registerFloat(registry, prefix + ".respawnRate", &paramRespawnRate_, paramRespawnRate_, "Flow Respawn Rate", 0.0f, 0.2f, 0.001f);
    registerFloat(registry, prefix + ".spawnRadius", &paramSpawnRadius_, paramSpawnRadius_, "Flow Spawn Radius", 0.0f, 1.0f, 0.01f,
                  "0=center, 1=full field.");

    meta = {};
    meta.group = "Generative";
    meta.label = "Flow Edge Wrap";
    registry.addBool(prefix + ".edgeWrap", &paramEdgeWrap_, paramEdgeWrap_, meta);
    meta.label = "Flow Mirror X";
    registry.addBool(prefix + ".mirrorX", &paramMirrorX_, paramMirrorX_, meta);
    meta.label = "Flow Mirror Y";
    registry.addBool(prefix + ".mirrorY", &paramMirrorY_, paramMirrorY_, meta);

    registerFloat(registry, prefix + ".fieldScale", &paramFieldScale_, paramFieldScale_, "Flow Field Scale", 0.2f, 16.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldStrength", &paramFieldStrength_, paramFieldStrength_, "Flow Field Strength", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".flowSpeed", &paramFlowSpeed_, paramFlowSpeed_, "Flow Field Speed", -2.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".curlAmount", &paramCurlAmount_, paramCurlAmount_, "Flow Curl", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".turbulence", &paramTurbulence_, paramTurbulence_, "Flow Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".stepSize", &paramStepSize_, paramStepSize_, "Flow Step Size", 0.1f, 6.0f, 0.01f);
    registerFloat(registry, prefix + ".inertia", &paramInertia_, paramInertia_, "Flow Inertia", 0.0f, 0.98f, 0.01f);
    registerFloat(registry, prefix + ".centerPull", &paramCenterPull_, paramCenterPull_, "Flow Center Pull", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".driftX", &paramDriftX_, paramDriftX_, "Flow Drift X", -2.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".driftY", &paramDriftY_, paramDriftY_, "Flow Drift Y", -2.0f, 2.0f, 0.01f);

    registerFloat(registry, prefix + ".trailFade", &paramTrailFade_, paramTrailFade_, "Flow Trail Fade", 0.0f, 0.2f, 0.001f);
    registerFloat(registry, prefix + ".trailDeposit", &paramTrailDeposit_, paramTrailDeposit_, "Flow Trail Deposit", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailBoost", &paramTrailBoost_, paramTrailBoost_, "Flow Trail Boost", 0.1f, 6.0f, 0.01f);
    registerFloat(registry, prefix + ".backgroundAlpha", &paramBackgroundAlpha_, paramBackgroundAlpha_, "Flow Bg Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, "Flow Trail Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".pointSize", &paramPointSize_, paramPointSize_, "Flow Point Size", 0.0f, 4.0f, 0.25f);

    meta = {};
    meta.group = "Generative";
    meta.label = "Flow Vectors";
    meta.description = "Draw low-opacity vector arrows on top of the trail texture.";
    registry.addBool(prefix + ".vectorOverlay", &paramVectorOverlay_, paramVectorOverlay_, meta);

    registerFloat(registry, prefix + ".vectorSpacing", &paramVectorSpacing_, paramVectorSpacing_, "Flow Vector Spacing", 8.0f, 64.0f, 1.0f);
    registerFloat(registry, prefix + ".vectorScale", &paramVectorScale_, paramVectorScale_, "Flow Vector Scale", 0.05f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".vectorAlpha", &paramVectorAlpha_, paramVectorAlpha_, "Flow Vector Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".colorBias", &paramColorBias_, paramColorBias_, "Flow Color Bias", 0.0f, 1.0f, 0.01f,
                  "Blend bias between color A and color B.");
    registerFloat(registry, prefix + ".paletteRate", &paramPaletteRate_, paramPaletteRate_, "Flow Palette Rate", -2.0f, 2.0f, 0.01f);

    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Flow Bg R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Flow Bg G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Flow Bg B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".colorAR", &paramColorAR_, paramColorAR_, "Flow Color A R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorAG", &paramColorAG_, paramColorAG_, "Flow Color A G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorAB", &paramColorAB_, paramColorAB_, "Flow Color A B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorBR", &paramColorBR_, paramColorBR_, "Flow Color B R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorBG", &paramColorBG_, paramColorBG_, "Flow Color B G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorBB", &paramColorBB_, paramColorBB_, "Flow Color B B", 0.0f, 1.5f, 0.01f);

    allocateTrail();
    resetParticles();
    syncTexture();
}

void FlowFieldLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    clampParams();

    if (std::abs(paramPaletteRate_) > 0.0001f) {
        palettePhase_ = wrapCoord(palettePhase_ + params.dt * paramPaletteRate_, 1.0f);
        dirty_ = true;
    }

    const int desiredCount = static_cast<int>(std::round(paramParticleCount_));
    if (desiredCount != static_cast<int>(particles_.size()) || paramReseedRequested_) {
        triggerReset();
        paramReseedRequested_ = false;
    }

    const float beatPosition = currentBeatPosition(params.time, params.bpm);
    if (paramAutoReseed_ && params.bpm > 0.0f) {
        if (nextAutoReseedBeat_ < 0.0f) {
            const float interval = std::max(1.0f, paramAutoReseedEveryBeats_);
            nextAutoReseedBeat_ = std::floor(beatPosition / interval) * interval + interval;
        }
        while (beatPosition >= nextAutoReseedBeat_) {
            triggerReset();
            nextAutoReseedBeat_ += std::max(1.0f, paramAutoReseedEveryBeats_);
        }
    } else {
        nextAutoReseedBeat_ = -1.0f;
    }

    const float stepRate = stepRateFor(params);
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
        fadeTrail();
        const float localTime = params.time + static_cast<float>(i) / static_cast<float>(std::max(1, iterations));
        stepParticles(localTime);
    }

    syncTexture();
    dirty_ = false;
}

void FlowFieldLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || !texture_.isAllocated() || params.slotOpacity <= 0.0f) return;

    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    ofSetColor(255, 255, 255, static_cast<int>(alpha * 255.0f));
    texture_.draw(0, 0, params.viewport.x, params.viewport.y);
    drawVectorOverlay(params);
    ofPopView();
    ofPopStyle();
}

void FlowFieldLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void FlowFieldLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void FlowFieldLayer::allocateTrail() {
    const std::size_t count = static_cast<std::size_t>(textureSize_.x * textureSize_.y);
    trailA_.assign(count, 0.0f);
    trailB_.assign(count, 0.0f);
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    dirty_ = true;
}

void FlowFieldLayer::resetParticles() {
    if (trailA_.empty() || trailB_.empty()) {
        allocateTrail();
    }

    std::fill(trailA_.begin(), trailA_.end(), 0.0f);
    std::fill(trailB_.begin(), trailB_.end(), 0.0f);
    particles_.assign(static_cast<std::size_t>(std::round(paramParticleCount_)), {});
    for (auto& particle : particles_) {
        spawnParticle(particle);
        depositPoint(particle.pos, paramTrailDeposit_, paramColorBias_);
    }
    nextAutoReseedBeat_ = -1.0f;
    dirty_ = true;
}

void FlowFieldLayer::spawnParticle(Particle& particle) {
    const float radius = ofClamp(paramSpawnRadius_, 0.0f, 1.0f);
    if (radius >= 0.995f) {
        particle.pos = { ofRandom(static_cast<float>(textureSize_.x)), ofRandom(static_cast<float>(textureSize_.y)) };
    } else {
        const glm::vec2 center(static_cast<float>(textureSize_.x) * 0.5f, static_cast<float>(textureSize_.y) * 0.5f);
        const float maxRadius = std::max(1.0f, std::min(static_cast<float>(textureSize_.x), static_cast<float>(textureSize_.y)) * 0.5f * radius);
        const float angle = ofRandom(TWO_PI);
        const float dist = std::sqrt(ofRandomuf()) * maxRadius;
        particle.pos = center + glm::vec2(std::cos(angle), std::sin(angle)) * dist;
    }
    particle.pos.x = ofClamp(particle.pos.x, 0.0f, static_cast<float>(textureSize_.x - 1));
    particle.pos.y = ofClamp(particle.pos.y, 0.0f, static_cast<float>(textureSize_.y - 1));
    particle.prev = particle.pos;
    particle.vel = flowAt(particle.pos, ofGetElapsedTimef()) * paramFieldStrength_;
    particle.age = ofRandom(paramParticleLife_);
    particle.life = std::max(1.0f, paramParticleLife_ * ofRandom(0.6f, 1.4f));
}

void FlowFieldLayer::fadeTrail() {
    const float fade = ofClamp(paramTrailFade_, 0.0f, 0.2f);
    if (fade <= 0.0f) return;
    for (std::size_t i = 0; i < trailA_.size(); ++i) {
        trailA_[i] = ofClamp(trailA_[i] - fade, 0.0f, 1.0f);
        trailB_[i] = ofClamp(trailB_[i] - fade, 0.0f, 1.0f);
    }
}

void FlowFieldLayer::stepParticles(float timeSeconds) {
    for (auto& particle : particles_) {
        particle.prev = particle.pos;
        glm::vec2 flow = flowAt(particle.pos, timeSeconds) * paramFieldStrength_;
        particle.vel = particle.vel * paramInertia_ + flow * (1.0f - paramInertia_);
        particle.pos += particle.vel * paramStepSize_;
        particle.age += 1.0f;

        const bool crossedEdge = outOfBounds(particle.pos);
        if (paramEdgeWrap_ && crossedEdge) {
            particle.pos = wrapPosition(particle.pos);
            particle.prev = particle.pos;
        }

        const bool expired = particle.age >= particle.life;
        const bool shouldRespawn = expired || (!paramEdgeWrap_ && crossedEdge) || (paramRespawnRate_ > 0.0f && ofRandomuf() < paramRespawnRate_);
        if (shouldRespawn) {
            spawnParticle(particle);
            continue;
        }

        const glm::vec2 dir = safeNormalize(particle.vel);
        const float angleMix = std::atan2(dir.y, dir.x) / TWO_PI + 0.5f;
        const float colorMix = wrapCoord(ofLerp(angleMix, paramColorBias_, 0.45f) + palettePhase_, 1.0f);
        depositTrail(particle.prev, particle.pos, paramTrailDeposit_, colorMix);
    }
}

void FlowFieldLayer::depositTrail(const glm::vec2& from, const glm::vec2& to, float amount, float colorMix) {
    const glm::vec2 delta = to - from;
    const int samples = std::max(1, static_cast<int>(std::ceil(glm::length(delta))));
    for (int i = 0; i <= samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        depositPoint(from + delta * t, amount, colorMix);
    }
}

void FlowFieldLayer::depositPoint(const glm::vec2& pos, float amount, float colorMix) {
    const int radius = std::max(0, static_cast<int>(std::round(paramPointSize_ * 0.5f)));
    const float mix = ofClamp(colorMix, 0.0f, 1.0f);
    for (int y = static_cast<int>(std::floor(pos.y)) - radius; y <= static_cast<int>(std::floor(pos.y)) + radius; ++y) {
        for (int x = static_cast<int>(std::floor(pos.x)) - radius; x <= static_cast<int>(std::floor(pos.x)) + radius; ++x) {
            if (x < 0 || x >= textureSize_.x || y < 0 || y >= textureSize_.y) continue;
            const std::size_t idx = static_cast<std::size_t>(indexFor(x, y));
            trailA_[idx] = ofClamp(trailA_[idx] + amount * (1.0f - mix), 0.0f, 1.0f);
            trailB_[idx] = ofClamp(trailB_[idx] + amount * mix, 0.0f, 1.0f);

            if (paramMirrorX_ || paramMirrorY_) {
                const int mx = paramMirrorX_ ? textureSize_.x - 1 - x : x;
                const int my = paramMirrorY_ ? textureSize_.y - 1 - y : y;
                if (mx >= 0 && mx < textureSize_.x && my >= 0 && my < textureSize_.y) {
                    const std::size_t mirrorIdx = static_cast<std::size_t>(indexFor(mx, my));
                    trailA_[mirrorIdx] = ofClamp(trailA_[mirrorIdx] + amount * 0.75f * (1.0f - mix), 0.0f, 1.0f);
                    trailB_[mirrorIdx] = ofClamp(trailB_[mirrorIdx] + amount * 0.75f * mix, 0.0f, 1.0f);
                }
            }
        }
    }
}

void FlowFieldLayer::syncTexture() {
    if (!pixels_.isAllocated()) return;

    const ofFloatColor bg(ofClamp(paramBgR_, 0.0f, 1.0f),
                          ofClamp(paramBgG_, 0.0f, 1.0f),
                          ofClamp(paramBgB_, 0.0f, 1.0f),
                          ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f));
    const ofFloatColor colorA(ofClamp(paramColorAR_, 0.0f, 1.5f),
                              ofClamp(paramColorAG_, 0.0f, 1.5f),
                              ofClamp(paramColorAB_, 0.0f, 1.5f),
                              ofClamp(paramTrailAlpha_, 0.0f, 1.0f));
    const ofFloatColor colorB(ofClamp(paramColorBR_, 0.0f, 1.5f),
                              ofClamp(paramColorBG_, 0.0f, 1.5f),
                              ofClamp(paramColorBB_, 0.0f, 1.5f),
                              ofClamp(paramTrailAlpha_, 0.0f, 1.0f));

    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const std::size_t idx = static_cast<std::size_t>(indexFor(x, y));
            const float a = ofClamp(trailA_[idx] * paramTrailBoost_, 0.0f, 1.0f);
            const float b = ofClamp(trailB_[idx] * paramTrailBoost_, 0.0f, 1.0f);
            const float intensity = ofClamp(a + b, 0.0f, 1.0f);
            const float denom = std::max(0.0001f, a + b);
            const float mix = b / denom;
            const ofFloatColor trail(ofLerp(colorA.r, colorB.r, mix),
                                     ofLerp(colorA.g, colorB.g, mix),
                                     ofLerp(colorA.b, colorB.b, mix),
                                     ofLerp(colorA.a, colorB.a, mix));

            pixels_.setColor(x, y, ofFloatColor(ofClamp(ofLerp(bg.r, trail.r, intensity), 0.0f, 1.0f),
                                                ofClamp(ofLerp(bg.g, trail.g, intensity), 0.0f, 1.0f),
                                                ofClamp(ofLerp(bg.b, trail.b, intensity), 0.0f, 1.0f),
                                                ofClamp(ofLerp(bg.a, trail.a, intensity), 0.0f, 1.0f)));
        }
    }

    texture_.loadData(pixels_);
}

void FlowFieldLayer::drawVectorOverlay(const LayerDrawParams& params) const {
    if (!paramVectorOverlay_ || paramVectorAlpha_ <= 0.0f) return;

    const float spacing = std::max(8.0f, paramVectorSpacing_);
    const float sx = params.viewport.x / static_cast<float>(textureSize_.x);
    const float sy = params.viewport.y / static_cast<float>(textureSize_.y);
    const float arrowLength = spacing * paramVectorScale_;
    ofSetLineWidth(1.0f);
    ofSetColor(static_cast<int>(ofClamp(paramColorAR_, 0.0f, 1.0f) * 255.0f),
               static_cast<int>(ofClamp(paramColorAG_, 0.0f, 1.0f) * 255.0f),
               static_cast<int>(ofClamp(paramColorAB_, 0.0f, 1.0f) * 255.0f),
               static_cast<int>(ofClamp(paramVectorAlpha_ * paramAlpha_ * params.slotOpacity, 0.0f, 1.0f) * 255.0f));

    for (float y = spacing * 0.5f; y < static_cast<float>(textureSize_.y); y += spacing) {
        for (float x = spacing * 0.5f; x < static_cast<float>(textureSize_.x); x += spacing) {
            const glm::vec2 pos(x, y);
            const glm::vec2 dir = safeNormalize(flowAt(pos, ofGetElapsedTimef()));
            const float px = x * sx;
            const float py = y * sy;
            const float ex = (x + dir.x * arrowLength) * sx;
            const float ey = (y + dir.y * arrowLength) * sy;
            ofDrawLine(px, py, ex, ey);
            const glm::vec2 side(-dir.y, dir.x);
            const glm::vec2 head = glm::vec2(ex, ey);
            const float headSize = std::max(2.0f, arrowLength * 0.25f);
            ofDrawLine(head.x, head.y, head.x - (dir.x + side.x * 0.45f) * headSize * sx, head.y - (dir.y + side.y * 0.45f) * headSize * sy);
            ofDrawLine(head.x, head.y, head.x - (dir.x - side.x * 0.45f) * headSize * sx, head.y - (dir.y - side.y * 0.45f) * headSize * sy);
        }
    }
}

void FlowFieldLayer::clampParams() {
    paramSpeed_ = ofClamp(paramSpeed_, 0.0f, 60.0f);
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 16.0f);
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 128.0f));
    paramParticleCount_ = std::round(ofClamp(paramParticleCount_, 16.0f, 4096.0f));
    paramParticleLife_ = std::round(ofClamp(paramParticleLife_, 16.0f, 2048.0f));
    paramRespawnRate_ = ofClamp(paramRespawnRate_, 0.0f, 0.2f);
    paramSpawnRadius_ = ofClamp(paramSpawnRadius_, 0.0f, 1.0f);
    paramFieldScale_ = ofClamp(paramFieldScale_, 0.2f, 16.0f);
    paramFieldStrength_ = ofClamp(paramFieldStrength_, 0.0f, 4.0f);
    paramFlowSpeed_ = ofClamp(paramFlowSpeed_, -2.0f, 2.0f);
    paramCurlAmount_ = ofClamp(paramCurlAmount_, 0.0f, 1.0f);
    paramTurbulence_ = ofClamp(paramTurbulence_, 0.0f, 2.0f);
    paramStepSize_ = ofClamp(paramStepSize_, 0.1f, 6.0f);
    paramInertia_ = ofClamp(paramInertia_, 0.0f, 0.98f);
    paramCenterPull_ = ofClamp(paramCenterPull_, -1.0f, 1.0f);
    paramDriftX_ = ofClamp(paramDriftX_, -2.0f, 2.0f);
    paramDriftY_ = ofClamp(paramDriftY_, -2.0f, 2.0f);
    paramTrailFade_ = ofClamp(paramTrailFade_, 0.0f, 0.2f);
    paramTrailDeposit_ = ofClamp(paramTrailDeposit_, 0.0f, 1.0f);
    paramTrailBoost_ = ofClamp(paramTrailBoost_, 0.1f, 6.0f);
    paramBackgroundAlpha_ = ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramPointSize_ = ofClamp(paramPointSize_, 0.0f, 4.0f);
    paramVectorSpacing_ = ofClamp(paramVectorSpacing_, 8.0f, 64.0f);
    paramVectorScale_ = ofClamp(paramVectorScale_, 0.05f, 1.5f);
    paramVectorAlpha_ = ofClamp(paramVectorAlpha_, 0.0f, 1.0f);
    paramColorBias_ = ofClamp(paramColorBias_, 0.0f, 1.0f);
    paramPaletteRate_ = ofClamp(paramPaletteRate_, -2.0f, 2.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramColorAR_ = ofClamp(paramColorAR_, 0.0f, 1.5f);
    paramColorAG_ = ofClamp(paramColorAG_, 0.0f, 1.5f);
    paramColorAB_ = ofClamp(paramColorAB_, 0.0f, 1.5f);
    paramColorBR_ = ofClamp(paramColorBR_, 0.0f, 1.5f);
    paramColorBG_ = ofClamp(paramColorBG_, 0.0f, 1.5f);
    paramColorBB_ = ofClamp(paramColorBB_, 0.0f, 1.5f);
}

float FlowFieldLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

float FlowFieldLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) return 0.0f;
    return std::max(0.0f, timeSeconds) * (bpm / 60.0f);
}

glm::vec2 FlowFieldLayer::flowAt(const glm::vec2& pos, float timeSeconds) const {
    const float nx = pos.x / std::max(1.0f, static_cast<float>(textureSize_.x));
    const float ny = pos.y / std::max(1.0f, static_cast<float>(textureSize_.y));
    const float scale = std::max(0.001f, paramFieldScale_);
    const float z = timeSeconds * paramFlowSpeed_;

    const float n = noiseSample(nx * scale + 11.7f, ny * scale - 3.2f, z);
    const float n2 = noiseSample(nx * scale * 2.1f - 19.4f, ny * scale * 2.1f + 8.3f, z * 1.37f);
    const float angle = (n * 2.0f + n2 * paramTurbulence_) * TWO_PI;
    glm::vec2 direction(std::cos(angle), std::sin(angle));

    const float left = noiseSample((nx - kNoiseEpsilon) * scale, ny * scale, z);
    const float right = noiseSample((nx + kNoiseEpsilon) * scale, ny * scale, z);
    const float up = noiseSample(nx * scale, (ny - kNoiseEpsilon) * scale, z);
    const float down = noiseSample(nx * scale, (ny + kNoiseEpsilon) * scale, z);
    glm::vec2 curl(down - up, -(right - left));
    curl = safeNormalize(curl, direction);
    direction = safeNormalize(direction * (1.0f - paramCurlAmount_) + curl * paramCurlAmount_, direction);

    if (std::abs(paramCenterPull_) > 0.0001f) {
        const glm::vec2 center(static_cast<float>(textureSize_.x) * 0.5f, static_cast<float>(textureSize_.y) * 0.5f);
        glm::vec2 toCenter = safeNormalize(center - pos, glm::vec2(0.0f, 1.0f));
        direction = safeNormalize(direction + toCenter * paramCenterPull_, direction);
    }

    direction += glm::vec2(paramDriftX_, paramDriftY_) * 0.35f;
    return safeNormalize(direction);
}

float FlowFieldLayer::noiseSample(float x, float y, float z) const {
    return ofNoise(x, y, z);
}

int FlowFieldLayer::indexFor(int x, int y) const {
    return y * textureSize_.x + x;
}

bool FlowFieldLayer::outOfBounds(const glm::vec2& pos) const {
    return pos.x < 0.0f || pos.x >= static_cast<float>(textureSize_.x) ||
           pos.y < 0.0f || pos.y >= static_cast<float>(textureSize_.y);
}

glm::vec2 FlowFieldLayer::wrapPosition(glm::vec2 pos) const {
    pos.x = wrapCoord(pos.x, static_cast<float>(textureSize_.x));
    pos.y = wrapCoord(pos.y, static_cast<float>(textureSize_.y));
    return pos;
}

void FlowFieldLayer::triggerReset() {
    resetParticles();
}

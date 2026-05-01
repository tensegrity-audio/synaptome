#include "FlockingLayer.h"
#include "ofGraphics.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
    constexpr int kModeCount = 2;
    const char* kModeLabels[kModeCount] = {
        "Murmuration",
        "Predator / Prey"
    };

    std::string modeDescriptions() {
        std::string desc;
        for (int i = 0; i < kModeCount; ++i) {
            if (!desc.empty()) desc += "  ";
            desc += ofToString(i) + "=" + kModeLabels[i];
        }
        return desc;
    }
}

void FlockingLayer::configure(const ofJson& config) {
    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramMode_ = def.value("mode", paramMode_);
        paramBoidCount_ = def.value("boidCount", paramBoidCount_);
        paramPredatorCount_ = def.value("predatorCount", paramPredatorCount_);
        paramCohesion_ = def.value("cohesion", paramCohesion_);
        paramAlignment_ = def.value("alignment", paramAlignment_);
        paramSeparation_ = def.value("separation", paramSeparation_);
        paramChase_ = def.value("chase", paramChase_);
        paramEvade_ = def.value("evade", paramEvade_);
        paramNoise_ = def.value("noise", paramNoise_);
        paramTrailFade_ = def.value("trailFade", paramTrailFade_);
        paramTrailDeposit_ = def.value("trailDeposit", paramTrailDeposit_);
        paramPointSize_ = def.value("pointSize", paramPointSize_);
        paramBackgroundAlpha_ = def.value("backgroundAlpha", paramBackgroundAlpha_);
        paramTrailAlpha_ = def.value("trailAlpha", paramTrailAlpha_);
        paramPreyAlpha_ = def.value("preyAlpha", paramPreyAlpha_);
        paramPredAlpha_ = def.value("predAlpha", paramPredAlpha_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(32, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(32, config["textureSize"][1].get<int>());
    }
}

void FlockingLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.flocking" : registryPrefix();

    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = "Flock Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "Flock Speed";
    meta.range.min = 0.0f;
    meta.range.max = 40.0f;
    meta.range.step = 0.1f;
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Flock BPM Sync";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);

    meta.label = "Flock BPM Mult";
    meta.range.min = 0.25f;
    meta.range.max = 8.0f;
    meta.range.step = 0.25f;
    registry.addFloat(prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Flock Alpha";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Flock Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Flock Mode";
    meta.range.min = 0.0f;
    meta.range.max = static_cast<float>(kModeCount - 1);
    meta.range.step = 1.0f;
    meta.description = modeDescriptions();
    registry.addFloat(prefix + ".mode", &paramMode_, paramMode_, meta);

    meta.label = "Flock Boids";
    meta.range.min = 8.0f;
    meta.range.max = 160.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".boidCount", &paramBoidCount_, paramBoidCount_, meta);

    meta.label = "Flock Predators";
    meta.range.min = 0.0f;
    meta.range.max = 24.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".predatorCount", &paramPredatorCount_, paramPredatorCount_, meta);

    meta.label = "Flock Cohesion";
    meta.range.min = 0.0f;
    meta.range.max = 0.05f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".cohesion", &paramCohesion_, paramCohesion_, meta);

    meta.label = "Flock Alignment";
    meta.range.min = 0.0f;
    meta.range.max = 0.08f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".alignment", &paramAlignment_, paramAlignment_, meta);

    meta.label = "Flock Separation";
    meta.range.min = 0.0f;
    meta.range.max = 0.02f;
    meta.range.step = 0.0005f;
    registry.addFloat(prefix + ".separation", &paramSeparation_, paramSeparation_, meta);

    meta.label = "Flock Chase";
    meta.range.min = 0.0f;
    meta.range.max = 0.05f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".chase", &paramChase_, paramChase_, meta);

    meta.label = "Flock Evade";
    meta.range.min = 0.0f;
    meta.range.max = 0.05f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".evade", &paramEvade_, paramEvade_, meta);

    meta.label = "Flock Noise";
    meta.range.min = 0.0f;
    meta.range.max = 0.03f;
    meta.range.step = 0.0005f;
    registry.addFloat(prefix + ".noise", &paramNoise_, paramNoise_, meta);

    meta.label = "Flock Trail Fade";
    meta.range.min = 0.0f;
    meta.range.max = 0.2f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".trailFade", &paramTrailFade_, paramTrailFade_, meta);

    meta.label = "Flock Trail Deposit";
    meta.range.min = 0.01f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".trailDeposit", &paramTrailDeposit_, paramTrailDeposit_, meta);

    meta.label = "Flock Point Size";
    meta.range.min = 1.0f;
    meta.range.max = 6.0f;
    meta.range.step = 0.25f;
    registry.addFloat(prefix + ".pointSize", &paramPointSize_, paramPointSize_, meta);

    meta.label = "Flock Bg Alpha";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".backgroundAlpha", &paramBackgroundAlpha_, paramBackgroundAlpha_, meta);

    meta.label = "Flock Trail Alpha";
    registry.addFloat(prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, meta);

    meta.label = "Flock Prey Alpha";
    registry.addFloat(prefix + ".preyAlpha", &paramPreyAlpha_, paramPreyAlpha_, meta);

    meta.label = "Flock Pred Alpha";
    registry.addFloat(prefix + ".predAlpha", &paramPredAlpha_, paramPredAlpha_, meta);

    allocateTrail();
    resetSimulation();
    syncTexture();
}

void FlockingLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    paramMode_ = std::round(ofClamp(paramMode_, 0.0f, static_cast<float>(kModeCount - 1)));
    paramBoidCount_ = std::round(ofClamp(paramBoidCount_, 8.0f, 160.0f));
    paramPredatorCount_ = std::round(ofClamp(paramPredatorCount_, 0.0f, 24.0f));
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 8.0f);
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramPointSize_ = ofClamp(paramPointSize_, 1.0f, 6.0f);
    paramBackgroundAlpha_ = ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramPreyAlpha_ = ofClamp(paramPreyAlpha_, 0.0f, 1.0f);
    paramPredAlpha_ = ofClamp(paramPredAlpha_, 0.0f, 1.0f);

    if (paramReseedRequested_ ||
        static_cast<int>(boids_.size()) != static_cast<int>(paramBoidCount_) ||
        static_cast<int>(predators_.size()) != static_cast<int>(paramPredatorCount_)) {
        resetSimulation();
        paramReseedRequested_ = false;
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
    int iterations = std::min(24, static_cast<int>(std::floor(stepAccumulator_)));
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
        if (static_cast<int>(paramMode_) == PredatorPrey) {
            stepPredatorPrey(1.0f / static_cast<float>(iterations));
        } else {
            stepMurmuration(1.0f / static_cast<float>(iterations), params.time);
        }
    }

    syncTexture();
    dirty_ = false;
}

void FlockingLayer::draw(const LayerDrawParams& params) {
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

void FlockingLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void FlockingLayer::allocateTrail() {
    const std::size_t count = static_cast<std::size_t>(textureSize_.x * textureSize_.y);
    trail_.assign(count, 0.0f);
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
}

void FlockingLayer::resetSimulation() {
    if (trail_.empty()) {
        allocateTrail();
    }

    std::fill(trail_.begin(), trail_.end(), 0.0f);
    boids_.assign(static_cast<std::size_t>(std::round(paramBoidCount_)), {});
    predators_.assign(static_cast<std::size_t>(std::round(paramPredatorCount_)), {});

    for (auto& boid : boids_) {
        boid.pos = { ofRandom(textureSize_.x), ofRandom(textureSize_.y) };
        glm::vec2 dir(ofRandom(-1.0f, 1.0f), ofRandom(-1.0f, 1.0f));
        if (glm::dot(dir, dir) < 0.0001f) dir = { 1.0f, 0.0f };
        boid.vel = glm::normalize(dir) * ofRandom(0.2f, 0.6f);
        depositTrail(boid.pos, paramTrailDeposit_);
    }
    for (auto& predator : predators_) {
        predator.pos = { ofRandom(textureSize_.x), ofRandom(textureSize_.y) };
        glm::vec2 dir(ofRandom(-1.0f, 1.0f), ofRandom(-1.0f, 1.0f));
        if (glm::dot(dir, dir) < 0.0001f) dir = { -1.0f, 0.0f };
        predator.vel = glm::normalize(dir) * ofRandom(0.15f, 0.45f);
    }

    dirty_ = true;
}

void FlockingLayer::fadeTrail() {
    for (auto& cell : trail_) {
        cell = ofClamp(cell - paramTrailFade_, 0.0f, 1.0f);
    }
}

void FlockingLayer::stepMurmuration(float dtScale, float time) {
    for (auto& boid : boids_) {
        glm::vec2 cohesion(0.0f);
        glm::vec2 alignment(0.0f);
        glm::vec2 separation(0.0f);
        int neighbors = 0;

        for (const auto& other : boids_) {
            if (&boid == &other) continue;
            glm::vec2 delta = other.pos - boid.pos;
            float d2 = glm::dot(delta, delta);
            if (d2 < 70.0f) {
                cohesion += other.pos;
                alignment += other.vel;
                ++neighbors;
            }
            if (d2 > 0.001f && d2 < 9.0f) {
                separation -= delta;
            }
        }

        if (neighbors > 0) {
            boid.vel += ((cohesion / static_cast<float>(neighbors)) - boid.pos) * paramCohesion_;
            boid.vel += ((alignment / static_cast<float>(neighbors)) - boid.vel) * paramAlignment_;
            boid.vel += separation * paramSeparation_;
        }

        boid.vel.x += std::sin(boid.pos.y * 0.2f + time * 0.8f) * paramNoise_;
        boid.vel.y += std::cos(boid.pos.x * 0.16f + time * 1.0f) * paramNoise_;
        clampSpeed(boid.vel, 0.38f, 0.8f, 1.2f);
        boid.pos = wrapPosition(boid.pos + boid.vel * (1.0f + dtScale));
        depositTrail(boid.pos, paramTrailDeposit_);
    }
}

void FlockingLayer::stepPredatorPrey(float dtScale) {
    for (auto& prey : boids_) {
        glm::vec2 cohesion(0.0f);
        glm::vec2 alignment(0.0f);
        int neighbors = 0;
        for (const auto& other : boids_) {
            if (&prey == &other) continue;
            glm::vec2 delta = other.pos - prey.pos;
            float d2 = glm::dot(delta, delta);
            if (d2 < 64.0f) {
                cohesion += other.pos;
                alignment += other.vel;
                ++neighbors;
            }
        }
        if (neighbors > 0) {
            prey.vel += ((cohesion / static_cast<float>(neighbors)) - prey.pos) * paramCohesion_;
            prey.vel += ((alignment / static_cast<float>(neighbors)) - prey.vel) * paramAlignment_;
        }
        for (const auto& predator : predators_) {
            glm::vec2 delta = predator.pos - prey.pos;
            float d2 = glm::dot(delta, delta);
            if (d2 < 90.0f) {
                prey.vel -= delta * paramEvade_;
            }
        }
        clampSpeed(prey.vel, 0.42f, 0.8f, 1.25f);
        prey.pos = wrapPosition(prey.pos + prey.vel * (1.0f + dtScale));
        depositTrail(prey.pos, paramTrailDeposit_);
    }

    for (auto& predator : predators_) {
        if (boids_.empty()) break;
        std::size_t best = 0;
        float bestDist = std::numeric_limits<float>::max();
        for (std::size_t i = 0; i < boids_.size(); ++i) {
            glm::vec2 delta = boids_[i].pos - predator.pos;
            float d2 = glm::dot(delta, delta);
            if (d2 < bestDist) {
                bestDist = d2;
                best = i;
            }
        }
        predator.vel += (boids_[best].pos - predator.pos) * paramChase_;
        clampSpeed(predator.vel, 0.36f, 0.85f, 1.25f);
        predator.pos = wrapPosition(predator.pos + predator.vel * (1.0f + dtScale));
        depositTrail(predator.pos, paramTrailDeposit_ * 1.4f);
    }
}

void FlockingLayer::depositTrail(const glm::vec2& pos, float amount) {
    int x = static_cast<int>(ofClamp(std::floor(pos.x), 0.0f, static_cast<float>(textureSize_.x - 1)));
    int y = static_cast<int>(ofClamp(std::floor(pos.y), 0.0f, static_cast<float>(textureSize_.y - 1)));
    const std::size_t idx = static_cast<std::size_t>(y * textureSize_.x + x);
    trail_[idx] = ofClamp(trail_[idx] + amount, 0.0f, 1.0f);
}

void FlockingLayer::syncTexture() {
    if (!pixels_.isAllocated()) return;
    const ofFloatColor preyColor(ofClamp(paramPreyR_, 0.0f, 1.0f),
                                 ofClamp(paramPreyG_, 0.0f, 1.0f),
                                 ofClamp(paramPreyB_, 0.0f, 1.0f),
                                 ofClamp(paramPreyAlpha_, 0.0f, 1.0f));
    const ofFloatColor predColor(ofClamp(paramPredR_, 0.0f, 1.0f),
                                 ofClamp(paramPredG_, 0.0f, 1.0f),
                                 ofClamp(paramPredB_, 0.0f, 1.0f),
                                 ofClamp(paramPredAlpha_, 0.0f, 1.0f));

    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const float value = trail_[static_cast<std::size_t>(y * textureSize_.x + x)];
            pixels_.setColor(x, y, ofFloatColor(ofLerp(paramBgR_, paramTrailR_, value),
                                                ofLerp(paramBgG_, paramTrailG_, value),
                                                ofLerp(paramBgB_, paramTrailB_, value),
                                                ofLerp(paramBackgroundAlpha_, paramTrailAlpha_, value)));
        }
    }

    const int preyRadius = std::max(0, static_cast<int>(std::round(paramPointSize_ * 0.5f)) - 1);
    const int predRadius = std::max(preyRadius, static_cast<int>(std::round(paramPointSize_ * 0.5f)));
    for (const auto& boid : boids_) {
        stampMarker(pixels_, boid.pos, preyColor, preyRadius);
    }
    for (const auto& predator : predators_) {
        stampMarker(pixels_, predator.pos, predColor, predRadius);
    }

    texture_.loadData(pixels_);
}

float FlockingLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

glm::vec2 FlockingLayer::wrapPosition(glm::vec2 pos) const {
    if (pos.x < 0.0f) pos.x += textureSize_.x;
    if (pos.x >= textureSize_.x) pos.x -= textureSize_.x;
    if (pos.y < 0.0f) pos.y += textureSize_.y;
    if (pos.y >= textureSize_.y) pos.y -= textureSize_.y;
    return pos;
}

void FlockingLayer::clampSpeed(glm::vec2& vel, float target, float minScale, float maxScale) const {
    float speed = glm::length(vel);
    if (speed <= 0.0001f) return;
    float scale = ofClamp(target / speed, minScale, maxScale);
    vel *= scale;
}

void FlockingLayer::stampMarker(ofFloatPixels& pixels, const glm::vec2& pos, const ofFloatColor& color, int radius) const {
    const int cx = static_cast<int>(std::round(pos.x));
    const int cy = static_cast<int>(std::round(pos.y));
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const int x = cx + dx;
            const int y = cy + dy;
            if (x < 0 || x >= textureSize_.x || y < 0 || y >= textureSize_.y) continue;
            pixels.setColor(x, y, color);
        }
    }
}

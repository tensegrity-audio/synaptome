#include "FlockingLayer.h"
#include "ofGraphics.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {
    constexpr int kModeCount = 3;
    const char* kSchoolingModeLabels[kModeCount] = {
        "Gather",
        "Scatter",
        "Panic"
    };
    const char* kMurmurationModeLabels[kModeCount] = {
        "Fold",
        "Bloom",
        "Ripple"
    };

    std::string modeDescriptions(bool schoolingModel) {
        const char* const* labels = schoolingModel ? kSchoolingModeLabels : kMurmurationModeLabels;
        std::string desc;
        for (int i = 0; i < kModeCount; ++i) {
            if (!desc.empty()) desc += "  ";
            desc += ofToString(i) + "=" + labels[i];
        }
        return desc;
    }
}

void FlockingLayer::configure(const ofJson& config) {
    const std::string model = config.value("model", std::string());
    if (model == "schooling") {
        model_ = Schooling;
    } else if (model == "murmuration") {
        model_ = Murmuration;
    }

    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        const bool legacyAlgorithmMode = model.empty() && def.contains("mode");
        if (model.empty()) {
            const int legacyMode = static_cast<int>(std::round(def.value("mode", paramMode_)));
            model_ = legacyMode == 1 ? Schooling : Murmuration;
            if (legacyMode == 1 && !def.contains("predatorEnabled")) {
                paramPredatorEnabled_ = true;
            }
        }
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramMode_ = def.value("mode", paramMode_);
        if (legacyAlgorithmMode) {
            paramMode_ = 0.0f;
        }
        paramBoidCount_ = def.value("boidCount", paramBoidCount_);
        paramPredatorEnabled_ = def.value("predatorEnabled", paramPredatorEnabled_);
        paramPredatorCount_ = def.value("predatorCount", paramPredatorCount_);
        paramPredatorPressure_ = def.value("predatorPressure", paramPredatorPressure_);
        paramNeighborCount_ = def.value("neighborCount", paramNeighborCount_);
        paramCohesion_ = def.value("cohesion", paramCohesion_);
        paramAlignment_ = def.value("alignment", paramAlignment_);
        paramSeparation_ = def.value("separation", paramSeparation_);
        paramPredatorSeparation_ = def.value("predatorSeparation", paramPredatorSeparation_);
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
        paramBgR_ = def.value("bgR", paramBgR_);
        paramBgG_ = def.value("bgG", paramBgG_);
        paramBgB_ = def.value("bgB", paramBgB_);
        paramTrailR_ = def.value("trailR", paramTrailR_);
        paramTrailG_ = def.value("trailG", paramTrailG_);
        paramTrailB_ = def.value("trailB", paramTrailB_);
        paramPreyR_ = def.value("preyR", paramPreyR_);
        paramPreyG_ = def.value("preyG", paramPreyG_);
        paramPreyB_ = def.value("preyB", paramPreyB_);
        paramPredR_ = def.value("predatorR", paramPredR_);
        paramPredG_ = def.value("predatorG", paramPredG_);
        paramPredB_ = def.value("predatorB", paramPredB_);
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
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "Time: Flock Speed";
    meta.range.min = 0.0f;
    meta.range.max = 40.0f;
    meta.range.step = 0.1f;
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: BPM Sync";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);

    meta.label = "Time: BPM Mult";
    meta.range.min = 0.25f;
    meta.range.max = 8.0f;
    meta.range.step = 0.25f;
    registry.addFloat(prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Visibility: Flock Opacity";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = model_ == Schooling ? "Action: School Mode" : "Action: Murmuration Mode";
    meta.range.min = 0.0f;
    meta.range.max = static_cast<float>(kModeCount - 1);
    meta.range.step = 1.0f;
    meta.description = modeDescriptions(model_ == Schooling);
    registry.addFloat(prefix + ".mode", &paramMode_, paramMode_, meta);

    meta.label = "Count: Prey";
    meta.range.min = 8.0f;
    meta.range.max = 512.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".boidCount", &paramBoidCount_, paramBoidCount_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: Predator Enabled";
    registry.addBool(prefix + ".predatorEnabled", &paramPredatorEnabled_, paramPredatorEnabled_, meta);

    meta.label = "Count: Predators";
    meta.range.min = 0.0f;
    meta.range.max = 24.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".predatorCount", &paramPredatorCount_, paramPredatorCount_, meta);

    meta.label = "Force: Predator Pressure";
    meta.range.min = 0.0f;
    meta.range.max = 2.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".predatorPressure", &paramPredatorPressure_, paramPredatorPressure_, meta);

    meta.label = model_ == Schooling ? "Scale: School Radius" : "Count: Topological Neighbors";
    meta.range.min = 1.0f;
    meta.range.max = 12.0f;
    meta.range.step = 1.0f;
    meta.description = model_ == Schooling
        ? "Metric neighborhood radius: higher values make the school clump and lane over a larger local zone."
        : "Topological neighborhood count: each bird follows this many nearest birds, independent of metric distance.";
    registry.addFloat(prefix + ".neighborCount", &paramNeighborCount_, paramNeighborCount_, meta);

    meta.label = "Force: Cohesion";
    meta.range.min = 0.0f;
    meta.range.max = 0.05f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".cohesion", &paramCohesion_, paramCohesion_, meta);

    meta.label = "Force: Alignment";
    meta.range.min = 0.0f;
    meta.range.max = 0.08f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".alignment", &paramAlignment_, paramAlignment_, meta);

    meta.label = "Force: Prey Separation";
    meta.range.min = 0.0f;
    meta.range.max = 0.02f;
    meta.range.step = 0.0005f;
    registry.addFloat(prefix + ".separation", &paramSeparation_, paramSeparation_, meta);

    meta.label = "Force: Predator Separation";
    meta.range.min = 0.0f;
    meta.range.max = 0.12f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".predatorSeparation", &paramPredatorSeparation_, paramPredatorSeparation_, meta);

    meta.label = "Force: Chase";
    meta.range.min = 0.0f;
    meta.range.max = 0.05f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".chase", &paramChase_, paramChase_, meta);

    meta.label = "Force: Evade";
    meta.range.min = 0.0f;
    meta.range.max = 0.05f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".evade", &paramEvade_, paramEvade_, meta);

    meta.label = "Motion: Noise";
    meta.range.min = 0.0f;
    meta.range.max = 0.03f;
    meta.range.step = 0.0005f;
    registry.addFloat(prefix + ".noise", &paramNoise_, paramNoise_, meta);

    meta.label = "Time: Trail Fade";
    meta.range.min = 0.0f;
    meta.range.max = 0.2f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".trailFade", &paramTrailFade_, paramTrailFade_, meta);

    meta.label = "Glow: Trail Deposit";
    meta.range.min = 0.01f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".trailDeposit", &paramTrailDeposit_, paramTrailDeposit_, meta);

    meta.label = "Scale: Point Size";
    meta.range.min = 1.0f;
    meta.range.max = 6.0f;
    meta.range.step = 0.25f;
    registry.addFloat(prefix + ".pointSize", &paramPointSize_, paramPointSize_, meta);

    meta.label = "Visibility: Background Opacity";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".backgroundAlpha", &paramBackgroundAlpha_, paramBackgroundAlpha_, meta);

    meta.label = "Visibility: Trail Opacity";
    registry.addFloat(prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, meta);

    meta.label = "Visibility: Prey Opacity";
    registry.addFloat(prefix + ".preyAlpha", &paramPreyAlpha_, paramPreyAlpha_, meta);

    meta.label = "Visibility: Predator Opacity";
    registry.addFloat(prefix + ".predAlpha", &paramPredAlpha_, paramPredAlpha_, meta);

    meta = {};
    meta.group = "Generative";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    meta.label = "Color: Background R";
    registry.addFloat(prefix + ".bgR", &paramBgR_, paramBgR_, meta);
    meta.label = "Color: Background G";
    registry.addFloat(prefix + ".bgG", &paramBgG_, paramBgG_, meta);
    meta.label = "Color: Background B";
    registry.addFloat(prefix + ".bgB", &paramBgB_, paramBgB_, meta);
    meta.label = "Color: Trail R";
    registry.addFloat(prefix + ".trailR", &paramTrailR_, paramTrailR_, meta);
    meta.label = "Color: Trail G";
    registry.addFloat(prefix + ".trailG", &paramTrailG_, paramTrailG_, meta);
    meta.label = "Color: Trail B";
    registry.addFloat(prefix + ".trailB", &paramTrailB_, paramTrailB_, meta);
    meta.label = "Color: Prey R";
    registry.addFloat(prefix + ".preyR", &paramPreyR_, paramPreyR_, meta);
    meta.label = "Color: Prey G";
    registry.addFloat(prefix + ".preyG", &paramPreyG_, paramPreyG_, meta);
    meta.label = "Color: Prey B";
    registry.addFloat(prefix + ".preyB", &paramPreyB_, paramPreyB_, meta);
    meta.label = "Color: Predator R";
    registry.addFloat(prefix + ".predatorR", &paramPredR_, paramPredR_, meta);
    meta.label = "Color: Predator G";
    registry.addFloat(prefix + ".predatorG", &paramPredG_, paramPredG_, meta);
    meta.label = "Color: Predator B";
    registry.addFloat(prefix + ".predatorB", &paramPredB_, paramPredB_, meta);

    allocateTrail();
    resetSimulation();
    syncTexture();
}

void FlockingLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    paramMode_ = std::round(ofClamp(paramMode_, 0.0f, static_cast<float>(kModeCount - 1)));
    paramBoidCount_ = std::round(ofClamp(paramBoidCount_, 8.0f, 512.0f));
    paramPredatorCount_ = std::round(ofClamp(paramPredatorCount_, 0.0f, 24.0f));
    paramPredatorPressure_ = ofClamp(paramPredatorPressure_, 0.0f, 2.0f);
    paramNeighborCount_ = std::round(ofClamp(paramNeighborCount_, 1.0f, 12.0f));
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 8.0f);
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramPointSize_ = ofClamp(paramPointSize_, 1.0f, 6.0f);
    paramBackgroundAlpha_ = ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramPreyAlpha_ = ofClamp(paramPreyAlpha_, 0.0f, 1.0f);
    paramPredAlpha_ = ofClamp(paramPredAlpha_, 0.0f, 1.0f);
    paramPredatorSeparation_ = ofClamp(paramPredatorSeparation_, 0.0f, 0.12f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramTrailR_ = ofClamp(paramTrailR_, 0.0f, 1.0f);
    paramTrailG_ = ofClamp(paramTrailG_, 0.0f, 1.0f);
    paramTrailB_ = ofClamp(paramTrailB_, 0.0f, 1.0f);
    paramPreyR_ = ofClamp(paramPreyR_, 0.0f, 1.0f);
    paramPreyG_ = ofClamp(paramPreyG_, 0.0f, 1.0f);
    paramPreyB_ = ofClamp(paramPreyB_, 0.0f, 1.0f);
    paramPredR_ = ofClamp(paramPredR_, 0.0f, 1.0f);
    paramPredG_ = ofClamp(paramPredG_, 0.0f, 1.0f);
    paramPredB_ = ofClamp(paramPredB_, 0.0f, 1.0f);

    if (paramReseedRequested_ ||
        static_cast<int>(boids_.size()) != static_cast<int>(paramBoidCount_) ||
        static_cast<int>(predators_.size()) != static_cast<int>(paramPredatorCount_)) {
        resetSimulation();
        paramReseedRequested_ = false;
    }

    const float stepRate = stepRateFor(params);
    if (stepRate <= 0.0f) {
        syncTexture();
        dirty_ = false;
        return;
    }

    stepAccumulator_ += params.dt * stepRate;
    int iterations = std::min(24, static_cast<int>(std::floor(stepAccumulator_)));
    if (iterations <= 0) {
        syncTexture();
        dirty_ = false;
        return;
    }

    stepAccumulator_ -= static_cast<float>(iterations);
    for (int i = 0; i < iterations; ++i) {
        fadeTrail();
        if (model_ == Schooling) {
            stepSchooling(1.0f / static_cast<float>(iterations));
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

void FlockingLayer::stepSchooling(float dtScale) {
    const float radiusControl = ofClamp(paramNeighborCount_, 1.0f, 12.0f);
    const float separationRadius = 2.5f + radiusControl * 0.55f;
    const float alignmentRadius = separationRadius + 4.0f + radiusControl * 0.85f;
    const float cohesionRadius = alignmentRadius + 3.0f + radiusControl * 0.9f;
    const float separationRadius2 = separationRadius * separationRadius;
    const float alignmentRadius2 = alignmentRadius * alignmentRadius;
    const float cohesionRadius2 = cohesionRadius * cohesionRadius;
    const float predatorThreatRadius = 15.0f + radiusControl * 1.5f;
    const float predatorThreatRadius2 = predatorThreatRadius * predatorThreatRadius;

    for (auto& boid : boids_) {
        glm::vec2 cohesion(0.0f);
        glm::vec2 alignment(0.0f);
        glm::vec2 separation(0.0f);
        int cohesionNeighbors = 0;
        int alignmentNeighbors = 0;

        for (const auto& other : boids_) {
            if (&boid == &other) continue;
            glm::vec2 delta = other.pos - boid.pos;
            float d2 = glm::dot(delta, delta);
            if (d2 < cohesionRadius2) {
                cohesion += other.pos;
                ++cohesionNeighbors;
            }
            if (d2 < alignmentRadius2) {
                alignment += other.vel;
                ++alignmentNeighbors;
            }
            if (d2 > 0.001f && d2 < separationRadius2) {
                const float distance = std::sqrt(d2);
                separation -= (delta / distance) * ((separationRadius - distance) / separationRadius);
            }
        }

        const int mode = behaviorMode();
        const float cohesionSign = mode == Diverge ? -0.65f : 1.0f;
        const float separationScale = mode == Diverge ? 2.2f : 1.0f;
        const float stressScale = mode == Stressed ? 1.6f : 1.0f;
        if (cohesionNeighbors > 0) {
            boid.vel += ((cohesion / static_cast<float>(cohesionNeighbors)) - boid.pos) * paramCohesion_ * cohesionSign;
        }
        if (alignmentNeighbors > 0) {
            boid.vel += ((alignment / static_cast<float>(alignmentNeighbors)) - boid.vel) * paramAlignment_;
        }
        boid.vel += separation * paramSeparation_ * separationScale;

        if (predatorsActive()) {
            for (const auto& predator : predators_) {
                glm::vec2 delta = predator.pos - boid.pos;
                float d2 = glm::dot(delta, delta);
                if (d2 > 0.001f && d2 < predatorThreatRadius2) {
                    const float distance = std::sqrt(d2);
                    const float falloff = (predatorThreatRadius - distance) / predatorThreatRadius;
                    boid.vel -= delta * paramEvade_ * paramPredatorPressure_ * (0.55f + falloff * 1.8f);
                }
            }
        }

        boid.vel.x += ofRandom(-1.0f, 1.0f) * paramNoise_ * stressScale;
        boid.vel.y += ofRandom(-1.0f, 1.0f) * paramNoise_ * stressScale;
        clampSpeed(boid.vel, 0.42f, 0.8f, 1.25f + (stressScale - 1.0f) * 0.2f);
        boid.pos = wrapPosition(boid.pos + boid.vel * (1.0f + dtScale));
        depositTrail(boid.pos, paramTrailDeposit_);
    }

    if (predatorsActive()) {
        stepPredators(dtScale);
    }
}

void FlockingLayer::stepMurmuration(float dtScale, float time) {
    const int maxNeighbors = std::max(1, static_cast<int>(paramNeighborCount_));
    const float predatorThreatRadius = 18.0f + ofClamp(paramNeighborCount_, 1.0f, 12.0f) * 1.7f;
    const float predatorThreatRadius2 = predatorThreatRadius * predatorThreatRadius;
    for (auto& boid : boids_) {
        std::vector<std::pair<float, const Boid*>> nearest;
        nearest.reserve(boids_.size());
        for (const auto& other : boids_) {
            if (&boid == &other) continue;
            glm::vec2 delta = other.pos - boid.pos;
            nearest.push_back({ glm::dot(delta, delta), &other });
        }
        std::sort(nearest.begin(), nearest.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        glm::vec2 cohesion(0.0f);
        glm::vec2 alignment(0.0f);
        glm::vec2 separation(0.0f);
        const int neighbors = std::min(maxNeighbors, static_cast<int>(nearest.size()));
        for (int i = 0; i < neighbors; ++i) {
            const auto& neighbor = nearest[static_cast<std::size_t>(i)];
            const Boid& other = *neighbor.second;
            glm::vec2 delta = other.pos - boid.pos;
            cohesion += other.pos;
            alignment += other.vel;
            if (neighbor.first > 0.001f && neighbor.first < 16.0f) {
                separation -= delta;
            }
        }

        if (neighbors > 0) {
            const int mode = behaviorMode();
            const float cohesionSign = mode == Diverge ? -0.45f : 1.0f;
            const float separationScale = mode == Diverge ? 2.0f : 1.0f;
            const float stressScale = mode == Stressed ? 1.9f : 1.0f;
            const float wave = std::sin((boid.pos.x + boid.pos.y) * 0.055f + time * 1.7f);
            glm::vec2 tangent(-boid.vel.y, boid.vel.x);
            if (glm::dot(tangent, tangent) > 0.0001f) {
                tangent = glm::normalize(tangent);
            }
            boid.vel += ((cohesion / static_cast<float>(neighbors)) - boid.pos) * paramCohesion_ * cohesionSign;
            boid.vel += ((alignment / static_cast<float>(neighbors)) - boid.vel) * paramAlignment_;
            boid.vel += separation * paramSeparation_ * separationScale;
            boid.vel += tangent * wave * paramNoise_ * stressScale * 1.8f;
            boid.vel.x += std::sin(boid.pos.y * 0.2f + time * 0.8f) * paramNoise_ * stressScale;
            boid.vel.y += std::cos(boid.pos.x * 0.16f + time * 1.0f) * paramNoise_ * stressScale;
        }

        if (predatorsActive()) {
            for (const auto& predator : predators_) {
                glm::vec2 delta = predator.pos - boid.pos;
                float d2 = glm::dot(delta, delta);
                if (d2 > 0.001f && d2 < predatorThreatRadius2) {
                    const float distance = std::sqrt(d2);
                    const float falloff = (predatorThreatRadius - distance) / predatorThreatRadius;
                    glm::vec2 away = -delta / distance;
                    glm::vec2 tangent(-away.y, away.x);
                    const float waveSign = std::sin(time * 1.3f + boid.pos.x * 0.11f + boid.pos.y * 0.07f) >= 0.0f ? 1.0f : -1.0f;
                    boid.vel += away * paramEvade_ * paramPredatorPressure_ * (0.55f + falloff * 1.4f);
                    boid.vel += tangent * waveSign * paramEvade_ * paramPredatorPressure_ * (0.35f + falloff * 0.9f);
                }
            }
        }
        clampSpeed(boid.vel, 0.38f, 0.8f, 1.2f);
        boid.pos = wrapPosition(boid.pos + boid.vel * (1.0f + dtScale));
        depositTrail(boid.pos, paramTrailDeposit_);
    }

    if (predatorsActive()) {
        stepPredators(dtScale);
    }
}

void FlockingLayer::stepPredators(float dtScale) {
    for (auto& predator : predators_) {
        if (boids_.empty()) break;
        glm::vec2 separation(0.0f);
        for (const auto& other : predators_) {
            if (&predator == &other) continue;
            glm::vec2 delta = other.pos - predator.pos;
            const float d2 = glm::dot(delta, delta);
            if (d2 > 0.001f && d2 < 100.0f) {
                separation -= delta * ((100.0f - d2) / 100.0f);
            }
        }
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
        glm::vec2 target = boids_[best].pos;
        if (model_ == Murmuration) {
            glm::vec2 centroid(0.0f);
            for (const auto& boid : boids_) {
                centroid += boid.pos;
            }
            centroid /= static_cast<float>(boids_.size());
            target = centroid * 0.65f + target * 0.35f;
        }

        predator.vel += separation * paramPredatorSeparation_;
        predator.vel += (target - predator.pos) * paramChase_ * paramPredatorPressure_ * (model_ == Schooling ? 1.15f : 0.55f);
        if (model_ == Murmuration) {
            glm::vec2 heading = predator.vel;
            if (glm::dot(heading, heading) > 0.0001f) {
                heading = glm::normalize(heading);
                glm::vec2 side(-heading.y, heading.x);
                const float weave = std::sin(predator.pos.x * 0.12f + predator.pos.y * 0.17f);
                predator.vel += side * weave * paramNoise_ * 2.0f;
            }
        }
        clampSpeed(predator.vel, model_ == Schooling ? 0.42f : 0.32f, 0.82f, model_ == Schooling ? 1.32f : 1.18f);
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
    const int predRadius = std::max(preyRadius + 2, static_cast<int>(std::round(paramPointSize_ * 1.35f)) + 1);
    for (const auto& boid : boids_) {
        stampMarker(pixels_, boid.pos, preyColor, preyRadius);
    }
    if (predatorsActive()) {
        for (const auto& predator : predators_) {
            stampPredator(pixels_, predator, predColor, predRadius);
        }
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

int FlockingLayer::behaviorMode() const {
    return static_cast<int>(std::round(ofClamp(paramMode_, 0.0f, static_cast<float>(kModeCount - 1))));
}

bool FlockingLayer::predatorsActive() const {
    return paramPredatorEnabled_ && paramPredatorPressure_ > 0.0f && !predators_.empty();
}

void FlockingLayer::stampMarker(ofFloatPixels& pixels, const glm::vec2& pos, const ofFloatColor& color, int radius) const {
    const int cx = static_cast<int>(std::round(pos.x));
    const int cy = static_cast<int>(std::round(pos.y));
    const float radius2 = static_cast<float>(radius * radius) + 0.25f;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const int x = cx + dx;
            const int y = cy + dy;
            if (x < 0 || x >= textureSize_.x || y < 0 || y >= textureSize_.y) continue;
            if (radius > 0 && static_cast<float>(dx * dx + dy * dy) > radius2) continue;
            pixels.setColor(x, y, color);
        }
    }
}

void FlockingLayer::stampPredator(ofFloatPixels& pixels, const Boid& predator, const ofFloatColor& color, int radius) const {
    glm::vec2 heading = predator.vel;
    if (glm::dot(heading, heading) < 0.0001f) {
        heading = { 1.0f, 0.0f };
    } else {
        heading = glm::normalize(heading);
    }
    const glm::vec2 side(-heading.y, heading.x);
    const glm::vec2 headOffset = heading * (static_cast<float>(radius) * 0.62f);
    const glm::vec2 tailOffset = -heading * (static_cast<float>(radius) * 0.72f);
    const float bodyLength = std::max(2.8f, static_cast<float>(radius) * 1.75f);
    const float bodyWidth = std::max(1.5f, static_cast<float>(radius) * 0.78f);
    const int bounds = static_cast<int>(std::ceil(static_cast<float>(radius) * 2.4f)) + 1;
    const int cx = static_cast<int>(std::round(predator.pos.x));
    const int cy = static_cast<int>(std::round(predator.pos.y));

    for (int dy = -bounds; dy <= bounds; ++dy) {
        for (int dx = -bounds; dx <= bounds; ++dx) {
            const int x = cx + dx;
            const int y = cy + dy;
            if (x < 0 || x >= textureSize_.x || y < 0 || y >= textureSize_.y) continue;

            const glm::vec2 rel(static_cast<float>(dx), static_cast<float>(dy));
            const float along = glm::dot(rel, heading);
            const float across = glm::dot(rel, side);
            const float taper = ofClamp(1.0f - std::abs(along) / (bodyLength + 0.001f) * 0.42f, 0.48f, 1.0f);
            const float edgeRipple = 0.9f + 0.1f * std::sin(static_cast<float>(x) * 1.7f + static_cast<float>(y) * 2.3f);
            const bool body = ((along * along) / (bodyLength * bodyLength) +
                               (across * across) / (bodyWidth * bodyWidth * taper * taper)) <= edgeRipple;

            const glm::vec2 headRel = rel - headOffset;
            const bool head = glm::dot(headRel, headRel) <= bodyWidth * bodyWidth * 0.85f;

            const glm::vec2 leftTail = rel - (tailOffset + side * bodyWidth * 0.55f);
            const glm::vec2 rightTail = rel - (tailOffset - side * bodyWidth * 0.55f);
            const float tailRadius = bodyWidth * 0.55f;
            const bool tail = glm::dot(leftTail, leftTail) <= tailRadius * tailRadius ||
                              glm::dot(rightTail, rightTail) <= tailRadius * tailRadius;

            const bool dorsal = std::abs(along + bodyLength * 0.12f) < bodyLength * 0.4f &&
                                std::abs(across) < bodyWidth * (0.18f + 0.12f * edgeRipple);
            if (body || head || tail || dorsal) {
                pixels.setColor(x, y, color);
            }
        }
    }
}

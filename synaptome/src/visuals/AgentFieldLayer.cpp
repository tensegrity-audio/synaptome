#include "AgentFieldLayer.h"
#include "LayerParameterBuilder.h"
#include "ofGraphics.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr int kModeCount = 3;
    const char* kModeLabels[kModeCount] = {
        "Balanced",
        "Explore",
        "Exploit"
    };

    std::string modeDescriptions() {
        std::string desc;
        for (int i = 0; i < kModeCount; ++i) {
            if (!desc.empty()) desc += "  ";
            desc += ofToString(i) + "=" + kModeLabels[i];
        }
        return desc;
    }

    float wrapCoord(float value, float limit) {
        while (value < 0.0f) value += limit;
        while (value >= limit) value -= limit;
        return value;
    }
}

void AgentFieldLayer::configure(const ofJson& config) {
    const std::string model = config.value("model", std::string());
    if (model == "antTunnels") {
        model_ = AntTunnels;
    } else if (model == "slimeMold") {
        model_ = SlimeMold;
    } else if (model == "physarum" || model == "physarumParticles") {
        model_ = Physarum;
    }

    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        paramEnabled_ = def.value("visible", paramEnabled_);
        const bool legacyAlgorithmMode = model.empty() && def.contains("mode");
        if (model.empty()) {
            const int legacyMode = static_cast<int>(std::round(def.value("mode", paramMode_)));
            if (legacyMode == 0) {
                model_ = AntTunnels;
            } else if (legacyMode == 1) {
                model_ = SlimeMold;
            } else {
                model_ = Physarum;
            }
        }
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramSeed_ = def.value("seed", paramSeed_);
        paramReseedRequested_ = def.value("reseed", paramReseedRequested_);
        paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
        paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
        paramMode_ = def.value("mode", paramMode_);
        if (legacyAlgorithmMode) {
            paramMode_ = 0.0f;
        }
        paramAgentCount_ = def.value("agentCount", paramAgentCount_);
        paramStepSize_ = def.value("stepSize", paramStepSize_);
        paramTurnRate_ = def.value("turnRate", paramTurnRate_);
        paramSensorAngle_ = def.value("sensorAngle", paramSensorAngle_);
        paramSensorDistance_ = def.value("sensorDistance", paramSensorDistance_);
        paramDeposit_ = def.value("deposit", paramDeposit_);
        paramDecay_ = def.value("decay", paramDecay_);
        paramDiffuse_ = def.value("diffuse", paramDiffuse_);
        paramTrailBoost_ = def.value("trailBoost", paramTrailBoost_);
        paramBackgroundAlpha_ = def.value("backgroundAlpha", paramBackgroundAlpha_);
        paramTrailAlpha_ = def.value("trailAlpha", paramTrailAlpha_);
        paramResetCoverage_ = def.value("resetCoverage", paramResetCoverage_);
        if (def.contains("backgroundColor") && def["backgroundColor"].is_array() && def["backgroundColor"].size() >= 3) {
            paramBgR_ = def["backgroundColor"][0].get<float>();
            paramBgG_ = def["backgroundColor"][1].get<float>();
            paramBgB_ = def["backgroundColor"][2].get<float>();
        }
        if (def.contains("trailColor") && def["trailColor"].is_array() && def["trailColor"].size() >= 3) {
            paramTrailR_ = def["trailColor"][0].get<float>();
            paramTrailG_ = def["trailColor"][1].get<float>();
            paramTrailB_ = def["trailColor"][2].get<float>();
        }
        paramBgR_ = def.value("bgR", paramBgR_);
        paramBgG_ = def.value("bgG", paramBgG_);
        paramBgB_ = def.value("bgB", paramBgB_);
        paramTrailR_ = def.value("trailR", paramTrailR_);
        paramTrailG_ = def.value("trailG", paramTrailG_);
        paramTrailB_ = def.value("trailB", paramTrailB_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(32, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(32, config["textureSize"][1].get<int>());
    }
}

void AgentFieldLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.agentField" : registryPrefix();

    LayerParameterBuilder common(registry, prefix, "Generative");
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_,
                     common.boolDescriptor({ "Action: Visible", {}, {} }));
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_,
                      common.floatDescriptor(
                          { "Time: Field Speed", {}, { 0.0f, 40.0f, 0.1f },
                            {}, {}, true, 10 }));

    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = "Action: BPM Sync";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);

    meta.label = "Time: BPM Mult";
    meta.range.min = 0.25f;
    meta.range.max = 8.0f;
    meta.range.step = 0.25f;
    registry.addFloat(prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, meta);

    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_,
                      common.floatDescriptor(
                          { "Visibility: Field Opacity", {},
                            { 0.0f, 1.0f, 0.01f } }));

    registry.addFloat(
        prefix + ".seed", &paramSeed_, paramSeed_,
        common.floatDescriptor(
            { "Seed: Deterministic Seed", "Trail Lifecycle",
              { 1.0f, 999999.0f, 1.0f }, {},
              "The same model, seed, and parameters reproduce the same initial field" }));

    meta = {};
    meta.group = "Trail Lifecycle";
    meta.label = "Action: Auto Reseed";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, meta);

    meta.label = "Time: Auto Reseed Beats";
    meta.range.min = 1.0f;
    meta.range.max = 64.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, meta);

    registry.addBool(prefix + ".reseed", &paramReseedRequested_,
                     paramReseedRequested_,
                     common.boolDescriptor(
                         { "Action: Reseed", "Trail Lifecycle", {} }));

    meta = {};
    meta.group = "Trail Behavior";
    meta.label = "Action: Behavior Mode";
    meta.range.min = 0.0f;
    meta.range.max = static_cast<float>(kModeCount - 1);
    meta.range.step = 1.0f;
    meta.description = modeDescriptions();
    meta.quickAccess = true;
    meta.quickAccessOrder = 20;
    registry.addFloat(prefix + ".mode", &paramMode_, paramMode_, meta);
    meta.quickAccess = false;
    meta.quickAccessOrder = 0;

    meta.label = "Count: Agents";
    meta.range.min = 4.0f;
    meta.range.max = 256.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".agentCount", &paramAgentCount_, paramAgentCount_, meta);

    meta.label = "Scale: Step Size";
    meta.range.min = 0.1f;
    meta.range.max = 4.0f;
    meta.range.step = 0.05f;
    registry.addFloat(prefix + ".stepSize", &paramStepSize_, paramStepSize_, meta);

    meta.label = "Motion: Turn Rate";
    meta.range.min = 0.01f;
    meta.range.max = 2.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".turnRate", &paramTurnRate_, paramTurnRate_, meta);

    meta.label = "Scale: Sensor Angle";
    meta.range.min = 0.05f;
    meta.range.max = 1.5f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".sensorAngle", &paramSensorAngle_, paramSensorAngle_, meta);

    meta.label = "Scale: Sensor Distance";
    meta.range.min = 0.5f;
    meta.range.max = 8.0f;
    meta.range.step = 0.1f;
    registry.addFloat(prefix + ".sensorDistance", &paramSensorDistance_, paramSensorDistance_, meta);

    meta.label = "Force: Trail Deposit";
    meta.range.min = 0.01f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".deposit", &paramDeposit_, paramDeposit_, meta);

    meta.label = "Time: Trail Decay";
    meta.range.min = 0.0f;
    meta.range.max = 0.2f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".decay", &paramDecay_, paramDecay_, meta);

    meta.label = "Motion: Trail Diffusion";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".diffuse", &paramDiffuse_, paramDiffuse_, meta);

    meta.label = "Glow: Trail Boost";
    meta.range.min = 0.1f;
    meta.range.max = 4.0f;
    meta.range.step = 0.05f;
    registry.addFloat(prefix + ".trailBoost", &paramTrailBoost_, paramTrailBoost_, meta);

    meta.label = "Scale: Reset Coverage";
    meta.range.min = 0.05f;
    meta.range.max = 0.98f;
    meta.range.step = 0.01f;
    meta.description = "Reset once this fraction of the field has grown in";
    registry.addFloat(prefix + ".resetCoverage", &paramResetCoverage_, paramResetCoverage_, meta);

    meta.label = "Visibility: Background Opacity";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".backgroundAlpha", &paramBackgroundAlpha_, paramBackgroundAlpha_, meta);

    meta.label = "Visibility: Trail Opacity";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, meta);

    meta = {};
    meta.group = "Trail Color";
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

    allocateField();
    resetAgents();
    syncTexture();
}

void AgentFieldLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    paramMode_ = std::round(ofClamp(paramMode_, 0.0f, static_cast<float>(kModeCount - 1)));
    paramAgentCount_ = std::round(ofClamp(paramAgentCount_, 4.0f, 256.0f));
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 8.0f);
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 1.0f, 999999.0f));
    paramStepSize_ = ofClamp(paramStepSize_, 0.1f, 4.0f);
    paramTurnRate_ = ofClamp(paramTurnRate_, 0.01f, 2.0f);
    paramSensorAngle_ = ofClamp(paramSensorAngle_, 0.05f, 1.5f);
    paramSensorDistance_ = ofClamp(paramSensorDistance_, 0.5f, 8.0f);
    paramDeposit_ = ofClamp(paramDeposit_, 0.01f, 1.0f);
    paramDecay_ = ofClamp(paramDecay_, 0.0f, 0.2f);
    paramDiffuse_ = ofClamp(paramDiffuse_, 0.0f, 1.0f);
    paramTrailBoost_ = ofClamp(paramTrailBoost_, 0.1f, 4.0f);
    paramBackgroundAlpha_ = ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramResetCoverage_ = ofClamp(paramResetCoverage_, 0.05f, 0.98f);
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 64.0f));

    const float beatPosition = currentBeatPosition(params.time, params.bpm);

    if (static_cast<int>(agents_.size()) != static_cast<int>(paramAgentCount_) ||
        requestedSeed() != appliedSeed_ ||
        paramReseedRequested_) {
        triggerReset();
        paramReseedRequested_ = false;
    }

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
        diffuseAndDecay();
        stepAgents(static_cast<float>(i) / static_cast<float>(std::max(1, iterations)));
    }

    if (fieldCoverage(0.18f) >= paramResetCoverage_) {
        triggerReset();
    }

    syncTexture();
    dirty_ = false;
}

void AgentFieldLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || !texture_.isAllocated() || params.slotOpacity <= 0.0f) return;

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    ofSetColor(255, 255, 255, static_cast<int>(ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f) * 255.0f));
    texture_.draw(0, 0, params.viewport.x, params.viewport.y);
    ofPopView();
    ofPopStyle();
}

void AgentFieldLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void AgentFieldLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void AgentFieldLayer::allocateField() {
    const std::size_t count = static_cast<std::size_t>(textureSize_.x * textureSize_.y);
    field_.assign(count, 0.0f);
    scratch_.assign(count, 0.0f);
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
}

void AgentFieldLayer::resetAgents() {
    if (field_.empty()) {
        allocateField();
    }

    std::fill(field_.begin(), field_.end(), 0.0f);
    std::fill(scratch_.begin(), scratch_.end(), 0.0f);
    agents_.assign(static_cast<std::size_t>(std::round(paramAgentCount_)), {});
    appliedSeed_ = requestedSeed();
    rng_.seed(appliedSeed_);

    for (auto& agent : agents_) {
        if (model_ == Physarum) {
            agent.x = randomRange(textureSize_.x * 0.25f, textureSize_.x * 0.75f);
            agent.y = randomRange(textureSize_.y * 0.25f, textureSize_.y * 0.75f);
        } else {
            agent.x = randomRange(4.0f, std::max(5.0f, static_cast<float>(textureSize_.x - 4)));
            agent.y = randomRange(4.0f, std::max(5.0f, static_cast<float>(textureSize_.y - 4)));
        }
        agent.angle = randomRange(0.0f, TWO_PI);
        agent.energy = randomRange(0.4f, 1.0f);
        deposit(static_cast<int>(agent.x), static_cast<int>(agent.y), 0.6f);
    }

    nextAutoReseedBeat_ = -1.0f;
    dirty_ = true;
}

void AgentFieldLayer::diffuseAndDecay() {
    const float decay = paramDecay_;
    const float diffuse = paramDiffuse_;
    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const int idx = indexFor(x, y);
            const float center = field_[idx];
            const float left = sample(static_cast<float>(x - 1), static_cast<float>(y));
            const float right = sample(static_cast<float>(x + 1), static_cast<float>(y));
            const float up = sample(static_cast<float>(x), static_cast<float>(y - 1));
            const float down = sample(static_cast<float>(x), static_cast<float>(y + 1));
            const float blur = (center * 4.0f + left + right + up + down) / 8.0f;
            scratch_[idx] = ofClamp(ofLerp(center, blur, diffuse) - decay, 0.0f, 1.0f);
        }
    }
    field_.swap(scratch_);
}

void AgentFieldLayer::stepAgents(float amount) {
    const float jitterScale = 0.7f + amount * 0.3f;
    for (auto& agent : agents_) {
        switch (model_) {
        case AntTunnels:
            stepAnt(agent, jitterScale);
            break;
        case SlimeMold:
            stepSlime(agent, jitterScale);
            break;
        case Physarum:
        default:
            stepPhysarum(agent, jitterScale);
            break;
        }
    }
}

void AgentFieldLayer::stepAnt(Agent& agent, float jitterScale) {
    const int mode = behaviorMode();
    const float explore = mode == Explore ? 1.0f : (mode == Exploit ? 0.15f : 0.45f);
    const float exploit = mode == Exploit ? 1.0f : (mode == Explore ? 0.15f : 0.55f);
    const float sensorDist = std::max(1.0f, paramSensorDistance_);
    const float forward = sample(agent.x + std::cos(agent.angle) * sensorDist,
                                 agent.y + std::sin(agent.angle) * sensorDist);
    const float left = sample(agent.x + std::cos(agent.angle - paramSensorAngle_) * sensorDist,
                              agent.y + std::sin(agent.angle - paramSensorAngle_) * sensorDist);
    const float right = sample(agent.x + std::cos(agent.angle + paramSensorAngle_) * sensorDist,
                               agent.y + std::sin(agent.angle + paramSensorAngle_) * sensorDist);

    if (exploit > 0.1f) {
        if (left > forward && left > right) {
            agent.angle -= paramTurnRate_ * exploit;
        } else if (right > forward && right > left) {
            agent.angle += paramTurnRate_ * exploit;
        }
    }
    agent.angle += randomRange(-1.0f, 1.0f) * paramTurnRate_ * (0.35f + jitterScale * explore);
    agent.x += std::cos(agent.angle) * paramStepSize_;
    agent.y += std::sin(agent.angle) * paramStepSize_;

    if (agent.x < 1.0f || agent.x >= textureSize_.x - 1.0f || agent.y < 1.0f || agent.y >= textureSize_.y - 1.0f) {
        agent.angle += PI + randomRange(-0.4f, 0.4f);
        agent.x = ofClamp(agent.x, 1.0f, static_cast<float>(textureSize_.x - 2));
        agent.y = ofClamp(agent.y, 1.0f, static_cast<float>(textureSize_.y - 2));
    }

    deposit(static_cast<int>(agent.x), static_cast<int>(agent.y), paramDeposit_ * (1.2f + exploit));
    if (randomUnit() < 0.18f + explore * 0.18f) {
        const float side = agent.angle + (randomUnit() < 0.5f ? HALF_PI : -HALF_PI);
        deposit(static_cast<int>(agent.x + std::cos(side)),
                static_cast<int>(agent.y + std::sin(side)),
                paramDeposit_ * 0.9f);
    }
}

void AgentFieldLayer::stepSlime(Agent& agent, float jitterScale) {
    const int mode = behaviorMode();
    const float explore = mode == Explore ? 1.0f : (mode == Exploit ? 0.2f : 0.55f);
    const float exploit = mode == Exploit ? 1.0f : (mode == Explore ? 0.15f : 0.45f);
    const float sensorDist = std::max(1.0f, paramSensorDistance_);
    const float left = sample(agent.x + std::cos(agent.angle - paramSensorAngle_) * sensorDist,
                              agent.y + std::sin(agent.angle - paramSensorAngle_) * sensorDist);
    const float right = sample(agent.x + std::cos(agent.angle + paramSensorAngle_) * sensorDist,
                               agent.y + std::sin(agent.angle + paramSensorAngle_) * sensorDist);
    if (exploit > 0.1f && std::abs(left - right) > 0.001f) {
        agent.angle += (right > left ? 1.0f : -1.0f) * paramTurnRate_ * exploit;
    }
    if (randomUnit() < 0.18f + explore * 0.2f + jitterScale * 0.08f) {
        agent.angle += randomRange(-1.0f, 1.0f) * paramTurnRate_;
    }
    agent.x = wrapCoord(agent.x + std::cos(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.x));
    agent.y = wrapCoord(agent.y + std::sin(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.y));

    agent.energy = std::max(0.1f, agent.energy - 0.015f);
    if (agent.energy < 0.2f || randomUnit() < 0.02f) {
        agent.energy = randomRange(0.5f, 1.0f);
        agent.angle += randomRange(-PI, PI);
    }

    deposit(static_cast<int>(agent.x), static_cast<int>(agent.y), paramDeposit_ * (1.0f + agent.energy + exploit * 0.35f));
    if (randomUnit() < 0.22f + explore * 0.25f) {
        const float branch = agent.angle + (randomUnit() < 0.5f ? paramSensorAngle_ : -paramSensorAngle_);
        deposit(static_cast<int>(agent.x + std::cos(branch)),
                static_cast<int>(agent.y + std::sin(branch)),
                paramDeposit_ * 0.7f);
    }
}

void AgentFieldLayer::stepPhysarum(Agent& agent, float jitterScale) {
    const int mode = behaviorMode();
    const float explore = mode == Explore ? 1.0f : (mode == Exploit ? 0.1f : 0.35f);
    const float exploit = mode == Exploit ? 1.0f : (mode == Explore ? 0.35f : 0.7f);
    const float sensorDist = paramSensorDistance_;
    const float sensorAngle = paramSensorAngle_;
    const float forward = sample(agent.x + std::cos(agent.angle) * sensorDist,
                                 agent.y + std::sin(agent.angle) * sensorDist);
    const float left = sample(agent.x + std::cos(agent.angle - sensorAngle) * sensorDist,
                              agent.y + std::sin(agent.angle - sensorAngle) * sensorDist);
    const float right = sample(agent.x + std::cos(agent.angle + sensorAngle) * sensorDist,
                               agent.y + std::sin(agent.angle + sensorAngle) * sensorDist);

    if (left > forward && left > right) {
        agent.angle -= paramTurnRate_ * exploit;
    } else if (right > forward && right > left) {
        agent.angle += paramTurnRate_ * exploit;
    } else {
        agent.angle += randomRange(-1.0f, 1.0f) * paramTurnRate_ * (0.25f + explore * 0.35f) * jitterScale;
    }

    agent.x = wrapCoord(agent.x + std::cos(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.x));
    agent.y = wrapCoord(agent.y + std::sin(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.y));
    deposit(static_cast<int>(agent.x), static_cast<int>(agent.y), paramDeposit_ * (1.1f + exploit * 0.45f));
}

void AgentFieldLayer::deposit(int x, int y, float amount) {
    if (x < 0 || x >= textureSize_.x || y < 0 || y >= textureSize_.y) return;
    const std::size_t idx = static_cast<std::size_t>(indexFor(x, y));
    field_[idx] = ofClamp(field_[idx] + amount, 0.0f, 1.0f);
}

float AgentFieldLayer::sample(float x, float y) const {
    int ix = static_cast<int>(std::floor(wrapCoord(x, static_cast<float>(textureSize_.x))));
    int iy = static_cast<int>(std::floor(wrapCoord(y, static_cast<float>(textureSize_.y))));
    return field_[static_cast<std::size_t>(indexFor(ix, iy))];
}

void AgentFieldLayer::syncTexture() {
    if (!pixels_.isAllocated()) return;

    const ofFloatColor bg(ofClamp(paramBgR_, 0.0f, 1.0f),
                          ofClamp(paramBgG_, 0.0f, 1.0f),
                          ofClamp(paramBgB_, 0.0f, 1.0f),
                          ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f));
    const ofFloatColor trail(ofClamp(paramTrailR_, 0.0f, 1.0f),
                             ofClamp(paramTrailG_, 0.0f, 1.0f),
                             ofClamp(paramTrailB_, 0.0f, 1.0f),
                             ofClamp(paramTrailAlpha_, 0.0f, 1.0f));

    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            float value = ofClamp(field_[static_cast<std::size_t>(indexFor(x, y))] * paramTrailBoost_, 0.0f, 1.0f);
            pixels_.setColor(x, y, ofFloatColor(ofLerp(bg.r, trail.r, value),
                                                ofLerp(bg.g, trail.g, value),
                                                ofLerp(bg.b, trail.b, value),
                                                ofLerp(ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f), trail.a, value)));
        }
    }
    texture_.loadData(pixels_);
}

float AgentFieldLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

int AgentFieldLayer::indexFor(int x, int y) const {
    return y * textureSize_.x + x;
}

float AgentFieldLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) return 0.0f;
    return std::max(0.0f, timeSeconds) * (bpm / 60.0f);
}

float AgentFieldLayer::fieldCoverage(float threshold) const {
    if (field_.empty()) return 0.0f;
    std::size_t filled = 0;
    for (float value : field_) {
        if (value >= threshold) {
            ++filled;
        }
    }
    return static_cast<float>(filled) / static_cast<float>(field_.size());
}

int AgentFieldLayer::behaviorMode() const {
    return static_cast<int>(std::round(ofClamp(paramMode_, 0.0f, static_cast<float>(kModeCount - 1))));
}

void AgentFieldLayer::triggerReset() {
    resetAgents();
}

float AgentFieldLayer::randomUnit() {
    return std::generate_canonical<float, 24>(rng_);
}

float AgentFieldLayer::randomRange(float minimum, float maximum) {
    return ofLerp(minimum, maximum, randomUnit());
}

std::uint32_t AgentFieldLayer::requestedSeed() const {
    return static_cast<std::uint32_t>(
        std::round(ofClamp(paramSeed_, 1.0f, 999999.0f)));
}

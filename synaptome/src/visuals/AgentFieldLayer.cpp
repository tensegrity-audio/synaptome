#include "AgentFieldLayer.h"
#include "ofGraphics.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr int kModeCount = 3;
    const char* kModeLabels[kModeCount] = {
        "Ant Tunnels",
        "Slime Mold",
        "Physarum"
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
    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
        paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
        paramMode_ = def.value("mode", paramMode_);
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
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        textureSize_.x = std::max(32, config["textureSize"][0].get<int>());
        textureSize_.y = std::max(32, config["textureSize"][1].get<int>());
    }
}

void AgentFieldLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "layer.agentField" : registryPrefix();

    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = "Field Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    meta.label = "Field Speed";
    meta.range.min = 0.0f;
    meta.range.max = 40.0f;
    meta.range.step = 0.1f;
    registry.addFloat(prefix + ".speed", &paramSpeed_, paramSpeed_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Field BPM Sync";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);

    meta.label = "Field BPM Mult";
    meta.range.min = 0.25f;
    meta.range.max = 8.0f;
    meta.range.step = 0.25f;
    registry.addFloat(prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Field Alpha";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".alpha", &paramAlpha_, paramAlpha_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Field Auto Reseed";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, meta);

    meta.label = "Field Auto Reseed Beats";
    meta.range.min = 1.0f;
    meta.range.max = 64.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Field Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    meta = {};
    meta.group = "Generative";
    meta.label = "Field Mode";
    meta.range.min = 0.0f;
    meta.range.max = static_cast<float>(kModeCount - 1);
    meta.range.step = 1.0f;
    meta.description = modeDescriptions();
    registry.addFloat(prefix + ".mode", &paramMode_, paramMode_, meta);

    meta.label = "Field Agents";
    meta.range.min = 4.0f;
    meta.range.max = 256.0f;
    meta.range.step = 1.0f;
    registry.addFloat(prefix + ".agentCount", &paramAgentCount_, paramAgentCount_, meta);

    meta.label = "Field Step Size";
    meta.range.min = 0.1f;
    meta.range.max = 4.0f;
    meta.range.step = 0.05f;
    registry.addFloat(prefix + ".stepSize", &paramStepSize_, paramStepSize_, meta);

    meta.label = "Field Turn Rate";
    meta.range.min = 0.01f;
    meta.range.max = 2.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".turnRate", &paramTurnRate_, paramTurnRate_, meta);

    meta.label = "Field Sensor Angle";
    meta.range.min = 0.05f;
    meta.range.max = 1.5f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".sensorAngle", &paramSensorAngle_, paramSensorAngle_, meta);

    meta.label = "Field Sensor Dist";
    meta.range.min = 0.5f;
    meta.range.max = 8.0f;
    meta.range.step = 0.1f;
    registry.addFloat(prefix + ".sensorDistance", &paramSensorDistance_, paramSensorDistance_, meta);

    meta.label = "Field Deposit";
    meta.range.min = 0.01f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".deposit", &paramDeposit_, paramDeposit_, meta);

    meta.label = "Field Decay";
    meta.range.min = 0.0f;
    meta.range.max = 0.2f;
    meta.range.step = 0.001f;
    registry.addFloat(prefix + ".decay", &paramDecay_, paramDecay_, meta);

    meta.label = "Field Diffuse";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".diffuse", &paramDiffuse_, paramDiffuse_, meta);

    meta.label = "Field Trail Boost";
    meta.range.min = 0.1f;
    meta.range.max = 4.0f;
    meta.range.step = 0.05f;
    registry.addFloat(prefix + ".trailBoost", &paramTrailBoost_, paramTrailBoost_, meta);

    meta.label = "Field Reset Coverage";
    meta.range.min = 0.05f;
    meta.range.max = 0.98f;
    meta.range.step = 0.01f;
    meta.description = "Reset once this fraction of the field has grown in";
    registry.addFloat(prefix + ".resetCoverage", &paramResetCoverage_, paramResetCoverage_, meta);

    meta.label = "Field Bg Alpha";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".backgroundAlpha", &paramBackgroundAlpha_, paramBackgroundAlpha_, meta);

    meta.label = "Field Trail Alpha";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    registry.addFloat(prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, meta);

    meta = {};
    meta.group = "Generative";
    meta.range.min = 0.0f;
    meta.range.max = 1.0f;
    meta.range.step = 0.01f;
    meta.label = "Field Bg R";
    registry.addFloat(prefix + ".bgR", &paramBgR_, paramBgR_, meta);
    meta.label = "Field Bg G";
    registry.addFloat(prefix + ".bgG", &paramBgG_, paramBgG_, meta);
    meta.label = "Field Bg B";
    registry.addFloat(prefix + ".bgB", &paramBgB_, paramBgB_, meta);
    meta.label = "Field Trail R";
    registry.addFloat(prefix + ".trailR", &paramTrailR_, paramTrailR_, meta);
    meta.label = "Field Trail G";
    registry.addFloat(prefix + ".trailG", &paramTrailG_, paramTrailG_, meta);
    meta.label = "Field Trail B";
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

    if (static_cast<int>(agents_.size()) != static_cast<int>(paramAgentCount_) || paramReseedRequested_) {
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

    const Mode mode = static_cast<Mode>(static_cast<int>(paramMode_));
    for (auto& agent : agents_) {
        if (mode == Physarum) {
            agent.x = ofRandom(textureSize_.x * 0.25f, textureSize_.x * 0.75f);
            agent.y = ofRandom(textureSize_.y * 0.25f, textureSize_.y * 0.75f);
        } else {
            agent.x = ofRandom(4.0f, std::max(5.0f, static_cast<float>(textureSize_.x - 4)));
            agent.y = ofRandom(4.0f, std::max(5.0f, static_cast<float>(textureSize_.y - 4)));
        }
        agent.angle = ofRandom(TWO_PI);
        agent.energy = ofRandom(0.4f, 1.0f);
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
    const Mode mode = static_cast<Mode>(static_cast<int>(paramMode_));
    for (auto& agent : agents_) {
        switch (mode) {
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
    agent.angle += ofRandom(-1.0f, 1.0f) * paramTurnRate_ * (0.5f + jitterScale);
    agent.x += std::cos(agent.angle) * paramStepSize_;
    agent.y += std::sin(agent.angle) * paramStepSize_;

    if (agent.x < 1.0f || agent.x >= textureSize_.x - 1.0f || agent.y < 1.0f || agent.y >= textureSize_.y - 1.0f) {
        agent.angle += PI + ofRandom(-0.4f, 0.4f);
        agent.x = ofClamp(agent.x, 1.0f, static_cast<float>(textureSize_.x - 2));
        agent.y = ofClamp(agent.y, 1.0f, static_cast<float>(textureSize_.y - 2));
    }

    deposit(static_cast<int>(agent.x), static_cast<int>(agent.y), paramDeposit_ * 1.8f);
    if (ofRandomuf() < 0.25f) {
        const float side = agent.angle + (ofRandomuf() < 0.5f ? HALF_PI : -HALF_PI);
        deposit(static_cast<int>(agent.x + std::cos(side)),
                static_cast<int>(agent.y + std::sin(side)),
                paramDeposit_ * 0.9f);
    }
}

void AgentFieldLayer::stepSlime(Agent& agent, float jitterScale) {
    if (ofRandomuf() < 0.25f + jitterScale * 0.1f) {
        agent.angle += ofRandom(-1.0f, 1.0f) * paramTurnRate_;
    }
    agent.x = wrapCoord(agent.x + std::cos(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.x));
    agent.y = wrapCoord(agent.y + std::sin(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.y));

    agent.energy = std::max(0.1f, agent.energy - 0.015f);
    if (agent.energy < 0.2f || ofRandomuf() < 0.02f) {
        agent.energy = ofRandom(0.5f, 1.0f);
        agent.angle += ofRandom(-PI, PI);
    }

    deposit(static_cast<int>(agent.x), static_cast<int>(agent.y), paramDeposit_ * (1.0f + agent.energy));
    if (ofRandomuf() < 0.35f) {
        const float branch = agent.angle + (ofRandomuf() < 0.5f ? paramSensorAngle_ : -paramSensorAngle_);
        deposit(static_cast<int>(agent.x + std::cos(branch)),
                static_cast<int>(agent.y + std::sin(branch)),
                paramDeposit_ * 0.7f);
    }
}

void AgentFieldLayer::stepPhysarum(Agent& agent, float jitterScale) {
    const float sensorDist = paramSensorDistance_;
    const float sensorAngle = paramSensorAngle_;
    const float forward = sample(agent.x + std::cos(agent.angle) * sensorDist,
                                 agent.y + std::sin(agent.angle) * sensorDist);
    const float left = sample(agent.x + std::cos(agent.angle - sensorAngle) * sensorDist,
                              agent.y + std::sin(agent.angle - sensorAngle) * sensorDist);
    const float right = sample(agent.x + std::cos(agent.angle + sensorAngle) * sensorDist,
                               agent.y + std::sin(agent.angle + sensorAngle) * sensorDist);

    if (left > forward && left > right) {
        agent.angle -= paramTurnRate_;
    } else if (right > forward && right > left) {
        agent.angle += paramTurnRate_;
    } else {
        agent.angle += ofRandom(-1.0f, 1.0f) * paramTurnRate_ * 0.4f * jitterScale;
    }

    agent.x = wrapCoord(agent.x + std::cos(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.x));
    agent.y = wrapCoord(agent.y + std::sin(agent.angle) * paramStepSize_, static_cast<float>(textureSize_.y));
    deposit(static_cast<int>(agent.x), static_cast<int>(agent.y), paramDeposit_ * 1.4f);
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

void AgentFieldLayer::triggerReset() {
    resetAgents();
}

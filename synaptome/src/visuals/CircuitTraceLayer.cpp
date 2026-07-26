#include "CircuitTraceLayer.h"
#include "LayerParameterBuilder.h"
#include "ofGraphics.h"
#include "ofUtils.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
constexpr int kBehaviorCount = 3;
// Circuit layers intentionally scale a small field with hard pixel edges. Keep
// the numeric constant local because the BrowserFlow GL stub does not expose
// the full OpenGL filter constant set.
constexpr int kNearestTextureFilter = 0x2600;
constexpr int kMyceliumSeedColumns = 4;
constexpr int kMyceliumSeedRows = 3;
constexpr const char* kBehaviorDescription =
    "0=Balanced  1=Explore  2=Exploit";

float clampDefault(const ofJson& defaults, const char* key, float fallback,
                   float minimum, float maximum) {
    return ofClamp(defaults.value(key, fallback), minimum, maximum);
}

void readColor(const ofJson& defaults, const char* key,
               float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() ||
        defaults[key].size() < 3) {
        return;
    }
    r = ofClamp(defaults[key][0].get<float>(), 0.0f, 1.0f);
    g = ofClamp(defaults[key][1].get<float>(), 0.0f, 1.0f);
    b = ofClamp(defaults[key][2].get<float>(), 0.0f, 1.0f);
}

std::uint64_t fnvMix(std::uint64_t hash, std::uint64_t value) {
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    hash ^= value;
    return hash * kPrime;
}
}

CircuitTraceLayer::CircuitTraceLayer(Model model)
    : model_(model) {
    applyModelDefaults(model_);
}

CircuitTraceLayer::Model CircuitTraceLayer::modelFromId(
    const std::string& id, Model fallback) {
    if (id == "circuitSlime" || id == "slime" || id == "electronicSlime") {
        return Model::CircuitSlime;
    }
    if (id == "circuitMycelium" || id == "mycelium") {
        return Model::CircuitMycelium;
    }
    if (id == "circuitRiver" || id == "river") {
        return Model::CircuitRiver;
    }
    if (id == "circuitAntTunnels" || id == "antTunnels" || id == "circuitAnts") {
        return Model::CircuitAntTunnels;
    }
    if (id == "circuitFlowField" || id == "flowField" || id == "circuitFlow") {
        return Model::CircuitFlowField;
    }
    return fallback;
}

const char* CircuitTraceLayer::modelId(Model model) {
    switch (model) {
    case Model::CircuitMycelium:
        return "circuitMycelium";
    case Model::CircuitRiver:
        return "circuitRiver";
    case Model::CircuitAntTunnels:
        return "circuitAntTunnels";
    case Model::CircuitFlowField:
        return "circuitFlowField";
    case Model::CircuitSlime:
    default:
        return "circuitSlime";
    }
}

void CircuitTraceLayer::applyModelDefaults(Model model) {
    model_ = model;
    if (model == Model::CircuitMycelium) {
        paramSpeed_ = 7.0f;
        paramSeed_ = 7331.0f;
        paramBehavior_ = static_cast<float>(Explore);
        paramAgentCount_ = 38.0f;
        paramSensorDistance_ = 6.0f;
        paramTurnChance_ = 0.36f;
        paramBranchChance_ = 0.16f;
        paramDeposit_ = 0.30f;
        paramDecay_ = 0.004f;
        paramDiffuse_ = 0.07f;
        paramTracePersistence_ = 0.998f;
        paramTraceWidth_ = 1.0f;
        paramGlow_ = 0.8f;
        paramViaChance_ = 0.045f;
        paramTraceR_ = 1.0f;
        paramTraceG_ = 0.52f;
        paramTraceB_ = 0.12f;
    } else if (model == Model::CircuitRiver) {
        paramSpeed_ = 12.0f;
        paramSeed_ = 2718.0f;
        paramBehavior_ = static_cast<float>(Exploit);
        paramAgentCount_ = 28.0f;
        paramStepSize_ = 2.0f;
        paramSensorDistance_ = 8.0f;
        paramTurnChance_ = 0.16f;
        paramBranchChance_ = 0.025f;
        paramDeposit_ = 0.38f;
        paramDecay_ = 0.012f;
        paramDiffuse_ = 0.18f;
        paramTracePersistence_ = 0.991f;
        paramTraceWidth_ = 2.0f;
        paramGlow_ = 1.8f;
        paramViaChance_ = 0.012f;
        paramTraceR_ = 0.15f;
        paramTraceG_ = 0.72f;
        paramTraceB_ = 1.0f;
    } else if (model == Model::CircuitAntTunnels) {
        paramSpeed_ = 14.0f;
        paramSeed_ = 1101.0f;
        paramBehavior_ = static_cast<float>(Balanced);
        paramAgentCount_ = 44.0f;
        paramStepSize_ = 1.0f;
        paramSensorDistance_ = 5.0f;
        paramTurnChance_ = 0.18f;
        paramBranchChance_ = 0.045f;
        paramDeposit_ = 0.27f;
        paramDecay_ = 0.006f;
        paramDiffuse_ = 0.035f;
        paramTracePersistence_ = 0.996f;
        paramTraceWidth_ = 1.0f;
        paramGlow_ = 0.45f;
        paramViaChance_ = 0.0f;
        paramTraceR_ = 0.98f;
        paramTraceG_ = 0.76f;
        paramTraceB_ = 0.18f;
    } else if (model == Model::CircuitFlowField) {
        paramSpeed_ = 16.0f;
        paramSeed_ = 8088.0f;
        paramBehavior_ = static_cast<float>(Balanced);
        paramAgentCount_ = 64.0f;
        paramStepSize_ = 1.0f;
        paramSensorDistance_ = 7.0f;
        paramTurnChance_ = 0.10f;
        paramBranchChance_ = 0.025f;
        paramDeposit_ = 0.22f;
        paramDecay_ = 0.010f;
        paramDiffuse_ = 0.025f;
        paramTracePersistence_ = 0.993f;
        paramTraceWidth_ = 1.0f;
        paramGlow_ = 0.65f;
        paramViaChance_ = 0.0f;
        paramTraceR_ = 0.72f;
        paramTraceG_ = 0.30f;
        paramTraceB_ = 1.0f;
    } else {
        paramSpeed_ = 10.0f;
        paramSeed_ = 4242.0f;
        paramBehavior_ = static_cast<float>(Balanced);
        paramAgentCount_ = 72.0f;
        paramStepSize_ = 1.0f;
        paramSensorDistance_ = 4.0f;
        paramTurnChance_ = 0.28f;
        paramBranchChance_ = 0.07f;
        paramDeposit_ = 0.24f;
        paramDecay_ = 0.008f;
        paramDiffuse_ = 0.12f;
        paramTracePersistence_ = 0.995f;
        paramTraceWidth_ = 1.0f;
        paramGlow_ = 1.2f;
        paramViaChance_ = 0.025f;
        paramTraceR_ = 0.15f;
        paramTraceG_ = 1.0f;
        paramTraceB_ = 0.45f;
    }
}

void CircuitTraceLayer::configure(const ofJson& config) {
    const Model configuredModel =
        modelFromId(config.value("model", std::string()), model_);
    if (configuredModel != model_) {
        applyModelDefaults(configuredModel);
    }

    const char* sizeKey = config.contains("textureSize")
        ? "textureSize"
        : (config.contains("fieldSize") ? "fieldSize" : nullptr);
    if (sizeKey != nullptr && config[sizeKey].is_array() &&
        config[sizeKey].size() >= 2) {
        textureSize_.x = std::max(48, config[sizeKey][0].get<int>());
        textureSize_.y = std::max(48, config[sizeKey][1].get<int>());
    }

    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }
    const auto& defaults = config["defaults"];
    paramVisible_ = defaults.value("visible", paramVisible_);
    paramSpeed_ = clampDefault(defaults, "speed", paramSpeed_, 0.0f, 40.0f);
    paramBpmSync_ = defaults.value("bpmSync", paramBpmSync_);
    paramBpmMultiplier_ =
        clampDefault(defaults, "bpmMultiplier", paramBpmMultiplier_, 0.25f, 8.0f);
    paramAlpha_ = clampDefault(defaults, "alpha", paramAlpha_, 0.0f, 1.0f);
    paramSeed_ = clampDefault(defaults, "seed", paramSeed_, 1.0f, 999999.0f);
    paramAutoReseed_ = defaults.value("autoReseed", paramAutoReseed_);
    paramAutoReseedEveryBeats_ = clampDefault(
        defaults, "autoReseedEveryBeats", paramAutoReseedEveryBeats_, 1.0f, 64.0f);
    paramBehavior_ =
        clampDefault(defaults, "behavior", paramBehavior_, 0.0f, 2.0f);
    // "mode" remains a friendly alias for existing generative configurations.
    paramBehavior_ = clampDefault(defaults, "mode", paramBehavior_, 0.0f, 2.0f);
    paramAgentCount_ =
        clampDefault(defaults, "agentCount", paramAgentCount_, 4.0f, 256.0f);
    paramStepSize_ =
        clampDefault(defaults, "stepSize", paramStepSize_, 1.0f, 4.0f);
    paramSensorDistance_ = clampDefault(
        defaults, "sensorDistance", paramSensorDistance_, 1.0f, 12.0f);
    paramTurnChance_ =
        clampDefault(defaults, "turnChance", paramTurnChance_, 0.0f, 1.0f);
    paramBranchChance_ =
        clampDefault(defaults, "branchChance", paramBranchChance_, 0.0f, 1.0f);
    paramDeposit_ =
        clampDefault(defaults, "deposit", paramDeposit_, 0.01f, 1.0f);
    paramDecay_ = clampDefault(defaults, "decay", paramDecay_, 0.0f, 0.2f);
    paramDiffuse_ =
        clampDefault(defaults, "diffuse", paramDiffuse_, 0.0f, 1.0f);
    paramTracePersistence_ = clampDefault(
        defaults, "tracePersistence", paramTracePersistence_, 0.90f, 1.0f);
    paramTraceWidth_ =
        clampDefault(defaults, "traceWidth", paramTraceWidth_, 1.0f, 4.0f);
    paramGlow_ = clampDefault(defaults, "glow", paramGlow_, 0.0f, 4.0f);
    paramViaChance_ =
        clampDefault(defaults, "viaChance", paramViaChance_, 0.0f, 1.0f);
    paramBackgroundAlpha_ = clampDefault(
        defaults, "backgroundAlpha", paramBackgroundAlpha_, 0.0f, 1.0f);
    paramTrailAlpha_ =
        clampDefault(defaults, "trailAlpha", paramTrailAlpha_, 0.0f, 1.0f);
    readColor(defaults, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(defaults, "traceColor", paramTraceR_, paramTraceG_, paramTraceB_);
    readColor(defaults, "trailColor", paramTraceR_, paramTraceG_, paramTraceB_);
    paramBgR_ = clampDefault(defaults, "bgR", paramBgR_, 0.0f, 1.0f);
    paramBgG_ = clampDefault(defaults, "bgG", paramBgG_, 0.0f, 1.0f);
    paramBgB_ = clampDefault(defaults, "bgB", paramBgB_, 0.0f, 1.0f);
    paramTraceR_ =
        clampDefault(defaults, "traceR", paramTraceR_, 0.0f, 1.0f);
    paramTraceG_ =
        clampDefault(defaults, "traceG", paramTraceG_, 0.0f, 1.0f);
    paramTraceB_ =
        clampDefault(defaults, "traceB", paramTraceB_, 0.0f, 1.0f);
}

void CircuitTraceLayer::setup(ParameterRegistry& registry) {
    const std::string prefix =
        registryPrefix().empty() ? "layer.circuitTrace" : registryPrefix();
    LayerParameterBuilder common(registry, prefix);

    auto addFloat = [&](const char* suffix, float* value, const char* label,
                        const char* group, float minimum, float maximum,
                        float step, const char* description = "",
                        bool quickAccess = false, int quickAccessOrder = 0) {
        ParameterRegistry::Descriptor meta;
        meta.label = label;
        meta.group = group;
        meta.range.min = minimum;
        meta.range.max = maximum;
        meta.range.step = step;
        meta.description = description;
        meta.quickAccess = quickAccess;
        meta.quickAccessOrder = quickAccessOrder;
        registry.addFloat(prefix + "." + suffix, value, *value, meta);
    };
    auto addBool = [&](const char* suffix, bool* value, const char* label,
                       const char* group, const char* description = "") {
        ParameterRegistry::Descriptor meta;
        meta.label = label;
        meta.group = group;
        meta.description = description;
        registry.addBool(prefix + "." + suffix, value, *value, meta);
    };

    common.visible(&paramVisible_, "Action: Visible", "Circuit Trace");
    common.speed(&paramSpeed_, { 0.0f, 40.0f, 0.1f },
                 "Time: Growth Speed", "Circuit Motion", true, 10);
    addBool("bpmSync", &paramBpmSync_, "Action: BPM Sync", "Circuit Motion");
    addFloat("bpmMultiplier", &paramBpmMultiplier_, "Time: BPM Multiplier",
             "Circuit Motion", 0.25f, 8.0f, 0.25f);
    common.alpha(&paramAlpha_, "Visibility: Layer Opacity", "Circuit Trace");

    common.number(
        "seed", &paramSeed_,
        { "Pattern: Deterministic Seed", "Circuit Growth",
          { 1.0f, 999999.0f, 1.0f }, {},
          "The same seed and parameters reproduce the same network" });
    common.boolean(
        "reseed", &paramReseed_,
        { "Action: Rebuild From Seed", "Circuit Growth",
          "Momentary action; rebuilding does not change Seed" });
    addBool("autoReseed", &paramAutoReseed_, "Action: Auto Reseed",
            "Circuit Growth");
    addFloat("autoReseedEveryBeats", &paramAutoReseedEveryBeats_,
             "Time: Auto Reseed Beats", "Circuit Growth",
             1.0f, 64.0f, 1.0f);
    addFloat("behavior", &paramBehavior_, "Choice: Growth Behavior",
             "Circuit Growth", 0.0f, 2.0f, 1.0f,
             kBehaviorDescription, true, 20);
    addFloat("agentCount", &paramAgentCount_, "Count: Growth Tips",
             "Circuit Growth", 4.0f, 256.0f, 1.0f);
    addFloat("stepSize", &paramStepSize_, "Scale: Grid Step",
             "Circuit Motion", 1.0f, 4.0f, 1.0f,
             "Integer pixels along one of eight compass directions");
    addFloat("sensorDistance", &paramSensorDistance_, "Scale: Sensor Reach",
             "Circuit Motion", 1.0f, 12.0f, 1.0f);
    addFloat("turnChance", &paramTurnChance_, "Motion: Corner Chance",
             "Circuit Motion", 0.0f, 1.0f, 0.01f);
    addFloat("branchChance", &paramBranchChance_, "Growth: Branch Chance",
             "Circuit Growth", 0.0f, 1.0f, 0.01f);
    addFloat("deposit", &paramDeposit_, "Growth: Signal Deposit",
             "Circuit Growth", 0.01f, 1.0f, 0.01f);
    addFloat("decay", &paramDecay_, "Time: Signal Decay",
             "Circuit Growth", 0.0f, 0.2f, 0.001f);
    addFloat("diffuse", &paramDiffuse_, "Growth: Signal Diffusion",
             "Circuit Growth", 0.0f, 1.0f, 0.01f);
    addFloat("tracePersistence", &paramTracePersistence_,
             "Time: Copper Persistence", "Circuit Appearance",
             0.90f, 1.0f, 0.001f);
    addFloat("traceWidth", &paramTraceWidth_, "Scale: Trace Width",
             "Circuit Appearance", 1.0f, 4.0f, 1.0f);
    addFloat("glow", &paramGlow_, "Glow: Signal Halo",
             "Circuit Appearance", 0.0f, 4.0f, 0.05f);
    addFloat("viaChance", &paramViaChance_, "Growth: Via Chance",
             "Circuit Appearance", 0.0f, 1.0f, 0.005f);
    addFloat("backgroundAlpha", &paramBackgroundAlpha_,
             "Visibility: Background Opacity", "Circuit Appearance",
             0.0f, 1.0f, 0.01f);
    addFloat("trailAlpha", &paramTrailAlpha_, "Visibility: Trace Opacity",
             "Circuit Appearance", 0.0f, 1.0f, 0.01f);

    addFloat("bgR", &paramBgR_, "Color: Background R",
             "Circuit Color", 0.0f, 1.0f, 0.01f);
    addFloat("bgG", &paramBgG_, "Color: Background G",
             "Circuit Color", 0.0f, 1.0f, 0.01f);
    addFloat("bgB", &paramBgB_, "Color: Background B",
             "Circuit Color", 0.0f, 1.0f, 0.01f);
    addFloat("traceR", &paramTraceR_, "Color: Trace R",
             "Circuit Color", 0.0f, 1.0f, 0.01f);
    addFloat("traceG", &paramTraceG_, "Color: Trace G",
             "Circuit Color", 0.0f, 1.0f, 0.01f);
    addFloat("traceB", &paramTraceB_, "Color: Trace B",
             "Circuit Color", 0.0f, 1.0f, 0.01f);

    allocateField();
    resetSimulation();
    syncTexture();
}

void CircuitTraceLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramVisible_;
    if (!enabled_) {
        return;
    }

    paramBehavior_ = std::round(ofClamp(
        paramBehavior_, 0.0f, static_cast<float>(kBehaviorCount - 1)));
    paramAgentCount_ =
        std::round(ofClamp(paramAgentCount_, 4.0f, 256.0f));
    paramStepSize_ = std::round(ofClamp(paramStepSize_, 1.0f, 4.0f));
    paramSensorDistance_ =
        std::round(ofClamp(paramSensorDistance_, 1.0f, 12.0f));
    paramSeed_ = std::round(ofClamp(paramSeed_, 1.0f, 999999.0f));
    paramAutoReseedEveryBeats_ =
        std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 64.0f));

    const bool shapeChanged =
        requestedSeed() != appliedSeed_ ||
        static_cast<int>(paramAgentCount_) != appliedAgentCount_;
    if (paramReseed_ || shapeChanged) {
        resetSimulation();
        paramReseed_ = false;
    }

    const float beatPosition = currentBeatPosition(params.time, params.bpm);
    if (paramAutoReseed_ && params.bpm > 0.0f) {
        const float interval = std::max(1.0f, paramAutoReseedEveryBeats_);
        if (nextAutoReseedBeat_ < 0.0f) {
            nextAutoReseedBeat_ =
                std::floor(beatPosition / interval) * interval + interval;
        }
        while (beatPosition >= nextAutoReseedBeat_) {
            resetSimulation();
            nextAutoReseedBeat_ += interval;
        }
    } else {
        nextAutoReseedBeat_ = -1.0f;
    }

    const float rate = stepRateFor(params);
    stepAccumulator_ += params.dt * std::max(0.0f, rate);
    const int iterations =
        std::min(24, static_cast<int>(std::floor(stepAccumulator_)));
    if (iterations > 0) {
        stepAccumulator_ -= static_cast<float>(iterations);
        for (int i = 0; i < iterations; ++i) {
            diffuseAndDecay();
            stepAgents();
        }
        syncTexture();
        dirty_ = false;
    } else if (dirty_) {
        syncTexture();
        dirty_ = false;
    }
}

void CircuitTraceLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || !texture_.isAllocated() ||
        params.slotOpacity <= 0.0f || params.viewport.x <= 0 ||
        params.viewport.y <= 0) {
        return;
    }

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    const float opacity =
        ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    ofSetColor(255, 255, 255, static_cast<int>(opacity * 255.0f));
    texture_.draw(0, 0, params.viewport.x, params.viewport.y);
    ofPopView();
    ofPopStyle();
}

void CircuitTraceLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void CircuitTraceLayer::setExternalEnabled(bool enabled) {
    paramVisible_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

void CircuitTraceLayer::allocateField() {
    const std::size_t count =
        static_cast<std::size_t>(textureSize_.x * textureSize_.y);
    chemical_.assign(count, 0.0f);
    scratch_.assign(count, 0.0f);
    trace_.assign(count, 0.0f);
    vias_.assign(count, 0);
    pixels_.allocate(textureSize_.x, textureSize_.y, 4);
    texture_.allocate(textureSize_.x, textureSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(
        kNearestTextureFilter, kNearestTextureFilter);
}

void CircuitTraceLayer::resetSimulation() {
    if (chemical_.empty()) {
        return;
    }
    std::fill(chemical_.begin(), chemical_.end(), 0.0f);
    std::fill(scratch_.begin(), scratch_.end(), 0.0f);
    std::fill(trace_.begin(), trace_.end(), 0.0f);
    std::fill(vias_.begin(), vias_.end(), 0);

    appliedSeed_ = requestedSeed();
    rng_.seed(appliedSeed_);
    appliedAgentCount_ =
        static_cast<int>(std::round(ofClamp(paramAgentCount_, 4.0f, 256.0f)));
    agents_.assign(static_cast<std::size_t>(appliedAgentCount_), {});

    const int margin = 6;
    for (std::size_t i = 0; i < agents_.size(); ++i) {
        Agent& agent = agents_[i];
        if (model_ == Model::CircuitRiver) {
            const int lane = static_cast<int>(i % 8);
            agent.position.x = margin + randomInt(
                0, std::max(0, textureSize_.x / 5));
            agent.position.y = margin +
                (lane * std::max(1, textureSize_.y - margin * 2 - 1)) / 7;
            agent.heading = randomInt(0, 2) - 1;
            agent.heading =
                synaptome::eight_direction::wrapIndex(agent.heading);
        } else if (model_ == Model::CircuitMycelium) {
            // Start several colonies across the canvas instead of making every
            // agent radiate from one central knot. A little seeded jitter keeps
            // the layout organic while the strata guarantee full-frame growth.
            const int colony = static_cast<int>(
                i % (kMyceliumSeedColumns * kMyceliumSeedRows));
            const int column = colony % kMyceliumSeedColumns;
            const int row = colony / kMyceliumSeedColumns;
            const int usableWidth = std::max(1, textureSize_.x - margin * 2);
            const int usableHeight = std::max(1, textureSize_.y - margin * 2);
            const int cellWidth = std::max(1, usableWidth / kMyceliumSeedColumns);
            const int cellHeight = std::max(1, usableHeight / kMyceliumSeedRows);
            const int jitterX = std::max(1, cellWidth / 4);
            const int jitterY = std::max(1, cellHeight / 4);
            agent.position = {
                margin + ((column * 2 + 1) * usableWidth) /
                    (kMyceliumSeedColumns * 2) +
                    randomInt(-jitterX, jitterX),
                margin + ((row * 2 + 1) * usableHeight) /
                    (kMyceliumSeedRows * 2) +
                    randomInt(-jitterY, jitterY)
            };
            agent.position = synaptome::eight_direction::clampPoint(
                agent.position, textureSize_, margin);
            agent.heading = static_cast<int>(i % 8);
        } else {
            agent.position = {
                randomInt(margin, textureSize_.x - margin - 1),
                randomInt(margin, textureSize_.y - margin - 1)
            };
            agent.heading = randomInt(0, 7);
        }
        agent.energy = 0.45f + randomUnit() * 0.55f;
        deposit(agent.position, 0.8f);
    }

    stepAccumulator_ = 0.0f;
    nextAutoReseedBeat_ = -1.0f;
    dirty_ = true;
}

void CircuitTraceLayer::diffuseAndDecay() {
    const float diffuse = ofClamp(paramDiffuse_, 0.0f, 1.0f);
    const float decay = ofClamp(paramDecay_, 0.0f, 0.2f);
    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            const glm::ivec2 p{ x, y };
            const int idx = indexFor(p);
            const float center = chemical_[static_cast<std::size_t>(idx)];
            const float neighbors =
                sample(p + glm::ivec2{ -1, 0 }) +
                sample(p + glm::ivec2{ 1, 0 }) +
                sample(p + glm::ivec2{ 0, -1 }) +
                sample(p + glm::ivec2{ 0, 1 });
            const float blur = (center * 4.0f + neighbors) / 8.0f;
            scratch_[static_cast<std::size_t>(idx)] =
                ofClamp(ofLerp(center, blur, diffuse) - decay, 0.0f, 1.0f);
            trace_[static_cast<std::size_t>(idx)] *=
                ofClamp(paramTracePersistence_, 0.90f, 1.0f);
            if (trace_[static_cast<std::size_t>(idx)] < 0.002f) {
                trace_[static_cast<std::size_t>(idx)] = 0.0f;
                vias_[static_cast<std::size_t>(idx)] = 0;
            }
        }
    }
    chemical_.swap(scratch_);
}

void CircuitTraceLayer::stepAgents() {
    for (auto& agent : agents_) {
        switch (model_) {
        case Model::CircuitMycelium:
            stepMycelium(agent);
            break;
        case Model::CircuitRiver:
            stepRiver(agent);
            break;
        case Model::CircuitAntTunnels:
            stepAntTunnels(agent);
            break;
        case Model::CircuitFlowField:
            stepFlowField(agent);
            break;
        case Model::CircuitSlime:
        default:
            stepSlime(agent);
            break;
        }
    }
}

void CircuitTraceLayer::stepSlime(Agent& agent) {
    const float explore =
        behavior() == Explore ? 1.0f : (behavior() == Exploit ? 0.15f : 0.5f);
    const float follow =
        behavior() == Exploit ? 1.5f : (behavior() == Explore ? 0.45f : 1.0f);
    int heading = chooseWeightedHeading(agent, follow, explore);
    if (randomUnit() < paramTurnChance_ * explore) {
        heading = synaptome::eight_direction::turn(
            agent.heading, randomUnit() < 0.5f ? -1 : 1);
    }
    if (randomUnit() < paramBranchChance_) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -2 : 2);
    }
    advance(agent, heading, true);
}

void CircuitTraceLayer::stepMycelium(Agent& agent) {
    const float explore =
        behavior() == Explore ? 1.25f : (behavior() == Exploit ? 0.2f : 0.75f);
    const float follow =
        behavior() == Exploit ? 1.4f : (behavior() == Explore ? 0.35f : 0.8f);
    int heading = chooseWeightedHeading(agent, follow, explore);
    if (randomUnit() < paramTurnChance_) {
        const int corner = randomUnit() < 0.5f ? -1 : 1;
        heading = synaptome::eight_direction::turn(heading, corner);
    }
    if (randomUnit() < paramBranchChance_ * (0.6f + agent.energy)) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -2 : 2);
        agent.energy = 0.4f + randomUnit() * 0.6f;
    } else {
        agent.energy = std::max(0.1f, agent.energy - 0.006f);
    }
    advance(agent, heading, false);
}

void CircuitTraceLayer::stepRiver(Agent& agent) {
    const float forward = sense(agent, agent.heading);
    const int leftHeading =
        synaptome::eight_direction::turn(agent.heading, -1);
    const int rightHeading =
        synaptome::eight_direction::turn(agent.heading, 1);
    const float left = sense(agent, leftHeading);
    const float right = sense(agent, rightHeading);

    int heading = agent.heading;
    if (behavior() != Explore && std::max(left, right) > forward + 0.01f) {
        heading = left > right ? leftHeading : rightHeading;
    } else if (randomUnit() < paramTurnChance_) {
        heading = randomUnit() < 0.5f ? leftHeading : rightHeading;
    }
    if (randomUnit() < paramBranchChance_) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -1 : 1);
    }
    advance(agent, heading, true);
}

void CircuitTraceLayer::stepAntTunnels(Agent& agent) {
    // Ant routes prefer long orthogonal runs. Balanced agents seek the edge of
    // an existing signal rather than its saturated center; Explore avoids it,
    // while Exploit reinforces it. This creates sparse routed corridors using
    // the same deterministic field lifecycle as the rest of the family.
    const std::array<int, 5> offsets{ 0, -2, 2, -1, 1 };
    int heading = agent.heading;
    float best = -std::numeric_limits<float>::infinity();
    for (int offset : offsets) {
        const int candidate =
            synaptome::eight_direction::turn(agent.heading, offset);
        const float signal = sense(agent, candidate);
        float score = 0.0f;
        if (behavior() == Explore) {
            score = -signal * 1.35f;
        } else if (behavior() == Exploit) {
            score = signal * 1.25f;
        } else {
            score = -std::abs(signal - 0.24f);
        }
        score += offset == 0 ? 0.16f : 0.0f;
        score += randomUnit() * 0.10f;
        if (score > best) {
            best = score;
            heading = candidate;
        }
    }
    if (randomUnit() < paramTurnChance_) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -2 : 2);
    }
    if (randomUnit() < paramBranchChance_) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -1 : 1);
    }
    advance(agent, heading, true);
}

void CircuitTraceLayer::stepFlowField(Agent& agent) {
    // A deterministic analytic flow vector is quantized at the final routing
    // seam, guaranteeing that even curved field motion becomes PCB-compatible
    // horizontal, vertical, or 45-degree trace segments.
    const float scale =
        1.0f / std::max(3.0f, paramSensorDistance_ * 2.0f);
    const float phase = static_cast<float>(appliedSeed_ % 10000U) * 0.0017f;
    const float x = static_cast<float>(agent.position.x);
    const float y = static_cast<float>(agent.position.y);
    const glm::vec2 flow{
        std::sin(y * scale + phase) +
            0.55f * std::cos((x + y) * scale * 0.63f - phase),
        std::cos(x * scale - phase) -
            0.55f * std::sin((x - y) * scale * 0.63f + phase)
    };
    int heading = synaptome::eight_direction::quantizeVector(
        flow, agent.heading);

    if (behavior() == Exploit) {
        const float currentSignal = sense(agent, heading);
        const int left =
            synaptome::eight_direction::turn(heading, -1);
        const int right =
            synaptome::eight_direction::turn(heading, 1);
        if (sense(agent, left) > currentSignal) {
            heading = left;
        }
        if (sense(agent, right) > sense(agent, heading)) {
            heading = right;
        }
    } else if (behavior() == Explore &&
               randomUnit() < paramTurnChance_ * 1.5f) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -1 : 1);
    } else if (randomUnit() < paramTurnChance_) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -1 : 1);
    }
    if (randomUnit() < paramBranchChance_) {
        heading = synaptome::eight_direction::turn(
            heading, randomUnit() < 0.5f ? -2 : 2);
    }
    advance(agent, heading, true);
}

void CircuitTraceLayer::advance(Agent& agent, int nextHeading, bool wrap) {
    (void)wrap;
    agent.heading = synaptome::eight_direction::wrapIndex(nextHeading);
    const glm::ivec2 unit = synaptome::eight_direction::step(agent.heading);
    const int distance =
        static_cast<int>(std::round(ofClamp(paramStepSize_, 1.0f, 4.0f)));
    const glm::ivec2 from = agent.position;

    for (int step = 0; step < distance; ++step) {
        glm::ivec2 next = agent.position + unit;
        if (next.x < 1 || next.x >= textureSize_.x - 1 ||
            next.y < 1 || next.y >= textureSize_.y - 1) {
            agent.heading =
                synaptome::eight_direction::opposite(agent.heading);
            const glm::ivec2 reflected =
                synaptome::eight_direction::step(agent.heading);
            next = agent.position + reflected;
            if (next.x < 1 || next.x >= textureSize_.x - 1 ||
                next.y < 1 || next.y >= textureSize_.y - 1) {
                break;
            }
        }
        agent.position = next;
    }

    depositSegment(from, agent.position,
                   paramDeposit_ * (0.65f + agent.energy));
    if (randomUnit() < paramViaChance_) {
        markVia(agent.position);
    }
}

void CircuitTraceLayer::depositSegment(
    glm::ivec2 from, glm::ivec2 to, float amount) {
    const glm::ivec2 delta = to - from;
    const int steps = std::max(std::abs(delta.x), std::abs(delta.y));
    if (steps <= 0) {
        deposit(to, amount);
        return;
    }
    // Since movement is axis-aligned or 45-degree diagonal, integer division
    // produces the exact compass step with no interpolated off-grid angle.
    const glm::ivec2 unit{ delta.x / steps, delta.y / steps };
    for (int i = 0; i <= steps; ++i) {
        deposit(from + unit * i, amount);
    }
}

void CircuitTraceLayer::deposit(
    glm::ivec2 point, float amount, bool hardTrace) {
    if (point.x < 0 || point.x >= textureSize_.x ||
        point.y < 0 || point.y >= textureSize_.y) {
        return;
    }
    const std::size_t idx = static_cast<std::size_t>(indexFor(point));
    chemical_[idx] = ofClamp(chemical_[idx] + amount, 0.0f, 1.0f);
    if (hardTrace) {
        trace_[idx] = ofClamp(trace_[idx] + amount * 1.35f, 0.0f, 1.0f);
    }
}

void CircuitTraceLayer::markVia(glm::ivec2 point) {
    if (point.x < 0 || point.x >= textureSize_.x ||
        point.y < 0 || point.y >= textureSize_.y) {
        return;
    }
    const std::size_t idx = static_cast<std::size_t>(indexFor(point));
    vias_[idx] = 255;
    trace_[idx] = 1.0f;
}

float CircuitTraceLayer::sense(const Agent& agent, int heading) const {
    const glm::ivec2 direction =
        synaptome::eight_direction::step(heading);
    const int distance = static_cast<int>(
        std::round(ofClamp(paramSensorDistance_, 1.0f, 12.0f)));
    return sample(agent.position + direction * distance);
}

float CircuitTraceLayer::sample(glm::ivec2 point) const {
    point = synaptome::eight_direction::wrapPoint(point, textureSize_);
    return chemical_[static_cast<std::size_t>(indexFor(point))];
}

int CircuitTraceLayer::chooseWeightedHeading(
    const Agent& agent, float followWeight, float exploreWeight) {
    const std::array<int, 5> offsets{ 0, -1, 1, -2, 2 };
    int chosen = agent.heading;
    float best = -std::numeric_limits<float>::infinity();
    for (int offset : offsets) {
        const int candidate =
            synaptome::eight_direction::turn(agent.heading, offset);
        const float forwardBias = offset == 0 ? 0.08f : 0.0f;
        const float score =
            sense(agent, candidate) * followWeight + forwardBias +
            randomUnit() * exploreWeight * 0.22f;
        if (score > best) {
            best = score;
            chosen = candidate;
        }
    }
    return chosen;
}

void CircuitTraceLayer::syncTexture() {
    if (!pixels_.isAllocated()) {
        return;
    }
    const ofFloatColor background(
        ofClamp(paramBgR_, 0.0f, 1.0f),
        ofClamp(paramBgG_, 0.0f, 1.0f),
        ofClamp(paramBgB_, 0.0f, 1.0f),
        ofClamp(paramBackgroundAlpha_, 0.0f, 1.0f));
    const ofFloatColor copper(
        ofClamp(paramTraceR_, 0.0f, 1.0f),
        ofClamp(paramTraceG_, 0.0f, 1.0f),
        ofClamp(paramTraceB_, 0.0f, 1.0f),
        ofClamp(paramTrailAlpha_, 0.0f, 1.0f));
    const float traceRadius =
        ofClamp(paramTraceWidth_, 1.0f, 4.0f) * 0.72f;
    const float junctionRadius = std::max(1.0f, traceRadius + 0.45f);
    const int sampleRadius =
        static_cast<int>(std::ceil(junctionRadius + 1.0f));

    for (int y = 0; y < textureSize_.y; ++y) {
        for (int x = 0; x < textureSize_.x; ++x) {
            float hard = 0.0f;
            float junction = 0.0f;
            for (int oy = -sampleRadius; oy <= sampleRadius; ++oy) {
                for (int ox = -sampleRadius; ox <= sampleRadius; ++ox) {
                    const glm::ivec2 q{ x + ox, y + oy };
                    if (q.x < 0 || q.x >= textureSize_.x ||
                        q.y < 0 || q.y >= textureSize_.y) {
                        continue;
                    }
                    const std::size_t qidx =
                        static_cast<std::size_t>(indexFor(q));
                    const float distance =
                        std::sqrt(static_cast<float>(ox * ox + oy * oy));
                    const float traceCoverage = ofClamp(
                        traceRadius + 0.55f - distance, 0.0f, 1.0f);
                    if (traceCoverage > 0.0f) {
                        hard = std::max(
                            hard, trace_[qidx] * traceCoverage);
                    }
                    if (vias_[qidx] != 0) {
                        const float junctionCoverage = ofClamp(
                            junctionRadius + 0.55f - distance, 0.0f, 1.0f);
                        junction = std::max(junction, junctionCoverage);
                    }
                }
            }

            const std::size_t idx =
                static_cast<std::size_t>(indexFor({ x, y }));
            const float halo = ofClamp(
                chemical_[idx] * ofClamp(paramGlow_, 0.0f, 4.0f) * 0.55f,
                0.0f, 1.0f);
            const float intensity =
                std::max(std::max(hard, halo), junction);

            ofFloatColor color(
                ofLerp(background.r, copper.r, intensity),
                ofLerp(background.g, copper.g, intensity),
                ofLerp(background.b, copper.b, intensity),
                ofLerp(background.a, copper.a, intensity));
            pixels_.setColor(x, y, color);
        }
    }
    texture_.loadData(pixels_);
}

float CircuitTraceLayer::stepRateFor(
    const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) *
            ofClamp(paramBpmMultiplier_, 0.25f, 8.0f) *
            std::max(0.0f, params.speed);
    }
    return ofClamp(paramSpeed_, 0.0f, 40.0f) *
        std::max(0.0f, params.speed);
}

float CircuitTraceLayer::currentBeatPosition(
    float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) {
        return 0.0f;
    }
    return std::max(0.0f, timeSeconds) * bpm / 60.0f;
}

float CircuitTraceLayer::randomUnit() {
    return std::generate_canonical<float, 24>(rng_);
}

int CircuitTraceLayer::randomInt(int minInclusive, int maxInclusive) {
    if (maxInclusive <= minInclusive) {
        return minInclusive;
    }
    std::uniform_int_distribution<int> distribution(
        minInclusive, maxInclusive);
    return distribution(rng_);
}

std::uint32_t CircuitTraceLayer::requestedSeed() const {
    return static_cast<std::uint32_t>(
        std::round(ofClamp(paramSeed_, 1.0f, 999999.0f)));
}

int CircuitTraceLayer::behavior() const {
    return static_cast<int>(std::round(ofClamp(
        paramBehavior_, 0.0f, static_cast<float>(kBehaviorCount - 1))));
}

int CircuitTraceLayer::indexFor(glm::ivec2 point) const {
    return point.y * textureSize_.x + point.x;
}

std::uint64_t CircuitTraceLayer::debugStateSignature() const {
    std::uint64_t hash = 1469598103934665603ULL;
    hash = fnvMix(hash, static_cast<std::uint64_t>(model_));
    hash = fnvMix(hash, appliedSeed_);
    for (const Agent& agent : agents_) {
        hash = fnvMix(hash, static_cast<std::uint64_t>(agent.position.x));
        hash = fnvMix(hash, static_cast<std::uint64_t>(agent.position.y));
        hash = fnvMix(hash, static_cast<std::uint64_t>(agent.heading));
    }
    // Quantization avoids platform-specific floating-point low-bit noise while
    // still detecting any meaningful simulation divergence.
    for (std::size_t i = 0; i < trace_.size(); i += 7) {
        hash = fnvMix(hash, static_cast<std::uint64_t>(
            std::round(ofClamp(trace_[i], 0.0f, 1.0f) * 65535.0f)));
    }
    return hash;
}

std::vector<int> CircuitTraceLayer::debugAgentHeadings() const {
    std::vector<int> result;
    result.reserve(agents_.size());
    for (const Agent& agent : agents_) {
        result.push_back(agent.heading);
    }
    return result;
}

void CircuitTraceLayer::reseedForTest() {
    resetSimulation();
}

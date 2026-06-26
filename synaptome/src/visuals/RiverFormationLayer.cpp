#include "RiverFormationLayer.h"

#include "ofGraphics.h"
#include "ofMath.h"
#include "ofUtils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace {
    constexpr float kMinSegmentLength = 0.0001f;

    float smootherStep(float value) {
        const float t = ofClamp(value, 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float cross2d(const glm::vec2& a, const glm::vec2& b) {
        return a.x * b.y - a.y * b.x;
    }

    glm::vec2 safeNormalize(const glm::vec2& value, const glm::vec2& fallback = glm::vec2(1.0f, 0.0f)) {
        const float len = glm::length(value);
        if (len <= kMinSegmentLength || !std::isfinite(len)) {
            return fallback;
        }
        return value / len;
    }

    float randomRange(std::mt19937& rng, float minValue, float maxValue) {
        std::uniform_real_distribution<float> dist(minValue, maxValue);
        return dist(rng);
    }

    int randomInt(std::mt19937& rng, int minValue, int maxValue) {
        std::uniform_int_distribution<int> dist(minValue, maxValue);
        return dist(rng);
    }

    float randomUnit(std::mt19937& rng) {
        return randomRange(rng, 0.0f, 1.0f);
    }

    struct SegmentApproach {
        float distanceSq = std::numeric_limits<float>::max();
        glm::vec2 first{ 0.0f, 0.0f };
        glm::vec2 second{ 0.0f, 0.0f };
        float firstT = 0.0f;
        float secondT = 0.0f;
    };

    SegmentApproach closestSegmentApproach(const glm::vec2& p1,
                                           const glm::vec2& q1,
                                           const glm::vec2& p2,
                                           const glm::vec2& q2) {
        SegmentApproach result;
        const glm::vec2 d1 = q1 - p1;
        const glm::vec2 d2 = q2 - p2;
        const glm::vec2 r = p1 - p2;
        const float a = glm::dot(d1, d1);
        const float e = glm::dot(d2, d2);
        const float f = glm::dot(d2, r);

        if (a <= kMinSegmentLength && e <= kMinSegmentLength) {
            result.first = p1;
            result.second = p2;
        } else if (a <= kMinSegmentLength) {
            result.first = p1;
            result.secondT = ofClamp(f / std::max(kMinSegmentLength, e), 0.0f, 1.0f);
            result.second = p2 + d2 * result.secondT;
        } else {
            const float c = glm::dot(d1, r);
            if (e <= kMinSegmentLength) {
                result.second = p2;
                result.firstT = ofClamp(-c / a, 0.0f, 1.0f);
                result.first = p1 + d1 * result.firstT;
            } else {
                const float b = glm::dot(d1, d2);
                const float denom = a * e - b * b;
                if (std::abs(denom) > kMinSegmentLength) {
                    result.firstT = ofClamp((b * f - c * e) / denom, 0.0f, 1.0f);
                }
                result.secondT = (b * result.firstT + f) / e;
                if (result.secondT < 0.0f) {
                    result.secondT = 0.0f;
                    result.firstT = ofClamp(-c / a, 0.0f, 1.0f);
                } else if (result.secondT > 1.0f) {
                    result.secondT = 1.0f;
                    result.firstT = ofClamp((b - c) / a, 0.0f, 1.0f);
                }
                result.first = p1 + d1 * result.firstT;
                result.second = p2 + d2 * result.secondT;
            }
        }

        const glm::vec2 delta = result.first - result.second;
        result.distanceSq = glm::dot(delta, delta);
        return result;
    }
}

void RiverFormationLayer::configure(const ofJson& config) {
    if (config.contains("defaults") && config["defaults"].is_object()) {
        const auto& def = config["defaults"];
        paramEnabled_ = def.value("visible", paramEnabled_);
        paramAlpha_ = def.value("alpha", paramAlpha_);
        paramSpeed_ = def.value("speed", paramSpeed_);
        paramBpmSync_ = def.value("bpmSync", paramBpmSync_);
        paramBpmMultiplier_ = def.value("bpmMultiplier", paramBpmMultiplier_);
        paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
        paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
        paramSeed_ = def.value("seed", paramSeed_);
        paramWarmupSteps_ = def.value("warmupSteps", paramWarmupSteps_);
        paramPathPoints_ = def.value("pathPoints", def.value("centerlinePoints", paramPathPoints_));
        paramRiverWidth_ = def.value("riverWidth", paramRiverWidth_);
        paramWidthVariation_ = def.value("widthVariation", paramWidthVariation_);
        paramWidthPulse_ = def.value("widthPulse", paramWidthPulse_);
        paramMigrationRate_ = def.value("migrationRate", paramMigrationRate_);
        paramErosionStrength_ = def.value("erosionStrength", paramErosionStrength_);
        paramDepositionStrength_ = def.value("depositionStrength", paramDepositionStrength_);
        paramChannelDepth_ = def.value("channelDepth", paramChannelDepth_);
        paramBankHardness_ = def.value("bankHardness", paramBankHardness_);
        paramTrailDecay_ = def.value("trailDecay", paramTrailDecay_);
        paramOxbowDecay_ = def.value("oxbowDecay", paramOxbowDecay_);
        paramCutoffFactor_ = def.value("cutoffFactor", paramCutoffFactor_);
        paramBranchChance_ = def.value("branchChance", paramBranchChance_);
        paramMaxBranches_ = def.value("maxBranches", paramMaxBranches_);
        paramBranchLength_ = def.value("branchLength", paramBranchLength_);
        paramBranchAngle_ = def.value("branchAngle", paramBranchAngle_);
        paramBranchWidth_ = def.value("branchWidth", paramBranchWidth_);
        paramNoiseAmount_ = def.value("noiseAmount", paramNoiseAmount_);
        paramValleyConfinement_ = def.value("valleyConfinement", paramValleyConfinement_);
        paramMeanderSmoothing_ = def.value("meanderSmoothing", paramMeanderSmoothing_);
        paramCurvatureMemory_ = def.value("curvatureMemory", paramCurvatureMemory_);
        paramStabilityClamp_ = def.value("stabilityClamp", paramStabilityClamp_);
        paramTrailBoost_ = def.value("trailBoost", paramTrailBoost_);
        paramTrailAlpha_ = def.value("trailAlpha", paramTrailAlpha_);
        paramOxbowAlpha_ = def.value("oxbowAlpha", paramOxbowAlpha_);
        paramGlowAmount_ = def.value("glowAmount", paramGlowAmount_);
        paramMaskThreshold_ = def.value("maskThreshold", paramMaskThreshold_);
        paramColorR_ = def.value("colorR", paramColorR_);
        paramColorG_ = def.value("colorG", paramColorG_);
        paramColorB_ = def.value("colorB", paramColorB_);
        readColorArray(def, "foregroundColor", paramColorR_, paramColorG_, paramColorB_);
        readColorArray(def, "trailColor", paramColorR_, paramColorG_, paramColorB_);
    }

    if (config.contains("textureSize") && config["textureSize"].is_array() && config["textureSize"].size() >= 2) {
        fieldSize_.x = std::max(64, config["textureSize"][0].get<int>());
        fieldSize_.y = std::max(48, config["textureSize"][1].get<int>());
    }
}

void RiverFormationLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.riverFormation" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Generative";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Visibility: River Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".speed", &paramSpeed_, paramSpeed_, "Time: Formation Speed", 0.0f, 720.0f, 0.5f,
                  "Free-running river graph simulation steps per second.");

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: BPM Sync";
    meta.description = "Use transport BPM instead of the free-running formation speed.";
    registry.addBool(prefix + ".bpmSync", &paramBpmSync_, paramBpmSync_, meta);
    registerFloat(registry, prefix + ".bpmMultiplier", &paramBpmMultiplier_, paramBpmMultiplier_, "Time: BPM Mult", 0.25f, 16.0f, 0.25f);

    meta = {};
    meta.group = "Generative";
    meta.label = "Action: Reseed";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);
    meta.label = "Action: Auto Reseed";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, meta);
    registerFloat(registry, prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, "Time: Auto Reseed Beats", 8.0f, 512.0f, 1.0f);
    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: River", 0.0f, 999999.0f, 1.0f);
    registerFloat(registry, prefix + ".warmupSteps", &paramWarmupSteps_, paramWarmupSteps_, "Time: Warmup Steps", 0.0f, 800.0f, 1.0f,
                  "Optional hidden simulation steps to pre-age the channel after reset. Use 0 for a straight-start reveal.");

    registerFloat(registry, prefix + ".pathPoints", &paramPathPoints_, paramPathPoints_, "Count: Path Points", 48.0f, 300.0f, 1.0f);
    registerFloat(registry, prefix + ".riverWidth", &paramRiverWidth_, paramRiverWidth_, "Scale: River Width", 1.0f, 18.0f, 0.1f);
    registerFloat(registry, prefix + ".widthVariation", &paramWidthVariation_, paramWidthVariation_, "Scale: Width Variation", 0.0f, 1.4f, 0.01f);
    registerFloat(registry, prefix + ".widthPulse", &paramWidthPulse_, paramWidthPulse_, "Motion: Width Pulse", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".migrationRate", &paramMigrationRate_, paramMigrationRate_, "Motion: Bank Migration", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".erosionStrength", &paramErosionStrength_, paramErosionStrength_, "Force: Erosion", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".depositionStrength", &paramDepositionStrength_, paramDepositionStrength_, "Force: Deposition", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".channelDepth", &paramChannelDepth_, paramChannelDepth_, "Scale: 3D Channel Depth", 0.0f, 1.0f, 0.01f,
                  "Stored in the heightfield for future 3D terrain use; the 2D renderer stays mask-based.");
    registerFloat(registry, prefix + ".bankHardness", &paramBankHardness_, paramBankHardness_, "Force: Bank Hardness", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailDecay", &paramTrailDecay_, paramTrailDecay_, "Time: Trail Decay", 0.0f, 0.12f, 0.001f);
    registerFloat(registry, prefix + ".oxbowDecay", &paramOxbowDecay_, paramOxbowDecay_, "Time: Oxbow Decay", 0.0f, 0.05f, 0.001f);
    registerFloat(registry, prefix + ".cutoffFactor", &paramCutoffFactor_, paramCutoffFactor_, "Scale: Cutoff Distance", 0.0f, 6.0f, 0.01f);
    registerFloat(registry, prefix + ".branchChance", &paramBranchChance_, paramBranchChance_, "Growth: Branch Chance", 0.0f, 0.20f, 0.001f);
    registerFloat(registry, prefix + ".maxBranches", &paramMaxBranches_, paramMaxBranches_, "Count: Max Branches", 0.0f, 12.0f, 1.0f);
    registerFloat(registry, prefix + ".branchLength", &paramBranchLength_, paramBranchLength_, "Scale: Branch Length", 0.15f, 1.6f, 0.01f);
    registerFloat(registry, prefix + ".branchAngle", &paramBranchAngle_, paramBranchAngle_, "Motion: Branch Angle", 0.0f, 1.2f, 0.01f);
    registerFloat(registry, prefix + ".branchWidth", &paramBranchWidth_, paramBranchWidth_, "Scale: Branch Width", 0.15f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".noiseAmount", &paramNoiseAmount_, paramNoiseAmount_, "Force: Meander Noise", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".valleyConfinement", &paramValleyConfinement_, paramValleyConfinement_, "Force: Valley Confinement", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".meanderSmoothing", &paramMeanderSmoothing_, paramMeanderSmoothing_, "Motion: Meander Smoothing", 0.0f, 0.45f, 0.01f);
    registerFloat(registry, prefix + ".curvatureMemory", &paramCurvatureMemory_, paramCurvatureMemory_, "Motion: Upstream Curvature", 1.0f, 24.0f, 0.1f,
                  "Howard-Knutson style upstream curvature memory measured in channel widths.");
    registerFloat(registry, prefix + ".stabilityClamp", &paramStabilityClamp_, paramStabilityClamp_, "Motion: Stability Clamp", 0.05f, 1.0f, 0.01f,
                  "CFL-style cap on per-step centerline displacement, relative to node spacing.");

    registerFloat(registry, prefix + ".trailBoost", &paramTrailBoost_, paramTrailBoost_, "Glow: Trail Boost", 0.1f, 8.0f, 0.01f);
    registerFloat(registry, prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, "Visibility: Trail Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".oxbowAlpha", &paramOxbowAlpha_, paramOxbowAlpha_, "Visibility: Oxbow Opacity", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".glowAmount", &paramGlowAmount_, paramGlowAmount_, "Glow: Soft Edge", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".maskThreshold", &paramMaskThreshold_, paramMaskThreshold_, "Visibility: Mask Threshold", 0.0f, 0.3f, 0.001f);
    registerFloat(registry, prefix + ".colorR", &paramColorR_, paramColorR_, "Color: Foreground R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorG", &paramColorG_, paramColorG_, "Color: Foreground G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorB", &paramColorB_, paramColorB_, "Color: Foreground B", 0.0f, 1.5f, 0.01f);

    resetSimulation();
    fieldSignatureState_ = renderSignature();
}

void RiverFormationLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) return;

    clampParams();

    const auto desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    const int desiredPoints = static_cast<int>(std::round(paramPathPoints_));
    if (cells_.empty() || desiredSeed != seedState_ || desiredPoints != pathPointState_ || paramReseedRequested_) {
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

    cutoffPulse_ = std::max(0.0f, cutoffPulse_ - params.dt * 0.85f);
    const float signature = renderSignature();
    if (std::abs(signature - fieldSignatureState_) > 0.0001f) {
        dirty_ = true;
        fieldSignatureState_ = signature;
    }

    const float stepRate = stepRateFor(params);
    if (stepRate > 0.0f) {
        stepAccumulator_ += params.dt * stepRate;
        const int iterations = std::min(384, static_cast<int>(std::floor(stepAccumulator_)));
        if (iterations > 0) {
            stepAccumulator_ -= static_cast<float>(iterations);
            for (int i = 0; i < iterations; ++i) {
                simulateStep();
            }
        }
    }

    if (dirty_) {
        refreshTexture();
        dirty_ = false;
    }
}

void RiverFormationLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f || !texture_.isAllocated()) {
        return;
    }

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(255, 255, 255, static_cast<int>(ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f) * 255.0f));
    texture_.draw(0, 0, params.viewport.x, params.viewport.y);

    const int r = static_cast<int>(ofClamp(paramColorR_, 0.0f, 1.0f) * 255.0f);
    const int g = static_cast<int>(ofClamp(paramColorG_, 0.0f, 1.0f) * 255.0f);
    const int b = static_cast<int>(ofClamp(paramColorB_, 0.0f, 1.0f) * 255.0f);
    const float slotAlpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);

    for (const auto& path : abandonedPaths_) {
        const float ageFade = 1.0f - ofClamp(path.age / 1200.0f, 0.0f, 1.0f);
        const float ghostAlpha = 0.22f * ageFade * paramOxbowAlpha_ * slotAlpha;
        if (ghostAlpha <= 0.002f) continue;
        ofSetColor(r, g, b, static_cast<int>(255.0f * ghostAlpha));
        drawPathPolyline(path, params, ghostAlpha, 1.15f);
    }

    if (!paths_.empty()) {
        ofSetColor(r, g, b, static_cast<int>(255.0f * slotAlpha));
        drawPathPolyline(paths_.front(), params, slotAlpha, 1.75f);
    }

    ofPopView();
    ofPopStyle();
}

void RiverFormationLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void RiverFormationLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    dirty_ = true;
}

float RiverFormationLayer::normalizedElevationAt(const glm::vec2& uv) const {
    if (cells_.empty()) return 0.0f;
    const float x = ofClamp(uv.x, 0.0f, 1.0f) * static_cast<float>(fieldSize_.x - 1);
    const float y = ofClamp(uv.y, 0.0f, 1.0f) * static_cast<float>(fieldSize_.y - 1);
    return ofClamp(sampleCell({ x, y }).elevation, 0.0f, 1.0f);
}

float RiverFormationLayer::wetnessAt(const glm::vec2& uv) const {
    if (cells_.empty()) return 0.0f;
    const float x = ofClamp(uv.x, 0.0f, 1.0f) * static_cast<float>(fieldSize_.x - 1);
    const float y = ofClamp(uv.y, 0.0f, 1.0f) * static_cast<float>(fieldSize_.y - 1);
    const auto cell = sampleCell({ x, y });
    return ofClamp(std::max(cell.wetness, std::max(cell.activeWater, cell.abandonedWater)), 0.0f, 1.0f);
}

void RiverFormationLayer::registerFloat(ParameterRegistry& registry,
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

void RiverFormationLayer::readColorArray(const ofJson& def, const char* key, float& r, float& g, float& b) {
    if (def.contains(key) && def[key].is_array() && def[key].size() >= 3) {
        r = def[key][0].get<float>();
        g = def[key][1].get<float>();
        b = def[key][2].get<float>();
    }
}

void RiverFormationLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramSpeed_ = ofClamp(paramSpeed_, 0.0f, 720.0f);
    paramBpmMultiplier_ = ofClamp(paramBpmMultiplier_, 0.25f, 16.0f);
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 8.0f, 512.0f));
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 999999.0f));
    paramWarmupSteps_ = std::round(ofClamp(paramWarmupSteps_, 0.0f, 800.0f));
    paramPathPoints_ = std::round(ofClamp(paramPathPoints_, 48.0f, 300.0f));
    paramRiverWidth_ = ofClamp(paramRiverWidth_, 1.0f, 18.0f);
    paramWidthVariation_ = ofClamp(paramWidthVariation_, 0.0f, 1.4f);
    paramWidthPulse_ = ofClamp(paramWidthPulse_, 0.0f, 1.0f);
    paramMigrationRate_ = ofClamp(paramMigrationRate_, 0.0f, 4.0f);
    paramErosionStrength_ = ofClamp(paramErosionStrength_, 0.0f, 2.0f);
    paramDepositionStrength_ = ofClamp(paramDepositionStrength_, 0.0f, 2.0f);
    paramChannelDepth_ = ofClamp(paramChannelDepth_, 0.0f, 1.0f);
    paramBankHardness_ = ofClamp(paramBankHardness_, 0.0f, 1.0f);
    paramTrailDecay_ = ofClamp(paramTrailDecay_, 0.0f, 0.12f);
    paramOxbowDecay_ = ofClamp(paramOxbowDecay_, 0.0f, 0.05f);
    paramCutoffFactor_ = ofClamp(paramCutoffFactor_, 0.0f, 6.0f);
    paramBranchChance_ = ofClamp(paramBranchChance_, 0.0f, 0.20f);
    paramMaxBranches_ = std::round(ofClamp(paramMaxBranches_, 0.0f, 12.0f));
    paramBranchLength_ = ofClamp(paramBranchLength_, 0.15f, 1.6f);
    paramBranchAngle_ = ofClamp(paramBranchAngle_, 0.0f, 1.2f);
    paramBranchWidth_ = ofClamp(paramBranchWidth_, 0.15f, 1.0f);
    paramNoiseAmount_ = ofClamp(paramNoiseAmount_, 0.0f, 2.0f);
    paramValleyConfinement_ = ofClamp(paramValleyConfinement_, 0.0f, 1.0f);
    paramMeanderSmoothing_ = ofClamp(paramMeanderSmoothing_, 0.0f, 0.45f);
    paramCurvatureMemory_ = ofClamp(paramCurvatureMemory_, 1.0f, 24.0f);
    paramStabilityClamp_ = ofClamp(paramStabilityClamp_, 0.05f, 1.0f);
    paramTrailBoost_ = ofClamp(paramTrailBoost_, 0.1f, 8.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramOxbowAlpha_ = ofClamp(paramOxbowAlpha_, 0.0f, 1.0f);
    paramGlowAmount_ = ofClamp(paramGlowAmount_, 0.0f, 1.0f);
    paramMaskThreshold_ = ofClamp(paramMaskThreshold_, 0.0f, 0.3f);
    paramColorR_ = ofClamp(paramColorR_, 0.0f, 1.5f);
    paramColorG_ = ofClamp(paramColorG_, 0.0f, 1.5f);
    paramColorB_ = ofClamp(paramColorB_, 0.0f, 1.5f);
}

void RiverFormationLayer::allocateFields() {
    fieldSize_.x = std::max(64, fieldSize_.x);
    fieldSize_.y = std::max(48, fieldSize_.y);
    const std::size_t count = static_cast<std::size_t>(fieldSize_.x * fieldSize_.y);
    cells_.assign(count, {});
    pixels_.allocate(fieldSize_.x, fieldSize_.y, 4);
    texture_.allocate(fieldSize_.x, fieldSize_.y, GL_RGBA32F);
    texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    dirty_ = true;
}

void RiverFormationLayer::resetSimulation() {
    clampParams();
    allocateFields();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    pathPointState_ = static_cast<int>(std::round(paramPathPoints_));
    std::mt19937 rng(seedState_ == 0 ? 1u : seedState_);
    initializeFloodplain();
    buildInitialMainPath(rng);
    abandonedPaths_.clear();
    simAge_ = 0.0f;
    cutoffCooldown_ = 24;
    cutoffPulse_ = 0.0f;
    stepAccumulator_ = 0.0f;
    for (auto& path : paths_) {
        updatePathGeometry(path);
    }
    syncMainCenterline();

    const int warmupSteps = static_cast<int>(std::round(ofClamp(paramWarmupSteps_, 0.0f, 800.0f)));
    for (int i = 0; i < warmupSteps; ++i) {
        simulateStep();
    }
    if (warmupSteps == 0) {
        paintRiverEffects();
    }
    refreshTexture();
    dirty_ = false;
}

void RiverFormationLayer::initializeFloodplain() {
    const float seedOffset = static_cast<float>(seedState_) * 0.0017f;
    for (int y = 0; y < fieldSize_.y; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(std::max(1, fieldSize_.y - 1));
        for (int x = 0; x < fieldSize_.x; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(1, fieldSize_.x - 1));
            RiverCell& cell = cellAt(x, y);
            const float broad = noiseSample(u * 2.4f + seedOffset, v * 2.4f - seedOffset, 3.0f);
            const float medium = noiseSample(u * 5.6f - seedOffset * 0.41f, v * 4.8f + seedOffset * 0.53f, 5.5f);
            const float fine = noiseSample(u * 11.0f - seedOffset, v * 8.5f + seedOffset, 8.0f);
            const float valley = std::abs(v - 0.5f) * 0.14f + u * 0.025f;
            cell.elevation = ofClamp(0.48f + valley + (broad - 0.5f) * 0.14f + (medium - 0.5f) * 0.07f + (fine - 0.5f) * 0.025f, 0.0f, 1.0f);
            cell.bankResistance = ofClamp(paramBankHardness_ * 0.50f + broad * 0.22f + medium * 0.22f + fine * 0.14f, 0.0f, 1.0f);
        }
    }
}

void RiverFormationLayer::buildInitialMainPath(std::mt19937& rng) {
    paths_.clear();
    RiverPath main;
    main.mainStem = true;
    main.widthScale = 1.0f;

    const int count = std::max(8, pathPointState_);
    main.nodes.reserve(static_cast<std::size_t>(count));
    const float offscreen = std::max(16.0f, paramRiverWidth_ * 4.0f);
    const float marginY = std::max(4.0f, paramRiverWidth_ * 1.5f);
    const float baseY = static_cast<float>(fieldSize_.y) * 0.5f;
    float seededOffset = 0.0f;
    const float seedAmplitude = std::max(1.25f, static_cast<float>(fieldSize_.y) * 0.008f);

    for (int i = 0; i < count; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(count - 1);
        RiverNode node;
        node.fieldPosition.x = ofLerp(-offscreen, static_cast<float>(fieldSize_.x) - 1.0f + offscreen, u);
        const float edgeTaper = smootherStep(ofClamp(u * 5.0f, 0.0f, 1.0f)) *
                                smootherStep(ofClamp((1.0f - u) * 5.0f, 0.0f, 1.0f));
        const float coarseOffset = signedNoise(u * 7.0f + static_cast<float>(seedState_) * 0.011f, 2.7f, 0.0f);
        const float fineOffset = signedNoise(u * 31.0f - static_cast<float>(seedState_) * 0.017f, 7.9f, 0.0f);
        const float targetOffset = (coarseOffset * 0.72f + fineOffset * 0.28f) * seedAmplitude;
        seededOffset = ofLerp(seededOffset, targetOffset, 0.22f);
        node.fieldPosition.y = baseY + seededOffset * edgeTaper;
        node.fieldPosition.y = ofClamp(node.fieldPosition.y, marginY, static_cast<float>(fieldSize_.y) - 1.0f - marginY);
        main.nodes.push_back(node);
    }

    paths_.push_back(std::move(main));
    for (int i = 0; i < 2; ++i) {
        smoothPath(paths_.back());
    }
    constrainPathProgress(paths_.back());
}

RiverFormationLayer::RiverPath RiverFormationLayer::buildBranchFrom(const RiverPath& parent, int parentIndex, std::mt19937& rng) const {
    const int downstreamOffset = std::max(14, static_cast<int>(std::round(paramPathPoints_ * randomRange(rng, 0.12f, 0.26f) * paramBranchLength_)));
    const int joinIndex = std::min(static_cast<int>(parent.nodes.size()) - 2, parentIndex + downstreamOffset);
    return buildBranchBetween(parent, parentIndex, joinIndex, rng);
}

RiverFormationLayer::RiverPath RiverFormationLayer::buildBranchBetween(const RiverPath& parent, int startIndex, int endIndex, std::mt19937& rng) const {
    RiverPath branch;
    branch.mainStem = false;
    branch.widthScale = paramBranchWidth_ * randomRange(rng, 0.42f, 0.72f);
    startIndex = ofClamp(startIndex, 1, static_cast<int>(parent.nodes.size()) - 3);
    endIndex = ofClamp(endIndex, startIndex + 8, static_cast<int>(parent.nodes.size()) - 2);
    const RiverNode& root = parent.nodes[static_cast<std::size_t>(startIndex)];
    const RiverNode& join = parent.nodes[static_cast<std::size_t>(endIndex)];
    branch.attachStartIndex = startIndex;
    branch.attachEndIndex = endIndex;
    const glm::vec2 shortcut = join.fieldPosition - root.fieldPosition;
    const glm::vec2 shortcutDir = safeNormalize(shortcut);
    const glm::vec2 normal(-shortcutDir.y, shortcutDir.x);
    const float reach = glm::length(join.fieldPosition - root.fieldPosition);
    const float parentMidIndex = static_cast<float>(startIndex + endIndex) * 0.5f;
    const float parentMidY = parent.nodes[static_cast<std::size_t>(ofClamp(static_cast<int>(std::round(parentMidIndex)), 0, static_cast<int>(parent.nodes.size()) - 1))].fieldPosition.y;
    const float side = ((root.fieldPosition.y + join.fieldPosition.y) * 0.5f < parentMidY) ? -1.0f : 1.0f;
    const float sideOffset = reach * ofLerp(0.035f, 0.16f, paramBranchAngle_) * side;
    const glm::vec2 control = (root.fieldPosition + join.fieldPosition) * 0.5f + normal * sideOffset;

    const int count = std::max(10, static_cast<int>(std::round(static_cast<float>(endIndex - startIndex) * 0.55f)));
    branch.nodes.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(count - 1);
        const glm::vec2 a = glm::mix(root.fieldPosition, control, u);
        const glm::vec2 b = glm::mix(control, join.fieldPosition, u);
        const glm::vec2 base = glm::mix(a, b, u);
        const float taper = smootherStep(ofClamp(u * 5.0f, 0.0f, 1.0f)) *
                            smootherStep(ofClamp((1.0f - u) * 5.0f, 0.0f, 1.0f));
        const float wobble = std::sin(u * PI * randomRange(rng, 0.8f, 1.5f) + randomRange(rng, 0.0f, TWO_PI)) *
                             std::max(0.35f, paramRiverWidth_ * 0.18f) * taper;
        RiverNode node;
        node.fieldPosition = base + normal * wobble;
        branch.nodes.push_back(node);
    }
    return branch;
}

bool RiverFormationLayer::findNeckCandidate(const RiverPath& path, int& startIndex, int& endIndex) const {
    if (path.nodes.size() < 40) return false;

    const int count = static_cast<int>(path.nodes.size());
    const int minSpacing = std::max(18, count / 10);
    const float minDistance = std::max(1.25f, paramRiverWidth_ * 1.05f);
    const float maxDistance = std::max(minDistance + 0.5f, paramRiverWidth_ * ofLerp(2.2f, 7.5f, ofClamp(paramBranchLength_ / 1.6f, 0.0f, 1.0f)));
    const float maxDistanceSq = maxDistance * maxDistance;
    float bestScore = std::numeric_limits<float>::max();
    int bestI = -1;
    int bestJ = -1;

    for (int i = 4; i < count - minSpacing - 4; ++i) {
        for (int j = i + minSpacing; j < count - 4; ++j) {
            const glm::vec2 delta = path.nodes[static_cast<std::size_t>(i)].fieldPosition - path.nodes[static_cast<std::size_t>(j)].fieldPosition;
            const float distSq = glm::dot(delta, delta);
            if (distSq > maxDistanceSq) continue;
            const float dist = std::sqrt(std::max(0.0f, distSq));
            if (dist < minDistance) continue;
            const float spacingScore = static_cast<float>(j - i) / static_cast<float>(count);
            const float score = dist + spacingScore * paramRiverWidth_ * 0.8f;
            if (score < bestScore) {
                bestScore = score;
                bestI = i;
                bestJ = j;
            }
        }
    }

    if (bestI < 0 || bestJ <= bestI + minSpacing) return false;
    startIndex = bestI;
    endIndex = bestJ;
    return true;
}

void RiverFormationLayer::simulateStep() {
    fadeFields();
    maybeSpawnBranch();

    if (!paths_.empty()) {
        RiverPath& main = paths_.front();
        migratePath(main);
        smoothPath(main);
        resamplePath(main, static_cast<int>(std::round(paramPathPoints_)));
        constrainPathProgress(main);
        updatePathGeometry(main);
        maybeApplyCutoff(main);
        resamplePath(main, static_cast<int>(std::round(paramPathPoints_)));
        constrainPathProgress(main);
        updatePathGeometry(main);
        main.age += 1.0f;

        if (paths_.size() > 1) {
            paths_.erase(std::remove_if(paths_.begin() + 1,
                                        paths_.end(),
                                        [&](const RiverPath& path) {
                                            return path.attachStartIndex < 0 ||
                                                   path.attachEndIndex >= static_cast<int>(main.nodes.size()) ||
                                                   path.attachEndIndex - path.attachStartIndex < 8;
                                        }),
                         paths_.end());
        }

        for (std::size_t i = 1; i < paths_.size(); ++i) {
            RiverPath& path = paths_[i];
            if (!path.nodes.empty() && path.attachStartIndex >= 0 && path.attachEndIndex >= 0 &&
                path.attachEndIndex < static_cast<int>(main.nodes.size())) {
                path.nodes.front().fieldPosition = main.nodes[static_cast<std::size_t>(path.attachStartIndex)].fieldPosition;
                path.nodes.back().fieldPosition = main.nodes[static_cast<std::size_t>(path.attachEndIndex)].fieldPosition;
            }
            migratePath(path);
            smoothPath(path);
            constrainPathProgress(path);
            const int targetCount = std::max(16, static_cast<int>(std::round(static_cast<float>(
                std::max(8, path.attachEndIndex - path.attachStartIndex)) * 0.85f)));
            resamplePath(path, targetCount);
            if (!path.nodes.empty() && path.attachStartIndex >= 0 && path.attachEndIndex >= 0 &&
                path.attachEndIndex < static_cast<int>(main.nodes.size())) {
                path.nodes.front().fieldPosition = main.nodes[static_cast<std::size_t>(path.attachStartIndex)].fieldPosition;
                path.nodes.back().fieldPosition = main.nodes[static_cast<std::size_t>(path.attachEndIndex)].fieldPosition;
            }
            constrainPathProgress(path);
            updatePathGeometry(path);
            path.age += 1.0f;
        }
    }

    const float abandonedMaxAge = ofLerp(80.0f, 1600.0f, 1.0f - ofClamp(paramOxbowDecay_ / 0.05f, 0.0f, 1.0f));
    for (auto& path : abandonedPaths_) {
        path.age += 1.0f;
    }
    abandonedPaths_.erase(std::remove_if(abandonedPaths_.begin(),
                                         abandonedPaths_.end(),
                                         [&](const RiverPath& path) {
                                             return path.age > abandonedMaxAge && abandonedPaths_.size() > 2;
                                         }),
                          abandonedPaths_.end());

    paintRiverEffects();
    syncMainCenterline();
    simAge_ += 1.0f;
    dirty_ = true;
}

void RiverFormationLayer::fadeFields() {
    for (auto& cell : cells_) {
        cell.activeWater = std::max(0.0f, cell.activeWater - paramTrailDecay_);
        cell.wetness = std::max(0.0f, cell.wetness - paramTrailDecay_ * 0.34f);
        cell.abandonedWater = std::max(0.0f, cell.abandonedWater - paramOxbowDecay_);
        cell.sediment = std::max(0.0f, cell.sediment - paramTrailDecay_ * 0.10f);
        cell.elevation = ofClamp(cell.elevation, 0.0f, 1.0f);
    }
}

void RiverFormationLayer::maybeSpawnBranch() {
    if (paths_.empty() || paramMaxBranches_ <= 0.0f) return;

    int branchCount = 0;
    for (const auto& path : paths_) {
        if (!path.mainStem) ++branchCount;
    }
    if (branchCount >= static_cast<int>(std::round(paramMaxBranches_))) return;

    const std::uint32_t salt = seedState_ + static_cast<std::uint32_t>(simAge_ * 977.0f) + static_cast<std::uint32_t>(branchCount * 131u);
    std::mt19937 rng(salt == 0 ? 1u : salt);
    if (randomUnit(rng) >= paramBranchChance_) return;

    const RiverPath& main = paths_.front();
    if (main.nodes.size() < 40) return;

    int startIndex = -1;
    int endIndex = -1;
    if (!findNeckCandidate(main, startIndex, endIndex)) return;

    paths_.push_back(buildBranchBetween(main, startIndex, endIndex, rng));
    updatePathGeometry(paths_.back());
}

void RiverFormationLayer::migratePath(RiverPath& path) {
    if (path.nodes.size() < 5) return;

    std::vector<RiverNode> next = path.nodes;
    const float marginY = std::max(2.0f, paramRiverWidth_ * 0.75f);
    const float centerY = static_cast<float>(fieldSize_.y) * 0.5f;
    const int count = static_cast<int>(path.nodes.size());
    const int firstEditable = path.mainStem ? 2 : 2;
    const int lastEditable = static_cast<int>(path.nodes.size()) - (path.mainStem ? 2 : 3);
    if (lastEditable <= firstEditable) return;

    std::vector<float> cumulative(static_cast<std::size_t>(count), 0.0f);
    std::vector<float> spacing(static_cast<std::size_t>(count), 1.0f);
    std::vector<float> nominalRate(static_cast<std::size_t>(count), 0.0f);

    for (int i = 1; i < count; ++i) {
        spacing[static_cast<std::size_t>(i)] = std::max(kMinSegmentLength,
                                                        glm::length(path.nodes[static_cast<std::size_t>(i)].fieldPosition -
                                                                    path.nodes[static_cast<std::size_t>(i - 1)].fieldPosition));
        cumulative[static_cast<std::size_t>(i)] = cumulative[static_cast<std::size_t>(i - 1)] + spacing[static_cast<std::size_t>(i)];
    }

    const float totalLength = std::max(kMinSegmentLength, cumulative.back());
    const float directLength = std::max(kMinSegmentLength, glm::length(path.nodes.back().fieldPosition - path.nodes.front().fieldPosition));
    const float sinuosityScale = std::pow(std::max(1.0f, totalLength / directLength), -2.0f / 3.0f);
    const float memoryDistance = std::max(1.0f, paramCurvatureMemory_ * std::max(1.0f, paramRiverWidth_));

    for (int i = 1; i + 1 < count; ++i) {
        const glm::vec2 prev = path.nodes[static_cast<std::size_t>(i - 1)].fieldPosition;
        const glm::vec2 current = path.nodes[static_cast<std::size_t>(i)].fieldPosition;
        const glm::vec2 nextPoint = path.nodes[static_cast<std::size_t>(i + 1)].fieldPosition;
        const glm::vec2 firstDerivative = (nextPoint - prev) * 0.5f;
        const glm::vec2 secondDerivative = nextPoint - current * 2.0f + prev;
        const float denom = std::pow(std::max(kMinSegmentLength, glm::dot(firstDerivative, firstDerivative)), 1.5f);
        const float curvature = cross2d(firstDerivative, secondDerivative) / denom;
        const float dimensionlessCurvature = path.nodes[static_cast<std::size_t>(i)].width * curvature;
        nominalRate[static_cast<std::size_t>(i)] = paramMigrationRate_ * paramErosionStrength_ * dimensionlessCurvature;
    }

    for (int i = firstEditable; i <= lastEditable; ++i) {
        const RiverNode& node = path.nodes[static_cast<std::size_t>(i)];
        const glm::vec2 tangent = safeNormalize(node.flowDirection);
        const glm::vec2 migrationNormal(tangent.y, -tangent.x);
        float weightedRate = 0.0f;
        float totalWeight = 0.0f;
        for (int j = i; j >= firstEditable; --j) {
            const float upstreamDistance = cumulative[static_cast<std::size_t>(i)] - cumulative[static_cast<std::size_t>(j)];
            const float weight = std::exp(-upstreamDistance / memoryDistance);
            weightedRate += nominalRate[static_cast<std::size_t>(j)] * weight;
            totalWeight += weight;
        }

        const float upstreamMean = totalWeight > 0.0f ? weightedRate / totalWeight : 0.0f;
        float migrationRate = (-nominalRate[static_cast<std::size_t>(i)] + 2.5f * upstreamMean) * sinuosityScale;
        if (!std::isfinite(migrationRate)) migrationRate = 0.0f;

        const RiverCell forwardBank = sampleCell(node.fieldPosition + migrationNormal * node.width);
        const RiverCell reverseBank = sampleCell(node.fieldPosition - migrationNormal * node.width);
        const float forwardSoftness = 1.0f - ofClamp(forwardBank.bankResistance * 0.72f + paramBankHardness_ * 0.28f, 0.0f, 0.96f);
        const float reverseSoftness = 1.0f - ofClamp(reverseBank.bankResistance * 0.72f + paramBankHardness_ * 0.28f, 0.0f, 0.96f);
        const float directionSign = migrationRate >= 0.0f ? 1.0f : -1.0f;
        const float softness = directionSign > 0.0f ? forwardSoftness : reverseSoftness;
        migrationRate *= softness;

        const float fixedBankBias = signedNoise(node.fieldPosition.x * 0.028f + static_cast<float>(seedState_) * 0.03f,
                                                node.fieldPosition.y * 0.043f,
                                                path.widthScale * 13.0f) * paramNoiseAmount_ * 0.075f;
        const float bankContrastBias = (forwardSoftness - reverseSoftness) * paramNoiseAmount_ * 0.065f;
        float displacement = migrationRate + fixedBankBias + bankContrastBias;
        const float localSpacing = std::max(1.0f, (spacing[static_cast<std::size_t>(i)] + spacing[static_cast<std::size_t>(i + 1)]) * 0.5f);
        const float maxDisplacement = std::max(0.05f, localSpacing * paramStabilityClamp_);
        displacement = ofClamp(displacement, -maxDisplacement, maxDisplacement);

        glm::vec2 delta = migrationNormal * displacement;
        delta.y += (centerY - node.fieldPosition.y) * paramValleyConfinement_ * (path.mainStem ? 0.004f : 0.002f);
        next[static_cast<std::size_t>(i)].fieldPosition += delta;
        next[static_cast<std::size_t>(i)].fieldPosition.y = ofClamp(next[static_cast<std::size_t>(i)].fieldPosition.y,
                                                                    marginY,
                                                                    static_cast<float>(fieldSize_.y) - 1.0f - marginY);
    }

    path.nodes = std::move(next);
}

void RiverFormationLayer::smoothPath(RiverPath& path) {
    if (path.nodes.size() < 3 || paramMeanderSmoothing_ <= 0.0f) return;

    const float amount = paramMeanderSmoothing_;
    const int passes = path.mainStem ? 1 : 2;
    for (int pass = 0; pass < passes; ++pass) {
        std::vector<RiverNode> smoothed = path.nodes;
        for (std::size_t i = 1; i + 1 < path.nodes.size(); ++i) {
            const glm::vec2 average = (path.nodes[i - 1].fieldPosition + path.nodes[i].fieldPosition + path.nodes[i + 1].fieldPosition) / 3.0f;
            smoothed[i].fieldPosition = glm::mix(path.nodes[i].fieldPosition, average, amount);
        }
        path.nodes = std::move(smoothed);
    }

    if (path.mainStem) return;

    const float maxStep = std::max(1.25f, std::min(3.0f, paramRiverWidth_ * 0.55f));
    for (std::size_t i = 1; i < path.nodes.size(); ++i) {
        const float previousY = path.nodes[i - 1].fieldPosition.y;
        path.nodes[i].fieldPosition.y = ofClamp(path.nodes[i].fieldPosition.y, previousY - maxStep, previousY + maxStep);
    }
    for (std::size_t i = path.nodes.size() - 1; i > 0; --i) {
        const float nextY = path.nodes[i].fieldPosition.y;
        path.nodes[i - 1].fieldPosition.y = ofClamp(path.nodes[i - 1].fieldPosition.y, nextY - maxStep, nextY + maxStep);
    }
}

void RiverFormationLayer::constrainPathProgress(RiverPath& path) {
    if (path.nodes.size() < 2) return;

    const float startX = path.nodes.front().fieldPosition.x;
    const float endX = path.nodes.back().fieldPosition.x;
    const float offscreen = std::max(16.0f, paramRiverWidth_ * 4.0f);
    const float marginY = std::max(2.0f, paramRiverWidth_ * 0.75f);
    for (std::size_t i = 0; i < path.nodes.size(); ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(path.nodes.size() - 1);
        if (path.mainStem) {
            path.nodes[i].fieldPosition.x = ofClamp(path.nodes[i].fieldPosition.x,
                                                    -offscreen,
                                                    static_cast<float>(fieldSize_.x - 1) + offscreen);
            if (i < 2 || i + 2 >= path.nodes.size()) {
                const float targetX = ofLerp(startX, endX, u);
                path.nodes[i].fieldPosition.x = ofLerp(path.nodes[i].fieldPosition.x, targetX, 0.85f);
            }
        } else {
            const float targetX = ofLerp(startX, endX, u);
            path.nodes[i].fieldPosition.x = ofLerp(path.nodes[i].fieldPosition.x, targetX, 0.65f);
        }
        path.nodes[i].fieldPosition.y = ofClamp(path.nodes[i].fieldPosition.y,
                                                marginY,
                                                static_cast<float>(fieldSize_.y) - 1.0f - marginY);
    }
}

void RiverFormationLayer::resamplePath(RiverPath& path, int targetCount) {
    if (path.nodes.size() < 2) return;

    targetCount = std::max(8, targetCount);
    std::vector<float> cumulative(path.nodes.size(), 0.0f);
    for (std::size_t i = 1; i < path.nodes.size(); ++i) {
        cumulative[i] = cumulative[i - 1] + glm::length(path.nodes[i].fieldPosition - path.nodes[i - 1].fieldPosition);
    }
    const float totalLength = cumulative.back();
    if (totalLength <= kMinSegmentLength) return;

    std::vector<RiverNode> resampled;
    resampled.reserve(static_cast<std::size_t>(targetCount));
    std::size_t segment = 1;
    for (int i = 0; i < targetCount; ++i) {
        const float targetDistance = totalLength * static_cast<float>(i) / static_cast<float>(targetCount - 1);
        while (segment + 1 < cumulative.size() && cumulative[segment] < targetDistance) {
            ++segment;
        }
        const float before = cumulative[segment - 1];
        const float after = cumulative[segment];
        const float denom = std::max(kMinSegmentLength, after - before);
        const float localT = ofClamp((targetDistance - before) / denom, 0.0f, 1.0f);
        RiverNode node;
        node.fieldPosition = glm::mix(path.nodes[segment - 1].fieldPosition, path.nodes[segment].fieldPosition, localT);
        resampled.push_back(node);
    }
    path.nodes = std::move(resampled);
}

void RiverFormationLayer::updatePathGeometry(RiverPath& path) {
    if (path.nodes.empty()) return;

    for (std::size_t i = 0; i < path.nodes.size(); ++i) {
        const glm::vec2 prev = i == 0 ? path.nodes[i].fieldPosition : path.nodes[i - 1].fieldPosition;
        const glm::vec2 next = i + 1 >= path.nodes.size() ? path.nodes[i].fieldPosition : path.nodes[i + 1].fieldPosition;
        const glm::vec2 flow = safeNormalize(next - prev);
        RiverNode& node = path.nodes[i];
        node.flowDirection = flow;

        const float u = path.nodes.size() > 1 ? static_cast<float>(i) / static_cast<float>(path.nodes.size() - 1) : 0.0f;
        const float downstream = path.mainStem ? ofLerp(0.82f, 1.24f, u) : ofLerp(1.0f, 0.38f, smootherStep(u));
        const float coarseVariation = signedNoise(node.fieldPosition.x * 0.035f + static_cast<float>(seedState_) * 0.011f,
                                                  node.fieldPosition.y * 0.045f,
                                                  path.widthScale * 19.0f);
        const float fineVariation = signedNoise(node.fieldPosition.x * 0.115f - static_cast<float>(seedState_) * 0.007f,
                                                node.fieldPosition.y * 0.090f,
                                                path.widthScale * 37.0f);
        const float variation = coarseVariation * 0.76f + fineVariation * 0.24f;
        const float pulse = 1.0f + paramWidthPulse_ * 0.08f * std::sin(u * TWO_PI * 2.0f + static_cast<float>(seedState_) * 0.0007f);
        node.width = std::max(0.65f, paramRiverWidth_ * path.widthScale * downstream * pulse * (1.0f + variation * paramWidthVariation_ * 0.45f));

        if (i == 0 || i + 1 >= path.nodes.size()) {
            node.curvature = 0.0f;
            continue;
        }
        const glm::vec2 prevDir = safeNormalize(path.nodes[i].fieldPosition - path.nodes[i - 1].fieldPosition, flow);
        const glm::vec2 nextDir = safeNormalize(path.nodes[i + 1].fieldPosition - path.nodes[i].fieldPosition, flow);
        node.curvature = ofClamp(cross2d(prevDir, nextDir), -1.0f, 1.0f);
    }
}

void RiverFormationLayer::maybeApplyCutoff(RiverPath& path) {
    if (!path.mainStem || paramCutoffFactor_ <= 0.0f || path.nodes.size() < 40) {
        return;
    }
    if (cutoffCooldown_ > 0) {
        --cutoffCooldown_;
        return;
    }

    const float threshold = std::max(2.0f, paramRiverWidth_ * paramCutoffFactor_);
    const float thresholdSq = threshold * threshold;
    const int count = static_cast<int>(path.nodes.size());
    const int minSpacing = std::max(12, count / 14);
    int bestI = -1;
    int bestJ = -1;
    float bestDistSq = thresholdSq;
    glm::vec2 bestCutPoint{ 0.0f, 0.0f };

    for (int i = 3; i < count - minSpacing - 3; ++i) {
        const glm::vec2 a0 = path.nodes[static_cast<std::size_t>(i)].fieldPosition;
        const glm::vec2 a1 = path.nodes[static_cast<std::size_t>(i + 1)].fieldPosition;
        for (int j = i + minSpacing; j < count - 3; ++j) {
            const glm::vec2 b0 = path.nodes[static_cast<std::size_t>(j)].fieldPosition;
            const glm::vec2 b1 = path.nodes[static_cast<std::size_t>(j + 1)].fieldPosition;
            const SegmentApproach approach = closestSegmentApproach(a0, a1, b0, b1);
            if (approach.distanceSq >= bestDistSq) continue;
            bestDistSq = approach.distanceSq;
            bestI = i + (approach.firstT > 0.5f ? 1 : 0);
            bestJ = j + (approach.secondT > 0.5f ? 1 : 0);
            bestCutPoint = (approach.first + approach.second) * 0.5f;
        }
    }

    if (bestI < 0 || bestJ <= bestI + minSpacing) {
        cutoffCooldown_ = 4;
        return;
    }

    RiverPath abandoned;
    abandoned.widthScale = path.widthScale * 0.82f;
    abandoned.nodes.reserve(static_cast<std::size_t>(bestJ - bestI + 1));
    for (int i = bestI; i <= bestJ; ++i) {
        abandoned.nodes.push_back(path.nodes[static_cast<std::size_t>(i)]);
    }
    abandonedPaths_.push_back(std::move(abandoned));
    if (abandonedPaths_.size() > 24) {
        abandonedPaths_.erase(abandonedPaths_.begin());
    }

    std::vector<RiverNode> shortcut;
    shortcut.reserve(path.nodes.size());
    for (int i = 0; i <= bestI; ++i) {
        shortcut.push_back(path.nodes[static_cast<std::size_t>(i)]);
    }
    if (std::isfinite(bestCutPoint.x) && std::isfinite(bestCutPoint.y)) {
        RiverNode cutNode = path.nodes[static_cast<std::size_t>(bestI)];
        cutNode.fieldPosition = bestCutPoint;
        shortcut.push_back(cutNode);
    }
    for (int i = bestJ; i < count; ++i) {
        shortcut.push_back(path.nodes[static_cast<std::size_t>(i)]);
    }
    path.nodes = std::move(shortcut);
    resamplePath(path, static_cast<int>(std::round(paramPathPoints_)));
    cutoffPulse_ = 1.0f;
    cutoffCooldown_ = std::max(18, count / 5);
}

void RiverFormationLayer::paintRiverEffects() {
    for (const auto& path : paths_) {
        paintPath(path, true);
    }
    for (const auto& path : abandonedPaths_) {
        paintPath(path, false);
    }
}

void RiverFormationLayer::paintPath(const RiverPath& path, bool active) {
    if (path.nodes.size() < 2) return;

    const float abandonedMaxAge = ofLerp(80.0f, 1600.0f, 1.0f - ofClamp(paramOxbowDecay_ / 0.05f, 0.0f, 1.0f));
    const float abandonedStrength = active ? 1.0f : 1.0f - smootherStep(ofClamp(path.age / std::max(1.0f, abandonedMaxAge), 0.0f, 1.0f));
    if (!active && abandonedStrength <= 0.001f) return;

    for (std::size_t i = 1; i < path.nodes.size(); ++i) {
        const RiverNode& a = path.nodes[i - 1];
        const RiverNode& b = path.nodes[i];
        const glm::vec2 delta = b.fieldPosition - a.fieldPosition;
        const float length = glm::length(delta);
        const int samples = std::max(1, static_cast<int>(std::ceil(length / std::max(1.0f, paramRiverWidth_ * 0.28f))));
        for (int s = 0; s <= samples; ++s) {
            const float t = static_cast<float>(s) / static_cast<float>(samples);
            const glm::vec2 p = glm::mix(a.fieldPosition, b.fieldPosition, t);
            const float width = ofLerp(a.width, b.width, t);
            if (active) {
                paintDisk(p,
                          width * 0.58f,
                          -0.008f * paramChannelDepth_,
                          -0.001f,
                          0.72f,
                          1.0f,
                          0.0f,
                          0.35f);
                if (paramGlowAmount_ > 0.0f) {
                    paintDisk(p,
                              width * (1.0f + paramGlowAmount_ * 1.8f),
                              0.0f,
                              0.0f,
                              0.22f * paramGlowAmount_,
                              0.0f,
                              0.0f,
                              0.60f);
                }
            } else {
                paintDisk(p,
                          width * (0.32f + abandonedStrength * 0.14f),
                          0.0008f,
                          0.0025f * abandonedStrength,
                          0.0f,
                          0.0f,
                          0.68f * abandonedStrength,
                          0.40f);
            }
        }
    }

    if (!active) return;

    for (std::size_t i = 1; i + 1 < path.nodes.size(); ++i) {
        const RiverNode& node = path.nodes[i];
        const float bend = smootherStep(ofClamp(std::abs(node.curvature) * 6.0f, 0.0f, 1.0f));
        if (bend <= 0.01f) continue;

        const glm::vec2 tangent = safeNormalize(node.flowDirection);
        const glm::vec2 leftNormal(-tangent.y, tangent.x);
        const glm::vec2 inside = leftNormal * (node.curvature >= 0.0f ? 1.0f : -1.0f);
        const glm::vec2 outside = -inside;
        paintDisk(node.fieldPosition + outside * node.width * 0.68f,
                  node.width * 0.58f,
                  -0.014f * paramErosionStrength_ * bend,
                  -0.0018f * bend,
                  0.0f,
                  0.0f,
                  0.0f,
                  1.0f);
        paintDisk(node.fieldPosition + inside * node.width * 0.50f,
                  node.width * 0.70f,
                  0.006f * paramDepositionStrength_ * bend,
                  0.016f * paramDepositionStrength_ * bend,
                  0.0f,
                  0.0f,
                  0.0f,
                  0.55f);
    }
}

void RiverFormationLayer::drawPathPolyline(const RiverPath& path,
                                           const LayerDrawParams& params,
                                           float alpha,
                                           float lineWidth) const {
    if (path.nodes.size() < 2 || alpha <= 0.0f) return;

    ofNoFill();
    ofSetLineWidth(lineWidth);
    ofBeginShape();
    for (const auto& node : path.nodes) {
        const float sx = ofMap(node.fieldPosition.x,
                               0.0f,
                               static_cast<float>(fieldSize_.x - 1),
                               0.0f,
                               static_cast<float>(params.viewport.x));
        const float sy = ofMap(node.fieldPosition.y,
                               0.0f,
                               static_cast<float>(fieldSize_.y - 1),
                               0.0f,
                               static_cast<float>(params.viewport.y));
        ofVertex(sx, sy);
    }
    ofEndShape(false);
}

void RiverFormationLayer::paintDisk(const glm::vec2& center,
                                    float radius,
                                    float elevationDelta,
                                    float sedimentDelta,
                                    float wetnessAmount,
                                    float activeWaterAmount,
                                    float abandonedWaterAmount,
                                    float bankResistanceScale) {
    if (radius <= 0.0f || cells_.empty()) return;

    const int minX = std::max(0, static_cast<int>(std::floor(center.x - radius - 1.0f)));
    const int maxX = std::min(fieldSize_.x - 1, static_cast<int>(std::ceil(center.x + radius + 1.0f)));
    const int minY = std::max(0, static_cast<int>(std::floor(center.y - radius - 1.0f)));
    const int maxY = std::min(fieldSize_.y - 1, static_cast<int>(std::ceil(center.y + radius + 1.0f)));
    if (minX > maxX || minY > maxY) return;

    const float radiusSq = radius * radius;
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const glm::vec2 p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const glm::vec2 delta = p - center;
            const float distSq = glm::dot(delta, delta);
            if (distSq > radiusSq) continue;
            const float dist = std::sqrt(std::max(0.0f, distSq));
            const float falloff = smootherStep(1.0f - dist / std::max(0.001f, radius));
            RiverCell& cell = cellAt(x, y);
            const float resistance = ofClamp(cell.bankResistance * bankResistanceScale + paramBankHardness_ * (1.0f - bankResistanceScale), 0.0f, 1.0f);
            const float erosionScale = elevationDelta < 0.0f ? (1.0f - resistance * 0.72f) : 1.0f;
            cell.elevation = ofClamp(cell.elevation + elevationDelta * falloff * erosionScale, 0.0f, 1.0f);
            cell.sediment = ofClamp(cell.sediment + sedimentDelta * falloff, 0.0f, 1.0f);
            cell.wetness = std::max(cell.wetness, wetnessAmount * falloff);
            cell.activeWater = std::max(cell.activeWater, activeWaterAmount * falloff);
            cell.abandonedWater = std::max(cell.abandonedWater, abandonedWaterAmount * falloff);
        }
    }
}

void RiverFormationLayer::refreshTexture() {
    if (!pixels_.isAllocated() || cells_.empty()) return;

    for (int y = 0; y < fieldSize_.y; ++y) {
        for (int x = 0; x < fieldSize_.x; ++x) {
            const RiverCell& cell = cellAt(x, y);
            const float active = ofClamp(cell.activeWater * paramTrailBoost_, 0.0f, 1.0f);
            const float history = ofClamp(cell.abandonedWater * paramOxbowAlpha_, 0.0f, 1.0f);
            const float activeInk = std::pow(active, 1.85f);
            const float historyInk = std::pow(history, 1.35f) * 0.35f;

            float alpha = std::max(activeInk, historyInk);
            if (alpha < paramMaskThreshold_) {
                alpha = 0.0f;
            } else {
                alpha = ofMap(alpha, paramMaskThreshold_, 1.0f, 0.0f, 1.0f, true);
            }

            pixels_.setColor(x, y, ofFloatColor(ofClamp(paramColorR_, 0.0f, 1.0f),
                                                ofClamp(paramColorG_, 0.0f, 1.0f),
                                                ofClamp(paramColorB_, 0.0f, 1.0f),
                                                ofClamp(alpha * paramTrailAlpha_, 0.0f, 1.0f)));
        }
    }

    texture_.loadData(pixels_);
}

void RiverFormationLayer::triggerReset() {
    resetSimulation();
    dirty_ = true;
}

void RiverFormationLayer::syncMainCenterline() {
    if (!paths_.empty()) {
        centerline_ = paths_.front().nodes;
    } else {
        centerline_.clear();
    }
}

float RiverFormationLayer::stepRateFor(const LayerUpdateParams& params) const {
    if (paramBpmSync_) {
        return std::max(0.0f, params.bpm / 60.0f) * std::max(0.25f, paramBpmMultiplier_);
    }
    return std::max(0.0f, paramSpeed_);
}

float RiverFormationLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) return 0.0f;
    return std::max(0.0f, timeSeconds) * (bpm / 60.0f);
}

float RiverFormationLayer::renderSignature() const {
    float signature = paramTrailBoost_ * 3.1f + paramTrailAlpha_ * 5.3f + paramOxbowAlpha_ * 7.1f;
    signature += paramGlowAmount_ * 11.0f + paramMaskThreshold_ * 13.0f;
    signature += paramColorR_ * 17.0f + paramColorG_ * 19.0f + paramColorB_ * 23.0f;
    return signature;
}

int RiverFormationLayer::indexFor(int x, int y) const {
    return y * fieldSize_.x + x;
}

RiverFormationLayer::RiverCell& RiverFormationLayer::cellAt(int x, int y) {
    return cells_[static_cast<std::size_t>(indexFor(x, y))];
}

const RiverFormationLayer::RiverCell& RiverFormationLayer::cellAt(int x, int y) const {
    return cells_[static_cast<std::size_t>(indexFor(x, y))];
}

RiverFormationLayer::RiverCell RiverFormationLayer::sampleCell(const glm::vec2& fieldPos) const {
    if (cells_.empty()) return {};

    const float fx = ofClamp(fieldPos.x, 0.0f, static_cast<float>(fieldSize_.x - 1));
    const float fy = ofClamp(fieldPos.y, 0.0f, static_cast<float>(fieldSize_.y - 1));
    const int x0 = static_cast<int>(std::floor(fx));
    const int y0 = static_cast<int>(std::floor(fy));
    const int x1 = std::min(fieldSize_.x - 1, x0 + 1);
    const int y1 = std::min(fieldSize_.y - 1, y0 + 1);
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    const RiverCell& c00 = cellAt(x0, y0);
    const RiverCell& c10 = cellAt(x1, y0);
    const RiverCell& c01 = cellAt(x0, y1);
    const RiverCell& c11 = cellAt(x1, y1);

    auto bilerp = [&](float RiverCell::*member) {
        const float a = ofLerp(c00.*member, c10.*member, tx);
        const float b = ofLerp(c01.*member, c11.*member, tx);
        return ofLerp(a, b, ty);
    };

    RiverCell out;
    out.elevation = bilerp(&RiverCell::elevation);
    out.sediment = bilerp(&RiverCell::sediment);
    out.wetness = bilerp(&RiverCell::wetness);
    out.activeWater = bilerp(&RiverCell::activeWater);
    out.abandonedWater = bilerp(&RiverCell::abandonedWater);
    out.bankResistance = bilerp(&RiverCell::bankResistance);
    return out;
}

float RiverFormationLayer::noiseSample(float x, float y, float z) const {
    return ofNoise(x, y, z);
}

float RiverFormationLayer::signedNoise(float x, float y, float z) const {
    return noiseSample(x, y, z) * 2.0f - 1.0f;
}

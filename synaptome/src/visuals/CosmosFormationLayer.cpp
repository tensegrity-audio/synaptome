#include "CosmosFormationLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

namespace {
    constexpr float kMinDt = 0.0f;
    constexpr float kMaxDt = 1.0f / 20.0f;
    constexpr int kGrowthCols = 96;
    constexpr int kGrowthRows = 54;
    constexpr float kGrowthExtent = 1.24f;

    float wrap01(float value) {
        while (value < 0.0f) value += 1.0f;
        while (value >= 1.0f) value -= 1.0f;
        return value;
    }

    float smoothStep(float edge0, float edge1, float value) {
        const float t = ofClamp((value - edge0) / std::max(0.0001f, edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    float bell(float value, float center, float width) {
        const float normalized = (value - center) / std::max(0.0001f, width);
        return std::exp(-normalized * normalized);
    }

    float spiralBand(float phase, float sharpness) {
        return std::pow(ofClamp(0.5f + 0.5f * std::cos(phase), 0.0f, 1.0f), sharpness);
    }

    float gaussianish(std::mt19937& rng) {
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        return (unit(rng) + unit(rng) + unit(rng) + unit(rng) - 2.0f) * 0.5f;
    }

    glm::vec2 safeNormalize(const glm::vec2& value, const glm::vec2& fallback = glm::vec2(1.0f, 0.0f)) {
        const float len = glm::length(value);
        if (len <= 0.0001f) {
            return fallback;
        }
        return value / len;
    }

    glm::vec2 rotateVec(const glm::vec2& value, float radians) {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        return glm::vec2(value.x * c - value.y * s, value.x * s + value.y * c);
    }

    glm::vec2 domainWarp(const glm::vec2& value, std::uint32_t seed, float amount) {
        const float nx = ofNoise(value.x * 1.55f + seed * 0.0021f,
                                 value.y * 1.55f - seed * 0.0017f) - 0.5f;
        const float ny = ofNoise(value.x * 1.37f - seed * 0.0013f + 41.0f,
                                 value.y * 1.37f + seed * 0.0025f - 17.0f) - 0.5f;
        return value + glm::vec2(nx, ny) * amount;
    }

    glm::vec2 organicFilamentPoint(const glm::vec2& a,
                                   const glm::vec2& b,
                                   const glm::vec2& bend,
                                   float t,
                                   int edgeA,
                                   int edgeB,
                                   std::uint32_t seed) {
        const glm::vec2 control = (a + b) * 0.5f + bend;
        const glm::vec2 ab = b - a;
        const float length = glm::length(ab);
        const glm::vec2 tangent = safeNormalize(ab);
        const glm::vec2 normal(-tangent.y, tangent.x);
        const glm::vec2 first = a * (1.0f - t) + control * t;
        const glm::vec2 second = control * (1.0f - t) + b * t;
        glm::vec2 p = first * (1.0f - t) + second * t;

        const float key = static_cast<float>((edgeA + 1) * 29 + (edgeB + 3) * 71) + seed * 0.0037f;
        const float envelope = std::sin(ofClamp(t, 0.0f, 1.0f) * PI);
        const float primary = std::sin((t * 2.25f + key * 0.071f) * TWO_PI);
        const float secondary = std::sin((t * 5.10f + key * 0.037f) * TWO_PI);
        const float tertiary = ofNoise(t * 3.2f + key * 0.011f, seed * 0.002f) - 0.5f;
        const float normalWarp = (primary * 0.46f + secondary * 0.18f + tertiary * 0.42f) *
                                 envelope * ofClamp(length * 0.12f, 0.010f, 0.085f);
        const float tangentWarp = std::sin((t * 3.6f + key * 0.049f) * TWO_PI) *
                                  envelope * ofClamp(length * 0.030f, 0.002f, 0.024f);
        return p + normal * normalWarp + tangent * tangentWarp;
    }

    float followAmount(float smoothing) {
        return 1.0f - ofClamp(smoothing, 0.0f, 0.98f);
    }

    ofFloatColor colorFrom(float r, float g, float b, float a) {
        return ofFloatColor(ofClamp(r, 0.0f, 1.0f),
                            ofClamp(g, 0.0f, 1.0f),
                            ofClamp(b, 0.0f, 1.0f),
                            ofClamp(a, 0.0f, 1.0f));
    }

    void setColor(const ofFloatColor& color) {
        ofSetColor(static_cast<int>(ofClamp(color.r, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.g, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.b, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(color.a, 0.0f, 1.0f) * 255.0f));
    }
}

void CosmosFormationLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramParticleCount_ = def.value("particleCount", paramParticleCount_);
    paramClusterCount_ = def.value("clusterCount", paramClusterCount_);
    paramRadius_ = def.value("radius", paramRadius_);
    paramOriginRadius_ = def.value("originRadius", paramOriginRadius_);
    paramExpansionRate_ = def.value("expansionRate", paramExpansionRate_);
    paramExpansionForce_ = def.value("expansionForce", paramExpansionForce_);
    paramAutoAdvance_ = def.value("autoAdvance", paramAutoAdvance_);
    paramFormationAge_ = def.value("formationAge", paramFormationAge_);
    paramFormationTime_ = def.value("formationTime", paramFormationTime_);
    paramEvolutionSpeed_ = def.value("evolutionSpeed", paramEvolutionSpeed_);
    paramGravity_ = def.value("gravity", paramGravity_);
    paramGravityDelay_ = def.value("gravityDelay", paramGravityDelay_);
    paramGravityEmergence_ = def.value("gravityEmergence", paramGravityEmergence_);
    paramClusterSwirl_ = def.value("clusterSwirl", paramClusterSwirl_);
    paramClusterSpread_ = def.value("clusterSpread", paramClusterSpread_);
    paramClusterDrift_ = def.value("clusterDrift", paramClusterDrift_);
    paramClusterSoftness_ = def.value("clusterSoftness", paramClusterSoftness_);
    paramShear_ = def.value("shear", paramShear_);
    paramVoidPressure_ = def.value("voidPressure", paramVoidPressure_);
    paramTurbulence_ = def.value("turbulence", paramTurbulence_);
    paramCoolingRate_ = def.value("coolingRate", paramCoolingRate_);
    paramFieldLuminosity_ = def.value("fieldLuminosity", paramFieldLuminosity_);
    paramGlowPersistence_ = def.value("glowPersistence", paramGlowPersistence_);
    paramFilamentMemory_ = def.value("filamentMemory", paramFilamentMemory_);
    paramMatterSize_ = def.value("matterSize", paramMatterSize_);
    paramMatterGlow_ = def.value("matterGlow", paramMatterGlow_);
    paramTrailAlpha_ = def.value("trailAlpha", paramTrailAlpha_);
    paramTrailThickness_ = def.value("trailThickness", paramTrailThickness_);
    paramHaloAlpha_ = def.value("haloAlpha", paramHaloAlpha_);
    paramHaloRadius_ = def.value("haloRadius", paramHaloRadius_);
    paramShockwaveCount_ = def.value("shockwaveCount", paramShockwaveCount_);
    paramShockwaveAlpha_ = def.value("shockwaveAlpha", paramShockwaveAlpha_);
    paramShockwaveWidth_ = def.value("shockwaveWidth", paramShockwaveWidth_);
    paramShockwaveSpeed_ = def.value("shockwaveSpeed", paramShockwaveSpeed_);
    paramPressureAmount_ = def.value("pressureAmount", paramPressureAmount_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramAudioAccretion_ = def.value("audioAccretion", paramAudioAccretion_);
    paramAudioGlow_ = def.value("audioGlow", paramAudioGlow_);
    paramAudioTwinkle_ = def.value("audioTwinkle", paramAudioTwinkle_);
    paramBassExpansion_ = def.value("bassExpansion", paramBassExpansion_);
    paramMidsTurbulence_ = def.value("midsTurbulence", paramMidsTurbulence_);
    paramHighsSparkle_ = def.value("highsSparkle", paramHighsSparkle_);
    paramWaveformWarp_ = def.value("waveformWarp", paramWaveformWarp_);
    paramPeakBangThreshold_ = def.value("peakBangThreshold", paramPeakBangThreshold_);
    paramPeakImpulse_ = def.value("peakImpulse", paramPeakImpulse_);
    paramBeatImpulse_ = def.value("beatImpulse", paramBeatImpulse_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramSeed_ = def.value("seed", paramSeed_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    paramBgR_ = def.value("bgR", paramBgR_);
    paramBgG_ = def.value("bgG", paramBgG_);
    paramBgB_ = def.value("bgB", paramBgB_);
    paramHotR_ = def.value("hotR", paramHotR_);
    paramHotG_ = def.value("hotG", paramHotG_);
    paramHotB_ = def.value("hotB", paramHotB_);
    paramMatterR_ = def.value("matterR", paramMatterR_);
    paramMatterG_ = def.value("matterG", paramMatterG_);
    paramMatterB_ = def.value("matterB", paramMatterB_);
    paramCoolR_ = def.value("coolR", paramCoolR_);
    paramCoolG_ = def.value("coolG", paramCoolG_);
    paramCoolB_ = def.value("coolB", paramCoolB_);
    paramClusterR_ = def.value("clusterR", paramClusterR_);
    paramClusterG_ = def.value("clusterG", paramClusterG_);
    paramClusterB_ = def.value("clusterB", paramClusterB_);
    paramWaveR_ = def.value("waveR", paramWaveR_);
    paramWaveG_ = def.value("waveG", paramWaveG_);
    paramWaveB_ = def.value("waveB", paramWaveB_);
    readColor(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(def, "hotColor", paramHotR_, paramHotG_, paramHotB_);
    readColor(def, "matterColor", paramMatterR_, paramMatterG_, paramMatterB_);
    readColor(def, "coolColor", paramCoolR_, paramCoolG_, paramCoolB_);
    readColor(def, "clusterColor", paramClusterR_, paramClusterG_, paramClusterB_);
    readColor(def, "waveColor", paramWaveR_, paramWaveG_, paramWaveB_);
    clampParams();
}

void CosmosFormationLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = (registryPrefix().empty() || registryPrefix() == "layer")
        ? "generative.cosmosFormation"
        : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Cosmos Formation";
    meta.label = "Layer: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Alpha: Cosmos", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".particleCount", &paramParticleCount_, paramParticleCount_, "Count: Matter Points", 96.0f, 3200.0f, 1.0f);
    registerFloat(registry, prefix + ".clusterCount", &paramClusterCount_, paramClusterCount_, "Count: Web Seeds", 4.0f, 128.0f, 1.0f);
    registerFloat(registry, prefix + ".radius", &paramRadius_, paramRadius_, "Scale: Cosmos", 0.25f, 2.4f, 0.01f);
    registerFloat(registry, prefix + ".originRadius", &paramOriginRadius_, paramOriginRadius_, "Scale: Singularity", 0.0f, 0.12f, 0.001f);
    registerFloat(registry, prefix + ".expansionRate", &paramExpansionRate_, paramExpansionRate_, "Time: Cosmic Rate", 0.05f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".expansionForce", &paramExpansionForce_, paramExpansionForce_, "Force: Expansion", 0.0f, 2.0f, 0.01f);

    meta = {};
    meta.group = "Cosmos Formation";
    meta.label = "Action: Auto Advance";
    meta.description = "Advance the formation lifecycle automatically after each bang.";
    registry.addBool(prefix + ".autoAdvance", &paramAutoAdvance_, paramAutoAdvance_, meta);

    registerFloat(registry, prefix + ".formationAge", &paramFormationAge_, paramFormationAge_, "Time: Formation Age", 0.0f, 1.0f, 0.001f,
                  "Normalized lifecycle: 0 is singular burst, 1 is mature cosmic web.");
    registerFloat(registry, prefix + ".formationTime", &paramFormationTime_, paramFormationTime_, "Time: Formation Duration", 1.0f, 300.0f, 0.1f);
    registerFloat(registry, prefix + ".evolutionSpeed", &paramEvolutionSpeed_, paramEvolutionSpeed_, "Time: Evolution Speed", 0.0f, 1.5f, 0.001f,
                  "Master time scale for web drift, field evolution, and matter transport.");
    registerFloat(registry, prefix + ".gravity", &paramGravity_, paramGravity_, "Force: Gravity", 0.0f, 2.5f, 0.01f);
    registerFloat(registry, prefix + ".gravityDelay", &paramGravityDelay_, paramGravityDelay_, "Time: Gravity Delay", 0.0f, 0.95f, 0.01f,
                  "Lifecycle position before web-node attraction becomes strong.");
    registerFloat(registry, prefix + ".gravityEmergence", &paramGravityEmergence_, paramGravityEmergence_, "Time: Gravity Emergence", 0.0f, 1.5f, 0.01f,
                  "How quickly latent web nodes become real gravitational wells.");
    registerFloat(registry, prefix + ".clusterSwirl", &paramClusterSwirl_, paramClusterSwirl_, "Motion: Node Swirl", -4.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".clusterSpread", &paramClusterSpread_, paramClusterSpread_, "Scale: Web Spread", 0.1f, 1.2f, 0.01f);
    registerFloat(registry, prefix + ".clusterDrift", &paramClusterDrift_, paramClusterDrift_, "Motion: Web Drift", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".clusterSoftness", &paramClusterSoftness_, paramClusterSoftness_, "Scale: Node Softness", 0.15f, 2.0f, 0.01f,
                  "Higher values make web nodes broader and less point-like.");
    registerFloat(registry, prefix + ".shear", &paramShear_, paramShear_, "Force: Shear", -2.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".voidPressure", &paramVoidPressure_, paramVoidPressure_, "Force: Void Pressure", 0.0f, 1.0f, 0.01f,
                  "Residual expansion that keeps matter from collapsing into fixed points.");
    registerFloat(registry, prefix + ".turbulence", &paramTurbulence_, paramTurbulence_, "Motion: Matter Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".coolingRate", &paramCoolingRate_, paramCoolingRate_, "Time: Cooling Rate", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldLuminosity", &paramFieldLuminosity_, paramFieldLuminosity_, "Glow: Field Luminosity", 0.0f, 4.0f, 0.01f,
                  "How strongly dense compressed field regions emit visible glow.");
    registerFloat(registry, prefix + ".glowPersistence", &paramGlowPersistence_, paramGlowPersistence_, "Glow: Persistence", 0.0f, 0.98f, 0.01f,
                  "How long dense field glow and audio excitation linger.");
    registerFloat(registry, prefix + ".filamentMemory", &paramFilamentMemory_, paramFilamentMemory_, "Time: Filament Memory", 0.0f, 0.995f, 0.001f,
                  "How long activated filaments remain stable after matter and audio move through them.");
    registerFloat(registry, prefix + ".matterSize", &paramMatterSize_, paramMatterSize_, "Scale: Matter Size", 0.25f, 8.0f, 0.05f);
    registerFloat(registry, prefix + ".matterGlow", &paramMatterGlow_, paramMatterGlow_, "Glow: Matter", 0.0f, 8.0f, 0.1f);
    registerFloat(registry, prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, "Alpha: Motion Trail", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailThickness", &paramTrailThickness_, paramTrailThickness_, "Scale: Motion Trail", 0.25f, 8.0f, 0.05f);
    registerFloat(registry, prefix + ".haloAlpha", &paramHaloAlpha_, paramHaloAlpha_, "Alpha: Web Field", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".haloRadius", &paramHaloRadius_, paramHaloRadius_, "Scale: Node Atmosphere", 0.02f, 0.45f, 0.001f);
    registerFloat(registry, prefix + ".shockwaveCount", &paramShockwaveCount_, paramShockwaveCount_, "Count: Pressure Waves", 0.0f, 12.0f, 1.0f);
    registerFloat(registry, prefix + ".shockwaveAlpha", &paramShockwaveAlpha_, paramShockwaveAlpha_, "Alpha: Pressure Wave", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".shockwaveWidth", &paramShockwaveWidth_, paramShockwaveWidth_, "Scale: Pressure Wave Width", 0.005f, 0.16f, 0.001f);
    registerFloat(registry, prefix + ".shockwaveSpeed", &paramShockwaveSpeed_, paramShockwaveSpeed_, "Time: Pressure Wave Speed", 0.05f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".pressureAmount", &paramPressureAmount_, paramPressureAmount_, "Force: Pressure Wave", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Audio: Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".audioAccretion", &paramAudioAccretion_, paramAudioAccretion_, "Audio: Accretion", 0.0f, 2.0f, 0.01f,
                  "How much audio contributes to long-term web-node and filament formation.");
    registerFloat(registry, prefix + ".audioGlow", &paramAudioGlow_, paramAudioGlow_, "Audio: Glow", 0.0f, 3.0f, 0.01f,
                  "How much audio boosts field, node, and matter glow.");
    registerFloat(registry, prefix + ".audioTwinkle", &paramAudioTwinkle_, paramAudioTwinkle_, "Audio: Twinkle", 0.0f, 3.0f, 0.01f,
                  "How much highs, peaks, and beats modulate star twinkle.");
    registerFloat(registry, prefix + ".bassExpansion", &paramBassExpansion_, paramBassExpansion_, "Audio: Bass Expansion", -1.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".midsTurbulence", &paramMidsTurbulence_, paramMidsTurbulence_, "Audio: Mids Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".highsSparkle", &paramHighsSparkle_, paramHighsSparkle_, "Audio: Highs Sparkle", 0.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".waveformWarp", &paramWaveformWarp_, paramWaveformWarp_, "Audio: Waveform Warp", 0.0f, 0.3f, 0.001f);
    registerFloat(registry, prefix + ".peakBangThreshold", &paramPeakBangThreshold_, paramPeakBangThreshold_, "Audio: Peak Bang Threshold", 0.01f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".peakImpulse", &paramPeakImpulse_, paramPeakImpulse_, "Audio: Peak Impulse", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".beatImpulse", &paramBeatImpulse_, paramBeatImpulse_, "Audio: Beat Impulse", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Audio: Smoothing", 0.0f, 0.98f, 0.01f);

    meta = {};
    meta.group = "Cosmos Formation";
    meta.label = "Action: Bang Trigger";
    meta.description = "Collapse matter back to the singularity and launch a new expansion.";
    registry.addBool(prefix + ".bang", &paramBangRequested_, paramBangRequested_, meta);

    meta = {};
    meta.group = "Cosmos Formation";
    meta.label = "Action: Reseed";
    meta.description = "Respawn matter and clusters using the current seed.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: Cosmos", 0.0f, 99999.0f, 1.0f);
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Alpha: Background", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Color: Background R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Color: Background G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Color: Background B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".hotR", &paramHotR_, paramHotR_, "Color: Hot Matter R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".hotG", &paramHotG_, paramHotG_, "Color: Hot Matter G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".hotB", &paramHotB_, paramHotB_, "Color: Hot Matter B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".matterR", &paramMatterR_, paramMatterR_, "Color: Matter R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".matterG", &paramMatterG_, paramMatterG_, "Color: Matter G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".matterB", &paramMatterB_, paramMatterB_, "Color: Matter B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coolR", &paramCoolR_, paramCoolR_, "Color: Cool Matter R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coolG", &paramCoolG_, paramCoolG_, "Color: Cool Matter G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coolB", &paramCoolB_, paramCoolB_, "Color: Cool Matter B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".clusterR", &paramClusterR_, paramClusterR_, "Color: Cluster R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".clusterG", &paramClusterG_, paramClusterG_, "Color: Cluster G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".clusterB", &paramClusterB_, paramClusterB_, "Color: Cluster B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waveR", &paramWaveR_, paramWaveR_, "Color: Wave R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waveG", &paramWaveG_, paramWaveG_, "Color: Wave G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".waveB", &paramWaveB_, paramWaveB_, "Color: Wave B", 0.0f, 1.5f, 0.01f);

    resetMatter();
}

void CosmosFormationLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    updateAudioState();
    updateBeatState(params.time, params.bpm);

    const int desiredCount = static_cast<int>(std::round(paramParticleCount_));
    const int desiredClusterCount = static_cast<int>(std::round(paramClusterCount_));
    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    if (desiredCount != static_cast<int>(matter_.size()) ||
        desiredClusterCount != clusterState_ ||
        desiredSeed != seedState_ ||
        paramReseedRequested_) {
        resetMatter();
        paramReseedRequested_ = false;
    }

    if (paramBangRequested_) {
        triggerBang(1.0f + level_ * paramAudioAmount_ * 0.35f, true);
        paramBangRequested_ = false;
    }

    const float dt = ofClamp(params.dt, kMinDt, kMaxDt);
    if (dt <= 0.0f) {
        return;
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float transportSpeed = std::max(0.0f, params.speed);
    const float simDt = dt * paramEvolutionSpeed_;
    simTime_ += simDt * std::max(0.25f, transportSpeed);
    if (paramAutoAdvance_) {
        age_ += simDt * paramExpansionRate_ * std::max(0.1f, transportSpeed) * (1.0f + bass_ * audioDrive * 0.18f);
        paramFormationAge_ = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    } else {
        age_ = paramFormationAge_ * paramFormationTime_;
    }
    const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    scaleFactor_ = ofLerp(0.18f, 1.0f, smoothStep(0.0f, 1.0f, lifecycle));
    cosmicTemperature_ = ofLerp(1.0f, 0.16f, smoothStep(0.0f, 1.0f, lifecycle));
    bangEnergy_ = ofLerp(bangEnergy_, 0.0f, ofClamp(dt * (0.45f + paramEvolutionSpeed_ * 0.90f), 0.0f, 1.0f));
    beatPulse_ = ofLerp(beatPulse_, 0.0f, ofClamp(dt * (3.0f + paramEvolutionSpeed_ * 3.0f), 0.0f, 1.0f));

    updateCosmicWeb(simDt, simTime_);
    updateGrowthField(simDt, simTime_, std::max(0.25f, transportSpeed));
    updateMatter(simDt, simTime_, std::max(0.25f, transportSpeed));
    updatePressureWaves(simDt);
}

void CosmosFormationLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (matter_.empty()) {
        resetMatter();
    }
    if (matter_.empty()) {
        return;
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float minDim = std::min(width, height);
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);
    ofSetCircleResolution(20);

    drawBackground(width, height, params.slotOpacity);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    drawGrowthField(width, height, minDim, alpha, simTime_);
    drawPressureWaves(width, height, minDim, alpha);
    drawDensityHalos(width, height, minDim, alpha, simTime_);
    drawCore(width, height, minDim, alpha);
    drawTrails(width, height, minDim, alpha);
    drawMatter(width, height, minDim, alpha, simTime_);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofPopView();
    ofPopStyle();
}

void CosmosFormationLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void CosmosFormationLayer::registerFloat(ParameterRegistry& registry,
                                         const std::string& id,
                                         float* target,
                                         float initial,
                                         const std::string& label,
                                         float min,
                                         float max,
                                         float step,
                                         const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Cosmos Formation";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void CosmosFormationLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void CosmosFormationLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramParticleCount_ = std::round(ofClamp(paramParticleCount_, 96.0f, 3200.0f));
    paramClusterCount_ = std::round(ofClamp(paramClusterCount_, 4.0f, 128.0f));
    paramRadius_ = ofClamp(paramRadius_, 0.25f, 2.4f);
    paramOriginRadius_ = ofClamp(paramOriginRadius_, 0.0f, 0.12f);
    paramExpansionRate_ = ofClamp(paramExpansionRate_, 0.05f, 3.0f);
    paramExpansionForce_ = ofClamp(paramExpansionForce_, 0.0f, 2.0f);
    paramFormationAge_ = ofClamp(paramFormationAge_, 0.0f, 1.0f);
    paramFormationTime_ = ofClamp(paramFormationTime_, 1.0f, 300.0f);
    paramEvolutionSpeed_ = ofClamp(paramEvolutionSpeed_, 0.0f, 1.5f);
    paramGravity_ = ofClamp(paramGravity_, 0.0f, 2.5f);
    paramGravityDelay_ = ofClamp(paramGravityDelay_, 0.0f, 0.95f);
    paramGravityEmergence_ = ofClamp(paramGravityEmergence_, 0.0f, 1.5f);
    paramClusterSwirl_ = ofClamp(paramClusterSwirl_, -4.0f, 4.0f);
    paramClusterSpread_ = ofClamp(paramClusterSpread_, 0.1f, 1.2f);
    paramClusterDrift_ = ofClamp(paramClusterDrift_, 0.0f, 1.0f);
    paramClusterSoftness_ = ofClamp(paramClusterSoftness_, 0.15f, 2.0f);
    paramShear_ = ofClamp(paramShear_, -2.0f, 2.0f);
    paramVoidPressure_ = ofClamp(paramVoidPressure_, 0.0f, 1.0f);
    paramTurbulence_ = ofClamp(paramTurbulence_, 0.0f, 2.0f);
    paramCoolingRate_ = ofClamp(paramCoolingRate_, 0.0f, 2.0f);
    paramFieldLuminosity_ = ofClamp(paramFieldLuminosity_, 0.0f, 4.0f);
    paramGlowPersistence_ = ofClamp(paramGlowPersistence_, 0.0f, 0.98f);
    paramFilamentMemory_ = ofClamp(paramFilamentMemory_, 0.0f, 0.995f);
    paramMatterSize_ = ofClamp(paramMatterSize_, 0.25f, 8.0f);
    paramMatterGlow_ = ofClamp(paramMatterGlow_, 0.0f, 8.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramTrailThickness_ = ofClamp(paramTrailThickness_, 0.25f, 8.0f);
    paramHaloAlpha_ = ofClamp(paramHaloAlpha_, 0.0f, 1.0f);
    paramHaloRadius_ = ofClamp(paramHaloRadius_, 0.02f, 0.45f);
    paramShockwaveCount_ = std::round(ofClamp(paramShockwaveCount_, 0.0f, 12.0f));
    paramShockwaveAlpha_ = ofClamp(paramShockwaveAlpha_, 0.0f, 1.0f);
    paramShockwaveWidth_ = ofClamp(paramShockwaveWidth_, 0.005f, 0.16f);
    paramShockwaveSpeed_ = ofClamp(paramShockwaveSpeed_, 0.05f, 3.0f);
    paramPressureAmount_ = ofClamp(paramPressureAmount_, 0.0f, 1.5f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 2.0f);
    paramAudioAccretion_ = ofClamp(paramAudioAccretion_, 0.0f, 2.0f);
    paramAudioGlow_ = ofClamp(paramAudioGlow_, 0.0f, 3.0f);
    paramAudioTwinkle_ = ofClamp(paramAudioTwinkle_, 0.0f, 3.0f);
    paramBassExpansion_ = ofClamp(paramBassExpansion_, -1.0f, 1.5f);
    paramMidsTurbulence_ = ofClamp(paramMidsTurbulence_, 0.0f, 2.0f);
    paramHighsSparkle_ = ofClamp(paramHighsSparkle_, 0.0f, 4.0f);
    paramWaveformWarp_ = ofClamp(paramWaveformWarp_, 0.0f, 0.3f);
    paramPeakBangThreshold_ = ofClamp(paramPeakBangThreshold_, 0.01f, 1.0f);
    paramPeakImpulse_ = ofClamp(paramPeakImpulse_, 0.0f, 2.0f);
    paramBeatImpulse_ = ofClamp(paramBeatImpulse_, 0.0f, 1.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999.0f));
    paramBgAlpha_ = ofClamp(paramBgAlpha_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramHotR_ = ofClamp(paramHotR_, 0.0f, 1.5f);
    paramHotG_ = ofClamp(paramHotG_, 0.0f, 1.5f);
    paramHotB_ = ofClamp(paramHotB_, 0.0f, 1.5f);
    paramMatterR_ = ofClamp(paramMatterR_, 0.0f, 1.5f);
    paramMatterG_ = ofClamp(paramMatterG_, 0.0f, 1.5f);
    paramMatterB_ = ofClamp(paramMatterB_, 0.0f, 1.5f);
    paramCoolR_ = ofClamp(paramCoolR_, 0.0f, 1.5f);
    paramCoolG_ = ofClamp(paramCoolG_, 0.0f, 1.5f);
    paramCoolB_ = ofClamp(paramCoolB_, 0.0f, 1.5f);
    paramClusterR_ = ofClamp(paramClusterR_, 0.0f, 1.5f);
    paramClusterG_ = ofClamp(paramClusterG_, 0.0f, 1.5f);
    paramClusterB_ = ofClamp(paramClusterB_, 0.0f, 1.5f);
    paramWaveR_ = ofClamp(paramWaveR_, 0.0f, 1.5f);
    paramWaveG_ = ofClamp(paramWaveG_, 0.0f, 1.5f);
    paramWaveB_ = ofClamp(paramWaveB_, 0.0f, 1.5f);
}

void CosmosFormationLayer::resetMatter() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    clusterState_ = static_cast<int>(std::round(paramClusterCount_));

    std::mt19937 rng(seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> centered(-1.0f, 1.0f);

    clusters_.clear();
    clusters_.reserve(static_cast<std::size_t>(clusterState_));
    std::vector<glm::vec2> sites;
    sites.reserve(static_cast<std::size_t>(clusterState_));
    const float fieldX = kGrowthExtent * 0.96f;
    const float fieldY = kGrowthExtent * 0.94f;

    for (int i = 0; i < clusterState_; ++i) {
        glm::vec2 best(centered(rng) * fieldX, centered(rng) * fieldY);
        float bestScore = -1.0f;
        const int attempts = i < 4 ? 36 : 18;
        for (int attempt = 0; attempt < attempts; ++attempt) {
            glm::vec2 candidate(centered(rng) * fieldX, centered(rng) * fieldY);
            candidate = domainWarp(candidate, seedState_ + static_cast<std::uint32_t>(i * 13 + attempt), 0.085f);
            candidate.x = ofClamp(candidate.x, -fieldX, fieldX);
            candidate.y = ofClamp(candidate.y, -fieldY, fieldY);

            float minD2 = 4.0f;
            for (const auto& site : sites) {
                const glm::vec2 delta = candidate - site;
                minD2 = std::min(minD2, glm::dot(delta, delta));
            }

            const float cloud = ofNoise(candidate.x * 0.72f + seedState_ * 0.004f,
                                        candidate.y * 0.72f - seedState_ * 0.003f);
            const float fine = ofNoise(candidate.x * 2.1f - seedState_ * 0.002f,
                                       candidate.y * 2.1f + seedState_ * 0.0025f);
            const float edgeLift = smoothStep(0.42f, 1.0f, std::max(std::abs(candidate.x) / fieldX,
                                                                    std::abs(candidate.y) / fieldY));
            const float score = minD2 * (0.68f + cloud * 0.42f) + fine * 0.035f + edgeLift * 0.018f;
            if (score > bestScore) {
                bestScore = score;
                best = candidate;
            }
        }
        sites.push_back(best);

        Cluster cluster;
        cluster.basePos = best * paramClusterSpread_;
        cluster.basePos.x = ofClamp(cluster.basePos.x, -fieldX, fieldX);
        cluster.basePos.y = ofClamp(cluster.basePos.y, -fieldY, fieldY);
        const float densityNoise = ofNoise(cluster.basePos.x * 0.92f + seedState_ * 0.0027f,
                                           cluster.basePos.y * 0.92f - seedState_ * 0.0031f);
        cluster.radius = ofLerp(0.060f, 0.170f, std::pow(unit(rng), 0.85f));
        cluster.strength = ofLerp(0.34f, 1.05f, ofClamp(unit(rng) * 0.72f + densityNoise * 0.34f, 0.0f, 1.0f));
        cluster.spin = (unit(rng) < 0.5f ? -1.0f : 1.0f) * ofLerp(0.10f, 0.75f, unit(rng));
        cluster.seed = unit(rng) * 10000.0f;
        cluster.heat = ofLerp(0.12f, 0.42f, unit(rng));
        clusters_.push_back(cluster);
    }
    seedWebNodesFromClusters();
    buildFilamentEdges();

    const float initialLifecycle = ofClamp(paramFormationAge_, 0.0f, 1.0f);
    const std::size_t count = static_cast<std::size_t>(std::max(1.0f, std::round(paramParticleCount_)));
    matter_.clear();
    matter_.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        MatterNode node;
        node.cluster = static_cast<std::size_t>(i % std::max(1, clusterState_));
        if (unit(rng) < 0.44f) {
            node.cluster = static_cast<std::size_t>(std::floor(unit(rng) * static_cast<float>(clusterState_)));
            node.cluster = std::min<std::size_t>(node.cluster, clusters_.size() - 1);
        }
        node.angle = unit(rng) * TWO_PI;
        node.orbitRadius = std::pow(unit(rng), 0.72f);
        node.speed = ofLerp(0.45f, 1.45f, unit(rng));
        node.size = ofLerp(0.45f, 1.65f, std::pow(unit(rng), 1.8f));
        node.brightness = ofLerp(0.28f, 1.0f, std::pow(unit(rng), 0.5f));
        node.heat = ofLerp(0.78f, 1.0f, unit(rng));
        node.seed = unit(rng) * 10000.0f;
        node.spin = (unit(rng) < 0.5f ? -1.0f : 1.0f) * ofLerp(0.45f, 1.35f, unit(rng));
        node.birthDelay = std::pow(unit(rng), 2.2f) * 0.95f;
        node.mass = ofLerp(0.65f, 1.35f, unit(rng));

        const glm::vec2 originDir(std::cos(node.angle), std::sin(node.angle));
        if (initialLifecycle > 0.35f && !webNodes_.empty()) {
            glm::vec2 filamentDir = originDir;
            const bool diffuseDust = unit(rng) < ofLerp(0.56f, 0.34f, smoothStep(0.35f, 1.0f, initialLifecycle));
            if (diffuseDust) {
                glm::vec2 p(centered(rng) * kGrowthExtent * 0.98f,
                            centered(rng) * kGrowthExtent * 0.96f);
                p = domainWarp(p, seedState_ + static_cast<std::uint32_t>(i * 19), 0.070f);
                p.x = ofClamp(p.x, -kGrowthExtent * 1.02f, kGrowthExtent * 1.02f);
                p.y = ofClamp(p.y, -kGrowthExtent * 1.00f, kGrowthExtent * 1.00f);
                node.pos = p;
                filamentDir = safeNormalize(glm::vec2(gaussianish(rng), gaussianish(rng)), originDir);
                node.brightness *= ofLerp(0.62f, 1.08f, unit(rng));
                node.heat = ofLerp(0.20f, 0.72f, unit(rng));
            } else if (!filamentEdges_.empty() && unit(rng) < 0.76f) {
                const FilamentEdge& edge = filamentEdges_[static_cast<std::size_t>(std::floor(unit(rng) * filamentEdges_.size())) % filamentEdges_.size()];
                const WebNode& a = webNodes_[static_cast<std::size_t>(std::max(0, edge.a)) % webNodes_.size()];
                const WebNode& b = webNodes_[static_cast<std::size_t>(std::max(0, edge.b)) % webNodes_.size()];
                const float t = ofLerp(0.04f, 0.96f, unit(rng));
                const glm::vec2 path = organicFilamentPoint(a.pos, b.pos, edge.bend, t, edge.a, edge.b, seedState_);
                const glm::vec2 pathForward = organicFilamentPoint(a.pos, b.pos, edge.bend, ofClamp(t + 0.025f, 0.0f, 1.0f), edge.a, edge.b, seedState_);
                filamentDir = safeNormalize(pathForward - path, safeNormalize(b.pos - a.pos, originDir));
                const glm::vec2 normal(-filamentDir.y, filamentDir.x);
                const float width = ofLerp(0.010f, 0.050f, std::pow(unit(rng), 1.7f));
                node.pos = path + normal * gaussianish(rng) * width +
                           filamentDir * gaussianish(rng) * width * 0.55f;
            } else {
                const WebNode& webNode = webNodes_[static_cast<std::size_t>(std::floor(unit(rng) * webNodes_.size())) % webNodes_.size()];
                filamentDir = safeNormalize(webNode.pos, originDir);
                node.pos = webNode.pos + glm::vec2(gaussianish(rng), gaussianish(rng)) * webNode.radius * ofLerp(0.36f, 1.25f, unit(rng));
            }
            node.prev = node.pos;
            node.vel = filamentDir * gaussianish(rng) * 0.018f +
                       glm::vec2(gaussianish(rng), gaussianish(rng)) * 0.010f;
        } else {
            const bool primordialDust = unit(rng) < 0.42f;
            if (primordialDust) {
                const float spread = ofLerp(0.22f, 1.0f, smoothStep(0.0f, 0.35f, initialLifecycle));
                node.pos = glm::vec2(centered(rng) * kGrowthExtent * 0.98f * spread,
                                     centered(rng) * kGrowthExtent * 0.96f * spread);
                node.pos = domainWarp(node.pos, seedState_ + static_cast<std::uint32_t>(i * 23), 0.050f * spread);
                node.heat = ofLerp(0.32f, 0.92f, unit(rng));
                node.brightness *= ofLerp(0.48f, 0.92f, unit(rng));
            } else {
                const float originRadius = std::pow(unit(rng), 2.3f) * std::max(0.001f, paramOriginRadius_);
                node.pos = originDir * originRadius + glm::vec2(gaussianish(rng), gaussianish(rng)) * paramOriginRadius_ * 0.22f;
            }
            node.prev = node.pos;

            const glm::vec2 clusterDir = safeNormalize(clusters_[node.cluster].basePos, originDir);
            const glm::vec2 seedDir(std::cos(node.seed * 0.017f), std::sin(node.seed * 0.021f));
            const glm::vec2 launchDir = safeNormalize(originDir * 0.86f + seedDir * 0.28f + clusterDir * 0.16f, originDir);
            node.vel = launchDir * paramExpansionForce_ * node.speed * ofLerp(0.35f, 1.15f, unit(rng));
        }
        matter_.push_back(node);
    }

    waves_.clear();
    const float lifecycle = initialLifecycle;
    age_ = lifecycle * paramFormationTime_;
    paramFormationAge_ = lifecycle;
    scaleFactor_ = ofLerp(0.18f, 1.0f, smoothStep(0.0f, 1.0f, lifecycle));
    cosmicTemperature_ = ofLerp(1.0f, 0.16f, smoothStep(0.0f, 1.0f, lifecycle));
    bangEnergy_ = lifecycle <= 0.001f ? 1.0f : 0.0f;
    beatPulse_ = 0.0f;
    lastPeakTime_ = -1000.0f;
    lastBeatIndex_ = -1;
    webRebuildAccumulator_ = 0.0f;
    resetGrowthField();
    triggerPressureWave(1.0f, glm::vec2(0.0f, 0.0f));
}

void CosmosFormationLayer::triggerBang(float strength, bool collapseMatter) {
    const float bang = ofClamp(strength, 0.0f, 2.5f);
    age_ = 0.0f;
    paramFormationAge_ = 0.0f;
    scaleFactor_ = 0.18f;
    cosmicTemperature_ = 1.0f;
    bangEnergy_ = std::max(bangEnergy_, bang);
    waves_.clear();
    seedWebNodesFromClusters();
    buildFilamentEdges();
    resetGrowthField();
    triggerPressureWave(bang, glm::vec2(0.0f, 0.0f));

    if (!collapseMatter || matter_.empty() || clusters_.empty()) {
        return;
    }

    for (auto& node : matter_) {
        const glm::vec2 originDir(std::cos(node.angle + node.seed * 0.0017f), std::sin(node.angle + node.seed * 0.0017f));
        const glm::vec2 clusterDir = safeNormalize(clusters_[node.cluster % clusters_.size()].basePos, originDir);
        const glm::vec2 seedDir(std::cos(node.seed * 0.017f + age_ * 0.05f), std::sin(node.seed * 0.021f - age_ * 0.04f));
        const glm::vec2 launchDir = safeNormalize(originDir * 0.86f + seedDir * 0.28f + clusterDir * 0.16f, originDir);
        const float dustGate = ofNoise(node.seed * 0.013f, seedState_ * 0.002f);
        if (dustGate < 0.46f) {
            glm::vec2 dust(std::sin(node.seed * 0.071f) * kGrowthExtent * 0.98f,
                           std::cos(node.seed * 0.047f) * kGrowthExtent * 0.96f);
            dust = domainWarp(dust, seedState_ + static_cast<std::uint32_t>(node.seed), 0.055f);
            node.prev = dust;
            node.pos = dust;
            node.vel = launchDir * paramExpansionForce_ * node.speed * (0.12f + bang * 0.14f) +
                       glm::vec2(std::sin(node.seed * 0.031f), std::cos(node.seed * 0.029f)) * 0.015f;
            node.heat = ofLerp(0.34f, 0.86f, dustGate);
            node.brightness = std::max(node.brightness, 0.34f);
        } else {
            node.prev = originDir * paramOriginRadius_ * 0.12f;
            node.pos = node.prev;
            node.vel = launchDir * paramExpansionForce_ * node.speed * (0.85f + bang * 0.45f);
            node.heat = 1.0f;
        }
    }
}

void CosmosFormationLayer::triggerIgnition(float strength, const glm::vec2& origin) {
    const float ignition = ofClamp(strength, 0.0f, 2.5f);
    if (ignition <= 0.001f) {
        return;
    }

    triggerPressureWave(ignition, origin);
    if (growthField_.size() != static_cast<std::size_t>(kGrowthCols * kGrowthRows)) {
        resetGrowthField();
    }

    for (int y = 0; y < kGrowthRows; ++y) {
        for (int x = 0; x < kGrowthCols; ++x) {
            GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
            const glm::vec2 p = growthFieldPoint(x, y);
            const float influence = bell(glm::length(p - origin), 0.0f, 0.13f) * ignition;
            if (influence <= 0.0001f) {
                continue;
            }

            cell.bloom = std::max(cell.bloom, ofClamp(influence, 0.0f, 1.0f));
            cell.audioEnergy = std::max(cell.audioEnergy, ofClamp(influence * 0.82f, 0.0f, 1.0f));
            cell.luminosity = std::max(cell.luminosity, ofClamp(influence * (0.48f + cell.node * 0.35f), 0.0f, 1.0f));
            cell.temperature = ofClamp(cell.temperature + influence * 0.35f, 0.0f, 1.0f);
            cell.gasDensity = ofClamp(cell.gasDensity + influence * 0.045f, 0.0f, 1.25f);
            cell.compression = std::max(cell.compression, ofClamp(influence * 0.42f, 0.0f, 1.0f));
            cell.node = std::max(cell.node, ofClamp(influence * 0.24f, 0.0f, 1.0f));
            const float handedness = ((x + y) % 2 == 0) ? 1.0f : -1.0f;
            cell.spin = ofClamp(cell.spin + handedness * ignition * influence * 0.10f, -1.0f, 1.0f);
        }
    }
}

void CosmosFormationLayer::triggerPressureWave(float strength, const glm::vec2& origin) {
    if (paramShockwaveAlpha_ <= 0.0f && paramPressureAmount_ <= 0.0f) {
        return;
    }

    PressureWave wave;
    wave.origin = origin;
    wave.strength = ofClamp(strength, 0.0f, 3.0f);
    waves_.push_back(wave);
    if (waves_.size() > 10) {
        waves_.erase(waves_.begin());
    }
}

void CosmosFormationLayer::updateAudioState() {
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    const float follow = followAmount(paramAudioSmoothing_);
    hasAudio_ = snapshot.valid;
    if (!snapshot.valid) {
        level_ = ofLerp(level_, 0.0f, follow * 0.25f);
        peak_ = ofLerp(peak_, 0.0f, follow * 0.25f);
        bass_ = ofLerp(bass_, 0.0f, follow * 0.25f);
        mids_ = ofLerp(mids_, 0.0f, follow * 0.25f);
        highs_ = ofLerp(highs_, 0.0f, follow * 0.25f);
        hasWaveform_ = false;
        return;
    }

    level_ = ofLerp(level_, snapshot.level, follow);
    peak_ = ofLerp(peak_, snapshot.peak, follow);
    bass_ = ofLerp(bass_, snapshot.bass, follow);
    mids_ = ofLerp(mids_, snapshot.mids, follow);
    highs_ = ofLerp(highs_, snapshot.highs, follow);
    if (snapshot.frame != lastAudioFrame_) {
        waveform_ = snapshot.waveform;
        hasWaveform_ = !waveform_.empty();
        lastAudioFrame_ = snapshot.frame;
    }
}

void CosmosFormationLayer::updateBeatState(float timeSeconds, float bpm) {
    const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    const float beatPosition = currentBeatPosition(timeSeconds, bpm);
    const int beatIndex = static_cast<int>(std::floor(beatPosition));
    if (lastBeatIndex_ < 0) {
        lastBeatIndex_ = beatIndex;
    } else if (beatIndex > lastBeatIndex_) {
        beatPulse_ = std::max(beatPulse_, paramBeatImpulse_);
        if (paramBeatImpulse_ > 0.0f) {
            glm::vec2 origin(0.0f, 0.0f);
            float strength = paramBeatImpulse_;
            if (!webNodes_.empty() && lifecycle > 0.24f) {
                origin = webNodes_[static_cast<std::size_t>(beatIndex) % webNodes_.size()].pos;
                strength *= 0.55f;
            }
            triggerIgnition(strength * 0.65f, origin);
        }
        lastBeatIndex_ = beatIndex;
    }

    if (hasAudio_ && peak_ >= paramPeakBangThreshold_ && timeSeconds - lastPeakTime_ >= 0.12f) {
        float impulse = peak_ * paramPeakImpulse_;
        glm::vec2 origin(0.0f, 0.0f);
        if (!webNodes_.empty() && lifecycle > 0.22f) {
            const std::size_t index = static_cast<std::size_t>(std::floor(timeSeconds * 1.7f + peak_ * 11.0f)) % webNodes_.size();
            origin = webNodes_[index].pos;
            impulse *= 0.65f;
        }
        bangEnergy_ = std::max(bangEnergy_, impulse * 0.45f);
        triggerIgnition(impulse, origin);
        lastPeakTime_ = timeSeconds;
    }
}

void CosmosFormationLayer::resetGrowthField() {
    growthField_.assign(static_cast<std::size_t>(kGrowthCols * kGrowthRows), GrowthCell{});
    growthScratch_ = growthField_;

    for (int y = 0; y < kGrowthRows; ++y) {
        for (int x = 0; x < kGrowthCols; ++x) {
            const glm::vec2 p = growthFieldPoint(x, y);
            const float r = glm::length(p);
            const float seedNoise = ofNoise(p.x * 5.1f + seedState_ * 0.013f, p.y * 5.1f - seedState_ * 0.009f);
            const float starNoise = ofNoise(p.x * 18.0f - seedState_ * 0.003f,
                                            p.y * 18.0f + seedState_ * 0.004f);
            const float fineStars = std::pow(ofClamp(starNoise, 0.0f, 1.0f), 5.2f) *
                                    (0.018f + smoothStep(0.0f, 1.0f, paramFormationAge_) * 0.030f);
            GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
            cell.darkDensity = ofClamp(seedNoise * 0.055f + bell(r, 0.0f, 0.12f) * bangEnergy_ * 0.10f, 0.0f, 1.0f);
            cell.gasDensity = bell(r, 0.0f, 0.075f) * bangEnergy_ * (0.20f + seedNoise * 0.18f);
            cell.stellarDensity = fineStars;
            cell.temperature = ofClamp(cell.gasDensity + bangEnergy_ * 0.20f, 0.0f, 1.0f);
            cell.spin = 0.0f;
            cell.bloom = cell.gasDensity * 0.35f;
            cell.compression = 0.0f;
            cell.luminosity = cell.bloom * 0.25f + cell.stellarDensity * 0.20f;
            cell.audioEnergy = 0.0f;
            cell.ridge = 0.0f;
            cell.node = 0.0f;
            cell.voidness = 0.0f;
            cell.filamentDir = glm::vec2(0.0f, 0.0f);
        }
    }
    growthScratch_ = growthField_;
    paintCosmicWebIntoField(0.0f, 0.0f);
}

void CosmosFormationLayer::seedWebNodesFromClusters() {
    webNodes_.clear();
    nextWebNodeId_ = 1;
    if (clusters_.empty()) {
        return;
    }

    std::mt19937 rng(seedState_ ^ 0x9e3779b9u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> centered(-1.0f, 1.0f);

    const std::size_t satelliteCount = std::max<std::size_t>(16, clusters_.size() * 5 / 4);
    webNodes_.reserve(clusters_.size() + satelliteCount);
    const float initialActivation = ofClamp(0.055f + smoothStep(0.0f, 1.0f, paramFormationAge_) * 0.18f, 0.0f, 0.32f);
    for (const auto& cluster : clusters_) {
        WebNode node;
        node.id = nextWebNodeId_++;
        node.pos = cluster.basePos;
        node.mass = cluster.strength;
        node.radius = ofClamp(cluster.radius * ofLerp(1.10f, 1.85f, unit(rng)), 0.052f, 0.245f);
        node.heat = cluster.heat;
        node.activation = initialActivation * ofLerp(0.70f, 1.18f, unit(rng));
        node.audioCharge = 0.0f;
        node.effectiveMass = node.mass * node.activation;
        webNodes_.push_back(node);
    }

    const int minorCount = static_cast<int>(satelliteCount);
    for (int i = 0; i < minorCount && clusters_.size() > 1; ++i) {
        const std::size_t a = static_cast<std::size_t>(std::floor(unit(rng) * static_cast<float>(clusters_.size()))) % clusters_.size();
        std::size_t b = static_cast<std::size_t>(std::floor(unit(rng) * static_cast<float>(clusters_.size()))) % clusters_.size();
        if (b == a) {
            b = (b + 1) % clusters_.size();
        }

        const float t = ofLerp(0.24f, 0.76f, unit(rng));
        const glm::vec2 span = clusters_[b].basePos - clusters_[a].basePos;
        const glm::vec2 tangent = safeNormalize(span, glm::vec2(1.0f, 0.0f));
        const glm::vec2 normal(-tangent.y, tangent.x);
        glm::vec2 p = clusters_[a].basePos * (1.0f - t) + clusters_[b].basePos * t;
        p += normal * gaussianish(rng) * ofLerp(0.045f, 0.20f, unit(rng));
        p += tangent * gaussianish(rng) * 0.065f;
        p = domainWarp(p, seedState_ + static_cast<std::uint32_t>(i * 37), 0.045f);
        p.x = ofClamp(p.x, -kGrowthExtent * 0.98f, kGrowthExtent * 0.98f);
        p.y = ofClamp(p.y, -kGrowthExtent * 0.96f, kGrowthExtent * 0.96f);

        WebNode node;
        node.id = nextWebNodeId_++;
        node.pos = p;
        node.mass = ofLerp(0.18f, 0.52f, unit(rng));
        node.radius = ofLerp(0.034f, 0.090f, unit(rng));
        node.heat = ofLerp(0.08f, 0.30f, unit(rng));
        node.activation = initialActivation * ofLerp(0.35f, 0.86f, unit(rng));
        node.audioCharge = 0.0f;
        node.effectiveMass = node.mass * node.activation;
        webNodes_.push_back(node);
    }
}

void CosmosFormationLayer::buildFilamentEdges() {
    const std::vector<FilamentEdge> previousEdges = filamentEdges_;
    filamentEdges_.clear();
    if (webNodes_.size() < 2) {
        return;
    }

    auto edgeExists = [&](int a, int b) {
        const int lo = std::min(a, b);
        const int hi = std::max(a, b);
        return std::any_of(filamentEdges_.begin(), filamentEdges_.end(), [&](const FilamentEdge& edge) {
            return edge.a == lo && edge.b == hi;
        });
    };

    auto previousFor = [&](int a, int b) -> const FilamentEdge* {
        const int lo = std::min(a, b);
        const int hi = std::max(a, b);
        for (const auto& edge : previousEdges) {
            if (edge.a == lo && edge.b == hi) {
                return &edge;
            }
        }
        return nullptr;
    };

    for (std::size_t i = 0; i < webNodes_.size(); ++i) {
        std::vector<std::pair<float, int>> nearest;
        nearest.reserve(webNodes_.size() - 1);
        for (std::size_t j = 0; j < webNodes_.size(); ++j) {
            if (i == j) {
                continue;
            }
            const glm::vec2 delta = webNodes_[i].pos - webNodes_[j].pos;
            nearest.push_back({ glm::dot(delta, delta), static_cast<int>(j) });
        }
        std::sort(nearest.begin(), nearest.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });

        const int connectCount = std::min(3, static_cast<int>(nearest.size()));
        for (int n = 0; n < connectCount; ++n) {
            const int a = static_cast<int>(i);
            const int b = nearest[static_cast<std::size_t>(n)].second;
            const int lo = std::min(a, b);
            const int hi = std::max(a, b);
            const float dist = std::sqrt(std::max(0.0001f, nearest[static_cast<std::size_t>(n)].first));
            if (edgeExists(lo, hi) || (n > 0 && dist > 0.96f)) {
                continue;
            }

            FilamentEdge edge;
            edge.a = lo;
            edge.b = hi;
            edge.strength = ofClamp(0.34f / dist, 0.16f, 1.0f);
            const glm::vec2 delta = webNodes_[static_cast<std::size_t>(hi)].pos - webNodes_[static_cast<std::size_t>(lo)].pos;
            const glm::vec2 normal = safeNormalize(glm::vec2(-delta.y, delta.x), glm::vec2(0.0f, 1.0f));
            const float bendNoise = ofNoise(static_cast<float>(lo) * 0.137f + seedState_ * 0.001f,
                                            static_cast<float>(hi) * 0.173f - seedState_ * 0.0013f);
            edge.bend = normal * dist * ofLerp(-0.22f, 0.22f, bendNoise);
            if (const FilamentEdge* old = previousFor(lo, hi)) {
                edge.activation = old->activation;
                edge.audioCharge = old->audioCharge;
                edge.conductivity = old->conductivity;
                edge.massFlow = old->massFlow;
                edge.age = old->age;
                edge.bend = old->bend;
            } else {
                const float endpointActivation = (webNodes_[static_cast<std::size_t>(lo)].activation +
                                                  webNodes_[static_cast<std::size_t>(hi)].activation) * 0.5f;
                edge.activation = endpointActivation * ofClamp(edge.strength * 0.70f, 0.0f, 1.0f);
                edge.conductivity = edge.activation;
            }
            filamentEdges_.push_back(edge);
        }
    }
}

void CosmosFormationLayer::updateCosmicWeb(float dt, float timeSeconds) {
    if (webNodes_.empty() && !clusters_.empty()) {
        seedWebNodesFromClusters();
        buildFilamentEdges();
    }
    if (webNodes_.empty()) {
        return;
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    const float chargeMemory = std::pow(paramFilamentMemory_, dt * 18.0f);
    const float driftScale = paramClusterDrift_ * (0.010f + level_ * audioDrive * 0.006f);
    for (auto& node : webNodes_) {
        node.age += dt;
        const GrowthSample field = sampleGrowthField(node.pos);
        const float fieldMass = ofClamp(field.darkDensity + field.gasDensity * 0.62f + field.stellarDensity * 1.25f +
                                        field.ridge * 0.72f + field.node * 0.95f + field.luminosity * 0.36f,
                                        0.0f, 1.8f);
        const float densityAccretion = smoothStep(0.08f, 0.95f, fieldMass);
        const float audioAccretion = audioDrive * paramAudioAccretion_ *
                                     (bass_ * (0.018f + field.voidness * 0.055f) +
                                      mids_ * (field.ridge * 0.16f + field.audioEnergy * 0.10f) +
                                      highs_ * (field.luminosity * 0.12f + field.stellarDensity * 0.08f) +
                                      beatPulse_ * (field.node * 0.34f + field.ridge * 0.12f));
        node.audioCharge = ofClamp(node.audioCharge * chargeMemory + audioAccretion * dt, 0.0f, 1.0f);
        node.activation = ofClamp(node.activation +
                                  dt * paramGravityEmergence_ *
                                      (0.010f + densityAccretion * 0.13f + node.audioCharge * 0.26f +
                                       smoothStep(0.18f, 1.0f, lifecycle) * 0.018f),
                                  0.0f, 1.0f);
        const float well = smoothStep(0.04f, 0.92f, node.activation);
        node.effectiveMass = node.mass * (well * 0.92f + node.audioCharge * 0.22f);

        const float nx = ofNoise(node.id * 0.071f, timeSeconds * 0.026f, seedState_ * 0.001f) - 0.5f;
        const float ny = ofNoise(node.id * 0.097f + 19.0f, timeSeconds * 0.022f, seedState_ * 0.0013f) - 0.5f;
        glm::vec2 wander(nx, ny);
        node.vel += wander * driftScale * dt * ofLerp(1.0f, 0.45f, well);
        node.vel -= node.pos * dt * 0.0018f;
        node.vel *= ofClamp(1.0f - dt * 0.36f, 0.0f, 1.0f);
        node.pos += node.vel * dt;
        if (glm::length(node.pos) > kGrowthExtent * 0.90f) {
            node.pos = safeNormalize(node.pos) * kGrowthExtent * 0.90f;
            node.vel *= 0.45f;
        }
    }

    for (auto& edge : filamentEdges_) {
        edge.age += dt;
        if (edge.a < 0 || edge.b < 0 ||
            edge.a >= static_cast<int>(webNodes_.size()) ||
            edge.b >= static_cast<int>(webNodes_.size())) {
            continue;
        }

        const WebNode& a = webNodes_[static_cast<std::size_t>(edge.a)];
        const WebNode& b = webNodes_[static_cast<std::size_t>(edge.b)];
        const glm::vec2 probe = (a.pos + b.pos) * 0.5f + edge.bend * 0.55f;
        const GrowthSample field = sampleGrowthField(probe);
        const float endpointActivation = (a.activation + b.activation) * 0.5f;
        const float ridgeAccretion = smoothStep(0.03f, 0.70f, field.ridge + field.darkDensity * 0.22f + field.gasDensity * 0.18f);
        const float audioAccretion = audioDrive * paramAudioAccretion_ *
                                     (mids_ * (0.040f + field.ridge * 0.22f) +
                                      bass_ * field.voidness * 0.050f +
                                      highs_ * field.luminosity * 0.10f +
                                      beatPulse_ * endpointActivation * 0.16f);
        edge.audioCharge = ofClamp(edge.audioCharge * chargeMemory + audioAccretion * dt, 0.0f, 1.0f);
        edge.massFlow = ofClamp(edge.massFlow * chargeMemory +
                                dt * (ridgeAccretion * 0.22f + endpointActivation * 0.10f + edge.audioCharge * 0.18f),
                                0.0f, 1.0f);
        edge.activation = ofClamp(edge.activation * std::pow(paramFilamentMemory_, dt * 2.0f) +
                                  dt * paramGravityEmergence_ *
                                      (endpointActivation * 0.055f + ridgeAccretion * 0.14f +
                                       edge.massFlow * 0.10f + edge.audioCharge * 0.24f),
                                  0.0f, 1.0f);
        edge.conductivity = ofClamp(edge.strength * (0.20f + edge.activation * 0.72f + edge.audioCharge * 0.22f),
                                    0.0f, 1.25f);
    }
    webRebuildAccumulator_ += dt;
    if (webRebuildAccumulator_ > 4.0f || filamentEdges_.empty()) {
        buildFilamentEdges();
        webRebuildAccumulator_ = 0.0f;
    }
}

void CosmosFormationLayer::paintCosmicWebIntoField(float dt, float timeSeconds) {
    if (growthField_.size() != static_cast<std::size_t>(kGrowthCols * kGrowthRows)) {
        return;
    }

    for (auto& cell : growthField_) {
        cell.ridge = 0.0f;
        cell.node = 0.0f;
        cell.voidness = 0.0f;
        cell.filamentDir = glm::vec2(0.0f, 0.0f);
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    auto gridBounds = [](float minValue, float maxValue, int cellCount) {
        const float start = ((minValue / kGrowthExtent + 1.0f) * 0.5f) * static_cast<float>(cellCount);
        const float end = ((maxValue / kGrowthExtent + 1.0f) * 0.5f) * static_cast<float>(cellCount);
        const int lo = std::max(0, std::min(cellCount - 1, static_cast<int>(std::floor(start))));
        const int hi = std::max(0, std::min(cellCount - 1, static_cast<int>(std::ceil(end))));
        return std::pair<int, int>{ lo, hi };
    };

    for (const auto& edge : filamentEdges_) {
        if (edge.a < 0 || edge.b < 0 ||
            edge.a >= static_cast<int>(webNodes_.size()) ||
            edge.b >= static_cast<int>(webNodes_.size())) {
            continue;
        }

        const glm::vec2 a = webNodes_[static_cast<std::size_t>(edge.a)].pos;
        const glm::vec2 b = webNodes_[static_cast<std::size_t>(edge.b)].pos;
        const float active = smoothStep(0.015f, 0.86f, edge.activation);
        const float conductivity = ofClamp(edge.conductivity + edge.audioCharge * 0.18f, 0.0f, 1.25f);
        const float pulse = (0.10f + active * 0.90f) *
                            (1.0f + beatPulse_ * 0.10f + mids_ * audioDrive * 0.08f +
                             edge.audioCharge * 0.16f + level_ * audioDrive * paramAudioGlow_ * 0.08f);
        const float influenceMargin = 0.24f;
        const glm::vec2 roughControl = (a + b) * 0.5f + edge.bend;
        const auto [minX, maxX] = gridBounds(std::min(std::min(a.x, b.x), roughControl.x) - influenceMargin,
                                             std::max(std::max(a.x, b.x), roughControl.x) + influenceMargin,
                                             kGrowthCols);
        const auto [minY, maxY] = gridBounds(std::min(std::min(a.y, b.y), roughControl.y) - influenceMargin,
                                             std::max(std::max(a.y, b.y), roughControl.y) + influenceMargin,
                                             kGrowthRows);

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
                const glm::vec2 p = growthFieldPoint(x, y);
                float bestD = 100.0f;
                float bestT = 0.0f;
                glm::vec2 bestDir = safeNormalize(b - a);
                glm::vec2 previous = organicFilamentPoint(a, b, edge.bend, 0.0f, edge.a, edge.b, seedState_);
                constexpr int kCurveSegments = 8;
                for (int segment = 1; segment <= kCurveSegments; ++segment) {
                    const float segmentT = static_cast<float>(segment) / static_cast<float>(kCurveSegments);
                    const glm::vec2 current = organicFilamentPoint(a, b, edge.bend, segmentT, edge.a, edge.b, seedState_);
                    float localT = 0.0f;
                    const float d = distanceToSegment(p, previous, current, localT);
                    if (d < bestD) {
                        bestD = d;
                        bestT = (static_cast<float>(segment - 1) + localT) / static_cast<float>(kCurveSegments);
                        bestDir = safeNormalize(current - previous, bestDir);
                    }
                    previous = current;
                }

                const float d = bestD;
                const float t = bestT;
                const glm::vec2 dir = bestDir;
                const float core = std::exp(-(d * d) / 0.0022f);
                const float skirt = std::exp(-(d * d) / 0.024f) * 0.32f;
                const float alongFade = smoothStep(0.0f, 0.08f, t) * (1.0f - smoothStep(0.92f, 1.0f, t));
                const float grain = 0.72f + ofNoise(p.x * 11.0f + edge.strength * 31.0f,
                                                    p.y * 11.0f - seedState_ * 0.002f,
                                                    timeSeconds * 0.035f) * 0.42f;
                const float filament = ofClamp((core + skirt) * alongFade * conductivity * pulse * grain, 0.0f, 1.0f);
                if (filament <= 0.0005f) {
                    continue;
                }

                cell.ridge = std::max(cell.ridge, filament);
                cell.darkDensity = ofClamp(cell.darkDensity + filament * (0.060f + dt * 0.025f), 0.0f, 1.5f);
                cell.gasDensity = ofClamp(cell.gasDensity + filament * (0.026f + bass_ * audioDrive * 0.010f), 0.0f, 1.5f);
                cell.compression = std::max(cell.compression, ofClamp(filament * 0.34f, 0.0f, 1.0f));
                cell.audioEnergy = std::max(cell.audioEnergy, edge.audioCharge * filament * 0.20f);
                cell.luminosity = std::max(cell.luminosity, filament * (cell.audioEnergy * 0.24f + edge.massFlow * 0.14f +
                                                                         level_ * audioDrive * paramAudioGlow_ * 0.035f));
                cell.filamentDir = safeNormalize(cell.filamentDir + dir * filament, dir);
            }
        }
    }

    if (webNodes_.size() >= 2) {
        for (int y = 0; y < kGrowthRows; ++y) {
            for (int x = 0; x < kGrowthCols; ++x) {
                GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
                const glm::vec2 p = growthFieldPoint(x, y);
                const glm::vec2 warped = domainWarp(p, seedState_, 0.115f);
                float nearestD2 = 100.0f;
                float secondD2 = 100.0f;
                int nearestIndex = -1;
                int secondIndex = -1;
                for (std::size_t i = 0; i < webNodes_.size(); ++i) {
                    const glm::vec2 site = domainWarp(webNodes_[i].pos,
                                                      seedState_ + static_cast<std::uint32_t>(webNodes_[i].id * 11),
                                                      0.028f);
                    const glm::vec2 delta = warped - site;
                    const float d2 = glm::dot(delta, delta);
                    if (d2 < nearestD2) {
                        secondD2 = nearestD2;
                        secondIndex = nearestIndex;
                        nearestD2 = d2;
                        nearestIndex = static_cast<int>(i);
                    } else if (d2 < secondD2) {
                        secondD2 = d2;
                        secondIndex = static_cast<int>(i);
                    }
                }

                if (nearestIndex < 0 || secondIndex < 0) {
                    continue;
                }

                const WebNode& nearest = webNodes_[static_cast<std::size_t>(nearestIndex)];
                const WebNode& second = webNodes_[static_cast<std::size_t>(secondIndex)];
                const float d0 = std::sqrt(std::max(0.0f, nearestD2));
                const float d1 = std::sqrt(std::max(0.0f, secondD2));
                const float gap = std::abs(d1 - d0);
                const float activation = ofClamp((nearest.activation + second.activation) * 0.5f +
                                                 (nearest.audioCharge + second.audioCharge) * 0.18f,
                                                 0.0f, 1.0f);
                const float mask = 0.58f + ofNoise(p.x * 3.4f + seedState_ * 0.0021f,
                                                   p.y * 3.4f - seedState_ * 0.0024f,
                                                   timeSeconds * 0.018f) * 0.52f;
                const float ridgeWidth = ofLerp(0.020f, 0.050f, paramClusterSoftness_ * 0.5f);
                const float spacingFade = smoothStep(0.10f, 0.58f, d1) * (1.0f - smoothStep(0.64f, 1.55f, d0));
                const float voronoiRidge = std::exp(-(gap * gap) / std::max(0.0008f, ridgeWidth * ridgeWidth)) *
                                           spacingFade * mask * (0.18f + activation * 0.54f);
                if (voronoiRidge <= 0.001f) {
                    continue;
                }

                const glm::vec2 between = safeNormalize(second.pos - nearest.pos);
                const glm::vec2 ridgeDir(-between.y, between.x);
                cell.ridge = std::max(cell.ridge, ofClamp(voronoiRidge, 0.0f, 1.0f));
                cell.darkDensity = ofClamp(cell.darkDensity + voronoiRidge * 0.040f, 0.0f, 1.5f);
                cell.gasDensity = ofClamp(cell.gasDensity + voronoiRidge * (0.018f + bass_ * audioDrive * 0.008f), 0.0f, 1.5f);
                cell.compression = std::max(cell.compression, ofClamp(voronoiRidge * 0.18f, 0.0f, 1.0f));
                cell.audioEnergy = std::max(cell.audioEnergy,
                                            voronoiRidge * (nearest.audioCharge + second.audioCharge) * 0.055f);
                cell.luminosity = std::max(cell.luminosity,
                                           voronoiRidge * level_ * audioDrive * paramAudioGlow_ * 0.022f);
                cell.filamentDir = safeNormalize(cell.filamentDir + ridgeDir * voronoiRidge, ridgeDir);
            }
        }
    }

    for (const auto& node : webNodes_) {
        const float active = smoothStep(0.015f, 0.86f, node.activation);
        const float effectiveMass = std::max(node.effectiveMass, node.mass * active * 0.08f);
        const float influenceMargin = std::max(0.120f, node.radius * 3.65f);
        const auto [minX, maxX] = gridBounds(node.pos.x - influenceMargin,
                                             node.pos.x + influenceMargin,
                                             kGrowthCols);
        const auto [minY, maxY] = gridBounds(node.pos.y - influenceMargin,
                                             node.pos.y + influenceMargin,
                                             kGrowthRows);

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
                const glm::vec2 p = growthFieldPoint(x, y);
                const float r = glm::length(p - node.pos);
                const float nodeInfluence = std::exp(-(r * r) / std::max(0.0025f, node.radius * node.radius * 1.45f)) * effectiveMass;
                if (nodeInfluence <= 0.0005f) {
                    continue;
                }

                cell.node = std::max(cell.node, ofClamp(nodeInfluence, 0.0f, 1.0f));
                cell.darkDensity = ofClamp(cell.darkDensity + nodeInfluence * 0.11f, 0.0f, 1.5f);
                cell.gasDensity = ofClamp(cell.gasDensity + nodeInfluence * 0.045f, 0.0f, 1.5f);
                cell.compression = std::max(cell.compression, ofClamp(nodeInfluence * 0.55f, 0.0f, 1.0f));
                cell.audioEnergy = std::max(cell.audioEnergy, node.audioCharge * nodeInfluence * 0.24f);
                cell.temperature = std::max(cell.temperature, ofClamp(node.heat * nodeInfluence + highs_ * audioDrive * 0.05f + node.audioCharge * 0.08f, 0.0f, 1.0f));
                cell.luminosity = std::max(cell.luminosity,
                                           ofClamp(nodeInfluence * (0.08f + cell.audioEnergy * 0.42f +
                                                                    node.audioCharge * 0.18f +
                                                                    level_ * audioDrive * paramAudioGlow_ * 0.060f),
                                                   0.0f, 1.0f));
            }
        }
    }

    for (int y = 0; y < kGrowthRows; ++y) {
        for (int x = 0; x < kGrowthCols; ++x) {
            GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
            const glm::vec2 p = growthFieldPoint(x, y);
            const float r = glm::length(p);
            const float edgeFade = 1.0f - smoothStep(kGrowthExtent * 0.82f, kGrowthExtent * 1.04f, r);
            const float mass = cell.darkDensity + cell.gasDensity * 0.62f + cell.stellarDensity * 1.20f +
                               cell.ridge * 0.78f + cell.node * 1.15f + cell.luminosity * 0.26f + cell.audioEnergy * 0.16f;
            cell.voidness = ofClamp((1.0f - smoothStep(0.08f, 0.46f, mass)) * edgeFade, 0.0f, 1.0f);
        }
    }
}

void CosmosFormationLayer::updateGrowthField(float dt, float timeSeconds, float transportSpeed) {
    if (growthField_.size() != static_cast<std::size_t>(kGrowthCols * kGrowthRows)) {
        resetGrowthField();
    }
    if (growthField_.empty() || clusters_.empty()) {
        return;
    }
    if (growthScratch_.size() != growthField_.size()) {
        growthScratch_ = growthField_;
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float audioGlow = audioDrive * paramAudioGlow_ * ofClamp(level_ * 0.52f + bass_ * 0.22f + mids_ * 0.14f + beatPulse_ * 0.16f, 0.0f, 1.4f);
    const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    const float emergence = smoothStep(0.02f, 0.62f, lifecycle);
    const float settling = smoothStep(0.20f, 1.0f, lifecycle);
    const float bloomFront = ofLerp(0.035f, 1.10f, smoothStep(0.0f, 0.78f, lifecycle));
    const float frontWidth = ofLerp(0.045f, 0.17f, emergence) * (1.0f + bass_ * audioDrive * 0.26f);
    const float globalTurn = age_ * 0.025f + timeSeconds * 0.008f;

    auto indexFor = [](int x, int y) {
        const int cx = std::max(0, std::min(kGrowthCols - 1, x));
        const int cy = std::max(0, std::min(kGrowthRows - 1, y));
        return static_cast<std::size_t>(cy * kGrowthCols + cx);
    };

    for (int y = 0; y < kGrowthRows; ++y) {
        for (int x = 0; x < kGrowthCols; ++x) {
            const std::size_t idx = indexFor(x, y);
            const GrowthCell& current = growthField_[idx];
            const glm::vec2 p = growthFieldPoint(x, y);
            const float r = glm::length(p);
            const float theta = std::atan2(p.y, p.x);
            const float edgeFade = 1.0f - smoothStep(kGrowthExtent * 0.82f, kGrowthExtent * 1.05f, r);
            const float noise = ofNoise(p.x * 2.4f + seedState_ * 0.011f,
                                        p.y * 2.4f - seedState_ * 0.017f,
                                        timeSeconds * 0.025f);
            const float fieldStructure = ofClamp(current.ridge * 0.58f + current.node * 0.82f + current.stellarDensity * 0.38f, 0.0f, 1.0f);

            const float spiral = spiralBand(theta * 4.0f - std::log(r + 0.045f) * 5.4f - globalTurn, 3.3f);
            const float broadDisk = std::exp(-(r * r) / 0.82f);
            const float earlyBloom = bell(r, bloomFront, frontWidth) * (1.0f - smoothStep(0.46f, 0.96f, lifecycle));
            const float core = bell(r, 0.0f, ofLerp(0.065f, 0.18f, emergence)) *
                               (1.0f - smoothStep(0.08f, 0.46f, lifecycle));
            float targetDark = (core * 0.18f + broadDisk * (0.018f + spiral * 0.026f) * settling) *
                               (0.78f + noise * 0.30f);
            float targetGas = (core * 0.52f + earlyBloom * 0.82f +
                               broadDisk * (0.016f + spiral * 0.030f) * settling) *
                              (0.76f + noise * 0.34f);
            float targetTemperature = earlyBloom * 0.9f + core * 0.55f;
            float targetSpin = 0.0f;

            targetDark = ofClamp(targetDark * edgeFade, 0.0f, 1.25f);
            targetGas = ofClamp(targetGas * edgeFade + bass_ * audioDrive * earlyBloom * 0.22f, 0.0f, 1.35f);
            targetTemperature = ofClamp(cosmicTemperature_ * targetTemperature +
                                        targetGas * 0.16f +
                                        highs_ * audioDrive * targetGas * 0.16f +
                                        beatPulse_ * earlyBloom * 0.22f,
                                        0.0f, 1.0f);
            targetSpin = ofClamp(targetSpin +
                                 current.node * paramClusterSwirl_ * 0.30f +
                                 current.ridge * paramShear_ * 0.08f +
                                 current.audioEnergy * mids_ * audioDrive * 0.32f,
                                 -1.0f, 1.0f);

            auto massAt = [&](int ix, int iy) {
                const GrowthCell& cell = growthField_[indexFor(ix, iy)];
                return cell.darkDensity + cell.gasDensity * 0.62f + cell.stellarDensity * 1.25f + cell.ridge * 0.76f + cell.node * 1.15f;
            };
            const float neighborDarkAverage = (growthField_[indexFor(x - 1, y)].darkDensity +
                                               growthField_[indexFor(x + 1, y)].darkDensity +
                                               growthField_[indexFor(x, y - 1)].darkDensity +
                                               growthField_[indexFor(x, y + 1)].darkDensity) * 0.25f;
            const float neighborGasAverage = (growthField_[indexFor(x - 1, y)].gasDensity +
                                              growthField_[indexFor(x + 1, y)].gasDensity +
                                              growthField_[indexFor(x, y - 1)].gasDensity +
                                              growthField_[indexFor(x, y + 1)].gasDensity) * 0.25f;
            const float neighborMassAverage = (massAt(x - 1, y) + massAt(x + 1, y) + massAt(x, y - 1) + massAt(x, y + 1)) * 0.25f;
            const float growthRate = ofClamp(dt * transportSpeed * (0.44f + emergence * 0.30f + bass_ * audioDrive * 0.16f), 0.0f, 1.0f);
            float darkDensity = ofLerp(current.darkDensity, targetDark, ofClamp(dt * (0.16f + settling * 0.10f), 0.0f, 1.0f));
            darkDensity = ofLerp(darkDensity, neighborDarkAverage, ofClamp(dt * 0.045f, 0.0f, 0.16f));
            darkDensity *= ofClamp(1.0f - dt * 0.006f, 0.0f, 1.0f);

            float gasDensity = ofLerp(current.gasDensity, targetGas, growthRate);
            gasDensity = ofLerp(gasDensity, neighborGasAverage, ofClamp(dt * (0.12f + (1.0f - settling) * 0.16f), 0.0f, 0.35f));
            gasDensity *= ofClamp(1.0f - dt * (0.022f + settling * 0.018f), 0.0f, 1.0f);

            GrowthCell& next = growthScratch_[idx];
            const float localMassTarget = darkDensity + gasDensity * 0.68f + current.stellarDensity * 1.25f;
            const float compressionTarget = ofClamp((localMassTarget - neighborMassAverage) * 0.85f +
                                                    darkDensity * gasDensity * 0.30f +
                                                    current.node * 0.30f + current.ridge * 0.16f,
                                                    0.0f, 1.0f);
            const float newGrowth = std::max(0.0f, gasDensity - current.gasDensity);
            const float audioInput = audioDrive * (bass_ * (0.05f + current.voidness * 0.18f) +
                                                   mids_ * (current.ridge * 0.30f + fieldStructure * 0.12f) +
                                                   highs_ * (current.node * 0.24f + current.stellarDensity * 0.18f) +
                                                   beatPulse_ * (current.node * 0.55f + current.ridge * 0.18f));
            const float audioDecay = std::pow(paramGlowPersistence_, dt * 30.0f);
            float audioEnergy = current.audioEnergy * audioDecay +
                                audioInput * dt * (0.65f + fieldStructure * 0.70f + paramAudioGlow_ * 0.16f);

            next.darkDensity = ofClamp(darkDensity, 0.0f, 1.25f);
            next.gasDensity = ofClamp(gasDensity, 0.0f, 1.25f);
            next.stellarDensity = ofClamp(current.stellarDensity, 0.0f, 1.0f);
            next.temperature = ofLerp(current.temperature, targetTemperature, ofClamp(dt * (0.18f + paramCoolingRate_ * 0.22f), 0.0f, 1.0f));
            next.spin = ofLerp(current.spin, targetSpin, ofClamp(dt * (0.18f + std::abs(paramShear_) * 0.05f + current.audioEnergy * 0.10f), 0.0f, 1.0f));
            next.compression = ofLerp(current.compression, compressionTarget, ofClamp(dt * (0.34f + settling * 0.20f), 0.0f, 1.0f));
            next.audioEnergy = ofClamp(audioEnergy, 0.0f, 1.0f);

            const float dense = smoothStep(0.26f, 0.82f, next.gasDensity + next.darkDensity * 0.16f + current.node * 0.30f + current.ridge * 0.12f);
            const float cool = 1.0f - smoothStep(0.38f, 0.86f, next.temperature);
            const float collapsing = smoothStep(0.02f, 0.18f, next.compression);
            const float starForm = dense * cool * collapsing * dt * (0.08f + highs_ * audioDrive * 0.06f + settling * 0.035f + next.audioEnergy * 0.050f);
            const float converted = std::min(next.gasDensity, starForm);
            next.gasDensity = ofClamp(next.gasDensity - converted, 0.0f, 1.25f);
            next.stellarDensity = ofClamp(next.stellarDensity + converted * 1.8f, 0.0f, 1.0f);
            const float luminositySource = converted * 7.5f +
                                           next.compression * next.stellarDensity * (0.10f + fieldStructure * 0.26f) +
                                           current.node * next.audioEnergy * 0.62f +
                                           current.ridge * next.audioEnergy * 0.20f +
                                           current.stellarDensity * audioGlow * 0.16f +
                                           current.bloom * (0.08f + audioGlow * 0.06f);
            const float luminosityDecay = std::pow(paramGlowPersistence_, dt * 22.0f);
            next.luminosity = ofClamp(current.luminosity * luminosityDecay +
                                      luminositySource * dt * paramFieldLuminosity_,
                                      0.0f, 1.35f);
            next.bloom = ofClamp(current.bloom * ofClamp(1.0f - dt * (0.70f + settling * 0.22f), 0.0f, 1.0f) +
                                 newGrowth * 3.2f + earlyBloom * bangEnergy_ * dt * 0.65f +
                                 converted * 8.0f +
                                 highs_ * audioDrive * (next.gasDensity + next.stellarDensity) * dt * 0.18f +
                                 next.luminosity * dt * 0.30f,
                                 0.0f, 1.0f);
        }
    }

    growthField_.swap(growthScratch_);
    paintCosmicWebIntoField(dt, timeSeconds);
}

void CosmosFormationLayer::updateMatter(float dt, float timeSeconds, float transportSpeed) {
    if (matter_.empty()) {
        return;
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float effectiveTurbulence = paramTurbulence_ + mids_ * paramMidsTurbulence_ * audioDrive;
    const float bassPush = bass_ * paramBassExpansion_ * audioDrive;
    const float waveformDrive = hasWaveform_ ? paramWaveformWarp_ * audioDrive * (0.35f + level_) : 0.0f;
    const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    const float gravityPhase = smoothStep(paramGravityDelay_, 1.0f, lifecycle);
    const float expansionPhase = 1.0f - smoothStep(0.08f, 0.92f, lifecycle);

    for (std::size_t i = 0; i < matter_.size(); ++i) {
        MatterNode& node = matter_[i];
        node.prev = node.pos;

        const float formation = formationAmount(node);
        const glm::vec2 radial = safeNormalize(node.pos, glm::vec2(std::cos(node.angle), std::sin(node.angle)));
        const float outer = glm::length(node.pos);
        const bool outsideField = std::abs(node.pos.x) > kGrowthExtent * 1.04f ||
                                  std::abs(node.pos.y) > kGrowthExtent * 1.04f ||
                                  outer > kGrowthExtent * 1.12f;
        if (outsideField) {
            glm::vec2 force = -radial * (0.18f + std::max(0.0f, outer - kGrowthExtent * 0.86f) * 1.35f);
            force += radial * paramVoidPressure_ * (1.0f - formation) * 0.018f;
            node.vel += force * dt * transportSpeed;
            node.vel *= ofClamp(1.0f - dt * 1.15f, 0.0f, 1.0f);
            node.pos += node.vel * dt * transportSpeed;
            if (glm::length(node.pos) > kGrowthExtent * 1.18f) {
                node.pos = safeNormalize(node.pos) * kGrowthExtent * 1.18f;
                node.vel *= 0.35f;
            }
            node.heat = ofLerp(node.heat, 0.16f, ofClamp(dt * (0.12f + paramCoolingRate_ * 0.4f), 0.0f, 1.0f));
            continue;
        }

        const float pressure = pressureForPoint(node.pos);
        const float sample = waveformDrive > 0.0f ? waveformSampleFor(i) : 0.0f;
        const GrowthSample field = sampleGrowthField(node.pos);
        const float fieldMass = ofClamp(field.darkDensity + field.gasDensity * 0.62f + field.stellarDensity * 1.25f +
                                        field.ridge * 0.76f + field.node * 1.15f, 0.0f, 1.8f);
        const float luminousField = ofClamp(field.gasDensity * 0.36f + field.stellarDensity * 0.70f +
                                           field.ridge * 0.42f + field.node * 0.65f + field.bloom * 0.28f +
                                           field.luminosity * 0.75f + field.audioEnergy * 0.25f,
                                           0.0f, 1.0f);
        const glm::vec2 gradientDir = safeNormalize(field.gradient, radial);
        const glm::vec2 filamentDir = safeNormalize(field.filamentDir, safeNormalize(field.flow, gradientDir));
        const float collapseStrength = ofClamp(field.darkDensity + field.gasDensity * 0.55f + field.node * 1.20f, 0.0f, 1.5f);
        const float webStructure = ofClamp(field.ridge * 0.70f + field.node * 0.95f + field.stellarDensity * 0.55f + field.luminosity * 0.30f, 0.0f, 1.0f);

        glm::vec2 force(0.0f, 0.0f);
        force += radial * paramExpansionForce_ * node.speed * (0.06f + bangEnergy_ * 0.70f) * (0.10f + expansionPhase * 0.48f);
        force += gradientDir * paramGravity_ * node.mass * gravityPhase * collapseStrength * (0.050f + formation * 0.040f);
        force += filamentDir * field.ridge * (0.060f + mids_ * audioDrive * 0.035f + level_ * audioDrive * 0.012f);
        force += field.flow * (0.11f + mids_ * audioDrive * 0.060f + field.audioEnergy * 0.050f);
        force += radial * paramVoidPressure_ * field.voidness * (0.025f + expansionPhase * 0.025f);
        force += radial * bassPush * (0.08f + field.voidness * 0.12f + (1.0f - formation) * 0.12f);
        force += filamentDir * sample * waveformDrive * (0.20f + field.ridge * 0.35f);
        force += gradientDir * (field.bloom + field.luminosity * 0.45f) * (0.022f + bass_ * audioDrive * 0.030f);

        const float localSwirl = (field.node + field.luminosity * 0.42f + field.audioEnergy * 0.22f) * smoothStep(0.32f, 0.95f, fieldMass);
        const glm::vec2 nodeTangent(-gradientDir.y, gradientDir.x);
        force += nodeTangent * paramClusterSwirl_ * node.spin * localSwirl *
                 (0.020f + mids_ * audioDrive * 0.020f + field.audioEnergy * 0.020f);

        const float nx = node.pos.x * 1.7f + node.seed * 0.013f + timeSeconds * 0.055f;
        const float ny = node.pos.y * 1.7f + node.seed * 0.017f - timeSeconds * 0.047f;
        const float noiseAngle = ofNoise(nx, ny, node.seed * 0.007f) * TWO_PI * 2.0f;
        force += glm::vec2(std::cos(noiseAngle), std::sin(noiseAngle)) *
                 effectiveTurbulence * (0.018f + field.voidness * 0.034f + field.ridge * 0.020f + (1.0f - formation) * 0.025f);

        if (pressure > 0.0f) {
            force += radial * pressure * paramPressureAmount_ * (0.25f + bangEnergy_ * 0.35f + bass_ * audioDrive * 0.18f);
        }

        if (outer > 1.32f) {
            force -= safeNormalize(node.pos) * (outer - 1.32f) * 1.35f;
        }

        node.vel += force * dt * transportSpeed;
        node.vel *= ofClamp(1.0f - dt * (0.42f + gravityPhase * 0.34f + fieldMass * 0.24f + webStructure * 0.20f), 0.0f, 1.0f);
        node.pos += node.vel * dt * transportSpeed;

        const float heatTarget = ofClamp((1.0f - formation) * 0.42f +
                                         field.temperature * 0.34f + field.bloom * 0.26f + luminousField * 0.22f +
                                         field.node * 0.18f + pressure * 0.24f + highs_ * audioDrive * 0.18f,
                                         0.03f, 1.0f);
        node.heat = ofLerp(node.heat, heatTarget, ofClamp(dt * (0.18f + paramCoolingRate_), 0.0f, 1.0f));
    }
}

void CosmosFormationLayer::updatePressureWaves(float dt) {
    for (auto& wave : waves_) {
        wave.age += dt;
    }
    waves_.erase(std::remove_if(waves_.begin(), waves_.end(), [&](const PressureWave& wave) {
        return wave.age * paramShockwaveSpeed_ > 1.8f || wave.strength <= 0.001f;
    }), waves_.end());
}

void CosmosFormationLayer::drawBackground(float width, float height, float alpha) const {
    if (paramBgAlpha_ <= 0.0f) {
        return;
    }
    setColor(colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * alpha));
    ofDrawRectangle(0.0f, 0.0f, width, height);
}

void CosmosFormationLayer::drawGrowthField(float width, float height, float minDim, float alpha, float timeSeconds) const {
    if (growthField_.size() != static_cast<std::size_t>(kGrowthCols * kGrowthRows)) {
        return;
    }

    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const glm::vec2 scale = screenScale(width, height);
    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float audioGlow = audioDrive * paramAudioGlow_ * ofClamp(level_ * 0.55f + bass_ * 0.22f + mids_ * 0.14f + beatPulse_ * 0.16f, 0.0f, 1.4f);
    const float audioTwinkle = audioDrive * paramAudioTwinkle_ * ofClamp(highs_ * 0.78f + peak_ * 0.24f + beatPulse_ * 0.18f, 0.0f, 1.4f);
    const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    const float fieldAlpha = alpha * ofClamp(0.32f + smoothStep(0.02f, 0.80f, lifecycle) * 0.68f, 0.0f, 1.0f);
    const ofFloatColor hotColor = colorFrom(paramHotR_, paramHotG_, paramHotB_, 1.0f);
    const ofFloatColor matterColor = colorFrom(paramMatterR_, paramMatterG_, paramMatterB_, 1.0f);
    const ofFloatColor coolColor = colorFrom(paramCoolR_, paramCoolG_, paramCoolB_, 1.0f);
    const ofFloatColor clusterColor = colorFrom(paramClusterR_, paramClusterG_, paramClusterB_, 1.0f);
    const ofFloatColor waveColor = colorFrom(paramWaveR_, paramWaveG_, paramWaveB_, 1.0f);

    ofSetLineWidth(std::max(0.6f, minDim * 0.0012f));
    for (const auto& edge : filamentEdges_) {
        if (edge.a < 0 || edge.b < 0 ||
            edge.a >= static_cast<int>(webNodes_.size()) ||
            edge.b >= static_cast<int>(webNodes_.size())) {
            continue;
        }
        const glm::vec2 aField = webNodes_[static_cast<std::size_t>(edge.a)].pos;
        const glm::vec2 bField = webNodes_[static_cast<std::size_t>(edge.b)].pos;
        const float edgeActive = smoothStep(0.015f, 0.86f, edge.activation);
        const float edgeVisible = ofClamp(edge.conductivity * 0.72f + edge.massFlow * 0.22f + edge.audioCharge * 0.18f, 0.0f, 1.0f);
        if (edgeVisible <= 0.010f) {
            continue;
        }
        ofFloatColor glow = coolColor.getLerped(waveColor, ofClamp(edgeVisible * 0.28f + highs_ * audioDrive * 0.06f + edge.audioCharge * 0.18f, 0.0f, 1.0f));
        glow.a = ofClamp(alpha * paramHaloAlpha_ * edgeVisible * (0.020f + edgeActive * 0.040f + level_ * audioDrive * 0.018f), 0.0f, 0.22f);
        setColor(glow);
        ofSetLineWidth(std::max(0.55f, minDim * (0.0008f + edgeVisible * 0.0024f)));
        glm::vec2 previous = screenPoint(organicFilamentPoint(aField, bField, edge.bend, 0.0f, edge.a, edge.b, seedState_), width, height);
        constexpr int kDrawSegments = 12;
        for (int segment = 1; segment <= kDrawSegments; ++segment) {
            const float t = static_cast<float>(segment) / static_cast<float>(kDrawSegments);
            const glm::vec2 current = screenPoint(organicFilamentPoint(aField, bField, edge.bend, t, edge.a, edge.b, seedState_), width, height);
            ofDrawLine(previous.x, previous.y, current.x, current.y);
            previous = current;
        }

        glow = matterColor.getLerped(clusterColor, 0.28f);
        glow.a = ofClamp(alpha * paramHaloAlpha_ * edgeVisible * 0.026f, 0.0f, 0.13f);
        setColor(glow);
        ofSetLineWidth(std::max(0.45f, minDim * 0.0009f));
        previous = screenPoint(organicFilamentPoint(aField, bField, edge.bend, 0.0f, edge.a, edge.b, seedState_), width, height);
        for (int segment = 1; segment <= kDrawSegments; ++segment) {
            const float t = static_cast<float>(segment) / static_cast<float>(kDrawSegments);
            const glm::vec2 current = screenPoint(organicFilamentPoint(aField, bField, edge.bend, t, edge.a, edge.b, seedState_), width, height);
            ofDrawLine(previous.x, previous.y, current.x, current.y);
            previous = current;
        }
    }

    for (int y = 1; y < kGrowthRows - 1; y += 1) {
        for (int x = 1; x < kGrowthCols - 1; x += 1) {
            const GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
            const float filamentVisible = cell.ridge * 0.88f;
            const float nodeVisible = cell.node * 0.98f;
            const float gasVisible = cell.gasDensity * 0.28f;
            const float starsVisible = cell.stellarDensity * 0.55f;
            const float birthVisible = cell.bloom * 0.42f;
            const float luminosityVisible = cell.luminosity * (0.98f + audioGlow * 0.34f) + cell.audioEnergy * (0.24f + audioGlow * 0.16f);
            const float visible = ofClamp(filamentVisible + nodeVisible + gasVisible + starsVisible + birthVisible + luminosityVisible, 0.0f, 1.0f);
            if (visible <= 0.006f) {
                continue;
            }

            const glm::vec2 p = growthFieldPoint(x, y);
            const float jitterPhase = ofNoise(p.x * 7.0f + seedState_ * 0.003f,
                                              p.y * 7.0f - seedState_ * 0.004f,
                                              timeSeconds * 0.06f) * TWO_PI;
            const glm::vec2 jitter(std::cos(jitterPhase), std::sin(jitterPhase * 1.17f));
            const glm::vec2 warped = p + jitter * (0.0045f + cell.bloom * 0.006f + cell.ridge * 0.003f + audioTwinkle * 0.0015f);
            const glm::vec2 pos(center.x + warped.x * scale.x,
                                center.y + warped.y * scale.y);
            const float mass = ofClamp(cell.darkDensity + cell.gasDensity * 0.62f + cell.stellarDensity * 1.25f +
                                       cell.ridge * 0.76f + cell.node * 1.15f, 0.0f, 1.8f);
            const float radius = minDim * (0.0022f + cell.ridge * 0.0034f + cell.node * 0.013f +
                                           cell.gasDensity * 0.0045f + cell.stellarDensity * 0.0055f +
                                           cell.bloom * 0.006f + cell.luminosity * 0.009f) *
                                 (1.0f + bass_ * audioDrive * 0.10f + cell.audioEnergy * 0.12f +
                                  audioGlow * 0.14f + audioTwinkle * 0.045f);

            ofFloatColor color = coolColor.getLerped(matterColor, ofClamp(cell.ridge * 0.52f + cell.gasDensity * 0.34f, 0.0f, 1.0f));
            color = color.getLerped(clusterColor, ofClamp(cell.node * 0.58f + mass * 0.12f + cell.stellarDensity * 0.20f, 0.0f, 1.0f));
            color = color.getLerped(hotColor, ofClamp(cell.temperature * 0.50f + cell.bloom * 0.26f + cell.luminosity * 0.30f, 0.0f, 1.0f));
            color = color.getLerped(waveColor, ofClamp(highs_ * audioDrive * 0.08f + beatPulse_ * 0.08f + cell.audioEnergy * 0.18f, 0.0f, 1.0f));
            color.a = ofClamp(fieldAlpha * paramHaloAlpha_ * (0.032f + visible * (0.205f + audioGlow * 0.035f)), 0.0f, 0.44f);
            setColor(color);
            ofDrawCircle(pos.x, pos.y, radius);

            if (cell.bloom > 0.20f || cell.luminosity > 0.08f) {
                color.a = ofClamp(fieldAlpha * paramHaloAlpha_ * (cell.bloom * 0.065f + cell.luminosity * (0.105f + audioGlow * 0.040f)), 0.0f, 0.30f);
                setColor(color);
                ofDrawCircle(pos.x, pos.y, radius * (2.35f + cell.luminosity * 1.8f + audioGlow * 0.30f));
            }

            if (cell.node > 0.10f || cell.luminosity > 0.14f) {
                ofFloatColor nodeColor = clusterColor.getLerped(hotColor, ofClamp(cell.temperature * 0.25f + cell.bloom * 0.18f + cell.luminosity * 0.42f, 0.0f, 1.0f));
                nodeColor = nodeColor.getLerped(waveColor, ofClamp(cell.audioEnergy * 0.22f, 0.0f, 1.0f));
                nodeColor.a = ofClamp(fieldAlpha * paramHaloAlpha_ * (cell.node * (0.14f + audioGlow * 0.030f) +
                                                                      cell.luminosity * (0.20f + audioGlow * 0.050f)),
                                      0.0f, 0.42f);
                setColor(nodeColor);
                ofDrawCircle(pos.x, pos.y, radius * (2.7f + cell.node * 2.1f + cell.luminosity * 2.2f + audioGlow * 0.35f));
            }

            if (cell.stellarDensity > 0.12f) {
                ofFloatColor starColor = matterColor.getLerped(hotColor, ofClamp(cell.temperature * 0.35f + cell.bloom * 0.18f, 0.0f, 1.0f));
                starColor.a = ofClamp(fieldAlpha * paramHaloAlpha_ * cell.stellarDensity * 0.075f, 0.0f, 0.18f);
                setColor(starColor);
                ofDrawCircle(pos.x, pos.y, radius * 0.62f);
            }
        }
    }
}

void CosmosFormationLayer::drawPressureWaves(float width, float height, float minDim, float alpha) const {
    if (paramShockwaveAlpha_ <= 0.0f) {
        return;
    }

    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const glm::vec2 scale = screenScale(width, height);
    const float radialScale = std::min(scale.x, scale.y);
    const ofFloatColor waveColor = colorFrom(paramWaveR_, paramWaveG_, paramWaveB_, 1.0f);
    const ofFloatColor hotColor = colorFrom(paramHotR_, paramHotG_, paramHotB_, 1.0f);

    ofNoFill();
    const int steadyCount = static_cast<int>(std::round(paramShockwaveCount_));
    for (int i = 0; i < steadyCount; ++i) {
        const float phase = wrap01(age_ * paramShockwaveSpeed_ * 0.18f + static_cast<float>(i) / static_cast<float>(std::max(1, steadyCount)));
        const float radius = std::pow(phase, 0.74f) * radialScale * (1.0f + bangEnergy_ * 0.14f);
        const float fade = (1.0f - smoothStep(0.68f, 1.0f, phase)) * smoothStep(0.0f, 0.08f, phase);
        ofFloatColor color = waveColor.getLerped(hotColor, ofClamp(bangEnergy_ * 0.32f + (1.0f - phase) * 0.15f, 0.0f, 1.0f));
        color.a = ofClamp(alpha * paramShockwaveAlpha_ * fade * (0.18f + bangEnergy_ * 0.45f + beatPulse_ * 0.25f), 0.0f, 1.0f);
        if (color.a <= 0.002f) {
            continue;
        }
        setColor(color);
        ofSetLineWidth(std::max(0.75f, paramShockwaveWidth_ * minDim * (0.55f + (1.0f - phase) * 0.55f)));
        ofDrawCircle(center.x, center.y, radius);
    }

    for (const auto& wave : waves_) {
        const float waveRadius = wave.age * paramShockwaveSpeed_;
        const float fade = std::exp(-wave.age * 1.28f) * (1.0f - smoothStep(1.25f, 1.8f, waveRadius));
        if (fade <= 0.001f) {
            continue;
        }
        glm::vec2 origin = screenPoint(wave.origin, width, height);
        ofFloatColor color = hotColor.getLerped(waveColor, ofClamp(wave.age * 0.8f, 0.0f, 1.0f));
        color.a = ofClamp(alpha * paramShockwaveAlpha_ * fade * wave.strength, 0.0f, 1.0f);
        setColor(color);
        ofSetLineWidth(std::max(0.8f, paramShockwaveWidth_ * minDim * 0.85f));
        ofDrawCircle(origin.x, origin.y, waveRadius * radialScale);
    }
    ofFill();
}

void CosmosFormationLayer::drawDensityHalos(float width, float height, float minDim, float alpha, float timeSeconds) const {
    if (paramHaloAlpha_ <= 0.0f || growthField_.size() != static_cast<std::size_t>(kGrowthCols * kGrowthRows)) {
        return;
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float audioGlow = audioDrive * paramAudioGlow_ * ofClamp(level_ * 0.55f + bass_ * 0.24f + beatPulse_ * 0.16f, 0.0f, 1.4f);
    const float maturity = ofClamp(0.30f + smoothStep(0.06f, 0.92f, age_ / std::max(0.01f, paramFormationTime_)) * 0.70f,
                                   0.0f,
                                   1.0f);
    const ofFloatColor coolColor = colorFrom(paramCoolR_, paramCoolG_, paramCoolB_, 1.0f);
    const ofFloatColor clusterColor = colorFrom(paramClusterR_, paramClusterG_, paramClusterB_, 1.0f);
    const ofFloatColor waveColor = colorFrom(paramWaveR_, paramWaveG_, paramWaveB_, 1.0f);

    for (int y = 1; y < kGrowthRows - 1; y += 2) {
        for (int x = 1; x < kGrowthCols - 1; x += 2) {
            const GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
            const float glow = ofClamp(cell.luminosity * (0.95f + audioGlow * 0.34f) +
                                       cell.node * 0.42f + cell.audioEnergy * (0.30f + audioGlow * 0.18f) +
                                       cell.stellarDensity * 0.18f,
                                       0.0f, 1.0f);
            if (glow <= 0.030f) {
                continue;
            }

            const glm::vec2 p = growthFieldPoint(x, y);
            const glm::vec2 pos = screenPoint(p, width, height);
            if (pos.x < -140.0f || pos.x > width + 140.0f || pos.y < -140.0f || pos.y > height + 140.0f) {
                continue;
            }

            const float pulse = ofNoise(p.x * 5.3f + seedState_ * 0.001f,
                                        p.y * 5.3f - seedState_ * 0.0013f,
                                        timeSeconds * 0.10f) * 0.08f +
                                cell.audioEnergy * 0.16f + level_ * audioDrive * 0.05f;
            const float radius = minDim * paramHaloRadius_ * (0.36f + glow * 1.42f + pulse + audioGlow * 0.12f);
            for (int ring = 3; ring >= 1; --ring) {
                const float t = static_cast<float>(ring) / 3.0f;
                ofFloatColor color = coolColor.getLerped(clusterColor, 0.24f + cell.node * 0.42f + cell.luminosity * 0.20f);
                color = color.getLerped(waveColor, ofClamp(cell.audioEnergy * 0.32f + beatPulse_ * 0.10f + highs_ * audioDrive * 0.04f, 0.0f, 1.0f));
                color.a = ofClamp(alpha * paramHaloAlpha_ * maturity * glow *
                                  (0.014f + 0.034f * t + audioGlow * 0.006f), 0.0f, 0.24f);
                setColor(color);
                ofDrawCircle(pos.x, pos.y, radius * (0.75f + t * 2.1f));
            }
            ofFloatColor core = clusterColor.getLerped(waveColor, ofClamp(cell.audioEnergy * 0.24f, 0.0f, 1.0f));
            core.a = ofClamp(alpha * paramHaloAlpha_ * glow * (0.15f + audioGlow * 0.030f), 0.0f, 0.34f);
            setColor(core);
            ofDrawCircle(pos.x, pos.y, radius * 0.34f);
        }
    }
}

void CosmosFormationLayer::drawCore(float width, float height, float minDim, float alpha) const {
    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const ofFloatColor hotColor = colorFrom(paramHotR_, paramHotG_, paramHotB_, 1.0f);
    const ofFloatColor waveColor = colorFrom(paramWaveR_, paramWaveG_, paramWaveB_, 1.0f);
    const float youth = 1.0f - smoothStep(0.0f, std::max(0.1f, paramFormationTime_ * 0.26f), age_);
    const float glow = ofClamp(youth * 0.55f + bangEnergy_ * 0.65f + beatPulse_ * 0.18f + level_ * paramAudioAmount_ * 0.12f, 0.0f, 1.0f);
    if (glow <= 0.001f) {
        return;
    }

    const float coreRadius = minDim * (paramOriginRadius_ * 0.9f + glow * 0.058f) * ofLerp(0.55f, 1.0f, scaleFactor_);
    for (int i = 4; i >= 1; --i) {
        const float t = static_cast<float>(i) / 4.0f;
        ofFloatColor color = hotColor.getLerped(waveColor, 0.18f + t * 0.24f);
        color.a = ofClamp(alpha * glow * (0.045f + 0.075f * t), 0.0f, 0.75f);
        setColor(color);
        ofDrawCircle(center.x, center.y, coreRadius * (1.0f + t * 4.0f));
    }
}

void CosmosFormationLayer::drawTrails(float width, float height, float minDim, float alpha) const {
    if (paramTrailAlpha_ <= 0.0f || matter_.empty()) {
        return;
    }

    const ofFloatColor hotColor = colorFrom(paramHotR_, paramHotG_, paramHotB_, 1.0f);
    const ofFloatColor matterColor = colorFrom(paramMatterR_, paramMatterG_, paramMatterB_, 1.0f);
    const ofFloatColor coolColor = colorFrom(paramCoolR_, paramCoolG_, paramCoolB_, 1.0f);
    const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
    const float trailPhase = ofLerp(1.0f, 0.38f, smoothStep(0.42f, 1.0f, lifecycle));

    const std::size_t stride = matter_.size() > 2200 ? 2 : 1;
    for (std::size_t i = 0; i < matter_.size(); i += stride) {
        const MatterNode& node = matter_[i];
        const glm::vec2 a = screenPoint(node.prev, width, height);
        const glm::vec2 b = screenPoint(node.pos, width, height);
        if ((a.x < -80.0f && b.x < -80.0f) ||
            (a.x > width + 80.0f && b.x > width + 80.0f) ||
            (a.y < -80.0f && b.y < -80.0f) ||
            (a.y > height + 80.0f && b.y > height + 80.0f)) {
            continue;
        }
        const float travel = glm::distance(a, b);
        if (travel <= 0.01f) {
            continue;
        }
        ofFloatColor color = coolColor.getLerped(matterColor, ofClamp(node.heat * 0.72f + 0.18f, 0.0f, 1.0f));
        color = color.getLerped(hotColor, ofClamp(node.heat * node.heat, 0.0f, 1.0f));
        const float trailAlpha = ofClamp(alpha * paramTrailAlpha_ * trailPhase * node.brightness *
                                         (0.16f + node.heat * 0.42f + bangEnergy_ * 0.24f), 0.0f, 1.0f);
        if (trailAlpha <= 0.002f) {
            continue;
        }

        const glm::vec2 mid = (a + b) * 0.5f;
        const float smear = std::max(0.35f, paramTrailThickness_ * (0.55f + std::min(5.0f, travel) * 0.10f));
        color.a = trailAlpha * 0.22f;
        setColor(color);
        ofDrawCircle(mid.x, mid.y, smear * 2.6f);
        color.a = trailAlpha * 0.34f;
        setColor(color);
        ofDrawCircle(b.x, b.y, smear * 1.25f);
    }
}

void CosmosFormationLayer::drawMatter(float width, float height, float minDim, float alpha, float timeSeconds) const {
    if (paramMatterSize_ <= 0.0f || matter_.empty()) {
        return;
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float audioGlow = audioDrive * paramAudioGlow_ * ofClamp(level_ * 0.52f + bass_ * 0.20f + mids_ * 0.14f + beatPulse_ * 0.16f, 0.0f, 1.4f);
    const float audioTwinkle = audioDrive * paramAudioTwinkle_ * ofClamp(highs_ * 0.82f + peak_ * 0.26f + beatPulse_ * 0.20f, 0.0f, 1.4f);
    const float sparkle = highs_ * paramHighsSparkle_ * audioDrive * (0.45f + paramAudioTwinkle_ * 0.55f) +
                          beatPulse_ * (0.18f + paramAudioTwinkle_ * 0.08f) +
                          bangEnergy_ * 0.22f;
    const ofFloatColor hotColor = colorFrom(paramHotR_, paramHotG_, paramHotB_, 1.0f);
    const ofFloatColor matterColor = colorFrom(paramMatterR_, paramMatterG_, paramMatterB_, 1.0f);
    const ofFloatColor coolColor = colorFrom(paramCoolR_, paramCoolG_, paramCoolB_, 1.0f);
    const ofFloatColor clusterColor = colorFrom(paramClusterR_, paramClusterG_, paramClusterB_, 1.0f);
    const ofFloatColor waveColor = colorFrom(paramWaveR_, paramWaveG_, paramWaveB_, 1.0f);

    for (std::size_t i = 0; i < matter_.size(); ++i) {
        const MatterNode& node = matter_[i];
        const glm::vec2 pos = screenPoint(node.pos, width, height);
        if (pos.x < -80.0f || pos.x > width + 80.0f || pos.y < -80.0f || pos.y > height + 80.0f) {
            continue;
        }

        const float pressure = pressureForPoint(node.pos);
        const GrowthSample field = sampleGrowthField(node.pos);
        const float fieldMass = ofClamp(field.darkDensity + field.gasDensity * 0.62f + field.stellarDensity * 1.25f +
                                        field.ridge * 0.76f + field.node * 1.15f, 0.0f, 1.8f);
        const float luminousField = ofClamp(field.gasDensity * 0.36f + field.stellarDensity * 0.70f +
                                           field.ridge * 0.46f + field.node * 0.68f + field.bloom * 0.28f,
                                           0.0f, 1.0f);
        const float fieldGlow = ofClamp(luminousField + field.luminosity * 0.85f + field.audioEnergy * 0.22f,
                                           0.0f, 1.0f);
        const float structure = ofClamp(field.ridge * 0.62f + field.node * 0.92f + field.stellarDensity * 0.68f + field.luminosity * 0.34f, 0.0f, 1.0f);
        const float twinkleNoise = ofNoise(node.seed * 0.011f, timeSeconds * (0.24f + paramHighsSparkle_ * 0.08f + audioTwinkle * 0.18f));
        const float twinkle = ofClamp(twinkleNoise * (0.62f + audioTwinkle * 0.38f) + audioTwinkle * 0.18f, 0.0f, 1.45f);
        ofFloatColor color = coolColor.getLerped(matterColor, ofClamp(node.heat * 0.65f + 0.18f, 0.0f, 1.0f));
        color = color.getLerped(hotColor, ofClamp(node.heat * node.heat + field.luminosity * 0.28f, 0.0f, 1.0f));
        color = color.getLerped(clusterColor, ofClamp((field.node * 0.55f + fieldMass * 0.16f + field.stellarDensity * 0.18f) *
                                                      (0.16f + (1.0f - node.heat) * 0.20f), 0.0f, 1.0f));
        color = color.getLerped(waveColor, ofClamp(pressure * 0.40f + sparkle * 0.06f + field.bloom * 0.10f +
                                                   field.ridge * 0.10f + field.audioEnergy * 0.24f,
                                                   0.0f, 1.0f));

        const float glowDrive = ofClamp(node.brightness * (0.30f + twinkle * 0.30f + sparkle * 0.12f +
                                                           pressure * 0.40f + field.temperature * 0.26f +
                                                           field.bloom * 0.26f + fieldGlow * (0.30f + audioGlow * 0.10f) +
                                                           field.node * 0.12f + audioGlow * 0.18f),
                                        0.0f, 1.0f);
        const float nodeAlpha = alpha * ofClamp(0.20f + structure * 0.78f + node.brightness * 0.12f +
                                                audioTwinkle * 0.035f + audioGlow * 0.035f,
                                                0.0f, 1.0f);
        const float radius = std::max(0.20f, paramMatterSize_ * node.size *
                                             (0.34f + twinkle * 0.18f + fieldGlow * 0.18f + structure * 0.20f) *
                                             (1.0f + sparkle * 0.09f + pressure * 0.16f + field.bloom * 0.16f +
                                              field.audioEnergy * 0.12f + audioGlow * 0.10f));
        if (nodeAlpha <= 0.002f) {
            continue;
        }

        if (paramMatterGlow_ > 0.0f && (structure > 0.035f || fieldGlow > 0.050f || audioGlow > 0.020f)) {
            ofFloatColor glow = color;
            glow.a = ofClamp(alpha * glowDrive * (0.032f + pressure * 0.045f + structure * 0.078f +
                                                  fieldGlow * 0.070f + audioGlow * 0.030f),
                             0.0f, 0.48f);
            setColor(glow);
            ofDrawCircle(pos.x, pos.y, radius * paramMatterGlow_ * (1.0f + audioGlow * 0.16f));
        }

        color.a = nodeAlpha;
        setColor(color);
        ofDrawCircle(pos.x, pos.y, radius);
    }
}

glm::vec2 CosmosFormationLayer::growthFieldPoint(int x, int y) const {
    const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(kGrowthCols);
    const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(kGrowthRows);
    return glm::vec2((u * 2.0f - 1.0f) * kGrowthExtent,
                     (v * 2.0f - 1.0f) * kGrowthExtent);
}

glm::vec2 CosmosFormationLayer::screenScale(float width, float height) const {
    const float lifecycleScale = ofLerp(0.50f, 1.0f, scaleFactor_);
    return glm::vec2(width * 0.5f * paramRadius_ * lifecycleScale / kGrowthExtent,
                     height * 0.5f * paramRadius_ * lifecycleScale / kGrowthExtent);
}

glm::vec2 CosmosFormationLayer::screenPoint(const glm::vec2& point, float width, float height) const {
    const glm::vec2 scale = screenScale(width, height);
    return glm::vec2(width * 0.5f + point.x * scale.x,
                     height * 0.5f + point.y * scale.y);
}

CosmosFormationLayer::GrowthSample CosmosFormationLayer::sampleGrowthField(const glm::vec2& point) const {
    GrowthSample sample;
    if (growthField_.size() != static_cast<std::size_t>(kGrowthCols * kGrowthRows)) {
        return sample;
    }

    auto indexFor = [](int ix, int iy) {
        const int cx = std::max(0, std::min(kGrowthCols - 1, ix));
        const int cy = std::max(0, std::min(kGrowthRows - 1, iy));
        return static_cast<std::size_t>(cy * kGrowthCols + cx);
    };

    const float u = ofClamp((point.x / kGrowthExtent + 1.0f) * 0.5f, 0.0f, 0.999f);
    const float v = ofClamp((point.y / kGrowthExtent + 1.0f) * 0.5f, 0.0f, 0.999f);
    const float gridX = u * static_cast<float>(kGrowthCols) - 0.5f;
    const float gridY = v * static_cast<float>(kGrowthRows) - 0.5f;
    const int x0 = std::max(0, std::min(kGrowthCols - 1, static_cast<int>(std::floor(gridX))));
    const int y0 = std::max(0, std::min(kGrowthRows - 1, static_cast<int>(std::floor(gridY))));
    const int x1 = std::max(0, std::min(kGrowthCols - 1, x0 + 1));
    const int y1 = std::max(0, std::min(kGrowthRows - 1, y0 + 1));
    const float tx = ofClamp(gridX - static_cast<float>(x0), 0.0f, 1.0f);
    const float ty = ofClamp(gridY - static_cast<float>(y0), 0.0f, 1.0f);
    const int x = std::max(0, std::min(kGrowthCols - 1, static_cast<int>(std::round(gridX))));
    const int y = std::max(0, std::min(kGrowthRows - 1, static_cast<int>(std::round(gridY))));

    auto addCell = [&](const GrowthCell& cell, float weight) {
        sample.darkDensity += cell.darkDensity * weight;
        sample.gasDensity += cell.gasDensity * weight;
        sample.stellarDensity += cell.stellarDensity * weight;
        sample.temperature += cell.temperature * weight;
        sample.spin += cell.spin * weight;
        sample.bloom += cell.bloom * weight;
        sample.compression += cell.compression * weight;
        sample.luminosity += cell.luminosity * weight;
        sample.audioEnergy += cell.audioEnergy * weight;
        sample.ridge += cell.ridge * weight;
        sample.node += cell.node * weight;
        sample.voidness += cell.voidness * weight;
        sample.filamentDir += cell.filamentDir * weight;
    };
    addCell(growthField_[indexFor(x0, y0)], (1.0f - tx) * (1.0f - ty));
    addCell(growthField_[indexFor(x1, y0)], tx * (1.0f - ty));
    addCell(growthField_[indexFor(x0, y1)], (1.0f - tx) * ty);
    addCell(growthField_[indexFor(x1, y1)], tx * ty);
    sample.filamentDir = safeNormalize(sample.filamentDir, glm::vec2(0.0f, 0.0f));

    auto massAt = [&](int ix, int iy) {
        const GrowthCell& c = growthField_[indexFor(ix, iy)];
        return c.darkDensity + c.gasDensity * 0.62f + c.stellarDensity * 1.25f +
               c.ridge * 0.76f + c.node * 1.15f + c.luminosity * 0.32f + c.audioEnergy * 0.18f;
    };
    const float mass = massAt(x, y);
    const float gradientX = massAt(x + 1, y) - massAt(x - 1, y);
    const float gradientY = massAt(x, y + 1) - massAt(x, y - 1);
    const glm::vec2 radial = safeNormalize(point, glm::vec2(1.0f, 0.0f));
    const glm::vec2 tangent(-radial.y, radial.x);
    sample.gradient = glm::vec2(gradientX, gradientY);
    const glm::vec2 gradient = safeNormalize(glm::vec2(gradientX, gradientY), radial);
    const glm::vec2 filament = safeNormalize(sample.filamentDir, tangent);
    const glm::vec2 localCurl(-gradient.y, gradient.x);
    sample.flow = (filament * (sample.ridge * 0.72f + sample.audioEnergy * 0.16f) +
                   gradient * (0.16f + sample.node * 0.22f + sample.luminosity * 0.12f) +
                   localCurl * ((sample.node + sample.luminosity * 0.55f + sample.audioEnergy * 0.22f) *
                                std::abs(sample.spin) * 0.18f)) *
                  ofClamp(mass * 0.48f + sample.bloom * 0.28f + sample.ridge * 0.36f + sample.luminosity * 0.35f, 0.0f, 1.0f);
    return sample;
}

float CosmosFormationLayer::distanceToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float& t) const {
    const glm::vec2 ab = b - a;
    const float len2 = glm::dot(ab, ab);
    if (len2 <= 0.000001f) {
        t = 0.0f;
        return glm::length(p - a);
    }
    t = ofClamp(glm::dot(p - a, ab) / len2, 0.0f, 1.0f);
    return glm::length(p - (a + ab * t));
}

float CosmosFormationLayer::waveformSampleFor(std::size_t index) const {
    if (!hasWaveform_ || waveform_.empty()) {
        return 0.0f;
    }
    return ofClamp(waveform_[index % waveform_.size()], -1.0f, 1.0f);
}

float CosmosFormationLayer::pressureForPoint(const glm::vec2& point) const {
    float value = 0.0f;
    for (const auto& wave : waves_) {
        const float radius = wave.age * paramShockwaveSpeed_;
        const float distance = glm::distance(point, wave.origin);
        const float ring = std::max(0.0f, 1.0f - std::abs(distance - radius) / std::max(0.001f, paramShockwaveWidth_ * 2.4f));
        const float decay = std::exp(-wave.age * 1.35f);
        value = std::max(value, ring * decay * wave.strength);
    }
    return ofClamp(value, 0.0f, 1.0f);
}

float CosmosFormationLayer::formationAmount(const MatterNode& node) const {
    const float localAge = std::max(0.0f, age_ - node.birthDelay);
    return smoothStep(0.0f, 1.0f, localAge / std::max(0.01f, paramFormationTime_));
}

float CosmosFormationLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) {
        return 0.0f;
    }
    return std::max(0.0f, timeSeconds) * bpm / 60.0f;
}

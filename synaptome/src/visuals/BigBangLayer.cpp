#include "BigBangLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {
    constexpr float kMinDt = 0.0f;
    constexpr float kMaxDt = 1.0f / 20.0f;

    float wrap01(float value) {
        while (value < 0.0f) value += 1.0f;
        while (value >= 1.0f) value -= 1.0f;
        return value;
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

    float smoothStep(float edge0, float edge1, float value) {
        const float t = ofClamp((value - edge0) / std::max(0.0001f, edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }
}

void BigBangLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramParticleCount_ = def.value("particleCount", paramParticleCount_);
    paramShellCount_ = def.value("shellCount", paramShellCount_);
    paramExpansionRate_ = def.value("expansionRate", paramExpansionRate_);
    paramExpansionPower_ = def.value("expansionPower", paramExpansionPower_);
    paramCollapse_ = def.value("collapse", paramCollapse_);
    paramRadius_ = def.value("radius", paramRadius_);
    paramCoreRadius_ = def.value("coreRadius", paramCoreRadius_);
    paramGalaxySpin_ = def.value("galaxySpin", paramGalaxySpin_);
    paramSwirl_ = def.value("swirl", paramSwirl_);
    paramTurbulence_ = def.value("turbulence", paramTurbulence_);
    paramGravity_ = def.value("gravity", paramGravity_);
    paramDustSize_ = def.value("dustSize", paramDustSize_);
    paramDustGlow_ = def.value("dustGlow", paramDustGlow_);
    paramTrailAlpha_ = def.value("trailAlpha", paramTrailAlpha_);
    paramTrailThickness_ = def.value("trailThickness", paramTrailThickness_);
    paramFilamentAlpha_ = def.value("filamentAlpha", paramFilamentAlpha_);
    paramFilamentStride_ = def.value("filamentStride", paramFilamentStride_);
    paramShellAlpha_ = def.value("shellAlpha", paramShellAlpha_);
    paramShellThickness_ = def.value("shellThickness", paramShellThickness_);
    paramShockwaveCount_ = def.value("shockwaveCount", paramShockwaveCount_);
    paramShockwaveWidth_ = def.value("shockwaveWidth", paramShockwaveWidth_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramBassExpansion_ = def.value("bassExpansion", paramBassExpansion_);
    paramMidsTurbulence_ = def.value("midsTurbulence", paramMidsTurbulence_);
    paramHighsSparkle_ = def.value("highsSparkle", paramHighsSparkle_);
    paramWaveformRipple_ = def.value("waveformRipple", paramWaveformRipple_);
    paramPeakBurstThreshold_ = def.value("peakBurstThreshold", paramPeakBurstThreshold_);
    paramPeakImpulse_ = def.value("peakImpulse", paramPeakImpulse_);
    paramBeatImpulse_ = def.value("beatImpulse", paramBeatImpulse_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramSeed_ = def.value("seed", paramSeed_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    paramBgR_ = def.value("bgR", paramBgR_);
    paramBgG_ = def.value("bgG", paramBgG_);
    paramBgB_ = def.value("bgB", paramBgB_);
    paramCoreR_ = def.value("coreR", paramCoreR_);
    paramCoreG_ = def.value("coreG", paramCoreG_);
    paramCoreB_ = def.value("coreB", paramCoreB_);
    paramDustR_ = def.value("dustR", paramDustR_);
    paramDustG_ = def.value("dustG", paramDustG_);
    paramDustB_ = def.value("dustB", paramDustB_);
    paramShellR_ = def.value("shellR", paramShellR_);
    paramShellG_ = def.value("shellG", paramShellG_);
    paramShellB_ = def.value("shellB", paramShellB_);
    paramFilamentR_ = def.value("filamentR", paramFilamentR_);
    paramFilamentG_ = def.value("filamentG", paramFilamentG_);
    paramFilamentB_ = def.value("filamentB", paramFilamentB_);
    readColor(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(def, "coreColor", paramCoreR_, paramCoreG_, paramCoreB_);
    readColor(def, "dustColor", paramDustR_, paramDustG_, paramDustB_);
    readColor(def, "shellColor", paramShellR_, paramShellG_, paramShellB_);
    readColor(def, "filamentColor", paramFilamentR_, paramFilamentG_, paramFilamentB_);
    clampParams();
}

void BigBangLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.bigBang" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Big Bang";
    meta.label = "Action: Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Alpha: Big Bang", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".particleCount", &paramParticleCount_, paramParticleCount_, "Count: Particles", 64.0f, 2400.0f, 1.0f);
    registerFloat(registry, prefix + ".shellCount", &paramShellCount_, paramShellCount_, "Count: Shells", 1.0f, 16.0f, 1.0f);
    registerFloat(registry, prefix + ".expansionRate", &paramExpansionRate_, paramExpansionRate_, "Time: Expansion Rate", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".expansionPower", &paramExpansionPower_, paramExpansionPower_, "Force: Expansion Power", 0.2f, 2.5f, 0.01f);
    registerFloat(registry, prefix + ".collapse", &paramCollapse_, paramCollapse_, "Force: Collapse", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".radius", &paramRadius_, paramRadius_, "Scale: Big Bang Radius", 0.2f, 1.4f, 0.01f);
    registerFloat(registry, prefix + ".coreRadius", &paramCoreRadius_, paramCoreRadius_, "Scale: Core Radius", 0.0f, 0.25f, 0.001f);
    registerFloat(registry, prefix + ".galaxySpin", &paramGalaxySpin_, paramGalaxySpin_, "Motion: Galaxy Spin", -4.0f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".swirl", &paramSwirl_, paramSwirl_, "Motion: Galaxy Swirl", -3.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".turbulence", &paramTurbulence_, paramTurbulence_, "Motion: Galaxy Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".gravity", &paramGravity_, paramGravity_, "Force: Galaxy Gravity", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".dustSize", &paramDustSize_, paramDustSize_, "Scale: Dust Size", 0.0f, 8.0f, 0.1f);
    registerFloat(registry, prefix + ".dustGlow", &paramDustGlow_, paramDustGlow_, "Glow: Dust", 0.0f, 8.0f, 0.1f);
    registerFloat(registry, prefix + ".trailAlpha", &paramTrailAlpha_, paramTrailAlpha_, "Alpha: Dust Trail", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".trailThickness", &paramTrailThickness_, paramTrailThickness_, "Scale: Dust Trail Thickness", 0.25f, 8.0f, 0.05f);
    registerFloat(registry, prefix + ".filamentAlpha", &paramFilamentAlpha_, paramFilamentAlpha_, "Alpha: Galaxy Filament", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".filamentStride", &paramFilamentStride_, paramFilamentStride_, "Scale: Galaxy Filament Stride", 1.0f, 24.0f, 1.0f);
    registerFloat(registry, prefix + ".shellAlpha", &paramShellAlpha_, paramShellAlpha_, "Alpha: Shock Shell", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".shellThickness", &paramShellThickness_, paramShellThickness_, "Scale: Shock Shell Thickness", 0.5f, 10.0f, 0.1f);
    registerFloat(registry, prefix + ".shockwaveCount", &paramShockwaveCount_, paramShockwaveCount_, "Count: Shockwaves", 0.0f, 12.0f, 1.0f);
    registerFloat(registry, prefix + ".shockwaveWidth", &paramShockwaveWidth_, paramShockwaveWidth_, "Scale: Shockwave Width", 0.005f, 0.2f, 0.001f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Audio: Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".bassExpansion", &paramBassExpansion_, paramBassExpansion_, "Audio: Bass Expansion", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".midsTurbulence", &paramMidsTurbulence_, paramMidsTurbulence_, "Audio: Mids Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".highsSparkle", &paramHighsSparkle_, paramHighsSparkle_, "Audio: Highs Sparkle", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".waveformRipple", &paramWaveformRipple_, paramWaveformRipple_, "Audio: Waveform Ripple", 0.0f, 0.25f, 0.001f);
    registerFloat(registry, prefix + ".peakBurstThreshold", &paramPeakBurstThreshold_, paramPeakBurstThreshold_, "Audio: Peak Burst Threshold", 0.01f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".peakImpulse", &paramPeakImpulse_, paramPeakImpulse_, "Audio: Peak Impulse", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".beatImpulse", &paramBeatImpulse_, paramBeatImpulse_, "Audio: Beat Impulse", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Audio: Smoothing", 0.0f, 0.98f, 0.01f);

    meta = {};
    meta.group = "Big Bang";
    meta.label = "Action: Reseed";
    meta.description = "Respawn dust with the current seed.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Seed: Big Bang", 0.0f, 99999.0f, 1.0f);
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Alpha: Background", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Color: Background R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Color: Background G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Color: Background B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".coreR", &paramCoreR_, paramCoreR_, "Color: Core R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coreG", &paramCoreG_, paramCoreG_, "Color: Core G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".coreB", &paramCoreB_, paramCoreB_, "Color: Core B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".dustR", &paramDustR_, paramDustR_, "Color: Dust R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".dustG", &paramDustG_, paramDustG_, "Color: Dust G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".dustB", &paramDustB_, paramDustB_, "Color: Dust B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shellR", &paramShellR_, paramShellR_, "Color: Shell R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shellG", &paramShellG_, paramShellG_, "Color: Shell G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".shellB", &paramShellB_, paramShellB_, "Color: Shell B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".filamentR", &paramFilamentR_, paramFilamentR_, "Color: Filament R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".filamentG", &paramFilamentG_, paramFilamentG_, "Color: Filament G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".filamentB", &paramFilamentB_, paramFilamentB_, "Color: Filament B", 0.0f, 1.5f, 0.01f);

    resetParticles();
}

void BigBangLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    updateAudioState();
    updateBeatState(params.time, params.bpm);

    const int desiredCount = static_cast<int>(std::round(paramParticleCount_));
    const std::size_t desiredShellCount = static_cast<std::size_t>(std::max(1.0f, std::round(paramShellCount_)));
    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    if (desiredCount != static_cast<int>(particles_.size()) ||
        desiredShellCount != shellCounts_.size() ||
        desiredSeed != seedState_ ||
        paramReseedRequested_) {
        resetParticles();
        paramReseedRequested_ = false;
    }

    const float dt = ofClamp(params.dt, kMinDt, kMaxDt);
    if (dt <= 0.0f) {
        return;
    }

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float expansion = paramExpansionRate_ * (1.0f + bass_ * paramBassExpansion_ * audioDrive + burst_ * 0.45f);
    phase_ = wrap01(phase_ + dt * std::max(0.0f, expansion));
    burst_ = ofLerp(burst_, 0.0f, ofClamp(dt * 4.2f, 0.0f, 1.0f));
    beatPulse_ = ofLerp(beatPulse_, 0.0f, ofClamp(dt * 5.5f, 0.0f, 1.0f));
    updateParticles(dt, params.time);
}

void BigBangLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (particles_.empty()) {
        resetParticles();
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float minDim = std::min(width, height);
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    drawBackground(width, height, params.slotOpacity);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    drawShells(width, height, minDim, alpha, params.beat);
    drawFilaments(width, height, minDim, alpha);
    drawParticles(width, height, minDim, alpha);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    ofPopView();
    ofPopStyle();
}

void BigBangLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void BigBangLayer::registerFloat(ParameterRegistry& registry,
                                 const std::string& id,
                                 float* target,
                                 float initial,
                                 const std::string& label,
                                 float min,
                                 float max,
                                 float step,
                                 const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Big Bang";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void BigBangLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void BigBangLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramParticleCount_ = std::round(ofClamp(paramParticleCount_, 64.0f, 2400.0f));
    paramShellCount_ = std::round(ofClamp(paramShellCount_, 1.0f, 16.0f));
    paramExpansionRate_ = ofClamp(paramExpansionRate_, 0.0f, 1.5f);
    paramExpansionPower_ = ofClamp(paramExpansionPower_, 0.2f, 2.5f);
    paramCollapse_ = ofClamp(paramCollapse_, 0.0f, 1.0f);
    paramRadius_ = ofClamp(paramRadius_, 0.2f, 1.4f);
    paramCoreRadius_ = ofClamp(paramCoreRadius_, 0.0f, 0.25f);
    paramGalaxySpin_ = ofClamp(paramGalaxySpin_, -4.0f, 4.0f);
    paramSwirl_ = ofClamp(paramSwirl_, -3.0f, 3.0f);
    paramTurbulence_ = ofClamp(paramTurbulence_, 0.0f, 2.0f);
    paramGravity_ = ofClamp(paramGravity_, -1.0f, 1.0f);
    paramDustSize_ = ofClamp(paramDustSize_, 0.0f, 8.0f);
    paramDustGlow_ = ofClamp(paramDustGlow_, 0.0f, 8.0f);
    paramTrailAlpha_ = ofClamp(paramTrailAlpha_, 0.0f, 1.0f);
    paramTrailThickness_ = ofClamp(paramTrailThickness_, 0.25f, 8.0f);
    paramFilamentAlpha_ = ofClamp(paramFilamentAlpha_, 0.0f, 1.0f);
    paramFilamentStride_ = std::round(ofClamp(paramFilamentStride_, 1.0f, 24.0f));
    paramShellAlpha_ = ofClamp(paramShellAlpha_, 0.0f, 1.0f);
    paramShellThickness_ = ofClamp(paramShellThickness_, 0.5f, 10.0f);
    paramShockwaveCount_ = std::round(ofClamp(paramShockwaveCount_, 0.0f, 12.0f));
    paramShockwaveWidth_ = ofClamp(paramShockwaveWidth_, 0.005f, 0.2f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 2.0f);
    paramBassExpansion_ = ofClamp(paramBassExpansion_, -1.0f, 1.0f);
    paramMidsTurbulence_ = ofClamp(paramMidsTurbulence_, 0.0f, 2.0f);
    paramHighsSparkle_ = ofClamp(paramHighsSparkle_, 0.0f, 3.0f);
    paramWaveformRipple_ = ofClamp(paramWaveformRipple_, 0.0f, 0.25f);
    paramPeakBurstThreshold_ = ofClamp(paramPeakBurstThreshold_, 0.01f, 1.0f);
    paramPeakImpulse_ = ofClamp(paramPeakImpulse_, 0.0f, 2.0f);
    paramBeatImpulse_ = ofClamp(paramBeatImpulse_, 0.0f, 1.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999.0f));
    paramBgAlpha_ = ofClamp(paramBgAlpha_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramCoreR_ = ofClamp(paramCoreR_, 0.0f, 1.5f);
    paramCoreG_ = ofClamp(paramCoreG_, 0.0f, 1.5f);
    paramCoreB_ = ofClamp(paramCoreB_, 0.0f, 1.5f);
    paramDustR_ = ofClamp(paramDustR_, 0.0f, 1.5f);
    paramDustG_ = ofClamp(paramDustG_, 0.0f, 1.5f);
    paramDustB_ = ofClamp(paramDustB_, 0.0f, 1.5f);
    paramShellR_ = ofClamp(paramShellR_, 0.0f, 1.5f);
    paramShellG_ = ofClamp(paramShellG_, 0.0f, 1.5f);
    paramShellB_ = ofClamp(paramShellB_, 0.0f, 1.5f);
    paramFilamentR_ = ofClamp(paramFilamentR_, 0.0f, 1.5f);
    paramFilamentG_ = ofClamp(paramFilamentG_, 0.0f, 1.5f);
    paramFilamentB_ = ofClamp(paramFilamentB_, 0.0f, 1.5f);
}

void BigBangLayer::resetParticles() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));

    const std::size_t count = static_cast<std::size_t>(std::max(1.0f, paramParticleCount_));
    const std::size_t shellCount = static_cast<std::size_t>(std::max(1.0f, paramShellCount_));
    const std::size_t perShell = std::max<std::size_t>(1, static_cast<std::size_t>(std::ceil(static_cast<float>(count) / static_cast<float>(shellCount))));

    std::mt19937 rng(seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);

    particles_.clear();
    particles_.reserve(count);
    shellOffsets_.assign(shellCount, 0);
    shellCounts_.assign(shellCount, 0);

    for (std::size_t shell = 0; shell < shellCount && particles_.size() < count; ++shell) {
        shellOffsets_[shell] = particles_.size();
        const std::size_t remaining = count - particles_.size();
        const std::size_t shellPopulation = std::min(perShell, remaining);
        shellCounts_[shell] = shellPopulation;
        for (std::size_t i = 0; i < shellPopulation; ++i) {
            const float ordinal = static_cast<float>(i) / static_cast<float>(std::max<std::size_t>(1, shellPopulation));
            Particle particle;
            particle.shell = shell;
            particle.shellIndex = i;
            particle.angle = ordinal * TWO_PI + jitter(rng) * 0.23f;
            particle.radius = ofClamp((static_cast<float>(shell) + unit(rng) * 0.75f) / static_cast<float>(shellCount), 0.0f, 1.0f);
            particle.speed = ofLerp(0.65f, 1.55f, unit(rng));
            particle.spin = jitter(rng) * 0.35f + (unit(rng) < 0.68f ? 1.0f : -1.0f) * ofLerp(0.06f, 0.22f, unit(rng));
            particle.seed = unit(rng) * 1000.0f;
            particle.size = ofLerp(0.55f, 1.75f, unit(rng));
            particle.brightness = ofLerp(0.35f, 1.0f, unit(rng));
            particle.pos = particlePosition(particle, 0.0f);
            particle.prev = particle.pos;
            particles_.push_back(particle);
        }
    }

    phase_ = 0.0f;
    burst_ = 0.0f;
    beatPulse_ = 0.0f;
}

void BigBangLayer::updateAudioState() {
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

void BigBangLayer::updateBeatState(float timeSeconds, float bpm) {
    const float beatPosition = currentBeatPosition(timeSeconds, bpm);
    const int beatIndex = static_cast<int>(std::floor(beatPosition));
    if (lastBeatIndex_ < 0) {
        lastBeatIndex_ = beatIndex;
    } else if (beatIndex > lastBeatIndex_) {
        beatPulse_ = std::max(beatPulse_, paramBeatImpulse_);
        lastBeatIndex_ = beatIndex;
    }

    if (hasAudio_ && peak_ >= paramPeakBurstThreshold_ && timeSeconds - lastPeakTime_ >= 0.08f) {
        burst_ = std::max(burst_, peak_ * paramPeakImpulse_);
        lastPeakTime_ = timeSeconds;
    }
}

void BigBangLayer::updateParticles(float dt, float timeSeconds) {
    (void)dt;
    for (auto& particle : particles_) {
        particle.prev = particle.pos;
        particle.pos = particlePosition(particle, timeSeconds);
    }
}

void BigBangLayer::drawBackground(float width, float height, float alpha) const {
    if (paramBgAlpha_ <= 0.0f) {
        return;
    }
    setColor(colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * alpha));
    ofDrawRectangle(0.0f, 0.0f, width, height);
}

void BigBangLayer::drawShells(float width, float height, float minDim, float alpha, float beat) const {
    const int rings = static_cast<int>(std::round(paramShockwaveCount_));
    if (rings <= 0 || paramShellAlpha_ <= 0.0f) {
        return;
    }

    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const float radiusScale = minDim * 0.5f * paramRadius_ * (1.0f + burst_ * 0.18f + beatPulse_ * 0.12f);
    const ofFloatColor shellColor = colorFrom(paramShellR_, paramShellG_, paramShellB_, 1.0f);
    const ofFloatColor coreColor = colorFrom(paramCoreR_, paramCoreG_, paramCoreB_, 1.0f);

    ofNoFill();
#ifndef TARGET_OPENGLES
    glLineWidth(ofClamp(paramShellThickness_, 0.5f, 10.0f));
#endif
    for (int i = 0; i < rings; ++i) {
        const float ringPhase = wrap01(phase_ + static_cast<float>(i) / static_cast<float>(std::max(1, rings)) + beat * 0.015f);
        const float ringRadius = std::pow(ringPhase, 0.68f) * radiusScale;
        const float edgeFade = 1.0f - smoothStep(0.72f, 1.0f, ringPhase);
        const float coreFlash = std::max(0.0f, 1.0f - ringPhase / std::max(0.001f, paramShockwaveWidth_ * 6.0f));
        const float ringAlpha = alpha * paramShellAlpha_ * (0.2f + edgeFade * 0.8f + burst_ * 0.25f) * (1.0f - coreFlash * 0.35f);
        const ofFloatColor color = shellColor.getLerped(coreColor, ofClamp(coreFlash + beatPulse_ * 0.35f, 0.0f, 1.0f));
        setColor(ofFloatColor(color.r, color.g, color.b, ofClamp(ringAlpha, 0.0f, 1.0f)));
        ofDrawCircle(center.x, center.y, ringRadius);
    }
    ofFill();
}

void BigBangLayer::drawFilaments(float width, float height, float minDim, float alpha) const {
    if (paramFilamentAlpha_ <= 0.0f || particles_.size() < 2) {
        return;
    }

    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const ofFloatColor filamentColor = colorFrom(paramFilamentR_, paramFilamentG_, paramFilamentB_, 1.0f);
    const ofFloatColor shellColor = colorFrom(paramShellR_, paramShellG_, paramShellB_, 1.0f);
    const std::size_t stride = static_cast<std::size_t>(std::max(1.0f, std::round(paramFilamentStride_)));
    const float scale = minDim * 0.5f * paramRadius_;
    const float audioGlow = 1.0f + (level_ + highs_ * 0.75f) * paramAudioAmount_;

    ofMesh trails;
    trails.setMode(OF_PRIMITIVE_LINES);
    for (const auto& particle : particles_) {
        const glm::vec2 a = center + particle.prev * scale;
        const glm::vec2 b = center + particle.pos * scale;
        const float distance = glm::distance(a, b);
        if (distance <= 0.01f) {
            continue;
        }
        const float trailAlpha = alpha * paramTrailAlpha_ * particle.brightness * audioGlow;
        trails.addVertex(glm::vec3(a.x, a.y, 0.0f));
        trails.addColor(ofFloatColor(filamentColor.r, filamentColor.g, filamentColor.b, ofClamp(trailAlpha * 0.2f, 0.0f, 1.0f)));
        trails.addVertex(glm::vec3(b.x, b.y, 0.0f));
        trails.addColor(ofFloatColor(filamentColor.r, filamentColor.g, filamentColor.b, ofClamp(trailAlpha, 0.0f, 1.0f)));
    }

    for (std::size_t shell = 0; shell < shellCounts_.size(); ++shell) {
        const std::size_t count = shellCounts_[shell];
        if (count <= stride) {
            continue;
        }
        const std::size_t offset = shellOffsets_[shell];
        for (std::size_t i = 0; i < count; i += stride) {
            const Particle& aParticle = particles_[offset + i];
            const Particle& bParticle = particles_[offset + ((i + stride) % count)];
            const glm::vec2 a = center + aParticle.pos * scale;
            const glm::vec2 b = center + bParticle.pos * scale;
            const float normalizedDistance = glm::distance(aParticle.pos, bParticle.pos);
            const float closeness = std::max(0.0f, 1.0f - normalizedDistance / 0.34f);
            if (closeness <= 0.0f) {
                continue;
            }
            const float mix = ofClamp(0.22f + burst_ * 0.45f + beatPulse_ * 0.2f, 0.0f, 1.0f);
            const ofFloatColor color = filamentColor.getLerped(shellColor, mix);
            const float edgeAlpha = ofClamp(alpha * paramFilamentAlpha_ * closeness * audioGlow, 0.0f, 1.0f);
            trails.addVertex(glm::vec3(a.x, a.y, 0.0f));
            trails.addColor(ofFloatColor(color.r, color.g, color.b, edgeAlpha));
            trails.addVertex(glm::vec3(b.x, b.y, 0.0f));
            trails.addColor(ofFloatColor(color.r, color.g, color.b, edgeAlpha));
        }
    }

#ifndef TARGET_OPENGLES
    glLineWidth(ofClamp(paramTrailThickness_, 0.25f, 8.0f));
#endif
    trails.draw();
}

void BigBangLayer::drawParticles(float width, float height, float minDim, float alpha) const {
    if (particles_.empty() || paramDustSize_ <= 0.0f) {
        return;
    }

    const glm::vec2 center(width * 0.5f, height * 0.5f);
    const float scale = minDim * 0.5f * paramRadius_;
    const float sparkle = 1.0f + highs_ * paramHighsSparkle_ * paramAudioAmount_ + burst_ * 0.55f + beatPulse_ * 0.35f;
    const ofFloatColor dustColor = colorFrom(paramDustR_, paramDustG_, paramDustB_, 1.0f);
    const ofFloatColor coreColor = colorFrom(paramCoreR_, paramCoreG_, paramCoreB_, 1.0f);
    const ofFloatColor shellColor = colorFrom(paramShellR_, paramShellG_, paramShellB_, 1.0f);

    const float coreR = minDim * paramCoreRadius_ * (1.0f + burst_ * 0.28f + beatPulse_ * 0.2f);
    if (coreR > 0.0f) {
        setColor(ofFloatColor(coreColor.r, coreColor.g, coreColor.b, ofClamp(alpha * 0.2f * sparkle, 0.0f, 1.0f)));
        ofDrawCircle(center.x, center.y, coreR * 2.5f);
        setColor(ofFloatColor(coreColor.r, coreColor.g, coreColor.b, ofClamp(alpha * 0.72f * sparkle, 0.0f, 1.0f)));
        ofDrawCircle(center.x, center.y, coreR);
    }

    for (std::size_t i = 0; i < particles_.size(); ++i) {
        const Particle& particle = particles_[i];
        const glm::vec2 point = center + particle.pos * scale;
        const float radial = ofClamp(glm::length(particle.pos), 0.0f, 1.25f);
        const float coreMix = std::max(0.0f, 1.0f - radial * 3.2f);
        const float shellMix = ofClamp(burst_ * 0.35f + beatPulse_ * 0.25f + waveformSampleFor(i) * 0.12f, 0.0f, 1.0f);
        const ofFloatColor color = dustColor.getLerped(coreColor, coreMix).getLerped(shellColor, shellMix);
        const float twinkle = ofNoise(particle.seed, phase_ * 7.0f, highs_ * 2.0f);
        const float radius = std::max(0.4f, paramDustSize_ * particle.size * (0.55f + twinkle * 0.65f) * sparkle);
        const float dustAlpha = ofClamp(alpha * particle.brightness * (0.42f + twinkle * 0.45f) * (1.0f - radial * 0.16f), 0.0f, 1.0f);

        if (paramDustGlow_ > 0.0f) {
            setColor(ofFloatColor(color.r, color.g, color.b, dustAlpha * 0.18f));
            ofDrawCircle(point.x, point.y, radius * paramDustGlow_);
        }
        setColor(ofFloatColor(color.r, color.g, color.b, dustAlpha));
        ofDrawCircle(point.x, point.y, radius);
    }
}

glm::vec2 BigBangLayer::particlePosition(const Particle& particle, float timeSeconds) const {
    const float shellPhase = wrap01(phase_ * particle.speed + particle.radius * 0.72f);
    const float expanding = std::pow(shellPhase, paramExpansionPower_);
    const float breathing = std::sin(shellPhase * PI);
    float radius = ofLerp(expanding, std::max(0.0f, breathing), paramCollapse_);
    radius *= ofLerp(0.14f, 1.0f, particle.radius);

    const float audioDrive = hasAudio_ ? paramAudioAmount_ : 0.0f;
    radius += bass_ * paramBassExpansion_ * audioDrive * 0.18f;
    radius += burst_ * 0.16f + beatPulse_ * 0.08f;
    radius += waveformSampleFor(particle.shellIndex + particle.shell * 17) * paramWaveformRipple_ * (0.35f + level_ * audioDrive);
    radius -= paramGravity_ * (1.0f - shellPhase) * 0.12f;
    radius = std::max(0.0f, radius);

    const float effectiveTurbulence = paramTurbulence_ + mids_ * paramMidsTurbulence_ * audioDrive;
    const float swirl = paramSwirl_ * radius * radius + paramGalaxySpin_ * timeSeconds * 0.22f + particle.spin * timeSeconds;
    const float noise = (ofNoise(particle.seed * 0.013f, phase_ * 2.1f, timeSeconds * 0.09f) - 0.5f) * effectiveTurbulence;
    const float angle = particle.angle + swirl + noise;
    const glm::vec2 radial(std::cos(angle), std::sin(angle));
    const glm::vec2 tangent(-radial.y, radial.x);
    const float discFlatten = ofLerp(1.0f, 0.54f, ofClamp(radius * 0.8f + paramCollapse_ * 0.25f, 0.0f, 1.0f));
    glm::vec2 pos(radial.x * radius, radial.y * radius * discFlatten);
    pos += tangent * noise * 0.04f;
    return pos;
}

float BigBangLayer::waveformSampleFor(std::size_t index) const {
    if (!hasWaveform_ || waveform_.empty()) {
        return 0.0f;
    }
    const float sample = waveform_[index % waveform_.size()];
    return ofClamp(sample, -1.0f, 1.0f);
}

float BigBangLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) {
        return 0.0f;
    }
    return std::max(0.0f, timeSeconds) * bpm / 60.0f;
}

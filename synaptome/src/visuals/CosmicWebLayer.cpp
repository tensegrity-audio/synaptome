#include "CosmicWebLayer.h"

#include "../io/AudioAnalysisBus.h"
#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofUtils.h"

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

    glm::vec2 wrap01(glm::vec2 value) {
        return glm::vec2(wrap01(value.x), wrap01(value.y));
    }

    glm::vec2 safeNormalize(const glm::vec2& value, const glm::vec2& fallback = glm::vec2(1.0f, 0.0f)) {
        const float len = glm::length(value);
        if (len <= 0.0001f) {
            return fallback;
        }
        return value / len;
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
}

void CosmicWebLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& def = config["defaults"];
    paramEnabled_ = def.value("visible", paramEnabled_);
    paramAlpha_ = def.value("alpha", paramAlpha_);
    paramStarCount_ = def.value("starCount", paramStarCount_);
    paramConnectionDistance_ = def.value("connectionDistance", paramConnectionDistance_);
    paramMaxConnections_ = def.value("maxConnections", paramMaxConnections_);
    paramFilamentAlpha_ = def.value("filamentAlpha", paramFilamentAlpha_);
    paramFilamentThickness_ = def.value("filamentThickness", paramFilamentThickness_);
    paramNodeSize_ = def.value("nodeSize", paramNodeSize_);
    paramTwinkle_ = def.value("twinkle", paramTwinkle_);
    paramPulseSpeed_ = def.value("pulseSpeed", paramPulseSpeed_);
    paramPulseWidth_ = def.value("pulseWidth", paramPulseWidth_);
    paramPulseDecay_ = def.value("pulseDecay", paramPulseDecay_);
    paramFieldScale_ = def.value("fieldScale", paramFieldScale_);
    paramFieldStrength_ = def.value("fieldStrength", paramFieldStrength_);
    paramFlowSpeed_ = def.value("flowSpeed", paramFlowSpeed_);
    paramTurbulence_ = def.value("turbulence", paramTurbulence_);
    paramCenterPull_ = def.value("centerPull", paramCenterPull_);
    paramDriftX_ = def.value("driftX", paramDriftX_);
    paramDriftY_ = def.value("driftY", paramDriftY_);
    paramWaveformDisplacement_ = def.value("waveformDisplacement", paramWaveformDisplacement_);
    paramAudioAmount_ = def.value("audioAmount", paramAudioAmount_);
    paramBassPull_ = def.value("bassPull", paramBassPull_);
    paramMidsTurbulence_ = def.value("midsTurbulence", paramMidsTurbulence_);
    paramHighsTwinkle_ = def.value("highsTwinkle", paramHighsTwinkle_);
    paramPeakPulseThreshold_ = def.value("peakPulseThreshold", paramPeakPulseThreshold_);
    paramPulseCooldown_ = def.value("pulseCooldown", paramPulseCooldown_);
    paramAudioSmoothing_ = def.value("audioSmoothing", paramAudioSmoothing_);
    paramXInput_ = def.value("xInput", paramXInput_);
    paramYInput_ = def.value("yInput", paramYInput_);
    paramAutoReseed_ = def.value("autoReseed", paramAutoReseed_);
    paramAutoReseedEveryBeats_ = def.value("autoReseedEveryBeats", paramAutoReseedEveryBeats_);
    paramSeed_ = def.value("seed", paramSeed_);
    paramBgAlpha_ = def.value("bgAlpha", paramBgAlpha_);
    paramBgR_ = def.value("bgR", paramBgR_);
    paramBgG_ = def.value("bgG", paramBgG_);
    paramBgB_ = def.value("bgB", paramBgB_);
    paramColorR_ = def.value("colorR", paramColorR_);
    paramColorG_ = def.value("colorG", paramColorG_);
    paramColorB_ = def.value("colorB", paramColorB_);
    paramPulseR_ = def.value("pulseR", paramPulseR_);
    paramPulseG_ = def.value("pulseG", paramPulseG_);
    paramPulseB_ = def.value("pulseB", paramPulseB_);
    paramNodeR_ = def.value("nodeColorR", def.value("nodeR", paramNodeR_));
    paramNodeG_ = def.value("nodeColorG", def.value("nodeG", paramNodeG_));
    paramNodeB_ = def.value("nodeColorB", def.value("nodeB", paramNodeB_));
    readColor(def, "backgroundColor", paramBgR_, paramBgG_, paramBgB_);
    readColor(def, "color", paramColorR_, paramColorG_, paramColorB_);
    readColor(def, "pulseColor", paramPulseR_, paramPulseG_, paramPulseB_);
    readColor(def, "nodeColor", paramNodeR_, paramNodeG_, paramNodeB_);
    clampParams();
}

void CosmicWebLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "generative.cosmicWeb" : registryPrefix();
    clampParams();

    ParameterRegistry::Descriptor meta;
    meta.group = "Cosmic Web";
    meta.label = "Cosmic Web Visible";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, meta);

    registerFloat(registry, prefix + ".alpha", &paramAlpha_, paramAlpha_, "Cosmic Web Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".starCount", &paramStarCount_, paramStarCount_, "Web Stars", 24.0f, 640.0f, 1.0f);
    registerFloat(registry, prefix + ".connectionDistance", &paramConnectionDistance_, paramConnectionDistance_, "Web Connect Distance", 0.02f, 0.42f, 0.001f);
    registerFloat(registry, prefix + ".maxConnections", &paramMaxConnections_, paramMaxConnections_, "Web Max Connections", 1.0f, 12.0f, 1.0f);
    registerFloat(registry, prefix + ".filamentAlpha", &paramFilamentAlpha_, paramFilamentAlpha_, "Web Filament Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".filamentThickness", &paramFilamentThickness_, paramFilamentThickness_, "Web Filament Thickness", 0.5f, 8.0f, 0.1f, "Line width in pixels.");
    registerFloat(registry, prefix + ".nodeSize", &paramNodeSize_, paramNodeSize_, "Web Node Size", 0.0f, 10.0f, 0.1f);
    registerFloat(registry, prefix + ".twinkle", &paramTwinkle_, paramTwinkle_, "Web Twinkle", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".pulseSpeed", &paramPulseSpeed_, paramPulseSpeed_, "Web Pulse Speed", 0.05f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".pulseWidth", &paramPulseWidth_, paramPulseWidth_, "Web Pulse Width", 0.01f, 0.5f, 0.001f);
    registerFloat(registry, prefix + ".pulseDecay", &paramPulseDecay_, paramPulseDecay_, "Web Pulse Decay", 0.1f, 4.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldScale", &paramFieldScale_, paramFieldScale_, "Web Field Scale", 0.2f, 12.0f, 0.01f);
    registerFloat(registry, prefix + ".fieldStrength", &paramFieldStrength_, paramFieldStrength_, "Web Field Strength", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".flowSpeed", &paramFlowSpeed_, paramFlowSpeed_, "Web Flow Speed", -1.5f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".turbulence", &paramTurbulence_, paramTurbulence_, "Web Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".centerPull", &paramCenterPull_, paramCenterPull_, "Web Center Pull", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".driftX", &paramDriftX_, paramDriftX_, "Web Drift X", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".driftY", &paramDriftY_, paramDriftY_, "Web Drift Y", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".waveformDisplacement", &paramWaveformDisplacement_, paramWaveformDisplacement_, "Web Waveform Displace", 0.0f, 0.35f, 0.001f);
    registerFloat(registry, prefix + ".audioAmount", &paramAudioAmount_, paramAudioAmount_, "Web Audio Amount", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".bassPull", &paramBassPull_, paramBassPull_, "Web Bass Pull", -1.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".midsTurbulence", &paramMidsTurbulence_, paramMidsTurbulence_, "Web Mids Turbulence", 0.0f, 2.0f, 0.01f);
    registerFloat(registry, prefix + ".highsTwinkle", &paramHighsTwinkle_, paramHighsTwinkle_, "Web Highs Twinkle", 0.0f, 3.0f, 0.01f);
    registerFloat(registry, prefix + ".peakPulseThreshold", &paramPeakPulseThreshold_, paramPeakPulseThreshold_, "Web Peak Pulse Threshold", 0.01f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".pulseCooldown", &paramPulseCooldown_, paramPulseCooldown_, "Web Pulse Cooldown", 0.0f, 2.0f, 0.01f, "Seconds between peak-triggered pulses.");
    registerFloat(registry, prefix + ".audioSmoothing", &paramAudioSmoothing_, paramAudioSmoothing_, "Web Audio Smoothing", 0.0f, 0.98f, 0.01f);
    registerFloat(registry, prefix + ".xInput", &paramXInput_, paramXInput_, "Web Attractor X", 0.0f, 1.0f, 0.001f);
    registerFloat(registry, prefix + ".yInput", &paramYInput_, paramYInput_, "Web Attractor Y", 0.0f, 1.0f, 0.001f);

    meta = {};
    meta.group = "Cosmic Web";
    meta.label = "Web Reseed";
    meta.description = "Respawn stars using the current seed.";
    registry.addBool(prefix + ".reseed", &paramReseedRequested_, paramReseedRequested_, meta);

    meta.label = "Web Auto Reseed";
    meta.description = "Respawn stars on a transport-quantized cadence.";
    registry.addBool(prefix + ".autoReseed", &paramAutoReseed_, paramAutoReseed_, meta);

    registerFloat(registry, prefix + ".autoReseedEveryBeats", &paramAutoReseedEveryBeats_, paramAutoReseedEveryBeats_, "Web Auto Reseed Beats", 1.0f, 256.0f, 1.0f);
    registerFloat(registry, prefix + ".seed", &paramSeed_, paramSeed_, "Web Seed", 0.0f, 99999.0f, 1.0f);
    registerFloat(registry, prefix + ".bgAlpha", &paramBgAlpha_, paramBgAlpha_, "Web Bg Alpha", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgR", &paramBgR_, paramBgR_, "Web Bg R", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgG", &paramBgG_, paramBgG_, "Web Bg G", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".bgB", &paramBgB_, paramBgB_, "Web Bg B", 0.0f, 1.0f, 0.01f);
    registerFloat(registry, prefix + ".colorR", &paramColorR_, paramColorR_, "Web Color R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorG", &paramColorG_, paramColorG_, "Web Color G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".colorB", &paramColorB_, paramColorB_, "Web Color B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".pulseR", &paramPulseR_, paramPulseR_, "Web Pulse R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".pulseG", &paramPulseG_, paramPulseG_, "Web Pulse G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".pulseB", &paramPulseB_, paramPulseB_, "Web Pulse B", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".nodeColorR", &paramNodeR_, paramNodeR_, "Web Node Color R", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".nodeColorG", &paramNodeG_, paramNodeG_, "Web Node Color G", 0.0f, 1.5f, 0.01f);
    registerFloat(registry, prefix + ".nodeColorB", &paramNodeB_, paramNodeB_, "Web Node Color B", 0.0f, 1.5f, 0.01f);

    resetNodes();
    loggedFirstDraw_ = false;
    loggedEmptyDraw_ = false;
    ofLogNotice("CosmicWeb") << "setup prefix=" << prefix
                             << " nodes=" << nodes_.size()
                             << " seed=" << seedState_
                             << " alpha=" << paramAlpha_;
}

void CosmicWebLayer::update(const LayerUpdateParams& params) {
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    clampParams();
    updateAudioState();

    const int desiredCount = static_cast<int>(std::round(paramStarCount_));
    const std::uint32_t desiredSeed = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    if (desiredCount != static_cast<int>(nodes_.size()) || desiredSeed != seedState_ || paramReseedRequested_) {
        resetNodes();
        paramReseedRequested_ = false;
    }

    const float beatPosition = currentBeatPosition(params.time, params.bpm);
    if (paramAutoReseed_ && params.bpm > 0.0f) {
        if (nextAutoReseedBeat_ < 0.0f) {
            const float interval = std::max(1.0f, paramAutoReseedEveryBeats_);
            nextAutoReseedBeat_ = std::floor(beatPosition / interval) * interval + interval;
        }
        while (beatPosition >= nextAutoReseedBeat_) {
            resetNodes();
            nextAutoReseedBeat_ += std::max(1.0f, paramAutoReseedEveryBeats_);
        }
    } else {
        nextAutoReseedBeat_ = -1.0f;
    }

    const float dt = ofClamp(params.dt, kMinDt, kMaxDt);
    if (dt > 0.0f) {
        updateNodes(dt, params.time);
        updatePulses(dt);
    }

    if (hasAudio_ && peak_ >= paramPeakPulseThreshold_ && params.time - lastPulseTime_ >= paramPulseCooldown_) {
        const glm::vec2 attractor(ofClamp(paramXInput_, 0.0f, 1.0f), ofClamp(paramYInput_, 0.0f, 1.0f));
        triggerPulse(ofClamp(peak_ * (0.65f + paramAudioAmount_), 0.1f, 2.0f), attractor);
        lastPulseTime_ = params.time;
    }
}

void CosmicWebLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f) {
        return;
    }
    if (nodes_.empty()) {
        resetNodes();
    }
    if (nodes_.empty()) {
        if (!loggedEmptyDraw_) {
            ofLogWarning("CosmicWeb") << "draw skipped; no nodes after reset"
                                      << " viewport=" << params.viewport.x << "x" << params.viewport.y
                                      << " starCount=" << paramStarCount_;
            loggedEmptyDraw_ = true;
        }
        return;
    }

    const float width = static_cast<float>(std::max(1, params.viewport.x));
    const float height = static_cast<float>(std::max(1, params.viewport.y));
    const float alpha = ofClamp(paramAlpha_ * params.slotOpacity, 0.0f, 1.0f);
    const float distance = std::max(0.001f, paramConnectionDistance_);
    const int maxConnections = std::max(1, static_cast<int>(std::round(paramMaxConnections_)));
    const ofFloatColor filamentColor = colorFrom(paramColorR_, paramColorG_, paramColorB_, 1.0f);
    const ofFloatColor pulseColor = colorFrom(paramPulseR_, paramPulseG_, paramPulseB_, 1.0f);
    const ofFloatColor nodeColor = colorFrom(paramNodeR_, paramNodeG_, paramNodeB_, 1.0f);
    std::vector<int> connections(nodes_.size(), 0);

    if (!loggedFirstDraw_) {
        ofLogNotice("CosmicWeb") << "first draw viewport=" << params.viewport.x << "x" << params.viewport.y
                                 << " nodes=" << nodes_.size()
                                 << " slotOpacity=" << params.slotOpacity
                                 << " audio=" << (hasAudio_ ? "yes" : "no");
        loggedFirstDraw_ = true;
    }

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    if (paramBgAlpha_ > 0.0f) {
        const ofFloatColor bg = colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * params.slotOpacity);
        ofSetColor(static_cast<int>(bg.r * 255.0f),
                   static_cast<int>(bg.g * 255.0f),
                   static_cast<int>(bg.b * 255.0f),
                   static_cast<int>(bg.a * 255.0f));
        ofDrawRectangle(0.0f, 0.0f, width, height);
    }

    ofEnableBlendMode(OF_BLENDMODE_ADD);

    ofMesh filaments;
    filaments.setMode(OF_PRIMITIVE_LINES);

    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        const glm::vec2 a = visualPositionFor(i, nodes_[i]);
        for (std::size_t j = i + 1; j < nodes_.size(); ++j) {
            if (connections[i] >= maxConnections || connections[j] >= maxConnections) {
                continue;
            }
            const glm::vec2 b = visualPositionFor(j, nodes_[j]);
            const float d = glm::distance(a, b);
            if (d > distance) {
                continue;
            }

            const glm::vec2 mid = (a + b) * 0.5f;
            const float closeness = 1.0f - ofClamp(d / distance, 0.0f, 1.0f);
            const float pulse = pulseForPoint(mid);
            const float baseA = (0.10f + paramFilamentAlpha_ * closeness) * alpha;
            const float pulseA = ofClamp(pulse * alpha, 0.0f, 1.0f);
            const ofFloatColor edge = filamentColor.getLerped(pulseColor, ofClamp(pulse * 1.2f, 0.0f, 1.0f));
            const float edgeAlpha = ofClamp(baseA + pulseA, 0.0f, 1.0f);
            filaments.addVertex(glm::vec3(a.x * width, a.y * height, 0.0f));
            filaments.addColor(ofFloatColor(edge.r, edge.g, edge.b, edgeAlpha));
            filaments.addVertex(glm::vec3(b.x * width, b.y * height, 0.0f));
            filaments.addColor(ofFloatColor(edge.r, edge.g, edge.b, edgeAlpha));
            ++connections[i];
            ++connections[j];
        }
    }

#ifndef TARGET_OPENGLES
    glLineWidth(ofClamp(paramFilamentThickness_, 0.5f, 8.0f));
#endif
    filaments.draw();

    const float minDim = std::min(width, height);
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        const glm::vec2 p = visualPositionFor(i, nodes_[i]);
        const float pulse = pulseForPoint(p);
        const float twinkle = ofNoise(nodes_[i].seed, params.time * 0.7f) * (paramTwinkle_ + highs_ * paramHighsTwinkle_ * paramAudioAmount_);
        const float radius = (paramNodeSize_ + pulse * 5.0f + twinkle * 1.6f) * (0.75f + level_ * paramAudioAmount_ * 0.45f);
        const float nodeAlpha = ofClamp((0.62f + twinkle * 0.32f + pulse * 0.8f) * alpha, 0.0f, 1.0f);
        const ofFloatColor c = nodeColor.getLerped(pulseColor, ofClamp(pulse, 0.0f, 1.0f));
        const float coreRadius = std::max(1.2f, radius * minDim * 0.0012f);
        ofSetColor(static_cast<int>(ofClamp(c.r, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(c.g, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(c.b, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(nodeAlpha * 0.22f, 0.0f, 1.0f) * 255.0f));
        ofDrawCircle(p.x * width, p.y * height, coreRadius * 2.8f);
        ofSetColor(static_cast<int>(ofClamp(c.r, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(c.g, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(ofClamp(c.b, 0.0f, 1.0f) * 255.0f),
                   static_cast<int>(nodeAlpha * 255.0f));
        ofDrawCircle(p.x * width, p.y * height, coreRadius);
    }

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofPopView();
    ofPopStyle();
}

void CosmicWebLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
}

void CosmicWebLayer::registerFloat(ParameterRegistry& registry,
                                   const std::string& id,
                                   float* target,
                                   float initial,
                                   const std::string& label,
                                   float min,
                                   float max,
                                   float step,
                                   const std::string& description) {
    ParameterRegistry::Descriptor meta;
    meta.group = "Cosmic Web";
    meta.label = label;
    meta.range.min = min;
    meta.range.max = max;
    meta.range.step = step;
    meta.description = description;
    registry.addFloat(id, target, initial, meta);
}

void CosmicWebLayer::readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
    if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
        return;
    }
    r = defaults[key][0].get<float>();
    g = defaults[key][1].get<float>();
    b = defaults[key][2].get<float>();
}

void CosmicWebLayer::clampParams() {
    paramAlpha_ = ofClamp(paramAlpha_, 0.0f, 1.0f);
    paramStarCount_ = std::round(ofClamp(paramStarCount_, 24.0f, 640.0f));
    paramConnectionDistance_ = ofClamp(paramConnectionDistance_, 0.02f, 0.42f);
    paramMaxConnections_ = std::round(ofClamp(paramMaxConnections_, 1.0f, 12.0f));
    paramFilamentAlpha_ = ofClamp(paramFilamentAlpha_, 0.0f, 1.0f);
    paramFilamentThickness_ = ofClamp(paramFilamentThickness_, 0.5f, 8.0f);
    paramNodeSize_ = ofClamp(paramNodeSize_, 0.0f, 10.0f);
    paramTwinkle_ = ofClamp(paramTwinkle_, 0.0f, 2.0f);
    paramPulseSpeed_ = ofClamp(paramPulseSpeed_, 0.05f, 2.0f);
    paramPulseWidth_ = ofClamp(paramPulseWidth_, 0.01f, 0.5f);
    paramPulseDecay_ = ofClamp(paramPulseDecay_, 0.1f, 4.0f);
    paramFieldScale_ = ofClamp(paramFieldScale_, 0.2f, 12.0f);
    paramFieldStrength_ = ofClamp(paramFieldStrength_, 0.0f, 2.0f);
    paramFlowSpeed_ = ofClamp(paramFlowSpeed_, -1.5f, 1.5f);
    paramTurbulence_ = ofClamp(paramTurbulence_, 0.0f, 2.0f);
    paramCenterPull_ = ofClamp(paramCenterPull_, -1.0f, 1.0f);
    paramDriftX_ = ofClamp(paramDriftX_, -1.0f, 1.0f);
    paramDriftY_ = ofClamp(paramDriftY_, -1.0f, 1.0f);
    paramWaveformDisplacement_ = ofClamp(paramWaveformDisplacement_, 0.0f, 0.35f);
    paramAudioAmount_ = ofClamp(paramAudioAmount_, 0.0f, 2.0f);
    paramBassPull_ = ofClamp(paramBassPull_, -1.0f, 1.0f);
    paramMidsTurbulence_ = ofClamp(paramMidsTurbulence_, 0.0f, 2.0f);
    paramHighsTwinkle_ = ofClamp(paramHighsTwinkle_, 0.0f, 3.0f);
    paramPeakPulseThreshold_ = ofClamp(paramPeakPulseThreshold_, 0.01f, 1.0f);
    paramPulseCooldown_ = ofClamp(paramPulseCooldown_, 0.0f, 2.0f);
    paramAudioSmoothing_ = ofClamp(paramAudioSmoothing_, 0.0f, 0.98f);
    paramXInput_ = ofClamp(paramXInput_, 0.0f, 1.0f);
    paramYInput_ = ofClamp(paramYInput_, 0.0f, 1.0f);
    paramAutoReseedEveryBeats_ = std::round(ofClamp(paramAutoReseedEveryBeats_, 1.0f, 256.0f));
    paramSeed_ = std::round(ofClamp(paramSeed_, 0.0f, 99999.0f));
    paramBgAlpha_ = ofClamp(paramBgAlpha_, 0.0f, 1.0f);
    paramBgR_ = ofClamp(paramBgR_, 0.0f, 1.0f);
    paramBgG_ = ofClamp(paramBgG_, 0.0f, 1.0f);
    paramBgB_ = ofClamp(paramBgB_, 0.0f, 1.0f);
    paramColorR_ = ofClamp(paramColorR_, 0.0f, 1.5f);
    paramColorG_ = ofClamp(paramColorG_, 0.0f, 1.5f);
    paramColorB_ = ofClamp(paramColorB_, 0.0f, 1.5f);
    paramPulseR_ = ofClamp(paramPulseR_, 0.0f, 1.5f);
    paramPulseG_ = ofClamp(paramPulseG_, 0.0f, 1.5f);
    paramPulseB_ = ofClamp(paramPulseB_, 0.0f, 1.5f);
    paramNodeR_ = ofClamp(paramNodeR_, 0.0f, 1.5f);
    paramNodeG_ = ofClamp(paramNodeG_, 0.0f, 1.5f);
    paramNodeB_ = ofClamp(paramNodeB_, 0.0f, 1.5f);
}

void CosmicWebLayer::resetNodes() {
    clampParams();
    seedState_ = static_cast<std::uint32_t>(std::max(0.0f, std::round(paramSeed_)));
    std::mt19937 rng(seedState_);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> centered(-0.5f, 0.5f);
    const int count = static_cast<int>(std::round(paramStarCount_));
    nodes_.assign(static_cast<std::size_t>(count), {});
    for (auto& node : nodes_) {
        node.pos = glm::vec2(unit(rng), unit(rng));
        node.vel = glm::vec2(centered(rng), centered(rng)) * 0.02f;
        node.seed = unit(rng) * 1000.0f;
    }
    pulses_.clear();
    nextAutoReseedBeat_ = -1.0f;
}

void CosmicWebLayer::triggerPulse(float strength, const glm::vec2& origin) {
    Pulse pulse;
    pulse.origin = glm::vec2(ofClamp(origin.x, 0.0f, 1.0f), ofClamp(origin.y, 0.0f, 1.0f));
    pulse.strength = ofClamp(strength, 0.0f, 3.0f);
    pulses_.push_back(pulse);
    if (pulses_.size() > 8) {
        pulses_.erase(pulses_.begin());
    }
}

void CosmicWebLayer::updateAudioState() {
    const auto snapshot = AudioAnalysisBus::instance().snapshot();
    const float follow = followAmount(paramAudioSmoothing_);
    const bool fresh = snapshot.valid;
    hasAudio_ = fresh;
    if (!fresh) {
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

void CosmicWebLayer::updateNodes(float dt, float timeSeconds) {
    const glm::vec2 attractor(paramXInput_, paramYInput_);
    const float audio = paramAudioAmount_;
    const float effectiveCenterPull = paramCenterPull_ + bass_ * paramBassPull_ * audio;
    const float effectiveTurbulence = paramTurbulence_ + mids_ * paramMidsTurbulence_ * audio;
    const glm::vec2 drift(paramDriftX_, paramDriftY_);

    for (auto& node : nodes_) {
        const float nx = node.pos.x * paramFieldScale_ + timeSeconds * paramFlowSpeed_ + node.seed * 0.013f;
        const float ny = node.pos.y * paramFieldScale_ - timeSeconds * paramFlowSpeed_ + node.seed * 0.017f;
        const float angleNoise = ofNoise(nx, ny, node.seed * 0.01f);
        const float wobble = ofNoise(nx * 1.7f, ny * 1.7f, timeSeconds * 0.2f + node.seed * 0.02f);
        const float angle = (angleNoise * TWO_PI * 2.0f) + (wobble - 0.5f) * effectiveTurbulence * TWO_PI;
        glm::vec2 desired(std::cos(angle), std::sin(angle));
        desired *= paramFieldStrength_ * (1.0f + level_ * audio * 0.4f);
        desired += drift * 0.08f;
        desired += safeNormalize(attractor - node.pos, glm::vec2(0.0f, 0.0f)) * effectiveCenterPull * 0.08f;

        const float steer = ofClamp(0.05f + effectiveTurbulence * 0.08f, 0.02f, 0.35f);
        node.vel += (desired - node.vel) * steer;
        node.pos = wrap01(node.pos + node.vel * dt);
    }
}

void CosmicWebLayer::updatePulses(float dt) {
    for (auto& pulse : pulses_) {
        pulse.age += dt;
    }
    pulses_.erase(std::remove_if(pulses_.begin(), pulses_.end(), [&](const Pulse& pulse) {
        return pulse.age * paramPulseSpeed_ > 1.8f || pulse.strength <= 0.001f;
    }), pulses_.end());
}

glm::vec2 CosmicWebLayer::visualPositionFor(std::size_t index, const Node& node) const {
    glm::vec2 pos = node.pos;
    if (hasWaveform_ && paramWaveformDisplacement_ > 0.0f) {
        const glm::vec2 center(0.5f, 0.5f);
        const glm::vec2 radial = safeNormalize(pos - center);
        const float sample = waveformSampleFor(index);
        pos += radial * sample * paramWaveformDisplacement_ * (0.4f + level_ * paramAudioAmount_);
    }
    return glm::vec2(ofClamp(pos.x, 0.0f, 1.0f), ofClamp(pos.y, 0.0f, 1.0f));
}

float CosmicWebLayer::waveformSampleFor(std::size_t index) const {
    if (waveform_.empty()) {
        return 0.0f;
    }
    const std::size_t i = index % waveform_.size();
    return ofClamp(waveform_[i], -1.0f, 1.0f);
}

float CosmicWebLayer::pulseForPoint(const glm::vec2& point) const {
    float value = 0.0f;
    for (const auto& pulse : pulses_) {
        const float radius = pulse.age * paramPulseSpeed_;
        const float d = glm::distance(point, pulse.origin);
        const float ring = std::max(0.0f, 1.0f - std::abs(d - radius) / std::max(0.001f, paramPulseWidth_));
        const float decay = std::exp(-pulse.age * paramPulseDecay_);
        value = std::max(value, ring * decay * pulse.strength);
    }
    return ofClamp(value, 0.0f, 1.0f);
}

float CosmicWebLayer::currentBeatPosition(float timeSeconds, float bpm) const {
    if (bpm <= 0.0f) {
        return 0.0f;
    }
    return timeSeconds * bpm / 60.0f;
}

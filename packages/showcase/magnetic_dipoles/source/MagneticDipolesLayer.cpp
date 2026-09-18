#include "MagneticDipolesLayer.h"
#include "ofGraphics.h"
#include <algorithm>
#include <cmath>

namespace {
float bounded(float v, float low, float high) {
    return std::isfinite(v) ? std::max(low, std::min(high, v)) : low;
}
}

void MagneticDipolesLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) return;
    const auto& defaults = config["defaults"];
    speed_ = defaults.value("speed", speed_);
    paused_ = defaults.value("paused", paused_);
    bpmSync_ = defaults.value("bpmSync", bpmSync_);
    bpmMultiplier_ = defaults.value("bpmMultiplier", bpmMultiplier_);
    seed_ = defaults.value("seed", seed_);
    reseed_ = defaults.value("reseed", reseed_);
    scale_ = defaults.value("scale", scale_);
    rotationDeg_ = defaults.value("rotationDeg", rotationDeg_);
    lineWidth_ = defaults.value("lineWidth", lineWidth_);
    glow_ = defaults.value("glow", glow_);
    colorR_ = defaults.value("colorR", colorR_);
    colorG_ = defaults.value("colorG", colorG_);
    colorB_ = defaults.value("colorB", colorB_);
    accentR_ = defaults.value("accentR", accentR_);
    accentG_ = defaults.value("accentG", accentG_);
    accentB_ = defaults.value("accentB", accentB_);
    controls_.separation = defaults.value("separation", controls_.separation);
    controls_.coupling = defaults.value("coupling", controls_.coupling);
    controls_.orbitRate = defaults.value("orbitRate", controls_.orbitRate);
    controls_.twist = defaults.value("twist", controls_.twist);
    controls_.fieldReach = defaults.value("fieldReach", controls_.fieldReach);
}

void MagneticDipolesLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", speed_);
    binder.bind("paused", paused_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("seed", seed_);
    binder.bind("reseed", reseed_);
    binder.bind("scale", scale_);
    binder.bind("rotationDeg", rotationDeg_);
    binder.bind("lineWidth", lineWidth_);
    binder.bind("glow", glow_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("accentR", accentR_);
    binder.bind("accentG", accentG_);
    binder.bind("accentB", accentB_);
    binder.bind("separation", controls_.separation);
    binder.bind("coupling", controls_.coupling);
    binder.bind("orbitRate", controls_.orbitRate);
    binder.bind("twist", controls_.twist);
    binder.bind("fieldReach", controls_.fieldReach);
}

void MagneticDipolesLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    appliedSeed_ = static_cast<std::uint32_t>(std::round(bounded(seed_, 1.0f, 999999.0f)));
    model_.reset(appliedSeed_);
}

void MagneticDipolesLayer::update(const LayerUpdateParams& params) {
    if (!enabled_) return;
    const auto desired = static_cast<std::uint32_t>(std::round(bounded(seed_, 1.0f, 999999.0f)));
    if (desired != appliedSeed_ || reseed_) {
        appliedSeed_ = desired;
        model_.reset(appliedSeed_);
        reseed_ = false;
    }
    const float bpmRate = bpmSync_
        ? bounded(params.bpm / 120.0f, 0.0f, 4.0f) * bounded(bpmMultiplier_, 0.25f, 4.0f)
        : 1.0f;
    const double seconds = paused_ ? 0.0 :
        static_cast<double>(bounded(params.dt, 0.0f, 0.25f)) * bounded(speed_, 0.0f, 3.0f)
        * bounded(params.speed, 0.0f, 4.0f) * bpmRate;
    model_.advance(seconds, controls_);
}

void MagneticDipolesLayer::draw(const LayerDrawParams& params) {
    if (!enabled_ || params.slotOpacity <= 0.0f || params.viewport.x <= 0 || params.viewport.y <= 0) return;
    const float slot = bounded(params.slotOpacity, 0.0f, 1.0f);
    const float emission = bounded(glow_, 0.0f, 2.0f);
    const float angle = bounded(rotationDeg_, -180.0f, 180.0f) * 0.01745329252f;
    const float co = std::cos(angle), si = std::sin(angle);
    const auto vertex = [co, si](float x, float y) { return glm::vec3(co*x-si*y, si*x+co*y, 0.0f); };
    const auto color = [&](float mix, float density) {
        mix = bounded(mix, 0.0f, 1.0f);
        const float gain = 0.45f + 0.55f * emission;
        return ofFloatColor(
            bounded((bounded(colorR_, 0.0f, 1.0f)*(1.0f-mix)+bounded(accentR_, 0.0f, 1.0f)*mix)*gain, 0.0f, 1.0f),
            bounded((bounded(colorG_, 0.0f, 1.0f)*(1.0f-mix)+bounded(accentG_, 0.0f, 1.0f)*mix)*gain, 0.0f, 1.0f),
            bounded((bounded(colorB_, 0.0f, 1.0f)*(1.0f-mix)+bounded(accentB_, 0.0f, 1.0f)*mix)*gain, 0.0f, 1.0f),
            slot * bounded(density, 0.0f, 1.0f));
    };
    mesh_.clear();
    mesh_.setMode(OF_PRIMITIVE_LINES);
    for (const auto& segment : model_.segments()) {
        const auto tint = color(segment.strength, 0.22f + segment.strength * 0.62f);
        mesh_.addVertex(vertex(segment.a.x, segment.a.y)); mesh_.addColor(tint);
        mesh_.addVertex(vertex(segment.b.x, segment.b.y)); mesh_.addColor(tint);
    }
    ofPushStyle();
    ofPushView();
    ofPushMatrix();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1.0f, 1.0f);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofTranslate(params.viewport.x * 0.5f, params.viewport.y * 0.5f);
    const float extent = std::min(params.viewport.x, params.viewport.y) * 0.46f * bounded(scale_, 0.2f, 1.4f);
    ofScale(extent, extent);
    ofSetLineWidth(bounded(lineWidth_, 0.5f, 5.0f));
    ofSetColor(255, 255, 255, 255);
    mesh_.draw();
    ofPopMatrix();
    ofPopView();
    ofPopStyle();
}

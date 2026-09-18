#include "DifferentialGrowthLayer.h"
#include "ofGraphics.h"
#include <algorithm>
#include <cmath>

namespace {
float bounded(float v, float low, float high) {
    return std::isfinite(v) ? std::max(low, std::min(high, v)) : low;
}
}

void DifferentialGrowthLayer::configure(const ofJson& config) {
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
    controls_.growthRate = defaults.value("growthRate", controls_.growthRate);
    controls_.repulsion = defaults.value("repulsion", controls_.repulsion);
    controls_.stiffness = defaults.value("stiffness", controls_.stiffness);
    controls_.confinement = defaults.value("confinement", controls_.confinement);
    controls_.driftRate = defaults.value("driftRate", controls_.driftRate);
}

void DifferentialGrowthLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
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
    binder.bind("growthRate", controls_.growthRate);
    binder.bind("repulsion", controls_.repulsion);
    binder.bind("stiffness", controls_.stiffness);
    binder.bind("confinement", controls_.confinement);
    binder.bind("driftRate", controls_.driftRate);
}

void DifferentialGrowthLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    appliedSeed_ = static_cast<std::uint32_t>(std::round(bounded(seed_, 1.0f, 999999.0f)));
    model_.reset(appliedSeed_);
}

void DifferentialGrowthLayer::update(const LayerUpdateParams& params) {
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

void DifferentialGrowthLayer::draw(const LayerDrawParams& params) {
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
    const auto& points = model_.points();
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto a = points[i];
        const auto b = points[(i + 1) % points.size()];
        const auto before = points[(i + points.size() - 1) % points.size()];
        const float curvature = std::sqrt((b.x-2*a.x+before.x)*(b.x-2*a.x+before.x)
                                      + (b.y-2*a.y+before.y)*(b.y-2*a.y+before.y));
        const auto tint = color(curvature * 30.0f, 0.92f);
        mesh_.addVertex(vertex(a.x, a.y)); mesh_.addColor(tint);
        mesh_.addVertex(vertex(b.x, b.y)); mesh_.addColor(tint);
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

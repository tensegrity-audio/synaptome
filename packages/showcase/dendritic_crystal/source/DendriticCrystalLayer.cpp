#include "DendriticCrystalLayer.h"
#include "ofGraphics.h"
#include <algorithm>
#include <cmath>

namespace {
float bounded(float v, float low, float high) {
    return std::isfinite(v) ? std::max(low, std::min(high, v)) : low;
}
}

void DendriticCrystalLayer::configure(const ofJson& config) {
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
    controls_.adhesion = defaults.value("adhesion", controls_.adhesion);
    controls_.anisotropy = defaults.value("anisotropy", controls_.anisotropy);
    controls_.dissolutionRate = defaults.value("dissolutionRate", controls_.dissolutionRate);
    controls_.drift = defaults.value("drift", controls_.drift);
}

void DendriticCrystalLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
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
    binder.bind("adhesion", controls_.adhesion);
    binder.bind("anisotropy", controls_.anisotropy);
    binder.bind("dissolutionRate", controls_.dissolutionRate);
    binder.bind("drift", controls_.drift);
}

void DendriticCrystalLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    appliedSeed_ = static_cast<std::uint32_t>(std::round(bounded(seed_, 1.0f, 999999.0f)));
    model_.reset(appliedSeed_);
}

void DendriticCrystalLayer::update(const LayerUpdateParams& params) {
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

void DendriticCrystalLayer::draw(const LayerDrawParams& params) {
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
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    const auto& cells = model_.cells();
    constexpr int width = synaptome_show_dendritic::DendriticCrystalModel::width;
    constexpr int height = synaptome_show_dendritic::DendriticCrystalModel::height;
    const float coverage = bounded(lineWidth_, 0.35f, 1.0f);
    const float half = coverage / static_cast<float>(height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto& cell = cells[static_cast<std::size_t>(y * width + x)];
            if (!cell.occupied) continue;
            const float px = (static_cast<float>(x) - width * 0.5f) * 2.0f / height;
            const float py = (static_cast<float>(y) - height * 0.5f) * 2.0f / height;
            const float fresh = cell.root ? 0.18f : bounded(1.0f-static_cast<float>(model_.time()-cell.born)*0.06f, 0.0f, 1.0f);
            const auto tint = color(fresh, 0.72f + fresh * 0.28f);
            const glm::vec3 a = vertex(px-half, py-half), b = vertex(px+half, py-half);
            const glm::vec3 c = vertex(px+half, py+half), d = vertex(px-half, py+half);
            for (const auto& p : {a,b,c,a,c,d}) { mesh_.addVertex(p); mesh_.addColor(tint); }
        }
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

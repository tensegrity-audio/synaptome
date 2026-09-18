#include "PhaseLatticeLayer.h"
#include "ofGraphics.h"
#include <algorithm>
#include <cmath>

namespace {
float finiteClamp(float value, float lo, float hi, float fallback) {
    return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
}
float readFloat(const ofJson& defaults, const char* key, float fallback, float lo, float hi) {
    const auto it = defaults.find(key);
    return it != defaults.end() && it->is_number()
        ? finiteClamp(it->get<float>(), lo, hi, fallback) : fallback;
}
bool readBool(const ofJson& defaults, const char* key, bool fallback) {
    const auto it = defaults.find(key);
    return it != defaults.end() && it->is_boolean() ? it->get<bool>() : fallback;
}
}

void PhaseLatticeLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) return;
    const auto& defaults = config["defaults"];
    speed_ = readFloat(defaults, "speed", speed_, 0.0f, 3.0f);
    paused_ = readBool(defaults, "paused", paused_);
    bpmSync_ = readBool(defaults, "bpmSync", bpmSync_);
    bpmMultiplier_ = readFloat(defaults, "bpmMultiplier", bpmMultiplier_, 0.25f, 4.0f);
    seed_ = readFloat(defaults, "seed", seed_, 1.0f, 1000000.0f);
    reseed_ = readBool(defaults, "reseed", reseed_);
    scale_ = readFloat(defaults, "scale", scale_, 0.2f, 1.6f);
    rotationDeg_ = readFloat(defaults, "rotationDeg", rotationDeg_, -180.0f, 180.0f);
    colorR_ = readFloat(defaults, "colorR", colorR_, 0.0f, 1.0f);
    colorG_ = readFloat(defaults, "colorG", colorG_, 0.0f, 1.0f);
    colorB_ = readFloat(defaults, "colorB", colorB_, 0.0f, 1.0f);
    coupling_ = readFloat(defaults, "coupling", coupling_, 0.0f, 12.0f);
    frequency_ = readFloat(defaults, "frequency", frequency_, 0.02f, 2.0f);
    dispersion_ = readFloat(defaults, "dispersion", dispersion_, 0.0f, 2.0f);
    phaseLagDeg_ = readFloat(defaults, "phaseLagDeg", phaseLagDeg_, -90.0f, 90.0f);
    radiance_ = readFloat(defaults, "radiance", radiance_, 0.1f, 3.0f);
    contourOpacity_ = readFloat(defaults, "contourOpacity", contourOpacity_, 0.0f, 1.0f);
}
void PhaseLatticeLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", speed_);
    binder.bind("paused", paused_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("seed", seed_);
    binder.bind("reseed", reseed_);
    binder.bind("scale", scale_);
    binder.bind("rotationDeg", rotationDeg_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("coupling", coupling_);
    binder.bind("frequency", frequency_);
    binder.bind("dispersion", dispersion_);
    binder.bind("phaseLagDeg", phaseLagDeg_);
    binder.bind("radiance", radiance_);
    binder.bind("contourOpacity", contourOpacity_);
}
void PhaseLatticeLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    activeSeed_ = static_cast<std::uint32_t>(finiteClamp(seed_, 1.0f, 1000000.0f, 1001.0f));
    model_.reset(activeSeed_);
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
}
void PhaseLatticeLayer::update(const LayerUpdateParams& params) {
    seed_ = std::floor(finiteClamp(seed_, 1.0f, 1000000.0f, 1001.0f));
    const bool resetRequested = reseed_;
    reseed_ = false;
    const auto nextSeed = static_cast<std::uint32_t>(seed_);
    if (resetRequested || nextSeed != activeSeed_) { activeSeed_ = nextSeed; model_.reset(activeSeed_); }
    if (paused_) return;
    const float localRate = finiteClamp(speed_, 0.0f, 3.0f, 0.8f);
    const float globalRate = finiteClamp(params.speed, 0.0f, 8.0f, 0.0f);
    const float tempo = bpmSync_
        ? finiteClamp(params.bpm, 0.0f, 300.0f, 120.0f) / 120.0f * finiteClamp(bpmMultiplier_, 0.25f, 4.0f, 1.0f)
        : 1.0f;
    const float dt = finiteClamp(params.dt, 0.0f, 0.1f, 0.0f) * localRate * globalRate * tempo;
    synaptome_show_phase_lattice::PhaseLatticeModel::Controls controls;
    controls.coupling = coupling_;
    controls.frequency = frequency_;
    controls.dispersion = dispersion_;
    controls.phaseLagDeg = phaseLagDeg_;
    model_.step(dt, controls);
}
void PhaseLatticeLayer::draw(const LayerDrawParams& params) {
    if (params.viewport.x <= 0 || params.viewport.y <= 0 ||
        !std::isfinite(params.slotOpacity) || params.slotOpacity <= 0.0f) return;
    const float opacity = std::clamp(params.slotOpacity, 0.0f, 1.0f);
    const float tintR = finiteClamp(colorR_, 0.0f, 1.0f, 0.5f);
    const float tintG = finiteClamp(colorG_, 0.0f, 1.0f, 0.5f);
    const float tintB = finiteClamp(colorB_, 0.0f, 1.0f, 1.0f);
    const float radiance = finiteClamp(radiance_, 0.1f, 3.0f, 1.0f);
    mesh_.clear();
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    using Model = synaptome_show_phase_lattice::PhaseLatticeModel;
    const auto& field = model_.phases();
    const float contourAlpha = finiteClamp(contourOpacity_, 0.0f, 1.0f, 0.5f);
    for (int y = 0; y < Model::height; ++y) for (int x = 0; x < Model::width; ++x) {
        const float phase = field[y * Model::width + x];
        const float pulse = 0.5f + 0.5f * std::cos(phase);
        const float edge = std::pow(std::max(0.0f, std::cos(phase * 3.0f)), 14.0f) * contourAlpha;
        const float light = (0.22f + pulse * 0.65f) * radiance;
        const ofFloatColor color(std::clamp(tintR * light + edge * 0.65f, 0.0f, 1.0f),
            std::clamp(tintG * light + (0.5f + 0.5f * std::sin(phase)) * 0.25f + edge * 0.6f, 0.0f, 1.0f),
            std::clamp(tintB * light + edge * 0.8f, 0.0f, 1.0f), opacity);
        const float x0 = (static_cast<float>(x) / Model::width - 0.5f) * 1.45454545f;
        const float x1 = (static_cast<float>(x + 1) / Model::width - 0.5f) * 1.45454545f;
        const float y0 = static_cast<float>(y) / Model::height - 0.5f;
        const float y1 = static_cast<float>(y + 1) / Model::height - 0.5f;
        const glm::vec3 vertices[] = {{x0,y0,0.0f},{x1,y0,0.0f},{x1,y1,0.0f},{x0,y0,0.0f},{x1,y1,0.0f},{x0,y1,0.0f}};
        for (const auto& vertex : vertices) { mesh_.addVertex(vertex); mesh_.addColor(color); }
    }
    ofPushStyle();
    ofPushView();
    ofViewport(0.0f, 0.0f, static_cast<float>(params.viewport.x), static_cast<float>(params.viewport.y));
    ofSetupScreenOrtho(static_cast<float>(params.viewport.x), static_cast<float>(params.viewport.y), -1.0f, 1.0f);
    ofPushMatrix();
    ofTranslate(static_cast<float>(params.viewport.x) * 0.5f, static_cast<float>(params.viewport.y) * 0.5f);
    const float extent = std::min(static_cast<float>(params.viewport.x) / 1.45454545f, static_cast<float>(params.viewport.y)) * finiteClamp(scale_, 0.2f, 1.6f, 0.94f);
    ofScale(extent, extent);
    ofRotateDeg(finiteClamp(rotationDeg_, -180.0f, 180.0f, 0.0f));
    ofSetColor(255, 255, 255, 255);
    ofFill();
    mesh_.draw();
    ofPopMatrix();
    ofPopView();
    ofPopStyle();
}

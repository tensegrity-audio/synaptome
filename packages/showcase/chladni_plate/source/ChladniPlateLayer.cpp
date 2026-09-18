#include "ChladniPlateLayer.h"
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

void ChladniPlateLayer::configure(const ofJson& config) {
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
    modeX_ = readFloat(defaults, "modeX", modeX_, 1.0f, 9.0f);
    modeY_ = readFloat(defaults, "modeY", modeY_, 1.0f, 9.0f);
    attraction_ = readFloat(defaults, "attraction", attraction_, 0.0f, 6.0f);
    agitation_ = readFloat(defaults, "agitation", agitation_, 0.0f, 1.0f);
    phaseRate_ = readFloat(defaults, "phaseRate", phaseRate_, 0.0f, 0.8f);
    particleRadius_ = readFloat(defaults, "particleRadius", particleRadius_, 0.001f, 0.008f);
    radiance_ = readFloat(defaults, "radiance", radiance_, 0.1f, 3.0f);
}
void ChladniPlateLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
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
    binder.bind("modeX", modeX_);
    binder.bind("modeY", modeY_);
    binder.bind("attraction", attraction_);
    binder.bind("agitation", agitation_);
    binder.bind("phaseRate", phaseRate_);
    binder.bind("particleRadius", particleRadius_);
    binder.bind("radiance", radiance_);
}
void ChladniPlateLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    activeSeed_ = static_cast<std::uint32_t>(finiteClamp(seed_, 1.0f, 1000000.0f, 1001.0f));
    model_.reset(activeSeed_);
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
}
void ChladniPlateLayer::update(const LayerUpdateParams& params) {
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
    synaptome_show_chladni_plate::ChladniPlateModel::Controls controls;
    controls.modeX = modeX_;
    controls.modeY = modeY_;
    controls.attraction = attraction_;
    controls.agitation = agitation_;
    controls.phaseRate = phaseRate_;
    model_.step(dt, controls);
}
void ChladniPlateLayer::draw(const LayerDrawParams& params) {
    if (params.viewport.x <= 0 || params.viewport.y <= 0 ||
        !std::isfinite(params.slotOpacity) || params.slotOpacity <= 0.0f) return;
    const float opacity = std::clamp(params.slotOpacity, 0.0f, 1.0f);
    const float tintR = finiteClamp(colorR_, 0.0f, 1.0f, 0.5f);
    const float tintG = finiteClamp(colorG_, 0.0f, 1.0f, 0.5f);
    const float tintB = finiteClamp(colorB_, 0.0f, 1.0f, 1.0f);
    const float radiance = finiteClamp(radiance_, 0.1f, 3.0f, 1.0f);
    mesh_.clear();
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    const float grain = finiteClamp(particleRadius_, 0.001f, 0.008f, 0.003f);
    for (const auto& particle : model_.particles()) {
        const float energy = std::min(1.0f, 0.3f + std::sqrt(particle.vx * particle.vx + particle.vy * particle.vy) * 6.0f);
        const float light = std::clamp((0.65f + energy * 0.35f) * radiance, 0.0f, 1.0f);
        const ofFloatColor color(std::clamp(tintR * light + energy * 0.12f, 0.0f, 1.0f),
            std::clamp(tintG * light + energy * 0.08f, 0.0f, 1.0f),
            std::clamp(tintB * light + energy * 0.22f, 0.0f, 1.0f), opacity * 0.85f);
        const float x = particle.x - 0.5f, y = particle.y - 0.5f;
        const glm::vec3 vertices[] = {{x-grain,y-grain,0.0f},{x+grain,y-grain,0.0f},{x+grain,y+grain,0.0f},
            {x-grain,y-grain,0.0f},{x+grain,y+grain,0.0f},{x-grain,y+grain,0.0f}};
        for (const auto& vertex : vertices) { mesh_.addVertex(vertex); mesh_.addColor(color); }
    }
    ofPushStyle();
    ofPushView();
    ofViewport(0.0f, 0.0f, static_cast<float>(params.viewport.x), static_cast<float>(params.viewport.y));
    ofSetupScreenOrtho(static_cast<float>(params.viewport.x), static_cast<float>(params.viewport.y), -1.0f, 1.0f);
    ofPushMatrix();
    ofTranslate(static_cast<float>(params.viewport.x) * 0.5f, static_cast<float>(params.viewport.y) * 0.5f);
    const float extent = std::min(static_cast<float>(params.viewport.x) / 1.0f, static_cast<float>(params.viewport.y)) * finiteClamp(scale_, 0.2f, 1.6f, 0.94f);
    ofScale(extent, extent);
    ofRotateDeg(finiteClamp(rotationDeg_, -180.0f, 180.0f, 0.0f));
    ofSetColor(255, 255, 255, 255);
    ofFill();
    mesh_.draw();
    ofPopMatrix();
    ofPopView();
    ofPopStyle();
}

#include "SandRipplesLayer.h"
#include <algorithm>
#include <cmath>

namespace {
float finiteClamp(float value, float low, float high) { return std::isfinite(value) ? std::clamp(value, low, high) : low; }
}

void SandRipplesLayer::configure(const ofJson& definition) {
    if (!definition.contains("defaults") || !definition["defaults"].is_object()) return;
    const auto& defaults = definition["defaults"];
    speed_ = defaults.value("speed", speed_);
    bpmSync_ = defaults.value("bpmSync", bpmSync_);
    bpmMultiplier_ = defaults.value("bpmMultiplier", bpmMultiplier_);
    scale_ = defaults.value("scale", scale_);
    rotationDeg_ = defaults.value("rotationDeg", rotationDeg_);
    transportRate_ = defaults.value("transportRate", transportRate_);
    windDirectionDeg_ = defaults.value("windDirectionDeg", windDirectionDeg_);
    hopLength_ = defaults.value("hopLength", hopLength_);
    reposeSlope_ = defaults.value("reposeSlope", reposeSlope_);
    relaxationRate_ = defaults.value("relaxationRate", relaxationRate_);
    reliefScale_ = defaults.value("reliefScale", reliefScale_);
    tiltDeg_ = defaults.value("tiltDeg", tiltDeg_);
    ridgeContrast_ = defaults.value("ridgeContrast", ridgeContrast_);
    colorR_ = defaults.value("colorR", colorR_);
    colorG_ = defaults.value("colorG", colorG_);
    colorB_ = defaults.value("colorB", colorB_);
    accentR_ = defaults.value("accentR", accentR_);
    accentG_ = defaults.value("accentG", accentG_);
    accentB_ = defaults.value("accentB", accentB_);
    seed_ = defaults.value("seed", seed_);
}

void SandRipplesLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", speed_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("scale", scale_);
    binder.bind("rotationDeg", rotationDeg_);
    binder.bind("transportRate", transportRate_);
    binder.bind("windDirectionDeg", windDirectionDeg_);
    binder.bind("hopLength", hopLength_);
    binder.bind("reposeSlope", reposeSlope_);
    binder.bind("relaxationRate", relaxationRate_);
    binder.bind("reliefScale", reliefScale_);
    binder.bind("tiltDeg", tiltDeg_);
    binder.bind("ridgeContrast", ridgeContrast_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("accentR", accentR_);
    binder.bind("accentG", accentG_);
    binder.bind("accentB", accentB_);
    binder.bind("seed", seed_);
}

void SandRipplesLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    activeSeed_ = static_cast<std::uint32_t>(finiteClamp(seed_, 0.0f, 65535.0f));
    model_.reset(activeSeed_);
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
}

void SandRipplesLayer::update(const LayerUpdateParams& params) {
    const auto requestedSeed = static_cast<std::uint32_t>(finiteClamp(seed_, 0.0f, 65535.0f));
    if (requestedSeed != activeSeed_) { activeSeed_ = requestedSeed; model_.reset(activeSeed_); }
    const float transport = finiteClamp(params.speed, 0.0f, 8.0f);
    const float tempo = bpmSync_ ? finiteClamp(params.bpm, 0.0f, 300.0f) / 120.0f * finiteClamp(bpmMultiplier_, 0.25f, 4.0f) : 1.0f;
    const float elapsed = finiteClamp(params.dt, 0.0f, 0.1f) * transport * finiteClamp(speed_, 0.0f, 3.0f) * tempo;
    SandRipplesModel::Controls controls;
    controls.transportRate = finiteClamp(transportRate_, 0.0f, 4.0f);
    controls.windDirectionDeg = finiteClamp(windDirectionDeg_, -180.0f, 180.0f);
    controls.hopLength = finiteClamp(hopLength_, 0.02f, 0.35f);
    controls.reposeSlope = finiteClamp(reposeSlope_, 0.005f, 0.12f);
    controls.relaxationRate = finiteClamp(relaxationRate_, 0.0f, 4.0f);
    model_.advance(elapsed, controls);
}

void SandRipplesLayer::draw(const LayerDrawParams& params) {
    if (params.viewport.x <= 0 || params.viewport.y <= 0 || params.slotOpacity <= 0.0f) return;
    const float opacity = finiteClamp(params.slotOpacity, 0.0f, 1.0f);
    const float tilt = finiteClamp(tiltDeg_, 10.0f, 80.0f) * 0.01745329252f;
    const float relief = finiteClamp(reliefScale_, 0.1f, 3.0f);
    const float contrast = finiteClamp(ridgeContrast_, 0.25f, 3.0f);
    // CPU orthographic projection of a triangulated height field. Painter order keeps
    // this self-contained relief independent of the host depth-buffer state.
    const auto point = [&](int x, int y) {
        const float px = -1.0f + 2.0f * x / (SandRipplesModel::columns - 1);
        const float py = -1.0f + 2.0f * y / (SandRipplesModel::rows - 1);
        const float h = (model_.height(x, y) - 0.22f) * relief * 3.0f;
        return glm::vec3(px * 1.45f, py * std::cos(tilt) - h * std::sin(tilt), 0.0f);
    };
    mesh_.clear();
    const auto triangle = [&](int ax, int ay, int bx, int by, int cx, int cy) {
        const float height = (model_.height(ax, ay) + model_.height(bx, by) + model_.height(cx, cy)) / 3.0f;
        const float slopeX = model_.height(ax + 1, ay) - model_.height(ax - 1, ay);
        const float slopeY = model_.height(ax, ay + 1) - model_.height(ax, ay - 1);
        const float blend = std::clamp(0.5f + (height - 0.22f) * 5.0f * contrast, 0.0f, 1.0f);
        const float light = std::clamp(0.78f - slopeX * 5.0f * relief + slopeY * 3.0f * relief, 0.25f, 1.0f);
        const ofFloatColor color(
            (finiteClamp(colorR_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentR_, 0.0f, 1.0f) * blend) * light,
            (finiteClamp(colorG_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentG_, 0.0f, 1.0f) * blend) * light,
            (finiteClamp(colorB_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentB_, 0.0f, 1.0f) * blend) * light, opacity);
        mesh_.addVertex(point(ax, ay)); mesh_.addColor(color);
        mesh_.addVertex(point(bx, by)); mesh_.addColor(color);
        mesh_.addVertex(point(cx, cy)); mesh_.addColor(color);
    };
    for (int y = 0; y < SandRipplesModel::rows - 1; ++y) {
        for (int x = 0; x < SandRipplesModel::columns - 1; ++x) {
            triangle(x, y, x + 1, y, x, y + 1);
            triangle(x + 1, y, x + 1, y + 1, x, y + 1);
        }
    }
    const float width = static_cast<float>(params.viewport.x), height = static_cast<float>(params.viewport.y);
    ofPushStyle(); ofPushView(); ofPushMatrix();
    ofViewport(0.0f, 0.0f, width, height); ofSetupScreenOrtho(width, height, -1.0f, 1.0f);
    ofTranslate(width * 0.5f, height * 0.5f);
    const float size = std::min(width / 3.05f, height * 0.65f) * finiteClamp(scale_, 0.25f, 2.0f);
    ofScale(size, size); ofRotateDeg(finiteClamp(rotationDeg_, -180.0f, 180.0f));
    ofSetColor(255, 255, 255, 255); mesh_.draw();
    ofPopMatrix(); ofPopView(); ofPopStyle();
}

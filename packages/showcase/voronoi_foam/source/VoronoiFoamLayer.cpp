#include "VoronoiFoamLayer.h"
#include <algorithm>
#include <cmath>

namespace {
float finiteClamp(float value, float low, float high) { return std::isfinite(value) ? std::clamp(value, low, high) : low; }
}

void VoronoiFoamLayer::configure(const ofJson& definition) {
    if (!definition.contains("defaults") || !definition["defaults"].is_object()) return;
    const auto& defaults = definition["defaults"];
    speed_ = defaults.value("speed", speed_);
    bpmSync_ = defaults.value("bpmSync", bpmSync_);
    bpmMultiplier_ = defaults.value("bpmMultiplier", bpmMultiplier_);
    scale_ = defaults.value("scale", scale_);
    rotationDeg_ = defaults.value("rotationDeg", rotationDeg_);
    driftSpeed_ = defaults.value("driftSpeed", driftSpeed_);
    relaxationRate_ = defaults.value("relaxationRate", relaxationRate_);
    repulsionWeight_ = defaults.value("repulsionWeight", repulsionWeight_);
    shear_ = defaults.value("shear", shear_);
    wallWidth_ = defaults.value("wallWidth", wallWidth_);
    cellFill_ = defaults.value("cellFill", cellFill_);
    edgeRadiance_ = defaults.value("edgeRadiance", edgeRadiance_);
    colorR_ = defaults.value("colorR", colorR_);
    colorG_ = defaults.value("colorG", colorG_);
    colorB_ = defaults.value("colorB", colorB_);
    accentR_ = defaults.value("accentR", accentR_);
    accentG_ = defaults.value("accentG", accentG_);
    accentB_ = defaults.value("accentB", accentB_);
    seed_ = defaults.value("seed", seed_);
}

void VoronoiFoamLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", speed_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("scale", scale_);
    binder.bind("rotationDeg", rotationDeg_);
    binder.bind("driftSpeed", driftSpeed_);
    binder.bind("relaxationRate", relaxationRate_);
    binder.bind("repulsionWeight", repulsionWeight_);
    binder.bind("shear", shear_);
    binder.bind("wallWidth", wallWidth_);
    binder.bind("cellFill", cellFill_);
    binder.bind("edgeRadiance", edgeRadiance_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("accentR", accentR_);
    binder.bind("accentG", accentG_);
    binder.bind("accentB", accentB_);
    binder.bind("seed", seed_);
}

void VoronoiFoamLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    activeSeed_ = static_cast<std::uint32_t>(finiteClamp(seed_, 0.0f, 65535.0f));
    model_.reset(activeSeed_);
    mesh_.setMode(OF_PRIMITIVE_TRIANGLES);
}

void VoronoiFoamLayer::update(const LayerUpdateParams& params) {
    const auto requestedSeed = static_cast<std::uint32_t>(finiteClamp(seed_, 0.0f, 65535.0f));
    if (requestedSeed != activeSeed_) { activeSeed_ = requestedSeed; model_.reset(activeSeed_); }
    const float transport = finiteClamp(params.speed, 0.0f, 8.0f);
    const float tempo = bpmSync_ ? finiteClamp(params.bpm, 0.0f, 300.0f) / 120.0f * finiteClamp(bpmMultiplier_, 0.25f, 4.0f) : 1.0f;
    const float elapsed = finiteClamp(params.dt, 0.0f, 0.1f) * transport * finiteClamp(speed_, 0.0f, 3.0f) * tempo;
    VoronoiFoamModel::Controls controls;
    controls.driftSpeed = finiteClamp(driftSpeed_, 0.0f, 2.0f);
    controls.relaxationRate = finiteClamp(relaxationRate_, 0.0f, 3.0f);
    controls.repulsionWeight = finiteClamp(repulsionWeight_, 0.0f, 2.0f);
    controls.shear = finiteClamp(shear_, -1.0f, 1.0f);
    model_.advance(elapsed, controls);
}

void VoronoiFoamLayer::draw(const LayerDrawParams& params) {
    if (params.viewport.x <= 0 || params.viewport.y <= 0 || params.slotOpacity <= 0.0f) return;
    const float opacity = finiteClamp(params.slotOpacity, 0.0f, 1.0f);
    const float wallWidth = finiteClamp(wallWidth_, 0.006f, 0.08f);
    const float fill = finiteClamp(cellFill_, 0.0f, 1.0f);
    const float radiance = finiteClamp(edgeRadiance_, 0.2f, 3.0f);
    constexpr int columns = 112, rows = 72;
    std::array<ofFloatColor, (columns + 1) * (rows + 1)> colors;
    for (int y = 0; y <= rows; ++y) for (int x = 0; x <= columns; ++x) {
        const auto sample = model_.sample(-1.0f + x * 2.0f / columns, -1.0f + y * 2.0f / rows);
        const float wall = std::exp(-sample.gap * sample.gap / (wallWidth * wallWidth));
        // Cell color follows measured cell area, so deformation changes the material.
        const float blend = std::clamp((model_.sites()[sample.site].area - 0.06f) * 8.0f, 0.0f, 1.0f);
        const float light = std::clamp(0.3f + wall * radiance, 0.0f, 1.0f);
        colors[y * (columns + 1) + x] = ofFloatColor(
            (finiteClamp(colorR_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentR_, 0.0f, 1.0f) * blend) * light,
            (finiteClamp(colorG_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentG_, 0.0f, 1.0f) * blend) * light,
            (finiteClamp(colorB_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentB_, 0.0f, 1.0f) * blend) * light,
            opacity * (fill + (1.0f - fill) * wall));
    }
    mesh_.clear();
    const auto vertex = [&](int x, int y) {
        mesh_.addVertex(glm::vec3(-1.0f + x * 2.0f / columns, -1.0f + y * 2.0f / rows, 0.0f));
        mesh_.addColor(colors[y * (columns + 1) + x]);
    };
    for (int y = 0; y < rows; ++y) for (int x = 0; x < columns; ++x) {
        vertex(x, y); vertex(x + 1, y); vertex(x, y + 1);
        vertex(x + 1, y); vertex(x + 1, y + 1); vertex(x, y + 1);
    }
    const float width = static_cast<float>(params.viewport.x), height = static_cast<float>(params.viewport.y);
    ofPushStyle(); ofPushView(); ofPushMatrix();
    ofViewport(0.0f, 0.0f, width, height); ofSetupScreenOrtho(width, height, -1.0f, 1.0f);
    ofTranslate(width * 0.5f, height * 0.5f);
    const float size = std::min(width, height) * 0.49f * finiteClamp(scale_, 0.25f, 2.0f);
    ofScale(size, size); ofRotateDeg(finiteClamp(rotationDeg_, -180.0f, 180.0f));
    ofSetColor(255, 255, 255, 255); mesh_.draw();
    ofPopMatrix(); ofPopView(); ofPopStyle();
}

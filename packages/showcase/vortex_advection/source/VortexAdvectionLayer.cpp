#include "VortexAdvectionLayer.h"
#include <algorithm>
#include <cmath>

namespace {
float finiteClamp(float value, float low, float high) { return std::isfinite(value) ? std::clamp(value, low, high) : low; }
}

void VortexAdvectionLayer::configure(const ofJson& definition) {
    if (!definition.contains("defaults") || !definition["defaults"].is_object()) return;
    const auto& defaults = definition["defaults"];
    speed_ = defaults.value("speed", speed_);
    bpmSync_ = defaults.value("bpmSync", bpmSync_);
    bpmMultiplier_ = defaults.value("bpmMultiplier", bpmMultiplier_);
    scale_ = defaults.value("scale", scale_);
    rotationDeg_ = defaults.value("rotationDeg", rotationDeg_);
    circulation_ = defaults.value("circulation", circulation_);
    coreRadius_ = defaults.value("coreRadius", coreRadius_);
    strain_ = defaults.value("strain", strain_);
    vortexDrift_ = defaults.value("vortexDrift", vortexDrift_);
    trailTimeSec_ = defaults.value("trailTimeSec", trailTimeSec_);
    lineWidth_ = defaults.value("lineWidth", lineWidth_);
    colorR_ = defaults.value("colorR", colorR_);
    colorG_ = defaults.value("colorG", colorG_);
    colorB_ = defaults.value("colorB", colorB_);
    accentR_ = defaults.value("accentR", accentR_);
    accentG_ = defaults.value("accentG", accentG_);
    accentB_ = defaults.value("accentB", accentB_);
    seed_ = defaults.value("seed", seed_);
}

void VortexAdvectionLayer::bindParameters(synaptome::element::ParameterBinder& binder) {
    binder.bind("speed", speed_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("scale", scale_);
    binder.bind("rotationDeg", rotationDeg_);
    binder.bind("circulation", circulation_);
    binder.bind("coreRadius", coreRadius_);
    binder.bind("strain", strain_);
    binder.bind("vortexDrift", vortexDrift_);
    binder.bind("trailTimeSec", trailTimeSec_);
    binder.bind("lineWidth", lineWidth_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("accentR", accentR_);
    binder.bind("accentG", accentG_);
    binder.bind("accentB", accentB_);
    binder.bind("seed", seed_);
}

void VortexAdvectionLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    activeSeed_ = static_cast<std::uint32_t>(finiteClamp(seed_, 0.0f, 65535.0f));
    model_.reset(activeSeed_);
    mesh_.setMode(OF_PRIMITIVE_LINES);
}

void VortexAdvectionLayer::update(const LayerUpdateParams& params) {
    const auto requestedSeed = static_cast<std::uint32_t>(finiteClamp(seed_, 0.0f, 65535.0f));
    if (requestedSeed != activeSeed_) { activeSeed_ = requestedSeed; model_.reset(activeSeed_); }
    const float transport = finiteClamp(params.speed, 0.0f, 8.0f);
    const float tempo = bpmSync_ ? finiteClamp(params.bpm, 0.0f, 300.0f) / 120.0f * finiteClamp(bpmMultiplier_, 0.25f, 4.0f) : 1.0f;
    const float elapsed = finiteClamp(params.dt, 0.0f, 0.1f) * transport * finiteClamp(speed_, 0.0f, 3.0f) * tempo;
    VortexAdvectionModel::Controls controls;
    controls.circulation = finiteClamp(circulation_, -2.5f, 2.5f);
    controls.coreRadius = finiteClamp(coreRadius_, 0.025f, 0.3f);
    controls.strain = finiteClamp(strain_, -1.0f, 1.0f);
    controls.vortexDrift = finiteClamp(vortexDrift_, 0.0f, 1.0f);
    model_.advance(elapsed, controls);
}

void VortexAdvectionLayer::draw(const LayerDrawParams& params) {
    if (params.viewport.x <= 0 || params.viewport.y <= 0 || params.slotOpacity <= 0.0f) return;
    const float opacity = finiteClamp(params.slotOpacity, 0.0f, 1.0f);
    const float duration = finiteClamp(trailTimeSec_, 0.1f, 1.3f);
    const int segments = std::clamp(static_cast<int>(duration * 30.0f), 3, VortexAdvectionModel::historyCount - 1);
    mesh_.clear();
    for (const auto& tracer : model_.tracers()) {
        const float blend = std::clamp(tracer.velocity * 0.8f, 0.0f, 1.0f);
        const float r = finiteClamp(colorR_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentR_, 0.0f, 1.0f) * blend;
        const float g = finiteClamp(colorG_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentG_, 0.0f, 1.0f) * blend;
        const float b = finiteClamp(colorB_, 0.0f, 1.0f) * (1.0f - blend) + finiteClamp(accentB_, 0.0f, 1.0f) * blend;
        auto last = tracer.position;
        for (int age = 0; age <= segments; ++age) {
            const int at = (model_.historyHead() - age + VortexAdvectionModel::historyCount) % VortexAdvectionModel::historyCount;
            const auto point = tracer.history[at];
            // Never connect a periodic boundary crossing across the image.
            if (std::abs(last.x - point.x) < 0.3f && std::abs(last.y - point.y) < 0.3f) {
                const float strength = 1.0f - static_cast<float>(age) / (segments + 1);
                mesh_.addVertex(glm::vec3(last.x, last.y, 0.0f));
                mesh_.addColor(ofFloatColor(r, g, b, opacity * strength * strength));
                mesh_.addVertex(glm::vec3(point.x, point.y, 0.0f));
                mesh_.addColor(ofFloatColor(r, g, b, opacity * strength * strength));
            }
            last = point;
        }
    }
    const float width = static_cast<float>(params.viewport.x), height = static_cast<float>(params.viewport.y);
    ofPushStyle(); ofPushView(); ofPushMatrix();
    ofViewport(0.0f, 0.0f, width, height); ofSetupScreenOrtho(width, height, -1.0f, 1.0f);
    ofTranslate(width * 0.5f, height * 0.5f);
    const float size = std::min(width, height) * 0.49f * finiteClamp(scale_, 0.25f, 2.0f);
    ofScale(size, size); ofRotateDeg(finiteClamp(rotationDeg_, -180.0f, 180.0f));
    ofSetLineWidth(finiteClamp(lineWidth_, 0.5f, 3.0f)); ofSetColor(255, 255, 255, 255);
    mesh_.draw();
    ofPopMatrix(); ofPopView(); ofPopStyle();
}

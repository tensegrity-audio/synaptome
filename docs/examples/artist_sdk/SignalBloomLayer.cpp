#include "SignalBloomLayer.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include <algorithm>
#include <cmath>

namespace {
    float normalizedDefault(const ofJson& defaults, const char* key, float fallback) {
        return ofClamp(defaults.value(key, fallback), 0.0f, 1.0f);
    }

    void readColor(const ofJson& defaults, const char* key, float& r, float& g, float& b) {
        if (!defaults.contains(key) || !defaults[key].is_array() || defaults[key].size() < 3) {
            return;
        }
        r = ofClamp(defaults[key][0].get<float>(), 0.0f, 1.0f);
        g = ofClamp(defaults[key][1].get<float>(), 0.0f, 1.0f);
        b = ofClamp(defaults[key][2].get<float>(), 0.0f, 1.0f);
    }
}

void SignalBloomLayer::configure(const ofJson& config) {
    if (!config.contains("defaults") || !config["defaults"].is_object()) {
        return;
    }

    const auto& defaults = config["defaults"];
    visible_ = defaults.value("visible", visible_);
    bpmSync_ = defaults.value("bpmSync", bpmSync_);
    speed_ = defaults.value("speed", speed_);
    bpmMultiplier_ = defaults.value("bpmMultiplier", bpmMultiplier_);
    scale_ = defaults.value("scale", scale_);
    rotationDeg_ = defaults.value("rotationDeg", rotationDeg_);
    alpha_ = normalizedDefault(defaults, "alpha", alpha_);
    gain_ = normalizedDefault(defaults, "gain", gain_);
    lineOpacity_ = normalizedDefault(defaults, "lineOpacity", lineOpacity_);
    xInput_ = normalizedDefault(defaults, "xInput", xInput_);
    yInput_ = normalizedDefault(defaults, "yInput", yInput_);
    speedInput_ = normalizedDefault(defaults, "speedInput", speedInput_);
    readColor(defaults, "color", colorR_, colorG_, colorB_);
    readColor(defaults, "backgroundColor", bgColorR_, bgColorG_, bgColorB_);
}

void SignalBloomLayer::setup(ParameterRegistry& registry) {
    const std::string prefix = registryPrefix().empty() ? "examples.signal_bloom" : registryPrefix();

    ParameterRegistry::Descriptor visibleMeta;
    visibleMeta.label = "Signal Bloom Visible";
    visibleMeta.group = "Example";
    registry.addBool(prefix + ".visible", &visible_, visible_, visibleMeta);

    ParameterRegistry::Descriptor speedMeta;
    speedMeta.label = "Signal Speed";
    speedMeta.group = "Example Motion";
    speedMeta.range.min = 0.0f;
    speedMeta.range.max = 4.0f;
    speedMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".speed", &speed_, speed_, speedMeta);

    ParameterRegistry::Descriptor bpmSyncMeta;
    bpmSyncMeta.label = "BPM Sync";
    bpmSyncMeta.group = "Example Motion";
    registry.addBool(prefix + ".bpmSync", &bpmSync_, bpmSync_, bpmSyncMeta);

    ParameterRegistry::Descriptor bpmMultiplierMeta;
    bpmMultiplierMeta.label = "BPM Multiplier";
    bpmMultiplierMeta.group = "Example Motion";
    bpmMultiplierMeta.range.min = 0.25f;
    bpmMultiplierMeta.range.max = 8.0f;
    bpmMultiplierMeta.range.step = 0.25f;
    registry.addFloat(prefix + ".bpmMultiplier", &bpmMultiplier_, bpmMultiplier_, bpmMultiplierMeta);

    ParameterRegistry::Descriptor scaleMeta;
    scaleMeta.label = "Scale";
    scaleMeta.group = "Example Transform";
    scaleMeta.range.min = 0.1f;
    scaleMeta.range.max = 2.0f;
    scaleMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".scale", &scale_, scale_, scaleMeta);

    ParameterRegistry::Descriptor rotationMeta;
    rotationMeta.label = "Rotation";
    rotationMeta.group = "Example Transform";
    rotationMeta.units = "deg";
    rotationMeta.range.min = -180.0f;
    rotationMeta.range.max = 180.0f;
    rotationMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".rotationDeg", &rotationDeg_, rotationDeg_, rotationMeta);

    ParameterRegistry::Descriptor alphaMeta;
    alphaMeta.label = "Alpha";
    alphaMeta.group = "Example Color";
    alphaMeta.range.min = 0.0f;
    alphaMeta.range.max = 1.0f;
    alphaMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".alpha", &alpha_, alpha_, alphaMeta);

    ParameterRegistry::Descriptor gainMeta;
    gainMeta.label = "Sensor Gain";
    gainMeta.group = "Example Modulation";
    gainMeta.range.min = 0.0f;
    gainMeta.range.max = 2.0f;
    gainMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".gain", &gain_, gain_, gainMeta);

    ParameterRegistry::Descriptor lineMeta;
    lineMeta.label = "Line Opacity";
    lineMeta.group = "Example Color";
    lineMeta.range.min = 0.0f;
    lineMeta.range.max = 1.0f;
    lineMeta.range.step = 0.01f;
    registry.addFloat(prefix + ".lineOpacity", &lineOpacity_, lineOpacity_, lineMeta);

    ParameterRegistry::Descriptor colorMeta;
    colorMeta.group = "Example Color";
    colorMeta.range.min = 0.0f;
    colorMeta.range.max = 1.0f;
    colorMeta.range.step = 0.01f;
    colorMeta.label = "Red";
    registry.addFloat(prefix + ".colorR", &colorR_, colorR_, colorMeta);
    colorMeta.label = "Green";
    registry.addFloat(prefix + ".colorG", &colorG_, colorG_, colorMeta);
    colorMeta.label = "Blue";
    registry.addFloat(prefix + ".colorB", &colorB_, colorB_, colorMeta);

    colorMeta.label = "Background Red";
    registry.addFloat(prefix + ".bgColorR", &bgColorR_, bgColorR_, colorMeta);
    colorMeta.label = "Background Green";
    registry.addFloat(prefix + ".bgColorG", &bgColorG_, bgColorG_, colorMeta);
    colorMeta.label = "Background Blue";
    registry.addFloat(prefix + ".bgColorB", &bgColorB_, bgColorB_, colorMeta);

    ParameterRegistry::Descriptor inputMeta;
    inputMeta.group = "Example Modulation";
    inputMeta.range.min = 0.0f;
    inputMeta.range.max = 1.0f;
    inputMeta.range.step = 0.001f;
    inputMeta.label = "X Input";
    registry.addFloat(prefix + ".xInput", &xInput_, xInput_, inputMeta);
    inputMeta.label = "Y Input";
    registry.addFloat(prefix + ".yInput", &yInput_, yInput_, inputMeta);
    inputMeta.label = "Speed Input";
    registry.addFloat(prefix + ".speedInput", &speedInput_, speedInput_, inputMeta);

    points_.assign(96, glm::vec2{ 0.0f, 0.0f });
}

void SignalBloomLayer::update(const LayerUpdateParams& params) {
    if (!visible_) {
        return;
    }

    float transportRate = bpmSync_
        ? std::max(0.01f, params.bpm / 120.0f) * bpmMultiplier_
        : speed_;
    float modulation = 1.0f + ofClamp(speedInput_, 0.0f, 1.0f) * gain_ * 2.0f;
    phase_ += params.dt * transportRate * modulation * std::max(0.0f, params.speed);

    const float twist = ofDegToRad(rotationDeg_);
    for (std::size_t i = 0; i < points_.size(); ++i) {
        float t = static_cast<float>(i) / static_cast<float>(std::max<std::size_t>(1, points_.size() - 1));
        float angle = t * TWO_PI * 6.0f + phase_ + twist;
        float pulse = std::sin(phase_ * 2.0f + t * TWO_PI);
        float radius = (0.18f + t * 0.72f) * scale_ * (1.0f + pulse * 0.12f * gain_);
        float xDrift = (xInput_ - 0.5f) * 0.35f;
        float yDrift = (yInput_ - 0.5f) * 0.35f;
        points_[i] = glm::vec2{
            std::cos(angle) * radius + xDrift,
            std::sin(angle * 1.7f) * radius + yDrift
        };
    }
}

void SignalBloomLayer::draw(const LayerDrawParams& params) {
    if (!visible_ || params.slotOpacity <= 0.0f) {
        return;
    }

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    ofSetColor(
        static_cast<int>(bgColorR_ * 255.0f),
        static_cast<int>(bgColorG_ * 255.0f),
        static_cast<int>(bgColorB_ * 255.0f),
        static_cast<int>(alpha_ * params.slotOpacity * 255.0f));
    ofDrawRectangle(0, 0, params.viewport.x, params.viewport.y);

    ofTranslate(params.viewport.x * 0.5f, params.viewport.y * 0.5f);
    float radius = std::min(params.viewport.x, params.viewport.y) * 0.44f;
    ofScale(radius, radius);

    ofNoFill();
    ofSetLineWidth(2.0f);
    ofSetColor(
        static_cast<int>(colorR_ * 255.0f),
        static_cast<int>(colorG_ * 255.0f),
        static_cast<int>(colorB_ * 255.0f),
        static_cast<int>(lineOpacity_ * alpha_ * params.slotOpacity * 255.0f));

    ofBeginShape();
    for (const auto& point : points_) {
        ofVertex(point.x, point.y);
    }
    ofEndShape(false);

    ofFill();
    for (const auto& point : points_) {
        ofDrawCircle(point.x, point.y, 0.008f + gain_ * 0.006f);
    }

    ofPopView();
    ofPopStyle();
}

void SignalBloomLayer::setExternalEnabled(bool enabled) {
    visible_ = enabled;
}

#include "SignalBloomLayer.h"
#include "ofGraphics.h"
#include "ofMath.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr float kTwoPi = 6.28318530717958647692f;
    constexpr float kDegreesToRadians = 0.01745329251994329577f;

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
    colorR_ = normalizedDefault(defaults, "colorR", colorR_);
    colorG_ = normalizedDefault(defaults, "colorG", colorG_);
    colorB_ = normalizedDefault(defaults, "colorB", colorB_);
    bgColorR_ = normalizedDefault(defaults, "bgColorR", bgColorR_);
    bgColorG_ = normalizedDefault(defaults, "bgColorG", bgColorG_);
    bgColorB_ = normalizedDefault(defaults, "bgColorB", bgColorB_);
    // Array aliases remain accepted for existing SDK example configs.
    readColor(defaults, "color", colorR_, colorG_, colorB_);
    readColor(defaults, "backgroundColor", bgColorR_, bgColorG_, bgColorB_);
}

void SignalBloomLayer::bindParameters(
    synaptome::element::ParameterBinder& binder) {
    binder.bind("visible", visible_);
    binder.bind("speed", speed_);
    binder.bind("bpmSync", bpmSync_);
    binder.bind("bpmMultiplier", bpmMultiplier_);
    binder.bind("scale", scale_);
    binder.bind("rotationDeg", rotationDeg_);
    binder.bind("alpha", alpha_);
    binder.bind("gain", gain_);
    binder.bind("lineOpacity", lineOpacity_);
    binder.bind("colorR", colorR_);
    binder.bind("colorG", colorG_);
    binder.bind("colorB", colorB_);
    binder.bind("bgColorR", bgColorR_);
    binder.bind("bgColorG", bgColorG_);
    binder.bind("bgColorB", bgColorB_);
    binder.bind("xInput", xInput_);
    binder.bind("yInput", yInput_);
    binder.bind("speedInput", speedInput_);
}

void SignalBloomLayer::setup(ParameterRegistry& registry) {
    (void)registry;
    points_.assign(96, glm::vec2{ 0.0f, 0.0f });
    lineMesh_.clear();
    lineMesh_.setMode(OF_PRIMITIVE_LINE_STRIP);
    dotMesh_.clear();
    dotMesh_.setMode(OF_PRIMITIVE_TRIANGLES);
    for (std::size_t i = 0; i < points_.size(); ++i) {
        lineMesh_.addVertex(glm::vec3(0.0f));
        const auto base = static_cast<ofIndexType>(i * 9);
        for (int vertex = 0; vertex < 9; ++vertex) {
            dotMesh_.addVertex(glm::vec3(0.0f));
        }
        for (ofIndexType triangle = 0; triangle < 8; ++triangle) {
            dotMesh_.addIndex(base);
            dotMesh_.addIndex(base + 1 + triangle);
            dotMesh_.addIndex(base + 1 + ((triangle + 1) % 8));
        }
    }
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

    const float twist = rotationDeg_ * kDegreesToRadians;
    for (std::size_t i = 0; i < points_.size(); ++i) {
        float t = static_cast<float>(i) / static_cast<float>(std::max<std::size_t>(1, points_.size() - 1));
        float angle = t * kTwoPi * 6.0f + phase_ + twist;
        float pulse = std::sin(phase_ * 2.0f + t * kTwoPi);
        float radius = (0.18f + t * 0.72f) * scale_ * (1.0f + pulse * 0.12f * gain_);
        float xDrift = (xInput_ - 0.5f) * 0.35f;
        float yDrift = (yInput_ - 0.5f) * 0.35f;
        points_[i] = glm::vec2{
            std::cos(angle) * radius + xDrift,
            std::sin(angle * 1.7f) * radius + yDrift
        };
        lineMesh_.setVertex(
            static_cast<ofIndexType>(i),
            glm::vec3(points_[i].x, points_[i].y, 0.0f));
        const float dotRadius = 0.008f + gain_ * 0.006f;
        const auto base = static_cast<ofIndexType>(i * 9);
        dotMesh_.setVertex(
            base,
            glm::vec3(points_[i].x, points_[i].y, 0.0f));
        for (ofIndexType vertex = 0; vertex < 8; ++vertex) {
            const float vertexAngle =
                static_cast<float>(vertex) * kTwoPi / 8.0f;
            dotMesh_.setVertex(
                base + 1 + vertex,
                glm::vec3(
                    points_[i].x + std::cos(vertexAngle) * dotRadius,
                    points_[i].y + std::sin(vertexAngle) * dotRadius,
                    0.0f));
        }
    }
}

void SignalBloomLayer::draw(const LayerDrawParams& params) {
    if (!visible_ || params.slotOpacity <= 0.0f) {
        return;
    }
    const float viewportWidth =
        static_cast<float>(params.viewport.x);
    const float viewportHeight =
        static_cast<float>(params.viewport.y);

    ofPushStyle();
    ofPushView();
    ofViewport(0.0f, 0.0f, viewportWidth, viewportHeight);
    ofSetupScreenOrtho(viewportWidth, viewportHeight, -1.0f, 1.0f);

    ofSetColor(
        static_cast<int>(bgColorR_ * 255.0f),
        static_cast<int>(bgColorG_ * 255.0f),
        static_cast<int>(bgColorB_ * 255.0f),
        static_cast<int>(alpha_ * params.slotOpacity * 255.0f));
    ofDrawRectangle(0.0f, 0.0f, viewportWidth, viewportHeight);

    ofTranslate(viewportWidth * 0.5f, viewportHeight * 0.5f);
    float radius = std::min(params.viewport.x, params.viewport.y) * 0.44f;
    ofScale(radius, radius);

    ofNoFill();
    ofSetLineWidth(2.0f);
    ofSetColor(
        static_cast<int>(colorR_ * 255.0f),
        static_cast<int>(colorG_ * 255.0f),
        static_cast<int>(colorB_ * 255.0f),
        static_cast<int>(lineOpacity_ * alpha_ * params.slotOpacity * 255.0f));

    lineMesh_.draw();

    ofFill();
    dotMesh_.draw();

    ofPopView();
    ofPopStyle();
}

void SignalBloomLayer::setExternalEnabled(bool enabled) {
    visible_ = enabled;
}

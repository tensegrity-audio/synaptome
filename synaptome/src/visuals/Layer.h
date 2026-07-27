#pragma once
#ifndef SYNAPTOME_VISUALS_LAYER_H_INCLUDED
#define SYNAPTOME_VISUALS_LAYER_H_INCLUDED

#include <synaptome/element/Action.h>
#include <synaptome/element/Telemetry.h>

#include "ofMain.h"
#include "../core/ParameterRegistry.h"
#include "../ofJson.h"
#include <string>

struct LayerUpdateParams {
    float dt = 0.0f;
    float time = 0.0f;
    float bpm = 120.0f;
    float speed = 1.0f;
};

struct LayerDrawParams {
    ofCamera& camera;
    glm::ivec2 viewport = { 0, 0 };
    float time = 0.0f;
    float beat = 0.0f;
    float slotOpacity = 1.0f;
};

class Layer {
public:
    virtual ~Layer() = default;

    void setRegistryPrefix(const std::string& prefix) { registryPrefix_ = prefix; }
    const std::string& registryPrefix() const { return registryPrefix_; }

    void setInstanceId(const std::string& id) { instanceId_ = id; }
    const std::string& instanceId() const { return instanceId_; }

    virtual void configure(const ofJson& config) { (void)config; }

    virtual void setup(ParameterRegistry& registry) = 0;
    virtual void onParameterRegistryCommitted(
        ParameterRegistry& registry) noexcept {
        (void)registry;
    }
    virtual void registerActions(
        synaptome::element::ActionRegistrar& registrar) {
        (void)registrar;
    }
    virtual void collectTelemetry(
        synaptome::element::TelemetrySink& sink) const {
        (void)sink;
    }
    virtual void update(const LayerUpdateParams& params) = 0;
    virtual void draw(const LayerDrawParams& params) = 0;
    virtual void onWindowResized(int width, int height) { (void)width; (void)height; }

    virtual void setExternalEnabled(bool enabled) { (void)enabled; }
    virtual bool isEnabled() const { return true; }

protected:
    std::string registryPrefix_ = "layer";
    std::string instanceId_;
};

#endif // SYNAPTOME_VISUALS_LAYER_H_INCLUDED

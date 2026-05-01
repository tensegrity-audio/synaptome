#pragma once

#include "ofMain.h"
#include "../core/ParameterRegistry.h"
#include "ofJson.h"
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
    virtual void update(const LayerUpdateParams& params) = 0;
    virtual void draw(const LayerDrawParams& params) = 0;
    virtual void onWindowResized(int width, int height) { (void)width; (void)height; }

    virtual void setExternalEnabled(bool enabled) { (void)enabled; }
    virtual bool isEnabled() const { return true; }

protected:
    std::string registryPrefix_ = "layer";
    std::string instanceId_;
};

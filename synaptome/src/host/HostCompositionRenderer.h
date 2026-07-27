#pragma once

#include "../runtime/CompositionTypes.h"
#include "HostCompositionEffects.h"

#include "ofFbo.h"
#include "ofRectangle.h"

#include <glm/vec2.hpp>

#include <array>

class ofCamera;

namespace synaptome::runtime {
class Runtime;
}

namespace synaptome::host {

enum class RenderStatus {
    Rendered,
    InvalidViewport,
    CompositeAllocationFailed,
};

class HostCompositionRenderer {
public:
    HostCompositionRenderer(
        runtime::Runtime& runtime,
        HostCompositionEffects& effects) noexcept;

    HostCompositionRenderer(const HostCompositionRenderer&) = delete;
    HostCompositionRenderer& operator=(const HostCompositionRenderer&) = delete;
    HostCompositionRenderer(HostCompositionRenderer&&) = delete;
    HostCompositionRenderer& operator=(HostCompositionRenderer&&) = delete;

    RenderStatus render(
        glm::ivec2 viewport,
        ofCamera& camera,
        float elapsedTime,
        float beatPhase);
    bool drawLatest(float x = 0.0f, float y = 0.0f) const;
    bool drawPreview(const ofRectangle& bounds) const;
    bool hasFrame() const noexcept;
    void releaseGraphicsResources() noexcept;

private:
    struct SlotTargets {
        ofFbo layer;
        ofFbo upstream;
        ofFbo effect;
    };

    static bool ensureSlotTarget(
        ofFbo& target,
        glm::ivec2 viewport);
    static void clearSlotTarget(ofFbo& target);
    bool ensureViewport(glm::ivec2 viewport);

    runtime::Runtime& runtime_;
    HostCompositionEffects& effects_;
    std::array<
        SlotTargets,
        runtime::kCompositionLayerCount> slotTargets_;
    ofFbo composite_;
    int width_ = 0;
    int height_ = 0;
    bool frameReady_ = false;
};

} // namespace synaptome::host

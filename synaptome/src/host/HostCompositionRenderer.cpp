#include "HostCompositionRenderer.h"

#include "HostCompositionEffects.h"
#include "../runtime/Runtime.h"

#include "ofGraphics.h"
#include "ofMain.h"

#include <algorithm>
#include <vector>

namespace synaptome::host {

namespace {

ofFbo::Settings targetSettings(
    glm::ivec2 viewport,
    bool useDepth) {
    ofFbo::Settings settings;
    settings.width = viewport.x;
    settings.height = viewport.y;
    settings.useDepth = useDepth;
    settings.useStencil = false;
    settings.internalformat = GL_RGBA;
    settings.textureTarget = GL_TEXTURE_2D;
    settings.minFilter = GL_LINEAR;
    settings.maxFilter = GL_LINEAR;
    settings.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
    settings.wrapModeVertical = GL_CLAMP_TO_EDGE;
    return settings;
}

} // namespace

HostCompositionRenderer::HostCompositionRenderer(
    runtime::Runtime& runtime,
    HostCompositionEffects& effects) noexcept
    : runtime_(runtime),
      effects_(effects) {}

RenderStatus HostCompositionRenderer::render(
    glm::ivec2 viewport,
    ofCamera& camera,
    float elapsedTime,
    float beatPhase) {
    frameReady_ = false;
    if (viewport.x <= 0 || viewport.y <= 0) {
        return RenderStatus::InvalidViewport;
    }

    if (!ensureViewport(viewport)) {
        return RenderStatus::CompositeAllocationFailed;
    }
    const auto composition = runtime_.compositionSnapshot();
    const auto& slots = composition.layers;
    std::array<bool, runtime::kCompositionLayerCount> slotVisible{};

    ofPushStyle();
    for (std::size_t i = 0; i < slots.size(); ++i) {
        const auto& slot = slots[i];
        if (!slot.active) {
            continue;
        }

        auto& targets = slotTargets_[i];
        if (slot.kind == runtime::CompositionKind::Effect) {
            if (!effects_.isConsoleRouted(slot.typeId)) {
                continue;
            }

            const float coverageValue = slot.coverage.defined
                ? static_cast<float>(slot.coverage.columns)
                : effects_.defaultCoverageForType(slot.typeId);
            const auto window =
                runtime_.resolveEffectCoverage(i, coverageValue);
            std::vector<std::size_t> processedSlots;
            std::vector<std::size_t> passthroughSlots;

            if (!ensureSlotTarget(targets.layer, viewport) ||
                !ensureSlotTarget(targets.upstream, viewport) ||
                !ensureSlotTarget(targets.effect, viewport)) {
                continue;
            }

            targets.upstream.begin();
            ofClear(0, 0, 0, 0);
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            bool haveInput = false;
            for (std::size_t upstreamIndex = 0;
                 upstreamIndex < i;
                 ++upstreamIndex) {
                if (!window.contains(upstreamIndex) ||
                    !slotVisible[upstreamIndex]) {
                    continue;
                }
                const auto& upstreamSlot = slots[upstreamIndex];
                const auto& upstreamTarget =
                    slotTargets_[upstreamIndex].layer;
                if (!upstreamSlot.active ||
                    !upstreamTarget.isAllocated()) {
                    continue;
                }
                upstreamTarget.draw(
                    0,
                    0,
                    viewport.x,
                    viewport.y);
                processedSlots.push_back(upstreamIndex);
                haveInput = true;
            }
            ofDisableBlendMode();
            targets.upstream.end();

            targets.layer.begin();
            ofClear(0, 0, 0, 0);
            ofDisableBlendMode();
            targets.layer.end();

            if (haveInput) {
                targets.effect.begin();
                ofClear(0, 0, 0, 0);
                targets.effect.end();
            }

            bool effectApplied = false;
            if (haveInput &&
                effects_.applySlot(
                    slot.typeId,
                    targets.upstream,
                    targets.effect)) {
                effectApplied = true;
                targets.layer.begin();
                ofEnableBlendMode(OF_BLENDMODE_ALPHA);
                targets.effect.draw(
                    0,
                    0,
                    viewport.x,
                    viewport.y);
                ofDisableBlendMode();
                targets.layer.end();
            }

            targets.layer.begin();
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            for (std::size_t upstreamIndex = 0;
                 upstreamIndex < i;
                 ++upstreamIndex) {
                if (window.contains(upstreamIndex) ||
                    !slotVisible[upstreamIndex]) {
                    continue;
                }
                const auto& upstreamSlot = slots[upstreamIndex];
                const auto& upstreamTarget =
                    slotTargets_[upstreamIndex].layer;
                if (!upstreamSlot.active ||
                    !upstreamTarget.isAllocated()) {
                    continue;
                }
                upstreamTarget.draw(
                    0,
                    0,
                    viewport.x,
                    viewport.y);
                passthroughSlots.push_back(upstreamIndex);
            }
            ofDisableBlendMode();
            targets.layer.end();

            if (effectApplied || !passthroughSlots.empty()) {
                if (effectApplied) {
                    for (const auto processedIndex : processedSlots) {
                        slotVisible[processedIndex] = false;
                    }
                }
                for (const auto passthroughIndex : passthroughSlots) {
                    slotVisible[passthroughIndex] = false;
                }
                slotVisible[i] = true;
            }
        } else if (slot.hasElement) {
            if (!ensureSlotTarget(targets.layer, viewport)) {
                continue;
            }
            const float opacity =
                ofClamp(slot.opacity, 0.0f, 1.0f);
            targets.layer.begin();
            ofClear(0, 0, 0, 0);
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            LayerDrawParams params{
                camera,
                viewport,
                elapsedTime,
                beatPhase,
                opacity
            };
            runtime_.drawCompositionElement(i, params);
            ofDisableBlendMode();
            targets.layer.end();
            slotVisible[i] = true;
        } else {
            clearSlotTarget(targets.layer);
        }
    }
    ofPopStyle();

    composite_.begin();
    ofClear(0, 0, 0, 0);
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    for (std::size_t i = 0; i < slots.size(); ++i) {
        if (!slotVisible[i] || !slots[i].active) {
            continue;
        }
        const auto& layer = slotTargets_[i].layer;
        if (!layer.isAllocated()) {
            continue;
        }
        ofSetColor(255);
        layer.draw(0, 0, viewport.x, viewport.y);
    }
    ofDisableBlendMode();
    ofPopStyle();
    composite_.end();

    effects_.applyGlobal(composite_);
    frameReady_ = true;
    return RenderStatus::Rendered;
}

bool HostCompositionRenderer::drawLatest(float x, float y) const {
    if (!hasFrame()) {
        return false;
    }
    ofSetColor(255);
    composite_.draw(x, y);
    return true;
}

bool HostCompositionRenderer::drawPreview(
    const ofRectangle& bounds) const {
    if (!hasFrame() ||
        bounds.width <= 0.0f ||
        bounds.height <= 0.0f) {
        return false;
    }
    ofPushStyle();
    ofSetColor(255);
    composite_.draw(
        bounds.x,
        bounds.y,
        bounds.width,
        bounds.height);
    ofPopStyle();
    return true;
}

bool HostCompositionRenderer::hasFrame() const noexcept {
    return frameReady_ &&
        composite_.isAllocated() &&
        static_cast<int>(composite_.getWidth()) == width_ &&
        static_cast<int>(composite_.getHeight()) == height_ &&
        width_ > 0 &&
        height_ > 0;
}

void HostCompositionRenderer::releaseGraphicsResources() noexcept {
    for (auto& targets : slotTargets_) {
        targets.layer.clear();
        targets.upstream.clear();
        targets.effect.clear();
    }
    composite_.clear();
    width_ = 0;
    height_ = 0;
    frameReady_ = false;
}

bool HostCompositionRenderer::ensureSlotTarget(
    ofFbo& target,
    glm::ivec2 viewport) {
    if (viewport.x <= 0 || viewport.y <= 0) {
        return false;
    }
    if (target.isAllocated() &&
        static_cast<int>(target.getWidth()) == viewport.x &&
        static_cast<int>(target.getHeight()) == viewport.y) {
        return true;
    }

    target.allocate(targetSettings(viewport, true));
    if (!target.isAllocated() ||
        static_cast<int>(target.getWidth()) != viewport.x ||
        static_cast<int>(target.getHeight()) != viewport.y) {
        return false;
    }
    target.begin();
    ofClear(0, 0, 0, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    target.end();
    return true;
}

void HostCompositionRenderer::clearSlotTarget(ofFbo& target) {
    if (!target.isAllocated()) {
        return;
    }
    target.begin();
    ofClear(0, 0, 0, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    target.end();
}

bool HostCompositionRenderer::ensureViewport(glm::ivec2 viewport) {
    if (composite_.isAllocated() &&
        width_ == viewport.x &&
        height_ == viewport.y &&
        static_cast<int>(composite_.getWidth()) == viewport.x &&
        static_cast<int>(composite_.getHeight()) == viewport.y) {
        return true;
    }

    composite_.allocate(targetSettings(viewport, false));
    if (!composite_.isAllocated() ||
        static_cast<int>(composite_.getWidth()) != viewport.x ||
        static_cast<int>(composite_.getHeight()) != viewport.y) {
        return false;
    }
    frameReady_ = false;
    width_ = viewport.x;
    height_ = viewport.y;
    runtime_.resizeCompositionElements(viewport.x, viewport.y);
    return true;
}

} // namespace synaptome::host

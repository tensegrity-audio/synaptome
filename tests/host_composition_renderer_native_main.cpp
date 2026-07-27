#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../synaptome/src/core/ParameterRegistry.h"
#include "../synaptome/src/host/HostCompositionRenderer.h"
#include "../synaptome/src/runtime/Runtime.h"
#include "../synaptome/src/visuals/LayerFactory.h"
#include "stubs/ofStubTrace.h"

namespace {

using synaptome::host::HostCompositionEffects;
using synaptome::host::HostCompositionRenderer;
using synaptome::host::RenderStatus;
using synaptome::runtime::CompositionAssignment;
using synaptome::runtime::CompositionCoverage;
using synaptome::runtime::CompositionKind;
using synaptome::runtime::Runtime;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require(
    const synaptome::runtime::CompositionMutationResult& result,
    const std::string& message) {
    if (!result) {
        throw std::runtime_error(
            message + (result.error.empty() ? "" : ": " + result.error));
    }
}

bool near(float left, float right) {
    return std::fabs(left - right) < 0.0001f;
}

struct ElementState {
    int drawCount = 0;
    int resizeCount = 0;
    int lastWidth = 0;
    int lastHeight = 0;
    glm::ivec2 lastViewport{0, 0};
    float lastTime = 0.0f;
    float lastBeat = 0.0f;
    float lastOpacity = 0.0f;
    const ofCamera* lastCamera = nullptr;
    std::uint64_t lastTargetId = 0;
    std::size_t layerIndex = 0;
    std::vector<std::size_t>* drawOrder = nullptr;
};

class TraceElement final : public Layer {
public:
    explicit TraceElement(std::shared_ptr<ElementState> state)
        : state_(std::move(state)) {}

    void setup(ParameterRegistry&) override {}
    void update(const LayerUpdateParams&) override {}

    void draw(const LayerDrawParams& params) override {
        ++state_->drawCount;
        state_->lastViewport = params.viewport;
        state_->lastTime = params.time;
        state_->lastBeat = params.beat;
        state_->lastOpacity = params.slotOpacity;
        state_->lastCamera = &params.camera;
        state_->lastTargetId = ofstub::activeTargetId();
        if (state_->drawOrder) {
            state_->drawOrder->push_back(state_->layerIndex);
        }
    }

    void onWindowResized(int width, int height) override {
        ++state_->resizeCount;
        state_->lastWidth = width;
        state_->lastHeight = height;
    }

private:
    std::shared_ptr<ElementState> state_;
};

struct DrawEdge {
    std::uint64_t source = 0;
    std::uint64_t destination = 0;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

class FboTrace {
public:
    FboTrace() {
        ofstub::reset();
    }

    ~FboTrace() {
        ofstub::reset();
    }

    FboTrace(const FboTrace&) = delete;
    FboTrace& operator=(const FboTrace&) = delete;

    void reset() {
        ofstub::reset();
    }

    std::size_t allocationCount() const {
        return count(ofstub::EventKind::Allocate);
    }

    std::size_t clearCount() const {
        return count(ofstub::EventKind::Release);
    }

    std::size_t eventCount() const {
        return ofstub::events().size();
    }

    const ofstub::Event& eventAt(std::size_t index) const {
        return ofstub::events().at(index);
    }

    std::vector<DrawEdge> edgesTo(std::uint64_t destination) const {
        std::vector<DrawEdge> result;
        for (const auto& edge : drawEdges()) {
            if (edge.destination == destination) {
                result.push_back(edge);
            }
        }
        return result;
    }

    std::vector<DrawEdge> edgesTo(const ofFbo* destination) const {
        return edgesTo(destination ? destination->stubTraceId() : 0);
    }

    std::vector<DrawEdge> edgesFrom(std::uint64_t source) const {
        std::vector<DrawEdge> result;
        for (const auto& edge : drawEdges()) {
            if (edge.source == source) {
                result.push_back(edge);
            }
        }
        return result;
    }

    std::vector<DrawEdge> edgesFrom(const ofFbo* source) const {
        return edgesFrom(source ? source->stubTraceId() : 0);
    }

    std::vector<DrawEdge> drawEdges() const {
        std::vector<DrawEdge> result;
        for (const auto& event : ofstub::events()) {
            if (event.kind != ofstub::EventKind::Draw) {
                continue;
            }
            result.push_back({
                event.objectId,
                event.activeTargetId,
                event.value0,
                event.value1,
                event.value2,
                event.value3,
            });
        }
        return result;
    }

    const ofstub::Event* allocationFor(std::uint64_t objectId) const {
        for (const auto& event : ofstub::events()) {
            if (event.kind == ofstub::EventKind::Allocate &&
                event.objectId == objectId) {
                return &event;
            }
        }
        return nullptr;
    }

    void requireBalanced(const std::string& scenario) const {
        std::vector<std::uint64_t> targets;
        int styleDepth = 0;
        bool blendEnabled = false;
        for (const auto& event : ofstub::events()) {
            if (event.kind == ofstub::EventKind::Begin &&
                event.allocated) {
                targets.push_back(event.objectId);
            } else if (event.kind == ofstub::EventKind::End &&
                       event.allocated) {
                require(
                    !targets.empty() &&
                        targets.back() == event.objectId,
                    scenario + " ended an FBO out of order");
                targets.pop_back();
            } else if (event.kind == ofstub::EventKind::PushStyle) {
                ++styleDepth;
            } else if (event.kind == ofstub::EventKind::PopStyle) {
                require(
                    styleDepth > 0,
                    scenario + " popped style without a matching push");
                --styleDepth;
            } else if (event.kind == ofstub::EventKind::EnableBlend) {
                blendEnabled = true;
            } else if (event.kind == ofstub::EventKind::DisableBlend) {
                blendEnabled = false;
            }
        }
        require(
            targets.empty() &&
                styleDepth == 0 &&
                !blendEnabled &&
                ofstub::activeTargetId() == 0,
            scenario + " leaked graphics state");
    }

private:
    std::size_t count(ofstub::EventKind kind) const {
        std::size_t result = 0;
        for (const auto& event : ofstub::events()) {
            if (event.kind == kind) {
                ++result;
            }
        }
        return result;
    }
};

class TraceEffects final : public HostCompositionEffects {
public:
    struct SlotCall {
        std::string typeId;
        const ofFbo* source = nullptr;
        const ofFbo* destination = nullptr;
    };

    bool isConsoleRouted(
        std::string_view effectType) const noexcept override {
        routeQueries.emplace_back(effectType);
        const auto found = consoleRoutes.find(std::string(effectType));
        return found != consoleRoutes.end() && found->second;
    }

    float defaultCoverageForType(
        std::string_view effectType) const noexcept override {
        defaultCoverageQueries.emplace_back(effectType);
        const auto found = defaultCoverages.find(std::string(effectType));
        return found == defaultCoverages.end() ? 0.0f : found->second;
    }

    bool applySlot(
        std::string_view effectType,
        const ofFbo& source,
        ofFbo& destination) override {
        slotCalls.push_back({
            std::string(effectType),
            &source,
            &destination,
        });
        const auto found = slotResults.find(std::string(effectType));
        return found == slotResults.end() || found->second;
    }

    void applyGlobal(ofFbo& composite) override {
        globalFrames.push_back(&composite);
        globalTraceOrdinals.push_back(
            traceEventCount ? traceEventCount() : 0);
    }

    void resetCalls() {
        routeQueries.clear();
        defaultCoverageQueries.clear();
        slotCalls.clear();
        globalFrames.clear();
        globalTraceOrdinals.clear();
    }

    std::unordered_map<std::string, bool> consoleRoutes;
    std::unordered_map<std::string, float> defaultCoverages;
    std::unordered_map<std::string, bool> slotResults;
    mutable std::vector<std::string> routeQueries;
    mutable std::vector<std::string> defaultCoverageQueries;
    std::vector<SlotCall> slotCalls;
    std::vector<const ofFbo*> globalFrames;
    std::vector<std::size_t> globalTraceOrdinals;
    std::function<std::size_t()> traceEventCount;
};

class Harness {
public:
    Harness()
        : runtime(factory, parameters),
          renderer(runtime, effects) {
        synaptome::element::ElementDescriptor descriptor;
        descriptor.typeId = "tests.hostRenderer.element";
        descriptor.kind = synaptome::element::ElementKind::Visual;
        factory.registerType(
            std::move(descriptor),
            [this] {
                require(
                    nextState_ < pendingStates_.size(),
                    "element factory was invoked without pending state");
                return std::make_unique<TraceElement>(
                    pendingStates_[nextState_++]);
            });
    }

    std::shared_ptr<ElementState> addElement(
        std::size_t index,
        float opacity = 1.0f,
        bool active = true) {
        auto state = std::make_shared<ElementState>();
        state->layerIndex = index;
        state->drawOrder = &elementDrawOrder;
        pendingStates_.push_back(state);

        Runtime::ElementRequest request;
        request.typeId = "tests.hostRenderer.element";
        request.definitionId =
            "tests.host-renderer.element." + std::to_string(index);
        request.instanceId =
            "tests.host-renderer.instance." + std::to_string(index);
        request.registryPrefix =
            "console.layer" + std::to_string(index + 1);
        request.enabled = active;
        auto prepared = runtime.prepareElement(request);
        require(
            static_cast<bool>(prepared),
            "could not prepare renderer fixture element");

        CompositionAssignment assignment;
        assignment.kind = CompositionKind::Element;
        assignment.definitionId = request.definitionId;
        assignment.label = "Renderer fixture";
        assignment.typeId = request.typeId;
        assignment.registryPrefix = request.registryPrefix;
        assignment.active = active;
        assignment.opacity = opacity;
        require(
            runtime.adoptPreparedElement(
                index,
                std::move(prepared),
                std::move(assignment)),
            "could not adopt renderer fixture element");
        return state;
    }

    void addEffect(
        std::size_t index,
        const std::string& typeId,
        bool coverageDefined,
        int coverageColumns,
        bool active = true) {
        CompositionAssignment assignment;
        assignment.kind = CompositionKind::Effect;
        assignment.definitionId = "tests.host-renderer." + typeId;
        assignment.label = "Renderer fixture effect";
        assignment.typeId = typeId;
        assignment.registryPrefix = "effects." + typeId;
        assignment.active = active;
        assignment.coverage.defined = coverageDefined;
        assignment.coverage.mode = "upstream";
        assignment.coverage.columns = coverageColumns;
        require(
            runtime.assignCompositionEntry(index, std::move(assignment)),
            "could not assign renderer fixture effect");
    }

    LayerFactory factory;
    ParameterRegistry parameters;
    Runtime runtime;
    TraceEffects effects;
    HostCompositionRenderer renderer;
    ofCamera camera;
    std::vector<std::size_t> elementDrawOrder;

private:
    std::vector<std::shared_ptr<ElementState>> pendingStates_;
    std::size_t nextState_ = 0;
};

std::set<std::uint64_t> sources(
    const std::vector<DrawEdge>& edges) {
    std::set<std::uint64_t> result;
    for (const auto& edge : edges) {
        result.insert(edge.source);
    }
    return result;
}

std::uint64_t singleDestinationFrom(
    const FboTrace& trace,
    const ofFbo* source,
    const std::string& message) {
    const auto outgoing = trace.edgesFrom(source);
    require(outgoing.size() == 1, message);
    return outgoing.front().destination;
}

void RunBaseLifecycleScenario() {
    Harness harness;
    const auto first = harness.addElement(0, 0.35f);
    const auto second = harness.addElement(1, 0.8f);
    FboTrace trace;
    harness.effects.traceEventCount = [&] {
        return trace.eventCount();
    };

    const auto status =
        harness.renderer.render({320, 180}, harness.camera, 12.5f, 3.25f);
    require(status == RenderStatus::Rendered, "base render failed");
    require(harness.renderer.hasFrame(), "render did not publish a frame");
    require(
        first->drawCount == 1 &&
            second->drawCount == 1 &&
            first->resizeCount == 1 &&
            second->resizeCount == 1,
        "base render did not route one draw and resize per element");
    require(
        first->lastViewport.x == 320 &&
            first->lastViewport.y == 180 &&
            near(first->lastTime, 12.5f) &&
            near(first->lastBeat, 3.25f) &&
            near(first->lastOpacity, 0.35f) &&
            first->lastCamera == &harness.camera &&
            second->lastCamera == &harness.camera &&
            first->lastTargetId != 0 &&
            second->lastTargetId != 0 &&
            first->lastTargetId != second->lastTargetId &&
            harness.elementDrawOrder ==
                std::vector<std::size_t>{0, 1},
        "render parameters drifted before reaching the element");
    require(
        trace.allocationCount() == 3,
        "base render did not allocate one composite and two element targets");
    require(
        harness.effects.globalFrames.size() == 1,
        "global effect stage was not invoked exactly once");
    const auto* composite = harness.effects.globalFrames.front();
    const auto compositeEdges = trace.edgesTo(composite);
    require(
        compositeEdges.size() == 2 &&
            compositeEdges[0].source == first->lastTargetId &&
            compositeEdges[1].source == second->lastTargetId,
        "base composition did not retain both visible elements");
    const auto* compositeAllocation =
        trace.allocationFor(composite->stubTraceId());
    const auto* firstAllocation =
        trace.allocationFor(first->lastTargetId);
    require(
        compositeAllocation &&
            firstAllocation &&
            compositeAllocation->settings.width == 320 &&
            compositeAllocation->settings.height == 180 &&
            !compositeAllocation->settings.useDepth &&
            !compositeAllocation->settings.useStencil &&
            compositeAllocation->settings.internalFormat == GL_RGBA &&
            compositeAllocation->settings.textureTarget == GL_TEXTURE_2D &&
            compositeAllocation->settings.minFilter == GL_LINEAR &&
            compositeAllocation->settings.maxFilter == GL_LINEAR &&
            compositeAllocation->settings.wrapHorizontal ==
                GL_CLAMP_TO_EDGE &&
            compositeAllocation->settings.wrapVertical ==
                GL_CLAMP_TO_EDGE &&
            firstAllocation->settings.useDepth &&
            !firstAllocation->settings.useStencil,
        "composite or slot target allocation settings drifted");
    require(
        harness.effects.globalTraceOrdinals.front() > 0 &&
            trace.eventAt(
                harness.effects.globalTraceOrdinals.front() - 1).kind ==
                ofstub::EventKind::End &&
            trace.eventAt(
                harness.effects.globalTraceOrdinals.front() - 1).objectId ==
                composite->stubTraceId(),
        "global effect stage did not run after completing the base composite");
    trace.requireBalanced("base render");

    trace.reset();
    require(
        harness.renderer.drawLatest(7.0f, 9.0f),
        "latest-frame draw rejected a completed frame");
    require(
        trace.drawEdges().size() == 1 &&
            trace.drawEdges().front().source == composite->stubTraceId() &&
            trace.drawEdges().front().destination == 0 &&
            near(trace.drawEdges().front().x, 7.0f) &&
            near(trace.drawEdges().front().y, 9.0f) &&
            near(trace.drawEdges().front().width, 320.0f) &&
            near(trace.drawEdges().front().height, 180.0f),
        "latest-frame draw did not preserve position or natural size");
    trace.reset();
    require(
        !harness.renderer.drawPreview({1.0f, 2.0f, 0.0f, 20.0f}) &&
            trace.drawEdges().empty(),
        "preview accepted an empty bounds rectangle");
    require(
        harness.renderer.drawPreview({4.0f, 5.0f, 64.0f, 36.0f}) &&
            trace.drawEdges().size() == 1 &&
            trace.drawEdges().front().source == composite->stubTraceId() &&
            near(trace.drawEdges().front().width, 64.0f) &&
            near(trace.drawEdges().front().height, 36.0f),
        "preview did not draw the retained frame into requested bounds");

    trace.reset();
    harness.effects.resetCalls();
    require(
        harness.renderer.render(
            {320, 180},
            harness.camera,
            13.0f,
            4.0f) == RenderStatus::Rendered &&
            trace.allocationCount() == 0 &&
            first->resizeCount == 1 &&
            second->resizeCount == 1,
        "same-size render did not reuse targets and element sizing");

    trace.reset();
    harness.effects.resetCalls();
    require(
        harness.renderer.render(
            {640, 360},
            harness.camera,
            14.0f,
            5.0f) == RenderStatus::Rendered &&
            trace.allocationCount() == 3 &&
            first->resizeCount == 2 &&
            second->resizeCount == 2 &&
            first->lastWidth == 640 &&
            first->lastHeight == 360,
        "viewport change did not resize targets and elements exactly once");

    trace.reset();
    harness.renderer.releaseGraphicsResources();
    require(
        !harness.renderer.hasFrame() &&
            !harness.renderer.drawLatest() &&
            !harness.renderer.drawPreview({0.0f, 0.0f, 20.0f, 20.0f}) &&
            trace.clearCount() == 25,
        "renderer release retained drawable graphics resources");
    trace.reset();
    harness.renderer.releaseGraphicsResources();
    require(
        !harness.renderer.hasFrame() &&
            trace.clearCount() == 25,
        "renderer release was not idempotent");

    trace.reset();
    require(
        harness.renderer.render(
            {640, 360},
            harness.camera,
            15.0f,
            6.0f) == RenderStatus::Rendered &&
            trace.allocationCount() == 3 &&
            first->resizeCount == 3 &&
            second->resizeCount == 3,
        "renderer did not recreate released graphics resources");

    trace.reset();
    const int drawsBeforeInvalid = first->drawCount;
    const auto invalidStatus = harness.renderer.render(
        {0, 360},
        harness.camera,
        16.0f,
        7.0f);
    require(
        invalidStatus == RenderStatus::InvalidViewport,
        "invalid viewport returned the wrong status");
    require(
        first->drawCount == drawsBeforeInvalid &&
            trace.eventCount() == 0,
        "invalid viewport mutated render or graphics state");
    require(
        !harness.renderer.hasFrame() &&
            !harness.renderer.drawLatest() &&
            trace.drawEdges().empty(),
        "invalid viewport retained or presented a stale frame");
}

void RunFullAndPartialCoverageScenario() {
    {
        Harness harness;
        harness.addElement(0);
        harness.addElement(1);
        harness.addElement(2);
        harness.addEffect(3, "fx.tests.full", false, 0);
        harness.effects.consoleRoutes["fx.tests.full"] = true;
        harness.effects.defaultCoverages["fx.tests.full"] = 0.0f;
        FboTrace trace;

        require(
            harness.renderer.render(
                {300, 200},
                harness.camera,
                1.0f,
                2.0f) == RenderStatus::Rendered,
            "full-coverage render failed");
        require(
            harness.effects.defaultCoverageQueries ==
                std::vector<std::string>{"fx.tests.full"} &&
                harness.effects.slotCalls.size() == 1,
            "undefined coverage did not consult the effect default once");
        const auto& call = harness.effects.slotCalls.front();
        const auto upstream = trace.edgesTo(call.source);
        require(
            upstream.size() == 3 &&
                sources(upstream).size() == 3,
            "full coverage did not gather every visible upstream slot");
        const auto effectLayer = singleDestinationFrom(
            trace,
            call.destination,
            "successful effect output was not copied once");
        const auto* composite = harness.effects.globalFrames.front();
        const auto compositeInputs = trace.edgesTo(composite);
        require(
            compositeInputs.size() == 1 &&
                compositeInputs.front().source == effectLayer,
            "successful full coverage did not consume upstream slots");
    }

    {
        Harness harness;
        harness.addElement(0);
        harness.addElement(1);
        harness.addElement(2);
        harness.addEffect(3, "fx.tests.partial", true, 1);
        harness.effects.consoleRoutes["fx.tests.partial"] = true;
        FboTrace trace;

        require(
            harness.renderer.render(
                {300, 200},
                harness.camera,
                1.0f,
                2.0f) == RenderStatus::Rendered,
            "partial-coverage render failed");
        require(
            harness.effects.defaultCoverageQueries.empty() &&
                harness.effects.slotCalls.size() == 1,
            "explicit coverage unexpectedly consulted the effect default");
        const auto& call = harness.effects.slotCalls.front();
        const auto upstream = trace.edgesTo(call.source);
        require(
            upstream.size() == 1,
            "one-column coverage did not select only the nearest upstream slot");
        const auto effectLayer = singleDestinationFrom(
            trace,
            call.destination,
            "partial effect output was not copied once");
        const auto foldedInputs = trace.edgesTo(effectLayer);
        require(
            foldedInputs.size() == 3,
            "partial effect did not fold output plus two passthrough slots");
        const auto* composite = harness.effects.globalFrames.front();
        const auto compositeInputs = trace.edgesTo(composite);
        require(
            compositeInputs.size() == 1 &&
                compositeInputs.front().source == effectLayer,
            "partial effect did not publish one ownership-preserving result");
        const auto upstreamSources = sources(upstream);
        for (const auto& edge : foldedInputs) {
            if (edge.source != call.destination->stubTraceId()) {
                require(
                    upstreamSources.count(edge.source) == 0,
                    "covered input was also copied through as passthrough");
            }
        }
    }
}

void RunFailOpenAndRouteScenario() {
    {
        Harness harness;
        harness.addElement(0);
        harness.addElement(1);
        harness.addEffect(2, "fx.tests.reject", false, 0);
        harness.effects.consoleRoutes["fx.tests.reject"] = true;
        harness.effects.slotResults["fx.tests.reject"] = false;
        FboTrace trace;

        require(
            harness.renderer.render(
                {256, 144},
                harness.camera,
                1.0f,
                2.0f) == RenderStatus::Rendered,
            "rejected-effect render failed");
        const auto& call = harness.effects.slotCalls.front();
        const auto attemptedInputs = trace.edgesTo(call.source);
        require(
            attemptedInputs.size() == 2 &&
                trace.edgesFrom(call.destination).empty(),
            "rejected effect unexpectedly published an effect output");
        const auto finalInputs =
            trace.edgesTo(harness.effects.globalFrames.front());
        require(
            finalInputs.size() == 2 &&
                sources(finalInputs) == sources(attemptedInputs),
            "rejected effect did not fail open to original upstream slots");
    }

    {
        Harness harness;
        const auto first = harness.addElement(0);
        const auto second = harness.addElement(1);
        harness.addEffect(2, "fx.tests.reject-partial", true, 1);
        harness.effects.consoleRoutes["fx.tests.reject-partial"] = true;
        harness.effects.slotResults["fx.tests.reject-partial"] = false;
        FboTrace trace;

        require(
            harness.renderer.render(
                {256, 144},
                harness.camera,
                1.0f,
                2.0f) == RenderStatus::Rendered,
            "partially covered rejected-effect render failed");
        const auto& call = harness.effects.slotCalls.front();
        const auto attemptedInputs = trace.edgesTo(call.source);
        require(
            attemptedInputs.size() == 1 &&
                attemptedInputs.front().source == second->lastTargetId &&
                trace.edgesFrom(call.destination).empty(),
            "partial rejected effect did not isolate the covered input");
        const auto firstOutgoing =
            trace.edgesFrom(first->lastTargetId);
        require(
            firstOutgoing.size() == 1,
            "partial rejected effect did not fold its passthrough input");
        const auto finalInputs =
            trace.edgesTo(harness.effects.globalFrames.front());
        require(
            finalInputs.size() == 2 &&
                finalInputs[0].source == second->lastTargetId &&
                finalInputs[1].source == firstOutgoing.front().destination,
            "partial rejected effect did not preserve covered and passthrough outputs");
        trace.requireBalanced("partially rejected effect");
    }

    {
        Harness harness;
        harness.addElement(0);
        harness.addEffect(1, "fx.tests.not-console", false, 0);
        harness.effects.consoleRoutes["fx.tests.not-console"] = false;
        FboTrace trace;

        require(
            harness.renderer.render(
                {256, 144},
                harness.camera,
                1.0f,
                2.0f) == RenderStatus::Rendered,
            "non-console route render failed");
        require(
            harness.effects.routeQueries ==
                std::vector<std::string>{"fx.tests.not-console"} &&
                harness.effects.defaultCoverageQueries.empty() &&
                harness.effects.slotCalls.empty(),
            "non-console effect entered the slot effect pipeline");
        require(
            trace.edgesTo(harness.effects.globalFrames.front()).size() == 1,
            "non-console route consumed the base element");
    }
}

void RunChainedEffectScenario() {
    Harness harness;
    harness.addElement(0);
    harness.addEffect(1, "fx.tests.first", false, 0);
    harness.addEffect(2, "fx.tests.second", false, 0);
    harness.effects.consoleRoutes["fx.tests.first"] = true;
    harness.effects.consoleRoutes["fx.tests.second"] = true;
    FboTrace trace;

    require(
        harness.renderer.render(
            {240, 135},
            harness.camera,
            1.0f,
            2.0f) == RenderStatus::Rendered,
        "chained-effect render failed");
    require(
        harness.effects.slotCalls.size() == 2 &&
            harness.effects.slotCalls[0].typeId == "fx.tests.first" &&
            harness.effects.slotCalls[1].typeId == "fx.tests.second",
        "console effects did not execute in slot order");
    const auto& first = harness.effects.slotCalls[0];
    const auto& second = harness.effects.slotCalls[1];
    const auto firstLayer = singleDestinationFrom(
        trace,
        first.destination,
        "first effect did not publish one layer");
    require(
        trace.edgesTo(second.source).size() == 1 &&
            trace.edgesTo(second.source).front().source == firstLayer,
        "second effect did not consume the first effect result");
    const auto secondLayer = singleDestinationFrom(
        trace,
        second.destination,
        "second effect did not publish one layer");
    const auto finalInputs =
        trace.edgesTo(harness.effects.globalFrames.front());
    require(
        finalInputs.size() == 1 &&
            finalInputs.front().source == secondLayer,
        "chained effects did not leave only the final result visible");
}

void RunAllocationFailureScenario() {
    {
        Harness harness;
        FboTrace trace;
        ofstub::failAllocationOnAttempt(1);
        require(
            harness.renderer.render(
                {200, 100},
                harness.camera,
                1.0f,
                2.0f) ==
                    RenderStatus::CompositeAllocationFailed &&
                !harness.renderer.hasFrame() &&
                harness.effects.globalFrames.empty(),
            "composite allocation failure published an invalid frame");
    }

    {
        Harness harness;
        FboTrace trace;
        require(
            harness.renderer.render(
                {200, 100},
                harness.camera,
                1.0f,
                2.0f) == RenderStatus::Rendered,
            "allocation fixture could not establish its composite");
        trace.reset();
        harness.effects.resetCalls();
        ofstub::failAllocationOnAttempt(1);
        require(
            harness.renderer.render(
                {201, 101},
                harness.camera,
                1.5f,
                2.5f) ==
                    RenderStatus::CompositeAllocationFailed &&
                !harness.renderer.hasFrame() &&
                !harness.renderer.drawLatest() &&
                trace.drawEdges().empty() &&
                harness.effects.globalFrames.empty(),
            "failed composite resize retained or presented a stale frame");

        trace.reset();
        require(
            harness.renderer.render(
                {200, 100},
                harness.camera,
                1.75f,
                2.75f) == RenderStatus::Rendered,
            "allocation fixture did not recover after composite failure");
        const auto state = harness.addElement(0);
        trace.reset();
        harness.effects.resetCalls();
        ofstub::failAllocationOnAttempt(1);
        require(
            harness.renderer.render(
                {200, 100},
                harness.camera,
                2.0f,
                3.0f) == RenderStatus::Rendered &&
                state->drawCount == 0 &&
                harness.renderer.hasFrame(),
            "slot allocation failure exposed an unallocated target");

        trace.reset();
        harness.effects.resetCalls();
        require(
            harness.renderer.render(
                {200, 100},
                harness.camera,
                3.0f,
                4.0f) == RenderStatus::Rendered &&
                state->drawCount == 1 &&
                trace.allocationCount() == 1,
            "renderer did not recover from a transient slot allocation failure");
    }
}

} // namespace

int main() {
    try {
        RunBaseLifecycleScenario();
        RunFullAndPartialCoverageScenario();
        RunFailOpenAndRouteScenario();
        RunChainedEffectScenario();
        RunAllocationFailureScenario();
        std::cout
            << "[host_composition_renderer] PASS: "
            << "stub-backed traversal, coverage, fail-open, reuse, release, "
               "and allocation-status scenarios\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "[host_composition_renderer] FAIL: "
            << error.what()
            << "\n";
        return 1;
    }
}

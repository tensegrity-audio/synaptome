#pragma once

#include "CompositionLayer.h"
#include "../core/ParameterValueOrigin.h"

#include <synaptome/element/compat/Layer.h>

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class LayerFactory;
class ParameterRegistry;

namespace synaptome::runtime {

class Runtime {
public:
    enum class ElementErrorCode {
        None,
        InvalidRequest,
        PrefixInUse,
        TypeNotRegistered,
        LifecycleFailure,
        ContractViolation,
    };

    struct ElementRequest {
        std::string typeId;
        std::string definitionId;
        std::string instanceId;
        std::string registryPrefix;
        ofJson config = ofJson::object();
        synaptome::state::ParameterBaseOrigins initialBaseOrigins;
        bool enabled = true;
    };

    struct ElementResult {
        ElementErrorCode errorCode = ElementErrorCode::None;
        std::string stage;
        std::string typeId;
        std::string definitionId;
        std::string instanceId;
        std::string registryPrefix;
        bool enabled = true;
        std::string error;

        ElementResult() = default;
        ~ElementResult();
        ElementResult(const ElementResult&) = delete;
        ElementResult& operator=(const ElementResult&) = delete;
        ElementResult(ElementResult&& other) noexcept;
        ElementResult& operator=(ElementResult&& other) noexcept;

        Layer* element() { return element_.get(); }
        const Layer* element() const { return element_.get(); }
        explicit operator bool() const { return element_ != nullptr; }

    private:
        friend class Runtime;
        // Declared before element_ so element destruction always runs while
        // the setup registry is still alive, including after Runtime expiry.
        std::unique_ptr<ParameterRegistry> stagedParameters_;
        std::unique_ptr<Layer> element_;
        // Declared after element_ so handlers are destroyed before the
        // candidate or retired element they may capture.
        ElementActionTable stagedActions_;
        Layer* replacementElement_ = nullptr;
        // Paired with replacementElement_ so commit never relies on pointer
        // identity alone after a slot has changed.
        std::uint64_t replacementElementRevision_ = 0;
        bool ownsPrefixReservation_ = false;
        Runtime* runtime_ = nullptr;
        std::weak_ptr<void> runtimeLifetime_;
    };

    using ProgressCallback = std::function<void(std::string_view step)>;

    Runtime(const LayerFactory& elementTypes, ParameterRegistry& parameters);
    ~Runtime() noexcept;

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    ElementResult prepareElement(
        const ElementRequest& request,
        const ProgressCallback& progress = {});
    ElementResult prepareCompositionElementReplacement(
        std::size_t zeroBasedIndex,
        const ElementRequest& request,
        const ProgressCallback& progress = {});
    bool hasElementType(const std::string& typeId) const noexcept;

    void releasePreparedElement(ElementResult& prepared) noexcept;

    std::size_t compositionLayerCount() const noexcept {
        return compositionLayers_.size();
    }

    CompositionSnapshot compositionSnapshot() const;
    std::optional<CompositionLayerSnapshot> compositionLayerSnapshot(
        std::size_t zeroBasedIndex) const;
    CompositionActionResult invokeCompositionAction(
        std::size_t zeroBasedIndex,
        std::string_view actionId);
    CompositionTelemetryResult compositionElementTelemetry(
        std::size_t zeroBasedIndex) const;

    CompositionCoverageWindow resolveEffectCoverage(
        std::size_t effectLayerIndex,
        float coverage) const noexcept;

    CompositionMutationResult adoptPreparedElement(
        std::size_t zeroBasedIndex,
        ElementResult&& prepared,
        CompositionAssignment assignment);
    CompositionMutationResult assignCompositionEntry(
        std::size_t zeroBasedIndex,
        CompositionAssignment assignment);
    CompositionMutationResult setCompositionLayerActive(
        std::size_t zeroBasedIndex,
        bool active);
    CompositionMutationResult setCompositionLayerLabel(
        std::size_t zeroBasedIndex,
        std::string label);
    CompositionMutationResult setCompositionLayerCoverage(
        std::size_t zeroBasedIndex,
        CompositionCoverage coverage);
    CompositionMutationResult clearCompositionLayer(
        std::size_t zeroBasedIndex);

    void resizeCompositionElements(int width, int height);
    void updateCompositionElements(const LayerUpdateParams& params);
    void drawCompositionElement(
        std::size_t zeroBasedIndex,
        const LayerDrawParams& params);
    void shutdownComposition();

private:
    enum class ParameterKind {
        Float,
        Bool,
        String,
    };

    struct ParameterKey {
        ParameterKind kind;
        std::string id;
    };

    struct ElementOwnership {
        std::string prefix;
        std::vector<ParameterKey> parameters;
    };

    bool prefixIsAvailable(const std::string& prefix) const;
    static std::vector<ParameterKey> parameterSnapshot(
        const ParameterRegistry& parameters);
    static bool idBelongsToPrefix(
        const std::string& id,
        const std::string& prefix);
    static bool isReservedCompositionParameter(
        const std::string& id,
        const std::string& prefix);
    ElementResult prepareElementImpl(
        const ElementRequest& request,
        Layer* replacing,
        const ProgressCallback& progress);
    CompositionLayer* mutableCompositionLayer(
        std::size_t zeroBasedIndex) noexcept;
    CompositionMutationResult adoptPreparedElementImpl(
        std::size_t zeroBasedIndex,
        ElementResult&& prepared,
        CompositionAssignment& assignment);
    static CompositionCoverage normalizeCoverage(
        CompositionCoverage coverage);
    static float normalizeOpacity(float opacity) noexcept;
    static CompositionLayerSnapshot snapshotCompositionLayer(
        const CompositionLayer& layer,
        std::size_t zeroBasedIndex);
    void forceClearCompositionLayerNoexcept(
        CompositionLayer& layer) noexcept;
    void removeParameters(const std::vector<ParameterKey>& parameters) noexcept;
    void releaseTrackedElement(Layer* element) noexcept;
    void releaseElement(std::unique_ptr<Layer>& element) noexcept;

    const LayerFactory& elementTypes_;
    ParameterRegistry& parameters_;
    std::unordered_map<Layer*, ElementOwnership> ownership_;
    std::unordered_set<std::string> activePrefixes_;
    std::shared_ptr<void> lifetime_ = std::make_shared<int>(0);
    std::array<CompositionLayer, kCompositionLayerCount> compositionLayers_;
};

} // namespace synaptome::runtime

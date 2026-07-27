#pragma once

#include "CompositionLayer.h"

#include <synaptome/element/compat/Layer.h>

#include <array>
#include <functional>
#include <memory>
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
        bool enabled = true;
    };

    struct ElementResult {
        ElementErrorCode errorCode = ElementErrorCode::None;
        std::string stage;
        std::string typeId;
        std::string definitionId;
        std::string instanceId;
        std::string registryPrefix;
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
        Layer* replacementElement_ = nullptr;
        bool ownsPrefixReservation_ = false;
        Runtime* runtime_ = nullptr;
        std::weak_ptr<void> runtimeLifetime_;
    };

    using ProgressCallback = std::function<void(std::string_view step)>;

    Runtime(LayerFactory& factory, ParameterRegistry& parameters);
    ~Runtime() noexcept;

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) = delete;
    Runtime& operator=(Runtime&&) = delete;

    ElementResult prepareElement(
        const ElementRequest& request,
        const ProgressCallback& progress = {});
    ElementResult prepareElementReplacement(
        const ElementRequest& request,
        Layer& replacing,
        const ProgressCallback& progress = {});

    void releasePreparedElement(ElementResult& prepared) noexcept;

    using CompositionLayers =
        std::array<CompositionLayer, kCompositionLayerCount>;

    CompositionLayers& compositionLayersForHost() { return compositionLayers_; }
    const CompositionLayers& compositionLayersForHost() const {
        return compositionLayers_;
    }
    CompositionLayer* compositionLayer(std::size_t zeroBasedIndex);
    const CompositionLayer* compositionLayer(std::size_t zeroBasedIndex) const;

    bool adoptPreparedElement(
        std::size_t zeroBasedIndex,
        ElementResult&& prepared);
    void releaseCompositionElement(std::size_t zeroBasedIndex) noexcept;
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
    void removeParameters(const std::vector<ParameterKey>& parameters) noexcept;
    void releaseTrackedElement(Layer* element) noexcept;
    void releaseElement(std::unique_ptr<Layer>& element) noexcept;

    LayerFactory& factory_;
    ParameterRegistry& parameters_;
    std::unordered_map<Layer*, ElementOwnership> ownership_;
    std::unordered_set<std::string> activePrefixes_;
    std::shared_ptr<void> lifetime_ = std::make_shared<int>(0);
    CompositionLayers compositionLayers_;
};

} // namespace synaptome::runtime

#include "Runtime.h"

#include "../core/ParameterRegistry.h"
#include "../visuals/LayerFactory.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <utility>

namespace synaptome::runtime {

bool Runtime::idBelongsToPrefix(
    const std::string& id,
    const std::string& prefix) {
    return id == prefix ||
        (id.size() > prefix.size() &&
         id.compare(0, prefix.size(), prefix) == 0 &&
         id[prefix.size()] == '.');
}

bool Runtime::isReservedCompositionParameter(
    const std::string& id,
    const std::string& prefix) {
    // Whole-layer opacity belongs to the composition container. Elements may
    // expose their own internal alpha controls under a different stable ID.
    return id == prefix + ".opacity";
}

Runtime::ElementResult::~ElementResult() {
    if (runtime_ && !runtimeLifetime_.expired()) {
        runtime_->releasePreparedElement(*this);
    } else {
        // A prepared result may outlive its Runtime. Keep the private setup
        // registry alive until after the candidate's destructor has run.
        element_.reset();
        stagedParameters_.reset();
    }
}

Runtime::ElementResult::ElementResult(ElementResult&& other) noexcept
    : errorCode(other.errorCode),
      stage(std::move(other.stage)),
      typeId(std::move(other.typeId)),
      definitionId(std::move(other.definitionId)),
      instanceId(std::move(other.instanceId)),
      registryPrefix(std::move(other.registryPrefix)),
      error(std::move(other.error)),
      stagedParameters_(std::move(other.stagedParameters_)),
      element_(std::move(other.element_)),
      replacementElement_(std::exchange(other.replacementElement_, nullptr)),
      ownsPrefixReservation_(
          std::exchange(other.ownsPrefixReservation_, false)),
      runtime_(std::exchange(other.runtime_, nullptr)),
      runtimeLifetime_(std::move(other.runtimeLifetime_)) {}

Runtime::ElementResult& Runtime::ElementResult::operator=(
    ElementResult&& other) noexcept {
    if (this == &other) return *this;
    if (runtime_ && !runtimeLifetime_.expired()) {
        runtime_->releasePreparedElement(*this);
    }
    element_ = std::move(other.element_);
    stagedParameters_ = std::move(other.stagedParameters_);
    replacementElement_ =
        std::exchange(other.replacementElement_, nullptr);
    ownsPrefixReservation_ =
        std::exchange(other.ownsPrefixReservation_, false);
    errorCode = other.errorCode;
    stage = std::move(other.stage);
    typeId = std::move(other.typeId);
    definitionId = std::move(other.definitionId);
    instanceId = std::move(other.instanceId);
    registryPrefix = std::move(other.registryPrefix);
    error = std::move(other.error);
    runtime_ = std::exchange(other.runtime_, nullptr);
    runtimeLifetime_ = std::move(other.runtimeLifetime_);
    return *this;
}

Runtime::Runtime(
    const LayerFactory& elementTypes,
    ParameterRegistry& parameters)
    : elementTypes_(elementTypes),
      parameters_(parameters) {}

Runtime::~Runtime() noexcept {
    lifetime_.reset();
    for (const auto& entry : ownership_) {
        removeParameters(entry.second.parameters);
    }
}

bool Runtime::prefixIsAvailable(const std::string& prefix) const {
    if (activePrefixes_.find(prefix) != activePrefixes_.end()) {
        return false;
    }
    for (const auto& entry : parameters_.floats()) {
        if (idBelongsToPrefix(entry.meta.id, prefix)) return false;
    }
    for (const auto& entry : parameters_.bools()) {
        if (idBelongsToPrefix(entry.meta.id, prefix)) return false;
    }
    for (const auto& entry : parameters_.strings()) {
        if (idBelongsToPrefix(entry.meta.id, prefix)) return false;
    }
    return true;
}

std::vector<Runtime::ParameterKey> Runtime::parameterSnapshot(
    const ParameterRegistry& parameters) {
    std::vector<ParameterKey> result;
    result.reserve(
        parameters.floats().size() +
        parameters.bools().size() +
        parameters.strings().size());
    for (const auto& entry : parameters.floats()) {
        result.push_back({ParameterKind::Float, entry.meta.id});
    }
    for (const auto& entry : parameters.bools()) {
        result.push_back({ParameterKind::Bool, entry.meta.id});
    }
    for (const auto& entry : parameters.strings()) {
        result.push_back({ParameterKind::String, entry.meta.id});
    }
    return result;
}

void Runtime::removeParameters(
    const std::vector<ParameterKey>& parameters) noexcept {
    for (const auto& parameter : parameters) {
        parameters_.removeById(parameter.id);
    }
}

Runtime::ElementResult Runtime::prepareElement(
    const ElementRequest& request,
    const ProgressCallback& progress) {
    return prepareElementImpl(request, nullptr, progress);
}

Runtime::ElementResult Runtime::prepareElementReplacement(
    const ElementRequest& request,
    Layer& replacing,
    const ProgressCallback& progress) {
    return prepareElementImpl(request, &replacing, progress);
}

bool Runtime::hasElementType(const std::string& typeId) const noexcept {
    return elementTypes_.contains(typeId);
}

Runtime::ElementResult Runtime::prepareElementImpl(
    const ElementRequest& request,
    Layer* replacing,
    const ProgressCallback& progress) {
    ElementResult result;
    result.stage = "validate";
    result.typeId = request.typeId;
    result.definitionId = request.definitionId;
    result.instanceId = request.instanceId;
    result.registryPrefix = request.registryPrefix;
    if (request.typeId.empty()) {
        result.errorCode = ElementErrorCode::InvalidRequest;
        result.error = "element type ID is empty";
        return result;
    }
    if (request.definitionId.empty()) {
        result.errorCode = ElementErrorCode::InvalidRequest;
        result.error = "element definition ID is empty";
        return result;
    }
    if (request.instanceId.empty()) {
        result.errorCode = ElementErrorCode::InvalidRequest;
        result.error = "element instance ID is empty";
        return result;
    }
    if (request.registryPrefix.empty()) {
        result.errorCode = ElementErrorCode::InvalidRequest;
        result.error = "element registry prefix is empty";
        return result;
    }
    const auto replacedOwnership =
        replacing ? ownership_.find(replacing) : ownership_.end();
    if (replacing &&
        (replacedOwnership == ownership_.end() ||
         replacedOwnership->second.prefix != request.registryPrefix)) {
        result.errorCode = ElementErrorCode::InvalidRequest;
        result.error =
            "replacement element does not own the requested registry prefix";
        return result;
    }
    if (!replacing && !prefixIsAvailable(request.registryPrefix)) {
        result.errorCode = ElementErrorCode::PrefixInUse;
        result.error = "element registry prefix is already in use: " +
            request.registryPrefix;
        return result;
    }

    if (!replacing) {
        activePrefixes_.insert(request.registryPrefix);
        result.ownsPrefixReservation_ = true;
    }
    try {
        result.stage = "create";
        result.element_ = elementTypes_.create(request.typeId);
        if (progress) progress("create");
        if (!result.element_) {
            if (result.ownsPrefixReservation_) {
                activePrefixes_.erase(request.registryPrefix);
                result.ownsPrefixReservation_ = false;
            }
            result.errorCode = ElementErrorCode::TypeNotRegistered;
            result.error = "element type is not registered: " + request.typeId;
            return result;
        }

        result.element_->setRegistryPrefix(request.registryPrefix);
        result.element_->setInstanceId(request.instanceId);
        result.stage = "configure";
        result.element_->configure(request.config);
        if (progress) progress("configure");

        result.stagedParameters_ = std::make_unique<ParameterRegistry>();
        result.stage = "setup";
        result.element_->setup(*result.stagedParameters_);
        const auto registeredParameters =
            parameterSnapshot(*result.stagedParameters_);
        const auto invalidParameter = std::find_if(
            registeredParameters.begin(),
            registeredParameters.end(),
            [&](const ParameterKey& parameter) {
                return !idBelongsToPrefix(
                    parameter.id,
                    request.registryPrefix) ||
                    isReservedCompositionParameter(
                        parameter.id,
                        request.registryPrefix);
            });
        if (invalidParameter != registeredParameters.end()) {
            if (result.ownsPrefixReservation_) {
                activePrefixes_.erase(request.registryPrefix);
                result.ownsPrefixReservation_ = false;
            }
            result.element_.reset();
            result.stagedParameters_.reset();
            result.errorCode = ElementErrorCode::ContractViolation;
            result.error = isReservedCompositionParameter(
                               invalidParameter->id,
                               request.registryPrefix)
                ? "element registered a layer-container reserved parameter: " +
                    invalidParameter->id
                : "element registered a parameter outside its namespace: " +
                    invalidParameter->id;
            return result;
        }
        if (progress) progress("setup");

        result.stage = "enable";
        result.element_->setExternalEnabled(request.enabled);
        if (progress) progress("enable");

        result.runtime_ = this;
        result.runtimeLifetime_ = lifetime_;
        result.replacementElement_ = replacing;
        result.stage = "ready";
        return result;
    } catch (const std::exception& error) {
        if (result.ownsPrefixReservation_) {
            activePrefixes_.erase(request.registryPrefix);
            result.ownsPrefixReservation_ = false;
        }
        result.element_.reset();
        result.stagedParameters_.reset();
        result.errorCode = ElementErrorCode::LifecycleFailure;
        result.error = error.what();
        return result;
    } catch (...) {
        if (result.ownsPrefixReservation_) {
            activePrefixes_.erase(request.registryPrefix);
            result.ownsPrefixReservation_ = false;
        }
        result.element_.reset();
        result.stagedParameters_.reset();
        result.errorCode = ElementErrorCode::LifecycleFailure;
        result.error = "unknown element lifecycle failure";
        return result;
    }
}

void Runtime::releaseTrackedElement(Layer* element) noexcept {
    auto found = ownership_.find(element);
    if (found == ownership_.end()) return;
    removeParameters(found->second.parameters);
    activePrefixes_.erase(found->second.prefix);
    ownership_.erase(found);
}

void Runtime::releaseElement(std::unique_ptr<Layer>& element) noexcept {
    if (!element) return;
    releaseTrackedElement(element.get());
    element.reset();
}

void Runtime::releasePreparedElement(ElementResult& prepared) noexcept {
    if (prepared.runtime_ != this || prepared.runtimeLifetime_.expired()) return;
    if (prepared.ownsPrefixReservation_) {
        activePrefixes_.erase(prepared.registryPrefix);
    }
    prepared.element_.reset();
    prepared.stagedParameters_.reset();
    prepared.replacementElement_ = nullptr;
    prepared.ownsPrefixReservation_ = false;
    prepared.runtime_ = nullptr;
    prepared.runtimeLifetime_.reset();
}

CompositionLayer* Runtime::compositionLayer(
    std::size_t zeroBasedIndex) {
    if (zeroBasedIndex >= compositionLayers_.size()) return nullptr;
    return &compositionLayers_[zeroBasedIndex];
}

const CompositionLayer* Runtime::compositionLayer(
    std::size_t zeroBasedIndex) const {
    if (zeroBasedIndex >= compositionLayers_.size()) return nullptr;
    return &compositionLayers_[zeroBasedIndex];
}

CompositionCoverageWindow Runtime::resolveEffectCoverage(
    std::size_t effectLayerIndex,
    float coverage) const noexcept {
    CompositionCoverageWindow window;
    window.effectLayerIndex = effectLayerIndex;
    if (effectLayerIndex >= compositionLayers_.size()) {
        return window;
    }

    window.inputEndLayerIndex = effectLayerIndex;
    if (std::isfinite(coverage) && coverage > 0.0f) {
        const double requested = std::floor(
            static_cast<double>(coverage) + 0.0001);
        window.requestedLayers =
            requested >= static_cast<double>(std::numeric_limits<int>::max())
            ? std::numeric_limits<int>::max()
            : static_cast<int>(requested);
    } else if (coverage > 0.0f) {
        window.requestedLayers = std::numeric_limits<int>::max();
    }

    const auto priorLayerCount = effectLayerIndex;
    if (window.requestedLayers <= 0 ||
        static_cast<std::size_t>(window.requestedLayers) >= priorLayerCount) {
        window.firstInputLayerIndex = 0;
        window.includesAllPrior = true;
    } else {
        window.firstInputLayerIndex =
            priorLayerCount -
            static_cast<std::size_t>(window.requestedLayers);
    }
    return window;
}

bool Runtime::adoptPreparedElement(
    std::size_t zeroBasedIndex,
    ElementResult&& prepared) {
    auto* layer = compositionLayer(zeroBasedIndex);
    if (!layer ||
        !prepared.element_ ||
        !prepared.stagedParameters_ ||
        prepared.runtime_ != this ||
        prepared.runtimeLifetime_.expired() ||
        prepared.registryPrefix !=
            "console.layer" + std::to_string(zeroBasedIndex + 1)) {
        return false;
    }
    Layer* const oldElement = layer->element_.get();
    if (oldElement != prepared.replacementElement_) return false;

    const auto oldOwnership =
        oldElement ? ownership_.find(oldElement) : ownership_.end();
    if (oldElement && oldOwnership == ownership_.end()) return false;
    if (!oldElement && prepared.replacementElement_) return false;

    try {
        std::vector<std::string> removedIds;
        if (oldOwnership != ownership_.end()) {
            removedIds.reserve(oldOwnership->second.parameters.size());
            for (const auto& parameter : oldOwnership->second.parameters) {
                removedIds.push_back(parameter.id);
            }
        }

        auto nextParameters = parameters_.replacingIds(
            removedIds,
            *prepared.stagedParameters_);
        auto nextOwnership = ownership_;
        if (oldElement) nextOwnership.erase(oldElement);
        Layer* const candidate = prepared.element_.get();
        nextOwnership.emplace(
            candidate,
            ElementOwnership{
                prepared.registryPrefix,
                parameterSnapshot(*prepared.stagedParameters_)});
        auto nextActivePrefixes = activePrefixes_;
        nextActivePrefixes.insert(prepared.registryPrefix);

        parameters_.swap(nextParameters);
        candidate->onParameterRegistryCommitted(parameters_);
        ownership_.swap(nextOwnership);
        activePrefixes_.swap(nextActivePrefixes);
        layer->element_.swap(prepared.element_);

        prepared.stagedParameters_.reset();
        prepared.replacementElement_ = nullptr;
        prepared.ownsPrefixReservation_ = false;
        prepared.runtime_ = nullptr;
        prepared.runtimeLifetime_.reset();
        return true;
    } catch (...) {
        return false;
    }
}

void Runtime::releaseCompositionElement(
    std::size_t zeroBasedIndex) noexcept {
    auto* layer = compositionLayer(zeroBasedIndex);
    if (!layer) return;
    releaseElement(layer->element_);
}

void Runtime::resizeCompositionElements(int width, int height) {
    for (auto& layer : compositionLayers_) {
        if (layer.element_) {
            layer.element_->onWindowResized(width, height);
        }
    }
}

void Runtime::updateCompositionElements(const LayerUpdateParams& params) {
    for (auto& layer : compositionLayers_) {
        if (!layer.active || !layer.element_) continue;
        layer.element_->update(params);
    }
}

void Runtime::drawCompositionElement(
    std::size_t zeroBasedIndex,
    const LayerDrawParams& params) {
    auto* layer = compositionLayer(zeroBasedIndex);
    if (!layer || !layer->active || !layer->element_) return;
    layer->element_->draw(params);
}

void Runtime::shutdownComposition() {
    for (auto& layer : compositionLayers_) {
        releaseElement(layer.element_);
        layer.layerFbo.clear();
        layer.upstreamFbo.clear();
        layer.effectFbo.clear();
    }
}

} // namespace synaptome::runtime

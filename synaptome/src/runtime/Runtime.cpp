#include "Runtime.h"

#include "../core/ParameterRegistry.h"
#include "../visuals/LayerFactory.h"

#include <algorithm>
#include <exception>
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

Runtime::ElementResult::~ElementResult() {
    if (runtime_ && !runtimeLifetime_.expired() && element_) {
        runtime_->releaseElement(element_);
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
      element_(std::move(other.element_)),
      runtime_(std::exchange(other.runtime_, nullptr)),
      runtimeLifetime_(std::move(other.runtimeLifetime_)) {}

Runtime::ElementResult& Runtime::ElementResult::operator=(
    ElementResult&& other) noexcept {
    if (this == &other) return *this;
    if (runtime_ && !runtimeLifetime_.expired() && element_) {
        runtime_->releaseElement(element_);
    }
    element_ = std::move(other.element_);
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

Runtime::Runtime(LayerFactory& factory, ParameterRegistry& parameters)
    : factory_(factory),
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

std::vector<Runtime::ParameterKey> Runtime::parameterSnapshot() const {
    std::vector<ParameterKey> result;
    result.reserve(
        parameters_.floats().size() +
        parameters_.bools().size() +
        parameters_.strings().size());
    for (const auto& entry : parameters_.floats()) {
        result.push_back({ParameterKind::Float, entry.meta.id});
    }
    for (const auto& entry : parameters_.bools()) {
        result.push_back({ParameterKind::Bool, entry.meta.id});
    }
    for (const auto& entry : parameters_.strings()) {
        result.push_back({ParameterKind::String, entry.meta.id});
    }
    return result;
}

std::vector<Runtime::ParameterKey> Runtime::parameterDelta(
    const std::vector<ParameterKey>& before,
    const std::vector<ParameterKey>& after) {
    std::vector<ParameterKey> result;
    for (const auto& candidate : after) {
        const bool existed = std::any_of(
            before.begin(),
            before.end(),
            [&](const ParameterKey& entry) {
                return entry.kind == candidate.kind && entry.id == candidate.id;
            });
        if (!existed) result.push_back(candidate);
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
    if (!prefixIsAvailable(request.registryPrefix)) {
        result.errorCode = ElementErrorCode::PrefixInUse;
        result.error = "element registry prefix is already in use: " +
            request.registryPrefix;
        return result;
    }

    activePrefixes_.insert(request.registryPrefix);
    std::vector<ParameterKey> registeredParameters;
    std::vector<ParameterKey> parametersBeforeSetup;
    bool setupStarted = false;
    try {
        result.stage = "create";
        result.element_ = factory_.create(request.typeId);
        if (progress) progress("create");
        if (!result.element_) {
            activePrefixes_.erase(request.registryPrefix);
            result.errorCode = ElementErrorCode::TypeNotRegistered;
            result.error = "element type is not registered: " + request.typeId;
            return result;
        }

        result.element_->setRegistryPrefix(request.registryPrefix);
        result.element_->setInstanceId(request.instanceId);
        result.stage = "configure";
        result.element_->configure(request.config);
        if (progress) progress("configure");

        parametersBeforeSetup = parameterSnapshot();
        setupStarted = true;
        result.stage = "setup";
        result.element_->setup(parameters_);
        registeredParameters = parameterDelta(
            parametersBeforeSetup,
            parameterSnapshot());
        const auto foreignParameter = std::find_if(
            registeredParameters.begin(),
            registeredParameters.end(),
            [&](const ParameterKey& parameter) {
                return !idBelongsToPrefix(
                    parameter.id,
                    request.registryPrefix);
            });
        if (foreignParameter != registeredParameters.end()) {
            removeParameters(registeredParameters);
            activePrefixes_.erase(request.registryPrefix);
            result.element_.reset();
            result.errorCode = ElementErrorCode::ContractViolation;
            result.error = "element registered a parameter outside its namespace: " +
                foreignParameter->id;
            return result;
        }
        if (progress) progress("setup");

        result.stage = "enable";
        result.element_->setExternalEnabled(request.enabled);
        if (progress) progress("enable");

        Layer* element = result.element_.get();
        result.runtime_ = this;
        result.runtimeLifetime_ = lifetime_;
        result.stage = "ready";
        ownership_.emplace(
            element,
            ElementOwnership{request.registryPrefix, registeredParameters});
        return result;
    } catch (const std::exception& error) {
        if (setupStarted) {
            registeredParameters = parameterDelta(
                parametersBeforeSetup,
                parameterSnapshot());
        }
        removeParameters(registeredParameters);
        activePrefixes_.erase(request.registryPrefix);
        result.element_.reset();
        result.errorCode = ElementErrorCode::LifecycleFailure;
        result.error = error.what();
        return result;
    } catch (...) {
        if (setupStarted) {
            registeredParameters = parameterDelta(
                parametersBeforeSetup,
                parameterSnapshot());
        }
        removeParameters(registeredParameters);
        activePrefixes_.erase(request.registryPrefix);
        result.element_.reset();
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
    releaseElement(prepared.element_);
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

bool Runtime::adoptPreparedElement(
    std::size_t zeroBasedIndex,
    ElementResult&& prepared) {
    auto* layer = compositionLayer(zeroBasedIndex);
    if (!layer ||
        layer->element_ ||
        !prepared.element_ ||
        prepared.runtime_ != this ||
        prepared.runtimeLifetime_.expired() ||
        ownership_.find(prepared.element_.get()) == ownership_.end() ||
        prepared.registryPrefix !=
            "console.layer" + std::to_string(zeroBasedIndex + 1)) {
        return false;
    }
    layer->element_ = std::move(prepared.element_);
    return true;
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

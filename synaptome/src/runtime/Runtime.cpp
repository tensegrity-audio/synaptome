#include "Runtime.h"

#include "ElementParameterTable.h"
#include "ElementTelemetryBuffer.h"
#include "../core/ParameterRegistry.h"
#include "../visuals/LayerFactory.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
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
        stagedActions_.clear();
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
      enabled(other.enabled),
      error(std::move(other.error)),
      stagedParameters_(std::move(other.stagedParameters_)),
      element_(std::move(other.element_)),
      stagedActions_(std::move(other.stagedActions_)),
      replacementElement_(std::exchange(other.replacementElement_, nullptr)),
      replacementElementRevision_(
          std::exchange(other.replacementElementRevision_, 0)),
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
    stagedActions_ = std::move(other.stagedActions_);
    element_ = std::move(other.element_);
    stagedParameters_ = std::move(other.stagedParameters_);
    replacementElement_ =
        std::exchange(other.replacementElement_, nullptr);
    replacementElementRevision_ =
        std::exchange(other.replacementElementRevision_, 0);
    ownsPrefixReservation_ =
        std::exchange(other.ownsPrefixReservation_, false);
    errorCode = other.errorCode;
    stage = std::move(other.stage);
    typeId = std::move(other.typeId);
    definitionId = std::move(other.definitionId);
    instanceId = std::move(other.instanceId);
    registryPrefix = std::move(other.registryPrefix);
    enabled = other.enabled;
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
    for (auto& layer : compositionLayers_) {
        if (!layer.opacityParameterId_.empty()) {
            parameters_.removeById(layer.opacityParameterId_);
        }
    }
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

Runtime::ElementResult Runtime::prepareCompositionElementReplacement(
    std::size_t zeroBasedIndex,
    const ElementRequest& request,
    const ProgressCallback& progress) {
    if (zeroBasedIndex >= compositionLayers_.size() ||
        compositionLayers_[zeroBasedIndex].kind != CompositionKind::Element ||
        !compositionLayers_[zeroBasedIndex].element_) {
        ElementResult result;
        result.errorCode = ElementErrorCode::InvalidRequest;
        result.stage = "validate";
        result.typeId = request.typeId;
        result.definitionId = request.definitionId;
        result.instanceId = request.instanceId;
        result.registryPrefix = request.registryPrefix;
        result.enabled = request.enabled;
        result.error = zeroBasedIndex >= compositionLayers_.size()
            ? "composition layer index is out of range"
            : "composition layer does not contain a replaceable element";
        return result;
    }
    auto result = prepareElementImpl(
        request,
        compositionLayers_[zeroBasedIndex].element_.get(),
        progress);
    if (result) {
        result.replacementElementRevision_ =
            compositionLayers_[zeroBasedIndex].elementRevision_;
    }
    return result;
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
    result.enabled = request.enabled;
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

    result.stage = "descriptor";
    const auto* descriptor = elementTypes_.descriptor(request.typeId);
    if (!descriptor) {
        result.errorCode = ElementErrorCode::TypeNotRegistered;
        result.error = "element type is not registered: " + request.typeId;
        return result;
    }
    if (descriptor->kind != element::ElementKind::Visual) {
        result.errorCode = ElementErrorCode::ContractViolation;
        result.error =
            "composition element requires a Visual element descriptor: " +
            request.typeId;
        return result;
    }
    const auto* typeContract =
        elementTypes_.typeContract(request.typeId);
    if (!typeContract) {
        result.errorCode = ElementErrorCode::ContractViolation;
        result.error =
            "registered element type has no type-contract record: " +
            request.typeId;
        return result;
    }

    if (!replacing) {
        activePrefixes_.insert(request.registryPrefix);
        result.ownsPrefixReservation_ = true;
    }
    try {
        result.stagedActions_ =
            ElementActionTable(descriptor->actions);
        result.stage = "create";
        result.element_ = elementTypes_.create(request.typeId);
        if (progress) progress("create");
        if (!result.element_) {
            throw std::logic_error(
                "registered element creator returned no element: " +
                request.typeId);
        }

        result.element_->setRegistryPrefix(request.registryPrefix);
        result.element_->setInstanceId(request.instanceId);
        result.stagedParameters_ = std::make_unique<ParameterRegistry>();
        if (typeContract->state ==
            LayerFactory::ParameterDeclarationState::Declared) {
            ElementParameterTable parameterTable(
                typeContract->contract.parameters);
            if (typeContract->bindingMode ==
                LayerFactory::ParameterBindingMode::Explicit) {
                result.stage = "bind";
                auto* bindable =
                    dynamic_cast<element::ParameterBindable*>(
                        result.element_.get());
                if (!bindable) {
                    if (result.ownsPrefixReservation_) {
                        activePrefixes_.erase(request.registryPrefix);
                        result.ownsPrefixReservation_ = false;
                    }
                    result.element_.reset();
                    result.stagedParameters_.reset();
                    result.errorCode = ElementErrorCode::ContractViolation;
                    result.error =
                        "explicitly bound declared element does not implement "
                        "parameter binding: " + request.typeId;
                    return result;
                }
                bindable->bindParameters(parameterTable);
                const auto parameterContractError =
                    parameterTable.contractError();
                if (!parameterContractError.empty()) {
                    if (result.ownsPrefixReservation_) {
                        activePrefixes_.erase(request.registryPrefix);
                        result.ownsPrefixReservation_ = false;
                    }
                    result.element_.reset();
                    result.stagedParameters_.reset();
                    result.errorCode = ElementErrorCode::ContractViolation;
                    result.error = parameterContractError;
                    return result;
                }
                parameterTable.applyDeclarationDefaults();

                result.stage = "configure";
                result.element_->configure(request.config);
                if (progress) progress("configure");

                // setup remains a resource-initialization hook for explicitly
                // bound elements. Static declarations own all parameter
                // metadata and defaults.
                result.stage = "setup";
                result.element_->setup(*result.stagedParameters_);
                if (!parameterSnapshot(*result.stagedParameters_).empty()) {
                    if (result.ownsPrefixReservation_) {
                        activePrefixes_.erase(request.registryPrefix);
                        result.ownsPrefixReservation_ = false;
                    }
                    result.element_.reset();
                    result.stagedParameters_.reset();
                    result.errorCode = ElementErrorCode::ContractViolation;
                    result.error =
                        "explicitly bound declared element registered "
                        "parameter metadata during setup: " + request.typeId;
                    return result;
                }
            } else {
                // Transitional built-ins still expose their storage through
                // setup(). The generated declaration is authoritative:
                // setup metadata is discarded after exact ID/kind binding.
                result.stage = "configure";
                result.element_->configure(request.config);
                if (progress) progress("configure");

                result.stage = "setup";
                result.element_->setup(*result.stagedParameters_);
                parameterTable.bindLegacyRegistry(
                    request.registryPrefix,
                    *result.stagedParameters_);
                const auto parameterContractError =
                    parameterTable.contractError();
                if (!parameterContractError.empty()) {
                    if (result.ownsPrefixReservation_) {
                        activePrefixes_.erase(request.registryPrefix);
                        result.ownsPrefixReservation_ = false;
                    }
                    result.element_.reset();
                    result.stagedParameters_.reset();
                    result.errorCode = ElementErrorCode::ContractViolation;
                    result.error = parameterContractError;
                    return result;
                }
                result.stagedParameters_ =
                    std::make_unique<ParameterRegistry>();
            }
            const auto parameterContractError =
                parameterTable.contractError();
            if (!parameterContractError.empty()) {
                if (result.ownsPrefixReservation_) {
                    activePrefixes_.erase(request.registryPrefix);
                    result.ownsPrefixReservation_ = false;
                }
                result.element_.reset();
                result.stagedParameters_.reset();
                result.errorCode = ElementErrorCode::ContractViolation;
                result.error = parameterContractError;
                return result;
            }
            parameterTable.populate(
                request.registryPrefix,
                *result.stagedParameters_);
        } else {
            result.stage = "configure";
            result.element_->configure(request.config);
            if (progress) progress("configure");

            result.stage = "setup";
            result.element_->setup(*result.stagedParameters_);
        }

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

        result.stage = "actions";
        result.element_->registerActions(result.stagedActions_);
        const auto actionContractError =
            result.stagedActions_.contractError();
        if (!actionContractError.empty()) {
            if (result.ownsPrefixReservation_) {
                activePrefixes_.erase(request.registryPrefix);
                result.ownsPrefixReservation_ = false;
            }
            result.stagedActions_.clear();
            result.element_.reset();
            result.stagedParameters_.reset();
            result.errorCode = ElementErrorCode::ContractViolation;
            result.error = actionContractError;
            return result;
        }

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
        result.stagedActions_.clear();
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
        result.stagedActions_.clear();
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
    prepared.stagedActions_.clear();
    prepared.element_.reset();
    prepared.stagedParameters_.reset();
    prepared.replacementElement_ = nullptr;
    prepared.replacementElementRevision_ = 0;
    prepared.ownsPrefixReservation_ = false;
    prepared.runtime_ = nullptr;
    prepared.runtimeLifetime_.reset();
}

CompositionLayer* Runtime::mutableCompositionLayer(
    std::size_t zeroBasedIndex) noexcept {
    if (zeroBasedIndex >= compositionLayers_.size()) return nullptr;
    return &compositionLayers_[zeroBasedIndex];
}

CompositionLayerSnapshot Runtime::snapshotCompositionLayer(
    const CompositionLayer& layer,
    std::size_t zeroBasedIndex) {
    CompositionLayerSnapshot state;
    state.zeroBasedIndex = zeroBasedIndex;
    state.occupied = !layer.assetId.empty();
    if (!state.occupied) {
        return state;
    }
    state.hasElement = layer.element_ != nullptr;
    state.kind = layer.kind;
    state.definitionId = layer.assetId;
    state.label = layer.label;
    state.typeId = layer.type;
    state.registryPrefix = layer.paramPrefix;
    state.active = layer.active;
    state.opacity = layer.opacity;
    state.coverage = layer.coverage;
    state.actions = layer.actions_.descriptors();
    return state;
}

CompositionSnapshot Runtime::compositionSnapshot() const {
    CompositionSnapshot snapshot;
    for (std::size_t i = 0; i < compositionLayers_.size(); ++i) {
        snapshot.layers[i] =
            snapshotCompositionLayer(compositionLayers_[i], i);
    }
    return snapshot;
}

std::optional<CompositionLayerSnapshot> Runtime::compositionLayerSnapshot(
    std::size_t zeroBasedIndex) const {
    if (zeroBasedIndex >= compositionLayers_.size()) {
        return std::nullopt;
    }
    return snapshotCompositionLayer(
        compositionLayers_[zeroBasedIndex],
        zeroBasedIndex);
}

CompositionActionResult Runtime::invokeCompositionAction(
    std::size_t zeroBasedIndex,
    std::string_view actionId) {
    auto fail = [actionId](
        CompositionActionError code,
        std::string error) {
        CompositionActionResult result;
        result.errorCode = code;
        result.actionId = std::string(actionId);
        result.error = std::move(error);
        return result;
    };

    if (zeroBasedIndex >= compositionLayers_.size()) {
        return fail(
            CompositionActionError::IndexOutOfRange,
            "composition layer index is out of range");
    }
    auto& layer = compositionLayers_[zeroBasedIndex];
    if (layer.assetId.empty()) {
        return fail(
            CompositionActionError::SlotEmpty,
            "composition layer is empty");
    }
    if (layer.kind != CompositionKind::Element) {
        return fail(
            CompositionActionError::KindMismatch,
            "composition action requires an Element composition entry");
    }
    if (!layer.element_) {
        return fail(
            CompositionActionError::SlotEmpty,
            "composition layer does not contain an element");
    }
    const auto* handler = layer.actions_.find(actionId);
    if (!handler) {
        return fail(
            CompositionActionError::ActionNotFound,
            "composition element does not declare action: " +
                std::string(actionId));
    }

    try {
        const auto execution = (*handler)();
        switch (execution.status) {
        case element::ActionExecutionStatus::Succeeded: {
            CompositionActionResult result;
            result.actionId = std::string(actionId);
            return result;
        }
        case element::ActionExecutionStatus::Rejected:
            return fail(
                CompositionActionError::Rejected,
                execution.message.empty()
                    ? "composition action was rejected"
                    : execution.message);
        case element::ActionExecutionStatus::Failed:
            return fail(
                CompositionActionError::ExecutionFailure,
                execution.message.empty()
                    ? "composition action execution failed"
                    : execution.message);
        }
        return fail(
            CompositionActionError::ExecutionFailure,
            "composition action returned an invalid execution status");
    } catch (const std::exception& error) {
        return fail(
            CompositionActionError::ExecutionFailure,
            std::string("composition action threw an exception: ") +
                error.what());
    } catch (...) {
        return fail(
            CompositionActionError::ExecutionFailure,
            "composition action threw an unknown exception");
    }
}

CompositionTelemetryResult Runtime::compositionElementTelemetry(
    std::size_t zeroBasedIndex) const {
    auto fail = [](
        CompositionTelemetryError code,
        std::string error) {
        CompositionTelemetryResult result;
        result.errorCode = code;
        result.error = std::move(error);
        return result;
    };

    if (zeroBasedIndex >= compositionLayers_.size()) {
        return fail(
            CompositionTelemetryError::IndexOutOfRange,
            "composition layer index is out of range");
    }
    const auto& layer = compositionLayers_[zeroBasedIndex];
    if (layer.assetId.empty()) {
        return fail(
            CompositionTelemetryError::SlotEmpty,
            "composition layer is empty");
    }
    if (layer.kind != CompositionKind::Element) {
        return fail(
            CompositionTelemetryError::KindMismatch,
            "composition telemetry requires an Element composition entry");
    }
    if (!layer.element_) {
        return fail(
            CompositionTelemetryError::SlotEmpty,
            "composition layer does not contain an element");
    }

    ElementTelemetryBuffer buffer;
    try {
        layer.element_->collectTelemetry(buffer);
    } catch (const std::exception& error) {
        return fail(
            CompositionTelemetryError::CollectionFailure,
            std::string("element telemetry collection threw an exception: ") +
                error.what());
    } catch (...) {
        return fail(
            CompositionTelemetryError::CollectionFailure,
            "element telemetry collection threw an unknown exception");
    }

    try {
        const auto contractError = buffer.contractError();
        if (!contractError.empty()) {
            return fail(
                CompositionTelemetryError::ContractViolation,
                contractError);
        }
        CompositionTelemetryResult result;
        result.entries = buffer.takeEntries();
        return result;
    } catch (const std::exception& error) {
        return fail(
            CompositionTelemetryError::CollectionFailure,
            std::string("element telemetry validation failed: ") +
                error.what());
    } catch (...) {
        return fail(
            CompositionTelemetryError::CollectionFailure,
            "element telemetry validation failed with an unknown exception");
    }
}

float Runtime::normalizeOpacity(float opacity) noexcept {
    return std::isfinite(opacity)
        ? std::clamp(opacity, 0.0f, 1.0f)
        : 1.0f;
}

CompositionCoverage Runtime::normalizeCoverage(
    CompositionCoverage coverage) {
    if (coverage.mode.empty()) {
        coverage.mode = "upstream";
    }
    coverage.columns = std::max(0, coverage.columns);
    return coverage;
}

void Runtime::forceClearCompositionLayerNoexcept(
    CompositionLayer& layer) noexcept {
    const bool hadElement = layer.element_ != nullptr;
    layer.actions_.clear();
    releaseElement(layer.element_);
    if (hadElement) {
        ++layer.elementRevision_;
    }
    if (!layer.opacityParameterId_.empty()) {
        parameters_.removeById(layer.opacityParameterId_);
    }
    layer.assetId.clear();
    layer.label.clear();
    layer.type.clear();
    layer.paramPrefix.clear();
    layer.opacityParameterId_.clear();
    layer.coverage.mode.clear();
    layer.kind = CompositionKind::Element;
    layer.active = false;
    layer.opacity = 1.0f;
    layer.coverage.defined = false;
    layer.coverage.columns = 0;
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

CompositionMutationResult Runtime::adoptPreparedElement(
    std::size_t zeroBasedIndex,
    ElementResult&& prepared,
    CompositionAssignment assignment) {
    return adoptPreparedElementImpl(
        zeroBasedIndex,
        std::move(prepared),
        assignment);
}

CompositionMutationResult Runtime::adoptPreparedElementImpl(
    std::size_t zeroBasedIndex,
    ElementResult&& prepared,
    CompositionAssignment& assignment) {
    auto fail = [](
        CompositionMutationError code,
        std::string error) {
        CompositionMutationResult result;
        result.errorCode = code;
        result.error = std::move(error);
        return result;
    };

    auto* layer = mutableCompositionLayer(zeroBasedIndex);
    if (!layer) {
        return fail(
            CompositionMutationError::IndexOutOfRange,
            "composition layer index is out of range");
    }
    if (!prepared.element_ ||
        !prepared.stagedParameters_ ||
        prepared.runtime_ != this ||
        prepared.runtimeLifetime_.expired()) {
        return fail(
            CompositionMutationError::ElementMismatch,
            "prepared element is not owned by this Runtime");
    }

    const std::string expectedPrefix =
        "console.layer" + std::to_string(zeroBasedIndex + 1);
    if (prepared.registryPrefix != expectedPrefix) {
        return fail(
            CompositionMutationError::ElementMismatch,
            "prepared element registry prefix does not match the composition layer");
    }
    if (assignment.kind != CompositionKind::Element) {
        return fail(
            CompositionMutationError::KindMismatch,
            "prepared elements require an Element composition assignment");
    }
    if (assignment.definitionId.empty() ||
        assignment.typeId.empty() ||
        assignment.registryPrefix.empty()) {
        return fail(
            CompositionMutationError::InvalidAssignment,
            "composition assignment identity is incomplete");
    }
    if (assignment.definitionId != prepared.definitionId ||
        assignment.typeId != prepared.typeId ||
        assignment.registryPrefix != prepared.registryPrefix ||
        assignment.active != prepared.enabled) {
        return fail(
            CompositionMutationError::ElementMismatch,
            "composition assignment does not match the prepared element");
    }
    assignment.opacity = normalizeOpacity(assignment.opacity);
    assignment.coverage = CompositionCoverage();

    Layer* const oldElement = layer->element_.get();
    if (prepared.replacementElement_ &&
        layer->elementRevision_ != prepared.replacementElementRevision_) {
        return fail(
            CompositionMutationError::ElementMismatch,
            "prepared replacement generation is stale");
    }
    if (oldElement != prepared.replacementElement_) {
        return fail(
            CompositionMutationError::ElementMismatch,
            "prepared replacement does not match the live composition element");
    }

    const auto oldOwnership =
        oldElement ? ownership_.find(oldElement) : ownership_.end();
    if (oldElement && oldOwnership == ownership_.end()) {
        return fail(
            CompositionMutationError::ElementMismatch,
            "live composition element has no Runtime ownership record");
    }
    if (!oldElement && prepared.replacementElement_) {
        return fail(
            CompositionMutationError::ElementMismatch,
            "prepared result unexpectedly targets a missing element");
    }

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

        ParameterRegistry::FloatParam* nextOpacityParam = nullptr;
        float stagedOpacity = 1.0f;
        stagedOpacity = assignment.opacity;
        std::string opacityId =
            assignment.registryPrefix + ".opacity";
        ParameterRegistry::Descriptor opacityDescriptor;
        opacityDescriptor.label = "Visibility: Layer Opacity";
        opacityDescriptor.group = "Visibility";
        opacityDescriptor.description =
            "Base opacity for this layer before FX or modifiers are applied";
        opacityDescriptor.range.min = 0.0f;
        opacityDescriptor.range.max = 1.0f;
        opacityDescriptor.range.step = 0.01f;
        opacityDescriptor.quickAccess = true;
        opacityDescriptor.quickAccessOrder = 0;

        nextOpacityParam = nextParameters.findFloat(opacityId);
        if (!nextOpacityParam) {
            nextOpacityParam = &nextParameters.addFloat(
                opacityId,
                &stagedOpacity,
                stagedOpacity,
                opacityDescriptor);
        } else {
            nextOpacityParam->meta = opacityDescriptor;
            nextOpacityParam->meta.id = opacityId;
            nextOpacityParam->defaultValue = stagedOpacity;
            nextOpacityParam->baseValue = stagedOpacity;
        }

        // All potentially throwing staging is complete. From here through the
        // ownership/element swaps, commit only pointer/scalar writes and swaps.
        if (nextOpacityParam) {
            nextOpacityParam->value = &layer->opacity;
            layer->opacity = stagedOpacity;
        }
        parameters_.swap(nextParameters);
        candidate->onParameterRegistryCommitted(parameters_);
        ownership_.swap(nextOwnership);
        activePrefixes_.swap(nextActivePrefixes);
        layer->actions_.swap(prepared.stagedActions_);
        layer->element_.swap(prepared.element_);
        ++layer->elementRevision_;
        layer->assetId.swap(assignment.definitionId);
        layer->label.swap(assignment.label);
        layer->type.swap(assignment.typeId);
        layer->paramPrefix.swap(assignment.registryPrefix);
        layer->opacityParameterId_.swap(opacityId);
        layer->kind = CompositionKind::Element;
        layer->active = assignment.active;
        layer->coverage.defined = false;
        layer->coverage.mode.swap(assignment.coverage.mode);
        layer->coverage.columns = 0;

        prepared.stagedParameters_.reset();
        prepared.replacementElement_ = nullptr;
        prepared.replacementElementRevision_ = 0;
        prepared.ownsPrefixReservation_ = false;
        prepared.runtime_ = nullptr;
        prepared.runtimeLifetime_.reset();
        CompositionMutationResult result;
        result.elementChanged = true;
        result.parametersChanged = true;
        return result;
    } catch (...) {
        return fail(
            CompositionMutationError::LifecycleFailure,
            "failed to stage or commit the composition element");
    }
}

CompositionMutationResult Runtime::assignCompositionEntry(
    std::size_t zeroBasedIndex,
    CompositionAssignment assignment) {
    auto* layer = mutableCompositionLayer(zeroBasedIndex);
    if (!layer) {
        return {
            CompositionMutationError::IndexOutOfRange,
            false,
            false,
            "composition layer index is out of range",
        };
    }
    if (assignment.kind == CompositionKind::Element) {
        return {
            CompositionMutationError::KindMismatch,
            false,
            false,
            "element assignments require a prepared element",
        };
    }
    if (layer->element_) {
        return {
            CompositionMutationError::ElementMismatch,
            false,
            false,
            "non-element assignment cannot replace a live element",
        };
    }
    if (assignment.definitionId.empty() ||
        assignment.typeId.empty() ||
        assignment.registryPrefix.empty()) {
        return {
            CompositionMutationError::InvalidAssignment,
            false,
            false,
            "composition assignment identity is incomplete",
        };
    }

    try {
        assignment.opacity = normalizeOpacity(assignment.opacity);
        assignment.coverage =
            assignment.kind == CompositionKind::Effect
            ? normalizeCoverage(std::move(assignment.coverage))
            : CompositionCoverage();

        layer->assetId.swap(assignment.definitionId);
        layer->label.swap(assignment.label);
        layer->type.swap(assignment.typeId);
        layer->paramPrefix.swap(assignment.registryPrefix);
        layer->opacityParameterId_.clear();
        layer->coverage.mode.swap(assignment.coverage.mode);
        layer->kind = assignment.kind;
        layer->active = assignment.active;
        layer->opacity = assignment.opacity;
        layer->coverage.defined = assignment.coverage.defined;
        layer->coverage.columns = assignment.coverage.columns;
        return {};
    } catch (...) {
        return {
            CompositionMutationError::LifecycleFailure,
            false,
            false,
            "failed to assign the composition entry",
        };
    }
}

CompositionMutationResult Runtime::setCompositionLayerActive(
    std::size_t zeroBasedIndex,
    bool active) {
    auto* layer = mutableCompositionLayer(zeroBasedIndex);
    if (!layer) {
        return {
            CompositionMutationError::IndexOutOfRange,
            false,
            false,
            "composition layer index is out of range",
        };
    }
    if (layer->assetId.empty()) {
        return {
            CompositionMutationError::InvalidAssignment,
            false,
            false,
            "empty composition layer has no active state",
        };
    }
    try {
        if (layer->element_) {
            layer->element_->setExternalEnabled(active);
        }
        layer->active = active;
        return {};
    } catch (...) {
        return {
            CompositionMutationError::LifecycleFailure,
            false,
            false,
            "element rejected the active-state change",
        };
    }
}

CompositionMutationResult Runtime::setCompositionLayerLabel(
    std::size_t zeroBasedIndex,
    std::string label) {
    auto* layer = mutableCompositionLayer(zeroBasedIndex);
    if (!layer) {
        return {
            CompositionMutationError::IndexOutOfRange,
            false,
            false,
            "composition layer index is out of range",
        };
    }
    if (layer->assetId.empty()) {
        return {
            CompositionMutationError::InvalidAssignment,
            false,
            false,
            "empty composition layer has no label",
        };
    }
    layer->label.swap(label);
    return {};
}

CompositionMutationResult Runtime::setCompositionLayerCoverage(
    std::size_t zeroBasedIndex,
    CompositionCoverage coverage) {
    auto* layer = mutableCompositionLayer(zeroBasedIndex);
    if (!layer) {
        return {
            CompositionMutationError::IndexOutOfRange,
            false,
            false,
            "composition layer index is out of range",
        };
    }
    if (layer->kind != CompositionKind::Effect) {
        return {
            CompositionMutationError::KindMismatch,
            false,
            false,
            "coverage is valid only for Effect composition entries",
        };
    }
    try {
        coverage = normalizeCoverage(std::move(coverage));
        layer->coverage.mode.swap(coverage.mode);
        layer->coverage.defined = coverage.defined;
        layer->coverage.columns = coverage.columns;
        return {};
    } catch (...) {
        return {
            CompositionMutationError::LifecycleFailure,
            false,
            false,
            "failed to update composition coverage",
        };
    }
}

CompositionMutationResult Runtime::clearCompositionLayer(
    std::size_t zeroBasedIndex) {
    auto* layer = mutableCompositionLayer(zeroBasedIndex);
    if (!layer) {
        return {
            CompositionMutationError::IndexOutOfRange,
            false,
            false,
            "composition layer index is out of range",
        };
    }

    Layer* const oldElement = layer->element_.get();
    const auto oldOwnership =
        oldElement ? ownership_.find(oldElement) : ownership_.end();
    if (oldElement && oldOwnership == ownership_.end()) {
        return {
            CompositionMutationError::ElementMismatch,
            false,
            false,
            "live composition element has no Runtime ownership record",
        };
    }

    try {
        std::vector<std::string> removedIds;
        if (oldOwnership != ownership_.end()) {
            removedIds.reserve(oldOwnership->second.parameters.size() + 1);
            for (const auto& parameter : oldOwnership->second.parameters) {
                removedIds.push_back(parameter.id);
            }
        }
        if (!layer->opacityParameterId_.empty() &&
            parameters_.findFloat(layer->opacityParameterId_)) {
            removedIds.push_back(layer->opacityParameterId_);
        }

        ParameterRegistry nextParameters;
        if (!removedIds.empty()) {
            ParameterRegistry noAdditions;
            nextParameters =
                parameters_.replacingIds(removedIds, noAdditions);
        }
        auto nextOwnership = ownership_;
        if (oldElement) nextOwnership.erase(oldElement);
        auto nextActivePrefixes = activePrefixes_;
        if (oldOwnership != ownership_.end()) {
            nextActivePrefixes.erase(oldOwnership->second.prefix);
        }
        CompositionCoverage emptyCoverage;

        if (!removedIds.empty()) {
            parameters_.swap(nextParameters);
        }
        ownership_.swap(nextOwnership);
        activePrefixes_.swap(nextActivePrefixes);
        std::unique_ptr<Layer> retiredElement;
        ElementActionTable retiredActions;
        retiredActions.swap(layer->actions_);
        retiredElement.swap(layer->element_);
        if (oldElement) {
            ++layer->elementRevision_;
        }
        layer->assetId.clear();
        layer->label.clear();
        layer->type.clear();
        layer->paramPrefix.clear();
        layer->opacityParameterId_.clear();
        layer->coverage.mode.swap(emptyCoverage.mode);
        layer->kind = CompositionKind::Element;
        layer->active = false;
        layer->opacity = 1.0f;
        layer->coverage.defined = false;
        layer->coverage.columns = 0;

        CompositionMutationResult result;
        result.elementChanged = oldElement != nullptr;
        result.parametersChanged = !removedIds.empty();
        return result;
    } catch (...) {
        return {
            CompositionMutationError::LifecycleFailure,
            false,
            false,
            "failed to clear the composition layer",
        };
    }
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
    auto* layer = mutableCompositionLayer(zeroBasedIndex);
    if (!layer || !layer->active || !layer->element_) return;
    layer->element_->draw(params);
}

void Runtime::shutdownComposition() {
    for (std::size_t i = 0; i < compositionLayers_.size(); ++i) {
        const auto cleared = clearCompositionLayer(i);
        auto& layer = compositionLayers_[i];
        if (!cleared) {
            // Shutdown must not retain element/parameter ownership merely
            // because the transactional clear could not allocate its staging
            // registry.
            forceClearCompositionLayerNoexcept(layer);
        }
    }
}

} // namespace synaptome::runtime

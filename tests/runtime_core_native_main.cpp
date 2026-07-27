#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../synaptome/src/runtime/Runtime.h"
#include "../synaptome/src/visuals/LayerFactory.h"

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void require(
    const synaptome::runtime::CompositionMutationResult& result,
    const std::string& message) {
    if (!result) {
        throw std::runtime_error(
            message + (result.error.empty() ? "" : ": " + result.error));
    }
}

synaptome::runtime::CompositionAssignment assignmentFor(
    const synaptome::runtime::Runtime::ElementRequest& request,
    const std::string& label = "RuntimeCore Element",
    float opacity = 1.0f) {
    synaptome::runtime::CompositionAssignment assignment;
    assignment.kind = synaptome::runtime::CompositionKind::Element;
    assignment.definitionId = request.definitionId;
    assignment.label = label;
    assignment.typeId = request.typeId;
    assignment.registryPrefix = request.registryPrefix;
    assignment.active = request.enabled;
    assignment.opacity = opacity;
    return assignment;
}

class ContractElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Contract Value";
        registry.addFloat(registryPrefix() + ".value", &value_, 0.25f, descriptor);
        descriptor.label = "Contract Gate";
        registry.addBool(registryPrefix() + ".gate", &gate_, true, descriptor);
    }
    void update(const LayerUpdateParams&) override { ++updateCount; }
    void draw(const LayerDrawParams&) override { ++drawCount; }
    void onWindowResized(int width, int height) override {
        ++resizeCount;
        lastWidth = width;
        lastHeight = height;
    }
    void setExternalEnabled(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

    int updateCount = 0;
    int drawCount = 0;
    int resizeCount = 0;
    int lastWidth = 0;
    int lastHeight = 0;

private:
    float value_ = 0.25f;
    bool gate_ = true;
    bool enabled_ = true;
};

class FailingElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Partial Value";
        registry.addFloat("outside.partial", &value_, 0.5f, descriptor);
        throw std::runtime_error("intentional setup failure");
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.5f;
};

class EmptyElement final : public Layer {
public:
    void setup(ParameterRegistry&) override {}
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}
};

class ForeignParameterElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Foreign Value";
        registry.addFloat("foreign.value", &value_, 0.75f, descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.75f;
};

class DestructiveFailingElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        registry.removeById("host.live");
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Partial Value";
        registry.addFloat(
            registryPrefix() + ".partial",
            &value_,
            0.5f,
            descriptor);
        throw std::runtime_error("destructive setup failure");
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.5f;
};

class RegistryAwareElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        setupRegistry = &registry;
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Registry Aware Value";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            0.4f,
            descriptor);
    }
    void onParameterRegistryCommitted(
        ParameterRegistry& registry) noexcept override {
        committedRegistry = &registry;
        ++commitCount;
    }
    void update(const LayerUpdateParams&) override {
        sawLiveRegistry =
            committedRegistry &&
            committedRegistry != setupRegistry &&
            committedRegistry->findFloat(
                registryPrefix() + ".value") != nullptr;
    }
    void draw(const LayerDrawParams&) override {}

    ParameterRegistry* setupRegistry = nullptr;
    ParameterRegistry* committedRegistry = nullptr;
    int commitCount = 0;
    bool sawLiveRegistry = false;

private:
    float value_ = 0.4f;
};

class ReservedOpacityElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Element-owned Opacity";
        registry.addFloat(
            registryPrefix() + ".opacity",
            &opacity_,
            0.2f,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float opacity_ = 0.2f;
};

class HostCollisionElement final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Host Collision";
        registry.addFloat(
            registryPrefix() + ".hostOwned",
            &value_,
            0.2f,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.2f;
};

bool retainedRegistryDestructorRan = false;
bool retainedRegistryWasAlive = false;

class RetainedSetupRegistryElement final : public Layer {
public:
    ~RetainedSetupRegistryElement() override {
        retainedRegistryDestructorRan = true;
        retainedRegistryWasAlive =
            setupRegistry_ &&
            setupRegistry_->findFloat(registryPrefix() + ".retained") != nullptr;
    }

    void setup(ParameterRegistry& registry) override {
        setupRegistry_ = &registry;
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Retained Setup Registry";
        registry.addFloat(
            registryPrefix() + ".retained",
            &value_,
            0.3f,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    ParameterRegistry* setupRegistry_ = nullptr;
    float value_ = 0.3f;
};

class ScopedRegistryElementA final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Scoped Registry A";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            value_,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.1f;
};

class ScopedRegistryElementB final : public Layer {
public:
    void setup(ParameterRegistry& registry) override {
        ParameterRegistry::Descriptor descriptor;
        descriptor.label = "Scoped Registry B";
        registry.addFloat(
            registryPrefix() + ".value",
            &value_,
            value_,
            descriptor);
    }
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}

private:
    float value_ = 0.9f;
};

void RunScopedElementTypeRegistryIsolationScenario() {
    // Runtime receives its type registry by reference, so independently
    // constructed registries must never share or fall back to global state.
    LayerFactory registryA;
    LayerFactory registryB;
    constexpr const char* kSharedType = "tests.runtime.scoped.shared";
    constexpr const char* kOnlyInAType = "tests.runtime.scoped.only-a";

    registryA.registerType(kSharedType, [] {
        return std::make_unique<ScopedRegistryElementA>();
    });
    registryB.registerType(kSharedType, [] {
        return std::make_unique<ScopedRegistryElementB>();
    });
    registryA.registerType(kOnlyInAType, [] {
        return std::make_unique<ScopedRegistryElementA>();
    });
    int lookupConstructionCount = 0;
    registryA.registerType("tests.runtime.scoped.lookup-only", [&] {
        ++lookupConstructionCount;
        return std::make_unique<ScopedRegistryElementA>();
    });

    ParameterRegistry parametersA;
    ParameterRegistry parametersB;
    synaptome::runtime::Runtime runtimeA(registryA, parametersA);
    synaptome::runtime::Runtime runtimeB(registryB, parametersB);
    require(
        runtimeA.hasElementType("tests.runtime.scoped.lookup-only") &&
            !runtimeB.hasElementType("tests.runtime.scoped.lookup-only") &&
            lookupConstructionCount == 0,
        "scoped type lookup constructed an element or leaked across runtimes");

    synaptome::runtime::Runtime::ElementRequest request;
    request.typeId = kSharedType;
    request.definitionId = "tests.definition.scoped.shared";
    request.instanceId = "tests.instance.scoped.a";
    request.registryPrefix = "console.layer1";
    auto preparedA = runtimeA.prepareElement(request);
    require(
        preparedA &&
            dynamic_cast<ScopedRegistryElementA*>(preparedA.element()) !=
                nullptr,
        "runtime A did not resolve its scoped shared type");

    request.instanceId = "tests.instance.scoped.b";
    auto preparedB = runtimeB.prepareElement(request);
    require(
        preparedB &&
            dynamic_cast<ScopedRegistryElementB*>(preparedB.element()) !=
                nullptr,
        "runtime B leaked runtime A's shared type registration");

    request.typeId = kOnlyInAType;
    request.definitionId = "tests.definition.scoped.only-a";
    request.instanceId = "tests.instance.scoped.only-a";
    request.registryPrefix = "console.layer2";
    auto onlyInA = runtimeA.prepareElement(request);
    require(onlyInA &&
                dynamic_cast<ScopedRegistryElementA*>(onlyInA.element()) !=
                    nullptr,
            "runtime A could not resolve its private type registration");

    request.instanceId = "tests.instance.scoped.missing-in-b";
    auto missingInB = runtimeB.prepareElement(request);
    require(
        !missingInB &&
            missingInB.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::
                    TypeNotRegistered,
        "runtime B observed a type registered only in runtime A's scope");
}

void RunEffectCoverageWindowScenario(
    const synaptome::runtime::Runtime& runtime) {
    auto expectWindow = [&](std::size_t effectLayerIndex,
                            float coverage,
                            std::size_t expectedFirst,
                            std::size_t expectedEnd,
                            int expectedRequested,
                            bool expectedAll) {
        const auto window =
            runtime.resolveEffectCoverage(effectLayerIndex, coverage);
        require(
            window.effectLayerIndex == effectLayerIndex &&
                window.firstInputLayerIndex == expectedFirst &&
                window.inputEndLayerIndex == expectedEnd &&
                window.requestedLayers == expectedRequested &&
                window.includesAllPrior == expectedAll,
            "runtime effect coverage window did not match the requested range");
        return window;
    };

    expectWindow(3, 0.0f, 0, 3, 0, true);
    expectWindow(3, 1.0f, 2, 3, 1, false);
    const auto nearestTwo =
        expectWindow(4, 2.0f, 2, 4, 2, false);
    expectWindow(4, 10.0f, 0, 4, 10, true);
    expectWindow(4, 2.9f, 2, 4, 2, false);
    expectWindow(0, 2.0f, 0, 0, 2, true);
    expectWindow(2, -1.0f, 0, 2, 0, true);

    require(
        nearestTwo.contains(2) &&
            nearestTwo.contains(3) &&
            !nearestTwo.contains(1) &&
            !nearestTwo.contains(4),
        "runtime effect coverage window boundaries were not half-open");

    const auto invalid = runtime.resolveEffectCoverage(
        synaptome::runtime::kCompositionLayerCount,
        2.0f);
    require(
        invalid.firstInputLayerIndex == 0 &&
            invalid.inputEndLayerIndex == 0 &&
            invalid.requestedLayers == 0 &&
            !invalid.includesAllPrior &&
            !invalid.contains(0),
        "runtime effect coverage accepted an out-of-range effect layer");
}
}

int main() {
    try {
        RunScopedElementTypeRegistryIsolationScenario();

        LayerFactory factory;
        factory.registerType("tests.runtime.good", [] {
            return std::make_unique<ContractElement>();
        });
        factory.registerType("tests.runtime.failing", [] {
            return std::make_unique<FailingElement>();
        });
        factory.registerType("tests.runtime.empty", [] {
            return std::make_unique<EmptyElement>();
        });
        factory.registerType("tests.runtime.foreign", [] {
            return std::make_unique<ForeignParameterElement>();
        });
        factory.registerType("tests.runtime.destructive-failing", [] {
            return std::make_unique<DestructiveFailingElement>();
        });
        factory.registerType("tests.runtime.registry-aware", [] {
            return std::make_unique<RegistryAwareElement>();
        });
        factory.registerType("tests.runtime.reserved-opacity", [] {
            return std::make_unique<ReservedOpacityElement>();
        });
        factory.registerType("tests.runtime.host-collision", [] {
            return std::make_unique<HostCollisionElement>();
        });
        factory.registerType("tests.runtime.retained-registry", [] {
            return std::make_unique<RetainedSetupRegistryElement>();
        });

        ParameterRegistry parameters;
        float siblingFloat = 0.1f;
        bool siblingBool = true;
        std::string siblingString = "live";
        ParameterRegistry::Descriptor siblingDescriptor;
        siblingDescriptor.label = "Sibling";
        parameters.addFloat(
            "console.layer10.float",
            &siblingFloat,
            siblingFloat,
            siblingDescriptor);
        parameters.addBool(
            "console.layer10.bool",
            &siblingBool,
            siblingBool,
            siblingDescriptor);
        parameters.addString(
            "console.layer10.string",
            &siblingString,
            siblingString,
            siblingDescriptor);

        synaptome::runtime::Runtime runtime(factory, parameters);
        const auto& compositionLayers =
            runtime.compositionLayersForHost();
        RunEffectCoverageWindowScenario(runtime);
        synaptome::runtime::Runtime::ElementRequest request;
        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.good";
        request.instanceId = "tests.instance.good";
        request.registryPrefix = "console.layer1";
        request.enabled = false;

        std::vector<std::string> progress;
        auto prepared = runtime.prepareElement(
            request,
            [&](std::string_view step) { progress.emplace_back(step); });
        require(static_cast<bool>(prepared), prepared.error);
        require(prepared.element()->instanceId() == request.instanceId,
                "instance identity was not assigned");
        require(prepared.element()->registryPrefix() == request.registryPrefix,
                "registry prefix was not assigned");
        require(!prepared.element()->isEnabled(), "requested enable state was not applied");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "candidate setup mutated the live parameter registry");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(prepared),
                assignmentFor(request, "Good Element", 0.8f)),
            "runtime did not commit the prepared element");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "adoption did not commit staged parameters");
        require(parameters.findBool("console.layer1.gate") != nullptr,
                "adoption did not commit staged bool parameters");
        require(parameters.findFloat("console.layer1.opacity") != nullptr,
                "adoption did not register spine-owned opacity");
        require(
            progress == std::vector<std::string>({"create", "configure", "setup", "enable"}),
            "lifecycle progress order drifted");

        auto collision = runtime.prepareElement(request);
        require(!collision, "occupied parameter prefix was accepted");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "prefix collision damaged the live registration");

        const auto firstClear = runtime.clearCompositionLayer(0);
        require(
            firstClear &&
                !runtime.compositionLayersForHost()[0].hasElement(),
            "clear did not destroy the element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "clear did not remove instance parameters");
        require(parameters.findBool("console.layer1.gate") == nullptr,
                "clear did not remove instance bool parameters");
        require(parameters.findFloat("console.layer1.opacity") == nullptr,
                "clear retained spine-owned opacity");
        request.definitionId = "tests.definition.reloaded";
        request.instanceId = "tests.instance.reloaded";
        auto reloaded = runtime.prepareElement(request);
        require(static_cast<bool>(reloaded),
                "slot prefix could not be reused after Runtime clear");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(reloaded),
                assignmentFor(request)),
                "reloaded element was not adopted");
        require(runtime.clearCompositionLayer(0),
                "reloaded element was not cleared");
        require(parameters.findFloat("console.layer10.float") != nullptr,
                "release removed a sibling float namespace");
        require(parameters.findBool("console.layer10.bool") != nullptr,
                "release removed a sibling bool namespace");
        require(parameters.findString("console.layer10.string") != nullptr,
                "release removed a sibling string namespace");

        request.typeId = "tests.runtime.failing";
        request.definitionId = "tests.definition.failing";
        request.instanceId = "tests.instance.failing";
        request.registryPrefix = "console.layer2";
        auto failed = runtime.prepareElement(request);
        require(!failed, "throwing setup was reported as successful");
        require(
            failed.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::LifecycleFailure,
            "setup failure did not preserve its structured error code");
        require(failed.stage == "setup",
                "setup failure did not preserve its lifecycle stage");
        require(failed.error == "intentional setup failure",
                "structured setup failure was not preserved");
        require(parameters.findFloat("outside.partial") == nullptr,
                "failed setup leaked an out-of-namespace registration");

        request.typeId = "tests.runtime.foreign";
        request.definitionId = "tests.definition.foreign";
        request.instanceId = "tests.instance.foreign";
        request.registryPrefix = "console.layer2";
        auto foreign = runtime.prepareElement(request);
        require(!foreign, "out-of-namespace registration was accepted");
        require(
            foreign.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::ContractViolation,
            "out-of-namespace registration did not report a contract violation");
        require(parameters.findFloat("foreign.value") == nullptr,
                "contract violation leaked its foreign registration");

        request.typeId = "tests.runtime.reserved-opacity";
        request.definitionId = "tests.definition.reserved-opacity";
        request.instanceId = "tests.instance.reserved-opacity";
        request.registryPrefix = "console.layer2";
        auto reservedOpacity = runtime.prepareElement(request);
        require(!reservedOpacity,
                "element claimed the layer-container opacity parameter");
        require(
            reservedOpacity.errorCode ==
                synaptome::runtime::Runtime::ElementErrorCode::ContractViolation,
            "reserved opacity did not report a contract violation");
        require(
            reservedOpacity.error.find("layer-container reserved") !=
                std::string::npos,
            "reserved opacity did not explain the ownership violation");
        require(parameters.findFloat("console.layer2.opacity") == nullptr,
                "reserved opacity leaked into the live registry");

        retainedRegistryDestructorRan = false;
        retainedRegistryWasAlive = false;
        request.typeId = "tests.runtime.retained-registry";
        request.definitionId = "tests.definition.retained-registry";
        request.instanceId = "tests.instance.retained-registry";
        request.registryPrefix = "console.layer2";
        {
            auto retained = runtime.prepareElement(request);
            require(static_cast<bool>(retained), retained.error);
        }
        require(
            retainedRegistryDestructorRan && retainedRegistryWasAlive,
            "prepared element was not destroyed before its staging registry");

        retainedRegistryDestructorRan = false;
        retainedRegistryWasAlive = false;
        synaptome::runtime::Runtime::ElementResult outlivesRuntime;
        {
            ParameterRegistry temporaryParameters;
            synaptome::runtime::Runtime temporaryRuntime(
                factory,
                temporaryParameters);
            auto retained = temporaryRuntime.prepareElement(request);
            require(static_cast<bool>(retained), retained.error);
            outlivesRuntime = std::move(retained);
        }
        outlivesRuntime =
            synaptome::runtime::Runtime::ElementResult{};
        require(
            retainedRegistryDestructorRan && retainedRegistryWasAlive,
            "expired Runtime destroyed staging before its prepared element");

        request.typeId = "tests.runtime.empty";
        request.definitionId = "tests.definition.empty";
        request.instanceId = "tests.instance.empty";
        request.registryPrefix = "console.layer3";
        auto empty = runtime.prepareElement(request);
        require(static_cast<bool>(empty), "zero-parameter element failed to prepare");
        auto emptyCollision = runtime.prepareElement(request);
        require(!emptyCollision, "zero-parameter element did not reserve its prefix");
        runtime.releasePreparedElement(empty);

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.abandoned";
        request.instanceId = "tests.instance.abandoned";
        request.registryPrefix = "console.layer4";
        {
            auto abandoned = runtime.prepareElement(request);
            require(static_cast<bool>(abandoned), "abandoned result did not prepare");
            require(parameters.findFloat("console.layer4.value") == nullptr,
                    "abandoned candidate mutated live parameters");
        }
        require(parameters.findFloat("console.layer4.value") == nullptr,
                "abandoned result did not release owned parameters");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.composition";
        request.instanceId = "tests.instance.composition";
        request.registryPrefix = "console.layer5";
        {
            auto mismatched = runtime.prepareElement(request);
            require(static_cast<bool>(mismatched),
                    "mismatched composition candidate did not prepare");
            require(
                !runtime.adoptPreparedElement(
                    0,
                    std::move(mismatched),
                    assignmentFor(request)),
                "runtime accepted a candidate for the wrong composition address");
        }
        require(parameters.findFloat("console.layer5.value") == nullptr,
                "rejected composition candidate leaked parameters");

        ParameterRegistry foreignParameters;
        synaptome::runtime::Runtime foreignRuntime(factory, foreignParameters);
        request.definitionId = "tests.definition.foreign-runtime";
        request.instanceId = "tests.instance.foreign-runtime";
        request.registryPrefix = "console.layer1";
        {
            auto foreignRuntimeCandidate = foreignRuntime.prepareElement(request);
            require(static_cast<bool>(foreignRuntimeCandidate),
                    "foreign-runtime composition candidate did not prepare");
            require(
                !runtime.adoptPreparedElement(
                    0,
                    std::move(foreignRuntimeCandidate),
                    assignmentFor(request)),
                "runtime adopted an element owned by another runtime");
            require(
                foreignParameters.findFloat("console.layer1.value") == nullptr,
                "cross-runtime candidate escaped its staging registry");
        }
        require(
            foreignParameters.findFloat("console.layer1.value") == nullptr,
            "rejected cross-runtime candidate leaked source parameters");

        request.definitionId = "tests.definition.composition";
        request.instanceId = "tests.instance.composition";
        request.registryPrefix = "console.layer1";
        auto compositionPrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(compositionPrepared),
                assignmentFor(request, "Composition Element", 0.72f)),
            "runtime did not adopt the prepared composition element");
        const auto* compositionLayer = &compositionLayers[0];
        require(compositionLayer && compositionLayer->hasElement(),
                "runtime did not retain composition element ownership");
        require(runtime.setCompositionLayerActive(0, true),
                "runtime did not activate the composition element");
        auto* compositionElement =
            dynamic_cast<ContractElement*>(
                runtime.legacyCompositionElementForHost(0));
        require(compositionElement != nullptr,
                "composition element type was not preserved");

        float liveHostValue = 0.65f;
        ParameterRegistry::Descriptor liveHostDescriptor;
        liveHostDescriptor.label = "Live Host Value";
        parameters.addFloat(
            "host.live",
            &liveHostValue,
            liveHostValue,
            liveHostDescriptor);
        request.typeId = "tests.runtime.destructive-failing";
        request.definitionId = "tests.definition.failed-replacement";
        request.instanceId = "tests.instance.failed-replacement";
        auto failedReplacement = runtime.prepareElementReplacement(
            request,
            *runtime.legacyCompositionElementForHost(0));
        require(!failedReplacement,
                "throwing replacement was reported as prepared");
        require(compositionLayer->element() == compositionElement,
                "failed replacement destroyed the live element");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "failed replacement removed the live element parameter");
        require(parameters.findFloat("console.layer1.partial") == nullptr,
                "failed replacement leaked a staged parameter");
        require(parameters.findFloat("host.live") != nullptr &&
                    liveHostValue == 0.65f,
                "failed replacement mutated a live host registration");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.abandoned-replacement";
        request.instanceId = "tests.instance.abandoned-replacement";
        {
            auto abandonedReplacement = runtime.prepareElementReplacement(
                request,
                *runtime.legacyCompositionElementForHost(0));
            require(static_cast<bool>(abandonedReplacement),
                    abandonedReplacement.error);
        }
        require(
            compositionLayer->element() == compositionElement &&
                parameters.findFloat("console.layer1.value") != nullptr,
            "abandoned replacement disturbed the live slot");

        modifier::Modifier stableModifier;
        auto& stableRuntimeModifier = parameters.addFloatModifier(
            "console.layer1.value",
            stableModifier);
        stableRuntimeModifier.ownerTag = "tests.stable-slot-mapping";
        auto& stableBoolRuntimeModifier = parameters.addBoolModifier(
            "console.layer1.gate",
            stableModifier);
        stableBoolRuntimeModifier.ownerTag = "tests.stable-bool-mapping";

        ParameterRegistry::Descriptor replacementHostDescriptor;
        replacementHostDescriptor.label = "Host Opacity";
        const auto* replacementOpacityParam =
            parameters.findFloat("console.layer1.opacity");
        require(
            replacementOpacityParam &&
                replacementOpacityParam->value &&
                std::fabs(*replacementOpacityParam->value - 0.72f) < 0.0001f,
            "Runtime-owned replacement opacity was not available");
        float hostOwnedValue = 0.91f;
        parameters.addFloat(
            "console.layer1.hostOwned",
            &hostOwnedValue,
            hostOwnedValue,
            replacementHostDescriptor);
        request.typeId = "tests.runtime.host-collision";
        request.definitionId = "tests.definition.conflicting";
        request.instanceId = "tests.instance.conflicting";
        {
            auto conflictingReplacement = runtime.prepareElementReplacement(
                request,
                *runtime.legacyCompositionElementForHost(0));
            require(static_cast<bool>(conflictingReplacement),
                    conflictingReplacement.error);
            require(
                !runtime.adoptPreparedElement(
                    0,
                    std::move(conflictingReplacement),
                    assignmentFor(request, "Conflicting Element", 0.2f)),
                "replacement overwrote a host-owned parameter");
            require(compositionLayer->element() == compositionElement,
                    "failed commit replaced the live element");
            require(
                    parameters.findFloat("console.layer1.value") != nullptr &&
                    parameters.findFloat("console.layer1.opacity") != nullptr &&
                    parameters.findFloat("console.layer1.hostOwned") != nullptr &&
                    std::fabs(
                        *parameters.findFloat("console.layer1.opacity")->value -
                        0.72f) < 0.0001f &&
                    hostOwnedValue == 0.91f,
                "failed commit mutated the live registry");
        }
        parameters.removeById("console.layer1.hostOwned");
        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.replacement";
        request.instanceId = "tests.instance.replacement";
        request.enabled = true;
        auto replacement = runtime.prepareElementReplacement(
            request,
            *runtime.legacyCompositionElementForHost(0));
        require(static_cast<bool>(replacement), replacement.error);
        require(compositionLayer->element() == compositionElement,
                "preparation replaced the live element before commit");
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(replacement),
                assignmentFor(request, "Replacement Element", 0.72f)),
                "same-address replacement did not commit");
        auto* replacementElement =
            dynamic_cast<ContractElement*>(
                runtime.legacyCompositionElementForHost(0));
        require(replacementElement != nullptr &&
                    replacementElement != compositionElement,
                "replacement did not swap the live element");
        compositionElement = replacementElement;
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "replacement did not install candidate parameters");
        const auto* replacementModifiers =
            parameters.floatModifiers("console.layer1.value");
        require(
            replacementModifiers &&
                replacementModifiers->size() == 1 &&
                replacementModifiers->front().ownerTag ==
                    "tests.stable-slot-mapping",
            "replacement did not preserve matching slot modifiers");
        const auto* replacementBoolModifiers =
            parameters.boolModifiers("console.layer1.gate");
        require(
            replacementBoolModifiers &&
                replacementBoolModifiers->size() == 1 &&
                replacementBoolModifiers->front().ownerTag ==
                    "tests.stable-bool-mapping",
            "replacement did not preserve matching bool modifiers");
        const auto* committedReplacementOpacity =
            parameters.findFloat("console.layer1.opacity");
        require(
            committedReplacementOpacity &&
                committedReplacementOpacity->value &&
                std::fabs(*committedReplacementOpacity->value - 0.72f) <
                    0.0001f,
            "replacement lost spine-owned same-address opacity");

        runtime.updateCompositionElements(LayerUpdateParams{});
        runtime.resizeCompositionElements(1280, 720);
        ofCamera camera;
        runtime.drawCompositionElement(
            0,
            LayerDrawParams{camera, {1280, 720}, 0.0f, 0.0f, 1.0f});
        require(compositionElement->updateCount == 1,
                "runtime did not route composition update");
        require(compositionElement->drawCount == 1,
                "runtime did not route composition draw");
        require(
            compositionElement->resizeCount == 1 &&
                compositionElement->lastWidth == 1280 &&
                compositionElement->lastHeight == 720,
            "runtime did not route composition resize");
        const auto compositionClear = runtime.clearCompositionLayer(0);
        require(
            compositionClear &&
                !compositionLayer->hasElement(),
            "runtime did not clear its composition element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "composition clear leaked element parameters");
        require(parameters.findFloat("console.layer1.opacity") == nullptr,
                "composition clear retained spine-owned opacity");
        parameters.removeById("host.live");

        request.typeId = "tests.runtime.registry-aware";
        request.definitionId = "tests.definition.registry-aware";
        request.instanceId = "tests.instance.registry-aware";
        request.registryPrefix = "console.layer2";
        auto registryAwarePrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(
                1,
                std::move(registryAwarePrepared),
                assignmentFor(request, "Registry Aware")),
            "runtime did not adopt the registry-aware element");
        auto* registryAware = dynamic_cast<RegistryAwareElement*>(
            runtime.legacyCompositionElementForHost(1));
        require(
            registryAware &&
                registryAware->commitCount == 1 &&
                registryAware->committedRegistry == &parameters &&
                registryAware->setupRegistry != &parameters,
            "runtime did not rebind the staged registry after commit");
        require(runtime.setCompositionLayerActive(1, true),
                "runtime did not activate the registry-aware element");
        runtime.updateCompositionElements(LayerUpdateParams{});
        require(registryAware->sawLiveRegistry,
                "registry-aware update did not observe the live registry");
        require(runtime.clearCompositionLayer(1),
                "runtime did not clear the registry-aware element");

        require(runtime.compositionLayerCount() == 8,
                "runtime composition capacity drifted");
        require(
            static_cast<bool>(runtime.compositionRenderTargetsForHost(2)) &&
                !runtime.compositionRenderTargetsForHost(99),
            "runtime render-target access did not enforce composition bounds");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.control-plane-a";
        request.instanceId = "tests.instance.control-plane-a";
        request.registryPrefix = "console.layer3";
        request.enabled = true;
        synaptome::runtime::CompositionAssignment assignment;
        assignment.kind = synaptome::runtime::CompositionKind::Element;
        assignment.definitionId = request.definitionId;
        assignment.label = "Control Plane A";
        assignment.typeId = request.typeId;
        assignment.registryPrefix = request.registryPrefix;
        assignment.active = request.enabled;
        assignment.opacity = 0.42f;
        auto controlPlanePrepared = runtime.prepareElement(request);
        const auto controlPlaneCommit = runtime.adoptPreparedElement(
            2,
            std::move(controlPlanePrepared),
            assignment);
        require(
            controlPlaneCommit &&
                controlPlaneCommit.elementChanged &&
                controlPlaneCommit.parametersChanged,
            "control-plane adoption did not report its committed mutation");
        const auto* controlPlaneLayer = &compositionLayers[2];
        require(
            controlPlaneLayer &&
                controlPlaneLayer->hasElement() &&
                controlPlaneLayer->kind ==
                    synaptome::runtime::CompositionKind::Element &&
                controlPlaneLayer->assetId == assignment.definitionId &&
                controlPlaneLayer->label == assignment.label &&
                controlPlaneLayer->type == assignment.typeId &&
                controlPlaneLayer->paramPrefix == assignment.registryPrefix &&
                controlPlaneLayer->active &&
                std::fabs(controlPlaneLayer->opacity - 0.42f) < 0.0001f,
            "control-plane adoption did not publish the assignment");
        auto* controlPlaneOpacity =
            parameters.findFloat("console.layer3.opacity");
        require(
            controlPlaneOpacity &&
                controlPlaneOpacity->value == &compositionLayers[2].opacity &&
                std::fabs(controlPlaneOpacity->baseValue - 0.42f) < 0.0001f,
            "Runtime did not bind spine-owned opacity to stable slot storage");
        float* const stableOpacityAddress = controlPlaneOpacity->value;
        modifier::Modifier opacityModifier;
        auto& opacityRuntimeModifier = parameters.addFloatModifier(
            "console.layer3.opacity",
            opacityModifier);
        opacityRuntimeModifier.ownerTag = "tests.stable-opacity-mapping";

        request.definitionId = "tests.definition.control-plane-b";
        request.instanceId = "tests.instance.control-plane-b";
        request.enabled = false;
        auto replacementPrepared = runtime.prepareElementReplacement(
            request,
            *runtime.legacyCompositionElementForHost(2));
        synaptome::runtime::CompositionAssignment replacementAssignment;
        replacementAssignment.kind =
            synaptome::runtime::CompositionKind::Element;
        replacementAssignment.definitionId = request.definitionId;
        replacementAssignment.label = "Control Plane B";
        replacementAssignment.typeId = request.typeId;
        replacementAssignment.registryPrefix = request.registryPrefix;
        replacementAssignment.active = request.enabled;
        replacementAssignment.opacity = 0.73f;
        const auto replacementCommit = runtime.adoptPreparedElement(
            2,
            std::move(replacementPrepared),
            replacementAssignment);
        require(static_cast<bool>(replacementCommit),
                "control-plane replacement was rejected");
        controlPlaneOpacity = parameters.findFloat("console.layer3.opacity");
        require(
            controlPlaneOpacity &&
                controlPlaneOpacity->value == stableOpacityAddress &&
                std::fabs(*stableOpacityAddress - 0.73f) < 0.0001f &&
                std::fabs(controlPlaneOpacity->baseValue - 0.73f) < 0.0001f &&
                controlPlaneOpacity->modifiers.size() == 1 &&
                controlPlaneOpacity->modifiers.front().ownerTag ==
                    "tests.stable-opacity-mapping",
            "replacement did not preserve opacity address and modifiers");

        request.definitionId = "tests.definition.control-plane-c";
        request.instanceId = "tests.instance.control-plane-c";
        auto mismatchedPrepared = runtime.prepareElementReplacement(
            request,
            *runtime.legacyCompositionElementForHost(2));
        auto mismatchedAssignment = replacementAssignment;
        mismatchedAssignment.definitionId = "tests.definition.wrong";
        const Layer* const elementBeforeRejectedCommit =
            compositionLayers[2].element();
        const auto rejectedCommit = runtime.adoptPreparedElement(
            2,
            std::move(mismatchedPrepared),
            mismatchedAssignment);
        require(
            !rejectedCommit &&
                rejectedCommit.errorCode ==
                    synaptome::runtime::CompositionMutationError::
                        ElementMismatch &&
                compositionLayers[2].element() ==
                    elementBeforeRejectedCommit &&
                compositionLayers[2].assetId ==
                    replacementAssignment.definitionId &&
                parameters.findFloat("console.layer3.opacity")->value ==
                    stableOpacityAddress,
            "rejected assignment changed live control-plane state");

        require(
            runtime.setCompositionLayerLabel(2, "Performance Layer") &&
                compositionLayers[2].label == "Performance Layer",
            "Runtime label command did not update presentation state");
        require(
            runtime.setCompositionLayerActive(2, true) &&
                compositionLayers[2].active &&
                compositionLayers[2].element()->isEnabled(),
            "Runtime active command did not update element state");

        float siblingState = 0.25f;
        ParameterRegistry::Descriptor controlPlaneSiblingDescriptor;
        controlPlaneSiblingDescriptor.label = "Sibling State";
        parameters.addFloat(
            "console.layer30.keep",
            &siblingState,
            siblingState,
            controlPlaneSiblingDescriptor);
        const auto clearElement = runtime.clearCompositionLayer(2);
        require(
            clearElement &&
                clearElement.elementChanged &&
                clearElement.parametersChanged &&
                !compositionLayers[2].hasElement() &&
                compositionLayers[2].assetId.empty() &&
                parameters.findFloat("console.layer3.value") == nullptr &&
                parameters.findFloat("console.layer3.opacity") == nullptr &&
                parameters.findFloat("console.layer30.keep") != nullptr,
            "Runtime clear did not reset exact element/container ownership");

        float sharedEffectRoute = 1.0f;
        parameters.addFloat(
            "effects.tests.route",
            &sharedEffectRoute,
            sharedEffectRoute,
            controlPlaneSiblingDescriptor);
        synaptome::runtime::CompositionAssignment effectAssignment;
        effectAssignment.kind =
            synaptome::runtime::CompositionKind::Effect;
        effectAssignment.definitionId = "fx.tests";
        effectAssignment.label = "Test Effect";
        effectAssignment.typeId = "fx.tests";
        effectAssignment.registryPrefix = "effects.tests";
        effectAssignment.active = true;
        effectAssignment.opacity = 0.6f;
        effectAssignment.coverage.defined = true;
        effectAssignment.coverage.mode.clear();
        effectAssignment.coverage.columns = -4;
        require(
            static_cast<bool>(
                runtime.assignCompositionEntry(2, effectAssignment)),
                "Runtime rejected a valid effect assignment");
        require(
            compositionLayers[2].kind ==
                    synaptome::runtime::CompositionKind::Effect &&
                compositionLayers[2].coverage.defined &&
                compositionLayers[2].coverage.mode == "upstream" &&
                compositionLayers[2].coverage.columns == 0 &&
                parameters.findFloat("effects.tests.opacity") == nullptr,
            "effect assignment normalization or opacity ownership drifted");
        synaptome::runtime::CompositionCoverage effectCoverage;
        effectCoverage.defined = true;
        effectCoverage.mode.clear();
        effectCoverage.columns = 3;
        require(
            runtime.setCompositionLayerCoverage(2, effectCoverage) &&
                compositionLayers[2].coverage.mode == "upstream" &&
                compositionLayers[2].coverage.columns == 3,
            "Runtime coverage command did not normalize effect coverage");
        const auto clearEffect = runtime.clearCompositionLayer(2);
        require(
            clearEffect &&
                !clearEffect.elementChanged &&
                !clearEffect.parametersChanged &&
                parameters.findFloat("effects.tests.route") != nullptr,
            "clearing an effect removed shared effect parameters");

        synaptome::runtime::CompositionAssignment overlayAssignment;
        overlayAssignment.kind =
            synaptome::runtime::CompositionKind::Overlay;
        overlayAssignment.definitionId = "ui.tests";
        overlayAssignment.label = "Test Overlay";
        overlayAssignment.typeId = "ui.tests";
        overlayAssignment.registryPrefix = "ui.tests";
        overlayAssignment.active = true;
        require(
            runtime.assignCompositionEntry(2, overlayAssignment) &&
                compositionLayers[2].kind ==
                    synaptome::runtime::CompositionKind::Overlay &&
                !runtime.setCompositionLayerCoverage(2, effectCoverage),
            "overlay assignment accepted effect-only coverage");
        require(
            static_cast<bool>(runtime.clearCompositionLayer(2)),
                "Runtime did not clear the overlay assignment");
        parameters.removeById("effects.tests.route");
        parameters.removeById("console.layer30.keep");

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.shutdown";
        request.instanceId = "tests.instance.shutdown";
        request.registryPrefix = "console.layer1";
        auto shutdownPrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(
                0,
                std::move(shutdownPrepared),
                assignmentFor(request, "Shutdown Element")),
            "runtime did not adopt the shutdown contract element");
        runtime.shutdownComposition();
        require(!compositionLayer->hasElement(),
                "composition shutdown retained an element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "composition shutdown leaked element parameters");
        require(parameters.findFloat("console.layer1.opacity") == nullptr,
                "composition shutdown leaked spine-owned opacity");
        runtime.shutdownComposition();

        ParameterRegistry destructorParameters;
        {
            synaptome::runtime::Runtime destructorRuntime(
                factory,
                destructorParameters);
            synaptome::runtime::Runtime::ElementRequest destructorRequest;
            destructorRequest.typeId = "tests.runtime.good";
            destructorRequest.definitionId =
                "tests.definition.runtime-destructor";
            destructorRequest.instanceId =
                "tests.instance.runtime-destructor";
            destructorRequest.registryPrefix = "console.layer1";
            destructorRequest.enabled = true;
            auto destructorPrepared =
                destructorRuntime.prepareElement(destructorRequest);
            require(
                destructorRuntime.adoptPreparedElement(
                    0,
                    std::move(destructorPrepared),
                    assignmentFor(
                        destructorRequest,
                        "Runtime Destructor Element",
                        0.55f)),
                "destructor cleanup element was not adopted");
            require(
                destructorParameters.findFloat(
                    "console.layer1.opacity") != nullptr,
                "destructor cleanup setup lacked spine-owned opacity");
        }
        require(
            destructorParameters.findFloat("console.layer1.value") == nullptr &&
                destructorParameters.findBool("console.layer1.gate") == nullptr &&
                destructorParameters.findFloat(
                    "console.layer1.opacity") == nullptr,
            "Runtime destructor left element/container parameter pointers live");

        std::cout << "[runtime_core] PASS lifecycle, ownership, composition routing\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[runtime_core] FAIL " << error.what() << "\n";
        return 1;
    }
}

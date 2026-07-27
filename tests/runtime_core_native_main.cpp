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
}

int main() {
    try {
        LayerFactory& factory = LayerFactory::instance();
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
            runtime.adoptPreparedElement(0, std::move(prepared)),
            "runtime did not commit the prepared element");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "adoption did not commit staged parameters");
        require(parameters.findBool("console.layer1.gate") != nullptr,
                "adoption did not commit staged bool parameters");
        float hostOpacity = 0.8f;
        ParameterRegistry::Descriptor hostDescriptor;
        hostDescriptor.label = "Host Opacity";
        parameters.addFloat(
            "console.layer1.opacity",
            &hostOpacity,
            hostOpacity,
            hostDescriptor);
        require(
            progress == std::vector<std::string>({"create", "configure", "setup", "enable"}),
            "lifecycle progress order drifted");

        auto collision = runtime.prepareElement(request);
        require(!collision, "occupied parameter prefix was accepted");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "prefix collision damaged the live registration");

        runtime.releaseCompositionElement(0);
        require(!runtime.compositionLayer(0)->hasElement(),
                "release did not destroy the element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "release did not remove instance parameters");
        require(parameters.findBool("console.layer1.gate") == nullptr,
                "release did not remove instance bool parameters");
        require(parameters.findFloat("console.layer1.opacity") != nullptr,
                "release removed a host-owned layer parameter");
        parameters.removeById("console.layer1.opacity");
        request.definitionId = "tests.definition.reloaded";
        request.instanceId = "tests.instance.reloaded";
        auto reloaded = runtime.prepareElement(request);
        require(static_cast<bool>(reloaded),
                "slot prefix could not be reused after host-owned cleanup");
        require(runtime.adoptPreparedElement(0, std::move(reloaded)),
                "reloaded element was not adopted");
        runtime.releaseCompositionElement(0);
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
                !runtime.adoptPreparedElement(0, std::move(mismatched)),
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
                    std::move(foreignRuntimeCandidate)),
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
            runtime.adoptPreparedElement(0, std::move(compositionPrepared)),
            "runtime did not adopt the prepared composition element");
        auto* compositionLayer = runtime.compositionLayer(0);
        require(compositionLayer != nullptr && compositionLayer->hasElement(),
                "runtime did not retain composition element ownership");
        compositionLayer->active = true;
        auto* compositionElement =
            dynamic_cast<ContractElement*>(compositionLayer->element());
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
            *compositionLayer->element());
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
                *compositionLayer->element());
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

        float replacementOpacity = 0.72f;
        ParameterRegistry::Descriptor replacementHostDescriptor;
        replacementHostDescriptor.label = "Host Opacity";
        parameters.addFloat(
            "console.layer1.opacity",
            &replacementOpacity,
            replacementOpacity,
            replacementHostDescriptor);
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
                *compositionLayer->element());
            require(static_cast<bool>(conflictingReplacement),
                    conflictingReplacement.error);
            require(
                !runtime.adoptPreparedElement(
                    0,
                    std::move(conflictingReplacement)),
                "replacement overwrote a host-owned parameter");
            require(compositionLayer->element() == compositionElement,
                    "failed commit replaced the live element");
            require(
                parameters.findFloat("console.layer1.value") != nullptr &&
                    parameters.findFloat("console.layer1.opacity") != nullptr &&
                    parameters.findFloat("console.layer1.hostOwned") != nullptr &&
                    replacementOpacity == 0.72f &&
                    hostOwnedValue == 0.91f,
                "failed commit mutated the live registry");
        }
        parameters.removeById("console.layer1.hostOwned");
        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.replacement";
        request.instanceId = "tests.instance.replacement";
        auto replacement = runtime.prepareElementReplacement(
            request,
            *compositionLayer->element());
        require(static_cast<bool>(replacement), replacement.error);
        require(compositionLayer->element() == compositionElement,
                "preparation replaced the live element before commit");
        require(runtime.adoptPreparedElement(0, std::move(replacement)),
                "same-address replacement did not commit");
        auto* replacementElement =
            dynamic_cast<ContractElement*>(compositionLayer->element());
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
        require(parameters.findFloat("console.layer1.opacity") != nullptr &&
                    replacementOpacity == 0.72f,
                "replacement removed a host-owned same-address parameter");

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
        runtime.releaseCompositionElement(0);
        require(!compositionLayer->hasElement(),
                "runtime did not release its composition element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "composition release leaked element parameters");
        require(parameters.findFloat("console.layer1.opacity") != nullptr,
                "composition release removed the host-owned opacity parameter");
        parameters.removeById("console.layer1.opacity");
        parameters.removeById("host.live");

        request.typeId = "tests.runtime.registry-aware";
        request.definitionId = "tests.definition.registry-aware";
        request.instanceId = "tests.instance.registry-aware";
        request.registryPrefix = "console.layer2";
        auto registryAwarePrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(1, std::move(registryAwarePrepared)),
            "runtime did not adopt the registry-aware element");
        auto* registryAware = dynamic_cast<RegistryAwareElement*>(
            runtime.compositionLayer(1)->element());
        require(
            registryAware &&
                registryAware->commitCount == 1 &&
                registryAware->committedRegistry == &parameters &&
                registryAware->setupRegistry != &parameters,
            "runtime did not rebind the staged registry after commit");
        runtime.compositionLayer(1)->active = true;
        runtime.updateCompositionElements(LayerUpdateParams{});
        require(registryAware->sawLiveRegistry,
                "registry-aware update did not observe the live registry");
        runtime.releaseCompositionElement(1);

        request.typeId = "tests.runtime.good";
        request.definitionId = "tests.definition.shutdown";
        request.instanceId = "tests.instance.shutdown";
        request.registryPrefix = "console.layer1";
        auto shutdownPrepared = runtime.prepareElement(request);
        require(
            runtime.adoptPreparedElement(0, std::move(shutdownPrepared)),
            "runtime did not adopt the shutdown contract element");
        runtime.shutdownComposition();
        require(!compositionLayer->hasElement(),
                "composition shutdown retained an element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "composition shutdown leaked element parameters");
        runtime.shutdownComposition();

        std::cout << "[runtime_core] PASS lifecycle, ownership, composition routing\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[runtime_core] FAIL " << error.what() << "\n";
        return 1;
    }
}

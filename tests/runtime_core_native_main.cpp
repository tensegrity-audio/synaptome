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
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "setup parameter was not registered");
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

        runtime.releasePreparedElement(prepared);
        require(!prepared, "release did not destroy the element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "release did not remove instance parameters");
        require(parameters.findFloat("console.layer1.opacity") != nullptr,
                "release removed a host-owned layer parameter");
        parameters.removeById("console.layer1.opacity");
        request.definitionId = "tests.definition.reloaded";
        request.instanceId = "tests.instance.reloaded";
        auto reloaded = runtime.prepareElement(request);
        require(static_cast<bool>(reloaded),
                "slot prefix could not be reused after host-owned cleanup");
        runtime.releasePreparedElement(reloaded);
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
            require(parameters.findFloat("console.layer4.value") != nullptr,
                    "abandoned result did not register parameters");
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
                foreignParameters.findFloat("console.layer1.value") != nullptr,
                "cross-runtime rejection damaged source ownership");
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

        request.definitionId = "tests.definition.shutdown";
        request.instanceId = "tests.instance.shutdown";
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

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
    void update(const LayerUpdateParams&) override {}
    void draw(const LayerDrawParams&) override {}
    void setExternalEnabled(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

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
        require(prepared.element->instanceId() == request.instanceId,
                "instance identity was not assigned");
        require(prepared.element->registryPrefix() == request.registryPrefix,
                "registry prefix was not assigned");
        require(!prepared.element->isEnabled(), "requested enable state was not applied");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "setup parameter was not registered");
        require(
            progress == std::vector<std::string>({"create", "configure", "setup", "enable"}),
            "lifecycle progress order drifted");

        auto collision = runtime.prepareElement(request);
        require(!collision, "occupied parameter prefix was accepted");
        require(parameters.findFloat("console.layer1.value") != nullptr,
                "prefix collision damaged the live registration");

        runtime.releaseElement(prepared.element);
        require(!prepared.element, "release did not destroy the element");
        require(parameters.findFloat("console.layer1.value") == nullptr,
                "release did not remove instance parameters");
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
        runtime.releaseElement(empty.element);

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

        std::cout << "[runtime_core] PASS lifecycle, identity, ownership, cleanup\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[runtime_core] FAIL " << error.what() << "\n";
        return 1;
    }
}

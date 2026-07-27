#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "ofFbo.h"
#include "stubs/SynaptomeTestPaths.h"

#ifndef TWO_PI
#define TWO_PI 6.28318530717958647692f
#endif
inline float ofDegToRad(float degrees) { return degrees * 0.01745329251994329577f; }

#define private public
#include "../synaptome/src/visuals/SignalBloomLayer.h"
#undef private
#include "../synaptome/src/runtime/SignalBloomRegistration.h"
#include "../synaptome/src/visuals/LayerFactory.h"

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string kindName(synaptome::element::ParameterKind kind) {
    switch (kind) {
    case synaptome::element::ParameterKind::Float:
        return "float";
    case synaptome::element::ParameterKind::Bool:
        return "bool";
    case synaptome::element::ParameterKind::String:
        return "string";
    default:
        return {};
    }
}

bool parameterValueMatchesJson(
    const synaptome::element::ParameterValue& value,
    const ofJson& json) {
    if (const auto* number = std::get_if<float>(&value)) {
        return json.is_number() &&
            std::fabs(json.get<float>() - *number) < 0.0001f;
    }
    if (const auto* boolean = std::get_if<bool>(&value)) {
        return json.is_boolean() && json.get<bool>() == *boolean;
    }
    if (const auto* text = std::get_if<std::string>(&value)) {
        return json.is_string() && json.get<std::string>() == *text;
    }
    return false;
}

std::string simplePackageLabel(const std::string& label) {
    const auto separator = label.find(':');
    if (separator == std::string::npos) {
        return label;
    }
    const auto first = label.find_first_not_of(' ', separator + 1);
    return first == std::string::npos ? std::string() : label.substr(first);
}
}

int main() {
    try {
        LayerFactory factory;
        synaptome::runtime::registerSignalBloomElement(factory);

        const auto* descriptor =
            factory.descriptor("example.signalBloom");
        auto descriptors = factory.descriptors();
        require(
            descriptor &&
                descriptor->typeId == "example.signalBloom" &&
                descriptor->kind ==
                    synaptome::element::ElementKind::Visual &&
                descriptor->actions.empty() &&
                descriptors.size() == 1 &&
                descriptors.front().typeId == "example.signalBloom",
            "static Signal Bloom descriptor was not inspectable before creation");
        descriptors.front().typeId = "copy.mutated";
        require(
            factory.descriptor("example.signalBloom")->typeId ==
                "example.signalBloom",
            "mutating the enumerated descriptor copy changed the factory");

        const auto* typeContract =
            factory.typeContract("example.signalBloom");
        auto typeContractCopies = factory.typeContracts();
        require(
            typeContract &&
                typeContract->state ==
                    LayerFactory::ParameterDeclarationState::Declared &&
                typeContract->contract.element.typeId ==
                    "example.signalBloom" &&
                typeContract->contract.parameters.groups.size() == 5 &&
                typeContract->contract.parameters.parameters.size() == 18 &&
                typeContractCopies.size() == 1,
            "Signal Bloom static parameter contract was not inspectable before creation");

        const std::vector<std::pair<std::string, std::string>>
            expectedGroups = {
                {"example", "Example"},
                {"exampleMotion", "Example Motion"},
                {"exampleTransform", "Example Transform"},
                {"exampleColor", "Example Color"},
                {"exampleModulation", "Example Modulation"},
            };
        for (std::size_t index = 0; index < expectedGroups.size(); ++index) {
            const auto& actual =
                typeContract->contract.parameters.groups[index];
            require(
                actual.id == expectedGroups[index].first &&
                    actual.label == expectedGroups[index].second,
                "Signal Bloom static parameter group order or metadata drifted");
        }

        const auto packagePath = synaptome_test_paths::appRoot().parent_path() /
            "docs" / "examples" / "layer_packages" / "signal_bloom" / "layer.package.json";
        std::ifstream packageStream(packagePath);
        require(static_cast<bool>(packageStream), "could not read package declaration");
        ofJson package;
        packageStream >> package;
        require(
            package["asset"].value("type", std::string()) ==
                descriptor->typeId,
            "package asset type does not match the registered descriptor");
        require(
            package["parameters"].is_array() &&
                package["parameters"].size() ==
                    typeContract->contract.parameters.parameters.size(),
            "package/static parameter counts drifted");

        const std::vector<std::string> expectedParameterGroups = {
            "example",
            "exampleMotion",
            "exampleMotion",
            "exampleMotion",
            "exampleTransform",
            "exampleTransform",
            "exampleColor",
            "exampleModulation",
            "exampleColor",
            "exampleColor",
            "exampleColor",
            "exampleColor",
            "exampleColor",
            "exampleColor",
            "exampleColor",
            "exampleModulation",
            "exampleModulation",
            "exampleModulation",
        };
        for (std::size_t index = 0;
             index < typeContract->contract.parameters.parameters.size();
             ++index) {
            const auto& declaration =
                typeContract->contract.parameters.parameters[index];
            const auto& packaged = package["parameters"][index];
            const std::string context =
                "Signal Bloom parameter " + declaration.id;
            require(
                declaration.id ==
                        packaged.value("id", std::string()) &&
                    kindName(declaration.kind) ==
                        packaged.value("kind", std::string()) &&
                    declaration.groupId == expectedParameterGroups[index] &&
                    declaration.label ==
                        simplePackageLabel(
                            packaged.value("label", std::string())) &&
                    parameterValueMatchesJson(
                        declaration.defaultValue,
                        packaged["default"]) &&
                    declaration.units ==
                        packaged.value("units", std::string()) &&
                    declaration.description ==
                        packaged.value("description", std::string()) &&
                    !declaration.quickAccessOrder &&
                    declaration.aliases.empty(),
                context + " identity/default/metadata drifted");

            const bool packageHasRange = packaged.contains("range");
            require(
                declaration.range.has_value() == packageHasRange,
                context + " range presence drifted");
            if (declaration.range) {
                const auto& range = packaged["range"];
                require(
                    std::fabs(
                        declaration.range->min -
                        range.value("min", 0.0f)) < 0.0001f &&
                        std::fabs(
                            declaration.range->max -
                            range.value("max", 0.0f)) < 0.0001f &&
                        declaration.range->step.has_value() ==
                            range.contains("step") &&
                        (!declaration.range->step ||
                         std::fabs(
                             *declaration.range->step -
                             range.value("step", 0.0f)) < 0.0001f),
                    context + " range values drifted");
            }

            const auto packageOptions =
                packaged.value("options", ofJson::array());
            require(
                packageOptions.is_array() &&
                    declaration.options.size() ==
                        packageOptions.size(),
                context + " static option count drifted");
            for (std::size_t optionIndex = 0;
                 optionIndex < declaration.options.size();
                 ++optionIndex) {
                const auto& declaredOption =
                    declaration.options[optionIndex];
                const auto& packagedOption =
                    packageOptions[optionIndex];
                require(
                    parameterValueMatchesJson(
                        declaredOption.value,
                        packagedOption["value"]) &&
                        declaredOption.label ==
                            packagedOption.value(
                                "label",
                                std::string()) &&
                        declaredOption.description ==
                            packagedOption.value(
                                "description",
                                std::string()),
                    context + " static option metadata drifted");
            }

            const bool packageHasOptionSource =
                packaged.contains("optionsSource");
            require(
                declaration.optionSource.has_value() ==
                    packageHasOptionSource,
                context + " option-source presence drifted");
            if (declaration.optionSource) {
                const auto& source = packaged["optionsSource"];
                require(
                    declaration.optionSource->id ==
                            source.value("id", std::string()) &&
                        declaration.optionSource->valueField ==
                            source.value("value", std::string()) &&
                        declaration.optionSource->labelField ==
                            source.value("label", std::string()),
                    context + " option-source selectors drifted");
            }

            const bool packageHasDeprecation =
                packaged.contains("deprecated");
            require(
                declaration.deprecation.has_value() ==
                    packageHasDeprecation,
                context + " deprecation presence drifted");
            if (declaration.deprecation) {
                const auto& deprecated = packaged["deprecated"];
                require(
                    declaration.deprecation->replacementId ==
                            deprecated.value(
                                "replacement",
                                std::string()) &&
                        declaration.deprecation->reason ==
                            deprecated.value(
                                "reason",
                                std::string()),
                    context + " deprecation metadata drifted");
            }
        }

        typeContractCopies[0].state =
            LayerFactory::ParameterDeclarationState::LegacySetupDiscovery;
        typeContractCopies[0].contract.parameters.groups[0].id =
            "copyMutated";
        typeContractCopies[0].contract.parameters.parameters[0].id =
            "copyMutated";
        require(
            factory.typeContract("example.signalBloom") == typeContract &&
                typeContract->state ==
                    LayerFactory::ParameterDeclarationState::Declared &&
                typeContract->contract.parameters.groups[0].id ==
                    "example" &&
                typeContract->contract.parameters.parameters[0].id ==
                    "visible",
            "mutating copied Signal Bloom contracts changed factory state");

        LayerFactory migrationStateFactory;
        migrationStateFactory.registerType(
            synaptome::element::ElementDescriptor{
                "tests.signalBloom.legacy",
                synaptome::element::ElementKind::Visual,
                {},
            },
            [] {
                return std::make_unique<SignalBloomLayer>();
            });
        migrationStateFactory.registerType(
            synaptome::element::ElementTypeContract{
                {
                    "tests.signalBloom.declaredEmpty",
                    synaptome::element::ElementKind::Visual,
                    {},
                },
                {},
            },
            [] {
                return std::make_unique<SignalBloomLayer>();
            });
        require(
            migrationStateFactory
                    .typeContract("tests.signalBloom.legacy")
                    ->state ==
                LayerFactory::ParameterDeclarationState::
                    LegacySetupDiscovery &&
                migrationStateFactory
                    .typeContract("tests.signalBloom.declaredEmpty")
                    ->state ==
                LayerFactory::ParameterDeclarationState::Declared &&
                migrationStateFactory
                    .typeContract("tests.signalBloom.declaredEmpty")
                    ->contract.parameters.parameters.empty(),
            "legacy discovery and declared-empty parameter states collapsed");

        auto layer = factory.create("example.signalBloom");
        require(layer != nullptr, "factory did not create Signal Bloom");
        layer->setRegistryPrefix("bench.signal_bloom");
        layer->setInstanceId("bench-1");

        ofJson config = {
            {"defaults", {
                {"visible", true},
                {"speed", 0.9},
                {"bpmSync", true},
                {"bpmMultiplier", 2.0},
                {"scale", 0.75},
                {"color", {0.2, 0.8, 1.0}},
                {"backgroundColor", {0.01, 0.02, 0.06}}
            }}
        };
        layer->configure(config);

        ParameterRegistry registry;
        layer->setup(registry);
        require(registry.floats().size() == 16, "unexpected float parameter count");
        require(registry.bools().size() == 2, "unexpected bool parameter count");
        require(registry.findFloat("bench.signal_bloom.bpmMultiplier") != nullptr,
                "named BPM multiplier parameter missing");

        std::unordered_map<std::string, ofJson> declared;
        for (const auto& parameter : package["parameters"]) {
            declared[parameter.value("id", std::string())] = parameter;
        }
        require(declared.size() == registry.floats().size() + registry.bools().size(),
                "package/runtime parameter counts drifted");
        const std::string prefix = "bench.signal_bloom.";
        for (const auto& parameter : registry.floats()) {
            require(parameter.meta.id.rfind(prefix, 0) == 0,
                    "runtime float escaped the bench registry prefix");
            const std::string suffix = parameter.meta.id.substr(prefix.size());
            const auto it = declared.find(suffix);
            require(it != declared.end(), "runtime float is absent from package: " + suffix);
            require(it->second.value("kind", std::string()) == "float",
                    "package kind drift for: " + suffix);
            const auto range = it->second.value("range", ofJson::object());
            require(std::fabs(range.value("min", 0.0f) - parameter.meta.range.min) < 0.0001f &&
                    std::fabs(range.value("max", 0.0f) - parameter.meta.range.max) < 0.0001f,
                    "package/runtime range drift for: " + suffix);
        }
        for (const auto& parameter : registry.bools()) {
            require(parameter.meta.id.rfind(prefix, 0) == 0,
                    "runtime bool escaped the bench registry prefix");
            const std::string suffix = parameter.meta.id.substr(prefix.size());
            const auto it = declared.find(suffix);
            require(it != declared.end(), "runtime bool is absent from package: " + suffix);
            require(it->second.value("kind", std::string()) == "bool",
                    "package kind drift for: " + suffix);
        }

        // Scene/operator values are applied after package defaults and preset
        // values, so this explicit value must win for the live instance.
        registry.setFloatBase("bench.signal_bloom.speed", 2.5f, true);
        require(std::fabs(registry.getFloatBase("bench.signal_bloom.speed") - 2.5f) < 0.0001f,
                "explicit scene value did not win");

        for (int frame = 0; frame < 240; ++frame) {
            LayerUpdateParams update;
            update.dt = 1.0f / 60.0f;
            update.time = static_cast<float>(frame) * update.dt;
            update.bpm = 128.0f;
            update.speed = 1.0f;
            layer->update(update);
        }

        auto* signalBloom = dynamic_cast<SignalBloomLayer*>(layer.get());
        require(signalBloom != nullptr, "factory returned wrong layer type");
        require(signalBloom->points_.size() == 96, "point field was not initialized");
        require(std::isfinite(signalBloom->phase_) && signalBloom->phase_ > 0.0f,
                "update lifecycle did not advance a finite phase");

        ofFbo fbo;
        fbo.allocate(640, 360);
        require(fbo.isAllocated(), "offscreen framebuffer allocation failed");
        ofCamera camera;
        LayerDrawParams draw{camera};
        draw.viewport = {640, 360};
        draw.slotOpacity = 0.85f;
        fbo.begin();
        layer->draw(draw);
        fbo.end();

        layer->setExternalEnabled(false);
        require(!layer->isEnabled(), "external visibility toggle failed");

        bool duplicateRejected = false;
        try {
            synaptome::runtime::registerSignalBloomElement(factory);
        } catch (const std::logic_error&) {
            duplicateRejected = true;
        }
        require(duplicateRejected, "duplicate factory registration was not rejected");

        std::cout << "[layer_package_bench] PASS examples.signal_bloom: "
                  << registry.floats().size() + registry.bools().size()
                  << " parameters, 240 updates, stub-backed draw call\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[layer_package_bench] FAIL: " << e.what() << "\n";
        return 1;
    }
}

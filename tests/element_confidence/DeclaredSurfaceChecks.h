#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ofMain.h"
#include "visuals/LayerFactory.h"

namespace synaptome::tests::element_confidence {

inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

inline std::string parameterKindName(
    synaptome::element::ParameterKind kind) {
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

inline bool parameterValueMatchesJson(
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

inline std::string simplePackageLabel(const std::string& label) {
    const auto separator = label.find(':');
    if (separator == std::string::npos) {
        return label;
    }
    const auto first = label.find_first_not_of(' ', separator + 1);
    return first == std::string::npos
        ? std::string()
        : label.substr(first);
}

struct DeclaredSurfaceEvidence {
    const synaptome::element::ElementDescriptor* descriptor = nullptr;
    const LayerFactory::ElementTypeContractRecord* typeContract = nullptr;
    ofJson package;
};

inline DeclaredSurfaceEvidence verifyDeclaredPackageSurface(
    LayerFactory& factory,
    const std::string& typeId,
    const std::filesystem::path& packagePath,
    const std::vector<std::pair<std::string, std::string>>& expectedGroups,
    const std::vector<std::string>& expectedParameterGroups,
    const std::string& displayName) {
    DeclaredSurfaceEvidence evidence;
    evidence.descriptor = factory.descriptor(typeId);
    auto descriptors = factory.descriptors();
    require(
        evidence.descriptor &&
            evidence.descriptor->typeId == typeId &&
            evidence.descriptor->kind ==
                synaptome::element::ElementKind::Visual &&
            evidence.descriptor->actions.empty() &&
            descriptors.size() == 1 &&
            descriptors.front().typeId == typeId,
        "static " + displayName +
            " descriptor was not inspectable before creation");
    descriptors.front().typeId = "copy.mutated";
    require(
        factory.descriptor(typeId)->typeId == typeId,
        "mutating the enumerated descriptor copy changed the factory");

    evidence.typeContract = factory.typeContract(typeId);
    const auto typeContractCopies = factory.typeContracts();
    require(
        evidence.typeContract &&
            evidence.typeContract->state ==
                LayerFactory::ParameterDeclarationState::Declared &&
            evidence.typeContract->contract.element.typeId == typeId &&
            evidence.typeContract->contract.parameters.groups.size() ==
                expectedGroups.size() &&
            evidence.typeContract->contract.parameters.parameters.size() ==
                expectedParameterGroups.size() &&
            typeContractCopies.size() == 1,
        displayName +
            " static parameter contract was not inspectable before creation");

    for (std::size_t index = 0; index < expectedGroups.size(); ++index) {
        const auto& actual =
            evidence.typeContract->contract.parameters.groups[index];
        require(
            actual.id == expectedGroups[index].first &&
                actual.label == expectedGroups[index].second,
            displayName +
                " static parameter group order or metadata drifted");
    }

    std::ifstream packageStream(packagePath);
    require(
        static_cast<bool>(packageStream),
        "could not read package declaration");
    packageStream >> evidence.package;
    require(
        evidence.package["asset"].value("type", std::string()) ==
            evidence.descriptor->typeId,
        "package asset type does not match the registered descriptor");
    require(
        evidence.package["parameters"].is_array() &&
            evidence.package["parameters"].size() ==
                evidence.typeContract->contract.parameters.parameters.size(),
        "package/static parameter counts drifted");

    for (std::size_t index = 0;
         index <
             evidence.typeContract->contract.parameters.parameters.size();
         ++index) {
        const auto& declaration =
            evidence.typeContract->contract.parameters.parameters[index];
        const auto& packaged = evidence.package["parameters"][index];
        const std::string context =
            displayName + " parameter " + declaration.id;
        require(
            declaration.id == packaged.value("id", std::string()) &&
                parameterKindName(declaration.kind) ==
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
                declaration.options.size() == packageOptions.size(),
            context + " static option count drifted");
        for (std::size_t optionIndex = 0;
             optionIndex < declaration.options.size();
             ++optionIndex) {
            const auto& declaredOption =
                declaration.options[optionIndex];
            const auto& packagedOption = packageOptions[optionIndex];
            require(
                parameterValueMatchesJson(
                    declaredOption.value,
                    packagedOption["value"]) &&
                    declaredOption.label ==
                        packagedOption.value("label", std::string()) &&
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
                        deprecated.value("reason", std::string()),
                context + " deprecation metadata drifted");
        }
    }
    return evidence;
}

} // namespace synaptome::tests::element_confidence

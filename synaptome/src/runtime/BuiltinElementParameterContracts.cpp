#include "BuiltinElementParameterContracts.h"

#include "../ofJson.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace synaptome::runtime {
namespace {

#include "BuiltinElementParameterContracts.generated.inc"

element::ParameterKind parseKind(const std::string& kind) {
    if (kind == "float") {
        return element::ParameterKind::Float;
    }
    if (kind == "bool") {
        return element::ParameterKind::Bool;
    }
    if (kind == "string") {
        return element::ParameterKind::String;
    }
    throw std::logic_error(
        "generated built-in parameter contract has invalid kind: " +
        kind);
}

element::ParameterValue parseValue(
    const ofJson& value,
    element::ParameterKind kind) {
    switch (kind) {
    case element::ParameterKind::Float:
        return value.get<float>();
    case element::ParameterKind::Bool:
        return value.get<bool>();
    case element::ParameterKind::String:
        return value.get<std::string>();
    }
    throw std::logic_error(
        "generated built-in parameter contract has invalid value kind");
}

element::ParameterDeclaration parseParameter(
    const ofJson& node) {
    element::ParameterDeclaration parameter;
    parameter.id = node.at("id").get<std::string>();
    parameter.kind =
        parseKind(node.at("kind").get<std::string>());
    parameter.groupId =
        node.at("groupId").get<std::string>();
    parameter.label = node.at("label").get<std::string>();
    parameter.defaultValue =
        parseValue(node.at("default"), parameter.kind);
    parameter.units = node.value("units", std::string());
    parameter.description =
        node.value("description", std::string());

    if (node.contains("range")) {
        const auto& rangeNode = node.at("range");
        element::ParameterRange range;
        range.min = rangeNode.at("min").get<float>();
        range.max = rangeNode.at("max").get<float>();
        if (rangeNode.contains("step")) {
            range.step = rangeNode.at("step").get<float>();
        }
        parameter.range = range;
    }
    if (node.contains("options")) {
        for (const auto& optionNode : node.at("options")) {
            parameter.options.push_back({
                parseValue(
                    optionNode.at("value"),
                    parameter.kind),
                optionNode.at("label").get<std::string>(),
            });
        }
    }
    if (node.contains("optionSource")) {
        const auto& sourceNode = node.at("optionSource");
        parameter.optionSource =
            element::ParameterOptionSource{
                sourceNode.at("id").get<std::string>(),
                sourceNode.at("valueField").get<std::string>(),
                sourceNode.at("labelField").get<std::string>(),
            };
    }
    if (node.contains("quickAccessOrder")) {
        parameter.quickAccessOrder =
            node.at("quickAccessOrder").get<int>();
    }
    if (node.contains("aliases")) {
        parameter.aliases =
            node.at("aliases").get<std::vector<std::string>>();
    }
    if (node.contains("deprecation")) {
        const auto& deprecationNode =
            node.at("deprecation");
        parameter.deprecation =
            element::ParameterDeprecation{
                deprecationNode
                    .value("reason", std::string()),
                deprecationNode
                    .value("replacementId", std::string()),
            };
    }
    return parameter;
}

using DeclarationMap =
    std::unordered_map<
        std::string,
        element::ParameterDeclarationSet>;

DeclarationMap parseDeclarations() {
    std::string payload;
    for (const char* chunk :
         kBuiltinElementParameterContractJsonChunks) {
        payload += chunk;
    }
    const auto root = ofJson::parse(payload);
    DeclarationMap declarations;
    for (const auto& typeNode : root.at("types")) {
        const std::string typeId =
            typeNode.at("typeId").get<std::string>();
        const auto& declarationNode =
            typeNode.at("declarations");
        element::ParameterDeclarationSet set;
        for (const auto& groupNode :
             declarationNode.at("groups")) {
            set.groups.push_back({
                groupNode.at("id").get<std::string>(),
                groupNode.at("label").get<std::string>(),
                groupNode.value(
                    "description",
                    std::string()),
            });
        }
        for (const auto& parameterNode :
             declarationNode.at("parameters")) {
            set.parameters.push_back(
                parseParameter(parameterNode));
        }
        if (!declarations.emplace(
                typeId,
                std::move(set)).second) {
            throw std::logic_error(
                "generated built-in parameter contract has "
                "duplicate type: " + typeId);
        }
    }
    return declarations;
}

const DeclarationMap& declarationMap() {
    static const DeclarationMap declarations =
        parseDeclarations();
    return declarations;
}

} // namespace

const element::ParameterDeclarationSet&
builtinElementParameterDeclarations(std::string_view typeId) {
    const auto& declarations = declarationMap();
    const auto found = declarations.find(std::string(typeId));
    if (found == declarations.end()) {
        throw std::out_of_range(
            "missing generated built-in parameter contract: " +
            std::string(typeId));
    }
    return found->second;
}

std::vector<std::string> builtinElementParameterTypeIds() {
    std::vector<std::string> ids;
    ids.reserve(declarationMap().size());
    for (const auto& [typeId, declarations] :
         declarationMap()) {
        (void)declarations;
        ids.push_back(typeId);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace synaptome::runtime

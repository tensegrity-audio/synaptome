#include "BuiltinElementContractExporter.h"

#include "BuiltinElementParameterContracts.h"
#include "BuiltinElements.h"
#include "Runtime.h"

#include "../core/ParameterRegistry.h"
#include "../visuals/LayerFactory.h"
#include "../visuals/LayerLibrary.h"

#include <synaptome/element/Parameter.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace synaptome::runtime {
namespace {

constexpr const char* kSnapshotPrefix = "__element_contract__";

std::string kindName(element::ParameterKind kind) {
    switch (kind) {
    case element::ParameterKind::Float:
        return "float";
    case element::ParameterKind::Bool:
        return "bool";
    case element::ParameterKind::String:
        return "string";
    }
    return "invalid";
}

std::string groupIdForLabel(const std::string& source) {
    const std::string label = source.empty() ? "General" : source;
    std::string result;
    bool capitalizeNext = false;
    for (const unsigned char character : label) {
        if (!std::isalnum(character)) {
            capitalizeNext = !result.empty();
            continue;
        }
        char value = static_cast<char>(character);
        if (result.empty()) {
            if (!std::isalpha(character)) {
                result = "group";
                capitalizeNext = true;
            } else {
                value = static_cast<char>(std::tolower(character));
            }
        } else if (capitalizeNext) {
            value = static_cast<char>(std::toupper(character));
        }
        result.push_back(value);
        capitalizeNext = false;
    }
    return result.empty() ? "general" : result;
}

std::string localId(const std::string& fullId) {
    const std::string prefix =
        std::string(kSnapshotPrefix) + ".";
    if (fullId.rfind(prefix, 0) != 0 ||
        fullId.size() <= prefix.size()) {
        return {};
    }
    return fullId.substr(prefix.size());
}

ofJson valueJson(const element::ParameterValue& value) {
    return std::visit(
        [](const auto& entry) -> ofJson { return entry; },
        value);
}

ofJson declarationJson(
    const element::ParameterDeclaration& parameter) {
    ofJson result = {
        {"id", parameter.id},
        {"kind", kindName(parameter.kind)},
        {"groupId", parameter.groupId},
        {"label", parameter.label},
        {"default", valueJson(parameter.defaultValue)},
        {"units", parameter.units},
        {"description", parameter.description},
    };
    if (parameter.range) {
        result["range"] = {
            {"min", parameter.range->min},
            {"max", parameter.range->max},
        };
        if (parameter.range->step) {
            result["range"]["step"] =
                *parameter.range->step;
        }
    }
    if (parameter.quickAccessOrder) {
        result["quickAccessOrder"] =
            *parameter.quickAccessOrder;
    }
    if (!parameter.options.empty()) {
        result["options"] = ofJson::array();
        for (const auto& option : parameter.options) {
            result["options"].push_back({
                {"value", valueJson(option.value)},
                {"label", option.label},
            });
        }
    }
    if (parameter.optionSource) {
        result["optionSource"] = {
            {"id", parameter.optionSource->id},
            {"valueField", parameter.optionSource->valueField},
            {"labelField", parameter.optionSource->labelField},
        };
    }
    if (!parameter.aliases.empty()) {
        result["aliases"] = parameter.aliases;
    }
    if (parameter.deprecation) {
        result["deprecation"] = {
            {"reason", parameter.deprecation->reason},
            {"replacementId", parameter.deprecation->replacementId},
        };
    }
    return result;
}

ofJson declarationSetJson(
    const element::ParameterDeclarationSet& declarations) {
    ofJson result = {
        {"groups", ofJson::array()},
        {"parameters", ofJson::array()},
    };
    for (const auto& group : declarations.groups) {
        result["groups"].push_back({
            {"id", group.id},
            {"label", group.label},
            {"description", group.description},
        });
    }
    for (const auto& parameter : declarations.parameters) {
        result["parameters"].push_back(
            declarationJson(parameter));
    }
    return result;
}

element::ParameterDeclarationSet captureLegacyDeclarations(
    Layer& layer) {
    ParameterRegistry registry;
    layer.setRegistryPrefix(kSnapshotPrefix);
    layer.setInstanceId("contract.export");
    layer.setup(registry);

    element::ParameterDeclarationSet declarations;
    std::unordered_map<std::string, std::string>
        groupIdsByLabel;
    std::set<std::string> usedGroupIds;
    std::set<int> usedQuickAccessOrders;

    const auto groupId = [&](const std::string& rawLabel) {
        const std::string label =
            rawLabel.empty() ? "General" : rawLabel;
        const auto existing = groupIdsByLabel.find(label);
        if (existing != groupIdsByLabel.end()) {
            return existing->second;
        }
        std::string candidate = groupIdForLabel(label);
        const std::string base = candidate;
        int suffix = 2;
        while (!usedGroupIds.insert(candidate).second) {
            candidate = base + std::to_string(suffix++);
        }
        groupIdsByLabel.emplace(label, candidate);
        declarations.groups.push_back({
            candidate,
            label,
            {},
        });
        return candidate;
    };

    const auto quickAccessOrder =
        [&](const ParameterRegistry::Descriptor& meta)
        -> std::optional<int> {
        if (!meta.quickAccess) {
            return std::nullopt;
        }
        int order = std::max(0, meta.quickAccessOrder);
        while (!usedQuickAccessOrders.insert(order).second) {
            ++order;
        }
        return order;
    };

    for (const auto& parameter : registry.floats()) {
        const std::string id = localId(parameter.meta.id);
        if (id.empty()) {
            throw std::runtime_error(
                "legacy float parameter escaped export prefix: " +
                parameter.meta.id);
        }
        std::optional<element::ParameterRange> range;
        if (std::isfinite(parameter.meta.range.min) &&
            std::isfinite(parameter.meta.range.max)) {
            element::ParameterRange captured{
                parameter.meta.range.min,
                parameter.meta.range.max,
                std::nullopt,
            };
            if (std::isfinite(parameter.meta.range.step) &&
                parameter.meta.range.step > 0.0f) {
                captured.step = parameter.meta.range.step;
            }
            range = captured;
        }
        declarations.parameters.push_back({
            id,
            element::ParameterKind::Float,
            groupId(parameter.meta.group),
            parameter.meta.label,
            parameter.defaultValue,
            range,
            parameter.meta.units,
            parameter.meta.description,
            {},
            std::nullopt,
            quickAccessOrder(parameter.meta),
            {},
            std::nullopt,
        });
    }
    for (const auto& parameter : registry.bools()) {
        const std::string id = localId(parameter.meta.id);
        if (id.empty()) {
            throw std::runtime_error(
                "legacy bool parameter escaped export prefix: " +
                parameter.meta.id);
        }
        declarations.parameters.push_back({
            id,
            element::ParameterKind::Bool,
            groupId(parameter.meta.group),
            parameter.meta.label,
            parameter.defaultValue,
            std::nullopt,
            parameter.meta.units,
            parameter.meta.description,
            {},
            std::nullopt,
            quickAccessOrder(parameter.meta),
            {},
            std::nullopt,
        });
    }
    for (const auto& parameter : registry.strings()) {
        const std::string id = localId(parameter.meta.id);
        if (id.empty()) {
            throw std::runtime_error(
                "legacy string parameter escaped export prefix: " +
                parameter.meta.id);
        }
        declarations.parameters.push_back({
            id,
            element::ParameterKind::String,
            groupId(parameter.meta.group),
            parameter.meta.label,
            parameter.defaultValue,
            std::nullopt,
            parameter.meta.units,
            parameter.meta.description,
            {},
            std::nullopt,
            quickAccessOrder(parameter.meta),
            {},
            std::nullopt,
        });
    }
    return declarations;
}

void normalizeDynamicOptions(
    const std::string& typeId,
    element::ParameterDeclarationSet& declarations) {
    for (auto& parameter : declarations.parameters) {
        if (typeId == "media.clip" &&
            parameter.id == "clip") {
            parameter.range.reset();
            parameter.description =
                "Select a clip from the validated media catalog.";
            parameter.optionSource =
                element::ParameterOptionSource{
                    "media.videoClips",
                    "index",
                    "label",
                };
        } else if (
            typeId == "media.webcam" &&
            parameter.id == "device") {
            parameter.range.reset();
            parameter.description =
                "Select an available webcam device.";
            parameter.optionSource =
                element::ParameterOptionSource{
                    "media.webcamDevices",
                    "index",
                    "label",
                };
        } else if (
            typeId == "media.webcam" &&
            parameter.id == "resolution") {
            parameter.range.reset();
            parameter.description =
                "Select a resolution declared by the active webcam asset.";
            parameter.optionSource =
                element::ParameterOptionSource{
                    "media.webcamResolutions",
                    "index",
                    "label",
                };
        }
    }
}

ofJson parameterMetadataJson(
    const element::ParameterDeclaration& parameter) {
    auto metadata = declarationJson(parameter);
    metadata.erase("default");
    return metadata;
}

void mergeDeclarations(
    const std::string& typeId,
    const std::string& sourceId,
    element::ParameterDeclarationSet& target,
    const element::ParameterDeclarationSet& candidate) {
    for (const auto& group : candidate.groups) {
        const auto existing = std::find_if(
            target.groups.begin(),
            target.groups.end(),
            [&](const auto& entry) {
                return entry.id == group.id;
            });
        if (existing == target.groups.end()) {
            target.groups.push_back(group);
            continue;
        }
        if (existing->label != group.label ||
            existing->description != group.description) {
            throw std::runtime_error(
                "parameter group metadata conflict for " +
                typeId + "." + group.id + " in " + sourceId);
        }
    }

    for (const auto& parameter : candidate.parameters) {
        const auto existing = std::find_if(
            target.parameters.begin(),
            target.parameters.end(),
            [&](const auto& entry) {
                return entry.id == parameter.id;
            });
        if (existing == target.parameters.end()) {
            target.parameters.push_back(parameter);
            continue;
        }
        // Catalog assets may deliberately select different initial values.
        // Everything else is type-level API and must remain identical.
        if (parameterMetadataJson(*existing) !=
            parameterMetadataJson(parameter)) {
            throw std::runtime_error(
                "parameter metadata conflict for " +
                typeId + "." + parameter.id + " in " + sourceId);
        }
    }
}

ofJson exportConfig(
    const std::string& typeId,
    ofJson config) {
    if (typeId == "media.webcam") {
        config["deferOpen"] = true;
    }
    return config;
}

} // namespace

bool exportBuiltinElementParameterContracts(
    const std::string& outputPath,
    const std::string& layerRoot,
    std::string& error) {
    try {
        LayerFactory factory;
        registerBuiltinElements(factory);
        LayerLibrary library;
        if (!layerRoot.empty() &&
            !library.reload(layerRoot)) {
            throw std::runtime_error(
                "could not load layer catalog: " + layerRoot);
        }
        auto records = factory.typeContracts();
        std::sort(
            records.begin(),
            records.end(),
            [](const auto& left, const auto& right) {
                return left.contract.element.typeId <
                    right.contract.element.typeId;
            });

        ofJson root = {
            {"schemaVersion", 1},
            {"status", "generated-review-required"},
            {"generator", "Synaptome --export-builtin-element-contracts"},
            {"types", ofJson::array()},
        };
        for (const auto& record : records) {
            auto declarations = record.contract.parameters;
            std::string source = "declared";
            if (record.bindingMode ==
                LayerFactory::ParameterBindingMode::
                    LegacySetupAdapter) {
                auto layer = factory.create(
                    record.contract.element.typeId);
                if (!layer) {
                    throw std::runtime_error(
                        "could not construct registered element: " +
                        record.contract.element.typeId);
                }
                layer->configure(exportConfig(
                    record.contract.element.typeId,
                    ofJson::object()));
                declarations =
                    captureLegacyDeclarations(*layer);
                normalizeDynamicOptions(
                    record.contract.element.typeId,
                    declarations);
                source = layerRoot.empty()
                    ? "legacySetupSnapshot"
                    : "legacySetupCatalogUnion";

                for (const auto& entry : library.entries()) {
                    if (entry.type !=
                        record.contract.element.typeId) {
                        continue;
                    }
                    auto configured = factory.create(
                        record.contract.element.typeId);
                    if (!configured) {
                        throw std::runtime_error(
                            "could not construct registered element for " +
                            entry.id);
                    }
                    configured->configure(exportConfig(
                        record.contract.element.typeId,
                        entry.config));
                    auto candidate =
                        captureLegacyDeclarations(*configured);
                    normalizeDynamicOptions(
                        record.contract.element.typeId,
                        candidate);
                    mergeDeclarations(
                        record.contract.element.typeId,
                        entry.id,
                        declarations,
                        candidate);
                }
            }
            normalizeDynamicOptions(
                record.contract.element.typeId,
                declarations);
            root["types"].push_back({
                {"typeId", record.contract.element.typeId},
                {"source", source},
                {"declarations",
                 declarationSetJson(declarations)},
            });
        }

        std::ofstream output(
            outputPath,
            std::ios::binary | std::ios::trunc);
        if (!output) {
            error = "could not open contract output: " + outputPath;
            return false;
        }
        output << root.dump(2) << '\n';
        if (!output.good()) {
            error = "could not write contract output: " + outputPath;
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

bool validateBuiltinElementParameterContracts(
    const std::string& layerRoot,
    std::size_t& validatedTypes,
    std::size_t& validatedAssets,
    std::string& error) {
    validatedTypes = 0;
    validatedAssets = 0;
    try {
        LayerFactory factory;
        registerBuiltinElements(factory);
        LayerLibrary library;
        if (!library.reload(layerRoot)) {
            error = "could not load layer catalog: " + layerRoot;
            return false;
        }

        ParameterRegistry parameters;
        Runtime runtime(factory, parameters);
        std::set<std::string> testedTypes;
        const auto validateRequest =
            [&](const std::string& typeId,
                const std::string& definitionId,
                ofJson config) {
            if (typeId == "media.webcam") {
                config["deferOpen"] = true;
            }
            Runtime::ElementRequest request;
            request.typeId = typeId;
            request.definitionId = definitionId;
            request.instanceId =
                "contract.validation." + typeId;
            request.registryPrefix =
                "__contract_validation__";
            request.config = std::move(config);
            auto prepared = runtime.prepareElement(request);
            if (!prepared) {
                throw std::runtime_error(
                    "contract validation failed for " +
                    definitionId + " (" + typeId + ") at " +
                    prepared.stage + ": " + prepared.error);
            }
            runtime.releasePreparedElement(prepared);
            testedTypes.insert(typeId);
            ++validatedAssets;
        };

        for (const auto& entry : library.entries()) {
            if (!factory.contains(entry.type)) {
                continue;
            }
            validateRequest(
                entry.type,
                entry.id,
                entry.config);
        }
        for (const auto& typeId :
             builtinElementParameterTypeIds()) {
            if (testedTypes.find(typeId) !=
                testedTypes.end()) {
                continue;
            }
            validateRequest(
                typeId,
                "type." + typeId,
                ofJson::object());
        }
        validatedTypes = testedTypes.size();
        if (validatedTypes !=
            builtinElementParameterTypeIds().size()) {
            error =
                "not every generated built-in type was validated";
            return false;
        }
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

} // namespace synaptome::runtime

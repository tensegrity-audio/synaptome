#pragma once

#include <synaptome/element/Parameter.h>
#include <synaptome/element/ParameterBinding.h>

#include "../core/ParameterRegistry.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace synaptome::runtime {

class ElementParameterTable final : public element::ParameterBinder {
public:
    explicit ElementParameterTable(
        const element::ParameterDeclarationSet& declarations) {
        entries_.reserve(declarations.parameters.size());
        for (const auto& declaration : declarations.parameters) {
            entries_.push_back({declaration, {}});
        }
        groups_ = declarations.groups;
    }
    ElementParameterTable(const ElementParameterTable&) = delete;
    ElementParameterTable& operator=(const ElementParameterTable&) = delete;

    void bind(std::string parameterId, float& value) override {
        bindStorage(
            std::move(parameterId),
            element::ParameterKind::Float,
            &value);
    }

    void bind(std::string parameterId, bool& value) override {
        bindStorage(
            std::move(parameterId),
            element::ParameterKind::Bool,
            &value);
    }

    void bind(std::string parameterId, std::string& value) override {
        bindStorage(
            std::move(parameterId),
            element::ParameterKind::String,
            &value);
    }

    void bindLegacyRegistry(
        std::string_view registryPrefix,
        const ParameterRegistry& registry) {
        const std::string prefix =
            std::string(registryPrefix) + ".";
        const auto localId = [&](const std::string& id) {
            if (id.rfind(prefix, 0) != 0 ||
                id.size() <= prefix.size()) {
                bindingError_ =
                    "legacy setup registered a parameter outside its "
                    "declared prefix: " + id;
                return std::string();
            }
            return id.substr(prefix.size());
        };
        for (const auto& parameter : registry.floats()) {
            const auto id = localId(parameter.meta.id);
            if (!id.empty() && parameter.value) {
                bind(id, *parameter.value);
            }
        }
        for (const auto& parameter : registry.bools()) {
            const auto id = localId(parameter.meta.id);
            if (!id.empty() && parameter.value) {
                bind(id, *parameter.value);
            }
        }
        for (const auto& parameter : registry.strings()) {
            const auto id = localId(parameter.meta.id);
            if (!id.empty() && parameter.value) {
                bind(id, *parameter.value);
            }
        }
    }

    std::string contractError() const {
        if (!bindingError_.empty()) {
            return bindingError_;
        }
        for (const auto& entry : entries_) {
            if (std::holds_alternative<std::monostate>(entry.storage)) {
                return "element did not bind declared parameter: " +
                    entry.declaration.id;
            }
        }
        return {};
    }

    void applyDeclarationDefaults() const {
        const auto error = contractError();
        if (!error.empty()) {
            throw std::logic_error(error);
        }
        for (const auto& entry : entries_) {
            switch (entry.declaration.kind) {
            case element::ParameterKind::Float:
                *std::get<float*>(entry.storage) =
                    std::get<float>(entry.declaration.defaultValue);
                break;
            case element::ParameterKind::Bool:
                *std::get<bool*>(entry.storage) =
                    std::get<bool>(entry.declaration.defaultValue);
                break;
            case element::ParameterKind::String:
                *std::get<std::string*>(entry.storage) =
                    std::get<std::string>(
                        entry.declaration.defaultValue);
                break;
            }
        }
    }

    void populate(
        std::string_view registryPrefix,
        ParameterRegistry& registry) const {
        const auto error = contractError();
        if (!error.empty()) {
            throw std::logic_error(error);
        }

        for (const auto& entry : entries_) {
            const auto& declaration = entry.declaration;
            ParameterRegistry::Descriptor descriptor;
            descriptor.label = declaration.label;
            descriptor.group = groupLabel(declaration.groupId);
            descriptor.units = declaration.units;
            descriptor.description = declaration.description;
            if (declaration.range) {
                descriptor.range.min = declaration.range->min;
                descriptor.range.max = declaration.range->max;
                descriptor.range.step =
                    declaration.range->step.value_or(0.0f);
            }
            if (declaration.quickAccessOrder) {
                descriptor.quickAccess = true;
                descriptor.quickAccessOrder =
                    *declaration.quickAccessOrder;
            }

            const std::string id =
                std::string(registryPrefix) + "." + declaration.id;
            switch (declaration.kind) {
            case element::ParameterKind::Float: {
                auto* value = std::get<float*>(entry.storage);
                const float effectiveValue = *value;
                auto& parameter = registry.addFloat(
                    id,
                    value,
                    std::get<float>(declaration.defaultValue),
                    descriptor);
                parameter.meta = descriptor;
                parameter.meta.id = id;
                parameter.baseValue = effectiveValue;
                *value = effectiveValue;
                break;
            }
            case element::ParameterKind::Bool: {
                auto* value = std::get<bool*>(entry.storage);
                const bool effectiveValue = *value;
                auto& parameter = registry.addBool(
                    id,
                    value,
                    std::get<bool>(declaration.defaultValue),
                    descriptor);
                parameter.meta = descriptor;
                parameter.meta.id = id;
                parameter.baseValue = effectiveValue;
                *value = effectiveValue;
                break;
            }
            case element::ParameterKind::String: {
                auto* value = std::get<std::string*>(entry.storage);
                const std::string effectiveValue = *value;
                auto& parameter = registry.addString(
                    id,
                    value,
                    std::get<std::string>(declaration.defaultValue),
                    descriptor);
                parameter.meta = descriptor;
                parameter.meta.id = id;
                parameter.baseValue = effectiveValue;
                *value = effectiveValue;
                break;
            }
            }
        }
    }

private:
    using Storage =
        std::variant<std::monostate, float*, bool*, std::string*>;

    struct Entry {
        element::ParameterDeclaration declaration;
        Storage storage;
    };

    template <typename Pointer>
    void bindStorage(
        std::string parameterId,
        element::ParameterKind kind,
        Pointer value) {
        if (!bindingError_.empty()) {
            return;
        }
        if (parameterId.empty()) {
            bindingError_ =
                "element registered an empty parameter binding ID";
            return;
        }
        for (auto& entry : entries_) {
            if (entry.declaration.id != parameterId) {
                continue;
            }
            if (entry.declaration.kind != kind) {
                bindingError_ =
                    "element registered a parameter binding with the wrong "
                    "kind: " + parameterId;
                return;
            }
            if (!std::holds_alternative<std::monostate>(entry.storage)) {
                bindingError_ =
                    "element registered a duplicate parameter binding: " +
                    parameterId;
                return;
            }
            const auto storageAddress =
                static_cast<const void*>(value);
            for (const auto& existing : entries_) {
                if (std::holds_alternative<std::monostate>(
                        existing.storage)) {
                    continue;
                }
                const void* existingAddress = std::visit(
                    [](const auto& pointer) -> const void* {
                        using PointerType =
                            std::decay_t<decltype(pointer)>;
                        if constexpr (std::is_same_v<
                                          PointerType,
                                          std::monostate>) {
                            return nullptr;
                        } else {
                            return static_cast<const void*>(pointer);
                        }
                    },
                    existing.storage);
                if (existingAddress == storageAddress) {
                    bindingError_ =
                        "element bound one storage address to multiple "
                        "parameters: " + parameterId;
                    return;
                }
            }
            entry.storage = value;
            return;
        }
        bindingError_ =
            "element registered an undeclared parameter binding: " +
            parameterId;
    }

    std::string groupLabel(std::string_view groupId) const {
        for (const auto& group : groups_) {
            if (group.id == groupId) {
                return group.label;
            }
        }
        return std::string(groupId);
    }

    std::vector<element::ParameterGroupDeclaration> groups_;
    std::vector<Entry> entries_;
    std::string bindingError_;
};

} // namespace synaptome::runtime

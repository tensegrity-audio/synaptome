#include "LayerFactory.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {

bool isValidTypeId(std::string_view id) noexcept {
    bool atSegmentStart = true;
    for (const char character : id) {
        if (atSegmentStart) {
            if (character < 'a' || character > 'z') {
                return false;
            }
            atSegmentStart = false;
            continue;
        }
        if (character == '.') {
            atSegmentStart = true;
            continue;
        }
        const bool lowercase =
            character >= 'a' && character <= 'z';
        const bool uppercase =
            character >= 'A' && character <= 'Z';
        const bool digit =
            character >= '0' && character <= '9';
        if (!lowercase && !uppercase && !digit) {
            return false;
        }
    }
    return !id.empty() && !atSegmentStart;
}

bool isValidActionId(std::string_view id) noexcept {
    bool atSegmentStart = true;
    for (const char character : id) {
        if (atSegmentStart) {
            if (character < 'a' || character > 'z') {
                return false;
            }
            atSegmentStart = false;
            continue;
        }
        if (character == '.') {
            atSegmentStart = true;
            continue;
        }
        const bool lowercase =
            character >= 'a' && character <= 'z';
        const bool uppercase =
            character >= 'A' && character <= 'Z';
        const bool digit =
            character >= '0' && character <= '9';
        if (!lowercase && !uppercase && !digit) {
            return false;
        }
    }
    return !id.empty() && !atSegmentStart;
}

bool isValidGroupId(std::string_view id) noexcept {
    if (id.empty() || id.front() < 'a' || id.front() > 'z') {
        return false;
    }
    for (std::size_t i = 1; i < id.size(); ++i) {
        const char character = id[i];
        const bool lowercase =
            character >= 'a' && character <= 'z';
        const bool uppercase =
            character >= 'A' && character <= 'Z';
        const bool digit =
            character >= '0' && character <= '9';
        if (!lowercase && !uppercase && !digit) {
            return false;
        }
    }
    return true;
}

bool isValidDottedLowerCamelId(std::string_view id) noexcept {
    bool atSegmentStart = true;
    for (const char character : id) {
        if (atSegmentStart) {
            if (character < 'a' || character > 'z') {
                return false;
            }
            atSegmentStart = false;
            continue;
        }
        if (character == '.') {
            atSegmentStart = true;
            continue;
        }
        const bool lowercase =
            character >= 'a' && character <= 'z';
        const bool uppercase =
            character >= 'A' && character <= 'Z';
        const bool digit =
            character >= '0' && character <= '9';
        if (!lowercase && !uppercase && !digit) {
            return false;
        }
    }
    return !id.empty() && !atSegmentStart;
}

bool isReservedParameterId(std::string_view id) noexcept {
    return id == "active" || id == "opacity";
}

bool isValidParameterKind(
    synaptome::element::ParameterKind kind) noexcept {
    switch (kind) {
    case synaptome::element::ParameterKind::Float:
    case synaptome::element::ParameterKind::Bool:
    case synaptome::element::ParameterKind::String:
        return true;
    default:
        return false;
    }
}

bool valueMatchesKind(
    const synaptome::element::ParameterValue& value,
    synaptome::element::ParameterKind kind) noexcept {
    switch (kind) {
    case synaptome::element::ParameterKind::Float:
        return std::holds_alternative<float>(value);
    case synaptome::element::ParameterKind::Bool:
        return std::holds_alternative<bool>(value);
    case synaptome::element::ParameterKind::String:
        return std::holds_alternative<std::string>(value);
    default:
        return false;
    }
}

std::string descriptorContractError(
    const synaptome::element::ElementDescriptor& descriptor) {
    if (!isValidTypeId(descriptor.typeId)) {
        return "invalid element type ID: " + descriptor.typeId;
    }
    switch (descriptor.kind) {
    case synaptome::element::ElementKind::Visual:
    case synaptome::element::ElementKind::Effect:
        break;
    default:
        return "invalid element kind for type: " + descriptor.typeId;
    }

    for (std::size_t i = 0; i < descriptor.actions.size(); ++i) {
        const auto& action = descriptor.actions[i];
        if (!isValidActionId(action.id)) {
            return "element descriptor declares an invalid action ID: " +
                action.id;
        }
        if (action.label.empty()) {
            return "element descriptor declares an action with an empty label: " +
                action.id;
        }
        if (!isValidGroupId(action.groupId)) {
            return "element descriptor declares an invalid action group ID: " +
                action.groupId;
        }
        for (std::size_t prior = 0; prior < i; ++prior) {
            if (descriptor.actions[prior].id == action.id) {
                return "element descriptor declares a duplicate action ID: " +
                    action.id;
            }
        }
    }
    return {};
}

std::string parameterContractError(
    const synaptome::element::ParameterDeclarationSet& declarations) {
    std::unordered_set<std::string> groupIds;
    groupIds.reserve(declarations.groups.size());
    for (const auto& group : declarations.groups) {
        if (!isValidGroupId(group.id)) {
            return "element parameter contract declares an invalid group ID: " +
                group.id;
        }
        if (group.label.empty()) {
            return "element parameter contract declares a group with an empty label: " +
                group.id;
        }
        if (!groupIds.insert(group.id).second) {
            return "element parameter contract declares a duplicate group ID: " +
                group.id;
        }
    }

    std::unordered_set<std::string> parameterIds;
    parameterIds.reserve(declarations.parameters.size());
    std::unordered_set<int> quickAccessOrders;
    for (const auto& parameter : declarations.parameters) {
        if (!isValidGroupId(parameter.id)) {
            return "element parameter contract declares an invalid parameter ID: " +
                parameter.id;
        }
        if (isReservedParameterId(parameter.id)) {
            return "element parameter contract declares a layer-container reserved parameter ID: " +
                parameter.id;
        }
        if (!parameterIds.insert(parameter.id).second) {
            return "element parameter contract declares a duplicate parameter ID: " +
                parameter.id;
        }
        if (!isValidParameterKind(parameter.kind)) {
            return "element parameter contract declares an invalid kind: " +
                parameter.id;
        }
        if (parameter.label.empty()) {
            return "element parameter contract declares a parameter with an empty label: " +
                parameter.id;
        }
        if (groupIds.find(parameter.groupId) == groupIds.end()) {
            return "element parameter contract references an undeclared group: " +
                parameter.groupId;
        }
        if (!valueMatchesKind(parameter.defaultValue, parameter.kind)) {
            return "element parameter contract default does not match its kind: " +
                parameter.id;
        }
        if (parameter.quickAccessOrder) {
            if (*parameter.quickAccessOrder < 0) {
                return "element parameter contract declares a negative quick-access order: " +
                    parameter.id;
            }
            if (!quickAccessOrders.insert(
                    *parameter.quickAccessOrder).second) {
                return "element parameter contract declares a duplicate quick-access order: " +
                    std::to_string(*parameter.quickAccessOrder);
            }
        }

        const synaptome::element::ParameterRange* range = nullptr;
        if (parameter.range) {
            if (parameter.kind !=
                synaptome::element::ParameterKind::Float) {
                return "element parameter contract declares a range for a non-float parameter: " +
                    parameter.id;
            }
            range = &*parameter.range;
            if (!std::isfinite(range->min) ||
                !std::isfinite(range->max) ||
                range->min > range->max) {
                return "element parameter contract declares an invalid range: " +
                    parameter.id;
            }
            if (range->step &&
                (!std::isfinite(*range->step) ||
                 *range->step <= 0.0f)) {
                return "element parameter contract declares an invalid range step: " +
                    parameter.id;
            }
        }

        if (parameter.kind ==
            synaptome::element::ParameterKind::Float) {
            const float defaultValue =
                std::get<float>(parameter.defaultValue);
            if (!std::isfinite(defaultValue)) {
                return "element parameter contract declares a non-finite default: " +
                    parameter.id;
            }
            if (range &&
                (defaultValue < range->min ||
                 defaultValue > range->max)) {
                return "element parameter contract default is outside its range: " +
                    parameter.id;
            }
        }

        if (!parameter.options.empty() && parameter.optionSource) {
            return "element parameter contract declares both static options and an option source: " +
                parameter.id;
        }
        if (parameter.optionSource) {
            const auto& source = *parameter.optionSource;
            if (!isValidDottedLowerCamelId(source.id)) {
                return "element parameter contract declares an invalid option-source ID: " +
                    source.id;
            }
            if (source.valueField.empty() ||
                source.labelField.empty()) {
                return "element parameter contract declares an incomplete option source: " +
                    parameter.id;
            }
        }

        for (std::size_t i = 0; i < parameter.options.size(); ++i) {
            const auto& option = parameter.options[i];
            if (!valueMatchesKind(option.value, parameter.kind)) {
                return "element parameter contract option does not match its kind: " +
                    parameter.id;
            }
            if (option.label.empty()) {
                return "element parameter contract declares an option with an empty label: " +
                    parameter.id;
            }
            if (parameter.kind ==
                synaptome::element::ParameterKind::Float) {
                const float value = std::get<float>(option.value);
                if (!std::isfinite(value)) {
                    return "element parameter contract declares a non-finite option: " +
                        parameter.id;
                }
                if (range &&
                    (value < range->min || value > range->max)) {
                    return "element parameter contract option is outside its range: " +
                        parameter.id;
                }
            }
            for (std::size_t prior = 0; prior < i; ++prior) {
                if (parameter.options[prior].value == option.value) {
                    return "element parameter contract declares a duplicate option value: " +
                        parameter.id;
                }
            }
        }
    }

    std::unordered_set<std::string> publicIds = parameterIds;
    for (const auto& parameter : declarations.parameters) {
        for (const auto& alias : parameter.aliases) {
            if (!isValidGroupId(alias)) {
                return "element parameter contract declares an invalid alias: " +
                    alias;
            }
            if (isReservedParameterId(alias)) {
                return "element parameter contract declares a layer-container reserved alias: " +
                    alias;
            }
            if (!publicIds.insert(alias).second) {
                return "element parameter contract declares a duplicate or colliding alias: " +
                    alias;
            }
        }

        if (!parameter.deprecation) {
            continue;
        }
        const auto& deprecation = *parameter.deprecation;
        if (deprecation.reason.empty()) {
            return "element parameter contract declares deprecation without a reason: " +
                parameter.id;
        }
        if (deprecation.replacementId.empty()) {
            continue;
        }
        const bool legacyAlphaToContainerOpacity =
            parameter.id == "alpha" &&
            parameter.kind ==
                synaptome::element::ParameterKind::Float &&
            deprecation.replacementId == "opacity";
        if (legacyAlphaToContainerOpacity) {
            continue;
        }
        if (!isValidGroupId(deprecation.replacementId) ||
            isReservedParameterId(deprecation.replacementId)) {
            return "element parameter contract declares an invalid deprecation replacement: " +
                deprecation.replacementId;
        }
        const auto replacement = std::find_if(
            declarations.parameters.begin(),
            declarations.parameters.end(),
            [&](const auto& candidate) {
                return candidate.id == deprecation.replacementId;
            });
        if (replacement == declarations.parameters.end()) {
            return "element parameter contract deprecation replacement is undeclared: " +
                deprecation.replacementId;
        }
        if (replacement->kind != parameter.kind) {
            return "element parameter contract deprecation replacement has a different kind: " +
                deprecation.replacementId;
        }
        if (replacement->id == parameter.id) {
            return "element parameter contract deprecation cannot replace itself: " +
                parameter.id;
        }
    }

    return {};
}

} // namespace

void LayerFactory::registerType(
    synaptome::element::ElementDescriptor descriptor,
    Creator creator) {
    ElementTypeContractRecord record;
    record.state = ParameterDeclarationState::LegacySetupDiscovery;
    record.contract.element = std::move(descriptor);
    registerTypeRecord(std::move(record), std::move(creator));
}

void LayerFactory::registerType(
    synaptome::element::ElementTypeContract contract,
    Creator creator) {
    ElementTypeContractRecord record;
    record.state = ParameterDeclarationState::Declared;
    record.contract = std::move(contract);
    registerTypeRecord(std::move(record), std::move(creator));
}

void LayerFactory::registerTypeRecord(
    ElementTypeContractRecord record,
    Creator creator) {
    const auto contractError =
        descriptorContractError(record.contract.element);
    if (!contractError.empty()) {
        throw std::invalid_argument(
            "LayerFactory::registerType " + contractError);
    }
    if (record.state == ParameterDeclarationState::Declared) {
        const auto declarationError =
            parameterContractError(record.contract.parameters);
        if (!declarationError.empty()) {
            throw std::invalid_argument(
                "LayerFactory::registerType " + declarationError);
        }
    } else if (
        record.state !=
            ParameterDeclarationState::LegacySetupDiscovery) {
        throw std::invalid_argument(
            "LayerFactory::registerType invalid parameter declaration state");
    }
    if (!creator) {
        throw std::invalid_argument(
            "LayerFactory::registerType requires valid creator");
    }
    if (contains(record.contract.element.typeId)) {
        throw std::logic_error(
            "LayerFactory::registerType duplicate type: " +
            record.contract.element.typeId);
    }

    registrations_.push_back({
        std::move(record),
        std::move(creator),
    });
}

bool LayerFactory::contains(std::string_view typeId) const noexcept {
    return registration(typeId) != nullptr;
}

const synaptome::element::ElementDescriptor*
LayerFactory::descriptor(std::string_view typeId) const noexcept {
    const auto* found = registration(typeId);
    return found ? &found->typeContract.contract.element : nullptr;
}

std::vector<synaptome::element::ElementDescriptor>
LayerFactory::descriptors() const {
    std::vector<synaptome::element::ElementDescriptor> result;
    result.reserve(registrations_.size());
    for (const auto& entry : registrations_) {
        result.push_back(entry.typeContract.contract.element);
    }
    return result;
}

const LayerFactory::ElementTypeContractRecord*
LayerFactory::typeContract(std::string_view typeId) const noexcept {
    const auto* found = registration(typeId);
    return found ? &found->typeContract : nullptr;
}

std::vector<LayerFactory::ElementTypeContractRecord>
LayerFactory::typeContracts() const {
    std::vector<ElementTypeContractRecord> result;
    result.reserve(registrations_.size());
    for (const auto& entry : registrations_) {
        result.push_back(entry.typeContract);
    }
    return result;
}

std::unique_ptr<Layer> LayerFactory::create(
    std::string_view typeId) const {
    const auto* found = registration(typeId);
    if (!found) {
        return nullptr;
    }
    return found->creator();
}

const LayerFactory::Registration* LayerFactory::registration(
    std::string_view typeId) const noexcept {
    for (const auto& entry : registrations_) {
        if (entry.typeContract.contract.element.typeId == typeId) {
            return &entry;
        }
    }
    return nullptr;
}

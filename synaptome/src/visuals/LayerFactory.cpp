#include "LayerFactory.h"

#include <cstddef>
#include <stdexcept>
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

} // namespace

void LayerFactory::registerType(
    synaptome::element::ElementDescriptor descriptor,
    Creator creator) {
    const auto contractError = descriptorContractError(descriptor);
    if (!contractError.empty()) {
        throw std::invalid_argument(
            "LayerFactory::registerType " + contractError);
    }
    if (!creator) {
        throw std::invalid_argument(
            "LayerFactory::registerType requires valid creator");
    }
    if (contains(descriptor.typeId)) {
        throw std::logic_error(
            "LayerFactory::registerType duplicate type: " +
            descriptor.typeId);
    }

    registrations_.push_back({
        std::move(descriptor),
        std::move(creator),
    });
}

bool LayerFactory::contains(std::string_view typeId) const noexcept {
    return registration(typeId) != nullptr;
}

const synaptome::element::ElementDescriptor*
LayerFactory::descriptor(std::string_view typeId) const noexcept {
    const auto* found = registration(typeId);
    return found ? &found->descriptor : nullptr;
}

std::vector<synaptome::element::ElementDescriptor>
LayerFactory::descriptors() const {
    std::vector<synaptome::element::ElementDescriptor> result;
    result.reserve(registrations_.size());
    for (const auto& entry : registrations_) {
        result.push_back(entry.descriptor);
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
        if (entry.descriptor.typeId == typeId) {
            return &entry;
        }
    }
    return nullptr;
}

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace synaptome::element {

struct ActionDescriptor {
    std::string id;
    std::string label;
    std::string groupId;
    std::string description;
};

enum class ElementKind : std::uint8_t {
    Visual,
    Effect,
};

struct ElementDescriptor {
    std::string typeId;
    ElementKind kind = ElementKind::Visual;
    std::vector<ActionDescriptor> actions;
};

} // namespace synaptome::element

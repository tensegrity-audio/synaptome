#pragma once

#include <synaptome/element/ElementDescriptor.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace synaptome::element {

enum class ParameterKind : std::uint8_t {
    Float,
    Bool,
    String,
};

using ParameterValue = std::variant<float, bool, std::string>;

struct ParameterRange {
    float min = 0.0f;
    float max = 0.0f;
    std::optional<float> step;
};

struct ParameterOption {
    ParameterValue value;
    std::string label;
    std::string description;
};

struct ParameterOptionSource {
    std::string id;
    std::string valueField;
    std::string labelField;
};

struct ParameterGroupDeclaration {
    std::string id;
    std::string label;
    std::string description;
};

struct ParameterDeprecation {
    std::string replacementId;
    std::string reason;
};

struct ParameterDeclaration {
    std::string id;
    ParameterKind kind = ParameterKind::Float;
    std::string groupId;
    std::string label;
    ParameterValue defaultValue = 0.0f;
    std::optional<ParameterRange> range;
    std::string units;
    std::string description;
    std::vector<ParameterOption> options;
    std::optional<ParameterOptionSource> optionSource;
    std::optional<int> quickAccessOrder;
    std::vector<std::string> aliases;
    std::optional<ParameterDeprecation> deprecation;
    bool visible = true;
};

struct ParameterDeclarationSet {
    std::vector<ParameterGroupDeclaration> groups;
    std::vector<ParameterDeclaration> parameters;
};

struct ElementTypeContract {
    ElementDescriptor element;
    ParameterDeclarationSet parameters;
};

} // namespace synaptome::element

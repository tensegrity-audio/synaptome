#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace synaptome::state {

enum class ParameterBaseOriginKind {
    ElementDefault,
    DefinitionDefault,
    Preset,
    ActivationOverride,
    Scene,
    OperatorEdit,
};

inline const char* parameterBaseOriginKindName(
    ParameterBaseOriginKind kind) noexcept {
    switch (kind) {
    case ParameterBaseOriginKind::ElementDefault:
        return "element-default";
    case ParameterBaseOriginKind::DefinitionDefault:
        return "definition-default";
    case ParameterBaseOriginKind::Preset:
        return "preset";
    case ParameterBaseOriginKind::ActivationOverride:
        return "activation-override";
    case ParameterBaseOriginKind::Scene:
        return "scene";
    case ParameterBaseOriginKind::OperatorEdit:
        return "operator-edit";
    }
    return "element-default";
}

struct ParameterBaseOrigin {
    ParameterBaseOriginKind kind =
        ParameterBaseOriginKind::ElementDefault;
    std::string originId;
    int artifactVersion = 0;
    std::vector<std::string> migrationTrail;
    std::string artifactRevision;
};

inline bool operator==(
    const ParameterBaseOrigin& lhs,
    const ParameterBaseOrigin& rhs) {
    return lhs.kind == rhs.kind &&
           lhs.originId == rhs.originId &&
           lhs.artifactVersion == rhs.artifactVersion &&
           lhs.migrationTrail == rhs.migrationTrail &&
           lhs.artifactRevision == rhs.artifactRevision;
}

inline bool operator!=(
    const ParameterBaseOrigin& lhs,
    const ParameterBaseOrigin& rhs) {
    return !(lhs == rhs);
}

using ParameterBaseOrigins =
    std::unordered_map<std::string, ParameterBaseOrigin>;

enum class ParameterValueKind {
    Float,
    Bool,
    String,
};

using ParameterValue =
    std::variant<float, bool, std::string>;

struct ParameterModifierOriginSnapshot {
    std::size_t evaluationIndex = 0;
    std::string ownerTag;
    bool active = false;
    bool applied = false;
};

struct ParameterValueSnapshot {
    std::string id;
    ParameterValueKind kind = ParameterValueKind::Float;
    ParameterValue baseValue = 0.0f;
    ParameterValue liveValue = 0.0f;
    ParameterBaseOrigin baseOrigin;
    std::vector<ParameterModifierOriginSnapshot> modifiers;
};

}  // namespace synaptome::state

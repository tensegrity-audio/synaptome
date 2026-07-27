#pragma once

#include <cstddef>
#include <string>

namespace synaptome::runtime {

inline constexpr std::size_t kCompositionLayerCount = 8;

struct CompositionCoverage {
    bool defined = false;
    std::string mode = "upstream";
    int columns = 0;
};

enum class CompositionKind {
    Element,
    Effect,
    Overlay,
};

struct CompositionAssignment {
    CompositionKind kind = CompositionKind::Element;
    std::string definitionId;
    std::string label;
    std::string typeId;
    std::string registryPrefix;
    bool active = false;
    float opacity = 1.0f;
    CompositionCoverage coverage;
};

enum class CompositionMutationError {
    None,
    IndexOutOfRange,
    InvalidAssignment,
    KindMismatch,
    ElementMismatch,
    LifecycleFailure,
};

struct CompositionMutationResult {
    CompositionMutationError errorCode = CompositionMutationError::None;
    bool elementChanged = false;
    bool parametersChanged = false;
    std::string error;

    explicit operator bool() const noexcept {
        return errorCode == CompositionMutationError::None;
    }
};

struct CompositionCoverageWindow {
    std::size_t effectLayerIndex = 0;
    std::size_t firstInputLayerIndex = 0;
    std::size_t inputEndLayerIndex = 0;
    int requestedLayers = 0;
    bool includesAllPrior = false;

    bool contains(std::size_t layerIndex) const noexcept {
        return layerIndex >= firstInputLayerIndex &&
            layerIndex < inputEndLayerIndex;
    }
};

} // namespace synaptome::runtime

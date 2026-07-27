#pragma once

#include <synaptome/element/Action.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

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

struct CompositionLayerSnapshot {
    std::size_t zeroBasedIndex = 0;
    bool occupied = false;
    bool hasElement = false;
    CompositionKind kind = CompositionKind::Element;
    std::string definitionId;
    std::string label;
    std::string typeId;
    std::string registryPrefix;
    bool active = false;
    float opacity = 1.0f;
    CompositionCoverage coverage;
    std::vector<element::ActionDescriptor> actions;
};

struct CompositionSnapshot {
    std::array<CompositionLayerSnapshot, kCompositionLayerCount> layers;
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

enum class CompositionActionError {
    None,
    IndexOutOfRange,
    SlotEmpty,
    KindMismatch,
    ActionNotFound,
    Rejected,
    ExecutionFailure,
};

struct CompositionActionResult {
    CompositionActionError errorCode = CompositionActionError::None;
    std::string actionId;
    std::string error;

    explicit operator bool() const noexcept {
        return errorCode == CompositionActionError::None;
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

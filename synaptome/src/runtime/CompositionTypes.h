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

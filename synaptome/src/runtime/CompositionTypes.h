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

} // namespace synaptome::runtime

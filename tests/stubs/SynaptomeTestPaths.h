#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace synaptome_test_paths {
namespace detail {
inline bool isAppRoot(const std::filesystem::path& candidate) {
    std::error_code ec;
    return std::filesystem::exists(candidate / "src" / "ui" / "ControlMappingHubState.h", ec) &&
           std::filesystem::exists(candidate / "bin" / "data" / "device_maps", ec);
}

inline std::filesystem::path normalize(const std::filesystem::path& path) {
    std::error_code ec;
    auto absolute = std::filesystem::absolute(path, ec);
    if (ec) {
        absolute = path;
    }
    auto canonical = std::filesystem::weakly_canonical(absolute, ec);
    return ec ? absolute.lexically_normal() : canonical;
}

inline void addCandidateWithParents(std::vector<std::filesystem::path>& candidates,
                                    const std::filesystem::path& seed) {
    if (seed.empty()) {
        return;
    }
    auto current = normalize(seed);
    for (;;) {
        candidates.push_back(current);
        candidates.push_back(current / "openFrameworks_v0_1");
        candidates.push_back(current / "synaptome");
        if (!current.has_parent_path() || current.parent_path() == current) {
            break;
        }
        current = current.parent_path();
    }
}
} // namespace detail

inline std::filesystem::path appRoot() {
    static const std::filesystem::path root = [] {
        std::vector<std::filesystem::path> candidates;
        for (const char* name : {"SYNAPTOME_APP_ROOT", "OPENFRAMEWORKS_APP_ROOT", "TENSEGRITY_APP_ROOT"}) {
            if (const char* value = std::getenv(name)) {
                detail::addCandidateWithParents(candidates, value);
            }
        }
        detail::addCandidateWithParents(candidates, std::filesystem::current_path());
        detail::addCandidateWithParents(candidates, std::filesystem::path(__FILE__).parent_path());

        for (const auto& candidate : candidates) {
            if (detail::isAppRoot(candidate)) {
                return detail::normalize(candidate);
            }
        }

        return detail::normalize(std::filesystem::current_path() / "synaptome");
    }();
    return root;
}

inline std::filesystem::path dataRoot() {
    return appRoot() / "bin" / "data";
}

inline std::filesystem::path deviceMapsRoot() {
    return dataRoot() / "device_maps";
}
} // namespace synaptome_test_paths

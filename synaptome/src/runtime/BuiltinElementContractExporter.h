#pragma once

#include <cstddef>
#include <string>

namespace synaptome::runtime {

// Developer-only migration/export command. It constructs each registered
// built-in once under a live openFrameworks context and snapshots the legacy
// setup registry into pure parameter declarations. Normal runtime startup
// never calls this path.
bool exportBuiltinElementParameterContracts(
    const std::string& outputPath,
    const std::string& layerRoot,
    std::string& error);

bool validateBuiltinElementParameterContracts(
    const std::string& layerRoot,
    std::size_t& validatedTypes,
    std::size_t& validatedAssets,
    std::string& error);

} // namespace synaptome::runtime

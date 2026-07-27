#pragma once

class ParameterRegistry;

namespace synaptome::runtime {

// Registers host-owned controls that remain available even when their
// corresponding built-in element has not been assigned to a composition layer.
void registerBuiltinElementHostParameters(ParameterRegistry& registry);

// Synchronizes derived compatibility values owned by the host bindings.
void updateBuiltinElementHostParameters();

} // namespace synaptome::runtime

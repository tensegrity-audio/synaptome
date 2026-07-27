#pragma once

class LayerFactory;

namespace synaptome::runtime {

// Controlled static-registration entrypoint. This file is intentionally
// runtime-owned; element targets do not depend on LayerFactory.
void registerBuiltinElements(LayerFactory& factory);

} // namespace synaptome::runtime

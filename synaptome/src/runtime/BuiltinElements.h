#pragma once

class LayerFactory;

namespace synaptome::runtime {

// Controlled host registration entrypoint. This file is intentionally
// host-owned; element targets do not depend on LayerFactory.
void registerBuiltinElements(LayerFactory& elementTypes);

} // namespace synaptome::runtime

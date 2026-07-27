#pragma once

class LayerFactory;

namespace synaptome::runtime {

// Narrow host/bench registration bridge for the shipping example element.
// This is an app integration seam, not a public Element SDK or plug-in ABI.
void registerSignalBloomElement(LayerFactory& elementTypes);

} // namespace synaptome::runtime

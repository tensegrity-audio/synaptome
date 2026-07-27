#include "BuiltinElements.h"

#include "../visuals/LayerFactory.h"
#include "../visuals/SignalBloomLayer.h"

#include <memory>

namespace synaptome::runtime {

void registerBuiltinElements(LayerFactory& elementTypes) {
    elementTypes.registerType("example.signalBloom", []() {
        return std::make_unique<SignalBloomLayer>();
    });
}

} // namespace synaptome::runtime

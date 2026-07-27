#include "SignalBloomRegistration.h"

#include "../visuals/LayerFactory.h"
#include "../visuals/SignalBloomLayer.h"

#include <memory>

namespace synaptome::runtime {

void registerSignalBloomElement(LayerFactory& elementTypes) {
    elementTypes.registerType("example.signalBloom", []() {
        return std::make_unique<SignalBloomLayer>();
    });
}

} // namespace synaptome::runtime

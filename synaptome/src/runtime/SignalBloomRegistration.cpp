#include "SignalBloomRegistration.h"

#include "../visuals/LayerFactory.h"
#include "../visuals/SignalBloomLayer.h"

#include <synaptome/element/ElementDescriptor.h>

#include <memory>

namespace synaptome::runtime {

void registerSignalBloomElement(LayerFactory& elementTypes) {
    elementTypes.registerType(
        {
            "example.signalBloom",
            synaptome::element::ElementKind::Visual,
            {},
        },
        []() {
            return std::make_unique<SignalBloomLayer>();
        });
}

} // namespace synaptome::runtime

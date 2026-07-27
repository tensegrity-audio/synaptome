#include "SignalBloomLayer.h"
#include "../../../synaptome/src/visuals/LayerFactory.h"

#include <synaptome/element/ElementDescriptor.h>

#include <memory>

void registerSignalBloomLayer(LayerFactory& factory) {
    factory.registerType(
        {
            "example.signalBloom",
            synaptome::element::ElementKind::Visual,
            {},
        },
        []() {
            return std::make_unique<SignalBloomLayer>();
        });
}

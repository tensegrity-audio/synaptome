#include "SignalBloomLayer.h"
#include "../../../synaptome/src/visuals/LayerFactory.h"
#include <memory>

void registerSignalBloomLayer(LayerFactory& factory) {
    factory.registerType("example.signalBloom", []() {
        return std::make_unique<SignalBloomLayer>();
    });
}

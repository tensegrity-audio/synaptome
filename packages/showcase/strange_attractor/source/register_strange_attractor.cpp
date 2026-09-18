#include "StrangeAttractorLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_strange_attractor() {
    return std::make_unique<StrangeAttractorLayer>();
}

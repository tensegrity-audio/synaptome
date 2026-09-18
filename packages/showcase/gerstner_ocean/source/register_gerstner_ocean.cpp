#include "GerstnerOceanLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_gerstner_ocean() {
    return std::make_unique<GerstnerOceanLayer>();
}

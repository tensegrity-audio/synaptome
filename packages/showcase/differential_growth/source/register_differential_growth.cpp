#include "DifferentialGrowthLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_differential_growth() {
    return std::make_unique<DifferentialGrowthLayer>();
}

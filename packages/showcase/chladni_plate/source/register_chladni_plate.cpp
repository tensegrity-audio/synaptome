#include "ChladniPlateLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_chladni_plate() {
    return std::make_unique<ChladniPlateLayer>();
}

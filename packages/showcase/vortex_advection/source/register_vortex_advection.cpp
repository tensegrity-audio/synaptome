#include "VortexAdvectionLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_vortex_advection() {
    return std::make_unique<VortexAdvectionLayer>();
}

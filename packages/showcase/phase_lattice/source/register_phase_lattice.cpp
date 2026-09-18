#include "PhaseLatticeLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_phase_lattice() {
    return std::make_unique<PhaseLatticeLayer>();
}

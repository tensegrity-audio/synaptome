#include "MagneticDipolesLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_magnetic_dipoles() {
    return std::make_unique<MagneticDipolesLayer>();
}

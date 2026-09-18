#include "DendriticCrystalLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_dendritic_crystal() {
    return std::make_unique<DendriticCrystalLayer>();
}

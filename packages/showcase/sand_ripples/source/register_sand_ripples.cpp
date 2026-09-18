#include "SandRipplesLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_sand_ripples() {
    return std::make_unique<SandRipplesLayer>();
}

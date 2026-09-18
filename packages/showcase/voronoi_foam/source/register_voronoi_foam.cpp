#include "VoronoiFoamLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_voronoi_foam() {
    return std::make_unique<VoronoiFoamLayer>();
}

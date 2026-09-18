#include "ElasticLatticeLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_elastic_lattice() {
    return std::make_unique<ElasticLatticeLayer>();
}

#include "WaveTankLayer.h"
#include <memory>

std::unique_ptr<Layer> synaptomeCreateElementPackage_show_wave_tank() {
    return std::make_unique<WaveTankLayer>();
}

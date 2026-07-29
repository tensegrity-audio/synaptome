#include "SignalBloomLayer.h"

#include <memory>

std::unique_ptr<Layer>
synaptomeCreateElementPackage_examples_signal_bloom() {
    return std::make_unique<SignalBloomLayer>();
}

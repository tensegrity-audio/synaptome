#pragma once

#include "CompositionTypes.h"
#include "ElementActionTable.h"

#include <synaptome/element/compat/Layer.h>

#include <cstdint>
#include <memory>
#include <string>

namespace synaptome::runtime {

class Runtime;

class CompositionLayer {
public:
    Layer* element() { return element_.get(); }
    const Layer* element() const { return element_.get(); }
    bool hasElement() const { return element_ != nullptr; }

    std::string assetId;
    std::string label;
    std::string type;
    std::string paramPrefix;
    CompositionKind kind = CompositionKind::Element;
    bool active = false;
    float opacity = 1.0f;
    CompositionCoverage coverage;
    ofFbo layerFbo;
    ofFbo upstreamFbo;
    ofFbo effectFbo;

private:
    friend class Runtime;
    std::unique_ptr<Layer> element_;
    // Declared after element_ so handlers are destroyed before the element
    // they may capture.
    ElementActionTable actions_;
    std::string opacityParameterId_;
    // Invalidates prepared replacement tokens even if an allocator later
    // reuses the retired element's address.
    std::uint64_t elementRevision_ = 0;
};

} // namespace synaptome::runtime

#pragma once

#include "CompositionTypes.h"

#include <synaptome/element/compat/Layer.h>

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
    bool active = false;
    float opacity = 1.0f;
    CompositionCoverage coverage;
    float coverageParamValue = 0.0f;
    ofFbo layerFbo;
    ofFbo upstreamFbo;
    ofFbo effectFbo;

private:
    friend class Runtime;
    std::unique_ptr<Layer> element_;
};

} // namespace synaptome::runtime

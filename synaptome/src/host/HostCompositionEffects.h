#pragma once

#include <string_view>

class ofFbo;

namespace synaptome::host {

class HostCompositionEffects {
public:
    virtual ~HostCompositionEffects() = default;

    virtual bool isConsoleRouted(
        std::string_view effectType) const noexcept = 0;
    virtual float defaultCoverageForType(
        std::string_view effectType) const noexcept = 0;
    virtual bool applySlot(
        std::string_view effectType,
        const ofFbo& source,
        ofFbo& destination) = 0;
    virtual void applyGlobal(ofFbo& composite) = 0;
};

} // namespace synaptome::host

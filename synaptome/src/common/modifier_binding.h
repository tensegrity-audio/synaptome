#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "modifier.h"

namespace modifier {

enum class SourceType : uint8_t {
    kUnknown = 0,
    kKey,
    kMidiCc,
    kMidiNote,
    kOsc
};

struct BindingMetadata {
    SourceType sourceType = SourceType::kUnknown;
    int midiChannel = 0;
    int midiControl = -1;  // CC or note number
    std::string oscAddress;
    std::string label;
    bool snapInteger = false;
    float stepSize = 0.0f;
    float smoothing = 0.0f;
    float deadband = 0.0f;
    bool toggleLatch = false;
};

struct Binding {
    std::string bankId;
    std::string targetId;
    Modifier modifier;
    BindingMetadata meta;
};

using BindingList = std::vector<Binding>;

}  // namespace modifier

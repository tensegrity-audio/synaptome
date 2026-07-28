#pragma once

#include "ofJson.h"

#include <cstddef>
#include <string>

namespace synaptome::state {

constexpr int kCurrentMachineProfileSchemaVersion = 1;
constexpr std::size_t kMaxMachineProfileAssignmentKeyLength = 255;
constexpr std::size_t kMaxMachineProfileBindingIdLength = 255;

enum class MachineProfileDocumentKind {
    CurrentV1,
};

enum class MachineProfileDocumentError {
    None,
    InvalidDocument,
    UnsupportedFutureVersion,
};

struct MachineProfileDocumentResult {
    bool ok = false;
    MachineProfileDocumentKind kind =
        MachineProfileDocumentKind::CurrentV1;
    MachineProfileDocumentError errorCode =
        MachineProfileDocumentError::None;
    int sourceVersion = 0;
    ofJson document;
    std::string error;
};

// Validates OSC endpoint selection, optional exact physical MIDI input
// binding, and optional logical control-slot assignments without changing the
// source value or touching live/persisted machine state. Machine profile is a
// new artifact: there is no versionless aggregate compatibility shape, and
// writers emit exactly schemaVersion 1.
MachineProfileDocumentResult validateMachineProfileDocument(
    const ofJson& source);

} // namespace synaptome::state

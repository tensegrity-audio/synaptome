#pragma once

#include "ofJson.h"

#include <string>

namespace synaptome::state {

constexpr int kCurrentMappingBankSchemaVersion = 1;

enum class MappingBankDocumentKind {
    LegacyUnversioned,
    CurrentV1,
};

enum class MappingBankDocumentError {
    None,
    InvalidDocument,
    UnsupportedFutureVersion,
};

struct MappingBankDocumentResult {
    bool ok = false;
    MappingBankDocumentKind kind =
        MappingBankDocumentKind::LegacyUnversioned;
    MappingBankDocumentError errorCode =
        MappingBankDocumentError::None;
    // Zero identifies the legacy versionless runtime snapshot.
    int sourceVersion = 0;
    bool migratedInMemory = false;
    ofJson document;
    std::string error;
};

// Classifies and normalizes an actual MidiRouter snapshot without changing
// the source or touching live/persisted routes. Missing schemaVersion is the
// legacy compatibility shape. Writers emit only mapping-bank v1.
//
// A null snapshot remains the legacy explicit-empty representation accepted
// by MidiRouter::importMappingSnapshot.
MappingBankDocumentResult normalizeMappingBankDocument(
    const ofJson& source);

} // namespace synaptome::state

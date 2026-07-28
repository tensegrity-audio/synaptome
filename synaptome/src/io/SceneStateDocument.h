#pragma once

#include "ofJson.h"

#include <string>

namespace synaptome::state {

constexpr int kLegacySceneSchemaVersion = 1;
constexpr int kCurrentSceneSchemaVersion = 2;

enum class SceneDocumentKind {
    LegacyV1,
    CurrentV2,
};

enum class SceneDocumentError {
    None,
    InvalidDocument,
    UnsupportedFutureVersion,
};

struct SceneDocumentResult {
    bool ok = false;
    SceneDocumentKind kind = SceneDocumentKind::LegacyV1;
    SceneDocumentError errorCode = SceneDocumentError::None;
    int sourceVersion = 0;
    bool migratedInMemory = false;
    ofJson document;
    std::string error;
};

// Classifies and normalizes a parsed scene without changing the source value
// or touching live/persisted runtime state. Missing schemaVersion is the
// legacy v1 compatibility shape. Writers emit only current v2.
SceneDocumentResult normalizeSceneDocument(const ofJson& source);

} // namespace synaptome::state

#include "SceneStateDocument.h"

#include <utility>

namespace synaptome::state {
namespace {

SceneDocumentResult failure(
    std::string error,
    SceneDocumentError code = SceneDocumentError::InvalidDocument) {
    SceneDocumentResult result;
    result.errorCode = code;
    result.error = std::move(error);
    return result;
}

} // namespace

SceneDocumentResult normalizeSceneDocument(const ofJson& source) {
    if (!source.is_object()) {
        return failure("scene document must be an object");
    }

    SceneDocumentResult result;
    result.document = source;

    int sourceVersion = kLegacySceneSchemaVersion;
    if (source.contains("scene")) {
        if (!source["scene"].is_object()) {
            return failure("scene metadata must be an object");
        }
        const auto& metadata = source["scene"];
        if (metadata.contains("schemaVersion")) {
            if (!metadata["schemaVersion"].is_number_integer()) {
                return failure("scene schemaVersion must be an integer");
            }
            sourceVersion = metadata["schemaVersion"].get<int>();
        }
    }

    if (sourceVersion < kLegacySceneSchemaVersion) {
        return failure("scene schemaVersion must be positive");
    }
    if (sourceVersion > kCurrentSceneSchemaVersion) {
        return failure(
            "unsupported future scene schemaVersion " +
                std::to_string(sourceVersion),
            SceneDocumentError::UnsupportedFutureVersion);
    }

    result.sourceVersion = sourceVersion;
    if (sourceVersion == kCurrentSceneSchemaVersion) {
        result.kind = SceneDocumentKind::CurrentV2;
        result.ok = true;
        return result;
    }

    result.kind = SceneDocumentKind::LegacyV1;
    result.migratedInMemory = true;
    if (!result.document.contains("scene")) {
        result.document["scene"] = ofJson::object();
    }
    result.document["scene"]["schemaVersion"] = kCurrentSceneSchemaVersion;
    result.ok = true;
    return result;
}

} // namespace synaptome::state

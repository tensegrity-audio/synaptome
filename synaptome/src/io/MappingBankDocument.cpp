#include "MappingBankDocument.h"

#include <utility>

namespace synaptome::state {
namespace {

MappingBankDocumentResult mappingFailure(
    std::string error,
    MappingBankDocumentError code =
        MappingBankDocumentError::InvalidDocument) {
    MappingBankDocumentResult result;
    result.errorCode = code;
    result.error = std::move(error);
    return result;
}

} // namespace

MappingBankDocumentResult normalizeMappingBankDocument(
    const ofJson& source) {
    MappingBankDocumentResult result;
    if (source.is_null()) {
        result.document = ofJson::object();
    } else if (source.is_object()) {
        result.document = source;
    } else {
        return mappingFailure(
            "mapping-bank document must be an object");
    }

    if (!result.document.contains("schemaVersion")) {
        // The public {version, bank, mappings} Browser example is a distinct
        // interchange artifact, not a legacy MidiRouter snapshot.
        if (result.document.contains("bank") ||
            result.document.contains("mappings")) {
            return mappingFailure(
                "public MIDI interchange documents are not runtime "
                "mapping-bank snapshots");
        }
        result.kind =
            MappingBankDocumentKind::LegacyUnversioned;
        result.migratedInMemory = true;
        result.document["schemaVersion"] =
            kCurrentMappingBankSchemaVersion;
        result.ok = true;
        return result;
    }

    const auto& version = result.document["schemaVersion"];
    if (!version.is_number_integer()) {
        return mappingFailure(
            "mapping-bank schemaVersion must be an integer");
    }
    result.sourceVersion = version.get<int>();
    if (result.sourceVersion < 1) {
        return mappingFailure(
            "mapping-bank schemaVersion must be positive");
    }
    if (result.sourceVersion >
        kCurrentMappingBankSchemaVersion) {
        return mappingFailure(
            "unsupported future mapping-bank schemaVersion " +
                std::to_string(result.sourceVersion),
            MappingBankDocumentError::UnsupportedFutureVersion);
    }
    if (result.document.contains("bank") ||
        result.document.contains("mappings")) {
        return mappingFailure(
            "public MIDI interchange documents are not runtime "
            "mapping-bank snapshots");
    }

    result.kind = MappingBankDocumentKind::CurrentV1;
    result.ok = true;
    return result;
}

} // namespace synaptome::state

#pragma once

#include "ofJson.h"

#include <functional>
#include <string>

namespace synaptome::state {

constexpr int kCurrentPreferencesSchemaVersion = 1;

enum class PreferencesDocumentKind {
    LegacyControlHub,
    CurrentV1,
};

enum class PreferencesDocumentError {
    None,
    InvalidDocument,
    UnsupportedFutureVersion,
};

struct PreferencesDocumentResult {
    bool ok = false;
    PreferencesDocumentKind kind =
        PreferencesDocumentKind::LegacyControlHub;
    PreferencesDocumentError errorCode =
        PreferencesDocumentError::None;
    int sourceVersion = 0;
    bool migratedInMemory = false;
    ofJson document;
    std::string error;
};

// Missing schemaVersion is the legacy control_hub_prefs.json compatibility
// shape. Current documents are strict and section-authoritative: a present
// section owns that preference lane, while an omitted section delegates to
// its legacy reader.
PreferencesDocumentResult normalizePreferencesDocument(
    const ofJson& source);

// Projects canonical Browser/HUD sections into the existing unversioned
// ControlMappingHub reader shape. Missing sections stay omitted.
ofJson controlHubCompatibilityView(const ofJson& canonicalDocument);

struct PreferencesPublishResult {
    bool ok = false;
    bool rollbackSucceeded = false;
    ofJson document;
    std::string error;
};

// A small in-memory transaction seam. Publication validates a complete
// candidate, persists it first, then adopts it. If adoption fails, the prior
// persisted and adopted snapshots are restored.
class PreferencesPublisher {
public:
    using PersistCallback = std::function<bool(const ofJson&)>;
    using AdoptCallback = std::function<bool(const ofJson&)>;

    bool adoptInitial(const ofJson& source, std::string* error = nullptr);
    const ofJson& snapshot() const { return snapshot_; }

    void setPersistCallback(PersistCallback callback) {
        persist_ = std::move(callback);
    }
    void setAdoptCallback(AdoptCallback callback) {
        adopt_ = std::move(callback);
    }

    PreferencesPublishResult publish(const ofJson& candidate);
    PreferencesPublishResult publishSection(
        const std::string& section,
        const ofJson& value);

private:
    ofJson snapshot_ = {
        {"schemaVersion", kCurrentPreferencesSchemaVersion}
    };
    PersistCallback persist_;
    AdoptCallback adopt_;
};

} // namespace synaptome::state

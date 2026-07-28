#pragma once

#include "ofJson.h"

#include <functional>
#include <string>

namespace synaptome::state {

constexpr int kCurrentBankDefinitionsSchemaVersion = 1;

enum class BankDefinitionsDocumentError {
    None,
    InvalidDocument,
    UnsupportedFutureVersion,
};

struct BankDefinitionsDocumentResult {
    bool ok = false;
    BankDefinitionsDocumentError errorCode =
        BankDefinitionsDocumentError::None;
    int sourceVersion = 0;
    ofJson document;
    std::string error;
};

// Bank definitions are a new operator mapping-store artifact. There is no
// versionless aggregate shape. Legacy Scene banks remain a read-only
// compatibility input when this canonical document is absent.
BankDefinitionsDocumentResult validateBankDefinitionsDocument(
    const ofJson& source);

struct BankDefinitionsPublishResult {
    bool ok = false;
    bool rollbackSucceeded = false;
    ofJson document;
    std::string error;
};

class BankDefinitionsPublisher {
public:
    using Callback = std::function<bool(const ofJson&)>;

    bool adoptInitial(const ofJson& source, std::string* error = nullptr);
    const ofJson& snapshot() const { return snapshot_; }
    void setPersistCallback(Callback callback) {
        persist_ = std::move(callback);
    }
    void setAdoptCallback(Callback callback) {
        adopt_ = std::move(callback);
    }
    BankDefinitionsPublishResult publish(const ofJson& candidate);

private:
    ofJson snapshot_ = {
        {"schemaVersion", kCurrentBankDefinitionsSchemaVersion},
        {"globalBanks", ofJson::array()}
    };
    Callback persist_;
    Callback adopt_;
};

} // namespace synaptome::state

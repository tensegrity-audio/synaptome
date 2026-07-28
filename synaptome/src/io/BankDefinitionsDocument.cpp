#include "BankDefinitionsDocument.h"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

namespace synaptome::state {
namespace {

BankDefinitionsDocumentResult bankDefinitionsFailure(
    std::string error,
    BankDefinitionsDocumentError code =
        BankDefinitionsDocumentError::InvalidDocument) {
    BankDefinitionsDocumentResult result;
    result.errorCode = code;
    result.error = std::move(error);
    return result;
}

bool bankOnlyKeys(
    const ofJson& object,
    std::initializer_list<const char*> allowed) {
    std::set<std::string> keys;
    for (const char* key : allowed) {
        keys.emplace(key);
    }
    for (const auto& item : object.items()) {
        if (keys.find(item.key()) == keys.end()) {
            return false;
        }
    }
    return true;
}

bool bankStableId(const ofJson& value) {
    if (!value.is_string()) {
        return false;
    }
    const auto& text = value.get_ref<const std::string&>();
    if (text.empty() || text.size() > 127 ||
        std::isalnum(static_cast<unsigned char>(text.front())) == 0) {
        return false;
    }
    return std::all_of(
        text.begin(),
        text.end(),
        [](unsigned char character) {
            return std::isalnum(character) != 0 ||
                character == '.' ||
                character == '_' ||
                character == '-';
        });
}

bool optionalText(
    const ofJson& object,
    const char* key,
    const std::string& path,
    std::string& error) {
    if (!object.contains(key)) {
        return true;
    }
    if (!object[key].is_string() ||
        object[key].get_ref<const std::string&>().size() > 255) {
        error = path + "." + key +
            " must be a string no longer than 255 characters";
        return false;
    }
    return true;
}

bool validateControl(
    const ofJson& control,
    const std::string& path,
    std::string& controlId,
    std::string& error) {
    if (!control.is_object() ||
        !bankOnlyKeys(
            control,
            {
                "id",
                "label",
                "target",
                "modifier",
                "description",
                "softTakeover",
            })) {
        error = path + " contains unknown control fields";
        return false;
    }
    if (!control.contains("id") ||
        !bankStableId(control["id"])) {
        error = path + ".id must be a stable ID";
        return false;
    }
    controlId = control["id"].get<std::string>();
    if (!optionalText(control, "label", path, error) ||
        !optionalText(control, "description", path, error)) {
        return false;
    }
    const bool hasTarget =
        control.contains("target") &&
        bankStableId(control["target"]);
    const bool hasModifier =
        control.contains("modifier") &&
        bankStableId(control["modifier"]);
    if ((control.contains("target") && !hasTarget) ||
        (control.contains("modifier") && !hasModifier) ||
        (!hasTarget && !hasModifier)) {
        error =
            path +
            " requires at least one stable target or modifier ID";
        return false;
    }
    if (control.contains("softTakeover") &&
        !control["softTakeover"].is_boolean()) {
        error = path + ".softTakeover must be a boolean";
        return false;
    }
    return true;
}

bool containsCycle(
    const std::string& start,
    const std::unordered_map<std::string, std::string>& parents) {
    std::set<std::string> visited;
    std::string current = start;
    while (!current.empty()) {
        if (!visited.emplace(current).second) {
            return true;
        }
        const auto it = parents.find(current);
        current =
            it == parents.end() ? std::string() : it->second;
    }
    return false;
}

} // namespace

BankDefinitionsDocumentResult validateBankDefinitionsDocument(
    const ofJson& source) {
    if (!source.is_object() ||
        !bankOnlyKeys(source, {"schemaVersion", "globalBanks"})) {
        return bankDefinitionsFailure(
            "bank-definitions document must contain only schemaVersion and "
            "globalBanks");
    }
    if (!source.contains("schemaVersion") ||
        !source["schemaVersion"].is_number_integer()) {
        return bankDefinitionsFailure(
            "bank-definitions schemaVersion must be an integer");
    }
    const int version = source["schemaVersion"].get<int>();
    if (version < 1) {
        return bankDefinitionsFailure(
            "bank-definitions schemaVersion must be positive");
    }
    if (version > kCurrentBankDefinitionsSchemaVersion) {
        return bankDefinitionsFailure(
            "unsupported future bank-definitions schemaVersion " +
                std::to_string(version),
            BankDefinitionsDocumentError::UnsupportedFutureVersion);
    }
    if (!source.contains("globalBanks") ||
        !source["globalBanks"].is_array() ||
        source["globalBanks"].size() > 128) {
        return bankDefinitionsFailure(
            "globalBanks must be an array with at most 128 entries");
    }

    std::set<std::string> bankIds;
    std::unordered_map<std::string, std::string> parents;
    for (std::size_t index = 0;
         index < source["globalBanks"].size();
         ++index) {
        const auto& bank = source["globalBanks"][index];
        const std::string path =
            "globalBanks[" + std::to_string(index) + "]";
        if (!bank.is_object() ||
            !bankOnlyKeys(bank, {"id", "label", "parent", "controls"}) ||
            !bank.contains("id") ||
            !bankStableId(bank["id"])) {
            return bankDefinitionsFailure(
                path + " must contain a stable id and only canonical fields");
        }
        const std::string bankId =
            bank["id"].get<std::string>();
        if (!bankIds.emplace(bankId).second) {
            return bankDefinitionsFailure(
                "global bank IDs must be unique");
        }
        std::string error;
        if (!optionalText(bank, "label", path, error)) {
            return bankDefinitionsFailure(std::move(error));
        }
        if (bank.contains("parent")) {
            if (!bankStableId(bank["parent"])) {
                return bankDefinitionsFailure(
                    path + ".parent must be a stable ID");
            }
            parents[bankId] =
                bank["parent"].get<std::string>();
        }
        if (bank.contains("controls")) {
            if (!bank["controls"].is_array() ||
                bank["controls"].size() > 512) {
                return bankDefinitionsFailure(
                    path +
                    ".controls must be an array with at most 512 entries");
            }
            std::set<std::string> controlIds;
            for (std::size_t controlIndex = 0;
                 controlIndex < bank["controls"].size();
                 ++controlIndex) {
                std::string controlId;
                const std::string controlPath =
                    path + ".controls[" +
                    std::to_string(controlIndex) + "]";
                if (!validateControl(
                        bank["controls"][controlIndex],
                        controlPath,
                        controlId,
                        error)) {
                    return bankDefinitionsFailure(std::move(error));
                }
                if (!controlIds.emplace(controlId).second) {
                    return bankDefinitionsFailure(
                        path + " control IDs must be unique");
                }
            }
        }
    }
    for (const auto& parent : parents) {
        if (bankIds.find(parent.second) == bankIds.end()) {
            return bankDefinitionsFailure(
                "global bank '" + parent.first +
                "' references missing parent '" + parent.second + "'");
        }
        if (containsCycle(parent.first, parents)) {
            return bankDefinitionsFailure(
                "global bank inheritance must be acyclic");
        }
    }

    BankDefinitionsDocumentResult result;
    result.ok = true;
    result.sourceVersion = version;
    result.document = source;
    return result;
}

bool BankDefinitionsPublisher::adoptInitial(
    const ofJson& source,
    std::string* error) {
    const auto validated =
        validateBankDefinitionsDocument(source);
    if (!validated.ok) {
        if (error) {
            *error = validated.error;
        }
        return false;
    }
    snapshot_ = validated.document;
    return true;
}

BankDefinitionsPublishResult BankDefinitionsPublisher::publish(
    const ofJson& candidate) {
    BankDefinitionsPublishResult result;
    const auto validated =
        validateBankDefinitionsDocument(candidate);
    if (!validated.ok) {
        result.error = validated.error;
        result.document = snapshot_;
        return result;
    }
    const ofJson previous = snapshot_;
    bool persisted = true;
    try {
        persisted = !persist_ || persist_(validated.document);
    } catch (...) {
        persisted = false;
    }
    if (!persisted) {
        result.error = "failed to persist bank definitions";
        result.document = previous;
        return result;
    }
    bool adopted = true;
    try {
        adopted = !adopt_ || adopt_(validated.document);
    } catch (...) {
        adopted = false;
    }
    if (!adopted) {
        bool persistedRollback = true;
        bool adoptedRollback = true;
        try {
            persistedRollback = !persist_ || persist_(previous);
        } catch (...) {
            persistedRollback = false;
        }
        try {
            adoptedRollback = !adopt_ || adopt_(previous);
        } catch (...) {
            adoptedRollback = false;
        }
        result.rollbackSucceeded =
            persistedRollback && adoptedRollback;
        result.error = "failed to adopt bank definitions";
        result.document = previous;
        return result;
    }
    snapshot_ = validated.document;
    result.ok = true;
    result.rollbackSucceeded = true;
    result.document = snapshot_;
    return result;
}

} // namespace synaptome::state

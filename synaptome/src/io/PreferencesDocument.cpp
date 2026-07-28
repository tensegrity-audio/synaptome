#include "PreferencesDocument.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <initializer_list>
#include <set>
#include <string>
#include <utility>

namespace synaptome::state {
namespace {

PreferencesDocumentResult preferencesFailure(
    std::string error,
    PreferencesDocumentError code =
        PreferencesDocumentError::InvalidDocument) {
    PreferencesDocumentResult result;
    result.errorCode = code;
    result.error = std::move(error);
    return result;
}

bool onlyKeys(
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

bool stableId(const std::string& value, bool allowEmpty = false) {
    if (value.empty()) {
        return allowEmpty;
    }
    if (value.size() > 127 ||
        std::isalnum(static_cast<unsigned char>(value.front())) == 0) {
        return false;
    }
    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character) {
            return std::isalnum(character) != 0 ||
                character == '.' ||
                character == '_' ||
                character == '-';
        });
}

bool boundedText(const ofJson& value) {
    return value.is_string() &&
        value.get_ref<const std::string&>().size() <= 255;
}

bool validateStringArray(
    const ofJson& value,
    const std::string& path,
    std::string& error) {
    if (!value.is_array() || value.size() > 512) {
        error = path + " must be an array with at most 512 entries";
        return false;
    }
    std::set<std::string> unique;
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (!boundedText(value[index]) ||
            value[index].get<std::string>().empty()) {
            error = path + "[" + std::to_string(index) +
                "] must be a non-empty string no longer than 255 characters";
            return false;
        }
        if (!unique.emplace(value[index].get<std::string>()).second) {
            error = path + " entries must be unique";
            return false;
        }
    }
    return true;
}

bool validateBrowser(
    const ofJson& browser,
    std::string& error) {
    if (!browser.is_object() ||
        !onlyKeys(
            browser,
            {
                "treeWidthRatio",
                "selection",
                "selectedColumn",
                "visibleColumns",
                "collapsedCategories",
                "collapsedParameterSections",
            })) {
        error = "browser must contain only canonical Browser preference keys";
        return false;
    }
    if (browser.contains("treeWidthRatio")) {
        if (!browser["treeWidthRatio"].is_number()) {
            error = "browser.treeWidthRatio must be a number";
            return false;
        }
        const double ratio = browser["treeWidthRatio"].get<double>();
        if (ratio < 0.10 || ratio > 0.50) {
            error = "browser.treeWidthRatio must be between 0.10 and 0.50";
            return false;
        }
    }
    if (browser.contains("selection")) {
        const auto& selection = browser["selection"];
        if (!selection.is_object() ||
            !onlyKeys(selection, {"category", "subcategory", "asset"})) {
            error =
                "browser.selection must contain only category, subcategory, "
                "and asset";
            return false;
        }
        for (const char* key : {"category", "subcategory", "asset"}) {
            if (selection.contains(key) && !boundedText(selection[key])) {
                error = std::string("browser.selection.") + key +
                    " must be a string no longer than 255 characters";
                return false;
            }
        }
    }
    if (browser.contains("selectedColumn")) {
        static const std::set<std::string> columns = {
            "name", "value", "slot", "midi", "midi_min", "midi_max",
            "osc", "osc_deadband",
        };
        if (!browser["selectedColumn"].is_string() ||
            columns.find(browser["selectedColumn"].get<std::string>()) ==
                columns.end()) {
            error = "browser.selectedColumn is not a known column ID";
            return false;
        }
    }
    if (browser.contains("visibleColumns")) {
        const auto& visible = browser["visibleColumns"];
        if (!visible.is_object()) {
            error = "browser.visibleColumns must be an object";
            return false;
        }
        static const std::set<std::string> columns = {
            "name", "value", "slot", "midi", "midi_min", "midi_max",
            "osc", "osc_deadband",
        };
        for (const auto& item : visible.items()) {
            if (columns.find(item.key()) == columns.end() ||
                !item.value().is_boolean()) {
                error =
                    "browser.visibleColumns must contain only known boolean "
                    "column IDs";
                return false;
            }
        }
        if (visible.contains("name") &&
            !visible["name"].get<bool>()) {
            error = "browser.visibleColumns.name cannot be false";
            return false;
        }
    }
    for (const char* key :
         {"collapsedCategories", "collapsedParameterSections"}) {
        if (browser.contains(key) &&
            !validateStringArray(
                browser[key],
                std::string("browser.") + key,
                error)) {
            return false;
        }
    }
    return true;
}

bool validateWidget(
    const ofJson& widget,
    const std::string& path,
    std::set<std::string>& identities,
    std::string& error) {
    if (!widget.is_object() ||
        !onlyKeys(
            widget,
            {"id", "target", "column", "band", "visible", "collapsed"})) {
        error = path + " contains unknown widget keys";
        return false;
    }
    if (!widget.contains("id") || !widget["id"].is_string() ||
        !stableId(widget["id"].get<std::string>())) {
        error = path + ".id must be a stable ID";
        return false;
    }
    const std::string target =
        widget.value("target", std::string("projector"));
    if (target != "projector" && target != "controller") {
        error = path + ".target must be projector or controller";
        return false;
    }
    const std::string identity =
        target + ":" + widget["id"].get<std::string>();
    if (!identities.emplace(identity).second) {
        error = "hud.widgets must not repeat a target/id pair";
        return false;
    }
    if (widget.contains("column") &&
        (!widget["column"].is_number_integer() ||
         widget["column"].get<int>() < -1 ||
         widget["column"].get<int>() > 31)) {
        error = path + ".column must be an integer between -1 and 31";
        return false;
    }
    if (widget.contains("band") &&
        (!widget["band"].is_string() ||
         !stableId(widget["band"].get<std::string>()))) {
        error = path + ".band must be a stable ID";
        return false;
    }
    for (const char* key : {"visible", "collapsed"}) {
        if (widget.contains(key) && !widget[key].is_boolean()) {
            error = path + "." + key + " must be a boolean";
            return false;
        }
    }
    return true;
}

bool validateHud(const ofJson& hud, std::string& error) {
    if (!hud.is_object() ||
        !onlyKeys(hud, {"visible", "layoutTarget", "stateMigrated", "widgets"})) {
        error = "hud must contain only canonical HUD preference keys";
        return false;
    }
    for (const char* key : {"visible", "stateMigrated"}) {
        if (hud.contains(key) && !hud[key].is_boolean()) {
            error = std::string("hud.") + key + " must be a boolean";
            return false;
        }
    }
    if (hud.contains("layoutTarget")) {
        if (!hud["layoutTarget"].is_string()) {
            error = "hud.layoutTarget must be a string";
            return false;
        }
        const std::string target = hud["layoutTarget"].get<std::string>();
        if (target != "projector" && target != "controller") {
            error = "hud.layoutTarget must be projector or controller";
            return false;
        }
    }
    if (hud.contains("widgets")) {
        const auto& widgets = hud["widgets"];
        if (!widgets.is_array() || widgets.size() > 256) {
            error = "hud.widgets must be an array with at most 256 entries";
            return false;
        }
        std::set<std::string> identities;
        for (std::size_t index = 0; index < widgets.size(); ++index) {
            if (!validateWidget(
                    widgets[index],
                    "hud.widgets[" + std::to_string(index) + "]",
                    identities,
                    error)) {
                return false;
            }
        }
    }
    return true;
}

bool validateHotkeys(const ofJson& hotkeys, std::string& error) {
    if (!hotkeys.is_object() ||
        !onlyKeys(hotkeys, {"bindings"}) ||
        (hotkeys.contains("bindings") &&
         (!hotkeys["bindings"].is_array() ||
          hotkeys["bindings"].size() > 512))) {
        error = "hotkeys must contain only bindings[]";
        return false;
    }
    std::set<std::string> ids;
    if (!hotkeys.contains("bindings")) {
        return true;
    }
    for (std::size_t index = 0;
         index < hotkeys["bindings"].size();
         ++index) {
        const auto& binding = hotkeys["bindings"][index];
        const std::string path =
            "hotkeys.bindings[" + std::to_string(index) + "]";
        if (!binding.is_object() ||
            !onlyKeys(binding, {"id", "key"}) ||
            !binding.contains("id") ||
            !binding["id"].is_string() ||
            !stableId(binding["id"].get<std::string>()) ||
            !binding.contains("key") ||
            !binding["key"].is_number_integer()) {
            error = path + " must contain only stable id and integer key";
            return false;
        }
        const long long key = binding["key"].get<long long>();
        if (key < 0 || key > INT_MAX) {
            error = path + ".key must be between 0 and INT_MAX";
            return false;
        }
        if (!ids.emplace(binding["id"].get<std::string>()).second) {
            error = "hotkeys.bindings IDs must be unique";
            return false;
        }
    }
    return true;
}

bool validatePackages(const ofJson& packages, std::string& error) {
    if (!packages.is_object() ||
        !onlyKeys(packages, {"activations"}) ||
        (packages.contains("activations") &&
         (!packages["activations"].is_array() ||
          packages["activations"].size() > 256))) {
        error = "packages must contain only activations[]";
        return false;
    }
    std::set<std::string> ids;
    if (!packages.contains("activations")) {
        return true;
    }
    for (std::size_t index = 0;
         index < packages["activations"].size();
         ++index) {
        const auto& activation = packages["activations"][index];
        const std::string path =
            "packages.activations[" + std::to_string(index) + "]";
        if (!activation.is_object() ||
            !onlyKeys(
                activation,
                {"packageId", "enabled", "selectedPreset"}) ||
            !activation.contains("packageId") ||
            !activation["packageId"].is_string() ||
            !stableId(activation["packageId"].get<std::string>()) ||
            !activation.contains("enabled") ||
            !activation["enabled"].is_boolean()) {
            error =
                path +
                " must contain packageId, enabled, and optional "
                "selectedPreset";
            return false;
        }
        if (!ids.emplace(
                activation["packageId"].get<std::string>()).second) {
            error = "packages.activations package IDs must be unique";
            return false;
        }
        if (activation.contains("selectedPreset")) {
            const auto& preset = activation["selectedPreset"];
            if (!preset.is_object() ||
                !onlyKeys(preset, {"bankId", "presetId"}) ||
                !preset.contains("bankId") ||
                !preset["bankId"].is_string() ||
                !stableId(preset["bankId"].get<std::string>()) ||
                !preset.contains("presetId") ||
                !preset["presetId"].is_string() ||
                !stableId(preset["presetId"].get<std::string>())) {
                error =
                    path +
                    ".selectedPreset must contain stable bankId and presetId";
                return false;
            }
        }
    }
    return true;
}

bool validateMappings(const ofJson& mappings, std::string& error) {
    if (!mappings.is_object() ||
        !onlyKeys(mappings, {"activeBank"}) ||
        (mappings.contains("activeBank") &&
         (!mappings["activeBank"].is_string() ||
          !stableId(
              mappings["activeBank"].get<std::string>(),
              true)))) {
        error =
            "mappings must contain only activeBank as a stable ID or empty "
            "global-bank selection";
        return false;
    }
    return true;
}

ofJson migrateLegacyControlHub(const ofJson& source) {
    ofJson document = {
        {"schemaVersion", kCurrentPreferencesSchemaVersion}
    };
    ofJson browser = ofJson::object();
    for (const auto& pair :
         {
             std::pair<const char*, const char*>{
                 "treeWidthRatio", "treeWidthRatio"},
             {"selectedColumn", "selectedColumn"},
             {"visibleColumns", "visibleColumns"},
             {"collapsedCategories", "collapsedCategories"},
             {
                 "collapsedParameterSections",
                 "collapsedParameterSections",
             },
         }) {
        if (source.contains(pair.first)) {
            browser[pair.second] = source[pair.first];
        }
    }
    ofJson selection = ofJson::object();
    for (const auto& pair :
         {
             std::pair<const char*, const char*>{
                 "selectedCategory", "category"},
             {"selectedSubcategory", "subcategory"},
             {"selectedAsset", "asset"},
         }) {
        if (source.contains(pair.first)) {
            selection[pair.second] = source[pair.first];
        }
    }
    if (!selection.empty()) {
        browser["selection"] = std::move(selection);
    }
    if (!browser.empty()) {
        document["browser"] = std::move(browser);
    }

    ofJson hud = ofJson::object();
    if (source.contains("hudVisible")) {
        hud["visible"] = source["hudVisible"];
    }
    if (source.contains("hudLayoutTarget")) {
        hud["layoutTarget"] = source["hudLayoutTarget"];
    }
    if (source.contains("hudStateMigrated")) {
        hud["stateMigrated"] = source["hudStateMigrated"];
    }
    ofJson widgets = ofJson::array();
    auto appendWidgets =
        [&](const char* key, const char* target, bool includeVisible) {
            if (!source.contains(key) || !source[key].is_array()) {
                return;
            }
            for (const auto& legacy : source[key]) {
                if (!legacy.is_object()) {
                    widgets.push_back(legacy);
                    continue;
                }
                ofJson widget = legacy;
                widget["target"] = target;
                if (!includeVisible) {
                    widget.erase("visible");
                }
                widgets.push_back(std::move(widget));
            }
        };
    appendWidgets("hudWidgets", "projector", true);
    appendWidgets("hudControllerWidgets", "controller", false);
    if (!widgets.empty()) {
        hud["widgets"] = std::move(widgets);
    }
    if (!hud.empty()) {
        document["hud"] = std::move(hud);
    }
    return document;
}

} // namespace

PreferencesDocumentResult normalizePreferencesDocument(
    const ofJson& source) {
    if (!source.is_object()) {
        return preferencesFailure("preferences document must be an object");
    }

    PreferencesDocumentResult result;
    if (!source.contains("schemaVersion")) {
        result.kind = PreferencesDocumentKind::LegacyControlHub;
        result.migratedInMemory = true;
        result.document = migrateLegacyControlHub(source);
    } else {
        if (!source["schemaVersion"].is_number_integer()) {
            return preferencesFailure(
                "preferences schemaVersion must be an integer");
        }
        result.sourceVersion = source["schemaVersion"].get<int>();
        if (result.sourceVersion < 1) {
            return preferencesFailure(
                "preferences schemaVersion must be positive");
        }
        if (result.sourceVersion >
            kCurrentPreferencesSchemaVersion) {
            return preferencesFailure(
                "unsupported future preferences schemaVersion " +
                    std::to_string(result.sourceVersion),
                PreferencesDocumentError::UnsupportedFutureVersion);
        }
        result.kind = PreferencesDocumentKind::CurrentV1;
        result.document = source;
    }

    if (!onlyKeys(
            result.document,
            {
                "schemaVersion",
                "browser",
                "hud",
                "hotkeys",
                "packages",
                "mappings",
            })) {
        return preferencesFailure(
            "preferences document contains unknown top-level keys");
    }
    std::string error;
    if ((result.document.contains("browser") &&
         !validateBrowser(result.document["browser"], error)) ||
        (result.document.contains("hud") &&
         !validateHud(result.document["hud"], error)) ||
        (result.document.contains("hotkeys") &&
         !validateHotkeys(result.document["hotkeys"], error)) ||
        (result.document.contains("packages") &&
         !validatePackages(result.document["packages"], error)) ||
        (result.document.contains("mappings") &&
         !validateMappings(result.document["mappings"], error))) {
        return preferencesFailure(std::move(error));
    }

    result.ok = true;
    return result;
}

ofJson controlHubCompatibilityView(
    const ofJson& canonicalDocument) {
    ofJson legacy = ofJson::object();
    if (canonicalDocument.contains("browser")) {
        const auto& browser = canonicalDocument["browser"];
        for (const char* key :
             {
                 "treeWidthRatio",
                 "selectedColumn",
                 "visibleColumns",
                 "collapsedCategories",
                 "collapsedParameterSections",
             }) {
            if (browser.contains(key)) {
                legacy[key] = browser[key];
            }
        }
        if (browser.contains("selection")) {
            const auto& selection = browser["selection"];
            if (selection.contains("category")) {
                legacy["selectedCategory"] =
                    selection["category"];
            }
            if (selection.contains("subcategory")) {
                legacy["selectedSubcategory"] =
                    selection["subcategory"];
            }
            if (selection.contains("asset")) {
                legacy["selectedAsset"] =
                    selection["asset"];
            }
        }
    }
    if (canonicalDocument.contains("hud")) {
        const auto& hud = canonicalDocument["hud"];
        if (hud.contains("visible")) {
            legacy["hudVisible"] = hud["visible"];
        }
        if (hud.contains("layoutTarget")) {
            legacy["hudLayoutTarget"] =
                hud["layoutTarget"];
        }
        if (hud.contains("stateMigrated")) {
            legacy["hudStateMigrated"] =
                hud["stateMigrated"];
        }
        ofJson projector = ofJson::array();
        ofJson controller = ofJson::array();
        for (const auto& widget :
             hud.value("widgets", ofJson::array())) {
            ofJson copy = widget;
            const std::string target =
                copy.value(
                    "target",
                    std::string("projector"));
            copy.erase("target");
            if (target == "controller") {
                copy.erase("visible");
                controller.push_back(std::move(copy));
            } else {
                projector.push_back(std::move(copy));
            }
        }
        if (!projector.empty()) {
            legacy["hudWidgets"] = std::move(projector);
        }
        if (!controller.empty()) {
            legacy["hudControllerWidgets"] =
                std::move(controller);
        }
    }
    return legacy;
}

bool PreferencesPublisher::adoptInitial(
    const ofJson& source,
    std::string* error) {
    const auto normalized = normalizePreferencesDocument(source);
    if (!normalized.ok) {
        if (error) {
            *error = normalized.error;
        }
        return false;
    }
    snapshot_ = normalized.document;
    return true;
}

PreferencesPublishResult PreferencesPublisher::publish(
    const ofJson& candidate) {
    PreferencesPublishResult result;
    const auto normalized = normalizePreferencesDocument(candidate);
    if (!normalized.ok ||
        normalized.kind != PreferencesDocumentKind::CurrentV1) {
        result.error = normalized.ok
            ? "publication requires canonical preferences v1"
            : normalized.error;
        return result;
    }
    const ofJson previous = snapshot_;
    bool persisted = true;
    try {
        persisted =
            !persist_ || persist_(normalized.document);
    } catch (...) {
        persisted = false;
    }
    if (!persisted) {
        result.error = "failed to persist preferences";
        result.document = previous;
        return result;
    }
    bool adopted = true;
    try {
        adopted =
            !adopt_ || adopt_(normalized.document);
    } catch (...) {
        adopted = false;
    }
    if (!adopted) {
        bool persistedRollback = true;
        bool adoptedRollback = true;
        try {
            persistedRollback =
                !persist_ || persist_(previous);
        } catch (...) {
            persistedRollback = false;
        }
        try {
            adoptedRollback =
                !adopt_ || adopt_(previous);
        } catch (...) {
            adoptedRollback = false;
        }
        result.rollbackSucceeded =
            persistedRollback && adoptedRollback;
        result.error = "failed to adopt preferences";
        result.document = previous;
        return result;
    }
    snapshot_ = normalized.document;
    result.ok = true;
    result.rollbackSucceeded = true;
    result.document = snapshot_;
    return result;
}

PreferencesPublishResult PreferencesPublisher::publishSection(
    const std::string& section,
    const ofJson& value) {
    static const std::set<std::string> sections = {
        "browser", "hud", "hotkeys", "packages", "mappings"
    };
    if (sections.find(section) == sections.end()) {
        PreferencesPublishResult result;
        result.error = "unknown preferences section '" + section + "'";
        return result;
    }
    ofJson candidate = snapshot_;
    candidate["schemaVersion"] = kCurrentPreferencesSchemaVersion;
    candidate[section] = value;
    return publish(candidate);
}

} // namespace synaptome::state

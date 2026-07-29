#pragma once

#include "ParameterRegistry.h"
#include "ofJson.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace synaptome::controls {

struct PackageLayerTarget {
    int layerIndex = 0;
    std::string assetId;
    std::string registryPrefix;
    bool active = false;
};

struct MappingConflict {
    std::string kind;
    std::string detail;
    ofJson currentRoute;
};

struct MappingRoutePreview {
    std::string routeId;
    std::string packageId;
    std::string packageVersion;
    std::string presetId;
    std::string presetLabel;
    std::string targetKind;
    std::string localTargetId;
    std::string expandedTargetId;
    std::string sourcePattern;
    int mappingIndex = -1;
    int layerIndex = 0;
    ofJson source;
    ofJson candidateRoute;
    std::vector<MappingConflict> conflicts;
};

struct MappingSuggestionPreview {
    bool ok = false;
    std::string error;
    std::string packageId;
    std::string presetId;
    std::string presetLabel;
    std::vector<MappingRoutePreview> routes;

    std::size_t conflictCount() const {
        std::size_t count = 0;
        for (const auto& route : routes) {
            count += route.conflicts.size();
        }
        return count;
    }
};

struct MappingMutationResult {
    bool ok = false;
    bool rollbackSucceeded = true;
    std::string error;
    ofJson document;
};

inline std::string packageRouteId(const std::string& packageId,
                                  const std::string& presetId,
                                  int mappingIndex,
                                  int layerIndex) {
    return packageId + "/" + presetId + "/" +
        std::to_string(mappingIndex) + "/layer" +
        std::to_string(layerIndex);
}

inline bool numericPair(const ofJson& value) {
    return value.is_array() && value.size() == 2 &&
        value[0].is_number() && value[1].is_number() &&
        std::isfinite(value[0].get<double>()) &&
        std::isfinite(value[1].get<double>());
}

inline const ofJson* findMappingPreset(const ofJson& packageEntry,
                                       const std::string& presetId) {
    if (!packageEntry.contains("mappingPresets") ||
        !packageEntry["mappingPresets"].is_array()) {
        return nullptr;
    }
    for (const auto& preset : packageEntry["mappingPresets"]) {
        if (preset.is_object() &&
            preset.value("id", std::string()) == presetId) {
            return &preset;
        }
    }
    return nullptr;
}

inline bool declaredTarget(const ofJson& packageEntry,
                           const std::string& kind,
                           const std::string& id) {
    if (!packageEntry.contains("controls") ||
        !packageEntry["controls"].is_object()) {
        return false;
    }
    const char* section =
        kind == "action" ? "actions" : "parameters";
    const auto& controls = packageEntry["controls"];
    if (!controls.contains(section) ||
        !controls[section].is_array()) {
        return false;
    }
    for (const auto& declaration : controls[section]) {
        if (declaration.is_object() &&
            declaration.value("id", std::string()) == id) {
            return true;
        }
    }
    return false;
}

inline bool parseMappingTarget(const ofJson& mapping,
                               std::string& kind,
                               std::string& id,
                               std::string& error) {
    if (!mapping.contains("target")) {
        error = "mapping target is missing";
        return false;
    }
    const auto& target = mapping["target"];
    if (target.is_string()) {
        kind = "parameter";
        id = target.get<std::string>();
    } else if (target.is_object()) {
        kind = target.value("kind", std::string());
        id = target.value("id", std::string());
    } else {
        error = "mapping target must be a string or explicit target object";
        return false;
    }
    if ((kind != "parameter" && kind != "action") || id.empty()) {
        error = "mapping target must name a parameter or action";
        return false;
    }
    return true;
}

inline bool validateMappingSource(const ofJson& source,
                                  const std::string& targetKind,
                                  std::string& error) {
    if (!source.is_object() ||
        source.value("kind", std::string()) != "osc") {
        error = "only declared OSC package suggestions are supported";
        return false;
    }
    const std::string pattern =
        source.value("pattern", std::string());
    if (pattern.empty() || pattern.front() != '/') {
        error = "OSC mapping source must begin with '/'";
        return false;
    }
    for (const char* key : {"in", "out"}) {
        if (source.contains(key) && !numericPair(source[key])) {
            error = std::string("mapping source ") + key +
                " must be a finite numeric pair";
            return false;
        }
    }
    if (targetKind == "action") {
        if (!source.contains("trigger") ||
            !source["trigger"].is_object()) {
            error = "action mappings require explicit trigger semantics";
            return false;
        }
        const auto& trigger = source["trigger"];
        const std::string edge =
            trigger.value("edge", std::string());
        if (edge != "rising" && edge != "falling" &&
            edge != "both") {
            error =
                "action trigger edge must be rising, falling, or both";
            return false;
        }
        if (!trigger.contains("threshold") ||
            !trigger["threshold"].is_number() ||
            !std::isfinite(trigger["threshold"].get<double>())) {
            error = "action trigger threshold must be finite";
            return false;
        }
    } else if (source.contains("trigger")) {
        error = "parameter mappings must not declare action trigger semantics";
        return false;
    }
    return true;
}

inline MappingSuggestionPreview previewMappingSuggestion(
    const ofJson& packageEntry,
    const std::string& presetId,
    const std::vector<PackageLayerTarget>& layers,
    const ofJson& currentMappingBank) {
    MappingSuggestionPreview preview;
    preview.packageId =
        packageEntry.value("packageId",
                           packageEntry.value("assetId", std::string()));
    preview.presetId = presetId;
    if (preview.packageId.empty()) {
        preview.error = "package entry has no stable package identity";
        return preview;
    }
    const ofJson* preset = findMappingPreset(packageEntry, presetId);
    if (!preset) {
        preview.error = "unknown mapping suggestion '" + presetId + "'";
        return preview;
    }
    preview.presetLabel = preset->value("label", presetId);
    if (preset->value("applyMode", std::string()) !=
            "suggestion-only") {
        preview.error =
            "package mapping preset is not suggestion-only";
        return preview;
    }
    if (!preset->contains("mappings") ||
        !(*preset)["mappings"].is_array()) {
        preview.error = "mapping suggestion has no route declarations";
        return preview;
    }

    const std::string assetId =
        packageEntry.value("assetId", std::string());
    std::vector<PackageLayerTarget> matchingLayers;
    for (const auto& layer : layers) {
        if (layer.layerIndex > 0 && layer.assetId == assetId) {
            matchingLayers.push_back(layer);
        }
    }
    if (matchingLayers.empty()) {
        preview.error =
            "mapping suggestion has no active or assigned layer instance";
        return preview;
    }

    const ofJson currentOsc =
        currentMappingBank.value("osc", ofJson::array());
    int mappingIndex = 0;
    for (const auto& mapping : (*preset)["mappings"]) {
        if (!mapping.is_object()) {
            preview.error = "mapping declaration must be an object";
            return preview;
        }
        std::string targetKind;
        std::string targetId;
        if (!parseMappingTarget(
                mapping, targetKind, targetId, preview.error)) {
            return preview;
        }
        if (!declaredTarget(
                packageEntry, targetKind, targetId)) {
            preview.error = "mapping target '" + targetId +
                "' is not declared by the package element";
            return preview;
        }
        const ofJson source =
            mapping.value("source", ofJson::object());
        if (!validateMappingSource(
                source, targetKind, preview.error)) {
            return preview;
        }
        const std::string pattern =
            source.value("pattern", std::string());

        for (const auto& layer : matchingLayers) {
            MappingRoutePreview route;
            route.routeId = packageRouteId(
                preview.packageId,
                presetId,
                mappingIndex,
                layer.layerIndex);
            route.packageId = preview.packageId;
            route.packageVersion =
                packageEntry.value("packageVersion", std::string());
            route.presetId = presetId;
            route.presetLabel = preview.presetLabel;
            route.targetKind = targetKind;
            route.localTargetId = targetId;
            route.layerIndex = layer.layerIndex;
            route.mappingIndex = mappingIndex;
            route.sourcePattern = pattern;
            route.source = source;
            const std::string prefix =
                layer.registryPrefix.empty()
                    ? "console.layer" +
                        std::to_string(layer.layerIndex)
                    : layer.registryPrefix;
            route.expandedTargetId =
                targetKind == "action"
                    ? prefix + ".actions." + targetId
                    : prefix + "." + targetId;
            route.candidateRoute = {
                {"pattern", pattern},
                {"target", route.expandedTargetId},
                {"targetKind", targetKind},
                {"enabled", true},
                {"provenance", {
                    {"owner", "package-suggestion"},
                    {"packageId", preview.packageId},
                    {"packageVersion", route.packageVersion},
                    {"presetId", presetId},
                    {"routeId", route.routeId},
                    {"mappingIndex", mappingIndex},
                    {"layerIndex", layer.layerIndex},
                    {"assetId", assetId}
                }}
            };
            if (targetKind == "action") {
                route.candidateRoute["trigger"] =
                    source["trigger"];
            }
            if (currentOsc.is_array()) {
                for (const auto& current : currentOsc) {
                    if (!current.is_object()) {
                        continue;
                    }
                    const std::string currentTarget =
                        current.value("target", std::string());
                    const std::string currentPattern =
                        current.value("pattern", std::string());
                    if (currentTarget == route.expandedTargetId) {
                        MappingConflict conflict;
                        conflict.kind =
                            currentPattern == pattern
                                ? "exact-route"
                                : "target";
                        conflict.detail =
                            currentPattern == pattern
                                ? "the target already has this source"
                                : "the target already has another source";
                        conflict.currentRoute = current;
                        route.conflicts.push_back(
                            std::move(conflict));
                    } else if (currentPattern == pattern) {
                        MappingConflict conflict;
                        conflict.kind = "shared-source";
                        conflict.detail =
                            "the source already controls another target";
                        conflict.currentRoute = current;
                        route.conflicts.push_back(
                            std::move(conflict));
                    }
                }
            }
            preview.routes.push_back(std::move(route));
        }
        ++mappingIndex;
    }
    preview.ok = true;
    return preview;
}

inline void upsertOscSource(ofJson& document,
                            const MappingRoutePreview& route) {
    if (!document.contains("oscSources") ||
        !document["oscSources"].is_array()) {
        document["oscSources"] = ofJson::array();
    }
    const std::string packageBlend =
        route.source.value("blend", std::string("scale"));
    const std::string runtimeBlend =
        packageBlend == "set"
            ? "absolute"
            : (packageBlend == "add"
                   ? "additive"
                   : packageBlend);
    ofJson profile = {
        {"pattern", route.sourcePattern},
        {"in", route.source.value("in", ofJson::array({0.0, 1.0}))},
        {"out", route.source.value("out", ofJson::array({0.0, 1.0}))},
        {"blend", runtimeBlend},
        {"relative", route.source.value("relative", true)}
    };
    if (route.source.contains("smooth")) {
        profile["smooth"] = route.source["smooth"];
    }
    if (route.source.contains("deadband")) {
        profile["deadband"] = route.source["deadband"];
    }
    for (auto& current : document["oscSources"]) {
        if (current.is_object() &&
            current.value("pattern", std::string()) ==
                route.sourcePattern) {
            current = std::move(profile);
            return;
        }
    }
    document["oscSources"].push_back(std::move(profile));
}

inline MappingMutationResult buildMappingCandidate(
    const ofJson& currentMappingBank,
    const MappingSuggestionPreview& preview,
    bool replaceTargetConflicts) {
    MappingMutationResult result;
    if (!preview.ok) {
        result.error = preview.error.empty()
            ? "mapping preview is invalid"
            : preview.error;
        return result;
    }
    result.document = currentMappingBank;
    if (!result.document.is_object()) {
        result.document = ofJson::object();
    }
    result.document["schemaVersion"] = 1;
    for (const char* section :
         {"cc", "buttons", "oscSources", "osc"}) {
        if (!result.document.contains(section) ||
            !result.document[section].is_array()) {
            result.document[section] = ofJson::array();
        }
    }

    for (const auto& route : preview.routes) {
        const bool blockingConflict =
            std::any_of(
                route.conflicts.begin(),
                route.conflicts.end(),
                [&](const MappingConflict& conflict) {
                    if (conflict.kind == "exact-route" &&
                        conflict.currentRoute.contains("provenance") &&
                        conflict.currentRoute["provenance"].is_object() &&
                        conflict.currentRoute["provenance"].value(
                            "owner", std::string()) ==
                            "package-suggestion" &&
                        conflict.currentRoute["provenance"].value(
                            "routeId", std::string()) ==
                            route.routeId) {
                        return false;
                    }
                    return true;
                });
        if (blockingConflict && !replaceTargetConflicts) {
            result.error =
                "mapping conflict requires explicit operator approval";
            return result;
        }
        auto& routes = result.document["osc"];
        if (replaceTargetConflicts) {
            routes.erase(
                std::remove_if(
                    routes.begin(),
                    routes.end(),
                    [&](const ofJson& current) {
                        return current.is_object() &&
                            current.value("target", std::string()) ==
                                route.expandedTargetId;
                    }),
                routes.end());
        } else {
            bool exact = false;
            for (auto& current : routes) {
                if (current.is_object() &&
                    current.value("target", std::string()) ==
                        route.expandedTargetId &&
                    current.value("pattern", std::string()) ==
                        route.sourcePattern) {
                    current = route.candidateRoute;
                    exact = true;
                    break;
                }
            }
            if (exact) {
                upsertOscSource(result.document, route);
                continue;
            }
        }
        routes.push_back(route.candidateRoute);
        upsertOscSource(result.document, route);
    }
    result.ok = true;
    return result;
}

inline bool packageRouteMatches(const ofJson& route,
                                const std::string& routeId) {
    return route.is_object() &&
        route.contains("provenance") &&
        route["provenance"].is_object() &&
        route["provenance"].value("owner", std::string()) ==
            "package-suggestion" &&
        route["provenance"].value("routeId", std::string()) ==
            routeId;
}

inline void pruneUnusedOscSources(ofJson& document) {
    if (!document.is_object() ||
        !document.contains("oscSources") ||
        !document["oscSources"].is_array()) {
        return;
    }
    std::set<std::string> usedPatterns;
    if (document.contains("osc") &&
        document["osc"].is_array()) {
        for (const auto& route : document["osc"]) {
            if (route.is_object()) {
                const std::string pattern =
                    route.value("pattern", std::string());
                if (!pattern.empty()) {
                    usedPatterns.insert(pattern);
                }
            }
        }
    }
    auto& sources = document["oscSources"];
    sources.erase(
        std::remove_if(
            sources.begin(),
            sources.end(),
            [&](const ofJson& source) {
                return source.is_object() &&
                    usedPatterns.count(
                        source.value(
                            "pattern",
                            std::string())) == 0;
            }),
        sources.end());
}

inline MappingMutationResult setPackageRouteEnabled(
    const ofJson& currentMappingBank,
    const std::string& routeId,
    bool enabled) {
    MappingMutationResult result;
    result.document = currentMappingBank;
    if (!result.document.is_object() ||
        !result.document.contains("osc") ||
        !result.document["osc"].is_array()) {
        result.error = "mapping bank has no OSC routes";
        return result;
    }
    for (auto& route : result.document["osc"]) {
        if (packageRouteMatches(route, routeId)) {
            route["enabled"] = enabled;
            result.ok = true;
            return result;
        }
    }
    result.error = "package-owned route was not found";
    return result;
}

inline MappingMutationResult removePackageRoute(
    const ofJson& currentMappingBank,
    const std::string& routeId) {
    MappingMutationResult result;
    result.document = currentMappingBank;
    if (!result.document.is_object() ||
        !result.document.contains("osc") ||
        !result.document["osc"].is_array()) {
        result.error = "mapping bank has no OSC routes";
        return result;
    }
    auto& routes = result.document["osc"];
    const auto oldSize = routes.size();
    routes.erase(
        std::remove_if(
            routes.begin(),
            routes.end(),
            [&](const ofJson& route) {
                return packageRouteMatches(route, routeId);
            }),
        routes.end());
    result.ok = routes.size() != oldSize;
    if (result.ok) {
        pruneUnusedOscSources(result.document);
    }
    if (!result.ok) {
        result.error = "package-owned route was not found";
    }
    return result;
}

inline MappingMutationResult editPackageRoute(
    const ofJson& currentMappingBank,
    const std::string& routeId,
    const ofJson& source) {
    MappingMutationResult result;
    result.document = currentMappingBank;
    if (!result.document.is_object() ||
        !result.document.contains("osc") ||
        !result.document["osc"].is_array()) {
        result.error = "mapping bank has no OSC routes";
        return result;
    }
    for (auto& route : result.document["osc"]) {
        if (!packageRouteMatches(route, routeId)) {
            continue;
        }
        const std::string targetKind =
            route.value("targetKind", std::string("parameter"));
        std::string error;
        if (!validateMappingSource(
                source, targetKind, error)) {
            result.error = error;
            return result;
        }
        route["pattern"] =
            source.value("pattern", std::string());
        route["enabled"] = true;
        if (targetKind == "action") {
            route["trigger"] = source["trigger"];
        } else {
            route.erase("trigger");
        }
        MappingRoutePreview edited;
        edited.sourcePattern =
            source.value("pattern", std::string());
        edited.source = source;
        upsertOscSource(result.document, edited);
        pruneUnusedOscSources(result.document);
        result.ok = true;
        return result;
    }
    result.error = "package-owned route was not found";
    return result;
}

class MappingSuggestionTransaction {
public:
    using Publisher =
        std::function<bool(const ofJson&)>;

    MappingMutationResult publish(
        const ofJson& current,
        const ofJson& candidate,
        const Publisher& publisher) {
        MappingMutationResult result;
        if (!publisher) {
            result.error = "mapping publisher is unavailable";
            return result;
        }
        bool published = false;
        try {
            published = publisher(candidate);
        } catch (...) {
            published = false;
        }
        if (!published) {
            result.error =
                "failed to publish mapping transaction; prior state preserved";
            result.document = current;
            return result;
        }
        rollbackDocument_ = current;
        currentDocument_ = candidate;
        publisher_ = publisher;
        result.ok = true;
        result.document = candidate;
        return result;
    }

    MappingMutationResult rollback() {
        MappingMutationResult result;
        if (!rollbackDocument_ || !publisher_) {
            result.error = "no package mapping transaction to roll back";
            return result;
        }
        bool published = false;
        try {
            published = publisher_(*rollbackDocument_);
        } catch (...) {
            published = false;
        }
        if (!published) {
            result.error = "failed to publish mapping rollback";
            result.rollbackSucceeded = false;
            result.document = currentDocument_.value_or(ofJson::object());
            return result;
        }
        result.ok = true;
        result.document = *rollbackDocument_;
        currentDocument_ = *rollbackDocument_;
        rollbackDocument_.reset();
        return result;
    }

    bool rollbackAvailable() const {
        return rollbackDocument_.has_value();
    }

private:
    std::optional<ofJson> rollbackDocument_;
    std::optional<ofJson> currentDocument_;
    Publisher publisher_;
};

struct PresetValueChange {
    std::string targetId;
    std::string kind;
    ofJson beforeBase;
    ofJson beforeLive;
    ofJson after;
    state::ParameterBaseOrigin beforeOrigin;
};

struct PresetPreview {
    bool ok = false;
    std::string error;
    std::string packageId;
    std::string presetId;
    std::vector<PresetValueChange> changes;
};

class PackagePresetTransaction {
public:
    using Publisher = std::function<bool()>;

    PresetPreview preview(
        ParameterRegistry& registry,
        const std::string& packageId,
        const std::string& presetId,
        const std::string& registryPrefix,
        const ofJson& values) {
        cancel();
        PresetPreview preview;
        preview.packageId = packageId;
        preview.presetId = presetId;
        if (!values.is_object()) {
            preview.error = "preset values must be an object";
            return preview;
        }
        registry_ = &registry;
        active_ = preview;
        const auto fail = [&](const std::string& error) {
            PresetPreview failed = *active_;
            failed.error = error;
            restore();
            active_.reset();
            registry_ = nullptr;
            return failed;
        };
        for (auto it = values.begin(); it != values.end(); ++it) {
            const std::string targetId =
                registryPrefix + "." + it.key();
            PresetValueChange change;
            change.targetId = targetId;
            change.after = it.value();
            if (auto* param = registry.findFloat(targetId)) {
                if (!it.value().is_number() ||
                    !std::isfinite(it.value().get<double>())) {
                    return fail(
                        "preset float value is invalid for " +
                        targetId);
                }
                change.kind = "float";
                change.beforeBase = param->baseValue;
                change.beforeLive =
                    param->value ? ofJson(*param->value)
                                 : ofJson(param->baseValue);
                change.beforeOrigin = param->baseOrigin;
                if (param->value) {
                    *param->value = it.value().get<float>();
                }
            } else if (auto* param = registry.findBool(targetId)) {
                if (!it.value().is_boolean()) {
                    return fail(
                        "preset bool value is invalid for " +
                        targetId);
                }
                change.kind = "bool";
                change.beforeBase = param->baseValue;
                change.beforeLive =
                    param->value ? ofJson(*param->value)
                                 : ofJson(param->baseValue);
                change.beforeOrigin = param->baseOrigin;
                if (param->value) {
                    *param->value = it.value().get<bool>();
                }
            } else if (auto* param = registry.findString(targetId)) {
                if (!it.value().is_string()) {
                    return fail(
                        "preset string value is invalid for " +
                        targetId);
                }
                change.kind = "string";
                change.beforeBase = param->baseValue;
                change.beforeLive =
                    param->value ? ofJson(*param->value)
                                 : ofJson(param->baseValue);
                change.beforeOrigin = param->baseOrigin;
                if (param->value) {
                    *param->value = it.value().get<std::string>();
                }
            } else {
                return fail(
                    "preset target is not live: " + targetId);
            }
            active_->changes.push_back(std::move(change));
        }
        active_->ok = true;
        return *active_;
    }

    bool apply(const Publisher& publisher,
               int artifactVersion,
               const std::string& artifactRevision) {
        if (!registry_ || !active_ || !active_->ok || !publisher) {
            return false;
        }
        const state::ParameterBaseOrigin origin{
            state::ParameterBaseOriginKind::Preset,
            active_->packageId + "/" + active_->presetId,
            artifactVersion,
            {},
            artifactRevision,
        };
        appliedOrigin_ = origin;
        try {
            for (const auto& change : active_->changes) {
                if (change.kind == "float") {
                    registry_->setFloatBase(
                        change.targetId,
                        change.after.get<float>(),
                        origin,
                        true);
                } else if (change.kind == "bool") {
                    registry_->setBoolBase(
                        change.targetId,
                        change.after.get<bool>(),
                        origin,
                        true);
                } else {
                    registry_->setStringBase(
                        change.targetId,
                        change.after.get<std::string>(),
                        origin,
                        true);
                }
            }
        } catch (...) {
            restore();
            return false;
        }
        bool published = false;
        try {
            published = publisher();
        } catch (...) {
            published = false;
        }
        if (!published) {
            restore();
            return false;
        }
        rollback_ = active_;
        active_.reset();
        return true;
    }

    void cancel() {
        if (active_) {
            restore();
        }
        active_.reset();
        registry_ = nullptr;
    }

    bool rollback() {
        return rollback(Publisher{[]() { return true; }});
    }

    bool rollback(const Publisher& publisher) {
        if (!registry_ || !rollback_) {
            return false;
        }
        active_ = rollback_;
        restore();
        bool published = false;
        try {
            published = publisher && publisher();
        } catch (...) {
            published = false;
        }
        if (!published) {
            for (const auto& change : active_->changes) {
                if (change.kind == "float") {
                    registry_->setFloatBase(
                        change.targetId,
                        change.after.get<float>(),
                        appliedOrigin_,
                        true);
                } else if (change.kind == "bool") {
                    registry_->setBoolBase(
                        change.targetId,
                        change.after.get<bool>(),
                        appliedOrigin_,
                        true);
                } else {
                    registry_->setStringBase(
                        change.targetId,
                        change.after.get<std::string>(),
                        appliedOrigin_,
                        true);
                }
            }
            active_.reset();
            return false;
        }
        rollback_.reset();
        active_.reset();
        return true;
    }

    bool previewActive() const {
        return active_.has_value() && active_->ok;
    }

    bool rollbackAvailable() const {
        return rollback_.has_value();
    }

private:
    void restore() {
        if (!registry_ || !active_) {
            return;
        }
        for (const auto& change : active_->changes) {
            if (change.kind == "float") {
                registry_->setFloatBase(
                    change.targetId,
                    change.beforeBase.get<float>(),
                    change.beforeOrigin,
                    false);
                if (auto* param =
                        registry_->findFloat(change.targetId);
                    param && param->value) {
                    *param->value =
                        change.beforeLive.get<float>();
                }
            } else if (change.kind == "bool") {
                registry_->setBoolBase(
                    change.targetId,
                    change.beforeBase.get<bool>(),
                    change.beforeOrigin,
                    false);
                if (auto* param =
                        registry_->findBool(change.targetId);
                    param && param->value) {
                    *param->value =
                        change.beforeLive.get<bool>();
                }
            } else {
                registry_->setStringBase(
                    change.targetId,
                    change.beforeBase.get<std::string>(),
                    change.beforeOrigin,
                    false);
                if (auto* param =
                        registry_->findString(change.targetId);
                    param && param->value) {
                    *param->value =
                        change.beforeLive.get<std::string>();
                }
            }
        }
    }

    ParameterRegistry* registry_ = nullptr;
    std::optional<PresetPreview> active_;
    std::optional<PresetPreview> rollback_;
    state::ParameterBaseOrigin appliedOrigin_;
};

}  // namespace synaptome::controls

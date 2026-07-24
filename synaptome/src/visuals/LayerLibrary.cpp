#include "LayerLibrary.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include <algorithm>
#include <filesystem>
#include <vector>

namespace {
    void collectJsonFiles(const ofDirectory& dir, std::vector<ofFile>& out) {
        ofDirectory listing(dir);
        listing.listDir();
        for (const auto& entry : listing.getFiles()) {
            if (entry.isDirectory()) {
                collectJsonFiles(ofDirectory(entry.getAbsolutePath()), out);
            } else if (entry.getExtension() == "json") {
                out.push_back(entry);
            }
        }
    }

    bool compatibleJsonValue(const ofJson& expected, const ofJson& actual) {
        if (expected.is_number() && actual.is_number()) {
            return true;
        }
        return expected.type() == actual.type();
    }

    bool presetInBank(const ofJson& cfg,
                      const std::string& requestedBankId,
                      const std::string& presetId,
                      std::string& resolvedBankId) {
        const auto banks = cfg.value("presetBanks", ofJson::array());
        if (!banks.is_array()) {
            return requestedBankId.empty();
        }
        for (const auto& bank : banks) {
            if (!bank.is_object()) {
                continue;
            }
            const std::string bankId = bank.value("id", std::string());
            if (!requestedBankId.empty() && bankId != requestedBankId) {
                continue;
            }
            const auto presetIds = bank.value("presets", ofJson::array());
            if (!presetIds.is_array()) {
                continue;
            }
            for (const auto& item : presetIds) {
                if (item.is_string() && item.get<std::string>() == presetId) {
                    resolvedBankId = bankId;
                    return true;
                }
            }
            if (!requestedBankId.empty() && bankId == requestedBankId) {
                return false;
            }
        }
        return requestedBankId.empty() && banks.empty();
    }

    bool mergePackageDefaults(const ofJson& baseDefaults,
                              const ofJson& presets,
                              const std::string& presetId,
                              const ofJson& explicitOverrides,
                              ofJson& mergedDefaults,
                              std::string* error) {
        auto fail = [&](const std::string& message) {
            if (error) {
                *error = message;
            }
            return false;
        };
        if (!baseDefaults.is_object() || !presets.is_object() || !explicitOverrides.is_object()) {
            return fail("package preset sources must be JSON objects");
        }
        mergedDefaults = baseDefaults;
        if (!presetId.empty()) {
            if (!presets.contains(presetId) || !presets[presetId].is_object()) {
                return fail("unknown package preset '" + presetId + "'");
            }
            for (auto it = presets[presetId].begin(); it != presets[presetId].end(); ++it) {
                if (!baseDefaults.contains(it.key()) ||
                    !compatibleJsonValue(baseDefaults[it.key()], it.value())) {
                    return fail("invalid preset parameter '" + it.key() + "'");
                }
                mergedDefaults[it.key()] = it.value();
            }
        }
        for (auto it = explicitOverrides.begin(); it != explicitOverrides.end(); ++it) {
            if (!baseDefaults.contains(it.key()) ||
                !compatibleJsonValue(baseDefaults[it.key()], it.value())) {
                return fail("invalid activation parameter '" + it.key() + "'");
            }
            mergedDefaults[it.key()] = it.value();
        }
        return true;
    }
}

bool LayerLibrary::reload(const std::string& rootDir) {
    entries_.clear();

    ofDirectory dir(rootDir);
    if (!dir.exists()) {
        ofLogWarning("LayerLibrary") << "layer directory missing: " << rootDir;
        return false;
    }

    std::vector<ofFile> files;
    collectJsonFiles(dir, files);

    for (const auto& file : files) {
        ofJson cfg;
        try {
            cfg = ofLoadJson(file.getAbsolutePath());
        } catch (const std::exception& e) {
            ofLogWarning("LayerLibrary") << "failed to parse " << file.getAbsolutePath() << ": " << e.what();
            continue;
        }

        appendConfig(cfg, file.getAbsolutePath());
    }

    sortEntries();
    return !entries_.empty();
}

bool LayerLibrary::appendConfig(const ofJson& cfg, const std::string& configPath) {
        if (!cfg.contains("id") || !cfg.contains("type")) {
            ofLogWarning("LayerLibrary") << "skipping layer config missing required fields: " << configPath;
            return false;
        }

        Entry entry;
        entry.id = cfg.value("id", std::filesystem::path(configPath).stem().string());
        entry.label = cfg.value("label", entry.id);
        entry.category = cfg.value("category", std::string("Unsorted"));
        entry.layerGroup = cfg.value("layerGroup", std::string());
        entry.model = cfg.value("model", std::string());
        entry.stateModel = cfg.value("stateModel", std::string());
        entry.type = cfg.value("type", std::string());
        entry.registryPrefix = cfg.value("registryPrefix", entry.id);
        double rawOpacity = 1.0;
        if (cfg.contains("opacity")) {
            if (cfg["opacity"].is_number()) {
                rawOpacity = cfg["opacity"].get<double>();
            } else {
                ofLogWarning("LayerLibrary") << "opacity for " << entry.id << " must be numeric: " << configPath;
            }
        }
        rawOpacity = std::clamp(rawOpacity, 0.0, 1.0);
        entry.opacity = static_cast<float>(rawOpacity);
        entry.configPath = configPath;
        entry.config = cfg;
        if (cfg.contains("modes") && cfg["modes"].is_array()) {
            for (const auto& modeNode : cfg["modes"]) {
                if (!modeNode.is_object()) {
                    continue;
                }
                Entry::ModeInfo mode;
                mode.id = modeNode.value("id", std::string());
                mode.label = modeNode.value("label", mode.id);
                mode.kind = modeNode.value("kind", std::string());
                mode.description = modeNode.value("description", std::string());
                mode.live = modeNode.value("live", mode.kind == "live");
                if (!mode.id.empty()) {
                    entry.modes.push_back(std::move(mode));
                }
            }
        }
        if (cfg.contains("coverage") && cfg["coverage"].is_object()) {
            const auto& coverageNode = cfg["coverage"];
            entry.coverage.defined = true;
            entry.coverage.mode = coverageNode.value("mode", entry.coverage.mode);
            if (coverageNode.contains("columns") && coverageNode["columns"].is_number_integer()) {
                entry.coverage.columns = std::max(0, coverageNode["columns"].get<int>());
            }
        }
        if (cfg.contains("hudWidget") && cfg["hudWidget"].is_object()) {
            const auto& hudNode = cfg["hudWidget"];
            std::string module = hudNode.value("module", std::string());
            if (!module.empty()) {
                entry.hud.enabled = true;
                entry.hud.module = module;
                entry.hud.toggleId = hudNode.value("toggleId", entry.registryPrefix);
                entry.hud.defaultBand = hudNode.value("defaultBand", std::string("hud"));
                entry.hud.defaultColumn = hudNode.value("defaultColumn", 0);
                if (hudNode.contains("telemetry") && hudNode["telemetry"].is_array()) {
                    for (const auto& feed : hudNode["telemetry"]) {
                        if (feed.is_string()) {
                            entry.hud.telemetryFeeds.push_back(feed.get<std::string>());
                        }
                    }
                }
            }
        }

        const auto duplicate = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& existing) {
            return existing.id == entry.id;
        });
        if (duplicate != entries_.end()) {
            ofLogWarning("LayerLibrary") << "duplicate layer id '" << entry.id << "' from " << configPath;
            return false;
        }
        entries_.push_back(std::move(entry));
        return true;
}

void LayerLibrary::sortEntries() {
    std::sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        if (a.category == b.category) {
            if (a.layerGroup != b.layerGroup) {
                return a.layerGroup < b.layerGroup;
            }
            return a.label < b.label;
        }
        return a.category < b.category;
    });
}

bool LayerLibrary::loadOptInPackages(const std::string& activationPath) {
    if (!ofFile::doesFileExist(activationPath)) {
        return true;
    }

    ofJson activation;
    try {
        activation = ofLoadJson(activationPath);
    } catch (const std::exception& e) {
        ofLogWarning("LayerLibrary") << "failed to parse package activation " << activationPath << ": " << e.what();
        return false;
    }
    if (!activation.value("enabled", false)) {
        return true;
    }
    if (!activation.contains("packages") || !activation["packages"].is_array()) {
        ofLogWarning("LayerLibrary") << "enabled package activation requires packages[]: " << activationPath;
        return false;
    }

    bool valid = true;
    const std::filesystem::path activationDir = std::filesystem::path(activationPath).parent_path();
    for (const auto& package : activation["packages"]) {
        bool packageValid = true;
        if (!package.is_object() || !package.value("enabled", false)) {
            continue;
        }
        const std::string packageId = package.value("id", std::string());
        const std::string rawCatalogPath = package.value("catalogPath", std::string());
        if (packageId.empty() || rawCatalogPath.empty()) {
            ofLogWarning("LayerLibrary") << "enabled package entry requires id and catalogPath";
            valid = false;
            continue;
        }
        const std::filesystem::path catalogPath = (activationDir / rawCatalogPath).lexically_normal();
        ofJson cfg;
        try {
            cfg = ofLoadJson(catalogPath.string());
        } catch (const std::exception& e) {
            ofLogWarning("LayerLibrary") << "failed to parse opt-in package catalog " << catalogPath.string() << ": " << e.what();
            valid = false;
            continue;
        }
        if (cfg.value("id", std::string()) != packageId) {
            ofLogWarning("LayerLibrary") << "package activation id does not match catalog id: " << packageId;
            valid = false;
            continue;
        }

        // Package defaults are the base. A named preset may replace individual
        // defaults, and explicit activation overrides win last. Scene values
        // still win later through the existing scene-load path.
        const ofJson packageDefaults = cfg.value("defaults", ofJson::object());
        const ofJson presets = cfg.value("presets", ofJson::object());
        const ofJson explicitOverrides =
            package.contains("parameters") && package["parameters"].is_object()
                ? package["parameters"]
                : ofJson::object();
        const std::string presetId = package.value("preset", std::string());
        std::string presetBankId = package.value("presetBank", std::string());
        if (!presetId.empty() &&
            !presetInBank(cfg, presetBankId, presetId, presetBankId)) {
            ofLogWarning("LayerLibrary") << "preset '" << presetId
                                           << "' is not in package bank '"
                                           << package.value("presetBank", std::string())
                                           << "' for " << packageId;
            valid = false;
            continue;
        }
        ofJson mergedDefaults;
        std::string mergeError;
        if (!mergePackageDefaults(packageDefaults,
                                  presets,
                                  presetId,
                                  explicitOverrides,
                                  mergedDefaults,
                                  &mergeError)) {
            ofLogWarning("LayerLibrary") << mergeError << " for " << packageId;
            valid = false;
            packageValid = false;
        }
        if (!packageValid) {
            continue;
        }
        const std::string mappingPresetId = package.value("mappingPreset", std::string());
        if (!mappingPresetId.empty()) {
            bool mappingFound = false;
            const auto mappingPresets = cfg.value("mappingPresets", ofJson::array());
            for (const auto& mappingPreset : mappingPresets) {
                if (mappingPreset.is_object() && mappingPreset.value("id", std::string()) == mappingPresetId) {
                    mappingFound = true;
                    break;
                }
            }
            if (!mappingFound) {
                ofLogWarning("LayerLibrary") << "unknown package mapping preset '"
                                               << mappingPresetId << "' for " << packageId;
                valid = false;
                packageValid = false;
                continue;
            }
        }
        cfg["defaults"] = std::move(mergedDefaults);
        cfg["packageId"] = packageId;
        cfg["packageActivation"] = {
            { "preset", presetId },
            { "presetBank", presetBankId },
            { "mappingPreset", mappingPresetId },
            { "mappingApplied", false },
            { "baseDefaults", packageDefaults },
            { "parameters", explicitOverrides }
        };
        if (!mappingPresetId.empty()) {
            ofLogNotice("LayerLibrary") << "mapping preset '" << mappingPresetId
                                         << "' for " << packageId
                                         << " is visible but not auto-applied; scene/operator mappings retain ownership";
        }
        if (!appendConfig(cfg, catalogPath.string())) {
            valid = false;
        }
    }
    sortEntries();
    return valid;
}

const LayerLibrary::Entry* LayerLibrary::find(const std::string& id) const {
    auto it = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
        return entry.id == id;
    });
    return it != entries_.end() ? &(*it) : nullptr;
}

bool LayerLibrary::configForPackagePreset(const std::string& assetId,
                                          const std::string& bankId,
                                          const std::string& presetId,
                                          ofJson& resolvedConfig,
                                          std::string* error) const {
    auto fail = [&](const std::string& message) {
        if (error) {
            *error = message;
        }
        return false;
    };
    const Entry* entry = find(assetId);
    if (!entry) {
        return fail("unknown package asset '" + assetId + "'");
    }
    const auto activation = entry->config.value("packageActivation", ofJson::object());
    if (!activation.is_object() || !activation.contains("baseDefaults")) {
        return fail("asset is not an activated package");
    }
    std::string resolvedBankId = bankId;
    if (presetId.empty() ||
        !presetInBank(entry->config, bankId, presetId, resolvedBankId)) {
        return fail("preset '" + presetId + "' is not in bank '" + bankId + "'");
    }
    ofJson mergedDefaults;
    if (!mergePackageDefaults(
            activation.value("baseDefaults", ofJson::object()),
            entry->config.value("presets", ofJson::object()),
            presetId,
            activation.value("parameters", ofJson::object()),
            mergedDefaults,
            error)) {
        return false;
    }
    resolvedConfig = entry->config;
    resolvedConfig["defaults"] = std::move(mergedDefaults);
    resolvedConfig["packageActivation"]["preset"] = presetId;
    resolvedConfig["packageActivation"]["presetBank"] = resolvedBankId;
    return true;
}

#include "MidiRouter.h"
#include "ofFileUtils.h"
#include "ofUtils.h"
#include "ofLog.h"
#include <algorithm>
#include <map>
#include <unordered_map>
#include <utility>
#include <cmath>

namespace {
    float map01ToRange(float v01, float outMin, float outMax) {
        return ofLerp(outMin, outMax, ofClamp(v01, 0.0f, 1.0f));
    }

    modifier::BlendMode blendModeFromString(const std::string& value, modifier::BlendMode fallback = modifier::BlendMode::kScale) {
        if (value == "add" || value == "additive") return modifier::BlendMode::kAdditive;
        if (value == "absolute" || value == "abs") return modifier::BlendMode::kAbsolute;
        if (value == "scale" || value == "mul" || value == "multiply") return modifier::BlendMode::kScale;
        if (value == "clamp" || value == "limit") return modifier::BlendMode::kClamp;
        if (value == "toggle") return modifier::BlendMode::kToggle;
        return fallback;
    }

    std::string blendModeToString(modifier::BlendMode blend) {
        switch (blend) {
        case modifier::BlendMode::kAdditive: return "additive";
        case modifier::BlendMode::kAbsolute: return "absolute";
        case modifier::BlendMode::kScale: return "scale";
        case modifier::BlendMode::kClamp: return "clamp";
        case modifier::BlendMode::kToggle: return "toggle";
        }
        return "scale";
    }

    const std::map<std::string, std::vector<std::string>> kDeviceMetricOrder = {
        { "matrix", { "mic-level", "mic-peak", "mic-gain",
                      "bioamp-raw", "bioamp-signal", "bioamp-mean", "bioamp-rms", "bioamp-dom-hz",
                      "bioamp-sample-rate", "bioamp-window",
                      "battery-soc", "battery-volt", "rssi" } },
        { "deck",   { "deck-intensity", "deck-scene", "deck_paramx", "battery-soc", "battery-volt", "rssi" } },
        { "hr",     { "heart-bpm", "hr-conf", "battery-soc", "battery-volt", "rssi" } },
        { "eeg",    { "eeg-alpha", "eeg-beta", "eeg-gamma", "battery-soc", "battery-volt", "rssi" } },
        { "env",    { "battery-soc", "battery-volt", "rssi" } }
    };

    bool parseSensorAddress(const std::string& address, std::string& deviceType, std::string& deviceId, std::string& metric) {
        auto parts = ofSplitString(address, "/", true, true);
        if (parts.size() < 4) return false;
        if (parts[0] != "sensor") return false;
        deviceType = parts[1];
        deviceId = parts[2];
        metric = parts[3];
        for (std::size_t i = 4; i < parts.size(); ++i) {
            metric += "/" + parts[i];
        }
        return true;
    }

    void copyOscProfileToMap(const MidiRouter::OscSourceProfile& profile, MidiRouter::OscMap& map) {
        map.inMin = profile.inMin;
        map.inMax = profile.inMax;
        map.outMin = profile.outMin;
        map.outMax = profile.outMax;
        map.smooth = profile.smooth;
        map.deadband = profile.deadband;
        map.blend = profile.blend;
        map.relativeToBase = profile.relativeToBase;
    }

    void upsertOscSourceProfile(std::vector<MidiRouter::OscSourceProfile>& profiles,
                                const MidiRouter::OscSourceProfile& profile) {
        if (profile.pattern.empty()) {
            return;
        }
        auto it = std::find_if(profiles.begin(), profiles.end(), [&](const MidiRouter::OscSourceProfile& existing) {
            return existing.pattern == profile.pattern;
        });
        if (it != profiles.end()) {
            *it = profile;
        } else {
            profiles.push_back(profile);
        }
    }

    void upsertOscMap(std::vector<MidiRouter::OscMap>& maps, const MidiRouter::OscMap& map) {
        if (map.target.empty() || map.pattern.empty()) {
            return;
        }
        auto it = std::find_if(maps.begin(), maps.end(), [&](const MidiRouter::OscMap& existing) {
            return existing.target == map.target;
        });
        if (it != maps.end()) {
            *it = map;
            maps.erase(std::remove_if(std::next(it), maps.end(), [&](const MidiRouter::OscMap& existing) {
                           return existing.target == map.target;
                       }),
                       maps.end());
        } else {
            maps.push_back(map);
        }
    }

    void canonicalizeOscState(std::vector<MidiRouter::OscMap>& maps,
                              std::vector<MidiRouter::OscSourceProfile>& profiles) {
        std::vector<MidiRouter::OscSourceProfile> canonicalProfiles;
        canonicalProfiles.reserve(profiles.size());
        for (const auto& profile : profiles) {
            upsertOscSourceProfile(canonicalProfiles, profile);
        }
        profiles.swap(canonicalProfiles);

        std::vector<MidiRouter::OscMap> canonicalMaps;
        canonicalMaps.reserve(maps.size());
        for (const auto& map : maps) {
            upsertOscMap(canonicalMaps, map);
        }
        maps.swap(canonicalMaps);
    }

    ofJson buildMappingSnapshot(const std::vector<MidiRouter::CcMap>& ccMaps,
                                const std::vector<MidiRouter::BtnMap>& btnMaps,
                                const std::vector<MidiRouter::OscMap>& oscMaps,
                                const std::vector<MidiRouter::OscSourceProfile>& oscSourceProfiles) {
        ofJson doc = ofJson::object();
        std::vector<MidiRouter::OscMap> canonicalOscMaps = oscMaps;
        std::vector<MidiRouter::OscSourceProfile> canonicalOscProfiles = oscSourceProfiles;
        canonicalizeOscState(canonicalOscMaps, canonicalOscProfiles);

        doc["cc"] = ofJson::array();
        for (const auto& map : ccMaps) {
            if (map.target.empty()) continue;
            ofJson entry;
            entry["num"] = map.cc;
            if (map.channel >= 0) entry["channel"] = map.channel;
            entry["target"] = map.target;
            if (!map.bankId.empty()) entry["bank"] = map.bankId;
            if (!map.controlId.empty() && map.controlId != map.target) entry["control"] = map.controlId;
            if (!map.deviceId.empty()) entry["device"] = map.deviceId;
            if (!map.columnId.empty()) entry["column"] = map.columnId;
            if (!map.slotId.empty()) entry["slot"] = map.slotId;
            entry["out"] = { map.outMin, map.outMax };
            if (map.snapInt) entry["snapInt"] = true;
            if (map.step > 0.0f) entry["step"] = map.step;
            doc["cc"].push_back(entry);
        }

        if (!btnMaps.empty()) {
            doc["buttons"] = ofJson::array();
            for (const auto& map : btnMaps) {
                if (map.target.empty()) continue;
                ofJson entry;
                entry["num"] = map.num;
                entry["type"] = map.type;
                if (map.channel >= 0) entry["channel"] = map.channel;
                entry["target"] = map.target;
                if (!map.bankId.empty()) entry["bank"] = map.bankId;
                if (!map.controlId.empty() && map.controlId != map.target) entry["control"] = map.controlId;
                if (!map.deviceId.empty()) entry["device"] = map.deviceId;
                if (!map.columnId.empty()) entry["column"] = map.columnId;
                if (!map.slotId.empty()) entry["slot"] = map.slotId;
                entry["setValue"] = map.setValue;
                doc["buttons"].push_back(entry);
            }
        }

        if (!canonicalOscProfiles.empty()) {
            doc["oscSources"] = ofJson::array();
            for (const auto& profile : canonicalOscProfiles) {
                if (profile.pattern.empty()) continue;
                ofJson entry;
                entry["pattern"] = profile.pattern;
                entry["in"] = { profile.inMin, profile.inMax };
                entry["out"] = { profile.outMin, profile.outMax };
                if (profile.smooth != 0.2f) entry["smooth"] = profile.smooth;
                if (profile.deadband != 0.0f) entry["deadband"] = profile.deadband;
                entry["blend"] = blendModeToString(profile.blend);
                entry["relative"] = profile.relativeToBase;
                doc["oscSources"].push_back(entry);
            }
        }

        if (!canonicalOscMaps.empty()) {
            doc["osc"] = ofJson::array();
            for (const auto& map : canonicalOscMaps) {
                if (map.target.empty() || map.pattern.empty()) continue;
                ofJson entry;
                entry["pattern"] = map.pattern;
                entry["target"] = map.target;
                if (!map.bankId.empty()) entry["bank"] = map.bankId;
                if (!map.controlId.empty() && map.controlId != map.target) entry["control"] = map.controlId;
                doc["osc"].push_back(entry);
            }
        }

        return doc;
    }
}

void MidiRouter::bindFloat(const std::string& name, float* ptr, float defMin, float defMax, bool snapInt, float step, const std::string& bankId, const std::string& controlId) {
    if (!ptr) return;
    FloatTarget target;
    target.ptr = ptr;
    target.defMin = defMin;
    target.defMax = defMax;
    target.snapInt = snapInt;
    target.step = step;
    target.defaultBankId = bankId;
    target.defaultControlId = controlId.empty() ? name : controlId;
    floatTargets[name] = target;

    for (auto& map : ccMaps) {
        if (map.target == name) {
            if (map.outMin == 0.0f && map.outMax == 1.0f && (defMin != 0.0f || defMax != 1.0f)) {
                map.outMin = defMin;
                map.outMax = defMax;
            }
            if (snapInt) map.snapInt = true;
            if (step > 0.0f && map.step == 0.0f) map.step = step;
            if (map.bankId.empty() && !bankId.empty()) map.bankId = bankId;
            if (map.controlId.empty()) map.controlId = target.defaultControlId;
            break;
        }
    }
}

void MidiRouter::bindBool(const std::string& name, bool* ptr, BoolMode mode, const std::string& bankId, const std::string& controlId) {
    if (!ptr) return;
    BoolTarget target;
    target.ptr = ptr;
    target.mode = mode;
    target.lastHigh = false;
    target.defaultBankId = bankId;
    target.defaultControlId = controlId.empty() ? name : controlId;
    boolTargets[name] = target;
}

bool MidiRouter::load(const std::string& jsonPath) {
    close();

    mappingPath = jsonPath;
    deviceName.clear();
    deviceIndex = -1;
    currentPortLabel.clear();
    ccMaps.clear();
    btnMaps.clear();
    oscMaps.clear();
    oscSourceProfiles.clear();

    ofJson doc;
    if (ofFile::doesFileExist(jsonPath)) {
        try {
            doc = ofLoadJson(jsonPath);
        } catch (const std::exception& e) {
            ofLogError("MidiRouter") << "Failed to parse " << jsonPath << ": " << e.what();
        }
    } else {
        ofLogNotice("MidiRouter") << "Mapping file " << jsonPath << " not found; starting empty";
    }

    if (!doc.is_null()) {
        if (doc.contains("device") && doc["device"].is_string()) {
            deviceName = doc["device"].get<std::string>();
        }
        if (doc.contains("deviceIndex") && doc["deviceIndex"].is_number_integer()) {
            deviceIndex = doc["deviceIndex"].get<int>();
        }

        if (doc.contains("cc") && doc["cc"].is_array()) {
            for (auto& entry : doc["cc"]) {
                CcMap map;
                if (entry.contains("num")) map.cc = entry["num"].get<int>();
                if (entry.contains("channel")) map.channel = entry["channel"].get<int>();
                if (entry.contains("target")) map.target = entry["target"].get<std::string>();
                if (entry.contains("bank")) map.bankId = entry["bank"].get<std::string>();
                if (entry.contains("control")) map.controlId = entry["control"].get<std::string>();
                if (entry.contains("device")) map.deviceId = entry["device"].get<std::string>();
                if (entry.contains("column")) map.columnId = entry["column"].get<std::string>();
                if (entry.contains("slot")) map.slotId = entry["slot"].get<std::string>();
                if (entry.contains("out") && entry["out"].is_array() && entry["out"].size() == 2) {
                    map.outMin = entry["out"][0].get<float>();
                    map.outMax = entry["out"][1].get<float>();
                }
                if (entry.contains("snapInt")) map.snapInt = entry["snapInt"].get<bool>();
                if (entry.contains("step")) map.step = entry["step"].get<float>();
                if (!map.target.empty()) {
                    ccMaps.push_back(map);
                }
            }
        }

        if (doc.contains("buttons") && doc["buttons"].is_array()) {
            for (auto& entry : doc["buttons"]) {
                BtnMap map;
                if (entry.contains("num")) map.num = entry["num"].get<int>();
                if (entry.contains("channel")) map.channel = entry["channel"].get<int>();
                if (entry.contains("type")) map.type = entry["type"].get<std::string>();
                if (entry.contains("target")) map.target = entry["target"].get<std::string>();
                if (entry.contains("bank")) map.bankId = entry["bank"].get<std::string>();
                if (entry.contains("control")) map.controlId = entry["control"].get<std::string>();
                if (entry.contains("device")) map.deviceId = entry["device"].get<std::string>();
                if (entry.contains("column")) map.columnId = entry["column"].get<std::string>();
                if (entry.contains("slot")) map.slotId = entry["slot"].get<std::string>();
                if (entry.contains("setValue")) map.setValue = entry["setValue"].get<float>();
                if (!map.target.empty()) {
                    btnMaps.push_back(map);
                }
            }
        }

        if (doc.contains("oscSources") && doc["oscSources"].is_array()) {
            for (auto& entry : doc["oscSources"]) {
                OscSourceProfile profile;
                if (entry.contains("pattern")) profile.pattern = entry["pattern"].get<std::string>();
                if (entry.contains("in") && entry["in"].is_array() && entry["in"].size() == 2) {
                    profile.inMin = entry["in"][0].get<float>();
                    profile.inMax = entry["in"][1].get<float>();
                }
                if (entry.contains("out") && entry["out"].is_array() && entry["out"].size() == 2) {
                    profile.outMin = entry["out"][0].get<float>();
                    profile.outMax = entry["out"][1].get<float>();
                }
                if (entry.contains("smooth")) profile.smooth = entry["smooth"].get<float>();
                if (entry.contains("deadband")) profile.deadband = entry["deadband"].get<float>();
                if (entry.contains("blend") && entry["blend"].is_string()) {
                    profile.blend = blendModeFromString(entry["blend"].get<std::string>(), modifier::BlendMode::kScale);
                } else if (entry.contains("mode") && entry["mode"].is_string()) {
                    profile.blend = blendModeFromString(entry["mode"].get<std::string>(), modifier::BlendMode::kScale);
                }
                if (entry.contains("relative")) {
                    profile.relativeToBase = entry["relative"].get<bool>();
                }
                upsertOscSourceProfile(oscSourceProfiles, profile);
            }
        }

        if (doc.contains("osc") && doc["osc"].is_array()) {
            for (auto& entry : doc["osc"]) {
                OscMap m;
                if (entry.contains("pattern")) m.pattern = entry["pattern"].get<std::string>();
                if (entry.contains("target")) m.target = entry["target"].get<std::string>();
                if (entry.contains("bank")) m.bankId = entry["bank"].get<std::string>();
                if (entry.contains("control")) m.controlId = entry["control"].get<std::string>();
                if (!m.target.empty() && !m.pattern.empty()) {
                    auto* profile = ensureOscSourceProfile(m.pattern);
                    if (profile && entry.contains("in") && entry["in"].is_array() && entry["in"].size() == 2) {
                        profile->inMin = entry["in"][0].get<float>();
                        profile->inMax = entry["in"][1].get<float>();
                    }
                    if (profile && entry.contains("out") && entry["out"].is_array() && entry["out"].size() == 2) {
                        profile->outMin = entry["out"][0].get<float>();
                        profile->outMax = entry["out"][1].get<float>();
                    }
                    if (profile && entry.contains("smooth")) profile->smooth = entry["smooth"].get<float>();
                    if (profile && entry.contains("deadband")) profile->deadband = entry["deadband"].get<float>();
                    if (profile && entry.contains("blend") && entry["blend"].is_string()) {
                        profile->blend = blendModeFromString(entry["blend"].get<std::string>(), modifier::BlendMode::kScale);
                    } else if (profile && entry.contains("mode") && entry["mode"].is_string()) {
                        profile->blend = blendModeFromString(entry["mode"].get<std::string>(), modifier::BlendMode::kScale);
                    }
                    if (profile && entry.contains("relative")) {
                        profile->relativeToBase = entry["relative"].get<bool>();
                    }
                    upsertOscMap(oscMaps, m);
                }
            }
        }
    }

    canonicalizeOscState(oscMaps, oscSourceProfiles);

    for (auto& map : ccMaps) {
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
        auto fit = floatTargets.find(map.target);
        if (fit != floatTargets.end()) {
            const auto& meta = fit->second;
            if (map.outMin == 0.0f && map.outMax == 1.0f && (meta.defMin != 0.0f || meta.defMax != 1.0f)) {
                map.outMin = meta.defMin;
                map.outMax = meta.defMax;
            }
            if (meta.snapInt) map.snapInt = true;
            if (meta.step > 0.0f && map.step == 0.0f) map.step = meta.step;
            if (map.bankId.empty() && !meta.defaultBankId.empty()) {
                map.bankId = meta.defaultBankId;
            }
            if (map.controlId.empty() && !meta.defaultControlId.empty()) {
                map.controlId = meta.defaultControlId;
            }
        }
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
    }
    for (auto& map : btnMaps) {
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
        auto it = boolTargets.find(map.target);
        if (it != boolTargets.end()) {
            const auto& meta = it->second;
            if (map.bankId.empty() && !meta.defaultBankId.empty()) {
                map.bankId = meta.defaultBankId;
            }
            if (map.controlId.empty() && !meta.defaultControlId.empty()) {
                map.controlId = meta.defaultControlId;
            }
        }
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
    }
    for (auto& m : oscMaps) {
        if (m.controlId.empty()) {
            m.controlId = m.target;
        }
        if (auto* profile = ensureOscSourceProfile(m.pattern)) {
            copyOscProfileToMap(*profile, m);
        }
    }
    for (auto& entry : boolTargets) {
        entry.second.lastHigh = false;
    }

    if (!activeBankId.empty()) {
        std::string current = activeBankId;
        activeBankId.clear();
        setActiveBank(current);
    }

    listPortsToLog();

    if (!openPreferredPort()) {
        ofLogWarning("MidiRouter") << "No MIDI input device could be opened (will retry automatically)";
    } else {
        ofLogNotice("MidiRouter") << "Loaded " << jsonPath << "  cc: " << ccMaps.size()
                                   << " buttons: " << btnMaps.size() << " osc: " << oscMaps.size();
    }
    return true;
}

bool MidiRouter::save(const std::string& jsonPath) {
    std::string outPath = jsonPath.empty() ? mappingPath : jsonPath;
    if (outPath.empty()) {
        ofLogError("MidiRouter") << "save(): no path available";
        return false;
    }

    ofJson doc = exportMappingSnapshot();
    if (!deviceName.empty()) {
        doc["device"] = deviceName;
    } else if (deviceIndex >= 0) {
        doc["deviceIndex"] = deviceIndex;
    }

    std::string tmpPath = outPath + ".tmp";
    try {
        ofSavePrettyJson(tmpPath, doc);
    } catch (const std::exception& e) {
        ofLogError("MidiRouter") << "Failed to save " << tmpPath << ": " << e.what();
        ofFile::removeFile(tmpPath, false);
        return false;
    }

    if (!ofFile::moveFromTo(tmpPath, outPath, true, true)) {
        ofLogError("MidiRouter") << "Failed to commit mapping file to " << outPath;
        ofFile::removeFile(tmpPath, false);
        return false;
    }

    ofLogNotice("MidiRouter") << "Saved mapping to " << outPath;
    return true;
}

ofJson MidiRouter::exportMappingSnapshot() const {
    return buildMappingSnapshot(ccMaps, btnMaps, oscMaps, oscSourceProfiles);
}

bool MidiRouter::importMappingSnapshot(const ofJson& snapshot, bool replaceExisting) {
    if (!snapshot.is_object() && !snapshot.is_null()) {
        ofLogWarning("MidiRouter") << "Ignoring invalid mapping snapshot";
        return false;
    }

    if (replaceExisting) {
        ccMaps.clear();
        btnMaps.clear();
        oscMaps.clear();
        oscSourceProfiles.clear();
    }

    if (!snapshot.is_object()) {
        for (auto& entry : boolTargets) {
            entry.second.lastHigh = false;
        }
        return true;
    }

    if (snapshot.contains("cc") && snapshot["cc"].is_array()) {
        for (const auto& entry : snapshot["cc"]) {
            CcMap map;
            if (entry.contains("num")) map.cc = entry["num"].get<int>();
            if (entry.contains("channel")) map.channel = entry["channel"].get<int>();
            if (entry.contains("target")) map.target = entry["target"].get<std::string>();
            if (entry.contains("bank")) map.bankId = entry["bank"].get<std::string>();
            if (entry.contains("control")) map.controlId = entry["control"].get<std::string>();
            if (entry.contains("device")) map.deviceId = entry["device"].get<std::string>();
            if (entry.contains("column")) map.columnId = entry["column"].get<std::string>();
            if (entry.contains("slot")) map.slotId = entry["slot"].get<std::string>();
            if (entry.contains("out") && entry["out"].is_array() && entry["out"].size() == 2) {
                map.outMin = entry["out"][0].get<float>();
                map.outMax = entry["out"][1].get<float>();
            }
            if (entry.contains("snapInt")) map.snapInt = entry["snapInt"].get<bool>();
            if (entry.contains("step")) map.step = entry["step"].get<float>();
            if (!map.target.empty()) {
                ccMaps.push_back(std::move(map));
            }
        }
    }

    if (snapshot.contains("buttons") && snapshot["buttons"].is_array()) {
        for (const auto& entry : snapshot["buttons"]) {
            BtnMap map;
            if (entry.contains("num")) map.num = entry["num"].get<int>();
            if (entry.contains("channel")) map.channel = entry["channel"].get<int>();
            if (entry.contains("type")) map.type = entry["type"].get<std::string>();
            if (entry.contains("target")) map.target = entry["target"].get<std::string>();
            if (entry.contains("bank")) map.bankId = entry["bank"].get<std::string>();
            if (entry.contains("control")) map.controlId = entry["control"].get<std::string>();
            if (entry.contains("device")) map.deviceId = entry["device"].get<std::string>();
            if (entry.contains("column")) map.columnId = entry["column"].get<std::string>();
            if (entry.contains("slot")) map.slotId = entry["slot"].get<std::string>();
            if (entry.contains("setValue")) map.setValue = entry["setValue"].get<float>();
            if (!map.target.empty()) {
                btnMaps.push_back(std::move(map));
            }
        }
    }

    if (snapshot.contains("oscSources") && snapshot["oscSources"].is_array()) {
        for (const auto& entry : snapshot["oscSources"]) {
            OscSourceProfile profile;
            if (entry.contains("pattern")) profile.pattern = entry["pattern"].get<std::string>();
            if (entry.contains("in") && entry["in"].is_array() && entry["in"].size() == 2) {
                profile.inMin = entry["in"][0].get<float>();
                profile.inMax = entry["in"][1].get<float>();
            }
            if (entry.contains("out") && entry["out"].is_array() && entry["out"].size() == 2) {
                profile.outMin = entry["out"][0].get<float>();
                profile.outMax = entry["out"][1].get<float>();
            }
            if (entry.contains("smooth")) profile.smooth = entry["smooth"].get<float>();
            if (entry.contains("deadband")) profile.deadband = entry["deadband"].get<float>();
            if (entry.contains("blend") && entry["blend"].is_string()) {
                profile.blend = blendModeFromString(entry["blend"].get<std::string>(), modifier::BlendMode::kScale);
            } else if (entry.contains("mode") && entry["mode"].is_string()) {
                profile.blend = blendModeFromString(entry["mode"].get<std::string>(), modifier::BlendMode::kScale);
            }
            if (entry.contains("relative")) {
                profile.relativeToBase = entry["relative"].get<bool>();
            }
            upsertOscSourceProfile(oscSourceProfiles, profile);
        }
    }

    if (snapshot.contains("osc") && snapshot["osc"].is_array()) {
        for (const auto& entry : snapshot["osc"]) {
            OscMap map;
            if (entry.contains("pattern")) map.pattern = entry["pattern"].get<std::string>();
            if (entry.contains("target")) map.target = entry["target"].get<std::string>();
            if (entry.contains("bank")) map.bankId = entry["bank"].get<std::string>();
            if (entry.contains("control")) map.controlId = entry["control"].get<std::string>();
            if (!map.target.empty() && !map.pattern.empty()) {
                auto* profile = ensureOscSourceProfile(map.pattern);
                if (profile && entry.contains("in") && entry["in"].is_array() && entry["in"].size() == 2) {
                    profile->inMin = entry["in"][0].get<float>();
                    profile->inMax = entry["in"][1].get<float>();
                }
                if (profile && entry.contains("out") && entry["out"].is_array() && entry["out"].size() == 2) {
                    profile->outMin = entry["out"][0].get<float>();
                    profile->outMax = entry["out"][1].get<float>();
                }
                if (profile && entry.contains("smooth")) profile->smooth = entry["smooth"].get<float>();
                if (profile && entry.contains("deadband")) profile->deadband = entry["deadband"].get<float>();
                if (profile && entry.contains("blend") && entry["blend"].is_string()) {
                    profile->blend = blendModeFromString(entry["blend"].get<std::string>(), modifier::BlendMode::kScale);
                } else if (profile && entry.contains("mode") && entry["mode"].is_string()) {
                    profile->blend = blendModeFromString(entry["mode"].get<std::string>(), modifier::BlendMode::kScale);
                }
                if (profile && entry.contains("relative")) {
                    profile->relativeToBase = entry["relative"].get<bool>();
                }
                upsertOscMap(oscMaps, map);
            }
        }
    }

    canonicalizeOscState(oscMaps, oscSourceProfiles);

    for (auto& map : ccMaps) {
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
        auto fit = floatTargets.find(map.target);
        if (fit != floatTargets.end()) {
            const auto& meta = fit->second;
            if (map.outMin == 0.0f && map.outMax == 1.0f && (meta.defMin != 0.0f || meta.defMax != 1.0f)) {
                map.outMin = meta.defMin;
                map.outMax = meta.defMax;
            }
            if (meta.snapInt) map.snapInt = true;
            if (meta.step > 0.0f && map.step == 0.0f) map.step = meta.step;
            if (map.bankId.empty() && !meta.defaultBankId.empty()) {
                map.bankId = meta.defaultBankId;
            }
            if (map.controlId.empty() && !meta.defaultControlId.empty()) {
                map.controlId = meta.defaultControlId;
            }
        }
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
    }

    for (auto& map : btnMaps) {
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
        auto it = boolTargets.find(map.target);
        if (it != boolTargets.end()) {
            const auto& meta = it->second;
            if (map.bankId.empty() && !meta.defaultBankId.empty()) {
                map.bankId = meta.defaultBankId;
            }
            if (map.controlId.empty() && !meta.defaultControlId.empty()) {
                map.controlId = meta.defaultControlId;
            }
        }
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
    }

    for (auto& map : oscMaps) {
        if (map.controlId.empty()) {
            map.controlId = map.target;
        }
        if (auto* profile = ensureOscSourceProfile(map.pattern)) {
            copyOscProfileToMap(*profile, map);
        }
    }

    for (auto& entry : boolTargets) {
        entry.second.lastHigh = false;
    }

    if (!activeBankId.empty()) {
        std::string current = activeBankId;
        activeBankId.clear();
        setActiveBank(current);
    }

    if (onOscRoutesChanged) {
        onOscRoutesChanged();
    }
    return true;
}

void MidiRouter::close() {
    markClosed();
}


modifier::BindingList MidiRouter::snapshotModifierBindings() const {
    modifier::BindingList bindings;
    bindings.reserve(ccMaps.size() + oscMaps.size());

    auto appendBinding = [&](modifier::Binding&& binding) {
        if (!binding.targetId.empty()) {
            bindings.push_back(std::move(binding));
        }
    };

    for (const auto& map : ccMaps) {
        modifier::Binding binding;
        binding.bankId = map.bankId;
        binding.targetId = map.target;
        binding.meta.sourceType = modifier::SourceType::kMidiCc;
        if (!map.controlId.empty()) {
            binding.meta.label = map.controlId;
        }
        binding.meta.midiControl = map.cc;
        binding.meta.snapInteger = map.snapInt;
        binding.meta.stepSize = map.step;

        modifier::Modifier mod;
        mod.type = modifier::Type::kMidiCc;
        mod.inputRange = {0.0f, 127.0f, false};
        mod.outputRange = {map.outMin, map.outMax, false};

        const bool hasFloat = floatTargets.find(map.target) != floatTargets.end();
        auto boolIt = boolTargets.find(map.target);

        if (boolIt != boolTargets.end()) {
            mod.blend = modifier::BlendMode::kToggle;
            mod.outputRange.min = 0.0f;
            mod.outputRange.max = 1.0f;
            binding.meta.toggleLatch = (boolIt->second.mode == BoolMode::Toggle);
        } else if (hasFloat) {
            mod.blend = modifier::BlendMode::kAbsolute;
        } else {
            continue;
        }

        binding.modifier = mod;
        appendBinding(std::move(binding));
    }

    for (const auto& map : oscMaps) {
        const auto* profile = findOscSourceProfile(map.pattern);
        if (!profile) {
            continue;
        }
        modifier::Binding binding;
        binding.bankId = map.bankId;
        binding.targetId = map.target;
        binding.meta.sourceType = modifier::SourceType::kOsc;
        if (!map.controlId.empty()) {
            binding.meta.label = map.controlId;
        }
        binding.meta.oscAddress = map.pattern;
        binding.meta.smoothing = profile->smooth;
        binding.meta.deadband = profile->deadband;

        modifier::Modifier mod;
        mod.type = modifier::Type::kOsc;
        mod.inputRange = {profile->inMin, profile->inMax, false};
        mod.outputRange = {profile->outMin, profile->outMax, profile->relativeToBase};

        const bool hasFloat = floatTargets.find(map.target) != floatTargets.end();
        auto boolIt = boolTargets.find(map.target);

        if (boolIt != boolTargets.end()) {
            mod.blend = modifier::BlendMode::kToggle;
            mod.outputRange.min = 0.0f;
            mod.outputRange.max = 1.0f;
            mod.outputRange.relativeToBase = false;
            binding.meta.toggleLatch = (boolIt->second.mode == BoolMode::Toggle);
        } else if (hasFloat) {
            mod.blend = profile->blend;
        } else {
            continue;
        }

        binding.modifier = mod;
        appendBinding(std::move(binding));
    }

    return bindings;
}

void MidiRouter::setFloatTargetTouchedCallback(std::function<void(const std::string&)> cb) {
    floatTargetTouchedCallback_ = std::move(cb);
}
void MidiRouter::update() {
    if (isOpen) {
        return;
    }
    uint64_t now = ofGetElapsedTimeMillis();
    if (now - lastRetryMs < retryIntervalMs) {
        return;
    }
    lastRetryMs = now;
    openPreferredPort();
}

bool MidiRouter::openPreferredPort() {
    midiIn.closePort();
    bool ok = false;

    if (!deviceName.empty()) {
        ok = openByName(deviceName, true);
        if (!ok) {
            ofLogWarning("MidiRouter") << "Could not open device by name '" << deviceName << "'";
        }
    }

    if (!ok && deviceIndex >= 0) {
        ok = midiIn.openPort(deviceIndex);
        if (!ok) {
            ofLogWarning("MidiRouter") << "Could not open device at index " << deviceIndex;
        }
    }

    if (!ok) {
        auto ports = midiIn.getInPortList();
        if (ports.empty()) {
            midiIn.listInPorts();
            ports = midiIn.getInPortList();
        }
        if (!ports.empty()) {
            ok = midiIn.openPort(0);
            if (ok) {
                ofLogNotice("MidiRouter") << "Defaulting to first MIDI port: " << ports[0];
            }
        }
    }

    if (!ok) {
        markClosed();
        return false;
    }

    midiIn.addListener(this);
    midiIn.ignoreTypes(false, false, false);
    isOpen = true;

    std::string label;
    int port = midiIn.getPort();
    if (port >= 0) {
        label = midiIn.getInPortName(static_cast<unsigned int>(port));
    } else if (!deviceName.empty()) {
        label = deviceName;
    } else if (deviceIndex >= 0) {
        auto ports = midiIn.getInPortList();
        if (static_cast<std::size_t>(deviceIndex) < ports.size()) {
            label = ports[deviceIndex];
        }
    }
    currentPortLabel = label;

    lastRetryMs = ofGetElapsedTimeMillis();
    return true;
}

bool MidiRouter::openByName(const std::string& name, bool allowSubstring) {
    if (name.empty()) return false;
    if (midiIn.openPort(name)) {
        return true;
    }
    if (!allowSubstring) return false;

    auto ports = midiIn.getInPortList();
    if (ports.empty()) {
        midiIn.listInPorts();
        ports = midiIn.getInPortList();
    }
    std::string needle = ofToLower(name);
    for (std::size_t i = 0; i < ports.size(); ++i) {
        std::string lowered = ofToLower(ports[i]);
        if (lowered.find(needle) != std::string::npos) {
            if (midiIn.openPort(static_cast<unsigned int>(i))) {
                return true;
            }
        }
    }
    return false;
}

void MidiRouter::markClosed() {
    if (isOpen) {
        midiIn.removeListener(this);
        midiIn.closePort();
    } else {
        midiIn.closePort();
    }
    isOpen = false;
    currentPortLabel.clear();
    lastRetryMs = ofGetElapsedTimeMillis();
}

void MidiRouter::listPortsToLog() const {
    auto ports = availableInputPorts();
    ofLogNotice("MidiRouter") << "MIDI In Ports:";
    for (std::size_t i = 0; i < ports.size(); ++i) {
        ofLogNotice("MidiRouter") << "  [" << i << "] " << ports[i];
    }
}

std::vector<std::string> MidiRouter::availableInputPorts() const {
    if (useTestPortListOverride_) {
        return testPortListOverride_;
    }
    ofxMidiIn temp;
    auto ports = temp.getInPortList();
    if (ports.empty()) {
        temp.listInPorts();
        ports = temp.getInPortList();
    }
    return ports;
}

void MidiRouter::setTestPortList(const std::vector<std::string>& ports) {
    testPortListOverride_ = ports;
    useTestPortListOverride_ = true;
}

void MidiRouter::clearTestPortList() {
    testPortListOverride_.clear();
    useTestPortListOverride_ = false;
}

void MidiRouter::captureNextMidiControl(std::function<void(const CapturedMidiControl&)> callback) {
    pendingMidiCapture_ = std::move(callback);
}

void MidiRouter::cancelMidiControlCapture() {
    pendingMidiCapture_ = nullptr;
}

void MidiRouter::newMidiMessage(ofxMidiMessage& msg) {
    auto dispatchCapture = [&](const std::string& type, int channel, int number, int value) {
        if (pendingMidiCapture_) {
            auto cb = pendingMidiCapture_;
            pendingMidiCapture_ = nullptr;
            CapturedMidiControl capture;
            capture.type = type;
            capture.channel = channel;
            capture.number = number;
            capture.value = value;
            cb(capture);
        }
    };
    if (msg.status == MIDI_CONTROL_CHANGE) {
        int ccNum = msg.control;
        int value = msg.value;

        dispatchCapture("cc", msg.channel, ccNum, value);

        if (!learningTarget.empty()) {
            setOrUpdateCc(learningTarget, ccNum);
            ofLogNotice("MidiRouter") << "Learned '" << learningTarget << "' -> CC " << ccNum;
            learningTarget.clear();
        }

        for (auto& map : ccMaps) {
            if (map.cc != ccNum || map.target.empty()) {
                continue;
            }
            if (map.channel >= 0 && map.channel != msg.channel) {
                continue;
            }
            applyCc(map, value);
        }
    } else if (msg.status == MIDI_NOTE_ON || msg.status == MIDI_NOTE_OFF) {
        int number = msg.pitch;
        int value = (msg.status == MIDI_NOTE_ON) ? msg.velocity : 0;
        dispatchCapture("note", msg.channel, number, value);
        for (const auto& map : btnMaps) {
            if (map.num != number || map.target.empty()) {
                continue;
            }
            if (map.channel >= 0 && map.channel != msg.channel) {
                continue;
            }
            applyBtn(map, value);
        }
    }
}

bool MidiRouter::isMapActive(const std::string& bankId) const {
    if (bankId.empty()) {
        return true;
    }
    if (activeBankId.empty()) {
        return true;
    }
    return bankId == activeBankId;
}

float MidiRouter::computeOutputValue(const CcMap& map, float normalized) const {
    float out = map01ToRange(normalized, map.outMin, map.outMax);
    if (map.step > 0.0f) {
        out = std::round(out / map.step) * map.step;
    }
    if (map.snapInt) {
        out = std::round(out);
    }
    return out;
}

float MidiRouter::computeTolerance(const CcMap& map) const {
    float span = std::fabs(map.outMax - map.outMin);
    float baseTol = span > 0.0f ? softTakeoverTolerance * span : softTakeoverTolerance;
    float stepTol = (map.step > 0.0f) ? map.step * 0.5f : 0.0f;
    float minTol = std::max(0.001f, span * 0.0025f);
    return std::max({ baseTol, stepTol, minTol });
}

void MidiRouter::setActiveBank(const std::string& bankId) {
    if (bankId == activeBankId) {
        return;
    }
    activeBankId = bankId;
    uint64_t now = ofGetElapsedTimeMillis();
    for (auto& map : ccMaps) {
        if (map.bankId.empty()) {
            map.pending = false;
            map.pendingDelta = 0.0f;
            map.lastPendingDelta = 0.0f;
            map.pendingSinceMs = 0;
            continue;
        }
        if (map.bankId != activeBankId) {
            map.pending = false;
            map.pendingDelta = 0.0f;
            map.lastPendingDelta = 0.0f;
            map.pendingSinceMs = 0;
            continue;
        }
        auto fit = floatTargets.find(map.target);
        if (fit == floatTargets.end() || !fit->second.ptr) {
            map.pending = false;
            map.pendingDelta = 0.0f;
            map.lastPendingDelta = 0.0f;
            map.pendingSinceMs = 0;
            continue;
        }

        float currentValue = *fit->second.ptr;
        map.catchValue = currentValue;
        map.pendingSinceMs = now;
        if (!map.hardwareKnown) {
            map.pending = true;
            map.pendingDelta = 0.0f;
            map.lastPendingDelta = 0.0f;
            continue;
        }

        float hardwareValue = computeOutputValue(map, map.lastHardwareNorm);
        float delta = currentValue - hardwareValue;
        map.pendingDelta = delta;
        map.lastPendingDelta = delta;
        float tolerance = computeTolerance(map);
        if (std::fabs(delta) <= tolerance) {
            map.pending = false;
            map.pendingDelta = 0.0f;
            map.lastPendingDelta = 0.0f;
            map.pendingSinceMs = 0;
        } else {
            map.pending = true;
        }
    }
}

std::vector<MidiRouter::TakeoverState> MidiRouter::pendingTakeovers() const {
    std::vector<TakeoverState> results;
    results.reserve(ccMaps.size());
    for (const auto& map : ccMaps) {
        if (!map.pending) {
            continue;
        }
        if (!isMapActive(map.bankId)) {
            continue;
        }
        if (floatTargets.find(map.target) == floatTargets.end()) {
            continue;
        }
        TakeoverState state;
        state.bankId = map.bankId;
        state.targetId = map.target;
        state.controlId = map.controlId.empty() ? map.target : map.controlId;
        state.delta = map.pendingDelta;
        state.hardwareValue = computeOutputValue(map, map.lastHardwareNorm);
        state.catchValue = map.catchValue;
        state.pendingSinceMs = map.pendingSinceMs;
        results.push_back(state);
    }
    return results;
}

void MidiRouter::beginLearn(const std::string& targetName) {
    learningTarget = targetName;
    learningIsOsc = false;
}

void MidiRouter::beginLearn(const std::string& targetName, bool oscLearn) {
    learningTarget = targetName;
    learningIsOsc = oscLearn;
}

void MidiRouter::onOscMessage(const std::string& address, float value) {
    lastOscAddr = address;
    lastOscVal = value;

    std::string deviceType;
    std::string deviceId;
    std::string metric;
    if (parseSensorAddress(address, deviceType, deviceId, metric)) {
        ensureOscDeviceChannels(deviceType, deviceId);
    }

    uint64_t nowMs = ofGetElapsedTimeMillis();
    auto* info = findOscSource(address);
    if (!info) {
        OscSourceInfo placeholder;
        placeholder.address = address;
        oscSources.push_back(placeholder);
        info = &oscSources.back();
    }
    info->lastValue = value;
    info->lastSeenMs = nowMs;
    info->seen = true;

    if (!learningTarget.empty() && learningIsOsc) {
        OscMap m;
        m.pattern = address;
        m.target = learningTarget;
        m.bankId = activeBankId;
        bool hadProfile = findOscSourceProfile(address) != nullptr;
        auto* profile = ensureOscSourceProfile(address);
        auto fit = floatTargets.find(learningTarget);
        if (fit != floatTargets.end() && profile && !hadProfile) {
            profile->outMin = fit->second.defMin;
            profile->outMax = fit->second.defMax;
            if (!fit->second.defaultControlId.empty()) {
                m.controlId = fit->second.defaultControlId;
            }
        }
        if (m.controlId.empty()) {
            m.controlId = learningTarget;
        }
        if (profile) {
            copyOscProfileToMap(*profile, m);
        }
        upsertOscMap(oscMaps, m);
        if (onOscMapAdded) onOscMapAdded(m);
        if (onOscRoutesChanged) onOscRoutesChanged();
        ofLogNotice("MidiRouter") << "Learned OSC '" << learningTarget << "' -> " << address;
        learningTarget.clear();
        learningIsOsc = false;
    }
}

bool MidiRouter::setOscMapFromLast(const std::string& target) {
    if (lastOscAddr.empty()) return false;
    return setOscMapFromAddress(target, lastOscAddr);
}

void MidiRouter::seedOscSources(const std::vector<std::string>& addresses) {
    if (addresses.empty()) return;
    std::unordered_map<std::string, OscSourceInfo> existing;
    existing.reserve(oscSources.size());
    for (const auto& info : oscSources) {
        existing[info.address] = info;
    }
    std::vector<OscSourceInfo> rebuilt;
    rebuilt.reserve(addresses.size() + oscSources.size());
    for (const auto& addr : addresses) {
        auto it = existing.find(addr);
        if (it != existing.end()) {
            rebuilt.push_back(it->second);
            existing.erase(it);
        } else {
            OscSourceInfo info;
            info.address = addr;
            rebuilt.push_back(info);
        }
    }
    for (const auto& kv : existing) {
        rebuilt.push_back(kv.second);
    }
    oscSources.swap(rebuilt);
}

bool MidiRouter::setOscMapFromAddress(const std::string& target, const std::string& address) {
    if (address.empty()) return false;
    OscMap m;
    m.pattern = address;
    m.target = target;
    m.bankId = activeBankId;
    bool hadProfile = findOscSourceProfile(address) != nullptr;
    auto* profile = ensureOscSourceProfile(address);
    auto fit = floatTargets.find(target);
    if (fit != floatTargets.end() && profile && !hadProfile) {
        profile->outMin = fit->second.defMin;
        profile->outMax = fit->second.defMax;
        if (!fit->second.defaultControlId.empty()) {
            m.controlId = fit->second.defaultControlId;
        }
    }
    if (m.controlId.empty()) {
        m.controlId = target;
    }
    if (profile) {
        copyOscProfileToMap(*profile, m);
    }
    upsertOscMap(oscMaps, m);
    if (onOscMapAdded) onOscMapAdded(m);
    if (onOscRoutesChanged) onOscRoutesChanged();
    ofLogNotice("MidiRouter") << "Captured OSC '" << target << "' -> " << address;
    if (!findOscSource(address)) {
        OscSourceInfo placeholder;
        placeholder.address = address;
        oscSources.push_back(placeholder);
    }
    return true;
}

const MidiRouter::OscMap* MidiRouter::findOscMap(const std::string& target) const {
    auto it = std::find_if(oscMaps.begin(), oscMaps.end(), [&](const OscMap& map) {
        return map.target == target;
    });
    return it != oscMaps.end() ? &(*it) : nullptr;
}

MidiRouter::OscMap* MidiRouter::findOscMap(const std::string& target) {
    auto it = std::find_if(oscMaps.begin(), oscMaps.end(), [&](const OscMap& map) {
        return map.target == target;
    });
    return it != oscMaps.end() ? &(*it) : nullptr;
}

const MidiRouter::OscSourceProfile* MidiRouter::findOscSourceProfile(const std::string& pattern) const {
    auto it = std::find_if(oscSourceProfiles.begin(), oscSourceProfiles.end(), [&](const OscSourceProfile& profile) {
        return profile.pattern == pattern;
    });
    return it != oscSourceProfiles.end() ? &(*it) : nullptr;
}

MidiRouter::OscSourceProfile* MidiRouter::findOscSourceProfile(const std::string& pattern) {
    auto it = std::find_if(oscSourceProfiles.begin(), oscSourceProfiles.end(), [&](const OscSourceProfile& profile) {
        return profile.pattern == pattern;
    });
    return it != oscSourceProfiles.end() ? &(*it) : nullptr;
}

MidiRouter::OscSourceProfile* MidiRouter::ensureOscSourceProfile(const std::string& pattern) {
    if (pattern.empty()) {
        return nullptr;
    }
    if (auto* profile = findOscSourceProfile(pattern)) {
        return profile;
    }
    OscSourceProfile profile;
    profile.pattern = pattern;
    oscSourceProfiles.push_back(profile);
    return &oscSourceProfiles.back();
}

bool MidiRouter::adjustOscMap(const std::string& target,
                              float dInMin,
                              float dInMax,
                              float dOutMin,
                              float dOutMax,
                              float dSmooth,
                              float dDeadband) {
    auto* map = findOscMap(target);
    if (!map) {
        return false;
    }
    return adjustOscSourceProfile(map->pattern, dInMin, dInMax, dOutMin, dOutMax, dSmooth, dDeadband);
}

bool MidiRouter::adjustOscSourceProfile(const std::string& pattern,
                                        float dInMin,
                                        float dInMax,
                                        float dOutMin,
                                        float dOutMax,
                                        float dSmooth,
                                        float dDeadband) {
    auto* profile = findOscSourceProfile(pattern);
    if (!profile) {
        return false;
    }
    profile->inMin += dInMin;
    profile->inMax += dInMax;
    profile->outMin += dOutMin;
    profile->outMax += dOutMax;
    profile->smooth = std::clamp(profile->smooth + dSmooth, 0.0f, 1.0f);
    profile->deadband = std::max(0.0f, profile->deadband + dDeadband);

    for (auto& map : oscMaps) {
        if (map.pattern == pattern) {
            copyOscProfileToMap(*profile, map);
        }
    }
    if (onOscRoutesChanged) onOscRoutesChanged();
    return true;
}

bool MidiRouter::removeMidiMappingsForTarget(const std::string& target) {
    if (target.empty()) {
        return false;
    }
    auto removeTarget = [&](auto& container) {
        auto oldSize = container.size();
        container.erase(std::remove_if(container.begin(), container.end(), [&](const auto& entry) {
                            return entry.target == target;
                        }),
                        container.end());
        return container.size() != oldSize;
    };
    bool removed = removeTarget(ccMaps);
    removed = removeTarget(btnMaps) || removed;
    return removed;
}

bool MidiRouter::removeOscMappingsForTarget(const std::string& target) {
    if (target.empty()) {
        return false;
    }
    auto oldSize = oscMaps.size();
    oscMaps.erase(std::remove_if(oscMaps.begin(), oscMaps.end(), [&](const OscMap& map) {
                      return map.target == target;
                  }),
                  oscMaps.end());
    bool removed = oscMaps.size() != oldSize;
    if (removed && onOscRoutesChanged) {
        onOscRoutesChanged();
    }
    return removed;
}

void MidiRouter::setOrUpdateCc(const std::string& target,
                               int ccNum,
                               float outMin,
                               float outMax,
                               bool snapInt,
                               float step,
                               const BindingMetadata& binding) {
    for (auto& map : ccMaps) {
        if (map.target == target) {
            map.cc = ccNum;
            if (!activeBankId.empty()) {
                map.bankId = activeBankId;
            }
            auto fit = floatTargets.find(target);
            if (fit != floatTargets.end()) {
                const auto& meta = fit->second;
                if (map.outMin == 0.0f && map.outMax == 1.0f && (meta.defMin != 0.0f || meta.defMax != 1.0f)) {
                    map.outMin = meta.defMin;
                    map.outMax = meta.defMax;
                }
                if (meta.snapInt) map.snapInt = true;
                if (meta.step > 0.0f && map.step == 0.0f) map.step = meta.step;
                if (map.bankId.empty() && !meta.defaultBankId.empty()) {
                    map.bankId = meta.defaultBankId;
                }
                if (map.controlId.empty() && !meta.defaultControlId.empty()) {
                    map.controlId = meta.defaultControlId;
                }
            }
            if (!binding.controlId.empty()) {
                map.controlId = binding.controlId;
            } else if (map.controlId.empty()) {
                auto bit = boolTargets.find(target);
                if (bit != boolTargets.end() && !bit->second.defaultControlId.empty()) {
                    map.controlId = bit->second.defaultControlId;
                }
            }
            if (binding.channel >= 0) {
                map.channel = binding.channel;
            }
            if (!binding.deviceId.empty()) {
                map.deviceId = binding.deviceId;
            }
            if (!binding.columnId.empty()) {
                map.columnId = binding.columnId;
            }
            if (!binding.slotId.empty()) {
                map.slotId = binding.slotId;
            }
            if (map.controlId.empty()) {
                map.controlId = target;
            }
            if (onCcMapAdded) {
                onCcMapAdded(map);
            }
            return;
        }
    }

    CcMap map;
    map.target = target;
    map.cc = ccNum;
    map.bankId = activeBankId;
    map.channel = binding.channel;
    map.deviceId = binding.deviceId;
    map.columnId = binding.columnId;
    map.slotId = binding.slotId;
    auto fit = floatTargets.find(target);
    if (fit != floatTargets.end()) {
        const auto& meta = fit->second;
        map.outMin = meta.defMin;
        map.outMax = meta.defMax;
        map.snapInt = meta.snapInt;
        map.step = meta.step;
        if (!meta.defaultControlId.empty()) {
            map.controlId = meta.defaultControlId;
        }
        if (map.bankId.empty() && !meta.defaultBankId.empty()) {
            map.bankId = meta.defaultBankId;
        }
    } else {
        map.outMin = outMin;
        map.outMax = outMax;
        map.snapInt = snapInt;
        map.step = step;
    }
    if (!binding.controlId.empty()) {
        map.controlId = binding.controlId;
    } else {
        auto bit = boolTargets.find(target);
        if (bit != boolTargets.end()) {
            if (map.bankId.empty() && !bit->second.defaultBankId.empty()) {
                map.bankId = bit->second.defaultBankId;
            }
            if (!bit->second.defaultControlId.empty()) {
                map.controlId = bit->second.defaultControlId;
            }
        }
        if (map.controlId.empty()) {
            map.controlId = target;
        }
    }
    ccMaps.push_back(map);
    if (onCcMapAdded) {
        onCcMapAdded(ccMaps.back());
    }
}

void MidiRouter::setOrUpdateBtn(const std::string& target,
                                int noteNum,
                                const std::string& type,
                                float setValue,
                                const BindingMetadata& binding) {
    for (auto& map : btnMaps) {
        if (map.target == target) {
            map.num = noteNum;
            if (!type.empty()) {
                map.type = type;
            }
            map.setValue = setValue;
            if (!activeBankId.empty()) {
                map.bankId = activeBankId;
            }
            if (!binding.controlId.empty()) {
                map.controlId = binding.controlId;
            } else if (map.controlId.empty()) {
                auto bit = boolTargets.find(target);
                if (bit != boolTargets.end() && !bit->second.defaultControlId.empty()) {
                    map.controlId = bit->second.defaultControlId;
                }
            }
            if (binding.channel >= 0) {
                map.channel = binding.channel;
            }
            if (!binding.deviceId.empty()) {
                map.deviceId = binding.deviceId;
            }
            if (!binding.columnId.empty()) {
                map.columnId = binding.columnId;
            }
            if (!binding.slotId.empty()) {
                map.slotId = binding.slotId;
            }
            if (map.controlId.empty()) {
                map.controlId = target;
            }
            return;
        }
    }

    BtnMap map;
    map.target = target;
    map.num = noteNum;
    map.type = type.empty() ? std::string("toggle") : type;
    map.setValue = setValue;
    map.bankId = activeBankId;
    map.channel = binding.channel;
    map.deviceId = binding.deviceId;
    map.columnId = binding.columnId;
    map.slotId = binding.slotId;
    if (!binding.controlId.empty()) {
        map.controlId = binding.controlId;
    } else {
        auto bit = boolTargets.find(target);
        if (bit != boolTargets.end()) {
            if (map.bankId.empty() && !bit->second.defaultBankId.empty()) {
                map.bankId = bit->second.defaultBankId;
            }
            if (!bit->second.defaultControlId.empty()) {
                map.controlId = bit->second.defaultControlId;
            }
        }
        if (map.controlId.empty()) {
            map.controlId = target;
        }
    }
    btnMaps.push_back(std::move(map));
}void MidiRouter::adjustCcRange(const std::string& target, float dMin, float dMax) {
    for (auto& map : ccMaps) {
        if (map.target == target) {
            map.outMin += dMin;
            map.outMax += dMax;
            if (map.outMax < map.outMin) {
                std::swap(map.outMin, map.outMax);
            }
            return;
        }
    }

    // Create placeholder map so the tweak is preserved. cc stays -1 until learned.
    setOrUpdateCc(target, -1);
    adjustCcRange(target, dMin, dMax);
}

void MidiRouter::unbindByPrefix(const std::string& prefix) {
    auto starts = [&](const std::string& id) {
        return !prefix.empty() && id.compare(0, prefix.size(), prefix) == 0;
    };

    for (auto it = floatTargets.begin(); it != floatTargets.end();) {
        if (starts(it->first)) {
            it = floatTargets.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = boolTargets.begin(); it != boolTargets.end();) {
        if (starts(it->first)) {
            it = boolTargets.erase(it);
        } else {
            ++it;
        }
    }

    ccMaps.erase(std::remove_if(ccMaps.begin(), ccMaps.end(), [&](const CcMap& map) {
        return starts(map.target);
    }), ccMaps.end());
    btnMaps.erase(std::remove_if(btnMaps.begin(), btnMaps.end(), [&](const BtnMap& map) {
        return starts(map.target);
    }), btnMaps.end());
    oscMaps.erase(std::remove_if(oscMaps.begin(), oscMaps.end(), [&](const OscMap& map) {
        return starts(map.target);
    }), oscMaps.end());
}

MidiRouter::OscSourceInfo* MidiRouter::findOscSource(const std::string& address) {
    for (auto& info : oscSources) {
        if (info.address == address) {
            return &info;
        }
    }
    return nullptr;
}

const MidiRouter::OscSourceInfo* MidiRouter::findOscSource(const std::string& address) const {
    for (const auto& info : oscSources) {
        if (info.address == address) {
            return &info;
        }
    }
    return nullptr;
}

void MidiRouter::ensureOscDeviceChannels(const std::string& deviceType, const std::string& deviceId) {
    auto it = kDeviceMetricOrder.find(deviceType);
    if (it == kDeviceMetricOrder.end()) {
        return;
    }
    std::string base = "/sensor/" + deviceType + "/" + deviceId + "/";
    for (const auto& metric : it->second) {
        std::string addr = base + metric;
        if (!findOscSource(addr)) {
            OscSourceInfo placeholder;
            placeholder.address = addr;
            oscSources.push_back(placeholder);
        }
    }
}


void MidiRouter::applyCc(CcMap& map, int value0127) {
    float normalized = ofMap(static_cast<float>(value0127), 0.0f, 127.0f, 0.0f, 1.0f, true);
    map.lastHardwareNorm = normalized;
    map.hardwareKnown = true;

    float out = computeOutputValue(map, normalized);

    auto itF = floatTargets.find(map.target);
    if (itF != floatTargets.end() && itF->second.ptr) {
        if (!isMapActive(map.bankId)) {
            return;
        }

        if (map.pending) {
            float delta = map.catchValue - out;
            map.pendingDelta = delta;
            float tolerance = computeTolerance(map);
            bool crossed = std::fabs(delta) <= tolerance;
            if (!crossed) {
                if ((map.lastPendingDelta > 0.0f && delta <= 0.0f) ||
                    (map.lastPendingDelta < 0.0f && delta >= 0.0f)) {
                    crossed = true;
                }
            }
            map.lastPendingDelta = delta;
            if (!crossed) {
                return;
            }
            map.pending = false;
        }

        map.pendingDelta = 0.0f;
        map.lastPendingDelta = 0.0f;
        map.pendingSinceMs = 0;

        *itF->second.ptr = out;
        if (floatTargetTouchedCallback_) {
            floatTargetTouchedCallback_(map.target);
        }
        map.catchValue = *itF->second.ptr;
        return;
    }

    auto itB = boolTargets.find(map.target);
    if (itB != boolTargets.end() && itB->second.ptr) {
        if (!isMapActive(map.bankId)) {
            return;
        }
        auto& tgt = itB->second;
        float threshold = (map.outMin + map.outMax) * 0.5f;
        bool high = (out >= threshold);
        if (tgt.mode == BoolMode::Toggle) {
            if (high && !tgt.lastHigh) {
                *tgt.ptr = !(*tgt.ptr);
            }
            tgt.lastHigh = high;
        } else {
            *tgt.ptr = high;
            tgt.lastHigh = high;
        }
    }
}

void MidiRouter::applyBtn(const BtnMap& map, int value0127) {
    if (!isMapActive(map.bankId)) {
        return;
    }
    bool pressed = value0127 > 0;
    if (map.type == "toggle") {
        if (pressed) {
            auto it = boolTargets.find(map.target);
            if (it != boolTargets.end() && it->second.ptr) {
                *it->second.ptr = !(*it->second.ptr);
            }
        }
    } else if (map.type == "set") {
        auto itF = floatTargets.find(map.target);
        if (itF != floatTargets.end() && itF->second.ptr) {
            if (pressed) {
                *itF->second.ptr = map.setValue;
                if (floatTargetTouchedCallback_) {
                    floatTargetTouchedCallback_(map.target);
                }
            }
        }
    }
}

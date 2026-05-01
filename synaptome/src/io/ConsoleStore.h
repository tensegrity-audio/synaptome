#pragma once

#include "ofMain.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct ConsoleLayerCoverageInfo {
    bool defined = false;
    std::string mode = "upstream";
    int columns = 0;
};

struct ConsoleLayerInfo {
    int index = 0; // 1-based
    std::string assetId;
    bool active = false;
    float opacity = 1.0f;
    std::string label;
    std::string displayName; // legacy alias for label; kept for backward compatibility
    ConsoleLayerCoverageInfo coverage;
};

struct ConsoleOverlayVisibility {
    bool hudVisible = true;
    bool consoleVisible = true;
    bool controlHubVisible = true;
    bool menuVisible = true;
};

struct ConsoleDualDisplayConfig {
    std::string mode = "single";
};

struct ConsoleSecondaryDisplayState {
    bool enabled = false;
    std::string monitorId;
    int x = 0;
    int y = 0;
    int width = 1280;
    int height = 720;
    bool vsync = true;
    float dpiScale = 1.0f;
    std::string background = "#000000";
    bool followPrimary = true;
};

struct ConsoleControllerFocusState {
    bool consolePreferred = true;
};

struct ConsoleBioAmpSnapshot {
    bool hasRaw = false;
    float raw = 0.0f;
    uint64_t rawTimestampMs = 0;

    bool hasSignal = false;
    float signal = 0.0f;
    uint64_t signalTimestampMs = 0;

    bool hasMean = false;
    float mean = 0.0f;
    uint64_t meanTimestampMs = 0;

    bool hasRms = false;
    float rms = 0.0f;
    uint64_t rmsTimestampMs = 0;

    bool hasDomHz = false;
    float domHz = 0.0f;
    uint64_t domTimestampMs = 0;

    bool hasSampleRate = false;
    uint16_t sampleRate = 0;
    uint64_t sampleRateTimestampMs = 0;

    bool hasWindow = false;
    uint16_t window = 0;
    uint64_t windowTimestampMs = 0;
};

struct ConsoleSensorSnapshot {
    ConsoleBioAmpSnapshot bioAmp;
    bool bioAmpDefined = false;
};

struct ConsoleOverlayWidgetPlacement {
    std::string id;
    int columnIndex = -1;
    bool visible = true;
    bool collapsed = false;
    std::string bandId = "hud";
    std::string target = "projector";
};

struct ConsoleOverlayLayoutSnapshot {
    uint64_t capturedAtMs = 0;
    std::vector<ConsoleOverlayWidgetPlacement> widgets;
};

struct ConsoleOverlayLayouts {
    ConsoleOverlayLayoutSnapshot projector;
    ConsoleOverlayLayoutSnapshot controller;
    std::string activeTarget = "projector";
    uint64_t lastSyncMs = 0;
};

struct ConsolePresentationState {
    std::vector<ConsoleLayerInfo> layers;
    ConsoleOverlayVisibility overlays;
    ConsoleDualDisplayConfig dualDisplay;
    ConsoleSecondaryDisplayState secondaryDisplay;
    ConsoleControllerFocusState controllerFocus;
    ConsoleOverlayLayouts overlayLayouts;
    ConsoleSensorSnapshot sensors;
    int version = 1;
    bool overlaysDefined = false;
    bool dualDisplayDefined = false;
    bool secondaryDisplayDefined = false;
    bool controllerFocusDefined = false;
    bool overlayLayoutsDefined = false;
    bool sensorsDefined = false;
};

class ConsoleStore {
public:
    // Load console presentation state from JSON file.
    static ConsolePresentationState loadState(const std::string& path);
    // Save console presentation state to JSON file. Returns true on success.
    static bool saveState(const std::string& path, const ConsolePresentationState& state);
    // Legacy helpers for code/tests that only care about slot inventory.
    static std::vector<ConsoleLayerInfo> load(const std::string& path);
    static bool save(const std::string& path, const std::vector<ConsoleLayerInfo>& layers);
};

// Inline implementations so the symbols are available without requiring
// an additional translation unit to be added to project files.
inline ConsolePresentationState ConsoleStore::loadState(const std::string& path) {
    ConsolePresentationState state;
    if (path.empty()) return state;
    if (!ofFile::doesFileExist(path)) return state;
    ofJson root;
    try {
        root = ofLoadJson(path);
    } catch (const std::exception& ex) {
        ofLogWarning("ConsoleStore") << "Failed to load console config: " << ex.what();
        return state;
    }
    if (!root.is_object()) return state;
    state.version = root.value("version", 1);
    if (root.contains("layers") && root["layers"].is_array()) {
        for (const auto& node : root["layers"]) {
            if (!node.is_object()) continue;
            ConsoleLayerInfo info;
            if (node.contains("index") && node["index"].is_number_integer()) {
                info.index = node["index"].get<int>();
            }
            if (node.contains("assetId") && node["assetId"].is_string()) {
                info.assetId = node["assetId"].get<std::string>();
                if (info.assetId == "overlay.text") {
                    info.assetId = "text.layer";
                    if (info.label == "Text Overlay" || info.label.empty()) {
                        info.label = "Text Layer";
                    }
                    if (info.displayName == "Text Overlay" || info.displayName.empty()) {
                        info.displayName = "Text Layer";
                    }
                }
            }
            if (node.contains("active") && node["active"].is_boolean()) {
                info.active = node["active"].get<bool>();
            }
            if (node.contains("opacity") && node["opacity"].is_number()) {
                info.opacity = static_cast<float>(node["opacity"].get<double>());
            }
            if (node.contains("label") && node["label"].is_string()) {
                info.label = node["label"].get<std::string>();
            }
            if (node.contains("displayName") && node["displayName"].is_string()) {
                info.displayName = node["displayName"].get<std::string>();
                if (info.label.empty()) {
                    info.label = info.displayName;
                }
            } else if (info.displayName.empty()) {
                info.displayName = info.label;
            }
            if (info.assetId == "text.layer") {
                if (info.label.empty() || info.label == "Text Overlay") {
                    info.label = "Text Layer";
                }
                if (info.displayName.empty() || info.displayName == "Text Overlay") {
                    info.displayName = info.label;
                }
            }
            if (node.contains("coverage") && node["coverage"].is_object()) {
                const auto& coverageNode = node["coverage"];
                info.coverage.defined = true;
                if (coverageNode.contains("mode") && coverageNode["mode"].is_string()) {
                    info.coverage.mode = coverageNode["mode"].get<std::string>();
                }
                if (coverageNode.contains("columns") && coverageNode["columns"].is_number_integer()) {
                    info.coverage.columns = std::max(0, coverageNode["columns"].get<int>());
                }
            }
            if (!info.assetId.empty() && info.index >= 1 && info.index <= 8) {
                state.layers.push_back(info);
            }
        }
    }
    if (root.contains("overlays") && root["overlays"].is_object()) {
        state.overlaysDefined = true;
        const auto& overlayNode = root["overlays"];
        state.overlays.hudVisible = overlayNode.value("hudVisible", state.overlays.hudVisible);
        state.overlays.consoleVisible = overlayNode.value("consoleVisible", state.overlays.consoleVisible);
        state.overlays.controlHubVisible = overlayNode.value("controlHubVisible", state.overlays.controlHubVisible);
        state.overlays.menuVisible = overlayNode.value("menuVisible", state.overlays.menuVisible);
    }
    if (root.contains("dualDisplay") && root["dualDisplay"].is_object()) {
        state.dualDisplayDefined = true;
        const auto& dualNode = root["dualDisplay"];
        if (dualNode.contains("mode") && dualNode["mode"].is_string()) {
            state.dualDisplay.mode = dualNode["mode"].get<std::string>();
        }
    }
    if (root.contains("secondaryDisplay") && root["secondaryDisplay"].is_object()) {
        state.secondaryDisplayDefined = true;
        const auto& sec = root["secondaryDisplay"];
        state.secondaryDisplay.enabled = sec.value("enabled", state.secondaryDisplay.enabled);
        if (sec.contains("monitorId") && sec["monitorId"].is_string()) {
            state.secondaryDisplay.monitorId = sec["monitorId"].get<std::string>();
        }
        state.secondaryDisplay.x = sec.value("x", state.secondaryDisplay.x);
        state.secondaryDisplay.y = sec.value("y", state.secondaryDisplay.y);
        state.secondaryDisplay.width = sec.value("width", state.secondaryDisplay.width);
        state.secondaryDisplay.height = sec.value("height", state.secondaryDisplay.height);
        state.secondaryDisplay.vsync = sec.value("vsync", state.secondaryDisplay.vsync);
        state.secondaryDisplay.dpiScale = sec.value("dpiScale", state.secondaryDisplay.dpiScale);
        if (sec.contains("background") && sec["background"].is_string()) {
            state.secondaryDisplay.background = sec["background"].get<std::string>();
        }
        state.secondaryDisplay.followPrimary = sec.value("followPrimary", state.secondaryDisplay.followPrimary);
    }
    if (root.contains("controllerFocus") && root["controllerFocus"].is_object()) {
        state.controllerFocusDefined = true;
        const auto& focus = root["controllerFocus"];
        state.controllerFocus.consolePreferred = focus.value("consolePreferred", state.controllerFocus.consolePreferred);
    }
    if (root.contains("overlayLayouts") && root["overlayLayouts"].is_object()) {
        const auto& layoutNode = root["overlayLayouts"];
        state.overlayLayoutsDefined = true;
        state.overlayLayouts.activeTarget = layoutNode.value("activeTarget", state.overlayLayouts.activeTarget);
        state.overlayLayouts.lastSyncMs = layoutNode.value("lastSyncMs", state.overlayLayouts.lastSyncMs);
        auto parseSnapshot = [](const ofJson& node, ConsoleOverlayLayoutSnapshot& snapshot) {
            if (!node.is_object()) {
                return;
            }
            if (node.contains("capturedAtMs")) {
                if (node["capturedAtMs"].is_number_unsigned()) {
                    snapshot.capturedAtMs = node["capturedAtMs"].get<uint64_t>();
                } else if (node["capturedAtMs"].is_number_integer()) {
                    snapshot.capturedAtMs = static_cast<uint64_t>(node["capturedAtMs"].get<int64_t>());
                }
            }
            if (node.contains("widgets") && node["widgets"].is_array()) {
                snapshot.widgets.clear();
                for (const auto& widgetNode : node["widgets"]) {
                    if (!widgetNode.is_object() || !widgetNode.contains("id") || !widgetNode["id"].is_string()) {
                        continue;
                    }
                    ConsoleOverlayWidgetPlacement placement;
                    placement.id = widgetNode["id"].get<std::string>();
                    placement.columnIndex = widgetNode.value("column", placement.columnIndex);
                    placement.visible = widgetNode.value("visible", placement.visible);
                    placement.collapsed = widgetNode.value("collapsed", placement.collapsed);
                    if (widgetNode.contains("band") && widgetNode["band"].is_string()) {
                        placement.bandId = widgetNode["band"].get<std::string>();
                    }
                    if (widgetNode.contains("target") && widgetNode["target"].is_string()) {
                        placement.target = widgetNode["target"].get<std::string>();
                    }
                    snapshot.widgets.push_back(std::move(placement));
                }
            }
        };
        if (layoutNode.contains("projector")) {
            parseSnapshot(layoutNode["projector"], state.overlayLayouts.projector);
        }
        if (layoutNode.contains("controller")) {
            parseSnapshot(layoutNode["controller"], state.overlayLayouts.controller);
        }
    }
    if (root.contains("sensors") && root["sensors"].is_object()) {
        state.sensorsDefined = true;
        const auto& sensorsNode = root["sensors"];
        if (sensorsNode.contains("bioamp") && sensorsNode["bioamp"].is_object()) {
            const auto& bioNode = sensorsNode["bioamp"];
            auto parseFloatField = [&](const char* key, const char* tsKey, bool& hasField, float& value, uint64_t& ts) {
                if (bioNode.contains(key) && bioNode[key].is_number()) {
                    value = static_cast<float>(bioNode[key].get<double>());
                    hasField = true;
                }
                if (hasField && bioNode.contains(tsKey)) {
                    if (bioNode[tsKey].is_number_unsigned()) {
                        ts = bioNode[tsKey].get<uint64_t>();
                    } else if (bioNode[tsKey].is_number_integer()) {
                        ts = static_cast<uint64_t>(bioNode[tsKey].get<int64_t>());
                    }
                }
            };
            auto& snap = state.sensors.bioAmp;
            parseFloatField("raw", "rawTs", snap.hasRaw, snap.raw, snap.rawTimestampMs);
            parseFloatField("signal", "signalTs", snap.hasSignal, snap.signal, snap.signalTimestampMs);
            parseFloatField("mean", "meanTs", snap.hasMean, snap.mean, snap.meanTimestampMs);
            parseFloatField("rms", "rmsTs", snap.hasRms, snap.rms, snap.rmsTimestampMs);
            parseFloatField("domHz", "domHzTs", snap.hasDomHz, snap.domHz, snap.domTimestampMs);
            auto parseUintField = [&](const char* key, const char* tsKey, bool& hasField, uint16_t& value, uint64_t& ts) {
                if (bioNode.contains(key) && bioNode[key].is_number()) {
                    value = static_cast<uint16_t>(std::max(0, bioNode[key].get<int>()));
                    hasField = true;
                }
                if (hasField && bioNode.contains(tsKey)) {
                    if (bioNode[tsKey].is_number_unsigned()) {
                        ts = bioNode[tsKey].get<uint64_t>();
                    } else if (bioNode[tsKey].is_number_integer()) {
                        ts = static_cast<uint64_t>(bioNode[tsKey].get<int64_t>());
                    }
                }
            };
            parseUintField("sampleRate", "sampleRateTs", snap.hasSampleRate, snap.sampleRate, snap.sampleRateTimestampMs);
            parseUintField("window", "windowTs", snap.hasWindow, snap.window, snap.windowTimestampMs);
            if (snap.hasRaw || snap.hasSignal || snap.hasMean || snap.hasRms || snap.hasDomHz || snap.hasSampleRate || snap.hasWindow) {
                state.sensors.bioAmpDefined = true;
            }
        }
    }
    return state;
}

inline bool ConsoleStore::saveState(const std::string& path, const ConsolePresentationState& state) {
    if (path.empty()) return false;
    ofJson root;
    root["version"] = 3;
    ofJson arr = ofJson::array();
    for (const auto& l : state.layers) {
        ofJson n;
        n["index"] = l.index;
        n["assetId"] = l.assetId;
        n["active"] = l.active;
        n["opacity"] = l.opacity;
        if (!l.label.empty()) {
            n["label"] = l.label;
            n["displayName"] = l.label;
        } else if (!l.displayName.empty()) {
            n["label"] = l.displayName;
            n["displayName"] = l.displayName;
        }
        if (l.coverage.defined) {
            ofJson coverage = ofJson::object();
            if (!l.coverage.mode.empty()) {
                coverage["mode"] = l.coverage.mode;
            }
            coverage["columns"] = std::max(0, l.coverage.columns);
            n["coverage"] = std::move(coverage);
        }
        arr.push_back(n);
    }
    root["layers"] = arr;

    ofJson overlayNode;
    overlayNode["hudVisible"] = state.overlays.hudVisible;
    overlayNode["consoleVisible"] = state.overlays.consoleVisible;
    overlayNode["controlHubVisible"] = state.overlays.controlHubVisible;
    overlayNode["menuVisible"] = state.overlays.menuVisible;
    root["overlays"] = overlayNode;

    ofJson dualNode;
    dualNode["mode"] = state.dualDisplay.mode.empty() ? "single" : state.dualDisplay.mode;
    root["dualDisplay"] = dualNode;

    ofJson secondaryNode;
    secondaryNode["enabled"] = state.secondaryDisplay.enabled;
    secondaryNode["monitorId"] = state.secondaryDisplay.monitorId;
    secondaryNode["x"] = state.secondaryDisplay.x;
    secondaryNode["y"] = state.secondaryDisplay.y;
    secondaryNode["width"] = state.secondaryDisplay.width;
    secondaryNode["height"] = state.secondaryDisplay.height;
    secondaryNode["vsync"] = state.secondaryDisplay.vsync;
    secondaryNode["dpiScale"] = state.secondaryDisplay.dpiScale;
    secondaryNode["background"] = state.secondaryDisplay.background;
    secondaryNode["followPrimary"] = state.secondaryDisplay.followPrimary;
    root["secondaryDisplay"] = secondaryNode;

    ofJson focusNode;
    focusNode["consolePreferred"] = state.controllerFocus.consolePreferred;
    root["controllerFocus"] = focusNode;

    auto serializeSnapshot = [](const ConsoleOverlayLayoutSnapshot& snapshot) -> ofJson {
        ofJson node = ofJson::object();
        if (snapshot.capturedAtMs != 0) {
            node["capturedAtMs"] = snapshot.capturedAtMs;
        }
        if (!snapshot.widgets.empty()) {
            ofJson widgets = ofJson::array();
            for (const auto& placement : snapshot.widgets) {
                if (placement.id.empty()) {
                    continue;
                }
                ofJson widget;
                widget["id"] = placement.id;
                widget["column"] = placement.columnIndex;
                widget["visible"] = placement.visible;
                widget["collapsed"] = placement.collapsed;
                if (!placement.bandId.empty()) {
                    widget["band"] = placement.bandId;
                }
                if (!placement.target.empty()) {
                    widget["target"] = placement.target;
                }
                widgets.push_back(std::move(widget));
            }
            if (!widgets.empty()) {
                node["widgets"] = std::move(widgets);
            }
        }
        return node;
    };

    const bool hasOverlayLayouts =
        state.overlayLayoutsDefined ||
        !state.overlayLayouts.projector.widgets.empty() ||
        !state.overlayLayouts.controller.widgets.empty() ||
        state.overlayLayouts.lastSyncMs != 0;
    if (hasOverlayLayouts) {
        ofJson layoutsNode = ofJson::object();
        if (!state.overlayLayouts.activeTarget.empty()) {
            layoutsNode["activeTarget"] = state.overlayLayouts.activeTarget;
        }
        if (state.overlayLayouts.lastSyncMs != 0) {
            layoutsNode["lastSyncMs"] = state.overlayLayouts.lastSyncMs;
        }
        auto projectorSnapshot = serializeSnapshot(state.overlayLayouts.projector);
        if (!projectorSnapshot.empty()) {
            layoutsNode["projector"] = std::move(projectorSnapshot);
        }
        auto controllerSnapshot = serializeSnapshot(state.overlayLayouts.controller);
        if (!controllerSnapshot.empty()) {
            layoutsNode["controller"] = std::move(controllerSnapshot);
        }
        if (!layoutsNode.empty()) {
            root["overlayLayouts"] = std::move(layoutsNode);
        }
    }

    auto serializeBioField = [](ofJson& node,
                                const char* valueKey,
                                const char* tsKey,
                                bool hasField,
                                float value,
                                uint64_t ts) {
        if (!hasField) return;
        node[valueKey] = value;
        if (ts != 0) {
            node[tsKey] = ts;
        }
    };
    auto serializeBioUintField = [](ofJson& node,
                                    const char* valueKey,
                                    const char* tsKey,
                                    bool hasField,
                                    uint16_t value,
                                    uint64_t ts) {
        if (!hasField) return;
        node[valueKey] = value;
        if (ts != 0) {
            node[tsKey] = ts;
        }
    };
    if (state.sensorsDefined && state.sensors.bioAmpDefined) {
        ofJson sensorsNode = ofJson::object();
        ofJson bioNode = ofJson::object();
        const auto& snap = state.sensors.bioAmp;
        serializeBioField(bioNode, "raw", "rawTs", snap.hasRaw, snap.raw, snap.rawTimestampMs);
        serializeBioField(bioNode, "signal", "signalTs", snap.hasSignal, snap.signal, snap.signalTimestampMs);
        serializeBioField(bioNode, "mean", "meanTs", snap.hasMean, snap.mean, snap.meanTimestampMs);
        serializeBioField(bioNode, "rms", "rmsTs", snap.hasRms, snap.rms, snap.rmsTimestampMs);
        serializeBioField(bioNode, "domHz", "domHzTs", snap.hasDomHz, snap.domHz, snap.domTimestampMs);
        serializeBioUintField(bioNode, "sampleRate", "sampleRateTs", snap.hasSampleRate, snap.sampleRate, snap.sampleRateTimestampMs);
        serializeBioUintField(bioNode, "window", "windowTs", snap.hasWindow, snap.window, snap.windowTimestampMs);
        if (!bioNode.empty()) {
            sensorsNode["bioamp"] = std::move(bioNode);
        }
        if (!sensorsNode.empty()) {
            root["sensors"] = std::move(sensorsNode);
        }
    }

    auto dir = ofFilePath::getEnclosingDirectory(path, false);
    if (!dir.empty()) {
        ofDirectory::createDirectory(dir, true, true);
    }
    std::string tmp = path + ".tmp";
    if (!ofSavePrettyJson(tmp, root)) {
        ofLogWarning("ConsoleStore") << "Failed to write temp console config: " << tmp;
        try { ofFile::removeFile(tmp); } catch (...) {}
        return false;
    }
    ofFile::removeFile(path);
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        ofLogWarning("ConsoleStore") << "Failed to rename " << tmp << " -> " << path;
        try { ofFile::removeFile(tmp); } catch (...) {}
        return false;
    }
    return true;
}

inline std::vector<ConsoleLayerInfo> ConsoleStore::load(const std::string& path) {
    return loadState(path).layers;
}

inline bool ConsoleStore::save(const std::string& path, const std::vector<ConsoleLayerInfo>& layers) {
    ConsolePresentationState state;
    state.layers = layers;
    return saveState(path, state);
}

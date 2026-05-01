#include "LayerLibrary.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include <algorithm>
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

        if (!cfg.contains("id") || !cfg.contains("type")) {
            ofLogWarning("LayerLibrary") << "skipping layer config missing required fields: " << file.getAbsolutePath();
            continue;
        }

        Entry entry;
        entry.id = cfg.value("id", file.getBaseName());
        entry.label = cfg.value("label", entry.id);
        entry.category = cfg.value("category", std::string("Unsorted"));
        entry.type = cfg.value("type", std::string());
        entry.registryPrefix = cfg.value("registryPrefix", entry.id);
        double rawOpacity = 1.0;
        if (cfg.contains("opacity")) {
            if (cfg["opacity"].is_number()) {
                rawOpacity = cfg["opacity"].get<double>();
            } else {
                ofLogWarning("LayerLibrary") << "opacity for " << entry.id << " must be numeric: " << file.getAbsolutePath();
            }
        }
        rawOpacity = std::clamp(rawOpacity, 0.0, 1.0);
        entry.opacity = static_cast<float>(rawOpacity);
        entry.configPath = file.getAbsolutePath();
        entry.config = cfg;
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

        entries_.push_back(std::move(entry));
    }

    std::sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        if (a.category == b.category) {
            return a.label < b.label;
        }
        return a.category < b.category;
    });

    return !entries_.empty();
}

const LayerLibrary::Entry* LayerLibrary::find(const std::string& id) const {
    auto it = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
        return entry.id == id;
    });
    return it != entries_.end() ? &(*it) : nullptr;
}

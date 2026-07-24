#pragma once

#include "ofMain.h"
#include "ofJson.h"
#include <string>
#include <vector>

class LayerLibrary {
public:
    struct Entry {
        std::string id;
        std::string label;
        std::string category;
        std::string layerGroup;
        std::string model;
        std::string stateModel;
        std::string type;
        std::string configPath;
        std::string registryPrefix;
        float opacity = 1.0f;
        ofJson config;
        struct ModeInfo {
            std::string id;
            std::string label;
            std::string kind;
            std::string description;
            bool live = false;
        };
        std::vector<ModeInfo> modes;
        struct Coverage {
            bool defined = false;
            std::string mode = "upstream";
            int columns = 0;
        } coverage;
        struct HudWidget {
            bool enabled = false;
            std::string module;
            std::string toggleId;
            std::string defaultBand = "hud";
            int defaultColumn = 0;
            std::vector<std::string> telemetryFeeds;
        } hud;
        bool isHudWidget() const { return hud.enabled && !hud.module.empty(); }
    };

    bool reload(const std::string& rootDir);
    // Loads only explicitly enabled, source-registered package catalog entries.
    // A missing/disabled activation file is a successful no-op.
    bool loadOptInPackages(const std::string& activationPath);
    // Resolves a package-global preset selection for a future layer
    // instantiation. The catalog entry and any current layer remain unchanged.
    bool configForPackagePreset(const std::string& assetId,
                                const std::string& bankId,
                                const std::string& presetId,
                                ofJson& resolvedConfig,
                                std::string* error = nullptr) const;

    const std::vector<Entry>& entries() const { return entries_; }
    const Entry* find(const std::string& id) const;

private:
    bool appendConfig(const ofJson& cfg, const std::string& configPath);
    void sortEntries();
    std::vector<Entry> entries_;
};

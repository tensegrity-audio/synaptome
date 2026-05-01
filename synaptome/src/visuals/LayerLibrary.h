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
        std::string type;
        std::string configPath;
        std::string registryPrefix;
        float opacity = 1.0f;
        ofJson config;
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

    const std::vector<Entry>& entries() const { return entries_; }
    const Entry* find(const std::string& id) const;

private:
    std::vector<Entry> entries_;
};

#pragma once

#include "ofJson.h"
#include <string>
#include <unordered_map>
#include <vector>

class VideoCatalog {
public:
    struct Clip {
        std::string id;
        std::string label;
        std::string path;
        bool loop = true;
        bool prewarm = false;
    };

    static VideoCatalog& instance();

    void reload();

    const Clip* clipById(const std::string& id) const;
    const Clip* clipByIndex(std::size_t index) const;
    int indexForClip(const std::string& id) const;

    const std::vector<Clip>& clips() const;

private:
    void ensureLoaded() const;

    mutable bool loaded_ = false;
    mutable std::vector<Clip> clips_;
    mutable std::unordered_map<std::string, std::size_t> clipIndex_;
};

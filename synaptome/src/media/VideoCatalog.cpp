#include "VideoCatalog.h"
#include "ofFileUtils.h"
#include "ofJson.h"
#include "ofLog.h"
#include "ofUtils.h"
#include <utility>

VideoCatalog& VideoCatalog::instance() {
    static VideoCatalog catalog;
    return catalog;
}

void VideoCatalog::reload() {
    loaded_ = false;
    clips_.clear();
    clipIndex_.clear();
}

const VideoCatalog::Clip* VideoCatalog::clipById(const std::string& id) const {
    ensureLoaded();
    auto it = clipIndex_.find(id);
    if (it == clipIndex_.end()) {
        return nullptr;
    }
    std::size_t idx = it->second;
    if (idx >= clips_.size()) {
        return nullptr;
    }
    return &clips_[idx];
}

const VideoCatalog::Clip* VideoCatalog::clipByIndex(std::size_t index) const {
    ensureLoaded();
    if (index >= clips_.size()) {
        return nullptr;
    }
    return &clips_[index];
}

int VideoCatalog::indexForClip(const std::string& id) const {
    ensureLoaded();
    auto it = clipIndex_.find(id);
    if (it == clipIndex_.end()) {
        return -1;
    }
    return static_cast<int>(it->second);
}

const std::vector<VideoCatalog::Clip>& VideoCatalog::clips() const {
    ensureLoaded();
    return clips_;
}

void VideoCatalog::ensureLoaded() const {
    if (loaded_) {
        return;
    }
    loaded_ = true;

    std::string jsonPath = ofToDataPath("config/videos.json", true);
    ofFile file(jsonPath);
    if (!file.exists()) {
        ofLogWarning("VideoCatalog") << "videos.json missing at " << jsonPath;
        return;
    }

    ofJson data;
    try {
        data = ofLoadJson(jsonPath);
    } catch (const std::exception& exc) {
        ofLogWarning("VideoCatalog") << "failed to parse " << jsonPath << ": " << exc.what();
        return;
    }

    if (!data.contains("clips") || !data["clips"].is_array()) {
        ofLogWarning("VideoCatalog") << "videos.json missing 'clips' array";
        return;
    }

    const auto& clipArray = data["clips"];
    for (const auto& entry : clipArray) {
        if (!entry.is_object()) continue;
        if (!entry.contains("id") || !entry["id"].is_string()) continue;
        if (!entry.contains("path") || !entry["path"].is_string()) continue;

        Clip clip;
        clip.id = entry["id"].get<std::string>();
        clip.label = entry.value("label", clip.id);
        clip.path = entry["path"].get<std::string>();
        clip.loop = entry.value("loop", true);
        clip.prewarm = entry.value("prewarm", false);

        clipIndex_[clip.id] = clips_.size();
        clips_.push_back(std::move(clip));
    }

    if (clips_.empty()) {
        ofLogWarning("VideoCatalog") << "videos.json contained no valid clips";
    }
}

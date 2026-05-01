#include "VideoClipLayer.h"
#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr float kGainMin = 0.0f;
    constexpr float kGainMax = 4.0f;
    constexpr float kGainStep = 0.05f;
}

void VideoClipLayer::configure(const ofJson& config) {
    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        paramGain_ = def.value("gain", paramGain_);
        paramMirror_ = def.value("mirror", paramMirror_);
        paramLoop_ = def.value("loop", paramLoop_);
        if (def.contains("clipId") && def["clipId"].is_string()) {
            configClipId_ = def["clipId"].get<std::string>();
        }
    }
}

void VideoClipLayer::setup(ParameterRegistry& registry) {
    auto& catalog = VideoCatalog::instance();
    const auto& clips = catalog.clips();
    if (!clips.empty()) {
        if (!configClipId_.empty()) {
            int idx = catalog.indexForClip(configClipId_);
            if (idx >= 0) {
                paramClipIndex_ = static_cast<float>(idx);
            }
        }
        paramClipIndex_ = static_cast<float>(ofClamp(static_cast<int>(std::round(paramClipIndex_)), 0, static_cast<int>(clips.size()) - 1));
    } else {
        paramClipIndex_ = 0.0f;
    }

    const std::string prefix = registryPrefix().empty() ? "layer.clip" : registryPrefix();

    ParameterRegistry::Descriptor visMeta;
    visMeta.label = "Clip Visible";
    visMeta.group = "Media";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, visMeta);

    ParameterRegistry::Descriptor gainMeta;
    gainMeta.label = "Clip Gain";
    gainMeta.group = "Media";
    gainMeta.range.min = kGainMin;
    gainMeta.range.max = kGainMax;
    gainMeta.range.step = kGainStep;
    registry.addFloat(prefix + ".gain", &paramGain_, paramGain_, gainMeta);

    ParameterRegistry::Descriptor mirrorMeta;
    mirrorMeta.label = "Clip Mirror";
    mirrorMeta.group = "Media";
    registry.addBool(prefix + ".mirror", &paramMirror_, paramMirror_, mirrorMeta);

    ParameterRegistry::Descriptor loopMeta;
    loopMeta.label = "Clip Loop";
    loopMeta.group = "Media";
    registry.addBool(prefix + ".loop", &paramLoop_, paramLoop_, loopMeta);

    ParameterRegistry::Descriptor clipMeta;
    clipMeta.label = "Clip Index";
    clipMeta.group = "Media";
    clipMeta.range.min = 0.0f;
    clipMeta.range.max = clips.empty() ? 0.0f : static_cast<float>(std::max(0, static_cast<int>(clips.size()) - 1));
    clipMeta.range.step = 1.0f;
    registry.addFloat(prefix + ".clip", &paramClipIndex_, paramClipIndex_, clipMeta);

    clipDirty_ = true;
    applyClipSelection();
}

void VideoClipLayer::update(const LayerUpdateParams& params) {
    (void)params;
    enabled_ = paramEnabled_;
    if (!enabled_) {
        if (player_.isLoaded()) {
            player_.setPaused(true);
        }
        return;
    }

    if (clipDirty_) {
        applyClipSelection();
    }

    if (player_.isLoaded()) {
        player_.setLoopState(paramLoop_ ? OF_LOOP_NORMAL : OF_LOOP_NONE);
        if (!player_.isPlaying()) {
            player_.play();
        }
        player_.setPaused(false);
        player_.update();
    }
}

void VideoClipLayer::draw(const LayerDrawParams& params) {
    if (!enabled_) return;
    if (!player_.isLoaded()) return;
    if (params.slotOpacity <= 0.0f) return;

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    float gain = ofClamp(paramGain_, kGainMin, kGainMax);
    float alpha = ofClamp(params.slotOpacity, 0.0f, 1.0f);
    int colorValue = static_cast<int>(ofClamp(gain * 255.0f, 0.0f, 255.0f));
    ofSetColor(colorValue, colorValue, colorValue, static_cast<int>(alpha * 255.0f));

    ofPushMatrix();
    if (paramMirror_) {
        ofTranslate(params.viewport.x, 0.0f);
        ofScale(-1.0f, 1.0f);
    }
    player_.draw(0, 0, params.viewport.x, params.viewport.y);
    ofPopMatrix();

    ofPopView();
    ofPopStyle();
}

void VideoClipLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void VideoClipLayer::setExternalEnabled(bool enabled) {
    paramEnabled_ = enabled;
    enabled_ = enabled;
    if (!enabled && player_.isLoaded()) {
        player_.setPaused(true);
    }
}

void VideoClipLayer::cycleClip(int delta) {
    auto& catalog = VideoCatalog::instance();
    const auto& clips = catalog.clips();
    if (clips.empty()) return;
    int count = static_cast<int>(clips.size());
    int current = selectedClipIndex_ >= 0 ? selectedClipIndex_ : 0;
    current = (current + delta) % count;
    if (current < 0) current += count;
    setClipIndex(current);
}

void VideoClipLayer::setClipIndex(int index) {
    auto& catalog = VideoCatalog::instance();
    const auto& clips = catalog.clips();
    if (clips.empty()) return;
    index = ofClamp(index, 0, static_cast<int>(clips.size()) - 1);
    paramClipIndex_ = static_cast<float>(index);
    clipDirty_ = true;
}

std::string VideoClipLayer::currentClipLabel() const {
    if (selectedClipIndex_ < 0) return "(none)";
    auto& catalog = VideoCatalog::instance();
    const auto* clip = catalog.clipByIndex(static_cast<std::size_t>(selectedClipIndex_));
    if (!clip) return "(none)";
    return clip->label;
}

void VideoClipLayer::adjustGain(float delta) {
    paramGain_ = ofClamp(paramGain_ + delta, kGainMin, kGainMax);
}

void VideoClipLayer::toggleMirror() {
    paramMirror_ = !paramMirror_;
}

void VideoClipLayer::toggleLoop() {
    paramLoop_ = !paramLoop_;
}

void VideoClipLayer::applyClipSelection() {
    clipDirty_ = false;
    auto& catalog = VideoCatalog::instance();
    const auto& clips = catalog.clips();
    if (clips.empty()) {
        if (player_.isLoaded()) {
            player_.stop();
            player_.close();
        }
        selectedClipIndex_ = -1;
        return;
    }

    int desired = static_cast<int>(std::round(paramClipIndex_));
    desired = ofClamp(desired, 0, static_cast<int>(clips.size()) - 1);
    if (desired == selectedClipIndex_ && player_.isLoaded()) {
        return;
    }

    selectedClipIndex_ = desired;
    const auto* clip = catalog.clipByIndex(static_cast<std::size_t>(selectedClipIndex_));
    if (!clip) {
        return;
    }

    if (player_.isLoaded()) {
        player_.stop();
        player_.close();
    }

    if (!player_.load(clip->path)) {
        ofLogWarning("VideoClipLayer") << "Failed to load clip " << clip->path;
        selectedClipIndex_ = -1;
        return;
    }

    paramLoop_ = clip->loop;
    player_.setLoopState(paramLoop_ ? OF_LOOP_NORMAL : OF_LOOP_NONE);
    player_.play();
}

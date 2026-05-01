#include "VideoGrabberLayer.h"
#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofUtils.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr float kGainMin = 0.0f;
    constexpr float kGainMax = 4.0f;
    constexpr float kGainStep = 0.05f;

    uint64_t retryDelayForFailures(uint32_t failureCount) {
        const uint32_t clamped = std::min<uint32_t>(failureCount, 5u);
        return 500u + static_cast<uint64_t>(clamped) * 500u;
    }

    class DefaultGrabber : public VideoGrabberLayer::Grabber {
    public:
        std::vector<ofVideoDevice> listDevices() override { return grabber_.listDevices(); }
        bool isInitialized() const override { return grabber_.isInitialized(); }
        void close() override { grabber_.close(); }
        void update() override { grabber_.update(); }
        bool isFrameNew() const override { return grabber_.isFrameNew(); }
        float getWidth() const override { return grabber_.getWidth(); }
        float getHeight() const override { return grabber_.getHeight(); }
        ofTexture* getTexture() override {
            return grabber_.isInitialized() ? &grabber_.getTexture() : nullptr;
        }
        void draw(float x, float y, float w, float h) override { grabber_.draw(x, y, w, h); }
        void setDeviceID(int id) override { grabber_.setDeviceID(id); }
        void setDesiredFrameRate(int fps) override { grabber_.setDesiredFrameRate(fps); }
        bool setup(int width, int height) override { return grabber_.setup(width, height); }

    private:
        ofVideoGrabber grabber_;
    };
}

void VideoGrabberLayer::setGrabberForTesting(const std::shared_ptr<Grabber>& grabber) {
    grabber_ = grabber;
}

VideoGrabberLayer::Grabber& VideoGrabberLayer::grabber() {
    if (!grabber_) {
        grabber_ = std::make_shared<DefaultGrabber>();
    }
    return *grabber_;
}

const VideoGrabberLayer::Grabber& VideoGrabberLayer::grabber() const {
    return const_cast<VideoGrabberLayer*>(this)->grabber();
}

void VideoGrabberLayer::configure(const ofJson& config) {
    if (config.contains("deferOpen") && config["deferOpen"].is_boolean()) {
        deferOpen_ = config["deferOpen"].get<bool>();
    }
    if (config.contains("deferredOpenDelayMs") && config["deferredOpenDelayMs"].is_number_integer()) {
        deferredOpenDelayMs_ = static_cast<uint64_t>(std::max(0, config["deferredOpenDelayMs"].get<int>()));
    }
    if (config.contains("deferredOpenFrames") && config["deferredOpenFrames"].is_number_integer()) {
        deferredOpenFrameDelay_ = std::max(0, config["deferredOpenFrames"].get<int>());
    }

    if (config.contains("defaults")) {
        const auto& def = config["defaults"];
        paramGain_ = def.value("gain", paramGain_);
        paramMirror_ = def.value("mirror", paramMirror_);
        if (def.contains("deferOpen") && def["deferOpen"].is_boolean()) {
            deferOpen_ = def["deferOpen"].get<bool>();
        }
        if (def.contains("deferredOpenDelayMs") && def["deferredOpenDelayMs"].is_number_integer()) {
            deferredOpenDelayMs_ = static_cast<uint64_t>(std::max(0, def["deferredOpenDelayMs"].get<int>()));
        }
        if (def.contains("deferredOpenFrames") && def["deferredOpenFrames"].is_number_integer()) {
            deferredOpenFrameDelay_ = std::max(0, def["deferredOpenFrames"].get<int>());
        }
        if (def.contains("deviceIndex") && def["deviceIndex"].is_number()) {
            paramDeviceIndex_ = static_cast<float>(def["deviceIndex"].get<int>());
            configDeviceIndex_ = static_cast<int>(std::round(paramDeviceIndex_));
        }
        if (def.contains("deviceName") && def["deviceName"].is_string()) {
            configDeviceName_ = def["deviceName"].get<std::string>();
        }
        if (def.contains("width") && def["width"].is_number_integer()) {
            captureWidth_ = std::max(160, def["width"].get<int>());
        }
        if (def.contains("height") && def["height"].is_number_integer()) {
            captureHeight_ = std::max(120, def["height"].get<int>());
        }
        if (def.contains("fps") && def["fps"].is_number_integer()) {
            desiredFps_ = std::max(1, def["fps"].get<int>());
        }
        if (def.contains("resolutionIndex") && def["resolutionIndex"].is_number_integer()) {
            paramResolutionIndex_ = static_cast<float>(def["resolutionIndex"].get<int>());
        }
    }

    rebuildResolutionOptions(config);

    if (!resolutionOptions_.empty()) {
        int defaultIndex = static_cast<int>(std::round(paramResolutionIndex_));
        defaultIndex = clampResolutionIndex(defaultIndex);

        if (defaultIndex < 0) {
            for (std::size_t i = 0; i < resolutionOptions_.size(); ++i) {
                if (resolutionOptions_[i].width == captureWidth_ && resolutionOptions_[i].height == captureHeight_) {
                    defaultIndex = static_cast<int>(i);
                    break;
                }
            }
        }

        if (defaultIndex < 0) {
            defaultIndex = 0;
        }

        applyResolutionSelection(defaultIndex);
    } else {
        activeResolutionIndex_ = -1;
    }
}

void VideoGrabberLayer::rebuildResolutionOptions(const ofJson& config) {
    resolutionOptions_.clear();

    auto appendOption = [&](int width, int height, const std::string& label) {
        const int minWidth = 16;
        const int minHeight = 16;
        if (width < minWidth || height < minHeight) {
            return;
        }
        auto it = std::find_if(resolutionOptions_.begin(), resolutionOptions_.end(), [&](const ResolutionOption& opt) {
            return opt.width == width && opt.height == height;
        });
        if (it != resolutionOptions_.end()) {
            if (it->label.empty() && !label.empty()) {
                it->label = label;
            }
            return;
        }
        ResolutionOption option;
        option.width = width;
        option.height = height;
        option.label = label;
        resolutionOptions_.push_back(option);
    };

    auto parseSizeArray = [](const ofJson& value, int& width, int& height) {
        if (!value.is_array() || value.size() < 2) {
            return false;
        }
        if (!value[0].is_number_integer() || !value[1].is_number_integer()) {
            return false;
        }
        width = value[0].get<int>();
        height = value[1].get<int>();
        return true;
    };

    int baseWidth = 0;
    int baseHeight = 0;
    if (config.contains("resolutionBase")) {
        const ofJson& base = config["resolutionBase"];
        parseSizeArray(base, baseWidth, baseHeight);
    }

    if (config.contains("resolutions") && config["resolutions"].is_array()) {
        for (const auto& entry : config["resolutions"]) {
            int width = 0;
            int height = 0;
            std::string label;
            if (entry.is_object()) {
                if (entry.contains("width") && entry["width"].is_number_integer()) {
                    width = entry["width"].get<int>();
                }
                if (entry.contains("height") && entry["height"].is_number_integer()) {
                    height = entry["height"].get<int>();
                }
                if (entry.contains("label") && entry["label"].is_string()) {
                    label = entry["label"].get<std::string>();
                }
            } else {
                parseSizeArray(entry, width, height);
            }
            appendOption(width, height, label);
        }
    }

    std::vector<int> multipliers;
    if (config.contains("resolutionMultipliers") && config["resolutionMultipliers"].is_array()) {
        for (const auto& value : config["resolutionMultipliers"]) {
            if (value.is_number_integer()) {
                multipliers.push_back(value.get<int>());
            }
        }
    }

    if (baseWidth > 0 && baseHeight > 0) {
        if (multipliers.empty()) {
            multipliers = { 3, 4, 5 };
        }
        for (int multiplier : multipliers) {
            if (multiplier <= 0) continue;
            int width = baseWidth * multiplier;
            int height = baseHeight * multiplier;
            std::string label = std::to_string(width) + "x" + std::to_string(height) + " (" + std::to_string(multiplier) + "x)";
            appendOption(width, height, label);
        }
    }

    if (resolutionOptions_.empty()) {
        appendOption(captureWidth_, captureHeight_, "");
    }

    for (auto& option : resolutionOptions_) {
        if (option.label.empty()) {
            option.label = std::to_string(option.width) + "x" + std::to_string(option.height);
        }
    }

    std::sort(resolutionOptions_.begin(), resolutionOptions_.end(), [](const ResolutionOption& a, const ResolutionOption& b) {
        if (a.width != b.width) return a.width < b.width;
        return a.height < b.height;
    });
}

int VideoGrabberLayer::clampResolutionIndex(int index) const {
    if (resolutionOptions_.empty()) {
        return -1;
    }
    int minIndex = 0;
    int maxIndex = static_cast<int>(resolutionOptions_.size()) - 1;
    if (index < minIndex) return minIndex;
    if (index > maxIndex) return maxIndex;
    return index;
}

void VideoGrabberLayer::applyResolutionSelection(int index) {
    if (resolutionOptions_.empty()) {
        activeResolutionIndex_ = -1;
        return;
    }
    int clamped = clampResolutionIndex(index);
    if (clamped < 0) {
        activeResolutionIndex_ = -1;
        return;
    }
    const auto& option = resolutionOptions_[clamped];
    captureWidth_ = std::max(160, option.width);
    captureHeight_ = std::max(120, option.height);
    activeResolutionIndex_ = clamped;
    paramResolutionIndex_ = static_cast<float>(clamped);
}

std::string VideoGrabberLayer::resolutionOptionsSummary() const {
    if (resolutionOptions_.empty()) {
        return "";
    }
    std::ostringstream oss;
    for (std::size_t i = 0; i < resolutionOptions_.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << i << ":" << resolutionOptions_[i].width << "x" << resolutionOptions_[i].height;
        if (!resolutionOptions_[i].label.empty()) {
            oss << " (" << resolutionOptions_[i].label << ")";
        }
    }
    return oss.str();
}


void VideoGrabberLayer::setup(ParameterRegistry& registry) {
    refreshDevices();

    if (!devices_.empty()) {
        if (!configDeviceName_.empty()) {
            std::string needle = ofToLower(configDeviceName_);
            for (std::size_t i = 0; i < devices_.size(); ++i) {
                if (ofIsStringInString(ofToLower(devices_[i].label), needle)) {
                    paramDeviceIndex_ = static_cast<float>(i);
                    break;
                }
            }
        } else if (configDeviceIndex_ >= 0 && configDeviceIndex_ < static_cast<int>(devices_.size())) {
            paramDeviceIndex_ = static_cast<float>(configDeviceIndex_);
        }
        paramDeviceIndex_ = static_cast<float>(ofClamp(static_cast<int>(std::round(paramDeviceIndex_)), 0, static_cast<int>(devices_.size()) - 1));
    } else {
        paramDeviceIndex_ = 0.0f;
    }

    const std::string prefix = registryPrefix().empty() ? "layer.webcam" : registryPrefix();

    ParameterRegistry::Descriptor visMeta;
    visMeta.label = "Webcam Visible";
    visMeta.group = "Media";
    registry.addBool(prefix + ".visible", &paramEnabled_, paramEnabled_, visMeta);

    ParameterRegistry::Descriptor gainMeta;
    gainMeta.label = "Webcam Gain";
    gainMeta.group = "Media";
    gainMeta.range.min = kGainMin;
    gainMeta.range.max = kGainMax;
    gainMeta.range.step = kGainStep;
    registry.addFloat(prefix + ".gain", &paramGain_, paramGain_, gainMeta);

    ParameterRegistry::Descriptor mirrorMeta;
    mirrorMeta.label = "Webcam Mirror";
    mirrorMeta.group = "Media";
    registry.addBool(prefix + ".mirror", &paramMirror_, paramMirror_, mirrorMeta);

    ParameterRegistry::Descriptor deviceMeta;
    deviceMeta.label = "Webcam Device";
    deviceMeta.group = "Media";
    deviceMeta.range.min = 0.0f;
    deviceMeta.range.max = devices_.empty() ? 0.0f : static_cast<float>(std::max(0, static_cast<int>(devices_.size()) - 1));
    deviceMeta.range.step = 1.0f;
    registry_ = &registry;
    deviceParamId_ = prefix + ".device";
    registry.addFloat(deviceParamId_, &paramDeviceIndex_, paramDeviceIndex_, deviceMeta);

    if (!resolutionOptions_.empty()) {
        ParameterRegistry::Descriptor resMeta;
        resMeta.label = "Webcam Resolution";
        resMeta.group = "Media";
        resMeta.range.min = 0.0f;
        resMeta.range.max = static_cast<float>(std::max(0, static_cast<int>(resolutionOptions_.size()) - 1));
        resMeta.range.step = 1.0f;
        const std::string summary = resolutionOptionsSummary();
        if (!summary.empty()) {
            resMeta.description = summary;
        }
        registry.addFloat(prefix + ".resolution", &paramResolutionIndex_, paramResolutionIndex_, resMeta);
    }

    ParameterRegistry::Descriptor overlayMeta;
    overlayMeta.label = "Webcam Device Overlay";
    overlayMeta.group = "Media";
    registry.addBool(prefix + ".deviceInfoOverlay", &paramShowDeviceOverlay_, paramShowDeviceOverlay_, overlayMeta);

    updateDeviceParamMetadata();
    deviceDirty_ = true;
    if (deferOpen_) {
        scheduleDeferredOpen();
    } else {
        tryApplyDeviceSelection("setup");
    }
}



void VideoGrabberLayer::update(const LayerUpdateParams& params) {
    (void)params;
    enabled_ = paramEnabled_;
    if (!enabled_) {
        return;
    }

    if (!resolutionOptions_.empty()) {
        int desiredResolution = clampResolutionIndex(static_cast<int>(std::round(paramResolutionIndex_)));
        if (desiredResolution >= 0 && desiredResolution != activeResolutionIndex_) {
            applyResolutionSelection(desiredResolution);
            deviceDirty_ = true;
        }
    }

    if (!std::isfinite(lastDeviceParamValue_) || std::fabs(lastDeviceParamValue_ - paramDeviceIndex_) > 0.001f) {
        lastDeviceParamValue_ = paramDeviceIndex_;
        deviceDirty_ = true;
    }

    uint64_t now = ofGetElapsedTimeMillis();

    bool openingDeferredDevice = false;
    if (pendingDeferredOpen_) {
        if (deferredOpenFramesRemaining_ > 0) {
            --deferredOpenFramesRemaining_;
            return;
        }
        if (now < deferredOpenReadyMs_) {
            return;
        }
        pendingDeferredOpen_ = false;
        openingDeferredDevice = true;
        ofLogNotice("VideoGrabberLayer") << "Deferred webcam open starting for " << currentDeviceLabel();
    }

    if (deviceDirty_ && now >= nextDeviceRetryMs_) {
        if (!tryApplyDeviceSelection(openingDeferredDevice ? "deferred open" : "update")) {
            return;
        }
    }

    if (grabber().isInitialized()) {
        try {
            grabber().update();
        } catch (const std::exception& ex) {
            handleGrabberException(ex);
            return;
        } catch (...) {
            handleGrabberException(std::runtime_error("Unknown webcam exception during update"));
            return;
        }

        now = ofGetElapsedTimeMillis();
        bool frameNew = grabber().isFrameNew();
        handleFrameTiming(frameNew, now);
    }
}


void VideoGrabberLayer::draw(const LayerDrawParams& params) {
    if (!enabled_) return;
    if (!grabber().isInitialized()) return;
    if (params.slotOpacity <= 0.0f) return;

    ofPushStyle();
    ofPushView();
    ofViewport(0, 0, params.viewport.x, params.viewport.y);
    ofSetupScreenOrtho(params.viewport.x, params.viewport.y, -1, 1);

    float gain = ofClamp(paramGain_, kGainMin, kGainMax);
    float alpha = ofClamp(params.slotOpacity, 0.0f, 1.0f);
    float colorScale = ofClamp(gain, 0.0f, kGainMax);
    int colorValue = static_cast<int>(ofClamp(colorScale * 255.0f, 0.0f, 255.0f));
    ofSetColor(colorValue, colorValue, colorValue, static_cast<int>(alpha * 255.0f));

    const float destWidth = static_cast<float>(params.viewport.x);
    const float destHeight = static_cast<float>(params.viewport.y);

    ofPushMatrix();
    if (paramMirror_) {
        ofTranslate(destWidth, 0.0f);
        ofScale(-1.0f, 1.0f);
    }

    if (destWidth > 0.0f && destHeight > 0.0f) {
        float srcWidth = actualWidth_ > 0 ? static_cast<float>(actualWidth_) : static_cast<float>(grabber().getWidth());
        float srcHeight = actualHeight_ > 0 ? static_cast<float>(actualHeight_) : static_cast<float>(grabber().getHeight());

        if (srcWidth <= 0.0f || srcHeight <= 0.0f) {
            grabber().draw(0.0f, 0.0f, destWidth, destHeight);
        } else {
            float scale = destWidth / srcWidth;
            if (!std::isfinite(scale) || scale <= 0.0f) {
                grabber().draw(0.0f, 0.0f, destWidth, destHeight);
            } else {
                float scaledHeight = srcHeight * scale;
                float destY = 0.0f;
                float drawHeight = destHeight;
                float srcY = 0.0f;
                float srcDrawHeight = srcHeight;

                if (scaledHeight >= destHeight - 0.5f) {
                    float visibleSrcHeight = destHeight / scale;
                    visibleSrcHeight = std::min(visibleSrcHeight, srcHeight);
                    srcY = (srcHeight - visibleSrcHeight) * 0.5f;
                    srcDrawHeight = visibleSrcHeight;
                } else {
                    drawHeight = scaledHeight;
                    destY = (destHeight - drawHeight) * 0.5f;
                }

                const ofTexture* sourceTexture = nullptr;
                if (auto* texture = grabber().getTexture(); texture && texture->isAllocated()) {
                    sourceTexture = texture;
                }
                if (sourceTexture && sourceTexture->isAllocated()) {
                    static uint64_t lastLogMs = 0;
                    uint64_t nowMs = ofGetElapsedTimeMillis();
                    if (nowMs - lastLogMs > 1000) {
                        lastLogMs = nowMs;
                        ofLogNotice("VideoGrabberLayer") << "Draw frame - mirror=" << paramMirror_
                                                         << " slotOpacity=" << params.slotOpacity;
                    }
                    sourceTexture->drawSubsection(0.0f, destY, destWidth, drawHeight, 0.0f, srcY, srcWidth, srcDrawHeight);
                } else {
                    float drawY = destY - srcY * scale;
                    float drawHeightScaled = srcHeight * scale;
                    grabber().draw(0.0f, drawY, destWidth, drawHeightScaled);
                }
            }
        }
    }

    std::string statusText;
    const DeviceInfo* deviceInfo = (selectedDeviceIndex_ >= 0 && selectedDeviceIndex_ < static_cast<int>(devices_.size()))
        ? &devices_[selectedDeviceIndex_]
        : nullptr;
    uint64_t now = ofGetElapsedTimeMillis();
    if (deviceInfo) {
        if (waitingForFirstFrame_) {
            uint64_t pendingMs = now - deviceInfo->lastAttemptMs;
            statusText = "Webcam: waiting for frames (" + ofToString(static_cast<int>(pendingMs)) + " ms)";
        } else if (frameWarningActive_) {
            uint64_t stalledMs = deviceInfo->lastFrameMs > 0 ? now - deviceInfo->lastFrameMs : now - deviceInfo->lastAttemptMs;
            statusText = "Webcam: stalled (" + ofToString(static_cast<int>(stalledMs)) + " ms)";
        }
    }

    if (!statusText.empty()) {
        static ofBitmapFont bitmapFont;
        ofRectangle bounds = bitmapFont.getBoundingBox(statusText, 0, 0);
        float padding = 6.0f;
        float x = 16.0f;
        float y = 32.0f;
        ofSetColor(0, 0, 0, 160);
        ofDrawRectangle(x - padding, y - bounds.height - padding, bounds.width + padding * 2.0f, bounds.height + padding * 2.0f);
        ofSetColor(255);
        ofDrawBitmapString(statusText, x, y);
    }

    if (paramShowDeviceOverlay_) {
        const std::string summary = deviceListSummary();
        if (!summary.empty()) {
            static ofBitmapFont bitmapFont;
            const auto lines = ofSplitString(summary, "\n");
            float maxWidth = 0.0f;
            float lineHeight = 0.0f;
            for (const auto& line : lines) {
                ofRectangle bounds = bitmapFont.getBoundingBox(line, 0, 0);
                maxWidth = std::max(maxWidth, bounds.width);
                lineHeight = std::max(lineHeight, bounds.height);
            }
            if (lineHeight <= 0.0f) {
                lineHeight = 12.0f;
            }
            float padding = 6.0f;
            float spacing = 4.0f;
            float totalHeight = static_cast<float>(lines.size()) * (lineHeight + spacing);
            float x = 16.0f;
            float yStart = statusText.empty() ? 32.0f : 80.0f;
            ofSetColor(0, 0, 0, 160);
            ofDrawRectangle(x - padding,
                            yStart - lineHeight - padding,
                            maxWidth + padding * 2.0f,
                            totalHeight + padding);
            ofSetColor(255);
            float y = yStart;
            for (const auto& line : lines) {
                ofDrawBitmapString(line, x, y);
                y += lineHeight + spacing;
            }
        }
    }

    ofPopMatrix();

    ofPopView();
    ofPopStyle();
}


void VideoGrabberLayer::onWindowResized(int width, int height) {
    (void)width;
    (void)height;
}

void VideoGrabberLayer::setExternalEnabled(bool enabled) {
    if (paramEnabled_ == enabled) {
        return;
    }

    paramEnabled_ = enabled;
    enabled_ = enabled;

    if (!enabled_) {
        pendingDeferredOpen_ = false;
        waitingForFirstFrame_ = false;
        frameWarningActive_ = false;
        lastFrameLogMs_ = 0;
        if (grabber().isInitialized()) {
            ofLogNotice("VideoGrabberLayer") << "Disabling webcam device " << currentDeviceLabel() << ", closing grabber";
            grabber().close();
            actualWidth_ = 0;
            actualHeight_ = 0;
        }
    } else {
        deviceDirty_ = true;
        if (deferOpen_ && !grabber().isInitialized()) {
            scheduleDeferredOpen();
        }
    }
}

void VideoGrabberLayer::cycleDevice(int delta) {
    if (devices_.empty()) return;
    int count = static_cast<int>(devices_.size());
    int current = selectedDeviceIndex_ >= 0 ? selectedDeviceIndex_ : 0;
    current = (current + delta) % count;
    if (current < 0) current += count;
    setDeviceIndex(current);
}

void VideoGrabberLayer::setDeviceIndex(int index) {
    if (devices_.empty()) return;
    index = ofClamp(index, 0, static_cast<int>(devices_.size()) - 1);
    paramDeviceIndex_ = static_cast<float>(index);
    lastDeviceParamValue_ = paramDeviceIndex_;
    deviceDirty_ = true;
}

std::string VideoGrabberLayer::deviceLabel(int index) const {
    if (index < 0 || index >= static_cast<int>(devices_.size())) return "";
    return devices_[index].label;
}

std::string VideoGrabberLayer::currentDeviceLabel() const {
    if (selectedDeviceIndex_ < 0) return "(none)";
    return deviceLabel(selectedDeviceIndex_);
}

void VideoGrabberLayer::adjustGain(float delta) {
    paramGain_ = ofClamp(paramGain_ + delta, kGainMin, kGainMax);
}

void VideoGrabberLayer::toggleMirror() {
    paramMirror_ = !paramMirror_;
}

void VideoGrabberLayer::forceDeviceRefresh() {
    ofLogNotice("VideoGrabberLayer") << "Manual webcam device refresh requested";
    pendingDeferredOpen_ = false;
    refreshDevices();
    if (deviceDirty_) {
        tryApplyDeviceSelection("manual refresh");
    }
}

void VideoGrabberLayer::refreshDevices() {
    const std::vector<DeviceInfo> previous = devices_;
    int previousSelectedId = -1;
    if (selectedDeviceIndex_ >= 0 && selectedDeviceIndex_ < static_cast<int>(previous.size())) {
        previousSelectedId = previous[selectedDeviceIndex_].id;
    }

    devices_.clear();
    auto deviceList = grabber().listDevices();
    devices_.reserve(deviceList.size());
    for (const auto& dev : deviceList) {
        DeviceInfo info;
        info.id = dev.id;
        info.label = dev.deviceName;
        info.available = dev.bAvailable;
        auto it = std::find_if(previous.begin(), previous.end(), [&](const DeviceInfo& prev) {
            return prev.id == info.id;
        });
        if (it != previous.end()) {
            info.lastSetupSuccess = it->lastSetupSuccess;
            info.consecutiveFailures = it->consecutiveFailures;
            info.lastAttemptMs = it->lastAttemptMs;
            info.lastSuccessMs = it->lastSuccessMs;
            info.lastFrameMs = it->lastFrameMs;
        }
        devices_.push_back(info);
    }

    ofLogNotice("VideoGrabberLayer") << "Detected " << devices_.size() << " video device(s)";
    logDeviceList();
    updateDeviceParamMetadata();
    writeDeviceInventoryArtifact();

    if (devices_.empty()) {
        paramDeviceIndex_ = 0.0f;
        selectedDeviceIndex_ = -1;
        deviceDirty_ = true;
        waitingForFirstFrame_ = false;
        frameWarningActive_ = false;
        lastFrameLogMs_ = 0;
        return;
    }

    int matchedIndex = -1;
    if (previousSelectedId >= 0) {
        for (std::size_t i = 0; i < devices_.size(); ++i) {
            if (devices_[i].id == previousSelectedId) {
                matchedIndex = static_cast<int>(i);
                break;
            }
        }
    }

    if (matchedIndex >= 0) {
        selectedDeviceIndex_ = matchedIndex;
        paramDeviceIndex_ = static_cast<float>(matchedIndex);
    } else {
        int clamped = static_cast<int>(ofClamp(static_cast<int>(std::round(paramDeviceIndex_)), 0,
            static_cast<int>(devices_.size()) - 1));
        selectedDeviceIndex_ = clamped;
        paramDeviceIndex_ = static_cast<float>(clamped);
    }

    deviceDirty_ = true;
}


void VideoGrabberLayer::scheduleDeferredOpen() {
    pendingDeferredOpen_ = true;
    deferredOpenFramesRemaining_ = std::max(0, deferredOpenFrameDelay_);
    deferredOpenReadyMs_ = ofGetElapsedTimeMillis() + deferredOpenDelayMs_;
    ofLogNotice("VideoGrabberLayer") << "Deferring webcam open until after scene publish"
        << " (delay=" << deferredOpenDelayMs_ << " ms, frames=" << deferredOpenFramesRemaining_ << ")";
}


bool VideoGrabberLayer::tryApplyDeviceSelection(const std::string& context) {
    try {
        applyDeviceSelection();
        return true;
    } catch (const std::exception& ex) {
        ofLogWarning("VideoGrabberLayer") << "Webcam device selection failed during " << context
            << ": " << (ex.what() ? ex.what() : "unknown exception");
        handleGrabberException(ex);
        return false;
    } catch (...) {
        std::runtime_error ex("Unknown webcam exception during " + context);
        ofLogWarning("VideoGrabberLayer") << ex.what();
        handleGrabberException(ex);
        return false;
    }
}


void VideoGrabberLayer::applyDeviceSelection() {
    deviceDirty_ = false;
    uint64_t now = ofGetElapsedTimeMillis();

    if (devices_.empty()) {
        if (grabber().isInitialized()) {
            ofLogNotice("VideoGrabberLayer") << "No webcam devices available, closing current grabber";
            grabber().close();
        }
        actualWidth_ = 0;
        actualHeight_ = 0;
        selectedDeviceIndex_ = -1;
        waitingForFirstFrame_ = false;
        frameWarningActive_ = false;
        lastFrameLogMs_ = now;
        nextDeviceRetryMs_ = now + retryDelayForFailures(0);
        return;
    }

    int desired = static_cast<int>(std::round(paramDeviceIndex_));
    desired = ofClamp(desired, 0, static_cast<int>(devices_.size()) - 1);

    if (desired == selectedDeviceIndex_ && grabber().isInitialized()) {
        return;
    }

    if (grabber().isInitialized()) {
        if (selectedDeviceIndex_ >= 0 && selectedDeviceIndex_ < static_cast<int>(devices_.size())) {
            ofLogNotice("VideoGrabberLayer") << "Closing webcam device " << selectedDeviceIndex_
                << " -> " << devices_[selectedDeviceIndex_].label;
        } else {
            ofLogNotice("VideoGrabberLayer") << "Closing active webcam grabber";
        }
        grabber().close();
        actualWidth_ = 0;
        actualHeight_ = 0;
    }

    if (desired != selectedDeviceIndex_) {
        ofLogNotice("VideoGrabberLayer") << "Switching webcam selection " << selectedDeviceIndex_ << " -> " << desired;
    } else {
        ofLogNotice("VideoGrabberLayer") << "Re-initializing webcam device index " << desired;
    }

    selectedDeviceIndex_ = desired;

    if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= static_cast<int>(devices_.size())) {
        ofLogWarning("VideoGrabberLayer") << "Desired webcam index out of range: " << desired;
        selectedDeviceIndex_ = -1;
        nextDeviceRetryMs_ = now + retryDelayForFailures(0);
        return;
    }

    auto& device = devices_[selectedDeviceIndex_];
    device.lastAttemptMs = now;

    ofLogNotice("VideoGrabberLayer") << "Opening webcam device " << selectedDeviceIndex_
        << " (id=" << device.id << ", label='" << device.label << "') "
        << captureWidth_ << "x" << captureHeight_ << "@" << desiredFps_ << "fps";

    grabber().setDeviceID(device.id);
    grabber().setDesiredFrameRate(desiredFps_);
    bool success = grabber().setup(captureWidth_, captureHeight_);
    if (!success) {
        ofLogWarning("VideoGrabberLayer") << "Failed to setup device '" << device.label
            << "' (id=" << device.id << "), failure count=" << (device.consecutiveFailures + 1);
        if (device.consecutiveFailures < std::numeric_limits<uint32_t>::max()) {
            device.consecutiveFailures += 1;
        }
        device.lastSetupSuccess = false;
        selectedDeviceIndex_ = -1;
        grabber().close();
        actualWidth_ = 0;
        actualHeight_ = 0;

        uint64_t retryDelay = retryDelayForFailures(device.consecutiveFailures);
        nextDeviceRetryMs_ = now + retryDelay;

        int fallback = findFallbackDevice(desired);
        if (fallback >= 0) {
            ofLogNotice("VideoGrabberLayer") << "Attempting fallback webcam device " << fallback
                << " -> '" << devices_[fallback].label << "'";
            paramDeviceIndex_ = static_cast<float>(fallback);
            nextDeviceRetryMs_ = now;
            deviceDirty_ = true;
        } else {
            ofLogWarning("VideoGrabberLayer") << "No fallback webcam device available";
        }
        waitingForFirstFrame_ = false;
        frameWarningActive_ = false;
        lastFrameLogMs_ = now;
        return;
    }

    actualWidth_ = static_cast<int>(grabber().getWidth());
    actualHeight_ = static_cast<int>(grabber().getHeight());
    if (actualWidth_ <= 0 || actualHeight_ <= 0) {
        actualWidth_ = captureWidth_;
        actualHeight_ = captureHeight_;
    }
    if (actualWidth_ != captureWidth_ || actualHeight_ != captureHeight_) {
        ofLogNotice("VideoGrabberLayer") << "Webcam provided " << actualWidth_ << "x" << actualHeight_
            << " (requested " << captureWidth_ << "x" << captureHeight_ << ")";
    }

    device.lastSetupSuccess = true;
    device.consecutiveFailures = 0;
    device.lastSuccessMs = now;
    device.lastFrameMs = 0;
    waitingForFirstFrame_ = true;
    frameWarningActive_ = false;
    lastFrameLogMs_ = now;
    paramDeviceIndex_ = static_cast<float>(selectedDeviceIndex_);
    nextDeviceRetryMs_ = 0;
    ofLogNotice("VideoGrabberLayer") << "Webcam device '" << device.label << "' initialized successfully";
}






void VideoGrabberLayer::handleFrameTiming(bool frameNew, uint64_t now) {
    if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= static_cast<int>(devices_.size())) {
        return;
    }

    auto& device = devices_[selectedDeviceIndex_];
    uint64_t previousFrameMs = device.lastFrameMs;

    if (frameNew) {
        if (waitingForFirstFrame_) {
            uint64_t startupDelay = now - device.lastAttemptMs;
            if (startupDelay > 0) {
                ofLogNotice("VideoGrabberLayer") << "First webcam frame arrived after " << startupDelay
                    << " ms for '" << device.label << "'";
            }
        } else if (frameWarningActive_) {
            uint64_t gapMs = previousFrameMs > 0 ? now - previousFrameMs : now - lastFrameLogMs_;
            if (gapMs > 0) {
                ofLogNotice("VideoGrabberLayer") << "Webcam frames resumed for '" << device.label
                    << "' after " << gapMs << " ms gap";
            } else {
                ofLogNotice("VideoGrabberLayer") << "Webcam frames resumed for '" << device.label << "'";
            }
        }

        device.lastFrameMs = now;
        waitingForFirstFrame_ = false;
        frameWarningActive_ = false;
        lastFrameLogMs_ = now;
        return;
    }

    uint64_t elapsedSinceFrame = 0;
    if (device.lastFrameMs > 0) {
        elapsedSinceFrame = now - device.lastFrameMs;
    } else if (waitingForFirstFrame_) {
        elapsedSinceFrame = now - device.lastAttemptMs;
    } else {
        return;
    }

    uint64_t warnThreshold = waitingForFirstFrame_ ? 1500 : 2000;
    if (!frameWarningActive_ && elapsedSinceFrame > warnThreshold) {
        if (waitingForFirstFrame_) {
            ofLogWarning("VideoGrabberLayer") << "No webcam frames yet from '" << device.label
                << "' after " << elapsedSinceFrame << " ms";
        } else {
            ofLogWarning("VideoGrabberLayer") << "Webcam stalled, no new frames from '" << device.label
                << "' for " << elapsedSinceFrame << " ms";
        }
        frameWarningActive_ = true;
        lastFrameLogMs_ = now;
    } else if (frameWarningActive_ && now - lastFrameLogMs_ > 2000) {
        ofLogWarning("VideoGrabberLayer") << "Still waiting for webcam frames from '" << device.label
            << "' (" << elapsedSinceFrame << " ms total)";
        lastFrameLogMs_ = now;
    }
}

void VideoGrabberLayer::handleGrabberException(const std::exception& ex) {
    std::string message = ex.what() ? ex.what() : "unknown webcam exception";
    ofLogWarning("VideoGrabberLayer") << "Webcam update raised exception: " << message;

    int failedIndex = selectedDeviceIndex_;
    uint64_t now = ofGetElapsedTimeMillis();
    uint32_t failureCount = 0;
    int fallback = -1;

    if (failedIndex >= 0 && failedIndex < static_cast<int>(devices_.size())) {
        auto& device = devices_[failedIndex];
        device.lastSetupSuccess = false;
        if (device.consecutiveFailures < std::numeric_limits<uint32_t>::max()) {
            device.consecutiveFailures += 1;
        }
        failureCount = device.consecutiveFailures;
        device.lastAttemptMs = now;
        fallback = findFallbackDevice(failedIndex);
    }

    if (grabber().isInitialized()) {
        grabber().close();
    }

    actualWidth_ = 0;
    actualHeight_ = 0;
    waitingForFirstFrame_ = false;
    frameWarningActive_ = false;
    lastFrameLogMs_ = now;

    bool fallbackChosen = false;
    bool preempted = message.find("0xC00D3EA3") != std::string::npos;
    if (preempted) {
        ofLogWarning("VideoGrabberLayer") << "Camera access preempted by another application (0xC00D3EA3)";
    }

    selectedDeviceIndex_ = -1;

    if (fallback >= 0 && fallback < static_cast<int>(devices_.size())) {
        ofLogNotice("VideoGrabberLayer") << "Attempting fallback webcam device " << fallback
            << " -> '" << devices_[fallback].label << "'";
        paramDeviceIndex_ = static_cast<float>(fallback);
        fallbackChosen = true;
    } else if (failedIndex >= 0) {
        if (preempted) {
            ofLogNotice("VideoGrabberLayer") << "Retrying webcam index " << failedIndex << " once it becomes available again";
        }
        paramDeviceIndex_ = static_cast<float>(failedIndex);
    }

    nextDeviceRetryMs_ = fallbackChosen ? now : now + retryDelayForFailures(failureCount);
    deviceDirty_ = true;
}


int VideoGrabberLayer::findFallbackDevice(int failedIndex) const {
    if (devices_.empty()) {
        return -1;
    }
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        if (static_cast<int>(i) == failedIndex) {
            continue;
        }
        const auto& info = devices_[i];
        if (info.available && info.lastSetupSuccess) {
            return static_cast<int>(i);
        }
    }
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        if (static_cast<int>(i) == failedIndex) {
            continue;
        }
        const auto& info = devices_[i];
        if (info.available && info.consecutiveFailures == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void VideoGrabberLayer::logDeviceList() const {
    if (devices_.empty()) {
        ofLogNotice("VideoGrabberLayer") << "No video capture devices detected";
        return;
    }

    uint64_t now = ofGetElapsedTimeMillis();
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        const auto& info = devices_[i];
        std::string status = info.available ? "available" : "unavailable";
        if (info.lastSetupSuccess) {
            status += ", last success";
            if (info.lastSuccessMs > 0) {
                status += " " + ofToString(static_cast<int>(now - info.lastSuccessMs)) + "ms ago";
            }
        } else if (info.consecutiveFailures > 0) {
            status += ", failures=" + ofToString(info.consecutiveFailures);
            if (info.lastAttemptMs > 0) {
                status += ", last attempt " + ofToString(static_cast<int>(now - info.lastAttemptMs)) + "ms ago";
            }
        }
        if (info.lastFrameMs > 0) {
            status += ", last frame " + ofToString(static_cast<int>(now - info.lastFrameMs)) + "ms ago";
        }
        ofLogNotice("VideoGrabberLayer") << "  [" << i << "] id=" << info.id << " '" << info.label << "' (" << status << ")";
    }
}

void VideoGrabberLayer::updateDeviceParamMetadata() {
    if (!registry_) return;
    auto* param = registry_->findFloat(deviceParamId_);
    if (!param) return;
    if (devices_.empty()) {
        param->meta.range.min = 0.0f;
        param->meta.range.max = 0.0f;
        param->meta.description = "No webcam devices detected";
        return;
    }
    param->meta.range.min = 0.0f;
    param->meta.range.max = static_cast<float>(std::max(0, static_cast<int>(devices_.size()) - 1));
    param->meta.range.step = 1.0f;
    param->meta.description = deviceListSummary();
}

std::string VideoGrabberLayer::deviceListSummary() const {
    if (devices_.empty()) return "";
    std::ostringstream oss;
    int armedIndex = static_cast<int>(std::round(paramDeviceIndex_));
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        const auto& info = devices_[i];
        oss << "[" << i << "] id=" << info.id << " '" << info.label << "'";
        if (!info.available) {
            oss << " (offline)";
        }
        if (static_cast<int>(i) == selectedDeviceIndex_) {
            oss << " [active]";
        } else if (static_cast<int>(i) == armedIndex) {
            oss << " [armed]";
        }
        if (i + 1 < devices_.size()) {
            oss << "\n";
        }
    }
    return oss.str();
}

void VideoGrabberLayer::writeDeviceInventoryArtifact() const {
    std::filesystem::path path = ofToDataPath("logs/webcam_devices.json", true);
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    ofJson root = ofJson::object();
    root["timestampMs"] = static_cast<uint64_t>(ofGetElapsedTimeMillis());
    root["selectedIndex"] = selectedDeviceIndex_;
    root["armedIndex"] = static_cast<int>(std::round(paramDeviceIndex_));
    ofJson devicesJson = ofJson::array();
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        const auto& info = devices_[i];
        devicesJson.push_back({
            {"index", static_cast<int>(i)},
            {"id", info.id},
            {"label", info.label},
            {"available", info.available},
            {"active", static_cast<int>(i) == selectedDeviceIndex_},
            {"armed", static_cast<int>(i) == root["armedIndex"].get<int>()},
            {"consecutiveFailures", info.consecutiveFailures},
            {"lastAttemptMs", info.lastAttemptMs},
            {"lastSuccessMs", info.lastSuccessMs},
            {"lastFrameMs", info.lastFrameMs}
        });
    }
    root["devices"] = std::move(devicesJson);
    std::ofstream out(path.string(), std::ios::trunc);
    if (!out) {
        ofLogWarning("VideoGrabberLayer") << "Failed to write webcam inventory log to " << path.string();
        return;
    }
    out << std::setw(2) << root << "\n";
    ofLogNotice("VideoGrabberLayer") << "Logged webcam devices to " << path.string();
}

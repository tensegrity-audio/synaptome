#pragma once

#include "Layer.h"
#ifdef TENSEGRITY_CUSTOM_VIDEO_GRABBER_HEADER
#include TENSEGRITY_CUSTOM_VIDEO_GRABBER_HEADER
#else
#include "ofVideoGrabber.h"
#endif
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <exception>
#include <limits>

class ofTexture;

class VideoGrabberLayer : public Layer {
public:
    class Grabber {
    public:
        virtual ~Grabber() = default;
        virtual std::vector<ofVideoDevice> listDevices() = 0;
        virtual bool isInitialized() const = 0;
        virtual void close() = 0;
        virtual void update() = 0;
        virtual bool isFrameNew() const = 0;
        virtual float getWidth() const = 0;
        virtual float getHeight() const = 0;
        virtual ofTexture* getTexture() = 0;
        virtual void draw(float x, float y, float w, float h) = 0;
        virtual void setDeviceID(int id) = 0;
        virtual void setDesiredFrameRate(int fps) = 0;
        virtual bool setup(int width, int height) = 0;
    };

    void configure(const ofJson& config) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
    void onWindowResized(int width, int height) override;

    bool isEnabled() const override { return enabled_; }
    void setExternalEnabled(bool enabled) override;

    void cycleDevice(int delta);
    void setDeviceIndex(int index);
    int deviceCount() const { return static_cast<int>(devices_.size()); }
    int selectedDeviceIndex() const { return selectedDeviceIndex_; }
    std::string deviceLabel(int index) const;
    std::string currentDeviceLabel() const;
    float gain() const { return paramGain_; }
    bool mirror() const { return paramMirror_; }

    void adjustGain(float delta);
    void toggleMirror();
    void forceDeviceRefresh();
    void setGrabberForTesting(const std::shared_ptr<Grabber>& grabber);

    float* gainParamPtr() { return &paramGain_; }
    float* deviceParamPtr() { return &paramDeviceIndex_; }
    bool* mirrorParamPtr() { return &paramMirror_; }
    bool* enabledParamPtr() { return &paramEnabled_; }

private:
    struct DeviceInfo {
        int id = -1;
        std::string label;
        bool available = false;
        bool lastSetupSuccess = false;
        uint32_t consecutiveFailures = 0;
        uint64_t lastAttemptMs = 0;
        uint64_t lastSuccessMs = 0;
        uint64_t lastFrameMs = 0;
    };

    struct ResolutionOption {
        int width = 0;
        int height = 0;
        std::string label;
    };

    void refreshDevices();
    void rebuildResolutionOptions(const ofJson& config);
    void applyResolutionSelection(int index);
    int clampResolutionIndex(int index) const;
    std::string resolutionOptionsSummary() const;

    bool tryApplyDeviceSelection(const std::string& context);
    void applyDeviceSelection();
    void scheduleDeferredOpen();
    int findFallbackDevice(int failedIndex) const;
    void logDeviceList() const;
    void updateDeviceParamMetadata();
    std::string deviceListSummary() const;
    void writeDeviceInventoryArtifact() const;
    void handleFrameTiming(bool frameNew, uint64_t now);
    void handleGrabberException(const std::exception& ex);
    Grabber& grabber();
    const Grabber& grabber() const;

    bool paramEnabled_ = true;
    float paramGain_ = 1.0f;
    float paramDeviceIndex_ = 0.0f;
    float paramResolutionIndex_ = 0.0f;
    bool paramMirror_ = false;
    bool paramShowDeviceOverlay_ = true;

    bool enabled_ = true;
    std::vector<DeviceInfo> devices_;
    std::vector<ResolutionOption> resolutionOptions_;
    int selectedDeviceIndex_ = -1;
    bool deviceDirty_ = true;
    uint64_t lastFrameLogMs_ = 0;
    bool waitingForFirstFrame_ = false;
    bool frameWarningActive_ = false;
    uint64_t nextDeviceRetryMs_ = 0;
    bool deferOpen_ = false;
    bool pendingDeferredOpen_ = false;
    uint64_t deferredOpenReadyMs_ = 0;
    uint64_t deferredOpenDelayMs_ = 120;
    int deferredOpenFramesRemaining_ = 0;
    int deferredOpenFrameDelay_ = 2;

    std::string configDeviceName_;
    int configDeviceIndex_ = -1;

    int captureWidth_ = 1280;
    int captureHeight_ = 720;
    int desiredFps_ = 30;
    int activeResolutionIndex_ = -1;
    int actualWidth_ = 0;
    int actualHeight_ = 0;

    std::shared_ptr<Grabber> grabber_;
    ParameterRegistry* registry_ = nullptr;
    std::string deviceParamId_;
    float lastDeviceParamValue_ = std::numeric_limits<float>::quiet_NaN();
};

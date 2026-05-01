#pragma once

#ifdef OF_SDK_AVAILABLE
#include <video/ofVideoGrabber.h>
#else

#include <string>
#include <vector>

#include "ofTexture.h"

struct ofVideoDevice {
    int id = -1;
    std::string deviceName;
    std::string hardwareName;
    bool bAvailable = true;
};

class ofVideoGrabber {
public:
    std::vector<ofVideoDevice> listDevices() { return {}; }
    bool isInitialized() const { return false; }
    void close() {}
    void update() {}
    bool isFrameNew() const { return false; }
    float getWidth() const { return 0.0f; }
    float getHeight() const { return 0.0f; }
    ofTexture& getTexture() { return texture_; }
    void draw(float, float, float, float) {}
    void setDeviceID(int) {}
    void setDesiredFrameRate(int) {}
    bool setup(int, int) { return false; }

private:
    ofTexture texture_;
};

#endif

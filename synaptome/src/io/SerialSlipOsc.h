#pragma once
#include "ofMain.h"
#include <vector>
#include <cstdint>
#include "../thirdparty/tinyosc.h"

class SerialSlipOsc {
public:
    void setAutoPortHints(std::vector<std::string> hints);
    void setBaudRate(int baud);
    void setReconnectInterval(uint64_t ms);
    void setLogTag(const std::string& tag);

    bool isConnected() const { return connected; }
    std::string currentPort() const { return activePort; }
    void requestReconnect();


    template <typename Callback>
    void update(Callback&& cb) {
        if (!ensureConnected()) {
            return;
        }
        bool readAny = false;
        while (true) {
            int b = serial.readByte();
            if (b == OF_SERIAL_ERROR) {
                handleDisconnect();
                return;
            }
            if (b == OF_SERIAL_NO_DATA) {
                break;
            }
            readAny = true;
            handleByte(static_cast<uint8_t>(b), cb);
        }
        uint64_t now = ofGetElapsedTimeMillis();
        if (readAny) {
            lastByteMs = now;
            reportedIdle = false;
        } else if (connected && lastByteMs > 0 && now > lastByteMs + 2000 && !reportedIdle) {
            ofLogNotice(logTag) << "Connected to " << activePort << " but no serial bytes for " << (now - lastByteMs) << " ms";
            reportedIdle = true;
        }
    }

private:
#ifdef TARGET_WIN32
    class SlipSerial : public ofSerial {
    public:
        HANDLE nativeHandle() const { return hComm; }
    };
    SlipSerial serial;
#else
    ofSerial serial;
#endif
    bool connected = false;
    std::vector<std::string> portHints { "ser=10:51:db:31:12:cc", "10:51:db:31:12:cc", "feather esp32-s3", "vid:pid=239a:811b", "239a:811b", "239a", "feather", "usbmodem", "usbserial" };
    int baudRate = 115200;
    uint64_t reconnectIntervalMs = 1500;
    uint64_t lastAttemptMs = 0;
    std::string activePort;
    std::string logTag = "SerialSlipOsc";

    std::vector<uint8_t> frame;
    bool inFrame = false;
    bool escape = false;

    uint64_t lastByteMs = 0;
    bool reportedIdle = false;
    bool awaitingDevice_ = false;

    template <typename Callback>
    void handleByte(uint8_t b, Callback&& cb) {
        constexpr uint8_t END = 0xC0;
        constexpr uint8_t ESC = 0xDB;
        constexpr uint8_t ESC_END = 0xDC;
        constexpr uint8_t ESC_ESC = 0xDD;

        if (b == END) {
            if (inFrame && !frame.empty()) {
                parseFrame(frame.data(), static_cast<int>(frame.size()), cb);
            }
            frame.clear();
            inFrame = true;
            escape = false;
            return;
        }
        if (!inFrame) {
            return;
        }
        if (escape) {
            if (b == ESC_END) frame.push_back(END);
            else if (b == ESC_ESC) frame.push_back(ESC);
            else frame.push_back(b);
            escape = false;
            return;
        }
        if (b == ESC) {
            escape = true;
        } else {
            frame.push_back(b);
        }
    }

    template <typename Callback>
    void parseFrame(const uint8_t* data, int len, Callback&& cb) {
        tosc_message msg;
        if (tosc_readMessage(&msg, data, len) != 0) {
            ofLogWarning(logTag) << "Failed to parse SLIP frame len=" << len;
            return;
        }
        if (!msg.address || !msg.format) {
            ofLogWarning(logTag) << "OSC frame missing address/format len=" << len;
            return;
        }
        int offset = 0;
        if (strcmp(msg.format, ",f") == 0) {
            float v = tosc_getNextFloat(&msg, offset);
            ofLogVerbose(logTag) << "parsed OSC: " << (msg.address ? msg.address : "(null)") << " -> " << v;
            cb(std::string(msg.address), v);
        } else if (strcmp(msg.format, ",i") == 0) {
            int32_t v = tosc_getNextInt32(&msg, offset);
            ofLogVerbose(logTag) << "parsed OSC: " << (msg.address ? msg.address : "(null)") << " -> " << v;
            cb(std::string(msg.address), static_cast<float>(v));
        }
    }

    bool ensureConnected();
    void handleDisconnect();
    bool openPort(const std::string& name);
#ifdef TARGET_WIN32
    void assertDtrRts();
#endif
};

#include "SerialSlipOsc.h"

#include <algorithm>

#include <cstdlib>

#ifdef TARGET_WIN32

#include <windows.h>

#include <setupapi.h>

#include <initguid.h>

#include <devguid.h>

#pragma comment(lib, "setupapi.lib")

#endif

namespace {

#ifdef TARGET_WIN32

// GUID for the serial enumerator bus (matches definition in ofSerial.cpp).

static const GUID kGuidSerEnumBus = {0x4D36E978, 0xE325, 0x11CE,

                                     {0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18}};

std::string readHardwareIdForFriendlyName(const std::string& friendlyName) {

    if (friendlyName.empty()) {

        return {};

    }

    HDEVINFO devInfo = SetupDiGetClassDevs(&kGuidSerEnumBus, nullptr, nullptr, DIGCF_PRESENT);

    if (devInfo == INVALID_HANDLE_VALUE) {

        return {};

    }

    std::string hardwareId;

    SP_DEVINFO_DATA deviceData{};

    deviceData.cbSize = sizeof(deviceData);

    for (DWORD idx = 0; SetupDiEnumDeviceInfo(devInfo, idx, &deviceData); ++idx) {

        char friendlyBuf[256];

        if (!SetupDiGetDeviceRegistryPropertyA(devInfo, &deviceData, SPDRP_FRIENDLYNAME, nullptr,

                                               reinterpret_cast<PBYTE>(friendlyBuf),

                                               sizeof(friendlyBuf), nullptr)) {

            continue;

        }

        if (friendlyName != friendlyBuf) {

            continue;

        }

        char hardwareBuf[1024];

        if (SetupDiGetDeviceRegistryPropertyA(devInfo, &deviceData, SPDRP_HARDWAREID, nullptr,

                                              reinterpret_cast<PBYTE>(hardwareBuf),

                                              sizeof(hardwareBuf), nullptr)) {

            hardwareId.assign(hardwareBuf);

        }

        break;

    }

    SetupDiDestroyDeviceInfoList(devInfo);

    return hardwareId;

}

#endif  // TARGET_WIN32

struct DeviceCandidate {

    std::string rawPath;

    std::string rawFriendly;

    std::vector<std::string> haystack;

    int priority = 0;

};

}  // namespace

void SerialSlipOsc::setAutoPortHints(std::vector<std::string> hints) {

    portHints = std::move(hints);

}

void SerialSlipOsc::setBaudRate(int baud) {

    baudRate = baud;

}

void SerialSlipOsc::setReconnectInterval(uint64_t ms) {

    reconnectIntervalMs = ms;

}

void SerialSlipOsc::setLogTag(const std::string& tag) {

    logTag = tag;

}

bool SerialSlipOsc::ensureConnected() {

    if (connected && serial.isInitialized()) {

        return true;

    }

    uint64_t now = ofGetElapsedTimeMillis();

    if (now - lastAttemptMs < reconnectIntervalMs) {

        return false;

    }

    lastAttemptMs = now;

    serial.flush();

    serial.close();

    activePort.clear();

    connected = false;

    auto devices = serial.getDeviceList();

    std::vector<DeviceCandidate> candidates;

    candidates.reserve(devices.size());

    for (auto& device : devices) {

        DeviceCandidate c;

        c.rawPath = device.getDevicePath();

        c.rawFriendly = device.getDeviceName();

        if (!c.rawPath.empty()) {

            c.haystack.push_back(ofToLower(c.rawPath));

        }

        if (!c.rawFriendly.empty()) {

            c.haystack.push_back(ofToLower(c.rawFriendly));

        }

#ifdef TARGET_WIN32

        std::string hardwareId = readHardwareIdForFriendlyName(c.rawFriendly);

        if (!hardwareId.empty()) {

            std::string lowerHardware = ofToLower(hardwareId);

            c.haystack.push_back(lowerHardware);

            const auto vidPos = lowerHardware.find("vid_");

            const auto pidPos = lowerHardware.find("pid_");

            if (vidPos != std::string::npos && pidPos != std::string::npos) {

                const std::string vid = lowerHardware.substr(vidPos + 4, 4);

                const std::string pid = lowerHardware.substr(pidPos + 4, 4);

                if (vid.size() == 4 && pid.size() == 4) {

                    c.haystack.push_back(vid + ":" + pid);

                    c.haystack.push_back("vid:pid=" + vid + ":" + pid);

                }

            }

        }

#endif

#ifdef TARGET_WIN32
        auto assignComPriority = [&](const std::string& source) {
            if (source.empty()) return;
            std::string lower = ofToLower(source);
            const char* base = source.c_str();
            for (std::size_t pos = 0; pos < lower.size(); ++pos) {
                if (lower.compare(pos, 3, "com") != 0) continue;
                const char* digits = base + pos + 3;
                char* endPtr = nullptr;
                long value = std::strtol(digits, &endPtr, 10);
                if (endPtr != digits && value > 0 && value < 1000) {
                    c.priority = std::max(c.priority, static_cast<int>(value));
                }
            }
        };
        assignComPriority(c.rawPath);
        assignComPriority(c.rawFriendly);
#endif

        candidates.push_back(std::move(c));

    }

    std::stable_sort(candidates.begin(), candidates.end(), [](const DeviceCandidate& a, const DeviceCandidate& b) {
        return a.priority > b.priority;
    });

    bool matchedCandidate = false;
    for (auto& hint : portHints) {
        if (hint.empty()) continue;

        std::string lowerHint = ofToLower(hint);

        for (const auto& candidate : candidates) {

            bool matched = false;

            for (const auto& hay : candidate.haystack) {

                if (!hay.empty() && hay.find(lowerHint) != std::string::npos) {

                    matched = true;

                    break;

                }

            }

            if (!matched) {

                continue;

            }

            matchedCandidate = true;

            if (openPort(candidate.rawPath)) {
                awaitingDevice_ = false;
                return true;
            }

            if (openPort(candidate.rawFriendly)) {
                awaitingDevice_ = false;
                return true;
            }

        }

    }

    if (!matchedCandidate) {
        if (!awaitingDevice_) {
            ofLogNotice(logTag) << "No serial devices matched gateway hints; collector suspended until reconnect";
            awaitingDevice_ = true;
        }
        return false;
    }

    awaitingDevice_ = false;
    return connected;

}

bool SerialSlipOsc::openPort(const std::string& name) {

    if (name.empty()) return false;

#ifdef TARGET_WIN32
    auto portPathFor = [](const std::string& portName) -> std::string {
        static const std::string kPrefix = R"(\\.\\)";
        if (portName.rfind(kPrefix, 0) == 0) {
            return portName;
        }
        return kPrefix + portName;
    };

    auto logPortBinding = [&](const std::string& portName) {
        std::string portPath = portPathFor(portName);
        ofLogNotice(logTag) << "Attempting to bind " << portName << " (" << portPath << ")";
    };

    logPortBinding(name);
#else
    ofSleepMillis(50);
#endif

    if (serial.setup(name, baudRate)) {
#ifdef TARGET_WIN32
        assertDtrRts();
#endif
        // Kick the gateway the same way a manual 'h' command would.
        serial.writeByte('h');
        ofLogNotice(logTag) << "Wrote handshake byte 'h' to " << name;

        activePort = name;

        ofLogNotice(logTag) << "Connected to " << name;

        frame.clear();

        connected = true;

        inFrame = false;

        escape = false;

        lastByteMs = ofGetElapsedTimeMillis();
        reportedIdle = false;

        return true;

    }

    return false;

}

void SerialSlipOsc::requestReconnect() {
    if (connected) {
        ofLogNotice(logTag) << "Manual reconnect requested for " << activePort;
    } else {
        ofLogNotice(logTag) << "Manual reconnect requested";
    }
    if (serial.isInitialized()) {
        serial.flush();
        serial.close();
    }
    activePort.clear();
    connected = false;
    inFrame = false;
    escape = false;
    frame.clear();
    lastByteMs = 0;
    reportedIdle = false;
    lastAttemptMs = 0;
    awaitingDevice_ = false;
}


void SerialSlipOsc::handleDisconnect() {

    if (!connected) return;

    ofLogWarning(logTag) << "Serial disconnected from " << activePort;

    serial.close();

    activePort.clear();

    connected = false;

    inFrame = false;

    escape = false;

    frame.clear();

    lastByteMs = 0;
    reportedIdle = false;
    lastAttemptMs = ofGetElapsedTimeMillis();
    awaitingDevice_ = false;

}

#ifdef TARGET_WIN32
void SerialSlipOsc::assertDtrRts() {
    if (!serial.isInitialized()) {
        return;
    }
    HANDLE handle = serial.nativeHandle();
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        ofLogWarning(logTag) << "Cannot assert DTR/RTS: invalid serial handle";
        return;
    }
    EscapeCommFunction(handle, CLRDTR);
    EscapeCommFunction(handle, CLRRTS);
    ofSleepMillis(25);
    if (!EscapeCommFunction(handle, SETDTR)) {
        ofLogWarning(logTag) << "Failed to assert DTR on " << activePort;
    }
    if (!EscapeCommFunction(handle, SETRTS)) {
        ofLogWarning(logTag) << "Failed to assert RTS on " << activePort;
    } else {
        ofLogVerbose(logTag) << "Asserted DTR/RTS for " << activePort;
    }
}
#endif

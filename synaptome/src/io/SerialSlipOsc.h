#pragma once
#include "ofMain.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <limits>
#include "OscIngressMessage.h"
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
    bool frameOverflow = false;
    static constexpr std::size_t kMaxFrameBytes = 64 * 1024;

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
            if (inFrame && !frame.empty() && !frameOverflow) {
                parseFrame(frame.data(), static_cast<int>(frame.size()), cb);
            } else if (frameOverflow) {
                ofLogWarning(logTag) << "Discarded oversized OSC SLIP frame";
            }
            frame.clear();
            inFrame = true;
            escape = false;
            frameOverflow = false;
            return;
        }
        if (!inFrame) {
            return;
        }
        if (frameOverflow) {
            return;
        }
        if (escape) {
            appendFrameByte(b == ESC_END ? END : (b == ESC_ESC ? ESC : b));
            escape = false;
            return;
        }
        if (b == ESC) {
            escape = true;
        } else {
            appendFrameByte(b);
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

        OscIngressMessage event;
        event.rawAddress = msg.address;
        event.canonicalAddress = event.rawAddress;
        event.typeTags = msg.format;
        event.transport = "serial-slip";
        event.endpoint = activePort;
        event.timestampMs = static_cast<std::uint64_t>(ofGetElapsedTimeMillis());
        event.rawSize = static_cast<std::size_t>(len);

        int offset = 0;
        const std::size_t tagCount = std::strlen(msg.format);
        if (tagCount == 0 || msg.format[0] != ',') {
            ofLogWarning(logTag) << "OSC frame has invalid type tag string";
            return;
        }
        for (std::size_t tagIndex = 1; tagIndex < tagCount; ++tagIndex) {
            const char tag = msg.format[tagIndex];
            OscIngressAtom atom;
            if (tag == 'i' || tag == 'f' || tag == 'c' || tag == 'm' || tag == 'r') {
                if (!hasBytes(msg, offset, 4)) {
                    ofLogWarning(logTag) << "Truncated OSC argument type=" << tag;
                    return;
                }
                const std::uint32_t raw = tosc_u32be(msg.data + offset);
                offset += 4;
                if (tag == 'i') {
                    atom = OscIngressAtom::integer(
                        OscIngressAtomType::Int32,
                        static_cast<std::int32_t>(raw));
                } else if (tag == 'f') {
                    float value = 0.0f;
                    std::memcpy(&value, &raw, sizeof(value));
                    atom = OscIngressAtom::numeric(OscIngressAtomType::Float32, value);
                } else if (tag == 'c') {
                    atom.type = OscIngressAtomType::Char;
                    atom.textValue.assign(1, static_cast<char>(raw & 0xff));
                } else {
                    atom.type = tag == 'm'
                        ? OscIngressAtomType::Midi
                        : OscIngressAtomType::Rgba;
                    atom.unsignedValue = raw;
                }
            } else if (tag == 'h' || tag == 'd' || tag == 't') {
                if (!hasBytes(msg, offset, 8)) {
                    ofLogWarning(logTag) << "Truncated OSC argument type=" << tag;
                    return;
                }
                const std::uint64_t raw = readU64Be(msg.data + offset);
                offset += 8;
                if (tag == 'h') {
                    atom = OscIngressAtom::integer(
                        OscIngressAtomType::Int64,
                        static_cast<std::int64_t>(raw));
                } else if (tag == 'd') {
                    double value = 0.0;
                    std::memcpy(&value, &raw, sizeof(value));
                    atom = OscIngressAtom::numeric(OscIngressAtomType::Float64, value);
                } else {
                    atom.type = OscIngressAtomType::Timetag;
                    atom.unsignedValue = raw;
                }
            } else if (tag == 's' || tag == 'S') {
                std::string value;
                if (!readPaddedString(msg, offset, value)) {
                    ofLogWarning(logTag) << "Truncated OSC string argument";
                    return;
                }
                atom = OscIngressAtom::text(
                    tag == 's' ? OscIngressAtomType::String : OscIngressAtomType::Symbol,
                    std::move(value));
            } else if (tag == 'b') {
                if (!hasBytes(msg, offset, 4)) {
                    ofLogWarning(logTag) << "Truncated OSC blob length";
                    return;
                }
                const std::uint32_t byteCount = tosc_u32be(msg.data + offset);
                offset += 4;
                if (byteCount > kMaxFrameBytes
                    || !hasBytes(msg, offset, static_cast<int>(byteCount))) {
                    ofLogWarning(logTag) << "Invalid OSC blob size=" << byteCount;
                    return;
                }
                atom.type = OscIngressAtomType::Blob;
                atom.byteCount = byteCount;
                offset += tosc_align4(static_cast<int>(byteCount));
                if (offset > msg.len) {
                    ofLogWarning(logTag) << "Truncated padded OSC blob";
                    return;
                }
            } else if (tag == 'T' || tag == 'F') {
                atom.type = OscIngressAtomType::Bool;
                atom.numericValue = tag == 'T' ? 1.0 : 0.0;
            } else if (tag == 'N') {
                atom.type = OscIngressAtomType::Nil;
            } else if (tag == 'I') {
                atom.type = OscIngressAtomType::Impulse;
            } else {
                atom.type = OscIngressAtomType::Unsupported;
            }
            event.arguments.push_back(std::move(atom));
        }

        ofLogVerbose(logTag) << "parsed OSC: " << event.rawAddress
                             << " " << event.typeTags
                             << " " << event.payloadSummary();
        cb(event);
    }

    void appendFrameByte(uint8_t value) {
        if (frame.size() >= kMaxFrameBytes) {
            frame.clear();
            frameOverflow = true;
            return;
        }
        frame.push_back(value);
    }

    static bool hasBytes(const tosc_message& msg, int offset, int byteCount) {
        return offset >= 0
            && byteCount >= 0
            && offset <= msg.len
            && byteCount <= msg.len - offset;
    }

    static std::uint64_t readU64Be(const std::uint8_t* data) {
        return (static_cast<std::uint64_t>(tosc_u32be(data)) << 32)
            | static_cast<std::uint64_t>(tosc_u32be(data + 4));
    }

    static bool readPaddedString(const tosc_message& msg,
                                 int& offset,
                                 std::string& value) {
        if (!hasBytes(msg, offset, 1)) {
            return false;
        }
        int length = 0;
        while (offset + length < msg.len && msg.data[offset + length] != '\0') {
            ++length;
        }
        if (offset + length >= msg.len) {
            return false;
        }
        value.assign(
            reinterpret_cast<const char*>(msg.data + offset),
            static_cast<std::size_t>(length));
        offset += tosc_align4(length + 1);
        return offset <= msg.len;
    }

    bool ensureConnected();
    void handleDisconnect();
    bool openPort(const std::string& name);
#ifdef TARGET_WIN32
    void assertDtrRts();
#endif
};

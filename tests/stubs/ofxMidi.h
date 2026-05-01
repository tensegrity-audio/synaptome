#pragma once

#include <string>
#include <vector>

constexpr int MIDI_CONTROL_CHANGE = 0xB0;
constexpr int MIDI_NOTE_ON = 0x90;
constexpr int MIDI_NOTE_OFF = 0x80;

class ofxMidiMessage {
public:
    int status = 0;
    int channel = 0;
    int control = 0;
    int value = 0;
    int pitch = 0;
    int velocity = 0;
};

class ofxMidiListener {
public:
    virtual ~ofxMidiListener() = default;
    virtual void newMidiMessage(ofxMidiMessage&) {}
};

class ofxMidiIn {
public:
    bool openPort(int) { return false; }
    bool openPort(const std::string&) { return false; }
    bool openPort(unsigned int) { return false; }
    void closePort() {}
    int getPort() const { return -1; }
    std::string getInPortName(unsigned int) const { return {}; }
    std::vector<std::string> getInPortList() const { return {}; }
    void listInPorts() const {}
    void addListener(ofxMidiListener*) {}
    void removeListener(ofxMidiListener*) {}
    void ignoreTypes(bool, bool, bool) {}
};

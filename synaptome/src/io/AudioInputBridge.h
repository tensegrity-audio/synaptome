#pragma once
#include "ofMain.h"
#include "ofxOsc.h"
#include "../core/ParameterRegistry.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>
#include <string>

class AudioInputBridge : public ofBaseSoundInput {
public:
    AudioInputBridge();
    ~AudioInputBridge();

    // Enumerate input device indices and names (pair: {deviceIndex, name})
    std::vector<std::pair<int, std::string>> listInputDevices();

    // Choose device and start/stop
    bool setupDevice(int deviceIndex, int sampleRate = 44100, int bufferSize = 512, int channels = 1);
    void stop();

    // ofBaseSoundInput callback (audio thread)
    void audioIn(ofSoundBuffer& buffer) override;

    // Called from main thread periodically to push levels into ParameterRegistry
    void update(ParameterRegistry& registry, const std::string& paramId, std::size_t modifierIndex);

    // OSC publishing control (optional): publish local mic levels to host:port
    void startOscPublisher(const std::string& host = "127.0.0.1", int port = 12345);
    void stopOscPublisher();

    float lastRms() const { return lastRms_.load(); }
    float lastPeak() const { return lastPeak_.load(); }
    float lastBass() const { return lastBass_.load(); }
    float lastMids() const { return lastMids_.load(); }
    float lastHighs() const { return lastHighs_.load(); }

private:
    ofSoundStream soundStream_;
    std::atomic<float> lastRms_{0.0f};
    std::atomic<float> lastPeak_{0.0f};
    std::atomic<float> lastBass_{0.0f};
    std::atomic<float> lastMids_{0.0f};
    std::atomic<float> lastHighs_{0.0f};
    std::atomic<uint64_t> analysisFrame_{0};
    std::mutex deviceMutex_;
    std::mutex analysisMutex_;
    std::vector<float> analysisWindow_;
    std::string deviceLabel_;
    int sampleRate_ = 44100;
    int channels_ = 1;

    // Optional OSC sender
    ofxOscSender oscSender_;
    bool publishOsc_ = false;
    std::string oscHost_ = "127.0.0.1";
    int oscPort_ = 0;
};

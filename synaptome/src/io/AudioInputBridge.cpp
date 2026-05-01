#include "AudioInputBridge.h"
#include "ofLog.h"
#include "ofxOsc.h"

AudioInputBridge::AudioInputBridge() {}
AudioInputBridge::~AudioInputBridge() { stop(); }

std::vector<std::pair<int, std::string>> AudioInputBridge::listInputDevices() {
    std::vector<std::pair<int, std::string>> out;
    auto devices = soundStream_.getDeviceList();
    ofLogNotice("AudioInputBridge") << "listInputDevices: total devices=" << devices.size();
    for (size_t i = 0; i < devices.size(); ++i) {
        const auto& d = devices[i];
        ofLogNotice("AudioInputBridge") << "  [" << i << "] name='" << d.name << "' inputChannels=" << d.inputChannels;
        if (d.inputChannels > 0) out.emplace_back(static_cast<int>(i), d.name);
    }
    return out;
}

bool AudioInputBridge::setupDevice(int deviceIndex, int sampleRate, int bufferSize, int channels) {
    // stop() acquires deviceMutex_ internally; call it first to avoid double-lock
    stop();
    std::lock_guard<std::mutex> lock(deviceMutex_);

    ofSoundStreamSettings settings;
    auto devices = soundStream_.getDeviceList();
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        ofLogWarning("AudioInputBridge") << "invalid device index: " << deviceIndex << " (available=" << devices.size() << ")";
        return false;
    }
    auto dev = devices[deviceIndex];
    ofLogNotice("AudioInputBridge") << "setupDevice: selecting device index=" << deviceIndex << " name='" << dev.name << "' inputChannels=" << dev.inputChannels;
    settings.setInDevice(dev);
    settings.setInListener(this);
    settings.sampleRate = sampleRate;
    settings.numInputChannels = std::min(static_cast<int>(dev.inputChannels), channels);
    settings.numOutputChannels = 0;
    settings.bufferSize = bufferSize;

    try {
        soundStream_.setup(settings);
        ofLogNotice("AudioInputBridge") << "soundStream_.setup succeeded (inChannels=" << settings.numInputChannels << ", sampleRate=" << settings.sampleRate << ")";
    } catch (const std::exception& e) {
        ofLogError("AudioInputBridge") << "setupDevice failed: " << e.what();
        return false;
    }
    channels_ = static_cast<int>(settings.numInputChannels);
    return true;
}

void AudioInputBridge::stop() {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    // ofSoundStream may not expose an isSetup() method across OF versions;
    // just attempt stop/close safely and ignore any exceptions.
    try {
        soundStream_.stop();
        soundStream_.close();
    } catch (...) {
        // ignore
    }
}

void AudioInputBridge::audioIn(ofSoundBuffer& buffer) {
    float sumSq = 0.0f;
    float peak = 0.0f;
    size_t n = buffer.getNumFrames();
    for (size_t i = 0; i < n; ++i) {
        float sample = 0.0f;
        if (channels_ == 1) {
            sample = buffer.getSample(i, 0);
        } else {
            float s = 0.0f;
            for (int c = 0; c < channels_; ++c) {
                s += buffer.getSample(i, c);
            }
            sample = s / static_cast<float>(channels_);
        }
        sumSq += sample * sample;
        peak = std::max(peak, std::fabs(sample));
    }
    float rms = n > 0 ? std::sqrt(sumSq / static_cast<float>(n)) : 0.0f;
    lastRms_.store(rms);
    lastPeak_.store(peak);
    // Helpful audio-thread logging:
    //  - Log the first few audio callbacks immediately so we can verify the
    //    callback is running at all in the runtime capture.
    //  - Fall back to a throttled once-per-second notice after that.
    static std::atomic<int> audioInCount{0};
    int count = audioInCount.fetch_add(1);
    if (count < 10) {
        ofLogNotice("AudioInputBridge") << "audioIn called (initial): frames=" << n << " channels=" << channels_ << " rms=" << rms << " peak=" << peak << " (call#=" << (count + 1) << ")";
    } else {
        uint64_t now = ofGetElapsedTimeMillis();
        static std::atomic<uint64_t> lastLogMs{0};
        uint64_t prev = lastLogMs.load();
        if (now - prev > 1000) {
            if (lastLogMs.compare_exchange_strong(prev, now)) {
                ofLogNotice("AudioInputBridge") << "audioIn called: frames=" << n << " channels=" << channels_ << " rms=" << rms << " peak=" << peak;
            }
        }
    }
}

void AudioInputBridge::update(ParameterRegistry& registry, const std::string& paramId, std::size_t modifierIndex) {
    float rms = lastRms_.load();
    if (modifierIndex == static_cast<std::size_t>(-1)) return;
    try {
        registry.setFloatModifierInput(paramId, modifierIndex, rms, true);
        // caller is responsible for calling evaluateAllModifiers once per frame after updates
    } catch (const std::exception& e) {
        // defensive: ignore if param missing or index invalid
    }

    // Optionally publish via OSC to localhost (or configured host)
    if (publishOsc_) {
        try {
            float peak = lastPeak_.load();
            ofxOscMessage msg;
            msg.setAddress("/sensor/host/localmic/mic-level");
            msg.addFloatArg(rms);
            oscSender_.sendMessage(msg, false);

            ofxOscMessage msg2;
            msg2.setAddress("/sensor/host/localmic/mic-peak");
            msg2.addFloatArg(peak);
            oscSender_.sendMessage(msg2, false);
        } catch (...) {
            // keep publishing best-effort; don't throw to main app
        }
    }
}

void AudioInputBridge::startOscPublisher(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    try {
        oscSender_.setup(host, port);
        publishOsc_ = true;
        oscHost_ = host;
        oscPort_ = port;
    } catch (const std::exception& e) {
        ofLogWarning("AudioInputBridge") << "startOscPublisher failed: " << e.what();
        publishOsc_ = false;
    }
}

void AudioInputBridge::stopOscPublisher() {
    std::lock_guard<std::mutex> lock(deviceMutex_);
    publishOsc_ = false;
}

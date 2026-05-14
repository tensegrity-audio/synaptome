#include "AudioInputBridge.h"
#include "AudioAnalysisBus.h"
#include "ofLog.h"
#include "ofxOsc.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>

namespace {
    constexpr std::size_t kAnalysisWindowSize = 2048;
    constexpr std::size_t kPublishedWaveformSamples = 512;

    float goertzelMagnitude(const std::vector<float>& samples, int sampleRate, float frequency) {
        if (samples.empty() || sampleRate <= 0 || frequency <= 0.0f) {
            return 0.0f;
        }
        const float nyquist = static_cast<float>(sampleRate) * 0.5f;
        if (frequency >= nyquist) {
            return 0.0f;
        }

        const float n = static_cast<float>(samples.size());
        const float omega = TWO_PI * frequency / static_cast<float>(sampleRate);
        const float coeff = 2.0f * std::cos(omega);
        float q0 = 0.0f;
        float q1 = 0.0f;
        float q2 = 0.0f;
        for (std::size_t i = 0; i < samples.size(); ++i) {
            float window = samples.size() > 1
                ? 0.5f - 0.5f * std::cos(TWO_PI * static_cast<float>(i) / (n - 1.0f))
                : 1.0f;
            q0 = samples[i] * window + coeff * q1 - q2;
            q2 = q1;
            q1 = q0;
        }
        float power = q1 * q1 + q2 * q2 - coeff * q1 * q2;
        return std::sqrt(std::max(0.0f, power)) / std::max(1.0f, n * 0.5f);
    }

    float averageBandMagnitude(const std::vector<float>& samples,
                               int sampleRate,
                               const std::array<float, 3>& centers,
                               float gain) {
        float total = 0.0f;
        int count = 0;
        for (float center : centers) {
            const float magnitude = goertzelMagnitude(samples, sampleRate, center);
            if (magnitude > 0.0f) {
                total += magnitude;
                ++count;
            }
        }
        if (count == 0) {
            return 0.0f;
        }
        return ofClamp((total / static_cast<float>(count)) * gain, 0.0f, 1.0f);
    }

    std::vector<float> downsampleWaveform(const std::vector<float>& samples, std::size_t maxSamples) {
        if (samples.size() <= maxSamples) {
            return samples;
        }
        std::vector<float> out;
        out.reserve(maxSamples);
        const float last = static_cast<float>(samples.size() - 1);
        const float denom = static_cast<float>(std::max<std::size_t>(1, maxSamples - 1));
        for (std::size_t i = 0; i < maxSamples; ++i) {
            std::size_t index = static_cast<std::size_t>(std::round((static_cast<float>(i) / denom) * last));
            index = std::min(index, samples.size() - 1);
            out.push_back(samples[index]);
        }
        return out;
    }
}

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
    sampleRate_ = settings.sampleRate;
    deviceLabel_ = dev.name;
    analysisFrame_.store(0);
    {
        std::lock_guard<std::mutex> analysisLock(analysisMutex_);
        analysisWindow_.clear();
        analysisWindow_.reserve(kAnalysisWindowSize);
    }
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
    std::vector<float> monoSamples;
    monoSamples.reserve(n);
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
        monoSamples.push_back(sample);
    }
    float rms = n > 0 ? std::sqrt(sumSq / static_cast<float>(n)) : 0.0f;
    lastRms_.store(rms);
    lastPeak_.store(peak);

    std::vector<float> analysisSamples;
    {
        std::lock_guard<std::mutex> analysisLock(analysisMutex_);
        analysisWindow_.insert(analysisWindow_.end(), monoSamples.begin(), monoSamples.end());
        if (analysisWindow_.size() > kAnalysisWindowSize) {
            analysisWindow_.erase(analysisWindow_.begin(),
                                  analysisWindow_.begin() + static_cast<std::ptrdiff_t>(analysisWindow_.size() - kAnalysisWindowSize));
        }
        analysisSamples = analysisWindow_;
    }

    const float bass = averageBandMagnitude(analysisSamples, sampleRate_, { 60.0f, 120.0f, 200.0f }, 9.0f);
    const float mids = averageBandMagnitude(analysisSamples, sampleRate_, { 400.0f, 1000.0f, 2500.0f }, 8.0f);
    const float highs = averageBandMagnitude(analysisSamples, sampleRate_, { 5000.0f, 8000.0f, 11000.0f }, 10.0f);
    lastBass_.store(bass);
    lastMids_.store(mids);
    lastHighs_.store(highs);

    AudioAnalysisBus::Snapshot snapshot;
    snapshot.valid = true;
    snapshot.frame = analysisFrame_.fetch_add(1) + 1;
    snapshot.sampleRate = sampleRate_;
    snapshot.channels = channels_;
    snapshot.level = rms;
    snapshot.peak = peak;
    snapshot.bass = bass;
    snapshot.mids = mids;
    snapshot.highs = highs;
    snapshot.sourceLabel = deviceLabel_;
    snapshot.waveform = downsampleWaveform(analysisSamples, kPublishedWaveformSamples);
    AudioAnalysisBus::instance().publish(snapshot);

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
                ofLogNotice("AudioInputBridge") << "audioIn called: frames=" << n << " channels=" << channels_ << " rms=" << rms << " peak=" << peak << " bass=" << bass << " mids=" << mids << " highs=" << highs;
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

            ofxOscMessage bassMsg;
            bassMsg.setAddress("/sensor/host/localmic/mic-bass");
            bassMsg.addFloatArg(lastBass_.load());
            oscSender_.sendMessage(bassMsg, false);

            ofxOscMessage midsMsg;
            midsMsg.setAddress("/sensor/host/localmic/mic-mids");
            midsMsg.addFloatArg(lastMids_.load());
            oscSender_.sendMessage(midsMsg, false);

            ofxOscMessage highsMsg;
            highsMsg.setAddress("/sensor/host/localmic/mic-highs");
            highsMsg.addFloatArg(lastHighs_.load());
            oscSender_.sendMessage(highsMsg, false);
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

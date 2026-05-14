#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class AudioAnalysisBus {
public:
    struct Snapshot {
        bool valid = false;
        uint64_t frame = 0;
        int sampleRate = 0;
        int channels = 0;
        float level = 0.0f;
        float peak = 0.0f;
        float bass = 0.0f;
        float mids = 0.0f;
        float highs = 0.0f;
        std::string sourceLabel;
        std::vector<float> waveform;
    };

    static AudioAnalysisBus& instance() {
        static AudioAnalysisBus bus;
        return bus;
    }

    void publish(const Snapshot& snapshot) {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot_ = snapshot;
    }

    Snapshot snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return snapshot_;
    }

private:
    mutable std::mutex mutex_;
    Snapshot snapshot_;
};

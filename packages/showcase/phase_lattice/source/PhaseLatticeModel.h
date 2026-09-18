#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace synaptome_show_phase_lattice {

// Locally coupled Kuramoto-Sakaguchi oscillators on a periodic square lattice.
// Natural frequencies are persistent seeded properties of each oscillator.
class PhaseLatticeModel {
public:
    static constexpr int width = 64;
    static constexpr int height = 44;
    static constexpr int cellCount = width * height;
    struct Controls {
        float coupling = 2.4f;
        float frequency = 0.28f;
        float dispersion = 0.5f;
        float phaseLagDeg = 30.0f;
    };
    void reset(std::uint32_t seed) {
        randomState_ = seed ? seed : 1u;
        accumulator_ = 0.0;
        const float shift = random() * 6.2831853f;
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            const int at = y * width + x;
            const float u = static_cast<float>(x) / width;
            const float v = static_cast<float>(y) / height;
            // A coherent spiral/stripe mixture reads clearly on the first frame.
            phases_[at] = wrap(std::atan2(v - 0.5f, u - 0.5f) +
                12.0f * std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f)) +
                shift + (random() - 0.5f) * 0.65f);
            frequencyOffsets_[at] = random() * 2.0f - 1.0f;
        }
    }
    void step(float dt, Controls c) {
        if (!std::isfinite(dt) || dt <= 0.0f) return;
        c.coupling = bound(c.coupling, 0.0f, 12.0f, 2.4f);
        c.frequency = bound(c.frequency, 0.02f, 2.0f, 0.28f);
        c.dispersion = bound(c.dispersion, 0.0f, 2.0f, 0.5f);
        c.phaseLagDeg = bound(c.phaseLagDeg, -90.0f, 90.0f, 30.0f);
        constexpr double tick = 1.0 / 120.0;
        accumulator_ += std::min(static_cast<double>(dt), tick * 16.0);
        int steps = 0;
        while (accumulator_ + 1e-9 >= tick && steps++ < 16) {
            advance(static_cast<float>(tick), c);
            accumulator_ -= tick;
        }
        if (accumulator_ < 0.0) accumulator_ = 0.0;
    }
    const std::array<float, cellCount>& phases() const { return phases_; }
    bool boundedState() const {
        for (float value : phases_)
            if (!std::isfinite(value) || value < 0.0f || value >= 6.283186f) return false;
        return true;
    }
    float orderParameter() const {
        float re = 0.0f, im = 0.0f;
        for (float phase : phases_) { re += std::cos(phase); im += std::sin(phase); }
        return std::sqrt(re * re + im * im) / cellCount;
    }
    std::uint64_t stateSignature() const {
        std::uint64_t result = 1469598103934665603ull;
        for (float value : phases_) {
            result ^= static_cast<std::uint32_t>(value * 1000000.0f);
            result *= 1099511628211ull;
        }
        return result;
    }
private:
    std::array<float, cellCount> phases_{};
    std::array<float, cellCount> nextPhases_{};
    std::array<float, cellCount> frequencyOffsets_{};
    std::uint32_t randomState_ = 1u;
    double accumulator_ = 0.0;
    static float bound(float value, float lo, float hi, float fallback) {
        return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
    }
    static float wrap(float phase) {
        constexpr float tau = 6.28318530718f;
        phase = std::fmod(phase, tau);
        return phase < 0.0f ? phase + tau : phase;
    }
    float random() {
        randomState_ ^= randomState_ << 13;
        randomState_ ^= randomState_ >> 17;
        randomState_ ^= randomState_ << 5;
        return static_cast<float>(randomState_ >> 8) * (1.0f / 16777216.0f);
    }
    void advance(float dt, const Controls& c) {
        const float lag = c.phaseLagDeg * (3.14159265f / 180.0f);
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            const int at = y * width + x;
            const float phase = phases_[at];
            const float neighbors[] = {
                phases_[y * width + (x + width - 1) % width],
                phases_[y * width + (x + 1) % width],
                phases_[((y + height - 1) % height) * width + x],
                phases_[((y + 1) % height) * width + x]};
            float coupling = 0.0f;
            for (float neighbor : neighbors) coupling += std::sin(neighbor - phase - lag);
            const float natural = 6.2831853f * c.frequency + c.dispersion * frequencyOffsets_[at];
            nextPhases_[at] = wrap(phase + dt * (natural + c.coupling * coupling * 0.25f));
        }
        phases_.swap(nextPhases_);
    }
};
}

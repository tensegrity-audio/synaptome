#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace synaptome_show_wave_tank {

// Damped finite-difference wave equation on a fixed, bounded membrane.
// Coordinates are grid cells; time is seconds. No graphics or host services.
class WaveTankModel {
public:
    static constexpr int width = 72;
    static constexpr int height = 48;
    static constexpr int cellCount = width * height;
    struct Controls {
        float waveSpeed = 16.0f;
        float damping = 0.55f;
        float driveStrength = 5.0f;
        float driveFrequency = 0.8f;
        float sourceSpread = 0.6f;
    };

    void reset(std::uint32_t seed) {
        randomState_ = seed ? seed : 1u;
        time_ = 0.0;
        accumulator_ = 0.0;
        velocities_.fill(0.0f);
        for (auto& source : sources_) {
            source = {0.15f + random() * 0.7f,
                      0.15f + random() * 0.7f, random() * 6.2831853f};
        }
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const float u = static_cast<float>(x) / (width - 1);
                const float v = static_cast<float>(y) / (height - 1);
                float value = 0.0f;
                for (const auto& source : sources_) {
                    const float dx = u - source.x;
                    const float dy = v - source.y;
                    const float r = std::sqrt(dx * dx + dy * dy);
                    value += 0.16f * std::cos(r * 42.0f + source.phase) *
                        std::exp(-r * r * 7.0f);
                }
                values_[index(x, y)] = value *
                    std::sin(u * 3.14159265f) * std::sin(v * 3.14159265f);
            }
        }
    }

    void step(float dt, Controls controls) {
        if (!std::isfinite(dt) || dt <= 0.0f) return;
        controls.waveSpeed = bounded(controls.waveSpeed, 2.0f, 28.0f, 16.0f);
        controls.damping = bounded(controls.damping, 0.05f, 3.0f, 0.55f);
        controls.driveStrength = bounded(controls.driveStrength, 0.0f, 12.0f, 5.0f);
        controls.driveFrequency = bounded(controls.driveFrequency, 0.1f, 3.0f, 0.8f);
        controls.sourceSpread = bounded(controls.sourceSpread, 0.0f, 1.0f, 0.6f);
        constexpr double tick = 1.0 / 120.0;
        accumulator_ += std::min(static_cast<double>(dt), tick * 16.0);
        int steps = 0;
        while (accumulator_ + 1e-9 >= tick && steps++ < 16) {
            advance(static_cast<float>(tick), controls);
            accumulator_ -= tick;
        }
        if (accumulator_ < 0.0) accumulator_ = 0.0;
    }

    const std::array<float, cellCount>& values() const { return values_; }
    const std::array<float, cellCount>& velocities() const { return velocities_; }
    bool boundedState() const {
        for (int i = 0; i < cellCount; ++i)
            if (!std::isfinite(values_[i]) || std::abs(values_[i]) > 2.0f ||
                !std::isfinite(velocities_[i]) || std::abs(velocities_[i]) > 12.0f)
                return false;
        return true;
    }
    std::uint64_t stateSignature() const {
        std::uint64_t result = 1469598103934665603ull;
        for (int i = 0; i < cellCount; ++i) {
            result ^= static_cast<std::uint32_t>(static_cast<std::int32_t>(values_[i] * 1000000.0f));
            result *= 1099511628211ull;
            result ^= static_cast<std::uint32_t>(static_cast<std::int32_t>(velocities_[i] * 1000000.0f));
            result *= 1099511628211ull;
        }
        return result;
    }

private:
    struct Source { float x, y, phase; };
    std::array<float, cellCount> values_{};
    std::array<float, cellCount> velocities_{};
    std::array<float, cellCount> nextVelocities_{};
    std::array<Source, 4> sources_{};
    std::uint32_t randomState_ = 1u;
    double time_ = 0.0;
    double accumulator_ = 0.0;
    static int index(int x, int y) { return y * width + x; }
    static float bounded(float value, float lo, float hi, float fallback) {
        return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
    }
    float random() {
        randomState_ ^= randomState_ << 13;
        randomState_ ^= randomState_ >> 17;
        randomState_ ^= randomState_ << 5;
        return static_cast<float>(randomState_ >> 8) * (1.0f / 16777216.0f);
    }
    void advance(float dt, const Controls& c) {
        time_ += dt;
        const float waveSquared = c.waveSpeed * c.waveSpeed;
        nextVelocities_.fill(0.0f);
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                const int at = index(x, y);
                const float laplacian = values_[at - 1] + values_[at + 1] +
                    values_[at - width] + values_[at + width] - 4.0f * values_[at];
                nextVelocities_[at] = std::clamp(
                    (velocities_[at] + waveSquared * laplacian * dt) *
                        std::exp(-c.damping * dt), -12.0f, 12.0f);
            }
        }
        for (const auto& source : sources_) {
            const float u = 0.5f + (source.x - 0.5f) * c.sourceSpread;
            const float v = 0.5f + (source.y - 0.5f) * c.sourceSpread;
            const int cx = static_cast<int>(u * (width - 1));
            const int cy = static_cast<int>(v * (height - 1));
            const float force = std::sin(static_cast<float>(
                std::fmod(time_ * c.driveFrequency * 6.2831853, 6.2831853)) + source.phase);
            for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) {
                const int x = cx + dx, y = cy + dy;
                if (x <= 0 || y <= 0 || x >= width - 1 || y >= height - 1) continue;
                const int at = index(x, y);
                nextVelocities_[at] = std::clamp(nextVelocities_[at] +
                    force * c.driveStrength * std::exp(-0.5f * static_cast<float>(dx * dx + dy * dy)) * dt,
                    -12.0f, 12.0f);
            }
        }
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            const int at = index(x, y);
            velocities_[at] = nextVelocities_[at];
            values_[at] = (x == 0 || y == 0 || x == width - 1 || y == height - 1)
                ? 0.0f : std::clamp(values_[at] + velocities_[at] * dt, -2.0f, 2.0f);
        }
    }
};
}

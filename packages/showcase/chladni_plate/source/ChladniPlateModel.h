#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace synaptome_show_chladni_plate {

// Sand tracers descend the squared displacement of a rectangular standing
// wave. Changing mode orders preserves every particle's position and velocity.
class ChladniPlateModel {
public:
    static constexpr int particleCount = 1800;
    struct Particle { float x, y, vx, vy; };
    struct Controls {
        float modeX = 3.0f;
        float modeY = 5.0f;
        float attraction = 2.5f;
        float agitation = 0.16f;
        float phaseRate = 0.12f;
    };
    void reset(std::uint32_t seed) {
        randomState_ = seed ? seed : 1u;
        phase_ = 0.0f;
        accumulator_ = 0.0;
        for (auto& p : particles_) p = {random(), random(), 0.0f, 0.0f};
        // Seed a recognizable nodal pattern immediately without advancing the
        // transport or depending on graphics setup or live input.
        Controls initial;
        initial.agitation = 0.0f;
        initial.phaseRate = 0.0f;
        for (int i = 0; i < 80; ++i) advance(1.0f / 120.0f, initial);
    }
    void step(float dt, Controls c) {
        if (!std::isfinite(dt) || dt <= 0.0f) return;
        c.modeX = bound(c.modeX, 1.0f, 9.0f, 3.0f);
        c.modeY = bound(c.modeY, 1.0f, 9.0f, 5.0f);
        c.attraction = bound(c.attraction, 0.0f, 6.0f, 2.5f);
        c.agitation = bound(c.agitation, 0.0f, 1.0f, 0.16f);
        c.phaseRate = bound(c.phaseRate, 0.0f, 0.8f, 0.12f);
        constexpr double tick = 1.0 / 120.0;
        accumulator_ += std::min(static_cast<double>(dt), tick * 16.0);
        int steps = 0;
        while (accumulator_ + 1e-9 >= tick && steps++ < 16) {
            advance(static_cast<float>(tick), c);
            accumulator_ -= tick;
        }
        if (accumulator_ < 0.0) accumulator_ = 0.0;
    }
    const std::array<Particle, particleCount>& particles() const { return particles_; }
    float phase() const { return phase_; }
    bool boundedState() const {
        for (const auto& p : particles_)
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.vx) ||
                !std::isfinite(p.vy) || p.x < 0.0f || p.x > 1.0f || p.y < 0.0f || p.y > 1.0f ||
                std::abs(p.vx) > 1.25f || std::abs(p.vy) > 1.25f) return false;
        return true;
    }
    std::uint64_t stateSignature() const {
        std::uint64_t result = 1469598103934665603ull;
        for (const auto& p : particles_) for (float v : {p.x, p.y, p.vx, p.vy}) {
            result ^= static_cast<std::uint32_t>(static_cast<std::int32_t>(v * 1000000.0f));
            result *= 1099511628211ull;
        }
        return result;
    }
private:
    std::array<Particle, particleCount> particles_{};
    std::uint32_t randomState_ = 1u;
    float phase_ = 0.0f;
    double accumulator_ = 0.0;
    static float bound(float value, float lo, float hi, float fallback) {
        return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
    }
    float random() {
        randomState_ ^= randomState_ << 13;
        randomState_ ^= randomState_ >> 17;
        randomState_ ^= randomState_ << 5;
        return static_cast<float>(randomState_ >> 8) * (1.0f / 16777216.0f);
    }
    void advance(float dt, const Controls& c) {
        constexpr float pi = 3.14159265359f;
        phase_ = std::fmod(phase_ + c.phaseRate * 2.0f * pi * dt, 2.0f * pi);
        const float m = c.modeX * pi, n = c.modeY * pi;
        for (auto& p : particles_) {
            const float sx = std::sin(m * p.x), sy = std::sin(n * p.y);
            const float tx = std::sin(n * p.x + phase_), ty = std::sin(m * p.y);
            const float value = sx * sy + 0.72f * tx * ty;
            const float gx = m * std::cos(m * p.x) * sy +
                0.72f * n * std::cos(n * p.x + phase_) * ty;
            const float gy = n * sx * std::cos(n * p.y) +
                0.72f * m * tx * std::cos(m * p.y);
            // Normalize the descent gradient to keep high mode orders stable.
            const float normalizer = 1.0f / std::sqrt(1.0f + gx * gx + gy * gy);
            const float forceX = -value * gx * normalizer * c.attraction;
            const float forceY = -value * gy * normalizer * c.attraction;
            const float jitter = c.agitation * 0.6f * std::sqrt(dt);
            p.vx = std::clamp(p.vx * std::exp(-7.0f * dt) + forceX * dt + (random() - 0.5f) * jitter, -1.25f, 1.25f);
            p.vy = std::clamp(p.vy * std::exp(-7.0f * dt) + forceY * dt + (random() - 0.5f) * jitter, -1.25f, 1.25f);
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            if (p.x < 0.0f) { p.x = -p.x; p.vx = std::abs(p.vx); }
            if (p.x > 1.0f) { p.x = 2.0f - p.x; p.vx = -std::abs(p.vx); }
            if (p.y < 0.0f) { p.y = -p.y; p.vy = std::abs(p.vy); }
            if (p.y > 1.0f) { p.y = 2.0f - p.y; p.vy = -std::abs(p.vy); }
        }
    }
};
}

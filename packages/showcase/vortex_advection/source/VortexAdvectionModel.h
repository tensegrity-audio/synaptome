#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Periodic, softened point-vortex advection. Every allocation and work loop is bounded.
class VortexAdvectionModel {
public:
    static constexpr int tracerCount = 384;
    static constexpr int vortexCount = 8;
    static constexpr int historyCount = 40;
    struct Point { float x = 0.0f, y = 0.0f; };
    struct Vortex { Point position; float sign = 1.0f; };
    struct Tracer { Point position; std::array<Point, historyCount> history{}; float velocity = 0.0f; };
    struct Controls { float circulation = 1.0f, coreRadius = 0.085f, strain = 0.18f, vortexDrift = 0.4f; };

    VortexAdvectionModel() { reset(1701); }
    void reset(std::uint32_t seed) {
        rng_ = seed ? seed : 1u; accumulator_ = sampleClock_ = 0.0f; head_ = 0;
        for (int i = 0; i < vortexCount; ++i) {
            const float angle = static_cast<float>(i) * 6.2831853f / vortexCount;
            vortices_[i] = {{std::cos(angle) * 0.62f + random() * 0.12f,
                            std::sin(angle) * 0.62f + random() * 0.12f}, i % 2 ? -1.0f : 1.0f};
        }
        for (auto& tracer : tracers_) {
            tracer.position = {random(), random()};
            tracer.history.fill(tracer.position); tracer.velocity = 0.0f;
        }
        // A short deterministic startup history makes the first frame useful, even paused.
        for (int i = 0; i < 100; ++i) integrate(1.0f / 120.0f, Controls{});
    }
    void advance(float seconds, const Controls& controls) {
        if (!std::isfinite(seconds) || seconds <= 0.0f) return;
        accumulator_ += std::min(seconds, 0.1f);
        int steps = 0;
        while (accumulator_ >= step_ && steps++ < 12) {
            integrate(step_, controls); accumulator_ -= step_;
        }
        accumulator_ = std::min(accumulator_, step_);
    }
    const std::array<Tracer, tracerCount>& tracers() const { return tracers_; }
    const std::array<Vortex, vortexCount>& vortices() const { return vortices_; }
    int historyHead() const { return head_; }
    double signature() const {
        double value = 0.0; int i = 1;
        for (const auto& t : tracers_) value += i++ * (t.position.x + 1.73 * t.position.y);
        for (const auto& v : vortices_) value += i++ * (v.position.x + 1.13 * v.position.y);
        return value;
    }
    bool finite() const {
        for (const auto& t : tracers_) {
            if (!std::isfinite(t.position.x) || !std::isfinite(t.position.y) ||
                std::abs(t.position.x) > 1.001f || std::abs(t.position.y) > 1.001f) return false;
            for (const auto& p : t.history) if (!std::isfinite(p.x) || !std::isfinite(p.y)) return false;
        }
        for (const auto& v : vortices_) if (!std::isfinite(v.position.x) || !std::isfinite(v.position.y)) return false;
        return true;
    }
private:
    static constexpr float step_ = 1.0f / 120.0f;
    std::array<Tracer, tracerCount> tracers_{};
    std::array<Vortex, vortexCount> vortices_{};
    std::uint32_t rng_ = 1u;
    float accumulator_ = 0.0f, sampleClock_ = 0.0f;
    int head_ = 0;
    float random() { rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5; return static_cast<float>(rng_ & 0xFFFFFFu) / 8388608.0f - 1.0f; }
    static float periodic(float value) { return value - 2.0f * std::floor((value + 1.0f) * 0.5f); }
    Point field(Point p, int omitted, const Controls& c) const {
        Point result;
        const float core = std::clamp(c.coreRadius, 0.025f, 0.3f);
        for (int i = 0; i < vortexCount; ++i) {
            if (i == omitted) continue;
            const float dx = periodic(p.x - vortices_[i].position.x);
            const float dy = periodic(p.y - vortices_[i].position.y);
            const float strength = 0.035f * std::clamp(c.circulation, -2.5f, 2.5f) * vortices_[i].sign /
                (dx * dx + dy * dy + core * core);
            result.x -= dy * strength; result.y += dx * strength;
        }
        // A divergence-free periodic strain field preserves the vortical state model.
        result.x += std::sin(p.y * 3.1415927f) * std::clamp(c.strain, -1.0f, 1.0f) * 0.35f;
        result.y += std::sin(p.x * 3.1415927f) * std::clamp(c.strain, -1.0f, 1.0f) * 0.35f;
        const float length = std::sqrt(result.x * result.x + result.y * result.y);
        if (length > 2.0f) { result.x *= 2.0f / length; result.y *= 2.0f / length; }
        return result;
    }
    void integrate(float dt, const Controls& c) {
        std::array<Point, vortexCount> velocities{};
        for (int i = 0; i < vortexCount; ++i) velocities[i] = field(vortices_[i].position, i, c);
        for (auto& t : tracers_) {
            const auto first = field(t.position, -1, c);
            const Point midpoint{periodic(t.position.x + first.x * dt * 0.5f), periodic(t.position.y + first.y * dt * 0.5f)};
            const auto v = field(midpoint, -1, c);
            t.position = {periodic(t.position.x + v.x * dt), periodic(t.position.y + v.y * dt)};
            t.velocity = std::sqrt(v.x * v.x + v.y * v.y);
        }
        for (int i = 0; i < vortexCount; ++i) {
            const float drift = std::clamp(c.vortexDrift, 0.0f, 1.0f);
            auto& p = vortices_[i].position;
            p = {periodic(p.x + velocities[i].x * dt * drift), periodic(p.y + velocities[i].y * dt * drift)};
        }
        sampleClock_ += dt;
        if (sampleClock_ >= 1.0f / 30.0f) {
            sampleClock_ -= 1.0f / 30.0f; head_ = (head_ + 1) % historyCount;
            for (auto& t : tracers_) t.history[head_] = t.position;
        }
    }
};

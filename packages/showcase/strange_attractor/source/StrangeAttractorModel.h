#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace synaptome { namespace showcase {

// Four nearby Lorenz trajectories, integrated with bounded RK4 steps. No OF,
// transport, audio, device, allocation, or global random state dependencies.
class StrangeAttractorModel {
public:
    struct Point { double x = 0, y = 0, z = 0; };
    struct Controls { double sigma = 10, rho = 28, beta = 2.666666667; };
    static constexpr std::size_t trajectoryCount = 4;
    static constexpr std::size_t historyCount = 1200;
    static constexpr double stepSeconds = 0.005;
    using Trail = std::array<Point, historyCount>;

    StrangeAttractorModel() { reset(1001); }
    static double bounded(double v, double lo, double hi) {
        return std::isfinite(v) ? std::max(lo, std::min(hi, v)) : lo;
    }
    void reset(std::uint32_t seed) { reset(seed, Controls{}); }
    void reset(std::uint32_t seed, const Controls& c) {
        accumulator_ = 0; head_ = 0; steps_ = 0;
        for (std::size_t i = 0; i < trajectoryCount; ++i) {
            // Local, integer hash: same seed and toolchain reproduce every point.
            std::uint32_t h = (seed + 1u) * 1664525u + 1013904223u;
            h ^= h >> 16;
            points_[i] = {0.2 + (h & 65535u) / 65535.0 + i * 0.002,
                          0.4 + i * 0.001, 22.0 + i * 0.003};
        }
        // Explicit initial preparation fills the history so assignment is complete
        // on its first draw. No update or live parameter sweep clears this trail.
        for (int n = 0; n < 2400; ++n) integrate(c, false);
        for (std::size_t n = 0; n < historyCount; ++n) integrate(c, true);
    }
    void update(double seconds, const Controls& c) {
        if (!std::isfinite(seconds) || seconds <= 0) return;
        accumulator_ += std::min(seconds, 0.2);
        int budget = 40;
        while (budget-- > 0 && accumulator_ + 1e-12 >= stepSeconds) {
            integrate(c, true);
            accumulator_ = std::max(0.0, accumulator_ - stepSeconds);
        }
        // Never accumulate unbounded catch-up work after a host stall.
        accumulator_ = std::min(accumulator_, stepSeconds);
    }
    const Point& point(std::size_t trail, std::size_t age) const {
        return history_[trail % trajectoryCount][(head_ + age % historyCount) % historyCount];
    }
    const std::array<Trail, trajectoryCount>& history() const { return history_; }
    std::uint64_t steps() const { return steps_; }
    bool finite() const {
        for (const auto& t : history_) for (const auto& p : t)
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
                std::abs(p.x) > 200 || std::abs(p.y) > 200 || std::abs(p.z) > 200) return false;
        return true;
    }
    std::uint64_t stateSignature() const {
        std::uint64_t h = 1469598103934665603ull;
        for (const auto& trail : history_) for (const auto& p : trail) {
            hash(h, p.x); hash(h, p.y); hash(h, p.z);
        }
        hash(h, accumulator_); h ^= head_; h *= 1099511628211ull; h ^= steps_;
        return h;
    }
private:
    static void hash(std::uint64_t& h, double v) {
        std::uint64_t bits = 0; std::memcpy(&bits, &v, sizeof(v));
        h ^= bits; h *= 1099511628211ull;
    }
    static Point add(const Point& a, const Point& b, double k) {
        return {a.x + b.x * k, a.y + b.y * k, a.z + b.z * k};
    }
    static Point derivative(const Point& p, const Controls& c) {
        return {c.sigma * (p.y - p.x), p.x * (c.rho - p.z) - p.y,
                p.x * p.y - c.beta * p.z};
    }
    void integrate(const Controls& raw, bool store) {
        const Controls c{bounded(raw.sigma, 6, 16), bounded(raw.rho, 24, 40),
                         bounded(raw.beta, 2, 3.5)};
        for (std::size_t i = 0; i < trajectoryCount; ++i) {
            auto& p = points_[i];
            const auto a = derivative(p, c);
            const auto b = derivative(add(p, a, stepSeconds * .5), c);
            const auto d = derivative(add(p, b, stepSeconds * .5), c);
            const auto e = derivative(add(p, d, stepSeconds), c);
            p.x += stepSeconds * (a.x + 2*b.x + 2*d.x + e.x) / 6;
            p.y += stepSeconds * (a.y + 2*b.y + 2*d.y + e.y) / 6;
            p.z += stepSeconds * (a.z + 2*b.z + 2*d.z + e.z) / 6;
            if (store) history_[i][head_] = p;
        }
        if (store) { head_ = (head_ + 1) % historyCount; ++steps_; }
    }
    std::array<Point, trajectoryCount> points_{};
    std::array<Trail, trajectoryCount> history_{};
    std::size_t head_ = 0;
    std::uint64_t steps_ = 0;
    double accumulator_ = 0;
};

}}

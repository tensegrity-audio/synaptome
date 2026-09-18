#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace synaptome_show_differential {

struct Point { float x = 0.0f; float y = 0.0f; };

// A growing elastic ring. Neighbor springs and non-neighbor repulsion create
// folds; confinement and bounded insertion keep the live instrument finite.
class DifferentialGrowthModel {
public:
    struct Controls {
        float growthRate = 0.34f;
        float repulsion = 0.95f;
        float stiffness = 0.8f;
        float confinement = 0.55f;
        float driftRate = 0.12f;
    };
    static constexpr std::size_t maxNodes = 320;
    static constexpr double fixedStep = 1.0 / 60.0;

    void reset(std::uint32_t seed) {
        rng_.seed(seed);
        phase_ = static_cast<float>(rng_() % 10000u) * 0.00062831853f;
        clock_ = 0.0;
        accumulator_ = 0.0;
        restLength_ = 0.026f;
        points_.clear();
        points_.reserve(maxNodes);
        next_.reserve(maxNodes);
        constexpr std::size_t initialNodes = 256;
        constexpr float tau = 6.28318530718f;
        for (std::size_t i = 0; i < initialNodes; ++i) {
            // Follow one side of a coiled ribbon outwards and the other back
            // inwards. This is one closed, initially nonintersecting curve,
            // with substantial length available to buckle on its first frame.
            const bool returning = i >= initialNodes / 2;
            const std::size_t ordinal = returning ? initialNodes - 1 - i : i;
            const float t = static_cast<float>(ordinal) / (initialNodes / 2 - 1);
            const float a = phase_ + tau * 2.15f * t;
            const float r = 0.095f + 0.64f * t + (returning ? 0.032f : 0.0f)
                            + 0.009f * std::sin(a * 5.0f + phase_);
            points_.push_back({r * std::cos(a), r * std::sin(a)});
        }
    }

    void advance(double seconds, const Controls& controls) {
        if (points_.empty()) reset(104729u);
        if (!std::isfinite(seconds) || seconds <= 0.0) return;
        accumulator_ = std::min(accumulator_ + std::min(seconds, 0.25), fixedStep * 16.0);
        int work = 0;
        while (accumulator_ + 1.0e-10 >= fixedStep && work++ < 16) {
            step(controls);
            accumulator_ -= fixedStep;
        }
        accumulator_ = std::max(0.0, accumulator_);
    }

    const std::vector<Point>& points() const { return points_; }
    double time() const { return clock_; }
    std::uint64_t stateSignature() const {
        std::uint64_t h = 1469598103934665603ull;
        for (const auto& p : points_) {
            for (float v : {p.x, p.y}) {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &v, sizeof(bits));
                h = (h ^ bits) * 1099511628211ull;
            }
        }
        return h ^ static_cast<std::uint64_t>(points_.size());
    }

private:
    static float bounded(float x, float lo, float hi) {
        return std::isfinite(x) ? std::max(lo, std::min(hi, x)) : lo;
    }
    void step(const Controls& c) {
        const float growth = bounded(c.growthRate, 0.0f, 1.0f);
        const float repulsion = bounded(c.repulsion, 0.0f, 2.0f);
        const float stiffness = bounded(c.stiffness, 0.0f, 2.0f);
        const float confinement = bounded(c.confinement, 0.0f, 1.0f);
        const float drift = bounded(c.driftRate, -1.0f, 1.0f);
        const float dt = static_cast<float>(fixedStep);
        clock_ += fixedStep;
        restLength_ = std::min(0.052f, restLength_ + growth * dt * 0.00045f);
        next_.resize(points_.size());
        const std::size_t count = points_.size();
        const float repelRadius = 0.052f + growth * 0.027f;
        const float radius2 = repelRadius * repelRadius;
        for (std::size_t i = 0; i < count; ++i) {
            const Point p = points_[i];
            Point force{};
            for (std::size_t j : {(i + count - 1) % count, (i + 1) % count}) {
                const float dx = points_[j].x - p.x;
                const float dy = points_[j].y - p.y;
                const float d = std::sqrt(dx * dx + dy * dy + 1.0e-10f);
                const float spring = (d - restLength_) * stiffness * 5.0f / d;
                force.x += dx * spring;
                force.y += dy * spring;
            }
            // A small bending regularizer suppresses one-vertex sawteeth
            // without erasing the larger folds driven by differential growth.
            const Point previous = points_[(i + count - 1) % count];
            const Point following = points_[(i + 1) % count];
            force.x += (previous.x + following.x - 2.0f * p.x) * 1.8f;
            force.y += (previous.y + following.y - 2.0f * p.y) * 1.8f;
            for (std::size_t j = 0; j < count; ++j) {
                if (j == i || j == (i + 1) % count || j == (i + count - 1) % count) continue;
                const float dx = p.x - points_[j].x;
                const float dy = p.y - points_[j].y;
                const float d2 = dx * dx + dy * dy;
                if (d2 < radius2 && d2 > 1.0e-12f) {
                    const float d = std::sqrt(d2);
                    const float strength = repulsion * 0.11f * (1.0f - d / repelRadius) / d;
                    force.x += dx * strength;
                    force.y += dy * strength;
                }
            }
            const float r = std::sqrt(p.x * p.x + p.y * p.y + 1.0e-10f);
            const float boundary = 0.90f - confinement * 0.14f;
            if (r > boundary) {
                const float inward = (r - boundary) * 3.5f / r;
                force.x -= p.x * inward;
                force.y -= p.y * inward;
            }
            force.x += -p.y * drift * 0.12f;
            force.y += p.x * drift * 0.12f;
            const float move = std::sqrt(force.x * force.x + force.y * force.y);
            const float cap = move > 0.18f ? 0.18f / move : 1.0f;
            next_[i] = {bounded(p.x + force.x * dt * cap, -0.98f, 0.98f),
                        bounded(p.y + force.y * dt * cap, -0.98f, 0.98f)};
        }
        points_.swap(next_);
        if (growth > 0.0f && points_.size() < maxNodes) {
            // One insertion per solver step bounds both work and topology churn.
            for (std::size_t i = 0; i < points_.size(); ++i) {
                const auto a = points_[i];
                const auto b = points_[(i + 1) % points_.size()];
                const float dx = a.x - b.x;
                const float dy = a.y - b.y;
                if (dx * dx + dy * dy > restLength_ * restLength_ * 2.89f) {
                    points_.insert(points_.begin() + static_cast<std::ptrdiff_t>(i + 1),
                                   {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f});
                    break;
                }
            }
        }
    }
    std::vector<Point> points_;
    std::vector<Point> next_;
    std::mt19937 rng_;
    double accumulator_ = 0.0;
    double clock_ = 0.0;
    float restLength_ = 0.026f;
    float phase_ = 0.0f;
};
}

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

namespace synaptome_show_magnetic {

struct Point { float x = 0.0f; float y = 0.0f; };
struct Segment { Point a; Point b; float strength = 0.0f; };

// A planar slice through a superposed 3D dipole field. Streamlines use a
// softened analytical field and bounded midpoint integration, never N-body.
class MagneticDipolesModel {
public:
    struct Controls {
        float separation = 0.46f;
        float coupling = 0.62f;
        float orbitRate = 8.0f; // degrees per simulation second
        float twist = 0.35f;
        float fieldReach = 0.88f;
    };
    struct Dipole { Point position; Point moment; };
    static constexpr std::size_t maxSegments = 6912;
    static constexpr double fixedStep = 1.0 / 30.0;

    void reset(std::uint32_t seed) {
        rng_.seed(seed);
        for (auto& phase : phases_) phase = static_cast<float>(rng_() % 10000u) * 0.00062831853f;
        time_ = 0.0;
        accumulator_ = 0.0;
        angle_ = 0.0f;
        segments_.clear();
        segments_.reserve(maxSegments);
        rebuild(Controls{});
    }
    void advance(double seconds, const Controls& controls) {
        if (segments_.empty()) rebuild(controls);
        if (std::isfinite(seconds) && seconds > 0.0) {
            accumulator_ = std::min(accumulator_ + std::min(seconds, 0.25), fixedStep * 8.0);
            int work = 0;
            while (accumulator_ + 1.0e-10 >= fixedStep && work++ < 8) {
                time_ += fixedStep;
                angle_ += bound(controls.orbitRate, -60.0f, 60.0f)
                    * 0.01745329252f * static_cast<float>(fixedStep);
                angle_ = std::remainder(angle_, 6.28318530718f);
                accumulator_ -= fixedStep;
            }
            accumulator_ = std::max(0.0, accumulator_);
        }
        // Geometry responds to live controls even while evolution is paused.
        rebuild(controls);
    }
    const std::vector<Segment>& segments() const { return segments_; }
    const std::array<Dipole, 3>& dipoles() const { return dipoles_; }
    double time() const { return time_; }
    std::uint64_t stateSignature() const {
        std::uint64_t h = 1469598103934665603ull;
        for (const auto& s : segments_) {
            for (float v : {s.a.x, s.a.y, s.b.x, s.b.y}) {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &v, sizeof(bits));
                h = (h ^ bits) * 1099511628211ull;
            }
        }
        return h;
    }

private:
    static float bound(float x, float low, float high) {
        return std::isfinite(x) ? std::max(low, std::min(high, x)) : low;
    }
    Point field(Point p) const {
        Point b{};
        for (const auto& d : dipoles_) {
            const float x = p.x - d.position.x;
            const float y = p.y - d.position.y;
            const float r2 = x * x + y * y + 0.0016f;
            const float invR = 1.0f / std::sqrt(r2);
            const float invR3 = invR / r2;
            const float dot = x * d.moment.x + y * d.moment.y;
            b.x += (3.0f * x * dot / r2 - d.moment.x) * invR3;
            b.y += (3.0f * y * dot / r2 - d.moment.y) * invR3;
        }
        return b;
    }
    static Point normalized(Point p) {
        const float length = std::sqrt(p.x * p.x + p.y * p.y);
        if (length < 1.0e-7f || !std::isfinite(length)) return {};
        return {p.x / length, p.y / length};
    }
    void rebuild(const Controls& c) {
        const float separation = bound(c.separation, 0.15f, 0.70f);
        const float coupling = bound(c.coupling, 0.0f, 1.0f);
        const float twist = bound(c.twist, -1.0f, 1.0f);
        const float reach = bound(c.fieldReach, 0.55f, 1.15f);
        for (std::size_t i = 0; i < dipoles_.size(); ++i) {
            const float a = static_cast<float>(i) * 2.09439510239f + angle_ + phases_[i] * 0.12f;
            const float direction = a * 0.5f + phases_[i] + twist * 3.14159265f;
            const float strength = i == 0 ? 1.0f : 0.3f + coupling * 0.7f;
            dipoles_[i] = {{std::cos(a) * separation, std::sin(a) * separation * 0.80f},
                           {std::cos(direction) * strength, std::sin(direction) * strength}};
        }
        segments_.clear();
        constexpr int lineCount = 12;
        constexpr int samples = 96;
        constexpr float step = 0.017f;
        for (const auto& origin : dipoles_) {
            for (int line = 0; line < lineCount; ++line) {
                const float a = static_cast<float>(line) * 6.28318530718f / lineCount;
                const Point start{origin.position.x + std::cos(a) * 0.055f,
                                  origin.position.y + std::sin(a) * 0.055f};
                for (int direction : {-1, 1}) {
                    Point p = start;
                    for (int sample = 0; sample < samples; ++sample) {
                        const Point b = field(p);
                        const Point first = normalized(b);
                        if (first.x == 0.0f && first.y == 0.0f) break;
                        const float ds = step * static_cast<float>(direction);
                        const Point middle{p.x + first.x * ds * 0.5f, p.y + first.y * ds * 0.5f};
                        const Point gradient = normalized(field(middle));
                        const Point q{p.x + gradient.x * ds, p.y + gradient.y * ds};
                        if (q.x * q.x + q.y * q.y > reach * reach) break;
                        const float magnitude = std::sqrt(b.x * b.x + b.y * b.y);
                        const float glow = magnitude / (magnitude + 60.0f);
                        segments_.push_back({p, q, glow});
                        bool atPole = false;
                        for (const auto& pole : dipoles_) {
                            const float dx = q.x - pole.position.x;
                            const float dy = q.y - pole.position.y;
                            atPole = atPole || (dx * dx + dy * dy < 0.0013f);
                        }
                        if (atPole) break;
                        p = q;
                    }
                }
            }
        }
    }
    std::array<Dipole, 3> dipoles_{};
    std::array<float, 3> phases_{};
    std::vector<Segment> segments_;
    std::mt19937 rng_;
    double time_ = 0.0;
    double accumulator_ = 0.0;
    float angle_ = 0.0f;
};
}

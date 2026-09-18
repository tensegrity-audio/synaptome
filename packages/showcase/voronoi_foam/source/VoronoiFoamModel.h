#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Moving periodic Voronoi sites with sampled Lloyd relaxation and soft pair pressure.
class VoronoiFoamModel {
public:
    static constexpr int siteCount = 32;
    struct Point { float x = 0.0f, y = 0.0f; };
    struct Site { Point position, drift, centroidOffset; float area = 0.125f; };
    struct Sample { float nearest = 0.0f, gap = 0.0f; int site = 0; };
    struct Controls { float driftSpeed = 0.35f, relaxationRate = 0.7f, repulsionWeight = 0.5f, shear = 0.15f; };
    VoronoiFoamModel() { reset(1703); }
    void reset(std::uint32_t seed) {
        rng_ = seed ? seed : 1u; accumulator_ = centroidClock_ = phase_ = 0.0f;
        for (int i = 0; i < siteCount; ++i) {
            const float angle = random() * 3.1415927f;
            sites_[i] = {{-0.875f + (i % 8) * 0.25f + random() * 0.075f,
                          -0.75f + (i / 8) * 0.5f + random() * 0.10f},
                         {std::cos(angle), std::sin(angle)}, {}, 0.125f};
        }
        relaxCentroids();
    }
    void advance(float seconds, const Controls& c) {
        if (!std::isfinite(seconds) || seconds <= 0.0f) return;
        accumulator_ += std::min(seconds, 0.1f); int steps = 0;
        while (accumulator_ >= step_ && steps++ < 12) { integrate(step_, c); accumulator_ -= step_; }
        accumulator_ = std::min(accumulator_, step_);
    }
    const std::array<Site, siteCount>& sites() const { return sites_; }
    Sample sample(float x, float y) const {
        float first = 100.0f, second = 100.0f; int owner = 0;
        for (int i = 0; i < siteCount; ++i) {
            const float dx = periodic(x - sites_[i].position.x), dy = periodic(y - sites_[i].position.y);
            const float distance = dx * dx + dy * dy;
            if (distance < first) { second = first; first = distance; owner = i; }
            else if (distance < second) second = distance;
        }
        first = std::sqrt(first); second = std::sqrt(second);
        return {first, second - first, owner};
    }
    double signature() const { double value = 0.0; for (int i = 0; i < siteCount; ++i) value += (i + 1) * (sites_[i].position.x + 1.73 * sites_[i].position.y); return value; }
    bool finite() const {
        for (const auto& s : sites_) if (!std::isfinite(s.position.x) || !std::isfinite(s.position.y) ||
            std::abs(s.position.x) > 1.001f || std::abs(s.position.y) > 1.001f || !std::isfinite(s.area)) return false;
        return true;
    }
private:
    static constexpr float step_ = 1.0f / 120.0f;
    std::array<Site, siteCount> sites_{};
    std::uint32_t rng_ = 1u;
    float accumulator_ = 0.0f, centroidClock_ = 0.0f, phase_ = 0.0f;
    float random() { rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5; return static_cast<float>(rng_ & 0xFFFFFFu) / 8388608.0f - 1.0f; }
    static float periodic(float value) { return value - 2.0f * std::floor((value + 1.0f) * 0.5f); }
    void relaxCentroids() {
        std::array<Point, siteCount> sums{}; std::array<int, siteCount> counts{};
        constexpr int columns = 48, rows = 30;
        for (int y = 0; y < rows; ++y) for (int x = 0; x < columns; ++x) {
            const float px = -1.0f + (x + 0.5f) * 2.0f / columns, py = -1.0f + (y + 0.5f) * 2.0f / rows;
            const int nearest = sample(px, py).site;
            sums[nearest].x += periodic(px - sites_[nearest].position.x);
            sums[nearest].y += periodic(py - sites_[nearest].position.y); ++counts[nearest];
        }
        for (int i = 0; i < siteCount; ++i) {
            const float divisor = static_cast<float>(std::max(1, counts[i]));
            sites_[i].centroidOffset = {sums[i].x / divisor, sums[i].y / divisor};
            sites_[i].area = counts[i] * 4.0f / (columns * rows);
        }
    }
    void integrate(float dt, const Controls& c) {
        phase_ = std::fmod(phase_ + dt * 0.22f, 6.2831853f);
        std::array<Point, siteCount> velocity{};
        for (int i = 0; i < siteCount; ++i) {
            const auto& s = sites_[i]; auto& v = velocity[i];
            const float drift = std::clamp(c.driftSpeed, 0.0f, 2.0f) * 0.16f;
            const float relax = std::clamp(c.relaxationRate, 0.0f, 3.0f);
            v.x = (s.drift.x * std::cos(phase_) - s.drift.y * std::sin(phase_)) * drift + s.centroidOffset.x * relax;
            v.y = (s.drift.x * std::sin(phase_) + s.drift.y * std::cos(phase_)) * drift + s.centroidOffset.y * relax;
            v.x += std::sin(s.position.y * 3.1415927f) * std::clamp(c.shear, -1.0f, 1.0f) * 0.3f;
            for (int j = 0; j < siteCount; ++j) if (i != j) {
                const float dx = periodic(s.position.x - sites_[j].position.x), dy = periodic(s.position.y - sites_[j].position.y);
                const float distance = std::sqrt(dx * dx + dy * dy + 0.00001f);
                const float force = std::max(0.0f, 0.28f - distance) * std::clamp(c.repulsionWeight, 0.0f, 2.0f);
                v.x += dx * force / distance; v.y += dy * force / distance;
            }
        }
        for (int i = 0; i < siteCount; ++i) {
            sites_[i].position.x = periodic(sites_[i].position.x + velocity[i].x * dt);
            sites_[i].position.y = periodic(sites_[i].position.y + velocity[i].y * dt);
        }
        centroidClock_ += dt;
        if (centroidClock_ >= 0.1f) { centroidClock_ -= 0.1f; relaxCentroids(); }
    }
};

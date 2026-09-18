#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Conservative saltation and local avalanching on a periodic bed, in normalized world units.
class SandRipplesModel {
public:
    static constexpr int columns = 64, rows = 40, cellCount = columns * rows;
    struct Controls { float transportRate = 1.0f, windDirectionDeg = 12.0f, hopLength = 0.12f, reposeSlope = 0.025f, relaxationRate = 1.5f; };
    SandRipplesModel() { reset(1702); }
    void reset(std::uint32_t seed) {
        rng_ = seed ? seed : 1u; accumulator_ = 0.0f;
        for (int y = 0; y < rows; ++y) for (int x = 0; x < columns; ++x) {
            const float px = static_cast<float>(x) / columns, py = static_cast<float>(y) / rows;
            heights_[index(x, y)] = 0.22f + 0.060f * std::sin(px * 31.4159265f + 1.3f * std::sin(py * 6.2831853f)) +
                0.022f * std::cos(px * 12.5663706f + py * 18.849556f) + random() * 0.002f;
        }
    }
    void advance(float seconds, const Controls& c) {
        if (!std::isfinite(seconds) || seconds <= 0.0f) return;
        accumulator_ += std::min(seconds, 0.1f); int steps = 0;
        while (accumulator_ >= step_ && steps++ < 12) { integrate(step_, c); accumulator_ -= step_; }
        accumulator_ = std::min(accumulator_, step_);
    }
    const std::array<float, cellCount>& heights() const { return heights_; }
    float height(int x, int y) const { return heights_[index(x, y)]; }
    double signature() const { double value = 0.0; for (int i = 0; i < cellCount; ++i) value += (i + 1) * static_cast<double>(heights_[i]); return value; }
    double mass() const { double result = 0.0; for (float h : heights_) result += h; return result; }
    bool finite() const { for (float h : heights_) if (!std::isfinite(h) || h < -0.0001f || h > 4.0f) return false; return true; }
private:
    static constexpr float step_ = 1.0f / 120.0f;
    std::array<float, cellCount> heights_{}, changes_{};
    std::uint32_t rng_ = 1u;
    float accumulator_ = 0.0f;
    float random() { rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5; return static_cast<float>(rng_ & 0xFFFFFFu) / 8388608.0f - 1.0f; }
    static int index(int x, int y) { return ((y % rows + rows) % rows) * columns + ((x % columns + columns) % columns); }
    float sample(float x, float y) const {
        const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
        const float tx = x - ix, ty = y - iy;
        return (1.0f - ty) * ((1.0f - tx) * height(ix, iy) + tx * height(ix + 1, iy)) +
            ty * ((1.0f - tx) * height(ix, iy + 1) + tx * height(ix + 1, iy + 1));
    }
    void deposit(float x, float y, float amount) {
        const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
        const float tx = x - ix, ty = y - iy;
        changes_[index(ix, iy)] += amount * (1.0f - tx) * (1.0f - ty);
        changes_[index(ix + 1, iy)] += amount * tx * (1.0f - ty);
        changes_[index(ix, iy + 1)] += amount * (1.0f - tx) * ty;
        changes_[index(ix + 1, iy + 1)] += amount * tx * ty;
    }
    void integrate(float dt, const Controls& c) {
        const float angle = c.windDirectionDeg * 0.01745329252f;
        const float dx = std::cos(angle), dy = std::sin(angle);
        const float rate = std::clamp(c.transportRate, 0.0f, 4.0f);
        changes_.fill(0.0f);
        for (int y = 0; y < rows; ++y) for (int x = 0; x < columns; ++x) {
            const int at = index(x, y); const float h = heights_[at];
            const float upwindSlope = h - sample(x - dx * 2.0f, y - dy * 2.0f);
            const float exposure = std::clamp(0.7f + upwindSlope * 12.0f, 0.05f, 1.8f);
            const float amount = std::min(h * 0.08f, dt * rate * 0.025f * exposure);
            const float hop = std::clamp(c.hopLength, 0.02f, 0.35f) * (0.55f + h * 2.0f);
            changes_[at] -= amount;
            deposit(x + dx * hop * columns, y + dy * hop * rows, amount);
        }
        for (int i = 0; i < cellCount; ++i) heights_[i] += changes_[i];
        changes_.fill(0.0f);
        // Pair fluxes are conservative. Each edge is visited once; dt bounds prevent overshoot.
        const float relaxation = std::clamp(c.relaxationRate, 0.0f, 4.0f) * dt * 2.0f;
        const float repose = std::clamp(c.reposeSlope, 0.005f, 0.12f);
        for (int y = 0; y < rows; ++y) for (int x = 0; x < columns; ++x) {
            const int at = index(x, y);
            for (int neighbor : {index(x + 1, y), index(x, y + 1)}) {
                const float difference = heights_[at] - heights_[neighbor];
                const float excess = std::max(0.0f, std::abs(difference) - repose);
                const float flux = (difference > 0.0f ? 1.0f : -1.0f) * excess * relaxation;
                changes_[at] -= flux; changes_[neighbor] += flux;
            }
        }
        for (int i = 0; i < cellCount; ++i) heights_[i] += changes_[i];
    }
};

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace synaptome_show_dendritic {

// Diffusion-limited aggregation with finite nutrient walkers and gradual
// dissolution. Permanent seed branches preserve a readable image at all times.
class DendriticCrystalModel {
public:
    struct Controls {
        float growthRate = 3600.0f; // walker steps per simulation second
        float adhesion = 0.72f;
        float anisotropy = 0.68f;
        float dissolutionRate = 0.012f; // inverse seconds
        float drift = 0.12f;
    };
    struct Cell { float born = 0.0f; bool occupied = false; bool root = false; };
    struct Walker { int x = 0; int y = 0; };
    static constexpr int width = 192;
    static constexpr int height = 108;
    static constexpr std::size_t walkerCount = 96;
    static constexpr double fixedStep = 1.0 / 60.0;

    void reset(std::uint32_t seed) {
        rng_.seed(seed);
        cells_.assign(width * height, Cell{});
        time_ = 0.0;
        accumulator_ = 0.0;
        walkerBudget_ = 0.0;
        dissolveCursor_ = 0;
        nextWalker_ = 0;
        occupied_ = 0;
        // Distributed nuclei make a complete first frame and leave room for
        // new walkers to fuse branch tips into a evolving crystalline field.
        for (int root = 0; root < 6; ++root) {
            const int cx = 31 + (root % 3) * 64 + static_cast<int>(rng_() % 13u) - 6;
            const int cy = 29 + (root / 3) * 50 + static_cast<int>(rng_() % 11u) - 5;
            mark(cx, cy, true);
            const int rotation = static_cast<int>(rng_() % 8u);
            for (int arm = 0; arm < 6; ++arm) {
                const float angle = (static_cast<float>(arm) / 6.0f + rotation * 0.015f) * 6.28318530718f;
                const int length = 12 + static_cast<int>(rng_() % 10u);
                for (int s = 1; s < length; ++s) {
                    const int x = cx + static_cast<int>(std::round(std::cos(angle) * s));
                    const int y = cy + static_cast<int>(std::round(std::sin(angle) * s));
                    mark(x, y, true);
                    if (s > 4 && s % 4 == 0) {
                        const float branchAngle = angle + ((s % 8 == 0) ? 0.8f : -0.8f);
                        for (int branch = 1; branch <= 5; ++branch) {
                            mark(x + static_cast<int>(std::round(std::cos(branchAngle) * branch)),
                                 y + static_cast<int>(std::round(std::sin(branchAngle) * branch)), true);
                        }
                    }
                }
            }
        }
        for (auto& walker : walkers_) respawn(walker);
    }

    void advance(double seconds, const Controls& controls) {
        if (cells_.empty()) reset(130363u);
        if (!std::isfinite(seconds) || seconds <= 0.0) return;
        accumulator_ = std::min(accumulator_ + std::min(seconds, 0.25), fixedStep * 16.0);
        int work = 0;
        while (accumulator_ + 1.0e-10 >= fixedStep && work++ < 16) {
            step(controls);
            accumulator_ -= fixedStep;
        }
        accumulator_ = std::max(0.0, accumulator_);
    }
    const std::vector<Cell>& cells() const { return cells_; }
    std::size_t occupiedCount() const { return occupied_; }
    double time() const { return time_; }
    std::uint64_t stateSignature() const {
        std::uint64_t h = 1469598103934665603ull;
        for (std::size_t i = 0; i < cells_.size(); ++i) {
            if (cells_[i].occupied) h = (h ^ (i + 1)) * 1099511628211ull;
        }
        for (const auto& w : walkers_) h = (h ^ static_cast<std::uint64_t>(w.x + w.y * width)) * 1099511628211ull;
        return h;
    }

private:
    static float bound(float x, float low, float high) {
        return std::isfinite(x) ? std::max(low, std::min(high, x)) : low;
    }
    float unit() { return static_cast<float>(rng_() >> 8) * (1.0f / 16777216.0f); }
    void mark(int x, int y, bool root) {
        if (x < 2 || x >= width - 2 || y < 2 || y >= height - 2) return;
        auto& cell = cells_[static_cast<std::size_t>(y * width + x)];
        if (!cell.occupied) ++occupied_;
        cell.occupied = true;
        cell.root = cell.root || root;
        cell.born = static_cast<float>(time_);
    }
    void respawn(Walker& walker) {
        walker.x = 2 + static_cast<int>(rng_() % static_cast<unsigned>(width - 4));
        walker.y = 2 + static_cast<int>(rng_() % static_cast<unsigned>(height - 4));
    }
    bool neighbors(int x, int y) const {
        return cells_[static_cast<std::size_t>(y * width + x - 1)].occupied
            || cells_[static_cast<std::size_t>(y * width + x + 1)].occupied
            || cells_[static_cast<std::size_t>((y - 1) * width + x)].occupied
            || cells_[static_cast<std::size_t>((y + 1) * width + x)].occupied;
    }
    void step(const Controls& c) {
        time_ += fixedStep;
        const float adhesion = bound(c.adhesion, 0.05f, 1.0f);
        const float anisotropy = bound(c.anisotropy, 0.0f, 1.0f);
        const float drift = bound(c.drift, -1.0f, 1.0f);
        const float dissolution = bound(c.dissolutionRate, 0.0f, 0.1f);
        walkerBudget_ += bound(c.growthRate, 0.0f, 15000.0f) * fixedStep;
        const int work = std::min(256, static_cast<int>(walkerBudget_));
        walkerBudget_ -= work;
        walkerBudget_ = std::min(walkerBudget_, 256.0);
        static constexpr int dx[8] = {1, 0, -1, 0, 1, -1, -1, 1};
        static constexpr int dy[8] = {0, 1, 0, -1, 1, 1, -1, -1};
        for (int n = 0; n < work; ++n) {
            Walker& w = walkers_[nextWalker_++ % walkerCount];
            const unsigned directionCount = unit() < anisotropy ? 4u : 8u;
            int direction = static_cast<int>(rng_() % directionCount);
            if (unit() < std::abs(drift) * 0.25f) direction = drift >= 0.0f ? 1 : 3;
            w.x += dx[direction];
            w.y += dy[direction];
            if (w.x < 2 || w.x >= width - 2 || w.y < 2 || w.y >= height - 2) {
                respawn(w);
                continue;
            }
            if (cells_[static_cast<std::size_t>(w.y * width + w.x)].occupied) {
                respawn(w);
                continue;
            }
            if (occupied_ < cells_.size() / 2 && neighbors(w.x, w.y) && unit() < adhesion) {
                mark(w.x, w.y, false);
                respawn(w);
            }
        }
        // One full dissolution inspection per second, spread over frames.
        const std::size_t inspect = (cells_.size() + 59) / 60;
        if (dissolution > 0.0f) {
            const float lifetime = 1.0f / dissolution;
            for (std::size_t i = 0; i < inspect; ++i) {
                auto& cell = cells_[dissolveCursor_++ % cells_.size()];
                if (cell.occupied && !cell.root && time_ - cell.born > lifetime) {
                    cell.occupied = false;
                    --occupied_;
                }
            }
        }
    }
    std::vector<Cell> cells_;
    std::array<Walker, walkerCount> walkers_{};
    std::mt19937 rng_;
    std::size_t occupied_ = 0;
    std::size_t nextWalker_ = 0;
    std::size_t dissolveCursor_ = 0;
    double time_ = 0.0;
    double accumulator_ = 0.0;
    double walkerBudget_ = 0.0;
};
}

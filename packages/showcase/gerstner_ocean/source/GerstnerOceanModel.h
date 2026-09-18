#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace synaptome { namespace showcase {

// A fixed grid sampling three deep-water Gerstner waves. The sum of horizontal
// displacement gradients is capped at 0.72, keeping the surface non-folding.
class GerstnerOceanModel {
public:
    struct Point { double x = 0, y = 0, z = 0; };
    struct Controls {
        double waveHeight = 0.18, wavelength = 1.8, steepness = 0.75;
        double directionDeg = 18;
    };
    static constexpr int columns = 40, rows = 28;
    static constexpr double pi = 3.14159265358979323846;
    using Surface = std::array<Point, (columns + 1) * (rows + 1)>;
    GerstnerOceanModel() { reset(1001); }
    static double bounded(double v, double lo, double hi) {
        return std::isfinite(v) ? std::max(lo, std::min(hi, v)) : lo;
    }
    void reset(std::uint32_t seed) { reset(seed, Controls{}); }
    void reset(std::uint32_t seed, const Controls& c) {
        for (std::size_t i = 0; i < phase_.size(); ++i) {
            seed = seed * 1664525u + 1013904223u;
            phase_[i] = (seed & 65535u) * (2*pi/65536.0);
        }
        evaluate(c);
    }
    void update(double seconds, const Controls& c) {
        if (!std::isfinite(seconds) || seconds <= 0) return;
        const double dt = std::min(seconds, .2);
        const double wavelength = bounded(c.wavelength, .7, 3.0);
        constexpr double ratio[3] = {1.0, .57, .31};
        for (std::size_t i = 0; i < phase_.size(); ++i) {
            const double k = 2*pi/(wavelength*ratio[i]);
            phase_[i] = std::fmod(phase_[i] + std::sqrt(9.81*k)*dt, 2*pi);
        }
        evaluate(c);
    }
    void evaluate(const Controls& raw) {
        const double height = bounded(raw.waveHeight, .01, .32);
        const double wavelength = bounded(raw.wavelength, .7, 3.0);
        const double steepness = bounded(raw.steepness, 0, 1);
        const double direction = bounded(raw.directionDeg, -180, 180) * pi / 180;
        constexpr double ratio[3] = {1.0, .57, .31};
        constexpr double weights[3] = {.62, .26, .12};
        const double offsets[3] = {0, 37*pi/180, -51*pi/180};
        for (int row = 0; row <= rows; ++row) for (int col = 0; col <= columns; ++col) {
            const double u = 3.2 * (static_cast<double>(col)/columns - .5);
            const double v = 2.3 * (static_cast<double>(row)/rows - .5);
            Point p{u, 0, v};
            for (std::size_t i = 0; i < phase_.size(); ++i) {
                const double k = 2*pi/(wavelength*ratio[i]);
                const double dx = std::cos(direction+offsets[i]);
                const double dz = std::sin(direction+offsets[i]);
                const double angle = k*(dx*u+dz*v)-phase_[i];
                const double a = height*weights[i];
                const double horizontal = std::min(a*steepness, .24/k);
                p.x += horizontal*dx*std::cos(angle);
                p.z += horizontal*dz*std::cos(angle);
                p.y += a*std::sin(angle);
            }
            surface_[row*(columns+1)+col] = p;
        }
    }
    const Surface& surface() const { return surface_; }
    const std::array<double, 3>& phases() const { return phase_; }
    bool finite() const {
        for (const auto& p : surface_)
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
                std::abs(p.x)>2 || std::abs(p.y)>.321 || std::abs(p.z)>2) return false;
        return true;
    }
    std::uint64_t stateSignature() const {
        std::uint64_t h = 1469598103934665603ull;
        for (const auto& p : surface_) { hash(h,p.x); hash(h,p.y); hash(h,p.z); }
        for (double p : phase_) hash(h,p);
        return h;
    }
private:
    static void hash(std::uint64_t& h, double v) {
        std::uint64_t bits = 0; std::memcpy(&bits, &v, sizeof(v));
        h ^= bits; h *= 1099511628211ull;
    }
    Surface surface_{};
    std::array<double, 3> phase_{};
};

}}

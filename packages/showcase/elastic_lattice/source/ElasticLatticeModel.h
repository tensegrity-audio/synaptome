#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace synaptome { namespace showcase {

// Fixed mass/spring membrane with zero-displacement boundary, restoring
// anchors, viscous damping, and a smoothly travelling localized driver.
class ElasticLatticeModel {
public:
    struct Controls {
        double springStiffness = 45, restoringForce = 2.5, dampingRate = 1.2;
        double impulseStrength = 5, impulseRate = .45;
    };
    static constexpr int columns = 36, rows = 28;
    static constexpr std::size_t nodeCount = (columns+1)*(rows+1);
    static constexpr double stepSeconds = 1.0/240.0;
    using Field = std::array<double, nodeCount>;
    ElasticLatticeModel() { reset(1001); }
    static double bounded(double v, double lo, double hi) {
        return std::isfinite(v) ? std::max(lo, std::min(hi, v)) : lo;
    }
    void reset(std::uint32_t seed) { reset(seed, Controls{}); }
    void reset(std::uint32_t seed, const Controls&) {
        constexpr double pi = 3.14159265358979323846;
        seed = seed * 1664525u + 1013904223u;
        phase_ = (seed & 65535u) * (2*pi/65536.0);
        accumulator_ = 0; steps_ = 0;
        velocity_.fill(0); nextVelocity_.fill(0); height_.fill(0);
        for (int y=1; y<rows; ++y) for (int x=1; x<columns; ++x) {
            const double u = static_cast<double>(x)/columns;
            const double v = static_cast<double>(y)/rows;
            height_[y*(columns+1)+x] = .13*std::sin(pi*u)*std::sin(pi*v)*
                (std::sin(4*pi*u+phase_)+.6*std::sin(3*pi*v-phase_));
        }
    }
    void update(double seconds, const Controls& c) {
        if (!std::isfinite(seconds) || seconds<=0) return;
        accumulator_ += std::min(seconds, .2);
        int budget = 48;
        while (budget-- > 0 && accumulator_+1e-12>=stepSeconds) {
            integrate(c);
            accumulator_ = std::max(0.0, accumulator_-stepSeconds);
        }
        accumulator_ = std::min(accumulator_, stepSeconds);
    }
    const Field& heights() const { return height_; }
    const Field& velocities() const { return velocity_; }
    double phase() const { return phase_; }
    std::uint64_t steps() const { return steps_; }
    bool finite() const {
        for (std::size_t i=0; i<nodeCount; ++i)
            if (!std::isfinite(height_[i]) || !std::isfinite(velocity_[i]) ||
                std::abs(height_[i])>1.5 || std::abs(velocity_[i])>15) return false;
        return true;
    }
    std::uint64_t stateSignature() const {
        std::uint64_t h = 1469598103934665603ull;
        for (double v : height_) hash(h,v);
        for (double v : velocity_) hash(h,v);
        hash(h,phase_); hash(h,accumulator_); h ^= steps_; return h;
    }
private:
    static void hash(std::uint64_t& h, double v) {
        std::uint64_t bits=0; std::memcpy(&bits,&v,sizeof(v));
        h ^= bits; h *= 1099511628211ull;
    }
    void integrate(const Controls& raw) {
        constexpr double pi = 3.14159265358979323846;
        const double stiffness = bounded(raw.springStiffness,8,90);
        const double restoring = bounded(raw.restoringForce,.4,8);
        const double damping = bounded(raw.dampingRate,.3,6);
        const double strength = bounded(raw.impulseStrength,0,15);
        phase_ = std::fmod(phase_+2*pi*bounded(raw.impulseRate,.1,2)*stepSeconds,2*pi);
        const double centerX=.5+.28*std::cos(phase_);
        const double centerY=.5+.23*std::sin(phase_);
        for (int y=1; y<rows; ++y) for (int x=1; x<columns; ++x) {
            const int i=y*(columns+1)+x;
            const double lap=height_[i-1]+height_[i+1]+height_[i-columns-1]+
                             height_[i+columns+1]-4*height_[i];
            const double dx=static_cast<double>(x)/columns-centerX;
            const double dy=static_cast<double>(y)/rows-centerY;
            const double drive=strength*std::exp(-(dx*dx+dy*dy)/.009)*std::sin(phase_*3);
            const double h=height_[i];
            const double acceleration=stiffness*lap-restoring*h-3*h*h*h+drive;
            // Semi-implicit Euler with exact viscous decay is stable across the
            // declared stiffness range at this fixed, conservative time step.
            nextVelocity_[i]=bounded((velocity_[i]+acceleration*stepSeconds)*
                                    std::exp(-damping*stepSeconds),-15,15);
        }
        for (int y=1; y<rows; ++y) for (int x=1; x<columns; ++x) {
            const int i=y*(columns+1)+x;
            velocity_[i]=nextVelocity_[i];
            height_[i]=bounded(height_[i]+velocity_[i]*stepSeconds,-1.5,1.5);
        }
        ++steps_;
    }
    Field height_{},velocity_{},nextVelocity_{};
    double phase_=0, accumulator_=0;
    std::uint64_t steps_=0;
};

}}

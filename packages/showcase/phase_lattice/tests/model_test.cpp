#include "../source/PhaseLatticeModel.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using Model = synaptome_show_phase_lattice::PhaseLatticeModel;
int main() {
    Model first, second;
    first.reset(1001); second.reset(1001);
    assert(first.boundedState());
    const auto initial = first.stateSignature();
    Model::Controls c;
    for (int i = 0; i < 300; ++i) {
        first.step(1.0f / 60.0f, c);
        second.step(1.0f / 120.0f, c);
        second.step(1.0f / 120.0f, c);
    }
    assert(first.stateSignature() == second.stateSignature());
    assert(first.stateSignature() != initial);
    const auto beforePause = first.stateSignature();
    first.step(0.0f, c); first.step(-1.0f, c); first.step(std::numeric_limits<float>::quiet_NaN(), c);
    assert(first.stateSignature() == beforePause);
    first.reset(1001);
    c.coupling = 0.0f; c.dispersion = 0.0f; c.frequency = 0.5f;
    const float beforePhase = first.phases()[0];
    for (int i = 0; i < 60; ++i) first.step(1.0f / 60.0f, c);
    const float advance = std::fmod(first.phases()[0] - beforePhase + 6.2831853f, 6.2831853f);
    assert(std::abs(advance - 3.14159265f) < 0.001f);
    c.coupling = 12.0f; c.dispersion = 2.0f; c.frequency = 2.0f; c.phaseLagDeg = 90.0f;
    for (int i = 0; i < 1200; ++i) { first.step(1.0f / 60.0f, c); assert(first.boundedState()); }
    assert(first.orderParameter() >= 0.0f && first.orderParameter() <= 1.00001f);
    first.reset(1001); assert(first.stateSignature() == initial);
    first.reset(1002); assert(first.stateSignature() != initial);
    c.coupling = std::numeric_limits<float>::infinity(); c.phaseLagDeg = std::numeric_limits<float>::quiet_NaN();
    first.step(10000.0f, c); assert(first.boundedState());
    std::cout << "Phase Lattice: deterministic seed, fixed-step partition, pause, analytic uncoupled frequency, extreme coupling and bounds passed\n";
}

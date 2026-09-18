#include "../source/WaveTankModel.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using Model = synaptome_show_wave_tank::WaveTankModel;
double energy(const Model& model) {
    double sum = 0.0;
    for (float v : model.values()) sum += v * v;
    for (float v : model.velocities()) sum += v * v * 0.01;
    return sum;
}
int main() {
    Model first, second;
    first.reset(1001); second.reset(1001);
    assert(first.boundedState());
    assert(energy(first) > 0.1);
    const auto initial = first.stateSignature();
    Model::Controls c;
    for (int i = 0; i < 600; ++i) {
        first.step(1.0f / 60.0f, c);
        second.step(1.0f / 120.0f, c);
        second.step(1.0f / 120.0f, c);
    }
    assert(first.stateSignature() == second.stateSignature());
    assert(first.stateSignature() != initial);
    const auto beforePause = first.stateSignature();
    first.step(0.0f, c);
    first.step(-1.0f, c);
    first.step(std::numeric_limits<float>::quiet_NaN(), c);
    assert(first.stateSignature() == beforePause);
    c.waveSpeed = 28.0f; c.damping = 0.05f; c.driveStrength = 12.0f; c.driveFrequency = 3.0f;
    for (int i = 0; i < 2400; ++i) { first.step(1.0f / 60.0f, c); assert(first.boundedState()); }
    c.driveStrength = 0.0f; c.damping = 3.0f;
    const double beforeDecay = energy(first);
    for (int i = 0; i < 1200; ++i) first.step(1.0f / 60.0f, c);
    assert(energy(first) < beforeDecay * 0.05);
    first.reset(1001); assert(first.stateSignature() == initial);
    first.reset(1002); assert(first.stateSignature() != initial);
    c.waveSpeed = std::numeric_limits<float>::infinity();
    c.damping = std::numeric_limits<float>::quiet_NaN();
    first.step(10000.0f, c); assert(first.boundedState());
    std::cout << "Wave Tank: deterministic seed, fixed-step partition, pause, extreme forcing, damping and input bounds passed\n";
}

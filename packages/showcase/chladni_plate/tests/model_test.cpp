#include "../source/ChladniPlateModel.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using Model = synaptome_show_chladni_plate::ChladniPlateModel;
double potential(const Model& model) {
    double sum = 0.0;
    for (const auto& p : model.particles()) {
        const double psi = std::sin(3.0 * 3.14159265359 * p.x) * std::sin(5.0 * 3.14159265359 * p.y)
            + 0.72 * std::sin(5.0 * 3.14159265359 * p.x) * std::sin(3.0 * 3.14159265359 * p.y);
        sum += psi * psi;
    }
    return sum / Model::particleCount;
}
int main() {
    Model first, second;
    first.reset(1001); second.reset(1001);
    assert(first.boundedState());
    const auto initial = first.stateSignature();
    Model::Controls c;
    for (int i = 0; i < 240; ++i) {
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
    const double beforeSettling = potential(first);
    c.agitation = 0.0f; c.phaseRate = 0.0f;
    for (int i = 0; i < 600; ++i) first.step(1.0f / 60.0f, c);
    assert(potential(first) < beforeSettling * 0.15);
    c.modeX = 9.0f; c.modeY = 9.0f; c.attraction = 6.0f; c.agitation = 1.0f; c.phaseRate = 0.8f;
    for (int i = 0; i < 900; ++i) { first.step(1.0f / 60.0f, c); assert(first.boundedState()); }
    first.reset(1001); assert(first.stateSignature() == initial);
    first.reset(1002); assert(first.stateSignature() != initial);
    c.modeX = std::numeric_limits<float>::infinity(); c.agitation = std::numeric_limits<float>::quiet_NaN();
    first.step(10000.0f, c); assert(first.boundedState());
    std::cout << "Chladni Plate: deterministic seed, fixed-step partition, pause, nodal settling, extreme modes and input bounds passed\n";
}

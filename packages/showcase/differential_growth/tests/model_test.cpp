#include "../source/DifferentialGrowthModel.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using Model = synaptome_show_differential::DifferentialGrowthModel;
    Model a, b, other;
    Model::Controls controls;
    a.reset(104729u); b.reset(104729u); other.reset(104730u);
    assert(a.points().size() >= 100 && a.points().size() <= Model::maxNodes);
    assert(a.stateSignature() == b.stateSignature());
    assert(a.stateSignature() != other.stateSignature());
    const auto initial = a.stateSignature();
    for (int i = 0; i < 600; ++i) a.advance(1.0 / 60.0, controls);
    for (int i = 0; i < 300; ++i) b.advance(1.0 / 30.0, controls);
    assert(a.stateSignature() == b.stateSignature());
    assert(a.stateSignature() != initial);
    const auto paused = a.stateSignature();
    a.advance(0.0, controls);
    assert(a.stateSignature() == paused);
    controls.repulsion = 2.0f;
    controls.confinement = 1.0f;
    controls.growthRate = 1.0f;
    for (int i = 0; i < 1800; ++i) a.advance(1.0 / 60.0, controls);
    assert(a.points().size() <= Model::maxNodes);
    for (const auto& p : a.points()) {
        assert(std::isfinite(p.x) && std::isfinite(p.y));
        assert(std::abs(p.x) <= 0.981f && std::abs(p.y) <= 0.981f);
    }
    a.advance(1.0e9, controls);
    assert(a.points().size() <= Model::maxNodes);
    a.reset(104729u);
    assert(a.stateSignature() == initial);
    std::cout << "Differential Growth: deterministic, evolving, bounded, frame-partition invariant\n";
}

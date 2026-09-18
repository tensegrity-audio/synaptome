#include "../source/DendriticCrystalModel.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using Model = synaptome_show_dendritic::DendriticCrystalModel;
    Model a, b, other;
    Model::Controls controls;
    a.reset(130363u); b.reset(130363u); other.reset(130364u);
    const auto initial = a.stateSignature();
    const auto initialCount = a.occupiedCount();
    assert(initialCount > 300 && initialCount < 2500);
    assert(initial == b.stateSignature());
    assert(initial != other.stateSignature());
    for (int i = 0; i < 600; ++i) a.advance(1.0 / 60.0, controls);
    for (int i = 0; i < 300; ++i) b.advance(1.0 / 30.0, controls);
    assert(a.stateSignature() == b.stateSignature());
    assert(a.occupiedCount() > initialCount);
    const auto paused = a.stateSignature();
    a.advance(0.0, controls);
    assert(a.stateSignature() == paused);
    controls.growthRate = 15000.0f;
    controls.adhesion = 1.0f;
    controls.dissolutionRate = 0.1f;
    for (int i = 0; i < 7200; ++i) a.advance(1.0 / 60.0, controls);
    assert(a.cells().size() == static_cast<std::size_t>(Model::width * Model::height));
    assert(a.occupiedCount() >= initialCount && a.occupiedCount() <= a.cells().size() / 2);
    std::size_t occupied = 0;
    for (const auto& cell : a.cells()) {
        assert(std::isfinite(cell.born));
        if (cell.occupied) ++occupied;
        assert(!cell.root || cell.occupied);
    }
    assert(occupied == a.occupiedCount());
    a.advance(1.0e9, controls);
    a.reset(130363u);
    assert(a.stateSignature() == initial);
    std::cout << "Dendritic Crystal: visible initial state, deterministic growth, bounded turnover\n";
}

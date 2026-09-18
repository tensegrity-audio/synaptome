#include "../source/MagneticDipolesModel.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using Model = synaptome_show_magnetic::MagneticDipolesModel;
    Model a, b, other;
    Model::Controls controls;
    a.reset(155921u); b.reset(155921u); other.reset(155922u);
    const auto initial = a.stateSignature();
    assert(a.segments().size() > 100 && a.segments().size() <= Model::maxSegments);
    assert(initial == b.stateSignature());
    assert(initial != other.stateSignature());
    for (int i = 0; i < 600; ++i) a.advance(1.0 / 60.0, controls);
    for (int i = 0; i < 300; ++i) b.advance(1.0 / 30.0, controls);
    assert(a.stateSignature() == b.stateSignature());
    assert(a.stateSignature() != initial);
    const auto paused = a.stateSignature();
    a.advance(0.0, controls);
    assert(a.stateSignature() == paused);
    const auto time = a.time();
    controls.separation = 0.15f;
    controls.twist = -1.0f;
    controls.coupling = 1.0f;
    a.advance(0.0, controls);
    assert(a.time() == time); // Live geometry edits do not reset evolution.
    for (int i = 0; i < 300; ++i) a.advance(1.0 / 30.0, controls);
    assert(a.segments().size() <= Model::maxSegments && !a.segments().empty());
    for (const auto& s : a.segments()) {
        assert(std::isfinite(s.a.x) && std::isfinite(s.a.y));
        assert(std::isfinite(s.b.x) && std::isfinite(s.b.y));
        assert(std::isfinite(s.strength) && s.strength >= 0.0f && s.strength <= 1.0f);
    }
    a.advance(1.0e9, controls);
    a.reset(155921u);
    assert(a.stateSignature() == initial);
    std::cout << "Magnetic Dipoles: deterministic field, finite bounded integration, frame-partition invariant\n";
}

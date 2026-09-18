# Differential Growth

Ordered elastic closed curve with nonlocal repulsion, confinement and bounded edge subdivision.

This source package adds creative content through the existing Element SDK. It owns no audio device, OSC router, MIDI bindings, camera, runtime service implementation, or composition target. The provided Host Mic mapping preset is an optional visible suggestion; preview and apply it from Control & Mapping. Generic controls can also be learned from MIDI. Whole-element opacity belongs to the Console slot.

The default, calm, and energized presets retain the same model. Seed changes and Restart From Seed explicitly restart it. Other control sweeps preserve state. Speed zero and Pause freeze evolution; rendering controls remain live. BPM Sync scales model time relative to 120 BPM and multiplies Speed. Unusually large frame gaps discard excess catch-up time to preserve bounded work.

The pure model is in `source/DifferentialGrowthModel.h`; its public state and deterministic signature support focused non-graphics validation. The SDK adapter uses one batched mesh, a transparent background, and balanced view, matrix and style scopes. Field units are normalized; Stroke Width is in pixels, except the crystal's Cell Coverage, which is a cell fraction.

Run the focused model test with a C++17 compiler:

```sh
c++ -std=c++17 -O2 tests/model_test.cpp -o model_test
./model_test
```

Native model tests establish deterministic finite state, bounded storage and frame-partition equivalence. They do not prove actual OpenGL output or projector frame time.

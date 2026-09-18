# Phase Lattice

Locally coupled oscillators form moving phase domains, synchronization fronts, and glowing interference bands.

The source is self-contained and uses the existing Element SDK bind-only package boundary. No device, MIDI, OSC, runtime, or host ownership is added. Fixed storage and at most 16 fixed 1/120-second model steps per update bound CPU cost; excess elapsed time is dropped rather than caught up in an unbounded loop. Changing a model parameter preserves state. Seed edits explicitly reset it to a new variation. Reseed restarts the current stored seed without changing parameter ownership; edit Seed for a different variation. Paused and zero global/local speed freeze time.

The source Default, Calm, and Energized preset files omit seed/reseed, so applying their values preserves the current simulation. The shipped Browser looks are ordinary catalog definitions: selecting a different look replaces the instance. See [the show guide](../../../docs/show_content_guide.md). The optional Host Mic Motion mapping is suggestion-only and targets a generic model parameter. MIDI and other OSC sources can map the same declared controls through the existing Browser. Source profile conflicts remain visible at mapping apply time.

Run the portable model test with a C++17 compiler, for example:

```sh
g++ -std=c++17 -O2 tests/model_test.cpp -o /tmp/phase_lattice_test
/tmp/phase_lattice_test
```

This model test proves numerical/lifecycle behavior, not production OpenGL rendering or a Windows Release build. Those require the existing host confidence and rehearsal gates.

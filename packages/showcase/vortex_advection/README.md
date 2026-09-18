# Vortex Advection

Eight interacting softened point vortices advect 384 tracers with fixed-length history. The rendering reveals curling counter-rotating eddies and stretched transport filaments.

The fixed simulation storage and 120 Hz steps are bounded. Each update consumes at most 0.1 simulation seconds; excessive elapsed time is discarded instead of accumulating a catch-up stall. Global transport speed zero, local speed zero, or BPM zero while synced freezes motion. Model state continues through every force, color, transform, and preset change. Only the explicitly labeled seed resets state. Performance presets omit seed.

Use the package creator through controlled source registration. No device access, hidden audio bus, file I/O, or host runtime changes are required. Audio motion is a suggestion-only, visible OSC mapping into generic parameters; apply it explicitly in Browser after assigning a slot. The same parameters support ordinary MIDI learn. Shared mic source profiles use relative scale 0.65 to 1.65 and smoothing 0.2, so applying several package suggestions does not silently change the source curve.

The slot owns whole-layer opacity. This package has no duplicate whole-layer visibility or alpha. RGB controls are normalized, angles are degrees, and trail durations are seconds.

Validate: `python tools/validate_layer_packages.py packages/showcase/vortex_advection/layer.package.json`. Real renderer acceptance on the show machine remains required.

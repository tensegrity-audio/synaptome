# Gerstner Ocean

A low-poly ocean sampled from three deep-water Gerstner waves. Each wave has
phase `k dot(position,direction) - phaseTime` and dispersion
`omega = sqrt(9.81 k)`, with `k = 2 pi / wavelength`. The principal wavelength
has secondary ratios 0.57 and 0.31; amplitude weights are 0.62, 0.26, and 0.12.
Directions fan by +37 and -51 degrees. Wave Height sets their total amplitude.
These are stylized world units, not a calibrated ocean forecast.

The trochoidal horizontal displacement is `min(amplitude * steepness, 0.24/k)`
for each component. The sum of horizontal displacement-gradient bounds is at
most 0.72, keeping the surface non-folding across the declared control range.
Facet color derives from actual height, local slope, and the deformed triangle
normal under a fixed directional light. Face indices never choose colors.

## Performing

Default provides a complete turquoise wave surface. Calm reduces wave height
and speed and holds the view still. Energized shortens wavelength and raises
height and steepness. Wave Height, Wavelength, Steepness, and Wave Direction are
continuous live controls. Phase persists through changes and presets; nothing
rebuilds the grid. Only Seed or Action: Reseed changes initial phases.

Optional visible microphone mappings target Wave Height and Steepness. Apply
those suggestions through the existing mapping workflow and calibrate the
source to the show input. All ordinary parameters can use MIDI or OSC; the
model performs no hidden audio reads. Grid Opacity fades only the edge overlay.
The Console slot owns whole-element opacity.

## Rendering and limits

A fixed 40 by 28 cell grid contains 1,189 vertices and 2,240 triangles. Three
wrapped phases bound numerical state over arbitrarily long performances.
Updates clamp elapsed model time to 0.2 seconds; a host stall cannot create
unbounded catch-up work. Phase and surface evaluation are deterministic for
one seed and update sequence. Setup evaluates the full surface immediately.

The renderer uses explicit orthographic mathematical 3D projection, local yaw
and elevation, and back-to-front triangle sorting. It does not retain or alter
the host camera or require a depth attachment. Mesh facets and grid lines are
batched. Grid lines intentionally remain visible as a technical wire overlay,
including overlapped edges at steep view angles. The background is transparent.
This is a bounded layered surface treatment, not a change to the runtime's 3D
camera or lighting system.

BPM Sync multiplies local Speed by transport BPM / 120 and BPM Multiplier.
Zero transport speed freezes simulation and view rotation. State does not
advance in draw. View, matrices, style, depth-test, and face-cull state are
restored after rendering.

## Validation

The native `tests/model_test.cpp` checks seed behavior, deterministic repeated
runs, first-load finite geometry, pause/invalid deltas, bounded wrapped phases,
and 6,000 frames alternating extreme controls. It requires only C++17.
`tests/layer_test.json` supplies the package confidence fixture. Real projector
framing, physical controllers, and GPU output still need target-machine review.

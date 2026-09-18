# Elastic Lattice

A fixed mass-spring membrane with connected vertical degrees of freedom.
Each node accelerates from the four-neighbor spring Laplacian, an anchor force
toward its rest height, cubic restoring resistance, and a travelling Gaussian
force. Viscous damping removes energy. Boundary nodes stay at rest.

The driver moves smoothly around an elliptical path and oscillates three times
per orbit. Driver Frequency controls orbit frequency in Hz; Driver Strength
controls applied acceleration. The resulting waves are the actual evolved
membrane, not a time-displaced noise texture. Facet color follows displacement,
local triangle strain, and face-normal lighting.

## Performing

Default begins with a complete lightly perturbed sheet that develops travelling
ripples. Calm reduces the driver and damps waves more quickly. Energized raises
spring coupling and drive, sustaining stronger interference. Sweep Driver
Strength, Damping Rate, Spring Stiffness, and Height Scale. Height Scale changes
the rendered relief while leaving the underlying simulation untouched.

Presets preserve heights, velocities, and driver phase. Seed and Action: Reseed
are the only reset controls. Optional visible host-mic mappings target Driver
Strength and Height Scale. MIDI and OSC can target the same generic controls;
there are no hidden audio reads or input-specific model parameters. Grid Opacity
controls only the triangulated edge overlay. Whole-element fading belongs to
the Console slot.

## Rendering and limits

The model has a fixed 36 by 28 cell grid, 1,073 nodes, and 2,016 rendered facets.
It uses semi-implicit steps at 240 Hz with exact viscous velocity decay. At most
48 steps run per update; excessive host delays discard extra elapsed time.
Conservative hard guards bound displacement to 1.5 world units and velocity to
15 world units per second even under extreme controls. No memory or mesh
resolution grows with time. Startup uses deterministic nonzero membrane modes,
so the first frame is complete without waiting for the driver.

Geometry is explicitly orthographically projected with element-local yaw and
elevation, then facets are sorted back-to-front. This provides low-poly 3D relief
without changing the shared camera or depending on host depth-buffer setup.
Facet and wire geometry use separate batched meshes; wire edges remain visible
through overlapping portions as a technical overlay. The background remains
transparent for live layering.

BPM Sync multiplies local Speed by transport BPM / 120 and BPM Multiplier.
Transport speed zero freezes all evolution and view rotation. Drawing does not
advance the model. Style, view, matrix, depth-test, and face-cull state are
restored after rendering.

## Validation

`tests/model_test.cpp` tests deterministic seeds, finite startup, pause and
invalid deltas, the 48-step work budget, control-sweep state continuity, explicit
reseed reproducibility, and 6,000 frames at alternating declared extrema. The
pure model builds with C++17 and needs no openFrameworks headers.
`tests/layer_test.json` is the package confidence fixture. Hardware control,
projector framing, and actual GPU pixels still require a show-machine check.

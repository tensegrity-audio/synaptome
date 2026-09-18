# Strange Attractor

A Lorenz system rendered as four luminous 3D ribbon trajectories. The model is
`dx/dt = sigma (y-x)`, `dy/dt = x (rho-z)-y`, `dz/dt = xy-beta z`.
Nearby initial conditions separate naturally; no noise or decorative path is
added. Ribbon color follows instantaneous trajectory speed, age, and projected
depth. The underlying four trajectories and their histories remain available
through the pure C++ model.

## Performing

Start with Default for a complete butterfly-shaped attractor. Calm slows its
motion and retains the full trail. Energized raises sigma and rho, increasing
stretching and turnover. Sweep Rho, Ribbon Width, Trail History, and View Spin.
Sigma, rho, and beta are genuine model coefficients, so large sweeps cause
natural transient responses without clearing history. All three presets keep
the current seed and state. Only Seed changes and Action: Reseed reset it.

Suggested host microphone routes target Rho and Ribbon Width. They are optional
mapping suggestions, visible in the normal mapping workflow after applying.
They do not read audio inside this element. Generic registered controls can
also be learned from MIDI or OSC. Use the Console slot for overall opacity.

## Rendering and limits

The CPU model integrates four trajectories with RK4 at 0.005 model seconds per
step. Each stores a fixed circular history of 1,200 points, equivalent to six
model seconds. A single update performs at most 40 steps. Excess elapsed time
after a stall is discarded rather than producing a delayed catch-up burst.
Setup has a bounded warm-up that fills the histories before the first draw.
Storage and work do not grow with performance duration.

The renderer explicitly projects 3D positions into a fitted orthographic view.
It does not use or mutate the shared camera. View Yaw and View Elevation control
this element-local projection. A batched triangle mesh represents all ribbon
segments, with at most 28,776 vertices. Screen-overlapping ribbons blend as
translucent trails. This is intentionally a trajectory projection rather than
an opaque solid-surface depth renderer. No GPU simulation or new shader is
required. The transparent background allows composition with other elements.

BPM Sync multiplies local Speed by transport BPM / 120 and BPM Multiplier.
Global speed zero or local Speed zero freezes simulation and view rotation.
Rendering does not advance state. Style, view, matrix, depth-test, and face-cull
state are restored after drawing.

## Validation

`tests/model_test.cpp` exercises seeded determinism, pause and invalid delta
handling, finite long runs at both parameter extremes, history continuity under
sweeps, explicit reseeding, and the fixed catch-up budget. Compile with a C++17
compiler; the model has no openFrameworks dependency. `tests/layer_test.json`
is the package confidence fixture. Target-machine visual acceptance is still
required for actual projector framing, MIDI hardware, and GPU behavior.

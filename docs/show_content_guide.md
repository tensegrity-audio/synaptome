# Algorithmic Show Content

This pack adds 12 independent programmatic visual models, with 36 selectable
looks and six effect-look scenes. The host, Runtime, renderer, routing and
existing effect processors are unchanged. Generated registration and build
records attach the new source packages through the existing extension path.

## Get It Running

1. Save your current performance scene and keep your current working build.
2. Fetch and switch to the content branch in your existing repository:

   ```powershell
   git status --short
   git fetch origin
   git switch content/algorithmic-show-pack
   ```

   If the first command lists work you want to keep, commit or stash that work
   before switching. This branch does not require a new openFrameworks setup.
3. Open your existing Synaptome solution, choose **Release / x64**, and use
   **Build Solution**. This is an ordinary incremental application build.
   The new C++ models need to be compiled once. No package installation,
   runtime discovery, DLL loader, or data-generation command is required.
4. Launch the newly built `synaptome/bin/Synaptome.exe` from the existing data
   directory. In the asset Browser, search for the names below under
   **Generative** and assign one to a Console layer.
5. Each model has a baseline name plus **/ Calm** and **/ Energized** looks.
   They are ordinary catalog definitions using the same model implementation.
   Selecting another look replaces that element instance; it is not a live
   preset morph. Sweep its controls to preserve the running simulation.
6. Use the Console layer opacity to mix it with other elements. Most new
   content uses a transparent background. Filled field/mesh surfaces can
   cover earlier layers; lower their slot opacity or layer them first.

The package manifests also preserve the three original preset files for
future authoring. They are exposed here as catalog looks because the existing
package preset Apply command requires a machine-local activation record.
This pack does not modify those activation preferences or add nonfunctional
preset buttons.

## The Twelve Models

| Browser name | What it does | Start with these controls |
| --- | --- | --- |
| Wave Tank | Driven, damped membrane waves form moving interference contours. | Wave Speed, Drive Strength, Damping |
| Chladni Plate | Sand tracers migrate toward standing-wave nodal patterns. | Mode X/Y, Attraction, Agitation |
| Phase Lattice | Locally coupled oscillators form phase domains and traveling color waves. | Coupling, Dispersion, Phase Lag |
| Differential Growth | A closed elastic ribbon grows and folds under local springs and repulsion. | Growth Rate, Repulsion, Confinement |
| Dendritic Crystal | Bounded diffusion-limited growth branches from distributed nuclei with gradual turnover. | Growth Rate, Adhesion, Anisotropy |
| Magnetic Dipoles | Field lines reveal moving superposed magnetic dipoles. | Coupling, Separation, Orbit Rate, Twist |
| Vortex Advection | Tracer histories follow interacting soft-core vortices. | Circulation, Core Radius, Strain, Trail Time |
| Sand Ripples | Conservative wind transport and slope relaxation create faceted dunes. | Transport Rate, Wind Direction, Repose Slope |
| Voronoi Foam | Moving sites and centroid relaxation form cellular walls. | Drift Speed, Repulsion Weight, Wall Width |
| Strange Attractor | Four numerically integrated Lorenz trajectories draw persistent ribbons. | Rho, Sigma, Ribbon Width, Spin |
| Gerstner Ocean | Dispersive trochoidal waves produce a low-poly surface colored by height and slope. | Wave Height, Wavelength, Steepness |
| Elastic Lattice | A spring membrane responds to a traveling force and reveals strain through its facets. | Spring Stiffness, Impulse Strength, Damping |

Sand, Ocean and Elastic Lattice render mathematical 3D relief through an
explicit local projection. Strange Attractor projects a three-dimensional
trajectory. Local tilt/rotation controls frame them; they do not follow the
host camera or require shared depth/camera changes. These are bounded artistic
models, not calibrated scientific solvers.

All new models animate without audio. **Speed = 0** or global transport
speed zero freezes simulation. BPM Sync multiplies local speed by the supplied
tempo and BPM multiplier. Numerical stepping is bounded, so extreme speed or
a long stall drops excess elapsed time instead of running unlimited catch-up.
Continuous force/color controls preserve state. Changing **Seed** explicitly
restarts that model; models exposing **Reseed** label it as an action.

## Sound, MIDI And OSC

There are no hidden audio-bus reads or hardcoded hardware assignments. The
new controls are normal parameter targets. An installed visual in Console
layer 1 uses addresses such as `console.layer1.driveStrength`; the slot
index changes that prefix. Use **Ctrl+E** from the focused Console layer to
jump to its controls, then use the existing MIDI/OSC Learn workflow.

The Browser inspection metadata supplies one optional host-mic mapping
suggestion per look. Assign the visual first, select its mapping suggestion,
preview it, and apply. Preview shows conflicts; do not replace an existing
mapping unintentionally. The shipped suggestions use
`/sensor/host/localmic/mic-bass`. Other existing metrics are available for
manual mapping through Learn:

- `/sensor/host/localmic/mic-level`
- `/sensor/host/localmic/mic-bass`
- `/sensor/host/localmic/mic-mids`
- `/sensor/host/localmic/mic-highs`

Every shipped suggestion uses the same bass source profile across the pack:
relative **Scale**, output **0.65 to 1.65**, smoothing **0.2**, deadband **0.01**.
The bass input range is 0 to 0.18. For manual alternatives, start with 0 to
0.12 for level, 0 to 0.16 for mids, or 0 to 0.10 for highs, retaining the same
relative output profile. These are starting ranges; calibrate against your
actual source. Silent input retains a nonzero model baseline. Parameters
still respect their declared bounds.

The current router shares each source profile between all targets using that
address. Editing one source range affects its other mappings. If Signal
Control publishes `zoom` or another source name, relink to that observed
address. Nothing here selects your audio device or changes OSC receive mode.

Avoid mapping continuous audio to Seed/Reseed. Map continuous model controls,
then save the completed performance scene with its visible mappings.

## Building Scenes Live

Start with one of the six [effect-look scenes](show_effect_looks.md), then
replace its layer 1 source with a new element. The effect stays in layer 2.
Each look supplies complete saved settings for an existing processor; the FX
Browser still lists the established processors.

Try these pairings:

| Scene direction | Source | Effect look |
| --- | --- | --- |
| Organic unfolding | Differential Growth or Magnetic Dipoles | River Prism |
| Growing circuitry | Dendritic Crystal or Phase Lattice | Circuit Etching |
| Nodal typography | Chladni Plate or Wave Tank | Cellular Glyphs |
| Luminous flow | Vortex Advection or Strange Attractor | Phosphor Bloom |
| Faceted landscape | Gerstner Ocean or Sand Ripples | Chromatic Drift, reduced mix |
| Soft structural motion | Elastic Lattice or Voronoi Foam | Tidal Afterimage |

The existing effect chain shares controls by processor type. Two CRT entries
cannot hold independent settings. The six looks include two different CRT
treatments; they are six presets, not six new shader algorithms.

Load a starter scene **before** mapping your controls. Existing whole-scene
loading retires old visual slots and can clear their slot-specific routes,
even when the scene omits a router snapshot. Ordinary visual-to-visual slot
replacement retains route definitions and rebinds matching parameter names.
Save your own performance scene and reapply mappings after loading unrelated
scenes. This is an existing host behavior, recorded here without changing it.

## Before The Show

The Linux validation covers package declarations, generated metadata, native
model behavior, adapter compilation and recorded drawing geometry. It does
not establish Windows linking, real GPU pixels, projector performance,
physical MIDI behavior or audio-device availability.

On the actual show machine:

1. Build Release/x64 and load each planned visual once. Confirm nonblank
   output on the projection window and useful colors/scale.
2. Test the planned knob/fader mappings and audio meter. Confirm the base look
   still works when audio is silent.
3. Save and reload one finished performance scene. Confirm its intended
   elements, effect settings, seed values and mappings return.
4. Run the heaviest intended combination at the final output resolution.
   Warm it for 30 seconds, then observe frame time for at least 60 seconds.
   60 FPS requires about 16.7 ms per frame. Keep headroom; lower effect usage
   or active layer count if the projector machine cannot sustain it.
5. Keep the previously working executable and scene available until the new
   build passes these checks.

For contributor verification:

```powershell
python tools/validate_layer_packages.py packages/showcase
python tools/generate_element_package_registrations.py --check
python tools/generate_showcase_content.py --check
python tools/gen_parameter_manifest.py --check
python tools/validate_configs.py --public-app
python -m pytest -q tests/test_showcase_content.py
```

The portable native runner requires `g++` or `clang++`:

```text
python tools/validate_showcase_native.py --legacy-bench
```

Its SVG output records the actual adapter mesh geometry and vertex colors
on the CPU. It is a useful visual review surface, not a capture of the app's
OpenGL renderer. Package-owned native tests document each model's specific
numerical properties.

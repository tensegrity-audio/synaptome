# Cosmic Generative Layers Roadmap

Status: Historical/superseded show-development record; reviewed 2026-07-18.
Cosmos Formation became a single cosmic-web/universe-map layer, so this file no
longer owns active priority. It preserves visual intent and a possible resume
sequence only. Any further tuning or new cosmic layer requires a focused
Project Ops request after the pre-media safety gate.
Created: 2026-06-18.
Last detailed pass: 2026-06-21.

Priority and activation are owned by [`roadmap.md`](roadmap.md).

## Cosmos Formation / Cosmic Web Brief

The current flagship cosmic layer should read as a **DESI-style cosmic web / universe map**: dense nodes, branching filaments, dark voids, hierarchical clustering, and subtle local swirl near nodes only. It should not look like a clean annulus, a galaxy disk, a visible grid, or particles orbiting obvious centers. The audience-facing composition should be legible even in a still frame: sparse filaments connect luminous intersections, voids remain dark, and particles sit inside the structure as texture rather than defining the whole image.

Under the hood, the layer is now a **persistent web graph painted into a soft growth field**. Distributed web nodes and sparse nearest-neighbor filament edges define the large-scale topology. A hidden field stores dark density, gas density, stellar density, temperature, spin, bloom, compression, ridge strength, node strength, voidness, and filament direction. Matter samples that field for gradient collapse, ridge-following, local node swirl, damping, heat, color, and glow.

The most important sensor-driven controls are:

| Input | Visual Meaning | Key Parameters |
| --- | --- | --- |
| `mic-level` | Overall energy, gas visibility, glow lift, slow intensity changes. | `audioAmount`, `matterGlow`, `haloAlpha` |
| `mic-bass` | Large-scale bloom pressure, halo breathing, core/cluster compression and release. | `bassExpansion`, `expansionForce`, `voidPressure` |
| `mic-mids` | Filament shimmer, ridge-following turbulence, subtle node eddies. | `midsTurbulence`, `turbulence`, `clusterSwirl`, `shear` |
| `mic-highs` | Starbirth sparkle, hot knots, fine glow changes. | `highsSparkle`, `matterGlow`, `coolingRate` |
| `mic-peak` | Local ignition events and pressure ripples, not repeated full resets after the initial bloom. | `peakBangThreshold`, `peakImpulse`, `shockwaveAlpha` |
| Waveform | Subtle radial/gas warping and band ripple. | `waveformWarp` |
| Beat/BPM | Gentle cluster-local pulses and bloom accents. | `beatImpulse`, `shockwaveSpeed`, `shockwaveWidth` |
| Manual bang/reseed | Start a new universe or generate a new arrangement. | `bang`, `reseed`, `seed` |

The key user-editable controls should stay grouped around show operation:

- **Lifecycle:** `autoAdvance`, `formationAge`, `formationTime`, `expansionRate`, `bang`, `reseed`, `seed`.
- **Matter and scale:** `particleCount`, `clusterCount`, `radius`, `originRadius`, `matterSize`, `matterGlow`.
- **Formation behavior:** `gravity`, `gravityDelay`, `clusterSpread`, `clusterDrift`, `clusterSoftness`, `clusterSwirl`, `shear`, `voidPressure`, `turbulence`, `coolingRate`.
- **Glow and atmosphere:** `haloAlpha`, `haloRadius`, `trailAlpha`, `trailThickness`, `bgAlpha`, `shockwaveCount`, `shockwaveAlpha`, `shockwaveWidth`, `shockwaveSpeed`, `pressureAmount`.
- **Audio response:** `audioAmount`, `bassExpansion`, `midsTurbulence`, `highsSparkle`, `waveformWarp`, `peakBangThreshold`, `peakImpulse`, `beatImpulse`, `audioSmoothing`.
- **Palette:** `backgroundColor`, `hotColor`, `matterColor`, `coolColor`, `clusterColor`, `waveColor`, plus the individual RGB controls.

The cosmology inspiration is stylized but real. Modern structure formation starts with tiny density differences in the early universe; gravity amplifies those differences, matter condenses into dark-matter halos, gas cools, angular momentum produces rotating disks, and mergers build larger galaxies and clusters over time. The layer borrows that hierarchy: broad invisible basins act like halos, density fields condense before stars become visually dominant, spin creates disk/spiral behavior, bloom stands in for star formation, and cluster drift/merger-like eddies keep the system alive. It is not a numerically accurate cosmological simulation; it is a VJ instrument informed by cosmological ideas: expansion, cooling, density contrast, angular momentum, hierarchical clustering, and starbirth.

This roadmap pins down the cosmic visual track for Synaptome. It prioritizes layers that can become useful festival material quickly, while still leaving room for deeper simulation work after the first show-ready pack lands.

The core strategy is:

1. Treat **Cosmos Formation / Cosmic Web** as the flagship cosmic layer and primary show asset.
2. Build the cosmic track around web nodes, sparse filaments, dark voids, field glow, and small embedded matter texture.
3. Keep galaxy-disk/super-galaxy behavior demoted unless a future separate mode is explicitly requested.
4. Add helper libraries only when they accelerate field texture, gas, or particle throughput without changing the layer into a science project.
5. Use Big Bang / Supernova and Pulsar / Magnetar as supporting layers after the flagship cosmic-web look is polished.

## Runtime Assumptions

Synaptome can already support dynamic cosmic layers through:

- Audio scalar routes: `mic-level`, `mic-peak`, `mic-bass`, `mic-mids`, `mic-highs`.
- External waveform packets: `/sensor/host/<source>/waveform` with 64, 128, or 256 samples.
- Shared `AudioAnalysisBus` snapshots for layers that want direct audio/waveform reads.
- Browser, MIDI, OSC, and sensor mappings into registered layer parameters.
- Existing generative foundations: flow fields, Perlin noise, oscilloscope modulation, agent fields, flocking, Game of Life, grid/geodesic geometry, and post effects.

The strongest near-term cosmic layers should use fields, bloom, opaque star nodes, glow, palette drift, waveform deformation, and beat-triggered state changes rather than full scientific simulation.

## Prioritized Delivery Order

| Priority | Deliverable | Type | Why First | Dependency |
| ---: | --- | --- | --- | --- |
| P1 | Cosmos Formation / Cosmic Web | Flagship layer | Current direction; dense nodes, branching filaments, voids, and audio-reactive local ignition. | Existing `CosmosFormationLayer` |
| P2 | Big Bang / Supernova Transitions | Transition layer / mode | Best drop/impact support once the cosmic-web base reads correctly. | Cosmos pressure-wave and bloom logic |
| P3 | Pulsar / Magnetar | Oscilloscope-derived or new layer | Fast rhythmic contrast to the organic super-galaxy layer. | Existing oscilloscope concepts |
| P4 | Black Hole / Wormhole | New shader/mesh layer | High drama, higher custom rendering cost. | Basic cosmic pack complete |
| Deferred | Cosmic Preset Pack | Existing-layer presets/scenes | Useful fallback, but not the current direction. | None |
| Folded In | Cosmic Web / Filament Graph | Core behavior | Now implemented directly inside `CosmosFormationLayer`. | Live tuning |
| P5 | Solar System / Orrery | Simulation layer | Elegant but less audio-dynamic; better as later orbital instrument. | REBOUND spike |
| P6 | Drake Signal Map | Data-viz / graph layer | Conceptually strong, but lower immediate VJ impact. | Graph primitives if Cosmic Web resumes |
| P7 | N-Body Star Cluster | Simulation layer | Interesting but needs careful performance and stability limits. | REBOUND spike |

## Decision Matrix

| Layer | Show Impact | Audio Dynamism | Sensor Dynamism | Implementation Cost | Priority |
| --- | --- | --- | --- | --- | --- |
| Cosmos Formation / Cosmic Web | Very High | High | High | Medium-High | Build and polish first |
| Big Bang / Supernova | High | Very High | Medium | Medium | Build as transition/support |
| Pulsar / Magnetar | Medium-High | High | High | Low-Medium | Build third |
| Black Hole / Wormhole | Very High | Medium-High | Medium | High | Spike after flagship layer |
| Cosmic Web / Filament Graph | High | High | High | Medium | Folded into flagship layer |
| Hyperspace / Meteor Tunnel | Medium | Medium | Medium | Low | Build only if transitions are needed |
| Solar System / Orrery | Medium | Low-Medium | High | Medium-High | Defer |
| Drake Signal Map | Medium | Medium | Medium | Medium | Defer |
| N-Body Star Cluster | Medium-High | Medium | Medium | High | Defer |

## Shared Audio Mapping Recipe

Use this recipe consistently across cosmic layers so the operator can move between scenes without relearning the instrument.

| Audio/Sensor Input | Default Visual Meaning |
| --- | --- |
| `mic-level` | Overall energy, gas visibility, glow, density, mild speed lift. |
| `mic-peak` | Momentary local events: ignition, flare, pressure wave, reseed/bang only when mapped. |
| `mic-bass` | Bloom pressure, halo breathing, radius, core flare, large-scale expansion/compression. |
| `mic-mids` | Curl, shear, eddies, turbulence, orbital wobble, field strength. |
| `mic-highs` | Starbirth, sparkle, hot-knot shimmer, edge shimmer, fine noise. |
| waveform | Shape deformation, band ripple, radial/gas displacement. |
| x/y sensor | Optional bloom/ignition point, camera orbit, magnetic center. |
| knob/slider | Formation age, simulation phase, palette, global intensity. |

## Deferred Milestone: Cosmic Preset Pack

Decision: skip this for now and start with custom cosmic layers.

Goal if resumed: produce useful cosmic content with current layers before adding dependencies.

Deliverables:

- **Nebula Bed** from `PerlinNoiseLayer`.
- **Proto Super-Galaxy Gas** from `FlowFieldLayer`.
- **Pulsar Scope** from `OscilloscopeLayer`.
- **Star Swarm** from `FlockingLayer` or `AgentFieldLayer`.
- One saved scene that loads the strongest three together.

Implementation tasks:

- Create preset JSON assets or scene defaults with cosmic palettes.
- Map bass/mids/highs/peak into existing parameters through Browser/OSC mappings.
- Set defaults for dark backgrounds, bright cyan/magenta/white trails, and slow drift.
- Confirm each preset remains active with no audio using time, BPM, or auto motion.

Acceptance:

- At least three cosmic looks run from current code.
- Router UDP host audio visibly changes each look.
- The operator can control opacity, speed, palette, and intensity from Browser/MIDI/OSC.
- Presets can be saved and reloaded as a scene.

## Milestone 1: Low-Risk Procedural Helpers

Goal: add small helper libraries only if they reduce implementation time and keep the build stable.

Recommended helpers:

| Helper | Use | Decision |
| --- | --- | --- |
| FastNoiseLite | Domain-warped fields, gas texture, bloom variation, turbulence. | Optional; adopt only if the current noise tools become limiting. |
| delaunator-cpp | Cosmic web triangulation and constellation filaments. | Defer; not useful for the no-filaments super-galaxy brief. |
| REBOUND | Solar system and bounded N-body later. | Defer until P5/P7 spike. |
| ofxFlowTools | Fluid nebula/aurora-style gas and pressure/bloom overlays. | Spike only after the CPU growth-field look is approved. |
| GPU particle addon | Very high particle counts and shader-side rendering. | Defer until CPU layer hits frame-rate or density limits. |

Implementation tasks:

- Vendor helpers under an explicit third-party boundary.
- Add a tiny wrapper for deterministic seeds and parameter-friendly configuration.
- Keep new helpers optional for the flagship layer if integration slips.
- Document helper ownership and build notes in the roadmap or dev docs.

Acceptance:

- Helper code builds in the current Visual Studio/openFrameworks project.
- Cosmos Formation can run with deterministic seeded growth-field motion.
- If a helper fails to integrate quickly, the custom layer still has a fallback path.

## Milestone 2: Cosmos Formation / Super-Galaxy Layer

Priority: P1.

Goal: make `CosmosFormationLayer` the show-ready flagship cosmic layer by centering the visual around a soft growth field, opaque star cores, bloom, swirl, cooling, and galaxy-cluster settling.

Visual behavior:

- A hot origin blooms into a diffuse gas field.
- The gas field curls into broad super-galaxy bands with no visible grid.
- Bright knots condense into nested galaxy-like clusters.
- Star nodes are fully opaque, with glow and size carrying energy variation.
- The system settles over time instead of endlessly bouncing around.
- Audio creates local ignition, pressure, shimmer, and halo breathing without repeatedly resetting the whole universe.
- No filaments or connective graph lines are visible in the default look.

Core parameters:

- `visible`
- `alpha`
- `particleCount`
- `clusterCount`
- `radius`
- `originRadius`
- `autoAdvance`
- `formationAge`
- `formationTime`
- `expansionRate`
- `expansionForce`
- `gravity`
- `gravityDelay`
- `clusterSpread`
- `clusterDrift`
- `clusterSoftness`
- `clusterSwirl`
- `shear`
- `voidPressure`
- `turbulence`
- `coolingRate`
- `matterSize`
- `matterGlow`
- `haloAlpha`
- `haloRadius`
- `trailAlpha`
- `trailThickness`
- `shockwaveCount`
- `shockwaveAlpha`
- `shockwaveWidth`
- `shockwaveSpeed`
- `pressureAmount`
- `bang`
- `reseed`
- `seed`
- `backgroundColor`, `hotColor`, `matterColor`, `coolColor`, `clusterColor`, `waveColor`

Audio/sensor mappings:

- bass -> bloom pressure, halo breathing, large-scale expansion, `bassExpansion`, `voidPressure`
- mids -> curl, eddies, turbulence, `midsTurbulence`, `clusterSwirl`, `shear`
- highs -> starbirth, hot knots, glow intensity, `highsSparkle`, `matterGlow`
- peak -> local ignition/pressure pulse, `peakImpulse`, `shockwaveAlpha`
- waveform -> subtle radial/gas warping, `waveformWarp`
- knob/slider -> `formationAge`, `formationTime`, `clusterSoftness`, `audioAmount`
- button -> `bang`, `reseed`

Implementation slices:

1. Done: split the hidden growth field into dark/gas/stellar/temperature/spin/bloom/compression channels.
2. Done: add irreversible star formation so mature stellar regions persist after gas cools.
3. Done: route beats and peaks through local ignition pulses instead of repeated full-universe resets.
4. Done: keep star cores fully opaque and drive energy through glow, size, color, and bloom.
5. Done: add a simple scale factor and cosmic temperature so early formation reads compressed/hot and mature formation reads larger/cooler.
6. Next: tune defaults against live output so the field reads as a super-galaxy from across the room.
7. Next: add one low-intensity and one high-intensity scene recipe.

Acceptance:

- Reads as a super-galaxy or proto-galaxy cluster, not generic particles.
- No visible cellular grid, no connective filaments, and no static attractor dots.
- Opaque stars remain visible over diffuse gas.
- Formation age is playable live as a lifecycle control.
- Audio changes the bloom/swirl/starbirth without destroying the settled structure.
- Layer loads from Browser, saves/reloads in a scene, and runs without audio.

## Milestone 3: Super-Galaxy Polish And Hierarchy

Priority: P1 follow-up.

Goal: deepen the flagship layer after the core look is approved, focusing on hierarchy, mergers, and show control rather than adding a separate galaxy layer.

Visual behavior:

- Larger halos contain smaller galaxy knots.
- Local starbirth waves move through the field as hot blooms.
- Galaxy knots drift, shear, and occasionally create merger-like tidal arcs without drawing filaments.
- The mature state has calmer rotation and clearer cluster identity.
- The operator can push between "diffuse bloom", "spiral organism", and "clustered mature cosmos" modes.

Candidate controls:

- `fieldDensity`
- `fieldBloom`
- `fieldDiffusion`
- `starbirthRate`
- `spiralBandStrength`
- `haloBreathing`
- `mergerActivity`
- `settlingAmount`
- `gasVisibility`
- `galaxyKnotScale`
- `matureClusterStrength`

Audio/sensor mappings:

- bass -> halo breathing, bloom-front pressure, merger compression
- mids -> spiral shear, local eddy strength, band wobble
- highs -> starbirth rate and hot-knot shimmer
- peak -> localized ignition, not a global reset
- waveform -> band ripples and gas warp
- x/y sensor -> optional bloom center, camera/field bias, or local ignition point

Implementation slices:

1. Decide which candidate controls are worth making public parameters.
2. Add visual hierarchy: halos, galaxy knots, and starbirth substructure.
3. Add mode-friendly defaults for diffuse, swirling, and mature states.
4. Add optional localized ignition from x/y sensor or mapped control.
5. Profile CPU cost before considering GPU particle or shader migration.
6. Document final operator recipes for festival use.

Acceptance:

- The layer has at least three distinct playable modes without changing code.
- Mature states stay alive but do not look chaotic.
- Audio/sensor mappings feel intentional and repeatable.
- Performance remains stable at default particle and field settings.

## Milestone 4: Big Bang / Supernova Layer

Priority: P2.

Goal: build a high-impact transition/drop layer that can either stand alone or feed the Cosmos Formation layer's bloom and pressure-wave language.

Visual behavior:

- A point or small core detonates into particles, rings, and shockwaves.
- Debris expands, curls, and can collapse back inward without becoming the default super-galaxy motion model.
- Peak or button input can trigger a new detonation.
- Bass controls shockwave scale.
- Highs control fragmentation and sparkle.

Core parameters:

- `visible`
- `alpha`
- `originX`
- `originY`
- `particleCount`
- `expansionSpeed`
- `shockwaveCount`
- `shockwaveWidth`
- `gravityPullback`
- `drag`
- `curlAmount`
- `fragmentation`
- `lifespan`
- `repeatMode`
- `beatQuantize`
- `collapseAmount`
- `flashAmount`
- `trailDecay`
- `colorRamp`
- `reseed`

Audio/sensor mappings:

- peak -> trigger detonation
- bass -> `shockwaveWidth`, expansion radius
- mids -> `curlAmount`, debris turbulence
- highs -> `fragmentation`, sparkle
- x/y sensor -> explosion origin
- button -> manual detonate

Implementation slices:

1. Particle or field-burst system with deterministic reseed.
2. Shockwave ring mesh/line renderer.
3. Trigger state machine: idle, expanding, dissipating, collapse.
4. Audio peak detector with cooldown.
5. Beat-quantized trigger option.
6. Catalog asset and transition scene preset.
7. Optional handoff recipe into Cosmos Formation after the burst.

Acceptance:

- Works as a drop/transition layer over other visuals.
- Has clear manual and audio-triggered modes.
- Avoids repeated peak retriggers through cooldown/quantize.
- Can fade cleanly after the event.

## Milestone 5: Pulsar / Magnetar Layer

Priority: P3.

Goal: produce a rhythmic cosmic layer with beams, magnetic field lines, and beat-locked pulses.

Visual behavior:

- Rotating beams sweep out from a star core.
- Field lines bend and shimmer.
- Audio widens, brightens, and jitters the beam.
- Works either as a new layer or as an `OscilloscopeLayer` derivative/preset if that is faster.

Core parameters:

- `visible`
- `alpha`
- `beamCount`
- `rotationSpeed`
- `pulseWidth`
- `beamLength`
- `fieldLineDensity`
- `fieldBend`
- `fieldNoise`
- `coreGlow`
- `strobeIntensity`
- `historySize`
- `decay`
- `colorR`, `colorG`, `colorB`
- `bgColorR`, `bgColorG`, `bgColorB`

Audio/sensor mappings:

- bass -> `pulseWidth`, `coreGlow`
- mids -> `fieldBend`, `fieldNoise`
- highs -> shimmer and beam edge flicker
- peak -> strobe flare
- x/y sensor -> magnetic axis tilt

Implementation slices:

1. Decide preset-derived versus new layer.
2. Beam renderer with rotation and decay history.
3. Curved field-line renderer.
4. Audio-driven core/beam modulation.
5. Catalog asset and pulse scene preset.

Acceptance:

- Can lock visually to BPM or free-run.
- Beam shape remains readable at high speed.
- Audio affects pulse width and shimmer without becoming strobe-only.

## Milestone 6: Black Hole / Wormhole Spike

Priority: P4.

Goal: determine whether a black-hole layer should join the first pack or wait.

Spike behavior:

- Accretion disk rings orbit a dark center.
- Lensing-style radial distortion or ring compression suggests gravity.
- Starfield streaks or disk particles fall inward.

Spike tasks:

- Prototype disk/ring mesh without external dependencies.
- Test a simple screen-space distortion or fake lensing pass.
- Map bass to lens pulse, mids to disk turbulence, highs to particle sparks.
- Estimate performance and integration complexity.

Decision gate:

- Promote if the fake lensing reads clearly and performs well.
- Defer if it needs a larger shader/post-effect refactor.

## Deferred Simulation Work

These ideas stay out of the first delivery lane unless a specific show need appears.

| Layer | Why Deferred | Resume Condition |
| --- | --- | --- |
| Solar System / Orrery | Beautiful but less audio-reactive. | Need a calmer orbital layer or REBOUND integration spike. |
| N-Body Star Cluster | Stability/performance risks. | REBOUND spike proves bounded simulation is controllable. |
| Cosmic Web / Filament Graph | Conflicts with the no-filaments super-galaxy brief. | A separate artist/set explicitly wants connective web lines. |
| Drake Signal Map | Conceptually rich but lower immediate visual impact. | Graph primitives are resumed for a separate data-viz layer. |
| Scientifically Accurate Galaxy Formation | Accuracy work does not improve show value enough. | Only revisit for educational/science venue needs. |

## Validation And Smoke Checks

For each new cosmic layer:

- Loads from Browser catalog.
- Registers stable parameters with useful ranges.
- Can be assigned to a Console slot.
- Saves and reloads through a scene.
- Responds to local mic and Signal Control Router UDP host audio.
- Still animates without audio.
- Handles stale or empty `AudioAnalysisBus` snapshots.
- Does not require a second process to bind an already-used port.
- Has one default scene or preset recipe.
- Has at least one low-intensity and one high-intensity operating mode.

Suggested validation commands:

```powershell
python tools\layer_catalog_regression.py --check
python tools\gen_parameter_manifest.py --check
python tools\validate_parameter_targets.py --strict --contract-fixtures
python tools\validate_signal_control_receive_contract.py --check
```

## Preserved Resume Sequence

This is not an active backlog. If cosmic work is promoted again:

1. Polish **P1 Cosmos Formation / Super-Galaxy** until it reads clearly as blooming, swirling, and settling into galaxies/clusters.
2. Tune the default preset and scene recipes for low-intensity, high-intensity, and mature-cluster modes.
3. Add only the user-facing controls needed for live operation; keep the hidden growth-field internals private unless they need mapping.
4. Spike FastNoiseLite, ofxFlowTools, or GPU particles only if the current CPU field cannot deliver the desired texture or density.
5. Implement **P2 Big Bang / Supernova** as a transition/support layer once the flagship look is approved.
6. Convert **P3 Pulsar / Magnetar** from oscilloscope ideas into either a preset or a small custom layer.
7. Resume Cosmic Web or preset-pack work only if a separate show/look specifically calls for it.

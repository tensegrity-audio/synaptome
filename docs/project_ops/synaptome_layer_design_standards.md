# Synaptome Layer Design Standards

_Started: 2026-06-24_

## Purpose

This is the canonical best-practices document for Synaptome visual layers. It
covers layer grouping, layer boundaries, modes, parameter naming, Browser
sections, navigation behavior, parameter value formats, OSC pre-mapping,
scientific grounding, and validation.

Use this document when creating a new layer, deciding whether a feature should
be a mode or a separate layer, exposing parameters, or changing Browser
organization.

The goal is to prevent a pile of weak novelty layers. Related simulations
should be combined when they share the same state model, but kept separate when
they use meaningfully different algorithms, data structures, or scientific
metaphors.

Synaptome layers should be algorithmically driven and grounded in real
simulation families wherever possible. A visual effect is strongest when the
user can feel the behavior changing because the model changed, not because
decoration was added on top.

This document owns the layer-authoring contract: what a good layer declares,
how its public controls are named, and what behavior it must preserve. Package
loading architecture, generated registration, installable extensions, public
SDK boundaries, and bench implementation details are split between
`docs/architecture/synaptome_layer_system_roadmap.md` and
`docs/architecture/synaptome_artist_sdk.md`. Active debt and implementation
follow-up live in the owning roadmaps and Project Ops request artifacts;
`docs/contracts/contract_gaps.md` is only an index to those roadmaps.

## Source Of Truth

Layer design should resolve conflicts in this order:

1. The layer's scientific or algorithmic model.
2. The live performance workflow.
3. Stable parameter IDs and scene compatibility.
4. Browser usability.
5. Rendering style.

Rendering should reveal the model. It should not replace it.

## How Layers Need To Evolve

The new direction for Synaptome is not "more visual effects." It is a
layer-based performance system where each layer is inspectable, controllable,
packageable, preset-aware, mappable, and eventually testable in isolation.

Existing layers should gradually move from handcrafted runtime islands toward
self-describing creative modules.

| Old Pattern | Target Pattern |
| --- | --- |
| A layer is mostly defined by C++ behavior and a loose JSON catalog entry. | A layer declares its public identity, parameters, options, presets, mappings, media, source-registration references, and test expectations. |
| Parameters are whatever the implementation happened to register. | Public parameters are deliberate, stable suffixes with labels, ranges, units, defaults, descriptions, and deprecation policy. |
| Audio, OSC, MIDI, or sensor behavior is hidden in layer code. | External control arrives through visible mapping rows that target generic layer parameters. |
| Similar algorithms become many novelty layers. | Related behavior is grouped; separate layers are reserved for genuinely different state models. |
| Modes sometimes restart or replace the simulation. | Modes preserve state and remain safe for live performance. |
| Presets are scenes or ad hoc defaults. | Layer presets are named bundles of layer-local parameter values. |
| Browser behavior depends on instantiating or poking the layer. | The Browser can inspect package metadata first, with runtime setup used only when needed. |
| Testing requires launching the full app and manually inspecting behavior. | Static checks and, later, a single-layer bench prove the layer can configure, register, update, and draw safely. |

For new layers, design for the target pattern from the start. For existing
layers, migrate only when touching them for real work; do not churn stable
scene IDs or parameter IDs just to make a layer look newer.

The migration priority is:

1. Keep public IDs stable.
2. Clarify the layer's state model and whether variants are layers, modes, or
   presets.
3. Move public controls toward the shared parameter vocabulary.
4. Move source-specific reactivity into visible mappings.
5. Add package metadata, presets, option metadata, and validation fixtures.
6. Compare package declarations against runtime registration before promoting
   them into canonical contracts.

This is how layers match Synaptome's larger direction: the algorithm remains
expressive and openFrameworks-native, but the public surface becomes data-driven
enough for the Browser, scenes, mappings, manifests, packages, and tests to
trust it.

## Core Distinction

### Layer Group

A **layer group** is a family of related algorithms that answer a similar
question.

Examples:

- **Collective Motion** asks how many agents move together through a field.
- **Adaptive Trail** asks how paths, routes, and transport networks emerge over
  time.
- **Planetary / World Models** asks how terrain, orbital systems, atmospheres,
  and environmental strata evolve.

Groups are mostly organizational. They help the Browser, documentation, preset
system, and parameter naming stay coherent.

### Layer

A **layer** is a distinct simulation scene with its own stable state model.

A layer deserves to be separate when changing into it would require replacing
the core algorithm, reseeding the scene, or rebuilding the major data
structures.

Examples:

- **Schooling** and **Murmuration** both live under Collective Motion, but they
  should be separate layers if one is radius-zone based and the other is
  topological-neighbor based.
- **Ant Tunnels**, **Physarum Particles**, and **Flow Network** all live under
  Adaptive Trail, but they should be separate layers because they use different
  trail, particle, graph, or flow mechanics.
- **Mountain Island** and **Solar Orrery** both use low-poly 3D visual language,
  but one is an uplift/terrain/atmosphere model while the other is an orbital
  model.

### Mode

A **mode** is a live-controllable variation inside the same layer state.

A mode should be safe to toggle or animate without reseeding the whole scene. It
changes behavior, emphasis, or force weighting while preserving the
simulation's identity.

Examples:

- Schooling with predator pressure on/off.
- Convergence vs divergence.
- Exploration vs exploitation.
- Attraction vs repulsion.
- Calm, stressed, and disrupted variants of the same swarm.

If changing the option requires replacing the simulation engine, it is not a
mode. It is a separate layer.

## Layer Versus Mode Test

Use this decision test:

| Question | If yes | If no |
| --- | --- | --- |
| Can the current agents, fields, trails, mesh, or graph continue naturally after the change? | Mode | Layer |
| Does the state representation stay mostly the same? | Mode | Layer |
| Would live performance benefit from sweeping this value continuously? | Mode/parameter | Layer/preset |
| Would switching it make the previous frame semantically invalid? | Layer | Mode |
| Does the algorithm come from a different scientific model family? | Usually layer | Usually mode |

Risky controls should become scene changes, layer swaps, or crossfades between
two running simulations.

## Mode Versus Layer Standards Log

These standards are the implementation contract for deciding whether a change
belongs in a live mode, a parameter, a preset, or a separate layer.

### Standard 1: State Continuity

A mode must preserve the current simulation state.

Allowed as a mode:

- Changing force weights while agents keep their positions and velocities.
- Turning predator pressure on while prey and predator agents already exist.
- Sweeping convergence into divergence without clearing the current field.
- Increasing pheromone evaporation while the trail field keeps its history.
- Moving a Physarum particle layer from exploration toward exploitation.

Requires a separate layer or scene:

- Replacing a metric-radius schooling model with topological murmuration.
- Replacing graph-walking ant agents with continuous Physarum particles.
- Replacing a trail field with a graph conductivity network.
- Replacing a terrain height field with an orbital system.
- Changing the coordinate space or embedding basis that defines the layer's
  state.

If the previous frame cannot be interpreted as valid input for the next frame,
the change is not a mode.

### Standard 2: State Representation

A mode can change behavior, but it cannot change what the layer fundamentally
stores.

Examples:

| State Representation | Valid Mode Changes | Separate Layer Changes |
| --- | --- | --- |
| Agents with position and velocity | predator pressure, convergence, divergence, noise, speed | particle trail field, orbital bodies, mesh terrain |
| Continuous trail field | deposit, decay, diffusion, exploration, exploitation | graph edge conductivity, topological flocking |
| Discrete graph routes | pheromone weight, evaporation, route cost, redundancy | continuous chemotaxis particles |
| Height field / mesh terrain | uplift, erosion, water level, cloud density | solar-system orbital mechanics |

### Standard 3: Scientific Model Family

When the scientific model family changes, prefer a separate layer.

Examples:

- Boids/Couzin schooling and topological starling murmuration both belong in
  **Collective Motion**, but they should remain separate layers when their
  neighbor rules differ.
- Ant Colony Optimization and Physarum particle models both belong in
  **Adaptive Trail**, but they should remain separate layers because pheromone
  graph walking and chemotactic particle sensing are not the same algorithm.
- Flow-network Physarum should be separate from particle Physarum because edge
  conductivity and particle trail deposition store different state.

Exception: a mode may activate a secondary force from the same family if the
base state remains valid. Predator pressure inside Schooling is acceptable
because pursuit/evasion is an added steering force over the same agents.

### Standard 4: Live Performance Ergonomics

A mode should be useful as a live control.

Good live modes:

- `predatorEnabled`
- `converge`
- `diverge`
- `explore`
- `exploit`
- `grow`
- `prune`
- `efficient`
- `redundant`

Poor live modes:

- An algorithm selector that changes what state means.
- A hidden reseed disguised as a toggle.
- A mode where the current scene disappears or restarts unexpectedly.
- A mode whose only effect is a color or opacity change.

Color, opacity, and material changes are rendering parameters or presets. They
are not model modes unless they expose a meaningful simulation state.

### Standard 5: Reseed Semantics

A mode must not require reseeding.

The following actions may reseed, but should be explicit:

- `reseed`
- changing `agentCount`
- changing field resolution
- changing source data or coordinate embedding
- loading a different layer or scene

The following actions should not reseed:

- predator on/off
- convergence/divergence
- exploration/exploitation
- trail decay/diffusion/deposit
- pressure, force, noise, opacity, color, and time-scale changes

If a live control secretly clears history, it should be renamed as an action,
not presented as a mode.

### Standard 6: Manifest Ownership

Layer asset manifests should separate non-live model identity from live modes.

Use root-level manifest fields for non-live identity:

```json
{
  "layerGroup": "Collective Motion",
  "model": "schooling",
  "stateModel": "metric-radius agents with optional predator pressure"
}
```

Use `defaults.mode` only for the live behavior regime inside that model. Do not
use `mode` as a hidden algorithm selector.

Use `modes` metadata to describe the stable live choices:

```json
{
  "modes": [
    { "id": "converge", "label": "Converge", "kind": "live" },
    { "id": "diverge", "label": "Diverge", "kind": "live" }
  ]
}
```

Stable rule:

- `model` chooses the layer's algorithm.
- `defaults.mode` chooses the initial live regime.
- `modes[]` documents the live regimes.
- Presets may bundle many parameter values, but they do not redefine the model.

### Standard 7: Browser And Catalog Grouping

The Browser should present scientific families without collapsing distinct
algorithms into one asset.

Recommended catalog shape:

```text
Generative
  Collective Motion
    Schooling
    Murmuration
  Adaptive Trail
    Ant Tunnels
    Physarum
    Slime Mold
```

The high-level category remains broad, such as `Generative`. The layer group
names the scientific family. The layer label names the algorithmic scene.

Target organization should allow the Browser tree to be driven by the layer
folder tree:

```text
layers/
  Generative/
    Collective Motion/
      Schooling/
        layer.package.json
      Murmuration/
        layer.package.json
    Adaptive Trail/
      Ant Tunnels/
        layer.package.json
      Physarum/
        layer.package.json
  Media/
    Webcam/
      layer.package.json
  HUD/
    Sensors/
      layer.package.json
```

In that model, moving a layer package folder changes where it appears in the
Browser. It must not change the stable asset identity used by scenes, mappings,
presets, or manifests.

Folder-derived organization rules:

- Relative folder path may supply default `category`, `layerGroup`, and Browser
  nesting when the package manifest does not override them.
- The package manifest still owns stable identity: `asset.id`, `asset.type`,
  `asset.registryPrefix`, `model`, and `stateModel`.
- Saved scenes and mappings must reference stable asset and parameter IDs, not
  the package's current folder path.
- Reorganizing the desktop file tree should be a catalog/layout change, not a
  scene migration event.
- If a manifest explicitly sets `category` or `layerGroup`, Synaptome should
  either honor that override or warn when it conflicts with the folder path.

### Standard 8: Backward Compatibility

Existing numeric `mode` parameters may remain for scene compatibility, but new
work must document what the numbers mean.

If a legacy layer used `mode` as an algorithm selector, migrate by:

1. Adding root-level `model` metadata to the layer asset.
2. Treating `defaults.mode` as a live regime for new assets.
3. Preserving an inference path for older assets when practical.
4. Regenerating layer catalog and parameter manifests after the migration.

Do not create new layers where `mode = 0` means one scientific algorithm and
`mode = 1` means a different scientific algorithm.

## Browser Navigation Contract

The Browser should use one navigation grammar everywhere.

### Left Tree

- Up/Down moves through visible rows.
- Right expands a collapsed group. If already expanded, it moves into the first
  child.
- Left collapses an expanded group. If already collapsed, it moves toward the
  parent.
- Enter/Space activates the selected destination or transfers focus to the
  parameter grid.

### Parameter Grid Sections

Parameter section headers in the right grid must mirror the left-tree behavior:

- Up/Down moves through visible section headers and parameter rows.
- Right expands a collapsed parameter section. If already expanded, it moves
  into the first visible child row.
- Left collapses an expanded parameter section.
- Left/Right on ordinary parameter rows keep their existing grid-column
  behavior.
- Enter/Space should act on editable parameter rows, saved-scene rows, slot
  assignment rows, MIDI/OSC learn rows, and similar actions. It should not be
  the primary collapse/expand gesture for section headers.

This keeps `Color`, `Motion`, `Scale`, and other sections feeling like the same
UI pattern as the left-hand category menu.

## Parameter IDs And UI Labels

Stored parameter IDs should be technical, stable, and portable. In the current
codebase, prefer lower camelCase IDs because layer JSON, scene JSON, and
registry IDs already use that convention.

Good stored IDs:

- `pheromoneDecay`
- `trailDiffusion`
- `neighborCount`
- `repulsionRadius`
- `alignmentWeight`
- `predatorPressure`
- `flowConductivity`
- `cloudMountainAvoidance`
- `triangleTargetLength`

Weaker stored IDs:

- `magicAmount`
- `vibe`
- `chaos`
- `prettyStrength`
- `sauce`

UI labels may be more expressive, but they still need to describe the actual
control. Use the form:

```text
Section: Human Label
```

Examples:

- `Growth: Land Emergence`
- `Motion: Cloud Wind Speed`
- `Scale: Triangle Target Edge`
- `Force: Mountain Avoidance`
- `Color: Cloud Shade B`

The prefix before `:` defines the collapsible parameter section in the Browser.

Parameter IDs must describe the model value, not the input source driving it.
For a normal generative layer, `cloudDensity`, `growthRate`, `matterGlow`,
`sparkleAmount`, and `windDeflection` are good target parameters. Avoid source
terms such as `audioReactivity`, `micLevelAmount`, `bassLift`, `highsGlint`, or
`musicEnergy` unless the layer itself is an audio/sensor monitor whose subject
is that signal.

## Parameter Value Format Standards

New layers should use the same value formats everywhere: layer defaults,
registered parameter ranges, scene JSON, MIDI/OSC output ranges, and generated
parameter manifests.

| Concern | Standard |
| --- | --- |
| Color channels | Use float channels in `0.0-1.0` for RGB and alpha. Do not use `0-255` in new layer parameters. Renderer code may convert internally. If a value needs to be brighter than a base color, expose a separate `Glow`, `Emission`, `Radiance`, or `Intensity` multiplier rather than storing HDR color channels above `1.0`. |
| Opacity, alpha, masks | Use normalized `0.0-1.0` floats. Group Browser-facing controls under `Visibility`, not scattered between `Alpha`, `Action`, and layer-specific labels. |
| Normalized weights | Use `0.0-1.0` when the parameter represents a blend, phase, probability, maturity, threshold, or amount. |
| Gains and multipliers | Use `1.0` as neutral when the parameter multiplies an existing value. If the range exceeds `1.0`, name it as `Gain`, `Boost`, `Multiplier`, `Radiance`, or similar. |
| Distances and positions | Use scene/world units. Label units in the descriptor when the value has a physical interpretation, such as `px`, `world`, or `deg`. |
| Angles | Use degrees for Browser-facing parameters. Convert to radians only inside implementation code. |
| Time | Use seconds for durations and persistence, Hz for frequencies, and normalized `0.0-1.0` for phase. Avoid frame-count based parameters. |
| Rates | Name the unit: `perSecond`, `Hz`, `Bpm`, or `Rate`. A rate should not secretly mean "amount per frame". |
| Counts and resolution | Use integer-valued floats only where the registry requires floats; set step `1.0` and clamp before use. Names should be `Count`, `Samples`, `Segments`, `Rows`, `Columns`, `Octaves`, or equivalent. |
| Seeds | Use integer-valued floats with step `1.0` when exposed through the Browser. Treat seeds as deterministic IDs, not continuous values. |
| Booleans | Use bool parameters for toggles. One-shot buttons should live under `Action` and reset after handling. |
| Enums and modes | Prefer explicit string parameters when supported. If a numeric index is necessary, use integer-valued floats with step `1.0` and document the stable value set. |
| External input | Expose generic model targets. OSC, MIDI, audio, sensor, and host-source details belong in mappings, not parameter names. |

When touching legacy layers that already use `0-255` color values or
source-specific audio parameters, do not copy those patterns into new work. If
the layer is being substantially revised, migrate toward these standards and
preserve old scene compatibility with aliases or migration notes.

## Canonical Parameter Sections

Use these section prefixes for Browser-visible parameter labels.

| Section | Use For |
| --- | --- |
| `Action` | Buttons, triggers, reseed controls, one-shot operations. |
| `Growth` | Emergence, phase, maturity, uplift, lifecycle progression. |
| `Motion` | Speed, spin, drift, wind direction, position offsets, orbit motion. |
| `Scale` | Size, radius, height, depth, distance, spacing, resolution, thresholds that define physical extent. |
| `Count` | Sample counts, puffs, particles, agents, segments, octaves, rows. |
| `Force` | Weights, pressures, attraction, repulsion, turbulence, curl, deformation, avoidance. |
| `Time` | Rates, decay, memory, smoothing, sync, persistence, cadence. |
| `Audio` | Audio monitor layers and explicit audio/source routing surfaces. Normal generative layer parameters should not be named for audio bands. |
| `Glow` | Bloom, highlights, emission, shimmer, sparkle, light response. |
| `Visibility` | Visible/on/off state, opacity, alpha, coverage, masks, and transparent material amount. |
| `Input` | OSC/MIDI/source selection and external data feed options. |
| `Color` | All RGB/palette/material color controls. This section should often be collapsed during performance. |
| `Seed` | Random seeds and deterministic reseeding values. |
| `General` | Fallback only. Avoid using it when a clearer section exists. |

Do not invent near-duplicates such as `Alpha`, `Opacity`, `Colour`, `Size`,
`Rendering Color`, `Visual`, or `Display`. Use the canonical sections unless
there is a strong reason to extend the registry. `Alpha` is a legacy prefix and
should not be used for new layer parameters.

## Universal Parameter Group Ownership

Some parameter families must always land in the same Browser group. This is
more important than the layer's local naming habits.

| Parameter Family | Browser Section | Naming Rule |
| --- | --- | --- |
| Whole-layer visibility | `Visibility` | Prefer the console slot owner. Do not add a second layer-local `visible` unless it controls an independent subcomponent. |
| Whole-layer opacity | `Visibility` | Use the console slot `opacity` owner. Do not add a second layer-local `alpha`, `opacity`, or `layerOpacity` that multiplies the whole layer again. |
| Subcomponent visibility | `Visibility` | Name the object: `cloudShadowVisible`, `vectorOverlay`, `waterSurfaceVisible`. |
| Subcomponent opacity | `Visibility` | Prefer `...Opacity` for user-facing IDs and labels. Legacy `...Alpha` IDs may remain, but the UI label should still be `Visibility: ... Opacity`. |
| Visual masks and coverage | `Visibility` | Use this section when the control changes what portion of something is visible. Use `Time` only when the value is a fade duration or decay rate. |
| RGB and palettes | `Color` | Use `...R`, `...G`, `...B` float channels in `0.0-1.0`, or a named palette selector. |
| Emission and bloom | `Glow` | Use for light response, bloom, radiance, highlights, shimmer, sparkle, and luminous gain. |
| Physical extent | `Scale` | Radius, height, depth, width, spacing, threshold distances, and world-space offsets. |
| Counts and resolution | `Count` | Particles, samples, segments, rows, columns, octaves, and mesh density. |
| Motion and position | `Motion` | Speed, spin, drift, orbit, pan, tilt, phase offset, and position offsets. |
| Simulation forces | `Force` | Attraction, repulsion, turbulence, curl, pressure, avoidance, uplift, drag, and deformation weights. |
| Rates and memory | `Time` | Smoothing, decay, persistence, BPM sync, cadence, frequency, lifecycle duration, and rates. |
| Source selection | `Input` | OSC/MIDI/source/mode selectors and input routing controls. Do not put source names into ordinary model parameters. |
| Reseed and one-shots | `Action` | Buttons and commands that trigger an event rather than continuously shaping the model. |

When a parameter could fit two rows, use the row that describes what the user
believes they are changing. `trailOpacity` belongs in `Visibility`; `trailDecay`
belongs in `Time`; `trailDeposit` belongs in the model section that describes
the simulated behavior.

## Redundancy And Single-Owner Rules

Each visible concept should have one Browser owner. A layer can multiply several
values internally, but the user should not see multiple controls that appear to
perform the same job.

Use this ownership model:

| Concept | Owner | Do Not Duplicate As |
| --- | --- | --- |
| Whether a console slot draws | Slot active/visible control | Layer-local `visible`, `enabled`, or `draw` parameters that hide the whole layer again. |
| Overall layer opacity | Slot `opacity` / `Layer Opacity` | Layer-local `alpha`, `opacity`, `layerAlpha`, or `masterAlpha` parameters. |
| Layer-internal material opacity | Layer parameter named for the material or object | Generic `alpha` or `opacity` with no object name. |
| HUD/widget visibility | HUD layout or widget toggle owner | Layer parameters or scene parameters with duplicate widget visibility. |
| OSC/audio response | OSC/MIDI mapping owner | Source-specific layer params such as `audioReactivity` or hidden C++ reads that bypass the mapping UI. |

New visual layers should rely on `LayerDrawParams::slotOpacity` for whole-layer
fading. If a layer also needs internal transparency, expose only named
subcomponent controls such as `cloudOpacity`, `trailOpacity`,
`backgroundOpacity`, `wireOpacity`, or `waterHighlightOpacity`.

Avoid generic IDs such as:

- `alpha`
- `opacity`
- `visible`
- `enabled`
- `layerAlpha`
- `masterOpacity`

unless the parameter is owned by the console slot, HUD/widget system, or another
documented global owner. For layer-local controls, name the object or material
being affected.

## Default OSC Mapping Model

Synaptome should support layers that are audio-reactive, sensor-reactive, or
OSC-reactive by default without baking the source into the layer API.

The rule is:

```text
Generic layer parameter + visible OSC mapping = default source reaction
```

The default mapping is not itself a layer parameter. It is a router mapping
from an OSC address to a generic target parameter. The Browser must show that
mapping in the OSC section/column so the user can unlink it, adjust its range,
or relink it to another OSC source.

Good pattern:

- The layer exposes `console.layer1.cloudDensity`, `console.layer1.growthRate`,
  `console.layer1.waterHighlight`, or `console.layer1.matterGlow`.
- A scene or app mapping pre-binds `/sensor/host/localmic/mic-level` or
  `/sensor/host/localmic/mic-bass` to one of those generic targets.
- The Browser shows the OSC address, input range, output range, smoothing,
  deadband, blend mode, and bank like any other learned OSC mapping.

Bad pattern:

- The layer exposes `audioReactivity`, `audioAmount`, `bassLift`,
  `micSmoothing`, or `highsGlint` as normal generative parameters.
- The layer directly reads `AudioAnalysisBus` and changes public behavior while
  the Browser shows no OSC mapping that the user can remove or retarget.
- Audio defaults are hidden in C++ instead of being visible as mapping data.

For current scene and Browser mapping snapshots, pre-mapped OSC routes use the
router mapping structure under `mappings.router` in scene files, or the same
shape in `synaptome/bin/data/config/midi-map.json` for app-wide defaults:

```json
{
  "mappings": {
    "router": {
      "osc": [
        {
          "bank": "home",
          "pattern": "/sensor/host/localmic/mic-level",
          "target": "console.layer1.cloudDensity"
        }
      ],
      "oscSources": [
        {
          "pattern": "/sensor/host/localmic/mic-level",
          "in": [0.0, 0.12],
          "out": [0.0, 1.6],
          "smooth": 0.25,
          "deadband": 0.01,
          "blend": "scale",
          "relative": true
        }
      ]
    }
  }
}
```

`osc` entries say which generic parameter receives the source. `oscSources`
entries define the source profile: input range, output range, smoothing,
deadband, blend, and whether the output is relative to the target's base value.

Current router behavior profiles OSC ranges by `pattern`, so several targets
using the same OSC address share the same `oscSources` profile. If two targets
need meaningfully different range curves from the same audio metric, prefer
separate derived OSC addresses from the sender or extend the mapping system
before relying on per-target transforms.

Layer packages may eventually ship OSC mapping presets, but those presets must
still create visible mapping rows. They should store target parameter suffixes
such as `matterGlow`, `cloudDensity`, or `pulse`, then expand those suffixes to
the selected slot's public parameter IDs when the layer is assigned. A default
OSC reaction should always be removable, retargetable, and inspectable from the
Browser mapping surface.

Layer authors should treat OSC pre-mapping as part of the default scene or
control preset, not part of the layer's model defaults. The layer JSON may set
base values like `cloudDensity: 0.8` or `matterGlow: 1.0`; the scene or mapping
snapshot decides that `/sensor/host/localmic/mic-level` drives that value for a
particular performance setup.

## Model Concern Grouping

The Browser sections are for operator access. The simulation model should still
be designed around meaningful concerns.

### Shared

- `seed`
- `timeScale`
- `agentCount`
- `simulationStepsPerFrame`
- `noiseAmount`
- `memoryDecay`
- `inputSensitivity`

### Motion

- `speed`
- `turnRate`
- `inertia`
- `fieldOfView`
- `blindAngle`
- `boundaryBehavior`

### Neighborhood

- `interactionModel`
- `interactionRadius`
- `neighborCount`
- `repulsionRadius`
- `orientationRadius`
- `attractionRadius`

### Forces

- `separationWeight`
- `alignmentWeight`
- `cohesionWeight`
- `goalWeight`
- `repulsionWeight`
- `attractionWeight`
- `predatorPressure`

### Trail

- `trailDeposit`
- `trailDecay`
- `trailDiffusion`
- `trailSharpening`
- `sensorDistance`
- `sensorAngle`
- `turnAngle`

### Graph And Flow

- `edgeConductivity`
- `flowCapacity`
- `pathCostWeight`
- `pruneThreshold`
- `redundancyBias`
- `bridgeThreshold`

### Rendering

- `opacity`
- `lineWidth`
- `glowStrength`
- `colorMapping`
- `trailPersistence`
- `densityContrast`

Rendering parameters should not pretend to be algorithmic parameters. Keep the
simulation model and visual skin separable.

## Low-Poly Scientific Scene Rules

Low-poly scenes should be scientifically informed, not merely faceted.

Use this pattern:

1. Define the underlying field or state model first.
2. Sample or triangulate that model second.
3. Tie color and material changes to model variables, not arbitrary face IDs.
4. Expose parameters that alter the model, not just the decoration.
5. Preserve the low-poly aesthetic by controlling mesh density and facet shape,
   not by making the simulation less coherent.

Examples:

- Terrain should grow from uplift, erosion, slope, elevation, moisture, or
  shoreline state, then be rendered as a triangulated mesh.
- Clouds should have wind, pressure-layer height, density, turbulence, and
  orographic steering. They should not simply be grey blobs passing through
  mountains.
- Orbital scenes should be driven by orbital periods, inclination, eccentricity,
  scale choices, and selected source system, not random decorative circles.
- Cosmic scenes should be driven by density fields, filaments, voids, cooling,
  gravity, and clustering, not by particles orbiting arbitrary centers.

Field-first, mesh-second is the default rule.

## Mode Naming Principles

Mode names should describe a meaningful behavioral regime.

Good mode names:

- `converge`
- `diverge`
- `explore`
- `exploit`
- `calm`
- `stressed`
- `predatorEnabled`
- `redundant`
- `efficient`
- `prune`
- `grow`

Avoid mode names that only describe mood unless the behavior is defined:

- `dream`
- `chaos`
- `liquid`
- `cosmic`

Those can be preset names, not model modes.

## Modes Versus Presets

A **mode** changes a named behavioral rule.

A **preset** is a saved bundle of parameters.

Example:

- Mode: `predatorEnabled = true`
- Preset: `Nightclub Pressure`, which may set `predatorEnabled = true`,
  `predatorPressure = 0.75`, `turnRate = 0.4`, `noiseAmount = 0.2`, and
  `cohesionWeight = 0.9`.

Presets can be poetic. Modes and parameters should be precise.

## Live Performance Rules

Live controls should prefer continuous or reversible changes.

Good live controls:

- Increase predator pressure.
- Sweep convergence into divergence.
- Increase pheromone evaporation.
- Raise trail diffusion.
- Move from exploration toward exploitation.
- Increase neighbor count in a murmuration layer.
- Advance terrain growth or cloud density without reseeding.

Risky live controls:

- Switch Ant Tunnels into Physarum Particles.
- Replace metric-radius schooling with topological murmuration.
- Replace continuous trail fields with graph conductivity.
- Change coordinate spaces or input embeddings.
- Swap a terrain-uplift model for an orbital model.

## Crossfading Between Layers

When two layers are related but algorithmically distinct, prefer crossfading
instead of pretending the switch is a live mode.

Examples:

- Crossfade Schooling into Murmuration.
- Crossfade Ant Tunnels into Physarum Particles.
- Crossfade Physarum Particles into Flow Network.
- Crossfade a procedural island scene into a solar-system scene instead of
  forcing them into one layer.

The UI can present these as related siblings inside a group while preserving
their algorithmic integrity.

## Proposed Grouping

### Collective Motion

Collective Motion layers model agent movement, coordination, pressure, and group
response.

Scientific roots:

- Reynolds-style boids: separation, alignment, cohesion.
- Couzin-style zones: repulsion, orientation, attraction.
- Vicsek-style self-propelled particles: order, density, and noise.
- Starling murmuration research: topological neighbor tracking and scale-free
  response.
- Pursuit/evasion steering: predator and prey pressure.

Recommended layers:

| Layer | Core Model | Live Modes | Notes |
| --- | --- | --- | --- |
| Schooling | Metric-radius or zone-based agents | predator off/on, convergence/divergence, calm/stressed, exploratory/locked | Good default collective-motion layer. |
| Murmuration | Topological-neighbor flocking | calm/stressed, expansion/compression, wave response, predator pressure if it does not rewrite the model | Deserves its own layer because neighbor selection is fundamentally different. |

### Adaptive Trail

Adaptive Trail layers model memory, route reinforcement, network growth, decay,
and path optimization.

Scientific roots:

- Ant Colony Optimization: pheromone deposition, evaporation, and reinforced
  routes.
- Physarum particle models: sensor angle, trail deposition, diffusion, decay,
  and emergent transport patterns.
- Physarum flow/network models: conductivity, flux, pruning, cost, efficiency,
  and fault tolerance.
- Maze-solving and adaptive transport-network studies.

Recommended layers:

| Layer | Core Model | Live Modes | Notes |
| --- | --- | --- | --- |
| Ant Tunnels | Discrete graph-walking agents with pheromone memory | exploration/exploitation, fast/slow evaporation, converge/diverge, single/multi-colony | Best for route discovery across known nodes. |
| Physarum Particles | Continuous particles sensing and depositing trail | attraction/repulsion, growth/decay, diffuse/sharpen, exploratory/contractile | Best for organic path emergence and living texture. |
| Flow Network | Graph edges with conductivity and flow adaptation | efficient/redundant, prune/grow, cost-sensitive/fault-tolerant | Best for bridges, bottlenecks, and resilient routes. |
| Tunnel Memory | Persistent path rendering over another trail layer | fresh/historical, high/low decay, highlight bridges/dead ends | Usually an output view or overlay, not a full algorithm by itself. |

### Planetary And World Models

Planetary and world-model layers include terrain, atmospheres, oceans, orbital
systems, and cross-section mini-worlds.

Scientific roots:

- Height fields, uplift, erosion, slope, shoreline, and bathymetry.
- Atmospheric flow, pressure layers, wind fields, turbulence, and orographic
  steering.
- Orbital mechanics, inclination, eccentricity, period, and scale transforms.
- Low-poly mesh sampling of continuous fields.

Recommended layers:

| Layer | Core Model | Live Modes | Notes |
| --- | --- | --- | --- |
| Mountain Island | Seafloor uplift through water into terrain plus pressure-layer clouds | growth on/off, reseed, terrain density, cloud density, wind regime | Keep terrain and cloud behavior model-aware. |
| Solar Orrery | Scaled orbital system | source system, time scale, camera emphasis | Keep orbital data distinct from decorative space scenes. |

## Layer Strength Checklist

A Synaptome layer is strong when it has:

- A clear scientific model family.
- A clear input mapping from music, memory, identity, or taste data that is
  visible through the mapping system.
- A stable state model.
- Parameters grouped by canonical Browser sections.
- Stored parameter IDs that are technical and stable.
- Parameter value ranges that follow the shared format standards.
- At least one meaningful live mode.
- Metrics that can explain what the layer is showing.
- Presets that change behavior without hiding the underlying model.
- Rendering that reveals the simulation instead of replacing it.
- A default scene that looks complete without manual setup.
- A package manifest that declares its asset metadata, parameters, modes,
  option metadata, presets, media dependencies, and compatibility expectations.
- Parameter declarations that can be generated into the public parameter
  manifest without hand-editing the manifest.
- A fast single-layer test path that can instantiate, update, and draw the
  layer without rebuilding or launching the whole performance app.

## Compatibility Contract

This document defines the author-facing expectations that a compatibility gate
should enforce. It does not own the loader, installer, generated-registration,
or runtime-bench architecture.

The compatibility gate should answer one question:

```text
Can this layer be installed, inspected, controlled, saved, restored, and tested
without surprising the rest of Synaptome?
```

The first version can be mostly static. It should validate package files,
catalog metadata, parameter declarations, defaults, option metadata, presets,
scene targets, and factory registration. Later versions should add a runtime
smoke test that instantiates the layer in isolation and renders a short
offscreen sample.

### Compatibility Levels

| Level | What It Proves | Author-Facing Evidence |
| --- | --- | --- |
| Static package check | Files are present, JSON is schema-valid, IDs are stable, defaults match declared parameter types/ranges, and catalog metadata is complete. | A package manifest and referenced files that match schema. |
| Manifest check | Public parameters, modes, presets, and option metadata can be promoted into public contracts without hand edits. | Declared parameter suffixes, option metadata, presets, and catalog fields. |
| Registration check | The declared `type` resolves to one implementation entry. | A source registration file now; generated registration, module manifest, or package loader later. |
| Runtime setup check | The layer can be configured and can register the parameters it declared. Missing, extra, or type-mismatched public parameters fail the gate. | Setup has no surprising side effects and registers the declared public surface. |
| Draw smoke check | The layer can update and draw into an offscreen target for a few frames without crashing or producing a blank frame. | Deterministic defaults, assets, viewport assumptions, and seeds are declared. |

The architecture target and gap tracking for implementing these checks belongs
in the layer-system roadmap.

## Layer Package Standard

Target package shape:

```text
layers/<Browser Category>/<Layer Group>/<Layer Label>/
  layer.package.json
  src/
    <LayerName>.h
    <LayerName>.cpp
    register_<packageId>.cpp
  assets/
    ...
  presets/
    default.json
    <presetId>.json
  scenes/
    demo.json
  tests/
    layer_test.json
```

`layer.package.json` should be the package source of truth. Existing
`*.layer.json` assets can remain during migration, but the target is that a
layer package declares everything Synaptome needs to inspect the layer before
instantiating it.

The folder path supplies Browser organization. The package manifest supplies
stable identity. This lets authors reorganize visible Browser groups from the
desktop file tree without renaming the layer IDs that saved scenes depend on.

Minimum package fields:

```json
{
  "schemaVersion": 1,
  "packageId": "generative.schooling",
  "asset": {
    "id": "schooling",
    "label": "Schooling",
    "category": "Generative",
    "layerGroup": "Collective Motion",
    "model": "metric-radius-schooling",
    "stateModel": "agents with position, velocity, zone radii, and optional predator pressure",
    "type": "generative.schooling",
    "registryPrefix": "generative.schooling"
  },
  "parameters": [],
  "modes": [],
  "presets": [],
  "media": [],
  "tests": {
    "bench": "tests/layer_test.json"
  }
}
```

Package rules:

- `packageId` is globally unique.
- `asset.id` is stable and scene-facing.
- `asset.type` is factory-facing.
- `asset.registryPrefix` is authoring-facing and must expand into public
  parameter IDs.
- `parameters[]` declares the public parameter surface.
- `defaults` may live in the package or in a referenced asset file, but the
  validator must see one merged default set.
- `presets[]` declares bundled layer presets; user-created presets may live in
  operator-local storage.
- Source registration remains the honest first path until generated
  registration, a package loader, or dynamic module loading exists.

## Parameter Declaration And Manifest Standard

Layer parameters should be declared once in package metadata and then generated
into the public manifest. The implementation still registers live pointers with
`ParameterRegistry`, but the package declaration is the public contract.

Target parameter declaration:

```json
{
  "id": "neighborCount",
  "kind": "float",
  "label": "Count: Neighbors",
  "default": 7,
  "range": { "min": 1, "max": 20, "step": 1 },
  "units": "count",
  "description": "Topological neighbors used for alignment and cohesion."
}
```

Manifest generation should expand this through `registryPrefix`:

```text
generative.schooling.neighborCount
console.layer{slot}.neighborCount
```

Rules:

- Package parameter IDs are suffixes such as `neighborCount`, not full slot IDs.
- Generated manifest IDs are full public IDs.
- The runtime layer must register the same suffix, type, range, and label
  declared by the package, unless the package marks a parameter as deprecated.
- Defaults must match the declared type and range.
- Parameters not declared by the package should be treated as private until
  explicitly promoted.
- Generated manifests should never be manually edited to add a layer parameter.

This is both a standards update and scaffolding work:

- Standards: define the package declaration shape and compatibility rules.
- Scaffolding: teach the manifest generator, schema, Browser, and validators to
  consume package parameter declarations and compare them with runtime
  registration.

## Named And Dynamic Options

Numeric controls such as `mode = 0`, `mode = 1`, `device = 0`, or
`resolution = 2` are not sufficient for a public layer surface. When a
parameter has choices, the Browser should show names.

### Static Options

Use static option metadata when the set of choices is known at package time.

```json
{
  "id": "mode",
  "kind": "string",
  "label": "Input: Mode",
  "default": "converge",
  "options": [
    { "value": "converge", "label": "Converge" },
    { "value": "diverge", "label": "Diverge" },
    { "value": "orbit", "label": "Orbit" }
  ]
}
```

Prefer string values for new enum-style parameters. Legacy numeric parameters
may keep numeric values for scene compatibility, but they must declare labels:

```json
{
  "id": "colorMode",
  "kind": "float",
  "label": "Color: Mode",
  "default": 0,
  "range": { "min": 0, "max": 2, "step": 1 },
  "options": [
    { "value": 0, "label": "Palette" },
    { "value": 1, "label": "Signal" },
    { "value": 2, "label": "Heatmap" }
  ]
}
```

### Dynamic Options

Use dynamic option metadata when choices come from the machine, runtime state,
or content library.

```json
{
  "id": "device",
  "kind": "float",
  "label": "Input: Webcam",
  "default": 0,
  "range": { "min": 0, "max": 0, "step": 1 },
  "optionsSource": {
    "id": "devices.webcam",
    "value": "index",
    "label": "name"
  }
}
```

Recommended option sources:

| Source ID | Use For | Value |
| --- | --- | --- |
| `devices.webcam` | Available camera devices. | Stable device index or device ID when available. |
| `media.videoClips` | Video clip catalog entries. | Clip ID. |
| `midi.inputs` | MIDI input devices. | Device ID/name. |
| `osc.sources` | Known OSC source profiles. | Source pattern or source ID. |
| `transport.bpmMultipliers` | Transport-supported rhythmic multipliers. | Numeric multiplier. |
| `layer.modes` | Modes declared by the current layer package. | Mode ID. |
| `layer.presets` | Bundled and user layer presets for the current asset. | Preset ID. |

Dynamic option rules:

- The stored value must remain stable enough for scene and preset reload.
- If the underlying device disappears, Synaptome should keep the stored value
  and mark it unavailable instead of silently changing it.
- The Browser owns option refresh and display. Layers should not hand-roll
  device menus.
- A dynamic option source belongs to Synaptome scaffolding. A layer package only
  declares which source it needs.

## Single-Layer Test Bench

Layer authors need a quick way to answer:

```text
Does this one layer configure, register parameters, update, and draw correctly?
```

A layer package should give the future bench enough information to:

- Load one layer package or one legacy layer asset.
- Resolve factory registration.
- Merge package defaults and selected preset values.
- Configure and instantiate the layer.
- Register parameters into an isolated `ParameterRegistry`.
- Compare registered parameters against package declarations.
- Run deterministic update frames with a fixed viewport, BPM, speed, and seed.
- Draw into an offscreen target.
- Report crashes, missing parameters, duplicate parameters, out-of-range
  defaults, blank output, and unexpected heavy side effects.

The CLI shape, runtime seam, offscreen drawing setup, and no-`ofApp.cpp` test
path are Artist SDK architecture work, not layer-authoring doctrine.

## Layer Presets

A layer preset is a named bundle of parameter values for one layer asset. It is
not a full scene.

Use presets for:

- Starting looks.
- Performance-ready parameter bundles.
- Alternative moods or behaviors inside the same model.
- Shareable settings for a specific layer.

Do not use presets to:

- Change the layer's scientific model.
- Hide a reseed behind a normal mode change.
- Store slot assignment, other layers, global mappings, or window state.

Target preset shape:

```json
{
  "schemaVersion": 1,
  "assetId": "schooling",
  "presetId": "tight-pressure",
  "label": "Tight Pressure",
  "description": "Dense schooling with stronger predator response.",
  "parameters": {
    "neighborCount": 9,
    "cohesionWeight": 0.82,
    "predatorEnabled": true,
    "predatorPressure": 0.65
  }
}
```

Preset rules:

- Store parameter suffixes, not slot-expanded IDs.
- Apply a preset to the selected layer instance by expanding suffixes through
  that instance's live prefix.
- Validate preset values against package parameter declarations.
- Bundled presets live with the package.
- User presets should live in operator-local storage, keyed by `assetId` and
  package version.
- Scenes may reference or inline layer preset values, but scene loading must
  still restore explicit saved parameter values deterministically.
- The immutable layer default remains the fallback when no preset is selected.
- Preset banks may provide an ordered list of quick-toggle presets for
  performance, but each entry is still just a validated parameter bundle.

This document defines what layer authors may put in a preset. Browser save/load
UI, operator-local storage, scene merge behavior, and preset-aware validation
belong to SDK architecture and contract-gap tracking until they are implemented.

## Standards Versus Scaffolding Ownership

Use this table to decide whether a change belongs in this document/schema/tests
or in Synaptome runtime scaffolding.

| Need | Standards / Docs / Fixtures | Synaptome Scaffolding |
| --- | --- | --- |
| Layer-vs-mode decision | Define the decision test and examples. | Browser may group related sibling layers and modes. |
| Package layout | Define required files, fields, IDs, and examples. | Loader/installer discovers packages and resolves source/assets. |
| Parameter declarations | Define suffix IDs, labels, types, ranges, options, defaults, deprecation rules. | Registry, Browser, manifest generator, and validators consume declarations. |
| Automatic manifest entries | Declare that package parameters are the source. | Update `gen_parameter_manifest.py` and schema to merge package declarations and check runtime registration. |
| Static dropdowns | Define `options[]` metadata and legacy numeric-label policy. | Browser renders dropdowns and writes selected values. |
| Dynamic dropdowns | Define `optionsSource` contract and source IDs. | Runtime publishes option providers for webcams, media, MIDI, OSC, modes, and presets. |
| Single-layer test | Define what must be proven. | Build/run the bench target or app CLI mode, offscreen renderer, and blank-frame checks. |
| Layer presets | Define file shape and suffix-based values. | Save/load UI, local storage, package preset discovery, scene merge policy. |
| Scene compatibility | Define stable IDs and migration expectations. | Scene loader applies migrations and preserves old values. |
| Public validation gate | Define pass/fail rules and fixtures. | Implement validators and wire them into dev/release checks. |

Rule of thumb:

- If it changes what layer authors must declare, document it here and encode it
  in schema/fixtures.
- If it changes how Synaptome discovers, displays, stores, tests, or applies
  that declaration, it is scaffolding work.

## Validation Gate Roadmap

This document keeps the author-facing validation expectations. Implementation
order for package schemas, manifest generation, dropdown rendering, dynamic
option providers, preset storage, and the single-layer bench is tracked in:

- `docs/architecture/synaptome_artist_sdk.md`
- `docs/architecture/synaptome_layer_system_roadmap.md`
- `docs/project_ops/in_progress/`

When the contract shape is implemented, stable entries should be indexed from
`docs/contracts/README.md` and backed by schemas and fixtures under
`docs/schemas/**` and `tools/testdata/**`.

## Useful Metrics

### Collective Motion Metrics

- `cohesion`: how tightly agents cluster.
- `polarization`: how aligned their headings are.
- `fragmentation`: how many subgroups exist.
- `density`: how crowded local neighborhoods are.
- `leadershipInfluence`: how strongly informed agents steer the group.
- `pressureResponse`: how much the group changes under predator or constraint
  pressure.
- `milling`: how much agents orbit rather than travel directionally.

### Adaptive Trail Metrics

- `pathStrength`: accumulated reinforcement along a path.
- `pathAge`: how long a trail has persisted.
- `evaporationBalance`: how quickly unused trails disappear.
- `bridgeScore`: how important a route is between clusters.
- `deadEndScore`: how much effort leads nowhere.
- `redundancy`: how many alternate routes exist.
- `networkCost`: total distance or friction needed to connect targets.
- `faultTolerance`: how well the network survives removed nodes or edges.

### Planetary / World Metrics

- `elevation`: terrain height relative to water or datum.
- `slope`: local terrain gradient.
- `shorelineLength`: active land/water boundary length.
- `upliftEnergy`: aggregate terrain emergence force.
- `cloudClearanceConflict`: where terrain intersects a cloud pressure layer.
- `windDeflection`: how strongly clouds steer around terrain.
- `orbitalPhase`: normalized orbit progress.

Metrics should be available to the system even if the UI exposes only a small
subset at first.

## Implementation Checklist

When adding or revising a layer:

1. Identify the model family and layer group.
2. Decide layer vs mode using the state-continuity test.
3. Create or update the layer package metadata, including `asset`,
   `parameters`, `modes`, `presets`, media dependencies, and test metadata.
4. Define stable lower camelCase parameter IDs that describe model values, not
   source inputs.
5. Use canonical `Section: Label` UI labels.
6. Follow the shared value formats for colors, opacity, angles, time, rates,
   counts, seeds, and normalized values.
7. Put colors under `Color` and all visibility/opacity controls under
   `Visibility` so they can be collapsed predictably during performance.
8. Check the single-owner rules before adding `visible`, `opacity`, `alpha`, or
   similar controls.
9. Add `options[]` for named static choices and `optionsSource` for runtime
   choices such as webcams, media clips, MIDI inputs, OSC sources, modes, or
   presets.
10. Keep algorithmic controls separate from rendering controls.
11. If the layer should respond to audio, sensors, or OSC by default, add a
   visible router mapping to generic target parameters instead of adding
   source-specific layer parameters.
12. Add defaults that look complete on first load.
13. Add at least one useful layer preset when there is more than one natural
    starting look.
14. Register the layer in the factory/project while source registration remains
    the active integration path.
15. Regenerate generated manifests and catalog fixtures when metadata changes.
16. Run the relevant validation checks and, once available, the single-layer
    bench before handing off.

Common validation commands:

```powershell
python tools\layer_catalog_regression.py --check
python tools\gen_parameter_manifest.py --check
python tools\validate_parameter_targets.py --contract-fixtures
python tools\validate_configs.py
python tools\validate_osc_route_patterns.py
python tools\validate_console_layout_contract.py
```

Future package and bench commands should be added here when they exist.

Use targeted MSBuild compiles for the touched C++ translation unit whenever
code changes.

## Initial Recommendation

Start with these groups:

1. **Collective Motion**
   - Schooling
   - Murmuration

2. **Adaptive Trail**
   - Ant Tunnels
   - Physarum Particles
   - Flow Network
   - Tunnel Memory overlay

3. **Planetary And World Models**
   - Mountain Island
   - Solar Orrery

Use modes for live behavioral toggles inside those layers. Use separate scenes
or crossfades for algorithm changes that require a different state model.

# Synaptome Biological Layer Roadmap

Status: Parked supporting architecture; reviewed 2026-07-18. No biological
layer implementation is currently active. Resume through a focused Project Ops
request after the pre-media safety gate, package validation, and shared
field/graph infrastructure are ready. This document preserves the intended
implementation order for upgrades and new layer families.

Read with: [`synaptome_layer_system_roadmap.md`](synaptome_layer_system_roadmap.md),
[`synaptome_artist_sdk.md`](synaptome_artist_sdk.md),
[`../project_ops/synaptome_layer_design_standards.md`](../project_ops/synaptome_layer_design_standards.md),
and [`synaptome_transport_reactivity.md`](synaptome_transport_reactivity.md).

## Purpose

Synaptome already has strong ecosystem, geology, orbital, agent-trail, and
collective-motion layers. The missing middle is:

```text
particles -> cells -> tissues -> organs -> ecosystems -> planets -> cosmos
```

This roadmap fills that middle without creating a pile of weak novelty layers.
Each item should either strengthen an existing state model or become a new
layer only when the algorithm, stored state, or scientific metaphor changes.

## Decision Rule

Use the layer design standards as the gate:

- If the existing agents, fields, trails, meshes, or graphs can continue
  naturally after the change, implement it as a mode, parameter, preset, or
  internal module of the current layer.
- If the change replaces the core algorithm, field representation, graph model,
  or agent species model, implement it as a new layer.

For example, predator fear propagation belongs inside `FlockingLayer`; Lenia
does not belong inside `GameOfLifeLayer` because it changes binary cells into
continuous fields with convolution kernels and growth functions.

## Current-Layer Upgrades

These should improve the current layer set before new layer count grows.

| Candidate | Target | Implementation Shape | Why It Belongs Here |
| --- | --- | --- | --- |
| Swarm predation with escape waves | `FlockingLayer` assets `generative.schooling` and `generative.murmuration` | Add a fear/pressure field or neighbor-propagated panic signal, predator strike events, pressure decay, and wave rendering. | The boids, predators, velocities, and trail texture remain valid. |
| Ant colony optimization pathfinding | `AgentFieldLayer` asset `generative.antTunnels` | Add nest/food targets, route reinforcement, obstacle avoidance, exploration/exploitation presets, and route-collapse metrics. | The current ant trail model already stores pheromone-like trails and behavior modes. |
| Chemotaxis bridge | `AgentFieldLayer` assets `generative.physarum` and `generative.slimeMold` | Add an optional nutrient field, nutrient pulses, consumption, reproduction pressure, and waste/avoidance bias. | Light chemotaxis extends the existing trail-field agents without replacing them. |
| Turing pattern shader masks | `SolarSystemLayer`, `MountainIslandLayer`, `RiverFormationLayer`, `PerlinNoiseLayer` | Add a reusable biological texture module for spots, stripes, labyrinths, and threshold masks. | This is most valuable as a rendering/material generator over existing worlds. |
| Vascular and venation influence | `RiverFormationLayer`, `CosmosFormationLayer`, maybe `MountainIslandLayer` | Use demand points, branch thickening, and pruning as tributary, filament, lightning, or infrastructure detail. | A light venation pass can guide existing river/filament detail without owning the whole simulation. |
| Neural firing and pruning bridge | `CosmosFormationLayer` | Add firing waves, edge strengthening, weak-edge decay, synchronized pulses, and synaptic glow modes. | The current cosmic web already stores nodes and filaments; neural behavior can first enrich that graph. |

## New Layer Families

These deserve separate layer types because they require distinct state
representations or scientific model families.

| Layer | Core Model | Visual Identity | First Public Controls |
| --- | --- | --- | --- |
| `ReactionDiffusionLayer` | Gray-Scott-style two-chemical diffusion and reaction field. | Living skin, coral tissue, fungal bloom, chemical turbulence. | `feedRate`, `killRate`, `diffusionA`, `diffusionB`, `injectionRate`, `contourThreshold`, `fieldScale`. |
| `LeniaLayer` | Continuous cellular automata with convolution kernels and growth functions. | Protozoa, embryos, amoebas, artificial petri dish. | `kernelRadius`, `growthCenter`, `growthWidth`, `growthAmplitude`, `mutationAmount`, `edgeGlow`. |
| `ExcitableMediaLayer` | Resting, excited, and refractory states with propagating excitation fronts. | Cardiac waves, neural pulses, chemical spirals, bioluminescent rings. | `propagationRate`, `excitationThreshold`, `refractoryTime`, `seedRate`, `wavefrontWidth`, `sparkleAmount`. |
| `PredatorPreyFieldLayer` | Field ecology with prey growth, predator diffusion, predation, and predator decay. | Plankton bloom, immune map, microbial chase waves. | `preyGrowth`, `predatorMobility`, `predationRate`, `predatorDecay`, `injectionRate`, `fieldDiffusion`. |
| `TissueFieldLayer` | Simplified Cellular Potts or Voronoi tissue cells with pressure, adhesion, migration, and division. | Microscope tissue, embryogenesis, cell membranes. | `cellCount`, `adhesion`, `pressure`, `divisionRate`, `migrationSpeed`, `membraneGlow`. |
| `GrowthSystemLayer` | Branch growth using L-systems and/or space colonization, with attractors and buds. | Roots, dendrites, vines, coral fans, bronchial trees. | `branchingRate`, `growthSpeed`, `attractorCount`, `pruneRate`, `branchThickness`, `budGlow`. |
| `DifferentialGrowthLayer` | Expanding self-avoiding curves or meshes with repulsion and target spacing. | Leaf margins, brain folds, coral rims, organic lace. | `growthPressure`, `repulsionRadius`, `targetSpacing`, `splitThreshold`, `wrinkleAmount`, `lineThickness`. |
| `VenationLayer` | Demand-driven vessel growth, path thickening, and pruning. | Leaf skeletons, blood vessels, coral veins, living transit maps. | `demandCount`, `growthSpeed`, `vesselThickness`, `flowReinforcement`, `pruneThreshold`, `capillaryGlow`. |
| `ChemotaxisLayer` | Multi-field nutrient, waste, colony expansion, consumption, and reproduction. | Bacterial colonies, mold invasion, nutrient-depleted voids. | `nutrientDensity`, `consumptionRate`, `reproductionRate`, `wasteRepulsion`, `mutationAmount`, `frontGlow`. |
| `StigmergicConstructionLayer` | Agents pick up and deposit particles or density to build porous structures. | Termite mounds, sand architecture, cellular concrete. | `depositionRate`, `pickupRate`, `agentCount`, `humidityBias`, `erosionRate`, `structureHeight`. |
| `NeuralGrowthLayer` | Hebbian graph growth with firing history, synaptic strengthening, and pruning. | Synapses, axons, brain networks, living circuit board. | `nodeCount`, `fireThreshold`, `propagationSpeed`, `synapticPlasticity`, `pruneRate`, `signalGlow`. |
| `ImmuneResponseLayer` | Multi-agent recognition system with pathogens, immune cells, antibodies, and inflammation fields. | Microscope defense response, immune swarm, inflammatory halos. | `pathogenRate`, `replicationRate`, `immuneSpeed`, `antibodyBinding`, `inflammationRadius`, `engulfRate`. |

## Implementation Phases

### Phase 0: Shared Simulation Infrastructure

Build the common pieces that keep the later layers compact and testable.

Deliverables:

- A reusable double-buffer scalar/vector field helper for CPU texture layers.
- Standard field operations: diffusion, decay, blur, threshold, injection, and
  contour extraction.
- A reusable lightweight graph helper for nodes, edges, weights, pruning, and
  pulse propagation.
- Package parameter declarations for common field controls such as
  `diffusionRate`, `decayRate`, `seedRate`, `injectionRate`, `threshold`,
  `fieldScale`, and `contourOpacity`.
- Fixture rules for texture size, deterministic seeds, and nonblank offscreen
  draw tests once the single-layer bench exists.

### Phase 1: Immediate Current-Layer Payoff

Strengthen the layers users already know.

Deliverables:

- `FlockingLayer` fear-wave predation for schooling and murmuration.
- `AgentFieldLayer` ant colony optimization targets, obstacles, and route
  metrics.
- `AgentFieldLayer` optional nutrient field for Physarum and slime mold.
- Reusable Turing-pattern material masks in at least one world layer.
- Updated catalog metadata, modes, presets, parameter manifests, and validation
  snapshots for touched assets.

Success means the current vocabulary becomes visibly deeper without adding a
large new maintenance surface.

### Phase 2: Cells And Chemical Fields

Add the missing biological canon as standalone layers.

Deliverables:

- `ReactionDiffusionLayer`.
- `LeniaLayer`.
- `ExcitableMediaLayer`.
- `PredatorPreyFieldLayer`.
- Shared preset bank: calm, bloom, turbulence, beat-injected, and high-contrast
  contour looks where applicable.
- Mapping presets from beat/onset and audio bands into generic model controls,
  keeping source-specific naming out of layer parameter IDs.

Success means Synaptome can move from binary cellular logic into living,
continuous, beat-reactive fields.

### Phase 3: Growth, Branching, And Networks

Build the middle territory between texture fields and large-scale worlds.

Deliverables:

- `GrowthSystemLayer` with separate documented modes or sibling assets for
  L-system and space-colonization behavior.
- `DifferentialGrowthLayer`.
- `VenationLayer`.
- `NeuralGrowthLayer`.
- Cosmos bridge work for firing/pruning behavior so the new neural layer and
  existing cosmic web can share visual and parameter language.

Success means Synaptome has roots, dendrites, veins, folds, and living networks
as first-class layer families rather than decorative overlays.

### Phase 4: Tissues, Colonies, Construction, And Defense

Add richer multi-agent and multi-field biological scenes.

Deliverables:

- `TissueFieldLayer`.
- Full `ChemotaxisLayer`, promoted beyond the bridge behavior in
  `AgentFieldLayer`.
- `StigmergicConstructionLayer`.
- `ImmuneResponseLayer`.
- Cross-layer metric vocabulary for colony coverage, cell count, pressure,
  path strength, inflammation, pruning, and active wavefronts.

Success means Synaptome can represent living matter at organism-scale and
community-scale, not only particles and fields.

### Phase 5: Integration And Performance Readiness

Make the new vocabulary usable as an instrument.

Deliverables:

- Layer packages or legacy catalog entries for every layer in this roadmap.
- Package-declared parameters, modes, named options, presets, and mapping
  presets.
- Browser grouping under clear families:
  - Cellular Fields
  - Growth Systems
  - Tissue And Ecology
  - Collective Motion
  - Adaptive Trail
- Scene fixtures that combine cells, tissue, growth, ecosystem, planetary, and
  cosmic layers into the particles-to-cosmos continuum.
- Single-layer static validation for every new layer, and runtime/offscreen
  bench coverage when the bench exists.

Success means the biological expansion is not just implemented; it is
discoverable, mappable, presettable, saveable, and testable.

## Browser Organization Target

Recommended visible grouping:

```text
Generative
  Cellular Fields
    Reaction Diffusion
    Lenia
    Excitable Media
    Predator Prey Field
  Growth Systems
    Growth System
    Differential Growth
    Venation
    Neural Growth
  Tissue And Ecology
    Tissue Field
    Chemotaxis
    Stigmergic Construction
    Immune Response
  Collective Motion
    Schooling
    Murmuration
  Adaptive Trail
    Ant Tunnels
    Physarum
    Slime Mold
  Planetary And World Models
    Mountain Island
    River Formation
    Solar System
    Cosmos Formation
```

Existing assets keep their stable IDs. Folder or Browser grouping should not
force scene or mapping migrations.

## Priority Order

1. `FlockingLayer` fear-wave predation.
2. `AgentFieldLayer` ACO targets and nutrient bridge.
3. `ReactionDiffusionLayer`.
4. `LeniaLayer`.
5. `ExcitableMediaLayer`.
6. `GrowthSystemLayer`.
7. `DifferentialGrowthLayer`.
8. `VenationLayer`.
9. `NeuralGrowthLayer`.
10. `TissueFieldLayer`.
11. `PredatorPreyFieldLayer`.
12. `ChemotaxisLayer`.
13. `StigmergicConstructionLayer`.
14. `ImmuneResponseLayer`.

This order favors maximum visual payoff and shared infrastructure first, then
adds richer multi-species systems after the field and graph foundations exist.

## Validation Expectations

Every new layer or substantial current-layer upgrade should include:

- Stable catalog metadata: `id`, `label`, `category`, `layerGroup`, `model`,
  `stateModel`, `type`, and `registryPrefix`.
- Clear live modes that preserve state continuity.
- Parameter IDs that describe model controls, not audio or sensor sources.
- Canonical Browser sections from the layer design standards.
- Defaults that produce a complete visual on first load.
- At least one performance-ready preset when there are multiple natural looks.
- Mapping presets only as visible, editable mappings.
- Updated catalog and parameter-manifest fixtures.
- Deterministic seeds and texture sizes suitable for future single-layer bench
  checks.

## Open Design Questions

- Should `GrowthSystemLayer` hold both L-systems and space colonization as
  sibling assets under one implementation, or should they split into separate
  layer types once package metadata can express shared implementation families?
- Should `TuringPattern` become a private rendering module, a post-effect, or a
  lightweight standalone layer in addition to being embedded in world layers?
- How much multi-species behavior should remain in `AgentFieldLayer` before a
  full `ChemotaxisLayer` becomes the canonical colony model?
- Which metrics should become HUD feeds for biological layers, and which should
  stay internal until the HUD feed schema is more stable?

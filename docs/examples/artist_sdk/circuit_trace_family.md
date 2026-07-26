# Circuit Trace Layer Family

Status: Source-registered, catalog-driven layer family.

The Circuit Trace family uses one compiled runtime type, `circuitTrace`, for
five always-visible Browser assets:

| Asset | Profile | Visual intent |
| --- | --- | --- |
| `generative.circuitSlime` | Dense, active growth | Electronic slime mold |
| `generative.circuitMycelium` | Balanced branching | Plated mycelial junctions |
| `generative.circuitRiver` | Sparse, convergent growth | Tributary-like PCB routing |
| `generative.circuitAntTunnels` | Pheromone corridor routing | Sparse orthogonal ant tunnels |
| `generative.circuitFlowField` | Quantized analytic flow | Curved flow translated to circuit steps |

Each asset has its own stable ID, registry prefix, and defaults, but all five
instantiate the same runtime class. This keeps simulation and parameter
registration in one module while scenes select a named look without a hidden
asset mode switch.

The direction constraint is an invariant, not a parameter: every simulation
step is horizontal, vertical, or 45-degree diagonal. None of these assets can
fall back to free-angle motion.

## Catalog Manifests And Presets

- `synaptome/bin/data/layers/generative/circuit_slime.json`
- `synaptome/bin/data/layers/generative/circuit_mycelium.json`
- `synaptome/bin/data/layers/generative/circuit_river.json`
- `synaptome/bin/data/layers/generative/circuit_ant_tunnels.json`
- `synaptome/bin/data/layers/generative/circuit_flow_field.json`

The five `defaults` objects are the show-ready family presets. Once a layer is
loaded, its values use the normal registry, mapping, and scene-persistence
paths under that asset's `registryPrefix`.

## Package Pattern Audit

Signal Bloom's `layer.package.json` is a useful single-asset authoring example,
but the current package schema has one `asset` object and package validation
requires layer types to be unique across packages. Five package manifests
that all claim `circuitTrace` would therefore fail the existing contract.

For this family the honest show-ready seam is:

```text
one source-registered runtime type
  -> five standard catalog asset manifests
  -> five independent scene/mapping namespaces
```

No disabled optional-package activation is required. A future multi-asset
package contract should add an `assets` collection or explicitly permit
several package assets to share one runtime type before this family is
advertised as a Level 5 installable package.

## Stable Parameter Surface

```text
visible, speed, bpmSync, bpmMultiplier, alpha, seed, reseed,
autoReseed, autoReseedEveryBeats, behavior, agentCount, stepSize,
sensorDistance, turnChance, branchChance, deposit, decay, diffuse,
tracePersistence, traceWidth, glow, viaChance, backgroundAlpha,
trailAlpha, bgR, bgG, bgB, traceR, traceG, traceB
```

Keep these suffixes stable: scenes, MIDI mappings, and OSC mappings resolve
them through the selected asset's registry prefix.

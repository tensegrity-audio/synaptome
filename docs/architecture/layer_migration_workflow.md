# Layer Migration Workflow

Status: Active implementation guide, 2026-07-25.

This guide turns the Circuit Trace implementation lessons into a repeatable
path for modernizing existing Synaptome layers without breaking scenes,
mappings, or operator habits.

## Reference Shape

A migrated family has:

1. One runtime implementation for assets that genuinely share state and
   lifecycle.
2. Separate catalog assets only for distinct models, with presets used for
   cosmetic or behavioral variations.
3. Stable asset IDs, runtime types, registry prefixes, and parameter suffixes.
4. Deterministic owned randomness through a persisted `seed` plus momentary
   `reseed`; no process-global random generator.
5. Labeled parameter groups, deliberate ranges, and a complete catalog default
   for every registered parameter.
6. Explicit `model` identity and live `modes` that never secretly rebuild
   simulation state.
7. Static family validation, catalog/manifest regression, Release compilation,
   and one live visual/performance review before promotion.

## Safe Migration Sequence

```text
inventory current IDs and saved-scene targets
  -> freeze public IDs
  -> identify shared runtime families
  -> add deterministic lifecycle and complete defaults
  -> normalize labels/groups without renaming suffixes
  -> validate runtime declarations against every catalog asset
  -> regenerate catalog and parameter fixtures
  -> build and rehearse representative saved scenes
```

Existing aliases may remain in `configure()` for old catalog or scene shapes,
but new manifests should use the canonical scalar parameter names.

## Execution Priority

When the goal is faster creative iteration, improve the path in this order:

1. Run static catalog/default/registration checks before compiling.
2. Declare common public parameters through shared helpers instead of copying
   metadata and ranges into every layer.
3. Run a focused layer-family or single-layer native test.
4. Compile only the app or focused test project with project references
   disabled when the openFrameworks library is already current.
5. Launch the full app only for visual, interaction, device, and projection
   acceptance.

This order keeps obvious metadata failures out of the expensive C++ build and
keeps the full application launch focused on behavior that static tooling
cannot prove.

The current commands and profile format are documented in
[`../development/layer_authoring_validation.md`](../development/layer_authoring_validation.md).

Shared helpers do not make every common-looking control mandatory. In
particular, new layers should use the Console slot as the owner of whole-layer
visibility and opacity through `LayerDrawParams::slotOpacity`. A migration may
register legacy `visible` or `alpha` suffixes through the shared helper when
removing them would break scenes or mappings; that is compatibility, not a
pattern to copy into new work.

## Current Reference Families

| Runtime | Stable assets | Migration state |
| --- | --- | --- |
| `circuitTrace` | Circuit Slime, Mycelium, River, Ant Tunnels, Flow Field | New reference implementation |
| `agentField` | Ant Tunnels, Slime Mold, Physarum | Migrated: deterministic seed, complete defaults, stable IDs |
| `flocking` | Schooling, Murmuration | Migrated: deterministic seed, complete defaults, stable IDs |
| `gameOfLife` | Game of Life | Migrated distinct runtime: deterministic seed, canonical defaults, legacy aliases |
| `excitableMedia` | Excitable Media | Migrated distinct runtime: restored-seed rebuild, canonical defaults, legacy aliases |
| `lenia` | Lenia, Circuit Lenia | Shared continuous automaton with fixed catalog-selected organic/circuit presentations |

Validate these families with:

```powershell
python tools\validate_modular_layer_families.py
python tools\validate_cellular_fields.py
```

## What Not To Do

- Do not rename an asset or parameter merely for cosmetic consistency.
- Do not turn a different algorithm into a mode.
- Do not create extra catalog layers for palettes or minor tuning; use presets.
- Do not hide OSC, MIDI, audio, or sensor ownership inside a layer.
- Do not claim an installable package when source registration is still
  required.

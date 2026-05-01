# Synaptome Public Contract Gap Register

Status: active follow-up register for the public Synaptome app/runtime boundary.

Source: [`docs/contracts/README.md`](README.md), `python tools\validate_configs.py --public-app`, and the public architecture docs.

This file tracks known public contract debt that remains after the first app/runtime boundary lock. These are visible follow-up items, not unknown blockers.

## Reading This Register

- `Missing Validator`: the source exists, but no dedicated validation command owns it yet.
- `Missing Fixture`: a validator or schema exists, but there is not enough golden fixture coverage.
- `Policy Gap`: ownership, versioning, or migration policy is not locked.
- `Runtime Child`: resolution depends on app runtime work, especially scene-load isolation.

## Current Public Gaps

| ID | Gap | Status | Scope | Why It Matters | Next Action | Blocks |
| --- | --- | --- | --- | --- | --- | --- |
| CG-01 | Parameter ID manifest and target references | Strict Validator Added | `app-contract` | Parameter IDs are snapshotted, and strict target validation checks committed scene, OSC, MIDI, HUD, audio, and device-map references against the manifest, Console slot templates, and layer catalog IDs. | Keep the fixture-backed strict gate in `validate_configs.py --public-app`; extend it when new target-bearing config surfaces are promoted. | Confident public parameter renames and mappings. |
| CG-02 | Layer asset golden fixtures | Validator Added, SDK Fixture Added | `app-contract` | The layer catalog snapshot mirrors `LayerLibrary` ingestion and checks runtime types against factory registrations. The artist SDK fixture adds a minimal source/catalog/scene proof. | Use the catalog and SDK fixtures to drive the public layer authoring guide and future package boundary. | Layer runtime extraction and public SDK proof. |
| CG-03 | Scene persistence round-trip fixture | Static Validator Added, App-Native Fixture Follow-up | `app-contract`, `app-runtime` | Static scene fixtures prove saved JSON shape, catalog references, scalar/modifier persistence, slot bounds, effect/global keys, and canonical JSON stability. Runtime staged apply/rollback still protects show stability. | Add app-native staged-load/rollback fixtures when the runtime test seam is ready. Use `--live` only for intentional local scene-state smoke checks. | Scene schema tightening and scene-load safety. |
| CG-04 | Device-map logical slot fixture | Validator Added | `app-contract` | Device-map regression covers current MIDI Mix logical slots plus a synthetic controller fixture, role families, sensitivity range, MIDI binding shape, and duplicate physical bindings. | Extend it when target/action binding semantics become public. | Controller/device adapter confidence. |
| CG-05 | HUD layout/feed schema | Validator Added, Runtime Fixture Follow-up | `app-contract`, `app-runtime` | HUD fixture coverage snapshots widget identity, declared feed IDs, Browser HUD preferences, and Console overlay placements while leaving dynamic feed payloads runtime-local. | Extend only when feed payload schemas become public. Use `--live` for operator-state smoke checks. | UI shell extraction and scene-load snapshot proof. |
| CG-06 | Console layout and secondary display persistence | Validator Added, Runtime Fixture Follow-up | `app-contract`, `app-runtime` | Console/display validation checks the eight-slot inventory, layer references, overlay flags, display preference shape, slot assignment shape, and HUD placement shape. | Add app-native slot/display transaction fixtures when the runtime test seam is ready. | Control-window stability. |
| CG-07 | Public parameter vocabulary | Drafted, SDK Fixture Added | `app-contract` | Reusable layer parameters have a first vocabulary and the artist SDK fixture enforces selected suffix/type families. Broader range/unit policy remains advisory. | Expand the layer authoring guide from the SDK fixture, then tighten suffix/range rules after real examples agree. | Public layer authoring and reusable scene ecosystem. |
| CG-08 | Media catalog auto-discovery policy | Policy Gap, Missing Fixture | `app-contract` | Media layers exist, but clips are currently cataloged through `config/videos.json`; the desired public workflow may include folder scanning. | Decide whether Synaptome uses folder scanning, explicit manifests, or both; add fixtures for media discovery and Browser visibility. | Public media workflow and example content packaging. |
| CG-09 | External device/display contract map | Policy Gap, Missing Validator | `app-contract` | MIDI, OSC, audio, webcam, media, display, hotkey, and helper input boundaries exist, but they are not yet schemas/fixtures with one consistent ownership model. | Promote `docs/architecture/synaptome_external_contracts.md` into schemas/fixtures for each public boundary. | Public Synaptome packaging and helper seams. |
| CG-10 | Host audio input contract | Missing Validator | `app-contract`, `app-runtime` | Local mic capture behaves like a sensor source and emits `/sensor/host/localmic/*`, but `config/audio.json` is not schema-backed or contract-indexed. | Add audio config schema/fixture and define local mic persistence policy. | Public audio-reactive layer workflow. |
| CG-11 | Artist SDK compatibility slice | Registration Decision Locked, Packaging Follow-up | `artist-sdk` | The layer API, parameter registry, factory, catalog, media primitives, and validation tools now have a minimal public source/catalog/scene fixture. First public Synaptome can honestly ship a source-registration SDK path. | Promote the fixture into the layer authoring guide, keep source registration language honest, and later implement generated registration, a plugin manifest, or a module loader. | Public Synaptome repo readiness. |

## Priority Order

1. Scene/display runtime transaction fixtures (`CG-03`) because static scene persistence is locked, but staged apply/rollback still protects show stability.
2. Layer SDK authoring guide follow-through (`CG-02`, `CG-11`) because the fixture now defines the creative contract surface and the first public registration path is explicit.
3. External device/media/audio/display contracts (`CG-08`, `CG-09`, `CG-10`) because they define Synaptome's public outside-world seams.
4. Parameter vocabulary tightening (`CG-07`) after example content proves the reusable naming policy.

## Outside This Public Runtime

Radio decode, embedded firmware, generated hardware config, embedded UI catalog exchange, and legacy payload quarantine belong in future helper or radio-contract packages. Public Synaptome should consume their app-facing messages, not ship their implementation details.

## Current Coverage Snapshot

As of the current public app contract snapshot, `python tools\validate_configs.py --public-app` reports:

```text
validated=10
```

The strict suite uses committed fixtures under `tools/testdata/runtime_state/**` for scene/config/operator state. Manual app runs can still dirty `synaptome/bin/data/**`; those files are local runtime evidence, not the strict contract source of truth.

The goal is not to force every gap closed before all future work. The goal is that future work names which gap it is closing, which validation it adds, and which extraction gate it unblocks.

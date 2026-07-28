# Synaptome Public Runtime Contract Roadmap

Status: Planned supporting architecture; reviewed 2026-07-18. Current priority
and execution state are owned by
[`../project_ops/roadmap.md`](../project_ops/roadmap.md). This document owns
public app/runtime contract gaps that are not part of the layer system roadmap
or the transport/reactivity roadmap.

Read with: [`synaptome_layer_system_roadmap.md`](synaptome_layer_system_roadmap.md),
[`synaptome_transport_reactivity.md`](synaptome_transport_reactivity.md),
[`synaptome_external_contracts.md`](synaptome_external_contracts.md), and
[`../contracts/README.md`](../contracts/README.md).

## Purpose

Synaptome has public contracts beyond layers: scenes, console state, HUD layout,
device maps, media catalogs, host audio config, and external adapter boundaries.

This roadmap keeps those non-layer contracts visible without mixing them into
the layer package roadmap.

## What Belongs Here

- Parameter target validation that cuts across scenes, mappings, HUD, audio,
  and device maps.
- Scene persistence and staged-load/rollback fixtures.
- Device-map logical slot contract.
- HUD layout and feed schema follow-ups.
- Console layout and secondary display persistence.
- Media catalog discovery policy outside the layer package mechanism.
- External device/display contract map.
- Host audio input config and persistence policy.

## What Does Not Belong Here

- Layer package layout, package params, presets, Browser layer discovery, or
  single-layer bench work. Those live in
  [`synaptome_layer_system_roadmap.md`](synaptome_layer_system_roadmap.md).
- BPM, beat detection, clock source selection, and transport confidence. Those
  live in [`synaptome_transport_reactivity.md`](synaptome_transport_reactivity.md).

## Tracked Runtime Gaps

These gaps are folded into this roadmap. `docs/contracts/contract_gaps.md`
indexes them, but this document owns their meaning and next actions.

| ID | Gap | Current State | Next Action |
| --- | --- | --- | --- |
| CG-01 | Parameter ID manifest and target references | Strict target validation checks committed scenes, OSC, MIDI, HUD, audio, and device-map references against the generated manifest, Console slot templates, and layer catalog IDs. | Keep the fixture-backed strict gate in `validate_configs.py --public-app`; extend it when new target-bearing config surfaces are promoted. |
| CG-03 | Scene persistence round-trip fixture | Static scene fixtures prove saved JSON shape, catalog references, scalar/modifier persistence, slot bounds, effect/global keys, and canonical JSON stability. | Add app-native staged-load/rollback fixtures when the runtime test seam is ready; use `--live` only for intentional local scene-state smoke checks. |
| CG-04 | Device-map logical slot fixture | Device-map regression covers current MIDI Mix logical slots plus a synthetic controller fixture, role families, sensitivity range, MIDI binding shape, and duplicate physical bindings. | Extend it when target/action binding semantics become public. |
| CG-05 | HUD layout/feed schema | HUD fixture coverage snapshots widget identity, declared feed IDs, Browser HUD preferences, and Console overlay placements while dynamic feed payloads remain runtime-local. | Extend only when feed payload schemas become public; use `--live` for operator-state smoke checks. |
| CG-06 | Console layout and secondary display persistence | Console/display validation checks eight composition-slot inventory, layer references, overlay flags, display preference shape, and HUD placement shape. Logical hardware control slots are validated by machine-profile v1. | Add app-native composition-slot/display transaction fixtures when the runtime test seam is ready. |
| CG-08 | Media catalog intake and discovery policy | Manifest-only intake is now locked through `config/videos.json`; the empty baseline, schema, provenance/hash/replacement rules, public/local roots, and negative fixtures are validated. Folder scanning is deferred. | Add one reviewed redistributable media asset and prove Browser visibility plus slot loading without adding folder scanning. |
| CG-09 | External device/display contract map | MIDI, OSC, audio, webcam, media, display, hotkey, and helper input boundaries exist, but not all are schemas/fixtures with one consistent ownership model. | Promote remaining boundaries from `synaptome_external_contracts.md` into schemas/fixtures. |
| CG-10 | Host audio input contract | Local mic capture behaves like a sensor source and emits `/sensor/host/localmic/*`, but `config/audio.json` is not schema-backed or contract-indexed. | Add audio config schema/fixture and define local mic persistence policy. |

## Roadmap

1. Keep the strict public-app validator passing while layer package work evolves.
2. Keep CG-08 manifest-only intake passing; add one reviewed asset before
   promoting Browser visibility or runtime slot-load fixtures.
3. Add app-native scene staged-load/rollback fixtures when the runtime test seam
   is ready.
4. Extend device-map fixtures when target/action binding semantics become
   public.
5. Promote remaining external boundaries into schemas and fixtures.
6. Add host audio config schema and local mic persistence policy.
7. Add runtime fixtures only when they can run deterministically enough to be
   useful in CI or explicit local smoke mode.

The first CG-08 slice is complete: explicit manifests are canonical, folder
scanning is deferred, roots and provenance/replacement rules are defined, and
the dangling `default-loop` entry has been removed rather than silently
replaced. Folder scanning can be proposed later with its own conflict, refresh,
and Browser-visibility fixtures.

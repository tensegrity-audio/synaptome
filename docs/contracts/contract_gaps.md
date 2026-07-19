# Synaptome Contract Gap Index

Status: index only. Detailed gap ownership has moved into the relevant
roadmaps, so this file no longer acts as a parallel debt register.

Source: [`docs/contracts/README.md`](README.md),
`python tools\validate_configs.py --public-app`, and the public architecture
roadmaps.

## Purpose

Use this file to answer:

```text
Which roadmap owns this kind of missing contract work?
```

Do not add detailed next-action tables here. Put detailed gap status, next
actions, and sequencing in the owning roadmap.

Gap ownership does not imply active priority. Current priority and promotion
state live only in [`../project_ops/roadmap.md`](../project_ops/roadmap.md).

## Roadmap Ownership

| Area | Gap IDs | Owning Roadmap |
| --- | --- | --- |
| Layer catalog, package system, package params, presets, mapping presets, folder/file discovery, and single-layer validation | `CG-02`, `CG-07`, `CG-11` through `CG-18` | [`../architecture/synaptome_layer_system_roadmap.md`](../architecture/synaptome_layer_system_roadmap.md) |
| Transport, BPM, beat detection, clock source, confidence, onset/downbeat, and mapping-visible timing events | `CG-19` | [`../architecture/synaptome_transport_reactivity.md`](../architecture/synaptome_transport_reactivity.md) |
| Public runtime contracts outside the layer system: scene persistence, parameter targets, device maps, HUD, console/display, media discovery, external boundaries, and host audio config | `CG-01`, `CG-03` through `CG-06`, `CG-08` through `CG-10` | [`../architecture/synaptome_public_runtime_contract_roadmap.md`](../architecture/synaptome_public_runtime_contract_roadmap.md) |

## Status Labels

Roadmaps may continue to use these shared labels:

- `Missing Validator`: the source exists, but no dedicated validation command
  owns it yet.
- `Missing Fixture`: a validator or schema exists, but golden fixture coverage
  is not sufficient.
- `Policy Gap`: ownership, versioning, migration, or fallback policy is not
  locked.
- `Runtime Child`: resolution depends on runtime work or an app-native test
  seam.

## Current Coverage Snapshot

As of the current public app contract snapshot,
`python tools\validate_configs.py --public-app` reports:

```text
validated=12
```

Strict public contract mode reads committed fixtures under `tools/testdata/**`
and examples under `docs/examples/**`. Live app-written files under
`synaptome/bin/data/config/` and `synaptome/bin/data/layers/scenes/` remain
runtime smoke state unless intentionally promoted into fixtures.

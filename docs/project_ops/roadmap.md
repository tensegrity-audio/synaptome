# Synaptome Project Ops Roadmap

This roadmap is the Project Ops request index for Synaptome. It is not the public runtime feature documentation and does not replace `docs/release_policy.md`.

## In Progress

- **Layer Package And Compatibility Bench Scaffolding** ([Request Doc](in_progress/layer_package_compatibility_bench_scaffolding.md))
State Summary
- Phase: INTAKE
- Status: Draft
- Steps Complete: 3 / 10
- Progress: Draft package/preset schemas, shared package discovery roots, a Signal Bloom package fixture, package-derived snapshots, opt-in combined catalog/manifest checks, one STL/model generated-layer template fixture, static and dynamic option metadata fixtures, and a schema-checked read-only Browser inspection payload now exist without changing runtime loading behavior.
- Last Step Outcome: 2026-06-28 - Added one generated-layer `optionsSource` metadata fixture and proved it flows into generated catalog plus inspection snapshots without adding provider lookup, Browser UI, or runtime scanning.
- Next Step: Expand only in tiny, schema-checked slices, such as one package-owned option metadata example or one additional generated content kind, then consider read-only Browser UI behind an explicit draft path.
- Dependencies / Overlap: `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`, `docs/contracts/parameter_manifest.json`, `docs/schemas/layer_asset.schema.json`, `tools/layer_catalog_regression.py`, `tools/gen_parameter_manifest.py`.
- Blocking Issues / Unknowns: Runtime bench shape depends on an app-native or offscreen layer test seam; generated registration/module loading remains a later architecture decision; beat-reactive mapping presets depend on the separate transport/reactivity contract.
- Impact / Priority Notes: Promotes package-driven layer authoring, folder/file discovery, visible mapping defaults, preset banks, and validation/bench work into trackable layer-system scaffolding without claiming hot-loaded plugins.
- Resume From: Intake; keep the read-only inspection payload passing, then add the smallest next metadata expansion without touching Browser/runtime behavior.

## Completed

- **Project Ops Compatibility** ([Request Doc](completed/project_ops_compatibility.md))
State Summary
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 4 / 4
- Progress: Synaptome now has a namespaced Project Ops operating surface under `docs/project_ops/**`, CI checks out the external Project Ops repo at `v0.1.2`, changed Project Ops request docs are audited remotely, and Synaptome's contract report is public-runtime-only.
- Last Step Outcome: 2026-05-05 - Added remote Project Ops changed-request audit workflow and removed stale Tensegrity process-contract entries from Synaptome's contract report.
- Next Step: Use the request template for future public-runtime work.
- Dependencies / Overlap: `.project_ops/config.json`, `docs/project_ops/README.md`, `docs/project_ops/roadmap.md`, `docs/project_ops/reports/changelog.md`, `docs/project_ops/governance/README.md`, `docs/contributing.md`, `docs/release_policy.md`.
- Blocking Issues / Unknowns: None.
- Impact / Priority Notes: Gives Synaptome a Project Ops-compatible administrative surface without importing Tensegrity history or replacing public runtime docs.
- Project Ops / Roadmap Updates (timestamped): 2026-05-04 - Added namespaced Project Ops operating surface and passed adapter audit. 2026-05-05 - Synaptome CI now checks out the external Project Ops repo at `v0.1.1` before public runtime validation. 2026-05-05 - Added remote Project Ops changed-request audit workflow and pruned stale Tensegrity process-contract entries from `tools/validate_configs.py --contracts`.
- Resume From: Complete; future work starts from a new request artifact.
Request Doc: docs/project_ops/completed/project_ops_compatibility.md

## Backlog

- [3D Arctic Aurora Scene Roadmap](arctic_aurora_3d_scene_roadmap.md) tracks the integrated camera-space arctic scene layer, including the 3D sea plane, floating iceberg meshes, audio-scoped aurora volume, and today-scope milestones.
- [Aurora Generative Layers Roadmap](aurora_generative_layers_roadmap.md) tracks waveform-first aurora layers, spectrum/FFT upgrade work, and audio-reactive curtain controls.
- [Cosmic Generative Layers Roadmap](cosmic_generative_layers_roadmap.md) tracks show-oriented cosmic layer priorities, audio/sensor mappings, and external simulation-library candidates.
- [Transport And Reactivity Contract](../architecture/synaptome_transport_reactivity.md) tracks non-layer BPM, beat detection, clock source, confidence, onset/downbeat, and mapping-visible timing source work.
- [Public Runtime Contract Roadmap](../architecture/synaptome_public_runtime_contract_roadmap.md) tracks non-layer scene, HUD, console/display, media, device, external boundary, and host audio contract follow-up.
- Use request artifacts from `docs/project_ops/in_progress/_REQUEST_TEMPLATE.md` when Project Ops-managed work begins.
- Project Ops adapter and changed-request audits now run in Synaptome CI through the external `tensegrity-audio/project_ops` checkout pinned at `v0.1.2`.
- Add release-note templates only if Synaptome starts publishing release notes beyond `docs/release_policy.md`.

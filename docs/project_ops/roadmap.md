# Synaptome Project Ops Roadmap

Status: Active project index, reviewed 2026-07-18.

This file is the single source of truth for current priority, execution state,
and the next safe promotion point. Architecture roadmaps describe sequencing
inside a subsystem; show-development roadmaps preserve visual direction. They
do not become active commitments until they are promoted here through a
Project Ops request.

This roadmap does not replace the public runtime documentation or
[`docs/release_policy.md`](../release_policy.md).

## Current Objective

Status: Achieved 2026-07-18.

Synaptome has reached a clean, deterministic pre-media baseline before
generating or importing more tracked media or adding another show-content
family.

The baseline is intentionally narrower than runtime package loading. It means:

```text
roadmap ownership is unambiguous
  -> current package/generated-layer fixtures are deterministic
  -> canonical runtime contracts still pass unchanged
  -> media ownership, IDs, paths, and discovery policy are explicit
  -> new media can be added without hiding architecture debt or changing runtime behavior
```

## Active Work

### Layer Package And Compatibility Bench Scaffolding

State Summary
- Request ID: layer_package_compatibility_bench_scaffolding
- Phase: EXECUTION
- Status: In Progress
- Steps Complete: 3 / 10 (3 additional steps in progress)
- Progress: Draft package/preset schemas, shared package discovery roots, a Signal Bloom package fixture with package-owned named options, package-derived snapshots, opt-in combined catalog/manifest checks, one STL/model generated-layer template fixture, static and dynamic option metadata fixtures, and a schema-checked read-only Browser inspection payload now exist without changing runtime loading behavior.
- Last Step Outcome: 2026-07-18 - Added package-owned named BPM multiplier options to Signal Bloom and regenerated deterministic package, combined compatibility, and inspection snapshots.
- Next Step: Hold at the static safe boundary; promote either one package-owned `optionsSource` fixture or read-only Browser inspection through a separate explicit slice before changing runtime behavior.
- Dependencies / Overlap: `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`, `docs/contracts/parameter_manifest.json`, `docs/schemas/layer_asset.schema.json`, `tools/layer_catalog_regression.py`, `tools/gen_parameter_manifest.py`.
- Primary Scope: contracts
- Secondary Scopes: artist-sdk, docs, tests, runtime
- Blocking Issues / Unknowns: Runtime bench shape depends on an app-native or offscreen layer test seam; generated registration/module loading remains a later architecture decision; beat-reactive mapping presets depend on the separate transport/reactivity contract.
- Impact / Priority Notes: Establishes deterministic package and generated-content contracts before more tracked media, Browser activation, or runtime discovery increases compatibility risk.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: Met for bounded schema, fixture, snapshot, and documentation slices; runtime activation remains out of scope.
- Project Ops / Roadmap Updates (timestamped): 2026-06-25 - Promoted layer package scaffolding from backlog. 2026-07-18 - Reconciled the request with the current Project Ops readiness contract and pre-media gate. 2026-07-18 - Completed the bounded package-owned static option slice and paused before runtime promotion.
- Resume From: Execution; choose one bounded follow-up, record fresh validation evidence, and do not touch Browser activation or runtime loading behavior without explicit promotion.
Request Doc: docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md

Task breakdown: 3 done, 3 in progress, and 4 planned. Browser activation,
package-root scanning, canonical manifest promotion, and loader work remain
outside the current bounded slice.

## Pre-Media Safety Gate

Gate status: Complete 2026-07-18. New tracked media must now enter through one
bounded asset request. Exploratory local files remain outside the committed
catalog, scene, and fixture roots.

### 1. Roadmap And Ownership

- [x] One master roadmap owns current priority and task state.
- [x] Architecture roadmaps are supporting sequence documents, not competing
  backlogs.
- [x] Superseded and parked show roadmaps are labeled explicitly.
- [x] Update this roadmap and the active request together after the next
  package-contract slice.

### 2. Contract Baseline

- [x] Keep the read-only Browser inspection payload deterministic and
  schema-valid.
- [x] Keep package-only and combined package/runtime snapshots passing.
- [x] Keep the canonical layer catalog and parameter manifest unchanged unless
  a separately approved promotion request says otherwise.
- [x] Run the public-app contract gate and record the commands and result in the
  active request.

Verified 2026-07-18. Contract cleanup is green.

### 3. Media Intake Policy

Policy: explicit manifests are canonical and folder scanning is deferred. That
matches current runtime behavior and gives generated assets a reviewable ID,
path, provenance record, and replacement history before the Browser discovers
anything automatically.

Locked root split:

- `synaptome/bin/data/media/public/**`: tracked, redistributable runtime media.
- `synaptome/bin/data/media/local/**`: operator-local media ignored by source
  control and forbidden in the committed catalog.
- `docs/examples/media_catalog_example.json`: public manifest-shape example
  with no binary media.
- `tools/testdata/media_catalog/**`: semantic fixtures with no production
  media.

The dangling `default-loop` entry was removed without generating a silent
replacement. `videos.json` is now an explicit, valid empty baseline.

- [x] Lock CG-08's first policy to explicit manifests; folder scanning is not
  implied.
- [x] Define the allowed roots for tracked fixtures, distributable example
  media, and operator-local/show media.
- [x] Define stable media IDs and filenames that do not depend on absolute
  paths or generation timestamps.
- [x] Require source/provenance, license, generation notes, and metadata needed
  to reproduce or replace each committed generated asset.
- [x] Define duplicate, replacement, and deletion behavior before a generated
  asset is referenced by a catalog entry, preset, or scene.

### 4. Safe Promotion Point

The pre-media cleanup is complete. The dedicated media validator and public-app
suite pass with an empty manifest-only catalog. The next media request must
contain one content family, one public destination, one stable naming rule, one
manifest entry, complete provenance, and validation evidence. Do not combine it
with Browser activation or runtime folder scanning.

## Horizon

### Next: One Reviewed Media Asset

1. Open a request for one asset and one intended use.
2. Generate or import it into `synaptome/bin/data/media/public/**`.
3. Record stable ID, revision, SHA-256, creator/source, license,
   redistribution, and generation details where applicable.
4. Add exactly one manifest entry and run the media/public contract gates.
5. Only then add Browser visibility and runtime slot-load evidence for that
   asset.

### Then: Read-Only Product Surface

1. Add read-only Browser inspection behind an explicit draft path.
2. Prove that inspection does not instantiate layers, mutate scenes, resolve
   dynamic providers, or rewrite stored values.
3. Add a focused static package command for author feedback.

### Later: Controlled Runtime Promotion

1. Add opt-in package activation with scene save/load and target-validation
   coverage.
2. Promote package outputs into canonical catalog/manifest contracts only when
   runtime registration can be compared against package declarations.
3. Add disabled-by-default package/content-root scanning with strict conflict
   handling.
4. Add the single-layer runtime/offscreen bench.
5. Consider generated registration or a loader only after the bench and a
   compatibility policy exist.

## Supporting Roadmaps

| Roadmap | State | Role | Resume condition |
| --- | --- | --- | --- |
| [Layer system](../architecture/synaptome_layer_system_roadmap.md) | Active support | Package, discovery, presets, mappings, Browser, validation, and registration sequence. | Drives the active scaffolding request. |
| [Public runtime contracts](../architecture/synaptome_public_runtime_contract_roadmap.md) | Planned support | Non-layer contracts, including media discovery and host audio. | Media intake is safe; promote one reviewed asset next, while other runtime gaps remain planned. |
| [Transport and reactivity](../architecture/synaptome_transport_reactivity.md) | Planned dependency | Beat source, confidence, onset/downbeat, and fallback contract. | Required before beat-reactive mapping presets become canonical. |
| [Biological layers](../architecture/synaptome_biological_layer_roadmap.md) | Parked | Long-range simulation and layer-family direction. | Resume after shared field/graph infrastructure and package validation are ready. |
| [Arctic Aurora scene](arctic_aurora_3d_scene_roadmap.md) | Ready for closeout | Existing scene polish and validation only. | Run the documented polish/validation pass; do not expand scope. |
| [Aurora generative layers](aurora_generative_layers_roadmap.md) | Parked | Waveform-first show-development direction. | Resume with one Waveform Aurora Curtains request after the pre-media gate. |
| [Cosmic generative layers](cosmic_generative_layers_roadmap.md) | Historical/superseded | Preserves earlier concepts and the remaining Cosmos Formation tuning notes. | Resume only through a new focused request after the flagship look is approved. |

## Completed

### Media Manifest Intake Contract

State Summary
- Request ID: media_manifest_intake_contract
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 5 / 5
- Progress: Synaptome now has a validated manifest-only, zero-asset media baseline with explicit roots, stable IDs, SHA-256, provenance, generated-media metadata, replacement rules, and negative fixtures.
- Last Step Outcome: 2026-07-18 - Removed the dangling default clip, added the media schema/policy/validator/fixtures, and passed the public-app contract suite with 12 validated contracts.
- Next Step: Open one bounded request for one reviewed redistributable asset; do not combine it with folder scanning, Browser activation, or package loading.
- Dependencies / Overlap: `docs/project_ops/roadmap.md`, `docs/architecture/synaptome_public_runtime_contract_roadmap.md`, `docs/contracts/media_catalog.md`, `docs/schemas/media_catalog.schema.json`, `synaptome/bin/data/config/videos.json`, `tools/media_catalog_regression.py`.
- Primary Scope: contracts
- Secondary Scopes: docs, tests, runtime
- Blocking Issues / Unknowns: None for safe manifest intake; the first actual asset still needs creative selection, redistribution review, and provenance.
- Impact / Priority Notes: Prevents undocumented or missing media from entering the public runtime and establishes a reproducible gate before generated media work resumes.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: Met; the request was limited to contract, validation, and empty-baseline changes with no media generation or folder scanning.
- Project Ops / Roadmap Updates (timestamped): 2026-07-18 - Promoted the pre-media policy gap, implemented manifest-only intake, validated it, and closed the request in one bounded cleanup pass.
- Resume From: Phase COMPLETE, State Complete, Next Action create a new request from `docs/project_ops/in_progress/_REQUEST_TEMPLATE.md` for one reviewed asset.
Request Doc: docs/project_ops/completed/media_manifest_intake_contract.md

### Project Ops Compatibility

Request: [project_ops_compatibility.md](completed/project_ops_compatibility.md)

- Phase: Complete
- Status: Complete
- Outcome: Synaptome has a namespaced Project Ops surface, CI uses the external
  Project Ops checkout pinned at `v0.1.2`, changed request docs are audited,
  and the contract report remains public-runtime-only.
- Completed: 2026-05-05
- Follow-up: start future public-runtime work from the request template.

## Operating Rule

Use [the request template](in_progress/_REQUEST_TEMPLATE.md) when work begins.
An item is not active merely because it appears in a supporting roadmap. Keep
the request state, this index, validation evidence, and related public docs in
sync through closeout.

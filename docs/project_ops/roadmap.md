# Synaptome Project Ops Roadmap

Status: Active project index, reviewed 2026-07-24.

This file is the single source of truth for current priority, execution state,
and the next safe promotion point. Architecture roadmaps describe sequencing
inside a subsystem; show-development roadmaps preserve visual direction. They
do not become active commitments until they are promoted here through a
Project Ops request.

This roadmap does not replace the public runtime documentation or
[`docs/release_policy.md`](../release_policy.md).

## Current Objective

Status: Active 2026-07-24.

Bring the existing app to a show-safe operating point before expanding package
mapping, media, or visual scope. The immediate objective is predictable scene
round-tripping, clear operator feedback, consistent controls across Browser,
Console, and HUD surfaces, navigable recovery paths, and stable performance on
the show machine.

The current safe sequence is:

```text
remove output-preserving runtime waste
  -> prove visible parameter values are what scenes persist
  -> expose active/dirty/load/save state consistently
  -> remove navigation dead ends and ambiguous ownership
  -> rehearse the heaviest show scene and recovery path
  -> resume package mapping and new media only after the show-safe checkpoint
```

## Active Work

### Show Readiness And Operator Stability

State Summary
- Request ID: show_readiness_operator_stability
- Phase: EXECUTION
- Status: In Progress
- Progress: Removed unused per-frame render passes and eager effect buffers;
  scene serialization now captures live unmodulated float, bool, and string
  values while preserving modifier-owned base values; operator text scaling
  now reaches every app surface and uses target-size GNU Unifont rendering;
  Mirror modes 0/1 are lossless full-frame flips.
- Last Step Outcome: 2026-07-24 - Bundled GNU Unifont 17.0.05 and moved the
  shared operator renderer from fractional bitmap enlargement to cached
  target-size rasterization, with rendered-width measurement and a built-in
  bitmap fallback. Release and BrowserFlow builds, all 21 BrowserFlow
  scenarios, all 13 public-app contracts, an eight-second runtime smoke, and
  a 2.22-second unchanged incremental build pass.
- Next Step: Finish unsaved-change/save-result feedback, then audit keyboard
  navigation and rehearse visual/recovery behavior on the show machine.
- Priority Lane: Show Blocker
- Ready State: Ready
- Resume From: Scene status and dirty-state ownership.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`

Show-safe checklist:

- [x] Remove output-preserving full-frame render waste.
- [x] Persist the visible live value for unmodulated scene parameters.
- [x] Preserve the underlying base and modifier stack for modulated parameters.
- [x] Scale operator text and row spacing consistently across all app surfaces.
- [x] Rasterize operator text at its requested size instead of scaling a bitmap.
- [x] Preserve the complete source frame in Mirror's horizontal/vertical modes.
- [ ] Prove save → mutate → reload on the show machine with the heaviest scene.
- [ ] Show active scene, unsaved changes, and last load/save result consistently.
- [ ] Audit Browser, Console, and HUD navigation for dead ends and conflicting labels.
- [ ] Rehearse restart, missing-device, and failed-scene recovery.

### Layer Package And Compatibility Bench Scaffolding

State Summary
- Request ID: layer_package_compatibility_bench_scaffolding
- Phase: EXECUTION
- Status: Paused for show stabilization
- Steps Complete: 8 / 10 (2 additional steps in progress)
- Progress: The Browser now provides reusable labeled dropdowns on matching live package parameters for both static `options[]` and registered `optionsSource` choices, alongside the operator-local package preset-bank picker. Selections update the existing live/base parameter value, while unavailable provider values are preserved until the operator explicitly replaces them.
- Last Step Outcome: 2026-07-24 - Added reusable live labeled parameter selection, including `Half Time` / `Normal` / `Double Time` for BPM and `Compact` / `Default` / `Full` for Scale, with provider-revision cancellation, unavailable-value preservation, telemetry, and scene/mapping/inspection isolation coverage.
- Next Step: After the show-safe checkpoint, add an explicit package
  mapping-preset preview/apply/edit flow with slot expansion, conflict handling,
  rollback, and no automatic mapping installation.
- Dependencies / Overlap: `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`, `docs/contracts/parameter_manifest.json`, `docs/schemas/layer_asset.schema.json`, `tools/layer_catalog_regression.py`, `tools/gen_parameter_manifest.py`.
- Primary Scope: contracts
- Secondary Scopes: artist-sdk, docs, tests, runtime
- Blocking Issues / Unknowns: Generated registration/module loading remains a later architecture decision; mapping-preset editing still needs explicit Browser ownership; beat-reactive mappings depend on the separate transport/reactivity contract.
- Impact / Priority Notes: Establishes deterministic package and generated-content contracts before more tracked media, Browser activation, or runtime discovery increases compatibility risk.
- Priority Score: N/A
- Priority Lane: Deferred behind Show Blocker
- Ready State: Ready
- Ready Gate: Met for read-only inspection, vetted opt-in source registration, operator-local next-load preset selection, and explicit labeled live parameter selection; automatic discovery and mapping activation remain out of scope.
- Project Ops / Roadmap Updates (timestamped): 2026-06-25 - Promoted layer package scaffolding from backlog. 2026-07-18 - Reconciled the request with the current Project Ops readiness contract and pre-media gate. 2026-07-18 - Completed the bounded package-owned static option slice and paused before runtime promotion. 2026-07-24 - Converged the reviewed runtime adapter, added package-owned dynamic option metadata and unavailable-value policy, completed the live-window smoke, rendered read-only option/provider state in the Browser, resolved the app-owned transport provider with missing-value preservation, added operator-local next-load preset-bank selection, and promoted static/runtime choices into explicit live labeled dropdowns.
- Resume From: Execution; define the preview/apply/rollback boundary for package mapping presets.
Request Doc: docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md

Task breakdown: 8 done and 2 in progress. Mapping controls, automatic
package-root scanning, and loader work remain outside the safe baseline.

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

### Complete: Converge The Vertical Slice

1. The reviewed optional catalog adapter is generated from the package and
   checked for byte-level drift.
2. Signal Bloom owns a dynamic `optionsSource` fixture, and the inspection
   payload requires stored unavailable values to be preserved and marked.
3. Signal Bloom and Aurora Veil both load in a live window; native, static, and
   public-app gates are green.

### Next: Finish Browser Ownership

1. Named and runtime-provider choices now render read-only without rewriting
   stored values; unavailable defaults are preserved and marked.
2. Preset-bank selection now persists stable IDs locally and applies on the
   next layer load without rewriting active scene or mapping state.
3. Live package parameters now use a reusable labeled dropdown for static and
   runtime-provider choices, preserving unavailable values until explicit edit.
4. Add an explicit mapping-preset preview/apply/edit flow; suggestions remain disabled
   until the operator acts.

### Later: Installation Evolution

1. Add disabled package/content-root discovery with strict duplicate handling.
2. Replace the hand-edited source registration with generated registration or
   a versioned module mechanism.
3. Consider binary/plugin loading only after dependency and ABI policy exist.
4. Add another media or show-content family only through a new bounded request.

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

### Aurora Veil Public Media Asset

State Summary
- Request ID: aurora_veil_public_media
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 5 / 5
- Progress: One generated visual source was reviewed, encoded as a public video loop, hashed, cataloged with complete provenance, and assigned to the existing Browser-visible media layer.
- Last Step Outcome: 2026-07-19 - The media catalog and public-app contract gates passed with `aurora-veil-r1` as the sole tracked clip.
- Next Step: Do not add another media family until there is a specific artistic need; keep folder scanning deferred.
- Dependencies / Overlap: `docs/contracts/media_catalog.md`, `synaptome/bin/data/config/videos.json`, `synaptome/bin/data/layers/media/clip_default.json`, `tools/media_catalog_regression.py`.
- Primary Scope: media
- Secondary Scopes: contracts, runtime, artist-sdk
- Blocking Issues / Unknowns: None for this asset. Visual acceptance in a live projection remains an artistic review, not a contract blocker.
- Impact / Priority Notes: Closes the empty-catalog horizon item without reopening discovery or provenance ambiguity.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: Met by the completed pre-media safety gate and explicit user authorization to execute the full cleanup sequence.
- Project Ops / Roadmap Updates (timestamped): 2026-07-19 - Opened and completed one bounded asset intake after the pre-media gate; synchronized the master roadmap, media contract, and changelog.
- Resume From: Complete; start a new bounded request for any future media asset.
Request Doc: docs/project_ops/completed/aurora_veil_public_media.md

### Media Manifest Intake Contract

State Summary
- Request ID: media_manifest_intake_contract
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 5 / 5
- Progress: Synaptome now has a validated manifest-only, zero-asset media baseline with explicit roots, stable IDs, SHA-256, provenance, generated-media metadata, replacement rules, and negative fixtures.
- Last Step Outcome: 2026-07-18 - Removed the dangling default clip, added the media schema/policy/validator/fixtures, and passed the public-app contract suite with 12 validated contracts.
- Next Step: The first asset request is complete; keep manifest-only discovery and require a new bounded request for any additional media.
- Dependencies / Overlap: `docs/project_ops/roadmap.md`, `docs/architecture/synaptome_public_runtime_contract_roadmap.md`, `docs/contracts/media_catalog.md`, `docs/schemas/media_catalog.schema.json`, `synaptome/bin/data/config/videos.json`, `tools/media_catalog_regression.py`.
- Primary Scope: contracts
- Secondary Scopes: docs, tests, runtime
- Blocking Issues / Unknowns: None for safe manifest intake; live projection review of individual assets remains an artistic acceptance step.
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

# Synaptome Project Ops Roadmap

Status: Active project index, reviewed 2026-07-28.

This file is the single source of truth for current priority, execution state,
and the next safe promotion point. Architecture roadmaps describe sequencing
inside a subsystem; show-development roadmaps preserve visual direction. They
do not become active commitments until they are promoted here through a
Project Ops request.

This roadmap does not replace the public runtime documentation or
[`docs/release_policy.md`](../release_policy.md).

## Current Objective

Status: Active 2026-07-26.

Continue the spine/element architecture convergence while preserving the
current working show baseline. The operator has now passed a live dual-screen
smoke test; live physical-MIDI hardware and the full show-machine recovery
rehearsal remain deferred. The host/dependency inventory, Element SDK v1
boundary, Runtime
extraction, authoritative built-in parameter contracts, Scene compatibility
reader, runtime value-origin slice, and mapping-bank route v1 are complete.
Machine-profile v1 now owns OSC transport/endpoints, one exact physical MIDI
input binding, and transactional logical control-slot assignments. Preferences
v1, transactional Text adoption, and the portable/local-state gate complete
SEAC-5. Audio/webcam/display/path adapters remain independently versioned
follow-up lanes. Live physical MIDI hardware remains untested.

The current safe sequence is:

```text
preserve the validated show/runtime baseline
  -> [done] pass the live dual-screen smoke test
  -> record deferred MIDI and recovery risk
  -> [done] inventory host-to-element dependencies and registrations
  -> [done] freeze the Element SDK v1 boundary
  -> [done] extract Runtime/build seams without changing output
  -> [done] unify built-in parameter declarations
  -> define versioned ownership and provenance for persisted state
  -> resume package controls and discovery through those contracts
```

## Active Work

### Element Workflow Acceleration

State Summary
- Status: Active implementation.
- Objective: Make a new or migrated element cheap to declare, validate, select,
  modify, and rehearse without weakening scene or mapping compatibility.
- Priority Order:
  1. Fail fast on catalog/default/registration drift without rebuilding
     openFrameworks.
  2. Reuse one common parameter declaration path for stable controls such as
     speed, BPM behavior, seed, and reseed, plus legacy visibility/alpha
     controls that must retain their public IDs.
  3. Make the active element and its useful controls faster to find during a
     performance.
  4. Expand the focused single-element lifecycle bench before adding more
     runtime discovery or package-loading behavior.
  5. Continue legacy migrations one real state model at a time.
- Compatibility Rule: Public asset IDs, registry prefixes, and established
  parameter suffixes remain stable. Shared infrastructure may remove authoring
  duplication, but it must not silently rename scene or MIDI/OSC targets. New
  elements should use their composition layer's visibility and opacity rather
  than adding duplicate whole-layer owners.
- Promotion Gate: A workflow improvement is complete only when its fast static
  check, focused native coverage, Release app build, scene persistence, and
  Browser/Console behavior agree.
- Current Slice: The fast authoring runner now validates Circuit Trace,
  Adaptive Trail, and Collective Motion in roughly `0.17-0.22s` per family
  without compiling openFrameworks. Its isolated native tier reruns in about
  one second. Four runtimes use the shared parameter builder for established
  common controls. The Console Asset Browser supports type-to-search, and
  `Ctrl+E` jumps from the focused Console layer to that element's first
  quick-access parameter in Control & Mapping.
- Current Migration Family: Cellular Fields. Game of Life and Excitable Media
  have completed the first bounded slice with distinct runtimes, deterministic
  scene-restored seeds, complete canonical defaults, compatibility aliases,
  and a `0.18-0.20s` fast profile. Lenia and Reaction Diffusion are next. All
  four remain separate algorithms even when they share lifecycle
  infrastructure.
- Priority Lane: Iteration speed and performance ergonomics.

### Show Readiness And Operator Stability

State Summary
- Request ID: show_readiness_operator_stability
- Phase: EXECUTION
- Status: Deferred by operator
- Steps Complete: 15 / 18
- Progress: Core persistence, mapping recovery, operator status, render, controller-window, and quit-safety work is implemented and validated; the operator reports dual-screen mode working well. Live physical-MIDI hardware and the full recovery rehearsal remain deferred.
- Last Step Outcome: 2026-07-27 - The operator ran the Release app successfully and passed a live dual-screen smoke test.
- Next Step: When show validation resumes, test live physical-MIDI control and complete the heaviest-scene and device-recovery rehearsal.
- Dependencies / Overlap: `docs/project_ops/roadmap.md`, `docs/architecture/synaptome_spine_element_model.md`, scene persistence, window/monitor placement, MIDI/OSC mappings, Browser, Console, and HUD.
- Primary Scope: runtime
- Secondary Scopes: tests, contracts, docs
- Blocking Issues / Unknowns: Live physical-MIDI behavior and the complete show-machine recovery sequence remain unproven; the operator accepted this as deferred validation risk.
- Impact / Priority Notes: Residual show-machine validation remains important but no longer blocks the spine/element architecture roadmap.
- Priority Score: N/A
- Priority Lane: Deferred
- Ready State: Ready
- Ready Gate: Core implementation and automated checks are complete enough to preserve; remaining acceptance requires later access to the show-machine display and device setup.
- Project Ops / Roadmap Updates (timestamped): 2026-07-24 - Opened as the active show blocker. 2026-07-26 - Recorded operator live evidence. 2026-07-26 - Deferred dual-screen and full recovery rehearsal and promoted the spine/element architecture request. 2026-07-27 - Recorded the successful live dual-screen smoke test; MIDI remains untested.
- Resume From: Phase EXECUTION, State Deferred by operator, Next Action test live physical-MIDI hardware and the recovery sequence when show-machine validation resumes.
Request Doc: docs/project_ops/in_progress/show_readiness_operator_stability.md

Show-safe checklist:

- [x] Remove output-preserving full-frame render waste.
- [x] Persist the visible live value for unmodulated scene parameters.
- [x] Preserve the underlying base and modifier stack for modulated parameters.
- [x] Scale operator text and row spacing consistently across all app surfaces.
- [x] Rasterize operator text at its requested size instead of scaling a bitmap.
- [x] Reflect one source half pixel-for-pixel in Mirror's horizontal/vertical modes.
- [x] Keep controller placement, fullscreen, and quit shortcuts show-safe.
- [x] Parse and validate MIDI/OSC mappings before replacing working routes.
- [x] Preserve global/live mappings when a legacy scene omits its mapping snapshot.
- [x] Treat a present scene mapping snapshot, including an empty one, as authoritative.
- [x] Verify scene/mapping/assignment temp JSON and retain a last-known-good backup.
- [x] Retain the active mapping bank and recovery-autosave modified scenes.
- [x] Show active scene, dirty/save/load state, mapping source/counts, and unresolved targets consistently.
- [x] Cover mapping save/mutate/restore/restart, malformed input, and missing MIDI hardware natively.
- [ ] Prove save → mutate → reload on the show machine with the heaviest scene.
- [x] Retest dual-screen mode; the operator reports it working well.
- [ ] Audit Browser, Console, and HUD navigation for dead ends and conflicting labels.
- [ ] Rehearse restart, missing-device, and failed-scene recovery.

### Spine And Element Architecture Convergence

State Summary
- Request ID: spine_element_architecture_convergence
- Phase: EXECUTION
- Status: In Progress
- Steps Complete: 9 / 12
- Progress: SEAC-9 is complete. Active package presets now support live preview, transactional apply/cancel/rollback, and visible base/live/modifier/origin state. Mapping suggestions expand against assigned layers and require explicit conflict-aware publication.
- Last Step Outcome: 2026-07-29 - Added recoverable package preset and mapping transactions, explicit parameter/action targets and edge triggers, stable route provenance, operator conflict protection, Browser apply/edit/disable/remove/rollback controls, and failed-write restoration.
- Next Step: Execute SEAC-10 default-off, inspect-before-activate package discovery without changing active show state on malformed, duplicate, incompatible, refreshed, removed, or unavailable content.
- Dependencies / Overlap: `show_readiness_operator_stability`, `layer_package_compatibility_bench_scaffolding`, `docs/architecture/synaptome_spine_element_model.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, parameter/scene/mapping contracts, and layer-authoring tests.
- Primary Scope: runtime
- Secondary Scopes: contracts, artist-sdk, tests, docs, release
- Blocking Issues / Unknowns: Native binary modules remain an optional architecture decision rather than a promised deliverable. SEAC-10 must freeze duplicate/replacement/refresh/deletion and unavailable-content policy before scanning package roots. Twenty-two built-ins still use a declared compatibility adapter during `setup()`; direct bind-only migration is cleanup. Audio input, webcam selection, display geometry, and content roots remain named legacy/local adapter lanes; adding any to the strict machine document requires a transactional machine-profile v2 normalizer rather than widening v1. Device-map `portHints` remain non-authoritative compatibility metadata. Live physical-MIDI hardware remains untested.
- Impact / Priority Notes: This is the active architecture lane and precedes automatic discovery, broader package activation, or new content-family expansion.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: The architecture direction, compatibility policy, ordered tasks, and stop conditions are explicit; the operator accepted residual show-validation risk and authorized execution.
- Resume From: Phase EXECUTION, State In Progress, Next Action implement SEAC-10 controlled discovery from the validated generated package inventory; preserve default-off inspection, stable identities, and active-state isolation.
- Project Ops / Roadmap Updates (timestamped): 2026-07-26 - Added the canonical model and subordinated package/discovery work to its contract and build gates. 2026-07-26 - Promoted SEAC to execution after dual-screen validation was deferred. 2026-07-26 - Completed the dependency inventory and froze the Element SDK v1 source/static-link boundary. 2026-07-26 - Landed the first SEAC-3 build and registration slice. 2026-07-26 - Moved generic element preparation/release and exact registration ownership behind the first Runtime facade seam. 2026-07-26 - Linked the first runtime-core library and moved fixed composition storage plus generic update/draw/resize ownership behind it. 2026-07-26 - Added isolated parameter staging and transactional same-address visual-element replacement. 2026-07-26 - Hardened reserved opacity ownership, prepared-result lifetime, FX/UI-to-visual adoption, bool modifier migration, and registry-consumer invalidation. 2026-07-26 - Removed the global element factory and proved per-Runtime type-registry isolation. 2026-07-26 - Moved zero-based effect coverage-window policy into Runtime and removed the duplicate `PostEffectChain` resolver without expanding the Element SDK. 2026-07-26 - Added the Runtime composition mutation control plane, Runtime-owned layer opacity, a const-only host view, and narrow render/legacy-element seams. 2026-07-27 - Replaced the const live host view with pointer-free by-value snapshots and removed public live composition access. 2026-07-27 - Replaced pointer-addressed generic element replacement with a zero-based composition-layer transaction and narrowed mutable legacy access to two compatibility areas. 2026-07-27 - Removed the derived element cache and moved ordinary built-in views, bindings, and parameter actions to snapshot-addressed registry access. 2026-07-27 - Added live-instance action registration, pointer-free snapshot discovery, and generic slot-addressed invocation without adding persisted action mappings. 2026-07-27 - Replaced read-only concrete element inspection with separate on-demand typed telemetry and made Geodesic subdivision durable parameter state. 2026-07-27 - Consolidated host creator bindings in the controlled aggregate and shared Signal Bloom's package leaf registrar with its bench without claiming generated registration. 2026-07-27 - Isolated legacy Text host parameters and font synchronization behind `BuiltinElementHostBindings` without claiming singleton retirement or authoritative declarations. 2026-07-27 - Closed SEAC-3R by extracting host-only composition rendering/GPU-target ownership, retiring the raw mutable target seam, and adding a dedicated stub-backed renderer policy harness. 2026-07-27 - Closed SEAC-4A with minimal static type/kind/action descriptor authority, atomic descriptor-plus-creator registration, construction-free inspection, exact live handler binding, and shipping registration migration. 2026-07-27 - Completed SEAC-4B1 with the pointer-free parameter DTO, explicit declared-versus-legacy registry state, and a construction-free five-group/18-parameter Signal Bloom declaration with exact package/static parity plus compatible live ID/kind/range registration. 2026-07-27 - Completed SEAC-4B2 for Signal Bloom with bind-only live storage and declaration-owned runtime metadata/defaults. 2026-07-27 - Froze SEAC-5A state ownership, provenance, version-reader, portability, and migration rules after three parallel audits.
  2026-07-27 - Implemented the side-effect-free Scene v1/v2 compatibility reader and non-destructive future-version gate.
  2026-07-27 - Implemented nonserialized parameter value origins and pointer-free base/live/modifier inspection without changing value precedence or public persistence.
  2026-07-27 - Implemented mapping-bank v1 for the actual flat `MidiRouter` route snapshot with legacy copied migration, complete canonical writes, and non-downgrading future-version rejection.
  2026-07-27 - Added transport-neutral typed OSC ingress plus the Synaptome-owned Mesh v0.1 consumer profile without changing the producer contract.
  2026-07-27 - Added the strict OSC-first machine-profile v1 document and moved explicit Browser transport changes behind recoverable local profile publication.
  2026-07-28 - Added strict `controlSlots`, canonical-first startup adoption, atomic assignment/MIDI-route/profile publication with rollback, and Scene/autosave writer omission while preserving legacy Scene reads.
  2026-07-28 - Added strict optional physical MIDI input ownership, omission-only legacy delegation, exact-unique resolution/reconnect, active-profile route filtering, and recoverable explicit Device Mapper binding with rollback.
  2026-07-28 - Closed SEAC-5 with strict preferences v1, recoverable section-preserving adapters, transactional Text adoption, operator-owned active-bank persistence, and an executable portable/local machine-state boundary.
  2026-07-28 - Completed SEAC-6 with one isolated confidence CLI/report, real SDK/lifecycle and graphics harnesses, reload/memory/timing gates, and Windows graphics CI.
  2026-07-29 - Completed SEAC-7 with strict Element Package v1, contained Signal Bloom source, exact construction-free Runtime parity, stable diagnostics, and package evidence in the confidence report.
  2026-07-29 - Completed SEAC-8 with an explicit validated registration set, deterministic generated Runtime/build records, creator-only package leaves, generic host/bench integration, and removal of Signal Bloom's handwritten mirror/project wiring.
Request Doc: docs/project_ops/in_progress/spine_element_architecture_convergence.md

Current Direction
- Status: Active architecture execution; residual show-machine validation is
  deferred.
- Objective: Make Synaptome one stable spine that hosts swappable creative
  elements in ordered composition layers, with scenes, parameters, presets,
  mappings, packages, and tests all using one explicit model.
- Terminology Decision:
  - An **element** is the creative module or algorithm.
  - A **layer** is one ordered Console composition container that hosts an
    element instance.
  - A **scene** saves a layered composition and intentionally scene-owned
    state.
- Current Compatibility Rule: Existing `Layer` C++ names, `layer` schema
  fields, asset IDs, registry prefixes, and `console.layer{slot}` parameter
  IDs remain stable until a focused migration provides aliases and fixtures.
- Contract Authority:
  [`docs/architecture/synaptome_spine_element_model.md`](../architecture/synaptome_spine_element_model.md).
- SDK Boundary Authority:
  [`docs/architecture/element_sdk_v1_boundary.md`](../architecture/element_sdk_v1_boundary.md).
- First Architecture Slice:
  1. Freeze identity, parameter metadata, state ownership, compatibility
     levels, and capability declarations.
  2. Define the Element SDK, runtime-core, host, and per-element build seams.
  3. Recast the existing package work as Element Package v1.
  4. Complete mapping-preset controls only inside the unified state and
     provenance model.
- Promotion Gate: A new package/discovery claim must say whether it is a raw
  openFrameworks reference, source-wrapped element, parametric element,
  data-only content definition, generated registration, or native module. It
  must not use “drop-in plugin” as an umbrella promise.
- Priority Lane: Current architecture gate.
- Ready State: SEAC-8 promotion criteria pass; SEAC-9 can consume the
  generated Signal Bloom declaration and suggestion-only mapping inventory.
  The implemented registration contract is
  [`docs/contracts/generated_element_package_registration_v1.md`](../contracts/generated_element_package_registration_v1.md).

Task breakdown: 8 complete and 4 planned. The ordered promotion path is static
descriptors/actions (complete) -> static Signal Bloom parameter declaration
(complete) -> live parameter binding/authority (complete) -> versioned state
(complete) -> confidence suite (complete) -> package serialization (complete)
-> generated registration (complete) -> operator mappings (current) ->
discovery -> native-module decision -> representative
migration.

### Circuit Trace Generative Family

State Summary
- Status: Implementation complete; live visual acceptance pending.
- Progress: Added one modular `circuitTrace` runtime with five catalog models:
  Circuit Slime, Circuit Mycelium, Circuit River, Circuit Ant Tunnels, and
  Circuit Flow Field. All movement is locked to
  the eight horizontal, vertical, and 45-degree diagonal lattice directions.
  Each asset has an independent stable parameter prefix, deterministic seed,
  scene persistence, and normal MIDI/OSC mapping compatibility. The presentation
  pass now intentionally uses a `256x144` field with nearest-neighbor scaling
  for large, hard-edged pixels, while retaining circular trace coverage and
  clean junctions. Forced drilled-via symbols and square dilation were removed.
  Mycelium now seeds twelve distributed colonies instead of concentrating every
  growth agent around the center. Circuit Ant Tunnels adds sparse
  pheromone-routed corridors, while Circuit Flow Field quantizes an analytic
  vector field through the same eight-direction routing seam.
  Circuit Lenia remains on the established deterministic `lenia` simulation
  and selects a fixed circuit presentation through its catalog: the continuous
  field becomes hard, nested isocontour traces at `160x90`, with its own stable
  asset ID, parameter namespace, and quick-access contour controls. Seven
  Circuit Lenia parameters now have semantic OSC defaults in the normal global
  mapping file. They remain visible and editable through Control & Mapping,
  persist with mapping snapshots, and are not hardcoded in the renderer.
- Validation: Release app project builds; all 32 BrowserFlow scenarios,
  Hotkey test, all 17 public-app contracts, the dedicated Circuit Trace and
  Circuit Lenia contracts, and diff checks pass.
- Next Step: Reload all six circuit assets on the show machine and confirm the
  coarse pixel scale matches the app's visual language, Mycelium fills the
  frame, River has no plus-in-square symbols, Slime has no accidental grid
  artifacts, and Lenia's contours remain legible through a mature organism.
  Then tune the visible Circuit Lenia parameters through Control & Mapping,
  choose useful default colors and growth rates under projection, and save one
  known-good scene with its mapping snapshot.
- Priority Lane: Bounded show-content tuning.
- Ready State: Ready for live review.

### Modular Element Migration

State Summary
- Status: First migration pass complete; live visual acceptance pending.
- Progress: The existing Ant Tunnels, Slime Mold, and Physarum assets now share
  an aligned `agentField` lifecycle, while Schooling and Murmuration share an
  aligned `flocking` lifecycle. Their catalog IDs, runtime types, registry
  prefixes, and established parameter suffixes remain stable, so saved scenes
  and MIDI/OSC mappings keep their addresses. Both runtimes now own a
  deterministic persisted seed/reseed path instead of depending on
  process-global randomness, and every registered parameter has a labeled
  group and catalog default.
- Workflow Baseline: Circuit Trace, Adaptive Trail, and Collective Motion now
  cover ten assets across three shared runtimes under the same migration
  guide and public validation gate. Game of Life and Excitable Media add two
  distinct Cellular Fields runtimes under the same lifecycle rules.
- Validation: Release app project builds; all 23 BrowserFlow scenarios and the
  modular-family contract pass. The public-app gate now contains 15 contracts.
- Next Step: Live-review the migrated assets for useful defaults and acceptable
  restart/scene-reload behavior. Then migrate Lenia and Reaction Diffusion
  individually, sharing lifecycle infrastructure where useful without
  collapsing distinct algorithms into a cosmetic mode switch.
- Priority Lane: Reliability and iteration infrastructure.
- Ready State: Ready for live review.
- Migration Guide:
  [`docs/architecture/layer_migration_workflow.md`](../architecture/layer_migration_workflow.md).

### Element Package And Compatibility Bench Scaffolding

State Summary
- Request ID: layer_package_compatibility_bench_scaffolding
- Phase: EXECUTION
- Status: Paused behind show and spine contract
- Steps Complete: 8 / 10 (2 additional steps in progress)
- Progress: The Browser now provides reusable labeled dropdowns on matching live package parameters for both static `options[]` and registered `optionsSource` choices, alongside the operator-local package preset-bank picker. Selections update the existing live/base parameter value, while unavailable provider values are preserved until the operator explicitly replaces them.
- Last Step Outcome: 2026-07-24 - Added reusable live labeled parameter selection, including `Half Time` / `Normal` / `Double Time` for BPM and `Compact` / `Default` / `Full` for Scale, with provider-revision cancellation, unavailable-value preservation, telemetry, and scene/mapping/inspection isolation coverage.
- Next Step: After the show-safe checkpoint, rebaseline the package schema and Browser mapping work against the canonical element contract and SEAC-7/SEAC-9; do not resume mapping UI independently.
- Dependencies / Overlap: `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`, `docs/contracts/parameter_manifest.json`, `docs/schemas/layer_asset.schema.json`, `tools/layer_catalog_regression.py`, `tools/gen_parameter_manifest.py`.
- Primary Scope: contracts
- Secondary Scopes: artist-sdk, docs, tests, runtime
- Blocking Issues / Unknowns: Generated registration/module loading remains a later architecture decision; mapping-preset editing still needs explicit Browser ownership; beat-reactive mappings depend on the separate transport/reactivity contract.
- Impact / Priority Notes: Establishes deterministic package and generated-content contracts before more tracked media, Browser activation, or runtime discovery increases compatibility risk.
- Priority Score: N/A
- Priority Lane: Deferred
- Ready State: Ready
- Ready Gate: Met for read-only inspection, vetted opt-in source registration, operator-local next-load preset selection, and explicit labeled live parameter selection; automatic discovery and mapping activation remain out of scope.
- Project Ops / Roadmap Updates (timestamped): 2026-06-25 - Promoted layer package scaffolding from backlog. 2026-07-18 - Reconciled the request with the current Project Ops readiness contract and pre-media gate. 2026-07-18 - Completed the bounded package-owned static option slice and paused before runtime promotion. 2026-07-24 - Converged the reviewed runtime adapter, added package-owned dynamic option metadata and unavailable-value policy, completed the live-window smoke, rendered read-only option/provider state in the Browser, resolved the app-owned transport provider with missing-value preservation, added operator-local next-load preset-bank selection, and promoted static/runtime choices into explicit live labeled dropdowns. 2026-07-26 - Paused independent mapping UI work and subordinated package continuation to the canonical spine/element request.
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

### In Progress: Complete The Spine And Element Contract

1. Use one canonical vocabulary for spine, elements, layers, scenes,
   parameters, mappings, presets, packages, and machine-local state.
2. Separate current source-registration capability from future generated
   registration, data-only discovery, and native modules.
3. Define the Element SDK, runtime-core, host, and per-element test/build
   boundaries before expanding runtime discovery.
4. Built-in parameter declaration, grouping, metadata, and Runtime authority
   are complete. Next resolve value provenance and state ownership; dependency,
   capability, teardown, and package serialization remain later gates.

### Then: Finish Browser Ownership

1. Named and runtime-provider choices now render read-only without rewriting
   stored values; unavailable defaults are preserved and marked.
2. Preset-bank selection now persists stable IDs locally and applies on the
   next element load without rewriting active scene or mapping state.
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
| [Element system](../architecture/synaptome_layer_system_roadmap.md) | Active support | Transitional package, discovery, presets, mappings, Browser, validation, and registration sequence. | Reconcile its legacy layer terminology and package shape with the canonical spine/element model. |
| [Public runtime contracts](../architecture/synaptome_public_runtime_contract_roadmap.md) | Planned support | Spine contracts outside the element system, including media discovery and host audio. | The first reviewed media asset is complete; promote the next runtime gap only through a focused request. |
| [Transport and reactivity](../architecture/synaptome_transport_reactivity.md) | Planned dependency | Beat source, confidence, onset/downbeat, and fallback contract. | Required before beat-reactive mapping presets become canonical. |
| [Biological elements](../architecture/synaptome_biological_layer_roadmap.md) | Parked | Long-range simulation and element-family direction. | Resume after shared field/graph infrastructure and package validation are ready. |
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

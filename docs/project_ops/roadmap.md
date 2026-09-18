# Synaptome Project Ops Roadmap

Status: Active project index, reviewed 2026-08-16.

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

### Algorithmic Show Content

State Summary
- Request ID: algorithmic_show_content
- Phase: EXECUTION
- Status: In progress
- Steps Complete: 3 / 4
- Progress: Twelve compiled source packages, 36 catalog looks and six effect-look scenes implemented; portable validation and operator guide complete.
- Last Step Outcome: 2026-09-17 - Verified declarations, deterministic models, native adapters and recorded geometry; corrected reseed persistence before delivery.
- Next Step: Build Release/x64 on the show machine, rehearse planned combinations and confirm physical MIDI/audio/projector behavior.
- Dependencies / Overlap: Element SDK v1, source registration, parameter manifests, existing effect chain, show_readiness_operator_stability.
- Primary Scope: contracts
- Secondary Scopes: tests, docs
- Blocking Issues / Unknowns: Windows Release build, projector rendering and physical MIDI cannot run in this Linux workspace.
- Impact / Priority Notes: Content for the operator's next-day visuals show; preserve runtime behavior.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: User authorized additive content; existing source-package boundary supports implementation. Target-machine acceptance remains a final validation gate.
- Project Ops / Roadmap Updates (timestamped): 2026-09-17 - Promoted content creation for the next-day show without resuming architecture work.
- Resume From: Phase EXECUTION, State In progress, Next Action record target-machine acceptance using docs/show_content_guide.md.
Request Doc: docs/project_ops/in_progress/algorithmic_show_content.md

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
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 12 / 12
- Progress: SEAC-12 and the spine/element convergence milestone are complete. Grid, STL Model, and Lenia are explicit bind-only references, the authoring guide is published, and 19 compatibility adapters remain bounded cleanup.
- Last Step Outcome: 2026-08-16 - The operator built and launched from Visual Studio and confirmed Grid, STL Tetra, organic Lenia, and Circuit Lenia load in the real host, closing the final live gate after all automated and Release evidence passed.
- Next Step: Preserve the completed public boundary; schedule the remaining adapter migrations and deferred device/show work independently.
- Dependencies / Overlap: `show_readiness_operator_stability`, `layer_package_compatibility_bench_scaffolding`, `docs/architecture/synaptome_spine_element_model.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, parameter/scene/mapping contracts, and layer-authoring tests.
- Primary Scope: runtime
- Secondary Scopes: contracts, artist-sdk, tests, docs, release
- Blocking Issues / Unknowns: None for this milestone. Nineteen compatibility adapters remain cleanup, and live physical-MIDI hardware remains a separately deferred show-readiness item.
- Impact / Priority Notes: This is the active architecture lane and precedes automatic discovery, broader package activation, or new content-family expansion.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Complete
- Ready Gate: The architecture direction, compatibility policy, ordered tasks, and stop conditions are explicit; the operator accepted residual show-validation risk and authorized execution.
- Resume From: Phase COMPLETE, State Complete; retain `docs/project_ops/reports/seac_12_representative_migration.md` as the closure record.
- Project Ops / Roadmap Updates (timestamped): 2026-07-26 - Added the canonical model and subordinated package/discovery work to its contract and build gates. 2026-07-26 - Promoted SEAC to execution after dual-screen validation was deferred. 2026-07-26 - Completed the dependency inventory and froze the Element SDK v1 source/static-link boundary. 2026-07-26 - Landed the first SEAC-3 build and registration slice. 2026-07-26 - Moved generic element preparation/release and exact registration ownership behind the first Runtime facade seam. 2026-07-26 - Linked the first runtime-core library and moved fixed composition storage plus generic update/draw/resize ownership behind it. 2026-07-26 - Added isolated parameter staging and transactional same-address visual-element replacement. 2026-07-26 - Hardened reserved opacity ownership, prepared-result lifetime, FX/UI-to-visual adoption, bool modifier migration, and registry-consumer invalidation. 2026-07-26 - Removed the global element factory and proved per-Runtime type-registry isolation. 2026-07-26 - Moved zero-based effect coverage-window policy into Runtime and removed the duplicate `PostEffectChain` resolver without expanding the Element SDK. 2026-07-26 - Added the Runtime composition mutation control plane, Runtime-owned layer opacity, a const-only host view, and narrow render/legacy-element seams. 2026-07-27 - Replaced the const live host view with pointer-free by-value snapshots and removed public live composition access. 2026-07-27 - Replaced pointer-addressed generic element replacement with a zero-based composition-layer transaction and narrowed mutable legacy access to two compatibility areas. 2026-07-27 - Removed the derived element cache and moved ordinary built-in views, bindings, and parameter actions to snapshot-addressed registry access. 2026-07-27 - Added live-instance action registration, pointer-free snapshot discovery, and generic slot-addressed invocation without adding persisted action mappings. 2026-07-27 - Replaced read-only concrete element inspection with separate on-demand typed telemetry and made Geodesic subdivision durable parameter state. 2026-07-27 - Consolidated host creator bindings in the controlled aggregate and shared Signal Bloom's package leaf registrar with its bench without claiming generated registration. 2026-07-27 - Isolated legacy Text host parameters and font synchronization behind `BuiltinElementHostBindings` without claiming singleton retirement or authoritative declarations. 2026-07-27 - Closed SEAC-3R by extracting host-only composition rendering/GPU-target ownership, retiring the raw mutable target seam, and adding a dedicated stub-backed renderer policy harness. 2026-07-27 - Closed SEAC-4A with minimal static type/kind/action descriptor authority, atomic descriptor-plus-creator registration, construction-free inspection, exact live handler binding, and shipping registration migration. 2026-07-27 - Completed SEAC-4B1 with the pointer-free parameter DTO, explicit declared-versus-legacy registry state, and a construction-free five-group/18-parameter Signal Bloom declaration with exact package/static parity plus compatible live ID/kind/range registration. 2026-07-27 - Completed SEAC-4B2 for Signal Bloom with bind-only live storage and declaration-owned runtime metadata/defaults. 2026-07-27 - Froze SEAC-5A state ownership, provenance, version-reader, portability, and migration rules after three parallel audits.
  2026-07-27 - Implemented the side-effect-free Scene v1/v2 compatibility reader and non-destructive future-version gate.
  2026-07-27 - Implemented nonserialized parameter value origins and pointer-free base/live/modifier inspection without changing value precedence or public persistence.
  2026-07-27 - Implemented mapping-bank v1 for the actual flat `MidiRouter` route snapshot with legacy copied migration, complete canonical writes, and non-downgrading future-version rejection.
  2026-07-27 - Added transport-neutral typed OSC ingress plus the Synaptome-owned Mesh v0.1 consumer profile without changing the producer contract.
  2026-07-27 - Added the strict OSC-first machine-profile v1 document and moved explicit Browser transport changes behind recoverable local profile publication.
  2026-07-28 - Added strict `controlSlots`, canonical-first startup adoption, atomic assignment/MIDI-route/profile publication with rollback, and Scene/autosave writer omission while preserving legacy Scene reads.
  2026-07-28 - Added strict optional physical MIDI input ownership, omission-only legacy delegation, exact-unique resolution/reconnect, active-profile route filtering, and recoverable explicit Device Mapper binding with rollback.
  2026-07-28 - Closed SEAC-5 with strict preferences v1, recoverable section-preserving adapters, transactional Text adoption, operator-owned active-bank persistence, and an executable port
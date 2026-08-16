# Spine And Element Architecture Convergence

State Summary
- Request ID: spine_element_architecture_convergence
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 12 / 12
- Progress: SEAC-12 and the spine/element architecture convergence milestone are complete. Grid, STL Model, and Lenia use explicit bind-only storage; the public authoring guide is published; 19 compatibility setup adapters remain cleanup.
- Last Step Outcome: 2026-08-16 - The operator built and launched from Visual Studio and confirmed Grid, STL Tetra, organic Lenia, and Circuit Lenia load in the real host. This completes the live gate after all confidence, BrowserFlow, public contract, discovery/package, and Release-build evidence passed.
- Next Step: Keep the public authoring and validation path stable. Migrate the remaining 19 compatibility adapters only as bounded cleanup without reopening the architecture milestone.
- Dependencies / Overlap: `show_readiness_operator_stability`, `layer_package_compatibility_bench_scaffolding`, `docs/architecture/synaptome_spine_element_model.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, parameter/scene/mapping contracts, and layer-authoring tests.
- Primary Scope: runtime
- Secondary Scopes: contracts, artist-sdk, tests, docs, release
- Blocking Issues / Unknowns: None for this milestone. Nineteen non-reference built-ins still use a declared compatibility adapter during `setup()`; their direct bind-only migration is cleanup. Audio input, webcam selection, display geometry, additional content roots, and live physical-MIDI validation remain independently routed follow-ups.
- Impact / Priority Notes: This is the active architecture lane and precedes automatic discovery, broader package activation, or new content-family expansion.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Complete
- Ready Gate: The architecture direction, compatibility policy, ordered tasks, and stop conditions are explicit; the operator accepted residual show-validation risk and authorized execution.
- Resume From: Phase COMPLETE, State Complete; use [`SEAC-12 evidence`](../reports/seac_12_representative_migration.md) as the closure record.
- Project Ops / Roadmap Updates (timestamped): 2026-07-26 - Added the canonical model and subordinated package/discovery work to its contract and build gates. 2026-07-26 - Promoted SEAC to execution after dual-screen validation was deferred. 2026-07-26 - Completed the dependency inventory and froze the Element SDK v1 source/static-link boundary. 2026-07-26 - Landed the first SEAC-3 build and registration slice. 2026-07-26 - Moved generic element preparation/release and exact registration ownership behind the first Runtime facade seam. 2026-07-26 - Linked the first runtime-core library and moved fixed composition storage plus generic update/draw/resize ownership behind it. 2026-07-26 - Added isolated parameter staging and transactional same-address visual-element replacement. 2026-07-26 - Hardened reserved opacity ownership, prepared-result lifetime, FX/UI-to-visual adoption, bool modifier migration, and registry-consumer invalidation. 2026-07-26 - Removed the global element factory and proved per-Runtime type-registry isolation. 2026-07-26 - Moved zero-based effect coverage-window policy into Runtime and removed the duplicate `PostEffectChain` resolver without expanding the Element SDK. 2026-07-26 - Added the Runtime composition mutation control plane, Runtime-owned layer opacity, a const-only host view, and narrow render/legacy-element seams. 2026-07-27 - Replaced the const live host view with pointer-free by-value snapshots and removed public live composition access. 2026-07-27 - Replaced pointer-addressed generic element replacement with a zero-based composition-layer transaction and narrowed mutable legacy access to two compatibility areas. 2026-07-27 - Removed the derived element cache and moved ordinary built-in views, bindings, and parameter actions to snapshot-addressed registry access. 2026-07-27 - Added live-instance action registration, pointer-free snapshot discovery, and generic slot-addressed invocation without adding persisted action mappings. 2026-07-27 - Replaced read-only concrete element inspection with separate on-demand typed telemetry and made Geodesic subdivision durable parameter state. 2026-07-27 - Consolidated host creator bindings in the controlled aggregate and shared Signal Bloom's package leaf registrar with its bench without claiming generated registration. 2026-07-27 - Isolated legacy Text host parameters and font synchronization behind `BuiltinElementHostBindings` without claiming singleton retirement or authoritative declarations. 2026-07-27 - Closed SEAC-3R by extracting host-only composition rendering/GPU-target ownership, retiring the raw mutable target seam, and adding a dedicated stub-backed renderer policy harness. 2026-07-27 - Closed SEAC-4A with minimal static type/kind/action descriptor authority, atomic descriptor-plus-creator registration, construction-free inspection, exact live handler binding, and shipping registration migration. 2026-07-27 - Completed SEAC-4B1 with the pointer-free parameter DTO, explicit declared-versus-legacy registry state, and a construction-free five-group/18-parameter Signal Bloom declaration with exact package/static parity plus compatible live ID/kind/range registration. 2026-07-27 - Completed SEAC-4B2 for Signal Bloom with bind-only live storage and declaration-owned runtime metadata/defaults. 2026-07-27 - Froze SEAC-5A state ownership, provenance, version-reader, portability, and migration rules after three parallel audits.
  2026-07-27 - Implemented the side-effect-free Scene v1/v2 compatibility reader and non-destructive future-version gate.
  2026-07-27 - Implemented nonserialized parameter value origins and pointer-free base/live/modifier inspection without changing value precedence or public persistence.
  2026-07-27 - Implemented mapping-bank v1 for the actual flat `MidiRouter` route snapshot with legacy copied migration, complete canonical writes, and non-downgrading future-version rejection.
  2026-07-27 - Added the typed OSC ingress envelope and Synaptome-owned `synaptome-mesh-v1` receiver profile without changing the producer contract; raw provenance remains visible while canonical numeric routes dispatch once.
  2026-07-27 - Introduced strict OSC-first machine-profile v1, moved explicit Browser mode writes behind recoverable local profile publication, and retained the legacy OSC file only as absent-profile compatibility input.
  2026-07-28 - Added strict `controlSlots`, canonical-first startup adoption, atomic assignment/MIDI-route/profile publication with rollback, and Scene/autosave writer omission while preserving legacy Scene reads.
  2026-07-28 - Added strict optional physical MIDI input ownership, omission-only legacy delegation, exact-unique resolution/reconnect, active-profile route filtering, and recoverable explicit Device Mapper binding with rollback.
  2026-07-28 - Closed SEAC-5 with strict preferences v1, canonical-first compatibility adapters, recoverable section-preserving writes, transactional Text adoption, operator-owned active-bank persistence, and an executable portable/local machine-state boundary.
  2026-07-28 - Froze the SEAC-6 execution handoff: Grid and Signal Bloom fixtures, runner/report contract, deterministic inputs, real-context boundary, reload/memory/performance gates, commands, CI tiers, and stop conditions are explicit.
  2026-07-28 - Completed SEAC-6 with one profile/package CLI, stable JSON reports, real SDK/lifecycle and hidden-context graphics harnesses, 200-cycle reload/memory/timing evidence, reviewed-baseline support, and a labeled Windows graphics CI lane.
  2026-07-29 - Fixed and documented the MSVC junction/physical include-namespace collision, preserved distinct app/runtime and repo-test root selection, and prepared the frozen SEAC-7 Element Package v1 execution handoff.
  2026-07-29 - Completed SEAC-7 with strict Element Package v1 schema/model/reader, a package-contained Signal Bloom source fixture, dependency/capability/version/path/migration validation, exact copied Runtime descriptor parity before creator invocation, report inventory/signatures, and focused negative coverage.
  2026-07-29 - Completed SEAC-8 with an explicit validated registration set, deterministic generated Runtime/build records, creator-only package leaves, stale/duplicate/dependency/symbol failure gates, generic host/bench integration, and removal of Signal Bloom's handwritten mirror, registrar, dedicated project, and solution wiring.
  2026-07-29 - Completed SEAC-9 with recoverable preset and mapping transactions, visible provenance and conflicts, explicit parameter/action targeting, and Browser preview/apply/edit/disable/remove/rollback controls.
  2026-07-29 - Froze the SEAC-10 controlled-discovery execution handoff: default-off local roots, construction-free candidate snapshots, path-independent identities, collision/replacement/refresh/deletion policy, an STL data-only slice, inspect-before-activate flow, promotion evidence, and stop conditions are explicit.
  2026-07-29 - Completed SEAC-10 with strict local configuration, pure package/STL scanning, exact generated/built-in type availability, immutable lifecycle snapshots, Browser diagnostics/refresh/activation gating, recoverable catalog publication, hostile-state isolation, and physical/junction Release evidence.
  2026-08-15 - Completed SEAC-11 by publishing Native Module Policy v1: no runtime native loader, a supported generated-registration/data-template/external-bridge ladder, and a complete future reversal gate. Advanced execution to SEAC-12 without adding schemas or code.

## Fresh-Context Handoff

- Remote handoff: PR #1 carries the architecture lane. Verify branch divergence
  and `git status` before publishing, and do not include operator-local maps,
  scenes, backups, IDE state, or runtime logs.
- Verified baseline: 23 built-in types, 786 declared parameters, all 17
  focused discovery tests passing on Windows 11 25H2, all 24 public-app
  contracts passing, 51 passing BrowserFlow scenarios, clean physical and
  junction Release app builds, and captured concurrent Signal Bloom/generated
  tetrahedron activation.
- Remote diagnosis: the closed evidence record, capture commands, and triage
  classifications are maintained in
  [`../reports/seac_10_remote_diagnostics.md`](../reports/seac_10_remote_diagnostics.md).
- Manual evidence: the Release app and dual-screen mode work. Live physical
  MIDI hardware and the complete show-machine recovery rehearsal have not been
  tested.
- Do not promote operator-local runtime files into fixtures accidentally.
  In particular, local MIDI maps, saved scenes, backups, and runtime logs may
  be dirty without representing architecture changes.
- SEAC-5A is complete: the state-ownership/provenance matrix and versioned
  compatibility rules are frozen in
  [`../../contracts/state_ownership_and_provenance.md`](../../contracts/state_ownership_and_provenance.md).
  The side-effect-free Scene version classifier/normalizer, read-only runtime
  value-origin slice, mapping-bank v1 route reader/writer, and transactional
  machine-profile OSC, physical-MIDI-input, and control-slot owners are
  implemented. Preferences v1 and transactional Text adoption close the
  remaining promotion criteria. SEAC-6 is also complete; keep audio, webcam,
  display, and path adapters independently versioned future lanes.
- SEAC-6 is implemented from
  [`../../architecture/element_confidence_suite_v1_handoff.md`](../../architecture/element_confidence_suite_v1_handoff.md).
  Grid and Signal Bloom now exercise the isolated runner and JSON report,
  stub-policy separation, real offscreen evidence, deterministic lifecycle,
  GL containment, reload, memory-growth, timing, CI, and stop gates.
- SEAC-7 is implemented from the frozen
  [`Element Package v1 handoff`](../../architecture/element_package_v1_handoff.md)
  and documented in
  [`Element Package v1`](../../contracts/element_package_v1.md).
- SEAC-8 is implemented in
  [`Generated Element Package Registration v1`](../../contracts/generated_element_package_registration_v1.md)
  with execution evidence in the
  [`SEAC-8 handoff`](../../architecture/generated_element_registration_v1_handoff.md).
- SEAC-9 is implemented in
  [`Package Control Transactions v1`](../../architecture/package_control_transactions_v1_handoff.md).
  SEAC-10 execution is frozen in
  [`Controlled Package Discovery v1`](../../architecture/controlled_package_discovery_v1_handoff.md).
- SEAC-11 is complete in
  [`Native Module Policy v1`](../../architecture/native_module_policy_v1.md);
  v1 retains no native runtime loader.
- The parameter maintenance and live-validation procedure is documented in
  [`../../contracts/builtin_element_parameter_contract.md`](../../contracts/builtin_element_parameter_contract.md).

## Milestone Synthesis

- Milestone ID: spine-element-architecture-convergence
- Milestone Name: Bulletproof Spine And Swappable Element System
- Milestone Type: architecture
- Source Requests: spine_element_architecture_convergence
- Outcome Statement (Done When): Synaptome has an independently buildable Element SDK and runtime-core boundary, one authoritative element/package contract, strict portable state ownership, a reusable isolated confidence suite, and a generated or otherwise controlled registration path that adds a reference element without editing `ofApp.cpp`.
- KPI / Success Signal: A reference source element is declared once, validated, built, registered, run, rendered, unloaded, reloaded, mapped, preset, saved in a scene, and restored through documented commands without hand-editing the host composition root.
- Target Window: Begin immediately under the accepted deferred-show-risk decision; complete the foundation before automatic package discovery or another broad content-family expansion.
- Dependency Gates: Preserved working show baseline, stable public IDs, current package fixtures, current scene/mapping recovery behavior, and a passing Release/public-app baseline.
- Contract Surfaces: Element SDK, lifecycle, render context, services, parameters, packages, presets, mappings, scenes, machine profiles, registration, discovery, diagnostics, schemas, fixtures, and build targets.
- Risk Posture: High because this changes dependency direction, build ownership, extension contracts, and persisted public state while preserving a working live application.
- Goal: Make Synaptome a stable advanced spine that can host, control, persist, test, and compose swappable creative elements without accumulating element-specific code in the host.
- Non-Goals: Directly loading an arbitrary raw `ofApp` folder, promising native hot reload, replacing the eight-layer composition model, renaming stable public IDs for cosmetic consistency, or migrating every legacy element in the first pass.
- Owner: Synaptome runtime and Artist SDK

## Roadmap Overlap Review

- Existing roadmap entries checked: `docs/project_ops/roadmap.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_public_runtime_contract_roadmap.md`, `docs/architecture/synaptome_transport_reactivity.md`, `docs/architecture/synaptome_artist_sdk.md`, and `docs/project_ops/synaptome_layer_design_standards.md`.
- Related active requests: show_readiness_operator_stability and layer_package_compatibility_bench_scaffolding
- Duplicate risk: Medium
- Merge / split decision: Keep show readiness as the current blocker; treat the package request as completed scaffolding plus a paused implementation input to this plan; move shared terminology, SDK/build boundaries, state ownership, and discovery promotion under this request.
- Priority conflict: The former package-roadmap next step proposed mapping UI immediately after show readiness; this plan inserts the spine/element contract and build-boundary gate first.

## Prioritization

- Policy Source: User request to audit and unify Synaptome around one stable spine with swappable elements in ordered composition layers.
- Priority Score: N/A
- Priority Lane: Standard
- Due Date / Timing Driver: Start now and complete the foundation before automatic discovery, native module claims, or broad new content work.
- Sort Key: 2026-07-26-after-show-spine-element
- Override: Show-machine stability remains the only higher-priority lane.

## Definition Of Ready

- Ready State: Ready
- Ready Date: 2026-07-26
- Ready Owner: Synaptome runtime and Artist SDK
- Ready Exceptions: Exact native-module strategy is intentionally deferred until the static/generated-registration architecture and dependency inventory provide evidence.
- Decision Links: `docs/architecture/synaptome_spine_element_model.md` and `docs/project_ops/roadmap.md`

## Complexity

- Level: High
- Predicted Count: 12
- Count Drivers: Terminology/identity, SDK boundary, runtime-core extraction, lifecycle, rendering/resources, parameters, state, bench, packages, controls, discovery, module decision, and reference migration.
- Drivers: Monolithic `ofApp` ownership, public compatibility, duplicated declarations, permissive schemas, native C++ dependencies, graphics-state risk, and live-performance regression risk.
- Confidence: High for the ordering and static/generated-registration path; medium for any later native binary module mechanism.

## Intake

- User Request: Produce a prioritized Project Ops roadmap that turns Synaptome into one bulletproof spine hosting swappable elements in ordered layers with scenes, parameters, mappings, presets, independent tests, and controlled installation.
- Context: Synaptome already has working composition layers, parameters, scenes, MIDI/OSC mappings, package fixtures, validation, and a first stub-backed lifecycle/draw-dispatch bench, but its build and runtime ownership remain monolithic.
- Acceptance Signal: A new reference element follows one documented package path from declaration through isolated validation and scene restoration without element-specific changes to `ofApp.cpp`.

## Form

- Problem Statement: The conceptual spine/element split is ahead of the physical code, build, lifecycle, state, and package boundaries, so new creative work still increases host coupling and compatibility risk.
- User / Operational Value: Artists gain a predictable way to turn experiments into controllable performance elements, while operators retain stable scenes, mappings, presets, recovery, and show performance.
- Change Type: architecture, runtime, contracts, build, validation, docs, and release
- Execution Mode: Strict gated
- Acceptance Criteria: Complete the task graph and milestone gates; preserve existing public IDs and current show behavior; pass isolated element, host integration, scene round-trip, and live projection validation.
- Constraints: Maintain Windows/openFrameworks support, preserve current scenes and mappings, keep raw `ofApp` compatibility claims honest, and do not enable discovery before validation and rollback exist.
- Must Not Change: Existing asset IDs, element type IDs, registry prefixes, `console.layer{slot}` targets, scene/mapping semantics, or show-machine behavior without explicit migration fixtures.
- Allowed To Change: Internal service boundaries, build projects, compatibility aliases, package schemas, generated registration, parameter declaration infrastructure, strict v2 state contracts, test harnesses, documentation, and default-off discovery.
- Inputs Needed: Current dependency inventory, target openFrameworks/compiler support window, and evidence from the first extracted reference element.

## Analysis

- Touch Map: `ofApp`, `Layer`/future Element SDK, `LayerFactory`, `LayerLibrary`, Visual Studio projects, ParameterRegistry, scene encoding/loading, MIDI/OSC mappings, package/preset schemas, Browser package controls, validation tools, native tests, and architecture/project-ops docs.
- Risks: Premature interface extraction, dependency cycles, ABI overpromising, scene/mapping regression, duplicate parameter truth, GL-state leakage, test seams that differ from the host, and an overly broad migration.
- Alternatives Considered: Continue incrementally extending the current layer package UI without extracting the host boundary; rejected because it would automate installation around a monolithic build and ambiguous runtime contract.

## Design Alignment

- Guiding Principles Affected: Stable public contracts, explicit ownership, deterministic validation, visible mappings, controlled activation, backward compatibility, and show-first reliability.
- Systems / Elements / Processes Used: Spine services, elements, composition layers, scenes, parameters, modifiers, adapters, presets, mappings, packages, generated registration, confidence gates, and Project Ops promotion.
- Alignment Rationale: The plan makes physical dependencies follow the accepted architecture while using current working features as regression evidence.
- Design Alignment Log Update: 2026-07-26 canonical spine/element/layer model and this Project Ops request establish the architecture authority and execution order.
- Student-Facing Explanation: First separate the instrument from the artwork, then define the plug shape, then prove one artwork fits safely, and only then automate finding and installing more artwork.

## Plan

- Steps: Execute SEAC-1 through SEAC-12 in order; later tasks may begin in parallel only when their predecessor's public contract is frozen and its regression gate passes.
- Validation Plan: Preserve the current Release, public-app, BrowserFlow, Hotkey, package, scene/mapping, and layer-authoring gates; add focused SDK dependency, lifecycle, descriptor, teardown/reload, rendered-output, and performance checks as their seams become real.
- Rollback / Stop Conditions: Stop promotion if extraction creates circular host/SDK dependencies, changes projection output, requires public ID churn without aliases, weakens scene or mapping recovery, makes isolated behavior differ from host behavior, or enables code/content discovery without deterministic conflicts and rollback.

## Task Graph

| Task ID | Description | Status |
| --- | --- | --- |
| SEAC-1 | Accept and route one canonical spine/element/layer vocabulary, capability ladder, and compatibility policy. | Done |
| SEAC-2 | Inventory current host/element dependencies and freeze Element SDK v1 identities, lifecycle, service, render, resource, and capability decisions. | Done |
| SEAC-3 | Complete the Element SDK/runtime-core/host build boundary, including controlled static registration outside `ofApp.cpp`, host-only GPU-target ownership, and presentation delegation without a raw mutable render-target seam. | Done |
| SEAC-4 | Make static `ElementDescriptor` action declarations and structured parameter declarations authoritative; enforce live binding parity and generate catalog, manifest, Browser, and documentation views from those declarations. Execute SEAC-4A descriptors/actions before SEAC-4B parameters/catalog parity. | Done |
| SEAC-5 | Define and implement versioned state ownership for defaults, presets, scenes, mapping banks, machine profiles, operator preferences, provenance, and migrations. | Done |
| SEAC-6 | Generalize the single-element confidence suite for dependency resolution, setup, descriptor comparison, deterministic update, offscreen rendering, GL containment, teardown/reload, and performance reporting. | Done |
| SEAC-7 | Serialize the frozen type descriptors, parameter declarations, compatibility requirements, capabilities, definitions, assets, presets, mapping suggestions, tests, and migrations into Element Package v1; validate package/runtime descriptor parity before activation. | Complete |
| SEAC-8 | Generate the controlled registration records from validated package/type metadata so a reference source element requires no `ofApp.cpp`, handwritten aggregate, or project-list edit. | Complete |
| SEAC-9 | Implement transactional preset and mapping preview/apply/edit/remove flows, including explicit parameter and action targets, trigger/edge semantics, layer-instance expansion, provenance, conflicts, and rollback. | Complete |
| SEAC-10 | Enable default-off package and supported data-only content discovery with stable IDs, duplicate policy, activation boundaries, and recovery. | Complete |
| SEAC-11 | Make and document the evidence-based native-module decision; implement a versioned module spike only if it adds value beyond generated registration. | Complete: no native runtime loader in v1 |
| SEAC-12 | Migrate representative visual, media/content-backed, and complex stateful elements; publish the authoring guide and close the architecture gate. | Complete |

SEAC-3R, SEAC-4A, and SEAC-4B are ordered execution gates inside the existing
12-task graph; they do not increase the milestone step count. SEAC-3R is
complete: controlled handwritten aggregation owns registration and
`HostCompositionRenderer` owns presentation/GPU targets behind a narrow host
effect interface. SEAC-4A is complete: minimal static type/kind/action
descriptors and the exact live handler-binding set are authoritative. SEAC-4B1
established the first construction-free static parameter declaration, and
SEAC-4B2 binds that declaration to Signal Bloom's live instance storage while
making its static metadata/defaults authoritative. SEAC-4B3 closes the gate:
all 23 built-ins now have authoritative static declarations and exact runtime
parity. SEAC-8 then made the validated package declaration the generated
Runtime/build authority for Signal Bloom.

SEAC-5A was the contract freeze inside SEAC-5; it does not increase the
milestone step count. The canonical
[`State Ownership And Provenance Contract`](../../contracts/state_ownership_and_provenance.md)
now owns the matrix, value-origin vocabulary, independent artifact versions,
legacy/current reader rules, portability boundary, and ordered implementation
slices. The implemented Scene, mapping-bank, machine-profile, preferences,
value-origin, portable-state, and Text transactions now satisfy its promotion
gate.

## Ordered Roadmap

### Priority 0: Preserve The Working Show Baseline

Dependency, not duplicate scope:

- Reproduce and close the dual-screen hiccup.
- Complete the heaviest-scene save/reload/restart and device-recovery rehearsal.
- Archive a known-good scene and mapping snapshot.
- Record current build, frame-time, contract, and visual baselines before
  extraction begins.

These residual checks are deferred and do not block SEAC-2. They must still be
completed before the show-readiness request itself closes.

### Priority 1: Freeze The Plug Shape

Tasks: SEAC-1 and SEAC-2.

Deliverables:

- One vocabulary and identity map.
- Current-versus-target capability matrix.
- Element SDK v1 header/API decision record.
- Explicit lifecycle including failure, teardown, and reload.
- Render-context and GL-state ownership.
- Service access rules for time, transport, camera, assets, logging, and future
  shared inputs.
- Capability/dependency declaration vocabulary.
- Compatibility alias and deprecation policy.

Promotion gate: the reference API can describe current visual elements without
exposing `ofApp` or promising unsupported native loading.

### Priority 2: Make The Build Match The Architecture

Task: SEAC-3.

Deliverables:

- Minimal Element SDK include boundary.
- Runtime-core services that do not depend on Browser/Console presentation.
- Host composition root that depends on core and adapters.
- Per-element build targets.
- Registration outside `ofApp.cpp`.
- Dependency-cycle and public-header checks.

Promotion gate: the no-output-change extraction passes Release, public-app,
scene round-trip, boundary, and focused renderer-policy gates. BrowserFlow
execution and live dual-screen rehearsal remain explicitly deferred; the
BrowserFlow Release target still compiles and links.

### Priority 3: Unify Parameters And State

Tasks: SEAC-4 and SEAC-5.

Deliverables:

- Structured parameter group IDs instead of label parsing.
- One declaration path feeding runtime binding, manifests, Browser metadata,
  defaults, presets, scenes, mappings, and docs.
- Normalized new-contract units and value rules.
- Explicit value provenance.
- Versioned Scene v2, preset, mapping-bank, machine-profile, and preference
  boundaries.
- Compatibility readers/migrations for current scene and mapping files.

Promotion gate: legacy scenes and mappings round-trip unchanged while new
fixtures reject ambiguous ownership and descriptor drift.

### Priority 4: Build The Reusable Confidence Suite

Task: SEAC-6.

Deliverables:

- One command that checks one element package without launching the host.
- Focused native compilation against the real Element SDK.
- Setup/declaration comparison.
- Deterministic update and offscreen render.
- Nonblank/pixel-signature options.
- GL-state, teardown, repeated reload, memory-growth, and frame-time evidence.
- Host integration as a separate final tier.

Promotion gate: at least one simple and one stateful element pass every
applicable tier, and the host sees the same declared surface as the bench.
The execution contract, fixtures, thresholds, command shape, tier boundaries,
and stop conditions are frozen in
[`../../architecture/element_confidence_suite_v1_handoff.md`](../../architecture/element_confidence_suite_v1_handoff.md).

### Priority 5: Make Packages Real Without Overpromising Plugins

Tasks: SEAC-7 and SEAC-8.

Deliverables:

- Element Package v1 schema and authoring example.
- Declared compatibility, dependencies, capabilities, content, presets,
  mappings, migrations, and tests.
- Deterministic generated registration or equivalent controlled registry.
- Strict duplicate and unresolved-dependency failure.
- No `ofApp.cpp` edit for the reference package.

Promotion gate: a clean checkout can validate, build, register, and host the
reference source element through documented commands.

### Priority 6: Complete Operator-Owned Presets And Mappings

Task: SEAC-9.

Deliverables:

- Live preset preview and transactional apply.
- Base/live/modifier/provenance display.
- Mapping suggestion preview with layer-instance target expansion.
- Conflict comparison against current scene/operator routes.
- Explicit apply, edit, disable, remove, and rollback.

Promotion gate: no package action silently mutates a scene, live mapping, or
machine profile, and failed writes restore the prior working state.

### Priority 7: Enable Controlled Discovery

Task: SEAC-10.

Deliverables:

- Default-off package-root discovery.
- Supported data-only content discovery through precompiled element types.
- Stable IDs independent of local absolute paths.
- Duplicate, replacement, refresh, deletion, and unavailable-content policy.
- Inspect-before-activate Browser flow.

Promotion gate: discovery can encounter malformed, duplicate, incompatible, or
missing content without changing the active show state.

### Priority 8: Decide, Do Not Assume, Native Modules

Task: SEAC-11.

Execution handoff:
[`native_module_decision_v1_handoff.md`](../../architecture/native_module_decision_v1_handoff.md).

Decision criteria:

- Does generated registration already satisfy the authoring workflow?
- Can a stable C ABI or narrow module boundary avoid exposing openFrameworks
  and C++ standard-library ABI details?
- Are compiler, runtime, add-on, GPU, dependency, unload, and crash policies
  supportable?
- Is process isolation or a Spout/NDI-style external bridge safer for arbitrary
  experiments?

Promotion gate: publish the decision. A rejected module loader is a valid
outcome; architecture honesty is more important than a plugin claim.

Outcome: Complete. The reviewed inventory contained no blocked native-code
workflow. Synaptome v1 retains generated registration, bounded data-only
templates, and external bridges, with a testable reversal gate for any future
native-loader proposal.

### Priority 9: Prove The Spine With Representative Elements

Task: SEAC-12.

Execution handoff:
[`representative_element_migration_v1_handoff.md`](../../architecture/representative_element_migration_v1_handoff.md).

Reference set:

- Grid as the simple generative element.
- StlModel plus the generated tetrahedron as the content-backed element.
- Lenia and Circuit Lenia as the complex deterministic simulation.

Deliverables:

- Migration guide from raw `ofApp` reference to wrapped and parametric element.
- Authoring/package/preset/mapping examples.
- Independent build and bench commands.
- Compatibility and release checklist.

Promotion gate: another artist can follow the documented path without
source-code archaeology or private host knowledge.

## Execution

- 2026-07-26 - Accepted the spine/element/layer terminology and capability model.
- 2026-07-26 - Created the consolidated Project Ops sequence and placed it behind the show-safe checkpoint.
- 2026-07-26 - Promoted SEAC to execution after the operator deferred dual-screen and full recovery rehearsal.
- 2026-07-26 - Completed the host/build/registration/test inventory and froze
  Element SDK v1 as a source/static-link boundary in
  `docs/architecture/element_sdk_v1_boundary.md`.
- 2026-07-26 - Added the public compatibility include root, shared C++/SDK
  property sheets, Signal Bloom shipping and compile-contract targets, and one
  controlled host/bench registration entrypoint. The host now links Signal
  Bloom without compiling or including its concrete implementation.
- 2026-07-26 - Added the first Runtime facade and delegated generic visual
  element create/configure/setup/activate/release behavior from `ofApp`.
  Runtime now distinguishes definition and instance identity, reserves empty
  parameter namespaces, records structured lifecycle failures, rejects
  out-of-namespace registrations, and removes only the exact parameters an
  element registered.
- 2026-07-26 - Added the focused `RuntimeCoreTest` build/profile and hardened
  `ParameterRegistry` so parameter IDs are unique across value kinds and
  namespace removal cannot confuse `console.layer1` with
  `console.layer10`.
- 2026-07-26 - Added `SynaptomeRuntimeCore` as a first-class static-library
  solution target. The app no longer compiles `Runtime.cpp` or
  `LayerFactory.cpp`; it links the runtime-core target.
- 2026-07-26 - Moved the fixed eight composition records, creative element
  pointers, and per-layer FBOs into Runtime. `ofApp` retains a temporary
  host-only compatibility view for the unchanged effect/compositing algorithm,
  while generic element adoption, release, resize, update, and draw dispatch
  now go through Runtime.
- 2026-07-26 - Restored exact cleanup for the host-owned layer-opacity
  parameter and made MIDI prefix unbinding namespace-aware so layer 1 cannot
  remove layer 10 or textual siblings.
- 2026-07-26 - Hardened the facade so prepared element ownership cannot escape,
  composition adoption rejects foreign runtimes and mismatched
  `console.layerN` addresses, and explicit shutdown releases element/FBO
  resources while the graphics context is live.
- 2026-07-26 - Moved candidate setup into an isolated parameter registry and
  made same-address adoption an atomic registry/element commit. Failed setup,
  foreign/host-ID collision, and commit rejection preserve the current element
  and live registry. Matching stable IDs retain modifiers while candidate
  defaults remain authoritative.
- 2026-07-26 - Changed the host replacement path to prepare/adopt before
  publishing slot metadata. Target pointers are rebound after commit while
  MIDI/OSC mapping definitions and per-layer FBOs remain intact.
- 2026-07-26 - Added `CompositionCoverageWindow` and
  `Runtime::resolveEffectCoverage` as the Runtime-owned, zero-based half-open
  coverage policy. `drawConsole` now consumes that result, while the duplicate
  `PostEffectChain::CoverageWindow` and `resolveCoverageWindow` policy were
  removed. `PostEffectChain` remains the concrete built-in shader and parameter
  executor; no public Element SDK or effect ABI surface was added.
- 2026-07-26 - Added `CompositionKind`, `CompositionAssignment`,
  `CompositionMutationError`, and `CompositionMutationResult`. Runtime now
  commits element adoption, effect/overlay assignment, active state, label,
  coverage, clear, and stable layer-opacity registration. `ofApp` reads a const
  live composition view, obtains FBOs through `CompositionRenderTargets`, and
  uses `legacyCompositionElementForHost` for remaining mutable type-specific
  compatibility adapters. Read-only compatibility inspection still follows
  const element pointers in the live view; `ofApp` no longer writes assignment
  metadata directly.
- 2026-07-27 - Added `CompositionLayerSnapshot` and `CompositionSnapshot`.
  Runtime now returns the fixed composition or one bounds-checked layer by
  value. Host metadata consumers use those copies; the public live aggregate
  and `CompositionLayer` accessor were removed. Read-only element inspection,
  mutable legacy element actions, and render targets remain separate named
  host-only seams.
- 2026-07-27 - Added
  `Runtime::prepareCompositionElementReplacement(zeroBasedIndex, request)`.
  Runtime now resolves the live replacement target internally and rejects
  out-of-range, empty, effect, and overlay layers before factory construction.
  Generic replacement no longer asks the host for a mutable element pointer.
  After commit, the caller-held prepared result guards the retired element
  through parameter and derived-pointer invalidation. Mutable legacy access
  remains only for optional Perlin/Game of Life post-install actions and
  `refreshLayerReferences()` derived-pointer caching.
- 2026-07-27 - Removed all derived Grid/Geodesic/Perlin/Game of Life pointer
  caches and the refresh path. Snapshot-selected registry prefixes now drive
  ordinary HUD reads, MIDI/OSC binding, Grid density cycling, and Game of Life
  pause. Geodesic subdivision adjustment and immediate Game of Life
  randomization are the only remaining mutable compatibility actions.
- 2026-07-27 - Added the public live-instance action contract. Runtime stages
  validated no-argument handlers, copies pointer-free descriptors into
  composition snapshots, and invokes actions by layer index plus local ID.
  Geodesic subdivision and Game of Life immediate randomization now use that
  path; no persisted action mapping or static descriptor claim was added.
- 2026-07-27 - Added the public typed telemetry contract and separate
  `Runtime::compositionElementTelemetry()` query. Collection is const,
  owner-thread, non-reentrant, pointer-free, and valid for adopted inactive
  elements without burdening ordinary composition snapshots. Webcam and clip
  source labels plus webcam capture readiness moved to exact `media.*`
  telemetry IDs. Geodesic subdivisions became a registered parameter, and the
  read-only `compositionElementForHost` seam was removed.
- 2026-07-27 - Consolidated host registration in the controlled
  `BuiltinElements.cpp` aggregate. Twenty-two core creator bindings live in the
  aggregate; Signal Bloom delegates to one package leaf registrar shared by
  the aggregate and package bench. `ofApp` no longer owns creator lambdas or
  registration-only concrete includes. This is the SEAC-3R controlled-source
  checkpoint, not generated registration or SEAC-8 completion.
- 2026-07-27 - Added host-only `BuiltinElementHostBindings` and moved all 12
  `overlay.text.*` registrations plus per-frame font-selection synchronization
  out of `ofApp`. The adapter privately owns the legacy `TextLayerState`
  bridge, is excluded from RuntimeCore and element/package targets, and is
  covered by a focused Browser scenario that does not instantiate
  `TextLayer`. Canonical and combined parameter manifests retain the same
  parameter semantics and counts with updated source provenance. The singleton,
  shared-instance values, and global compatibility IDs remain; candidate
  configuration is now staged and published only after Runtime adoption, so
  the pre-adoption leak is closed without claiming per-instance Text state.
- 2026-07-27 - Closed SEAC-3R. Runtime retains composition state, lifecycle,
  mutation/replacement, immutable queries, coverage policy, and generic element
  dispatch, but no longer owns or exposes FBOs. `HostCompositionRenderer`
  privately owns the fixed per-slot targets and composite output, presents the
  latest frame/preview, and consumes `PostEffectChain` only through
  `HostCompositionEffects`. `ofApp` delegates composition rendering and
  graphics-resource release; `CompositionRenderTargets` and
  `compositionRenderTargetsForHost` are retired.
- 2026-07-27 - Added `HostCompositionRendererTest`, a dedicated stub-backed
  policy harness that compiles the production renderer, Runtime, and
  `LayerFactory` without `ofApp`, `PostEffectChain`, the RuntimeCore library
  shortcut, or the openFrameworks library. It proves traversal, effect coverage
  and ordering, fail-open behavior, target reuse/release, and allocation status.
  It does not prove pixels, shader execution, a real GL context, or live output.
- 2026-07-27 - Completed SEAC-4A. Added the public minimal
  `ElementDescriptor` and closed `ElementKind`, changed `LayerFactory` to
  validate and atomically store descriptor-plus-creator records, and added
  construction-free descriptor lookup plus copied enumeration. Runtime now
  rejects missing or non-visual descriptors at the descriptor stage before
  prefix reservation or creation, seeds action tables from the static ordered
  declaration, and requires each declared handler ID to be bound exactly once
  with no undeclared, duplicate, or empty binding.
- 2026-07-27 - Migrated all 23 shipping type registrations to explicit
  `Visual` descriptors. Twenty-one declare no actions; Geodesic declares
  `subdivision.increment` then `subdivision.decrement`, Game of Life declares
  `simulation.randomize`, and Signal Bloom declares none. Snapshots preserve
  the static declaration order and metadata. This does not add package
  serialization, generated registration, authoritative parameters, or
  persisted action mappings.
- 2026-07-27 - Completed SEAC-4B1. Added the public pointer-free
  `Parameter.h` declaration DTOs for parameter kinds and values, ranges/steps,
  groups, inline options, option sources, quick-access order, aliases, and
  deprecation metadata. `ElementTypeContract` pairs that parameter set with
  the already-minimal `ElementDescriptor`; it does not expand the descriptor
  or serialize a package.
- 2026-07-27 - `LayerFactory` now records each registration atomically as
  either `Declared` with an `ElementTypeContract` or
  `LegacySetupDiscovery` with the minimal descriptor and creator. Validation,
  lookup, and copied enumeration operate on pure values without constructing
  an element; invalid or duplicate declarations preserve the prior record.
- 2026-07-27 - Signal Bloom is the first declared shipping fixture: five
  ordered groups and 18 ordered parameters in package order, including explicit
  visibility, scale options, the transport BPM-multiplier option source, no
  quick-access entries, and the exact `alpha` to `opacity` deprecation.
  Its explicit binding path is the reference implementation.
- 2026-07-27 - Completed SEAC-4B3 and closed SEAC-4. A reviewed catalog-wide
  snapshot now generates compiled Runtime declarations, the parameter catalog,
  Browser group/count inspection, compatibility manifests, and the human
  reference. The compatibility adapter binds existing setup storage by exact
  ID/kind while discarding setup metadata. The live gate passed all 23 types
  and 55 catalog assets.

## Validation

- Passed: Changed Markdown links resolve.
- Passed: Project Ops repository audit.
- Passed: `show_readiness_operator_stability` request audit.
- Passed: `git diff --check`.
- Passed: The pre-extraction Release/public-app, BrowserFlow, Hotkey,
  LayerPackageBench, authoring-profile, and Project Ops baselines remain the
  preserved promotion baseline.
- Passed: `python tools/validate_layer_authoring.py signal-bloom-sdk --native
  --incremental-app`.
- Passed: Public app validation (18 committed-fixture contracts), `pytest` (8
  tests plus 2 subtests), and BrowserFlow Release (36 scenarios). The former duplicated
  host-side coverage-window scenario moved to focused RuntimeCore coverage.
  BrowserFlow now also covers live/base/range registry views and MIDI rebind
  snap/step fidelity across element replacement.
- Passed: Element boundary policy, public/shipping source parity, shared
  registration, generated manifest, and catalog golden checks.
- Passed: `python tools/gen_builtin_element_contracts.py --check` (23 types,
  786 parameters).
- Passed: SEAC-5A read-only audits covered portable defaults/presets/scenes,
  mapping and bank persistence, fragmented machine/operator state, version
  handling, migration fixtures, and executable gates without reading or
  promoting dirty live runtime artifacts.
- Passed: The State Ownership And Provenance Contract freezes value precedence
  and origin, artifact ownership/version boundaries, missing-versus-empty
  semantics, non-destructive migration, Scene v1/v2 classification, package
  mapping suggestion policy, machine/preference separation, and legacy Text
  constraints.
- Passed: `SceneStateDocument` is pure and source-immutable; missing/explicit
  v1 normalizes to v2 only in memory, current v2 is copied unchanged,
  malformed/future versions reject before planning, and future-version input
  cannot silently activate an older backup.
- Passed: The scene golden records legacy/current classification, the
  compatibility schema accepts only v1/v2 when a version is present, the
  transaction source gate pins normalization before mapping ownership, and
  fixture-only scene/target/HUD/Console/package checks pass.
- Passed: BrowserFlow Release builds and all 39 scenarios pass, including
  Scene version/source-immutability/omitted-versus-empty plus mapping-bank
  version, explicit-empty, standalone save/reload, recovery, and
  no-future-downgrade scenarios.
- Passed: `MappingBankDocument` normalizes legacy unversioned snapshots only
  in copied memory, accepts exact v1, rejects the separate public interchange
  vocabulary, and distinguishes unsupported future versions. Canonical export
  emits all four route arrays; Scene planning validates embedded snapshots
  before apply, and a failed route-publication callback restores prior route
  vectors and button edge state.
- Passed: built-in live contract validation (23 types, 55 assets).
- Passed: RuntimeCore native adapter coverage for authoritative metadata,
  configured base values, live storage, cleanup, and missing/extra/wrong-kind
  failures.
- Passed: BrowserFlow Release execution (36 scenarios), including stable Lenia
  variant parameters and hermetic Circuit Lenia mapping defaults.
- Passed: `python tools/validate_layer_authoring.py runtime-core --native
  --incremental-app`.
- Passed: Runtime lifecycle identity, prefix reservation, exact ownership,
  failed-setup rollback, foreign-registration rejection, abandoned-result
  cleanup, and sibling-namespace preservation.
- Passed: Real-openFrameworks linked runtime-core build and composition
  adoption/update/draw/resize/release/shutdown native contract, including
  cross-runtime and wrong-address rejection.
- Passed: Destructive failed setup, abandoned staging, host-ID commit
  collision, successful same-address replacement, stable modifier
  preservation, and live-registry rebind contracts.
- Passed: Scoped element type registries resolve independently with no
  process-global fallback; scene validation checks registration without
  constructing an element; BrowserFlow covers injected offline hydration and
  the no-creator case.
- Passed: RuntimeCore covers all-prior, nearest-layer, fractional, first-layer,
  negative, invalid-index, and half-open effect coverage-window behavior.
- Passed: RuntimeCore covers typed mutation errors, transactional assignment
  mismatch rollback, element/effect/overlay assignment, active/label/coverage
  commands, exact clear behavior, stable layer-opacity address and modifier
  preservation, teardown cleanup, and composition bounds.
- Passed: Failed Runtime clears preserve live MIDI/OSC mappings and propagate
  through reassignment, Console/Browser unload, bulk clear, and scene
  publication rather than reporting a destructive partial success.
- Passed: RuntimeCore and Element SDK boundary validators enforce pointer-free
  by-value snapshots, explicit mutation commands, complete removal of mutable
  element and render-target access, absence of live host aggregate access,
  host-only renderer/effect wiring, and no public composition/effect SDK leak.
- Passed: RuntimeCore proves snapshot capacity and bounds, empty/element/effect/
  overlay projection, copy isolation, mutation freshness, clear behavior, and
  no element construction during query.
- Passed: RuntimeCore covers slot-addressed replacement bounds, empty/effect/
  overlay rejection before construction, live-state preservation during
  preparation and abort, cross-layer and stale-candidate rejection, atomic
  commit, lifecycle progress, retired-element guard lifetime, clear/reuse
  generation changes, and candidate cleanup after Runtime expiry.
- Passed: RuntimeCore action coverage rejects invalid/duplicate IDs, empty
  labels, invalid group IDs, empty handlers, and throwing registration; proves
  pointer-free live discovery and copy isolation; translates all invocation
  errors and handler outcomes; scopes local IDs per layer; and verifies atomic
  replacement, clear, shutdown, exception containment, and
  handler-before-element destruction.
- Passed: Runtime boundary validation rejects derived element caches,
  pointer-taking Perlin/Game of Life MIDI adapters, and mutable element access
  from host action paths. It pins live registry reads/writes,
  descriptor-sourced MIDI ranges, established snap/step policies,
  snapshot/registry-backed core OSC routes, pointer-free action discovery,
  structured Runtime dispatch, exact built-in action IDs, and the absence of
  those IDs from persisted JSON.
- Passed: Public telemetry uses a closed typed value variant and pointer-free
  entries. RuntimeCore covers empty/inactive success, structured bounds/kind/
  contract/collection failures, lookup/type helpers, copy isolation, and
  exception containment. Host media status consumes exact on-demand telemetry
  IDs; Geodesic subdivision reads the durable registered parameter.
- Passed: `compositionElementForHost` and all host concrete status casts are
  removed. Telemetry remains absent from ordinary composition snapshots,
  persistence, mappings, package/catalog contracts, and HUD feed ownership.
- Passed: The Release host build, LayerPackageBench build/run, and BrowserFlow
  Release build use the consolidated registration path. Element SDK/runtime
  boundary, cellular, circuit, catalog, canonical/combined parameter-manifest,
  and diff checks pass. Signal Bloom remains present with 23 registered factory
  types and unchanged parameter IDs/counts.
- Passed: Public-app validates all 18 committed-fixture contracts, including the static scene and
  display contract; the explicit scene/display transaction check also passes.
- Passed: The Element SDK boundary validator enforces the host-only Text bridge,
  exact 12-ID binding set, absence of direct Text dependencies from `ofApp`,
  exclusion from RuntimeCore and element/package targets, and the zero-Text
  Browser test wiring. Canonical and combined manifests remain semantically
  identical apart from binding-source provenance and retain their parameter
  counts.
- Passed: The Release host and BrowserFlow Release target compile and link with
  the host-binding adapter. BrowserFlow executes successfully with all 36
  scenarios passing; the operator also reports successful dual-screen use.
- Passed: `HostCompositionRendererTest` Release builds and reports PASS for
  stub-backed traversal, coverage, fail-open, target reuse, release, and
  allocation-status behavior. This is policy/draw-dispatch evidence, not
  pixel, shader, real-GL, or live-projection evidence.
- Passed: Focused factory coverage rejects invalid descriptor grammar and
  duplicate registrations atomically; construction-free lookup and copied
  enumeration preserve registry state; a failed replacement leaves the prior
  descriptor and creator intact.
- Passed: RuntimeCore rejects missing descriptors and non-`Visual` kinds at
  the descriptor stage without reserving a prefix or invoking a creator. It
  rejects missing, undeclared, duplicate, and empty action bindings, proves
  cleanup ordering and atomic replacement, and publishes canonical static
  action metadata/order in copied snapshots.
- Passed: Shipping registration validation covers all 23 built-in types as
  `Visual`, with the exact Geodesic and Game of Life declarations and empty
  declarations for the other 21. LayerPackageBench inspects Signal Bloom's
  copied empty-action descriptor and enumeration isolation before creation,
  then passes its existing lifecycle/parameter/stub-draw contract.
- Passed: The Element SDK compile contract accepts the public pure-value,
  pointer-free parameter declaration surface and closed value kinds without
  importing host or openFrameworks dependencies.
- Passed: Focused factory coverage validates declaration grammar, kind/default/
  range consistency, options and option-source rules, quick-access references,
  aliases and deprecations, atomic failure behavior, copied-state isolation,
  and explicit `Declared` versus `LegacySetupDiscovery` inspection without
  construction.
- Passed: LayerPackageBench inspects Signal Bloom's five ordered groups and 18
  ordered declarations before creation, proves exact reviewed-package/static
  parity plus compatible live ID/kind/range registration, then reports PASS
  for its existing 18-parameter, 240-update, stub-backed draw contract.
  Public and shipping Signal Bloom registrar bodies remain in parity.
- Passed: Preferences v1 validates canonical, legacy migration, five rejection
  fixtures, section-preserving publication, rollback, and callback exception
  containment. Bank-definitions v1 validates canonical/empty plus six
  rejection cases and provides the independent recoverable app publication
  seam for custom global banks.
- Passed: Shared Text candidates remain isolated through preparation and
  publish only after Runtime adoption. The portable-state gate scans 85
  portable JSON artifacts plus eight classification cases and rejects physical
  webcam selectors and absolute local paths.
- Passed: BrowserFlow Release executes all 49 scenarios. Physical and
  junction-opened Release app builds complete with zero errors; existing
  openFrameworks/add-on warnings remain outside the architecture change.
- Passed: `python tools/validate_configs.py --public-app` validates all 24
  current app/runtime contracts. Parameter/catalog/generated-registration
  outputs are current, and the extraction manifest has zero review-gated or
  unclassified files.
- Passed: SEAC-12 migrates Grid, STL Model, and Lenia to explicit bind-only
  storage, retains exact declaration/live/catalog parity, and reduces the
  compatibility setup-adapter count from 22 to 19. Grid, Signal Bloom, STL,
  and Lenia confidence profiles pass with real-context visual evidence.
- Passed: On 2026-08-16 the operator built and launched from Visual Studio and
  confirmed Grid, STL Tetra, organic Lenia, and Circuit Lenia load in the real
  host. Generated Tetrahedron Fixture discovery and failure isolation remain
  covered by controlled-discovery, BrowserFlow, and confidence gates.
- Current Gate: Complete. SEAC-1 through SEAC-12 are promoted. Audio, webcam,
  display, path ownership, the remaining 19 adapters, and physical-MIDI/show
  rehearsal remain independently routed follow-ups rather than architecture
  blockers.
- Not Run: Live physical-MIDI hardware control and the complete show-machine
  recovery rehearsal remain deferred.
- Local-state caveat: Public Circuit Lenia validation now uses a committed
  mapping fixture rather than the operator-local MIDI map. Dirty local maps,
  scenes, and backups remain outside the architecture evidence and were left
  untouched.
- Manual Evidence: User reports the Release app and dual-screen mode working
  well and confirms the four SEAC-12 reference presentations load in the real
  host. Live physical-MIDI hardware has not been tested.

## Doc Sync

- Roadmap updated: Yes
- Changelog updated: Yes
- Related docs updated: Yes
- Links checked: Yes

## Post-Mortem

- Lessons: Package UI and discovery should follow, not define, the spine/element contract and build boundary.
- Follow-ups: Rebaseline `layer_package_compatibility_bench_scaffolding` against SEAC-7 and SEAC-9 before resuming its implementation.

## Notes

- Existing `Layer` source names and `layer` schema fields remain compatibility
  terms until a separately validated alias/migration step.
- In public language, elements run in ordered composition layers.

# Spine And Element Architecture Convergence

State Summary
- Request ID: spine_element_architecture_convergence
- Phase: EXECUTION
- Status: In Progress
- Steps Complete: 2 / 12
- Progress: SEAC-3 is in progress. `SynaptomeRuntimeCore` now owns staged element setup, atomic same-address registry/element/action replacement, the fixed eight composition records, FBO state, exact parameter ownership, generic lifecycle/render routing, zero-based effect coverage-window policy, the composition mutation control plane, immutable by-value composition queries, live-instance action dispatch, and a separate on-demand typed telemetry query. Action descriptors and telemetry values cross Runtime boundaries without handlers or element pointers. Geodesic subdivisions are durable parameter state; media source/capture status uses volatile telemetry. The mutable and read-only composition-element seams are removed.
- Last Step Outcome: 2026-07-27 - Added public pointer-free typed element telemetry and `Runtime::compositionElementTelemetry()`, migrated webcam/video source status without adding collection to ordinary composition snapshots, and registered Geodesic subdivisions as durable parameter state.
- Next Step: Continue reducing the render-target seam. Static/offline `ElementDescriptor` action declarations, catalog parity, and persisted action mappings remain SEAC-4 gates.
- Dependencies / Overlap: `show_readiness_operator_stability`, `layer_package_compatibility_bench_scaffolding`, `docs/architecture/synaptome_spine_element_model.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, parameter/scene/mapping contracts, and layer-authoring tests.
- Primary Scope: runtime
- Secondary Scopes: contracts, artist-sdk, tests, docs, release
- Blocking Issues / Unknowns: Native binary modules remain an optional architecture decision rather than a promised deliverable. Typed descriptor/package catalog ownership, static action declaration parity, persisted action mapping semantics, concrete host effect execution, and the render-target seam remain boundaries.
- Impact / Priority Notes: This is the active architecture lane and precedes automatic discovery, broader package activation, or new content-family expansion.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: The architecture direction, compatibility policy, ordered tasks, and stop conditions are explicit; the operator accepted residual show-validation risk and authorized execution.
- Project Ops / Roadmap Updates (timestamped): 2026-07-26 - Added the canonical model and subordinated package/discovery work to its contract and build gates. 2026-07-26 - Promoted SEAC to execution after dual-screen validation was deferred. 2026-07-26 - Completed the dependency inventory and froze the Element SDK v1 source/static-link boundary. 2026-07-26 - Landed the first SEAC-3 build and registration slice. 2026-07-26 - Moved generic element preparation/release and exact registration ownership behind the first Runtime facade seam. 2026-07-26 - Linked the first runtime-core library and moved fixed composition storage plus generic update/draw/resize ownership behind it. 2026-07-26 - Added isolated parameter staging and transactional same-address visual-element replacement. 2026-07-26 - Hardened reserved opacity ownership, prepared-result lifetime, FX/UI-to-visual adoption, bool modifier migration, and registry-consumer invalidation. 2026-07-26 - Removed the global element factory and proved per-Runtime type-registry isolation. 2026-07-26 - Moved zero-based effect coverage-window policy into Runtime and removed the duplicate `PostEffectChain` resolver without expanding the Element SDK. 2026-07-26 - Added the Runtime composition mutation control plane, Runtime-owned layer opacity, a const-only host view, and narrow render/legacy-element seams. 2026-07-27 - Replaced the const live host view with pointer-free by-value snapshots and removed public live composition access. 2026-07-27 - Replaced pointer-addressed generic element replacement with a zero-based composition-layer transaction and narrowed mutable legacy access to two compatibility areas. 2026-07-27 - Removed the derived element cache and moved ordinary built-in views, bindings, and parameter actions to snapshot-addressed registry access. 2026-07-27 - Added live-instance action registration, pointer-free snapshot discovery, and generic slot-addressed invocation without adding persisted action mappings. 2026-07-27 - Replaced read-only concrete element inspection with separate on-demand typed telemetry and made Geodesic subdivision durable parameter state.
- Resume From: Phase EXECUTION, State In Progress, Next Action reduce the remaining render-target seam.

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
- Context: Synaptome already has working composition layers, parameters, scenes, MIDI/OSC mappings, package fixtures, validation, and a first offscreen bench, but its build and runtime ownership remain monolithic.
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
| SEAC-3 | Extract build boundaries for the Element SDK, runtime core, host composition root, and per-element targets without changing runtime output. | In Progress |
| SEAC-4 | Make parameter declarations authoritative and structured, including groups, labels, types, units, defaults, options, deprecation, runtime binding, and generated manifests. | Planned |
| SEAC-5 | Define and implement versioned state ownership for defaults, presets, scenes, mapping banks, machine profiles, operator preferences, provenance, and migrations. | Planned |
| SEAC-6 | Generalize the single-element confidence suite for dependency resolution, setup, descriptor comparison, deterministic update, offscreen rendering, GL containment, teardown/reload, and performance reporting. | Planned |
| SEAC-7 | Promote current package scaffolding into Element Package v1 with compatibility ranges, dependencies, capabilities, assets, presets, mapping suggestions, tests, and migration metadata. | Planned |
| SEAC-8 | Replace `ofApp.cpp` element registration edits with deterministic generated registration or an equivalent controlled static/module manifest path. | Planned |
| SEAC-9 | Complete transactional live preset and mapping preview/apply/edit/remove flows with provenance, layer-instance expansion, conflicts, and rollback. | Planned |
| SEAC-10 | Enable default-off package and supported data-only content discovery with stable IDs, duplicate policy, activation boundaries, and recovery. | Planned |
| SEAC-11 | Make and document the evidence-based native-module decision; implement a versioned module spike only if it adds value beyond generated registration. | Planned |
| SEAC-12 | Migrate representative visual, media/content-backed, and complex stateful elements; publish the authoring guide and close the architecture gate. | Planned |

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

Promotion gate: a no-output-change extraction passes Release, public-app,
BrowserFlow, scene round-trip, and live smoke gates.

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

### Priority 9: Prove The Spine With Representative Elements

Task: SEAC-12.

Reference set:

- One simple generative element.
- One content-backed element such as STL or video.
- One complex persistent simulation with deterministic seeds.

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
- Passed: Public app validation (17 contracts), `pytest` (8 tests plus 2
  subtests), and BrowserFlow Release (35 scenarios). The former duplicated
  host-side coverage-window scenario moved to focused RuntimeCore coverage.
  BrowserFlow now also covers live/base/range registry views and MIDI rebind
  snap/step fidelity across element replacement.
- Passed: Element boundary policy, public/shipping source parity, shared
  registration, generated manifest, and catalog golden checks.
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
  preservation, teardown cleanup, composition bounds, and render-target bounds.
- Passed: Failed Runtime clears preserve live MIDI/OSC mappings and propagate
  through reassignment, Console/Browser unload, bulk clear, and scene
  publication rather than reporting a destructive partial success.
- Passed: RuntimeCore and Element SDK boundary validators enforce pointer-free
  by-value snapshots, explicit mutation commands, named render/read-only seams,
  complete removal of mutable element access, absence of live host aggregate
  access, and no public composition/effect SDK leak.
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
- Open Gate: Reduce or retire `compositionRenderTargetsForHost`. Static action
  descriptors, catalog parity, and persisted mappings remain SEAC-4 gates.
- Not Run: Live dual-screen hardware rehearsal remains explicitly deferred.
- Manual Evidence: User approved the architecture direction and requested a prioritized Project Ops roadmap.

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

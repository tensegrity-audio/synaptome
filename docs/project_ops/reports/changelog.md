# Synaptome Project Ops Changelog

This changelog records Project Ops and administrative workflow changes. Product release versioning remains governed by `docs/release_policy.md`.

## 2026-07-27 - architecture - seac3r_host_composition_renderer_boundary

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 and SEAC-3R complete; 3 of 12 roadmap steps done
- Runtime ownership: `SynaptomeRuntimeCore` retains composition state,
  lifecycle, transactional replacement/mutation, immutable queries, effect
  coverage policy, actions, telemetry, and generic update/resize/draw dispatch.
  Runtime and `CompositionLayer` no longer own FBOs.
- Host rendering: `HostCompositionRenderer` privately owns all fixed per-slot
  and composite GPU targets, composition traversal, latest-frame presentation,
  preview presentation, and graphics-resource release. `ofApp` delegates these
  responsibilities.
- Effect boundary: The renderer reaches the concrete `PostEffectChain` only
  through the narrow internal `HostCompositionEffects` interface. This is a
  host implementation seam, not a public Element SDK effect API or ABI.
- Retired surface: `CompositionRenderTargets` and
  `compositionRenderTargetsForHost` are removed. No raw mutable render target
  crosses Runtime, the Element SDK, or `ofApp`.
- Focused confidence: Added `HostCompositionRendererTest`, which compiles the
  production renderer, Runtime, and `LayerFactory` against controlled FBO/GL
  stubs without `ofApp`, `PostEffectChain`, the RuntimeCore library shortcut,
  or the openFrameworks library. Its Release executable passes traversal,
  effect coverage/order, fail-open, target reuse/release, preview/presentation,
  and allocation-status scenarios.
- Evidence boundary: The dedicated renderer target is stub-backed policy and
  draw-dispatch evidence. It does not prove pixels, shader execution,
  graphics-state containment in a real GL context, or live projection.
  `LayerPackageBench` likewise proves lifecycle and stub-backed draw dispatch,
  not an offscreen framebuffer image.
- Deferred validation: BrowserFlow execution and live dual-screen hardware
  rehearsal were explicitly not run because the postponed dual-screen path
  remains in that suite.
- Validation: Release host, RuntimeCore and renderer native tests, BrowserFlow
  Release compile/link, Element SDK and RuntimeCore boundaries, all 17
  public-app contracts, scene/display contracts, pytest, Project Ops, and diff
  checks pass.
- Remaining debt / next gate: Handwritten aggregate registration remains until
  SEAC-8. The shared `TextLayerState` singleton and pre-adoption
  `configure()` effects remain SEAC-4B/SEAC-5 debt. SEAC-4A static
  `ElementDescriptor` identity/action declarations and exact live-binding
  parity are now the active gate.

## 2026-07-27 - architecture - seac3r_builtin_host_binding_isolation

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3R in progress inside SEAC-3
- Host boundary: Added host-only `BuiltinElementHostBindings` and moved the 12
  `overlay.text.*` registrations plus per-frame font-selection synchronization
  out of `ofApp`. The application root no longer includes or names
  `TextLayer`/`TextLayerState`.
- Contract boundary: The adapter privately owns the legacy Text singleton
  bridge and is excluded from RuntimeCore, the public Element SDK, and
  element/package targets. This checkpoint isolates direct coupling; it does
  not claim singleton retirement or authoritative parameter declarations.
- Parameter compatibility: Existing IDs, kinds, groups, labels, descriptions,
  ranges, units, registration order, and update timing remain unchanged.
  Canonical and combined manifests retain identical parameter semantics and
  counts; only binding-source provenance changes.
- Focused confidence: Added a Browser contract scenario proving all 12 controls
  remain registered, inspectable, editable, persistent, and font-synchronized
  without a `TextLayer` instance. The scenario pins presentation/range metadata,
  duplicate-registration rejection, and restores shared singleton state.
- Roadmap boundary: `HostCompositionRenderer` and retirement of
  `compositionRenderTargetsForHost` remain the final SEAC-3R gate. The shared
  Text singleton, cross-instance values, and pre-adoption `configure()` side
  effects remain explicit SEAC-4B/SEAC-5 parameter/state-ownership debt.
- Validation: Element SDK boundary, canonical/combined parameter-manifest,
  Release host, BrowserFlow Release compile/link, Project Ops, and diff checks
  pass. BrowserFlow execution remains intentionally deferred because the suite
  includes the postponed dual-screen path.

## 2026-07-27 - architecture - seac3r_controlled_registration_consolidation

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3R in progress inside SEAC-3
- Registration boundary: `ofApp` now delegates host registration to the
  controlled `BuiltinElements.cpp` aggregate. Twenty-two core creator bindings
  live in that aggregate; Signal Bloom delegates to one package leaf registrar
  shared by the aggregate and package bench.
- Host boundary: Registration-only concrete includes and creator ownership
  moved out of `ofApp`. Direct `TextLayerState` font/state synchronization
  remains an explicit host coupling, so this checkpoint does not claim that all
  concrete host references are gone.
- Build/test wiring: The Release host compiles the aggregate and Signal Bloom
  leaf, while LayerPackageBench compiles only the narrow package registrar and
  links the existing element target.
- Contract tooling: Catalog and parameter-manifest scanners now follow the
  controlled registration sources rather than inferring creator bindings from
  `ofApp.cpp`. Canonical and combined parameter manifests retain the same IDs
  and counts; their diffs contain the new source plus expected line shifts.
- Roadmap boundary: This is controlled handwritten source registration, not
  SEAC-8 generated registration. SEAC-3 remains open for the render-target
  adapter; SEAC-4A descriptors/actions, SEAC-4B parameters,
  SEAC-7 package serialization, SEAC-8 generation, and SEAC-9 persisted action
  mappings remain ordered future gates.
- Validation: Release host, LayerPackageBench build/run, BrowserFlow Release
  build, Element SDK/RuntimeCore boundary, cellular/circuit, canonical catalog,
  canonical/combined parameter-manifest, public-app 17-contract,
  scene/display transaction, Project Ops, and diff checks pass. BrowserFlow
  execution remains deferred with the dual-screen path; the render-target
  adapter is the remaining SEAC-3R gate.

## 2026-07-27 - architecture - seac3_on_demand_element_telemetry

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Public SDK: Added `Telemetry.h` with the closed
  `bool`/`int64_t`/`double`/`string` `TelemetryValue`, pointer-free
  `TelemetryEntry`, `TelemetrySink`, and optional const
  `Layer::collectTelemetry()` hook.
- Runtime boundary: Added the separate on-demand
  `Runtime::compositionElementTelemetry()` query and structured bounds, empty,
  kind, contract, and collection errors. Results provide ID lookup and typed
  value access without exposing live elements. Telemetry is deliberately not
  appended to frequent `CompositionSnapshot` reads.
- State ownership: Geodesic subdivisions are now a registered range/step
  parameter and remain the durable authority used by actions, presets, and
  scenes. Webcam and clip source labels use `media.sourceLabel`; webcam capture
  readiness uses `media.captureInitialized`.
- Collection rules: Live collection is const, synchronous on Runtime's owner
  thread, non-reentrant, bounded, side-effect-free, and available for an adopted
  inactive element. Runtime contains exceptions and does not publish partial
  entries after collection or contract failure.
- Terminology: Element telemetry is a volatile typed observation, not durable
  state or normalized lifecycle health. It is separate from `HudFeedRegistry`
  JSON, HUD widget telemetry feed names, and static package capabilities.
- Compatibility boundary: Telemetry does not enter parameters, scenes,
  presets, mappings, packages, catalogs, manifests, routers, schemas, or
  ordinary composition snapshots. The host may adapt copied telemetry to HUD
  presentation.
- Host migration: Media status and Geodesic display reads no longer use
  concrete element casts. The retired `compositionElementForHost` declaration,
  definition, call sites, and read-only seam were removed.
- Validation: Element SDK and RuntimeCore boundary checks pin the public typed
  DTO, const hook, on-demand structured query, contract validation, exact media
  IDs, durable Geodesic parameter, inactive query semantics, leakage bans, and
  complete removal of the concrete read-only element seam.

## 2026-07-27 - architecture - seac3_live_instance_action_contract

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Public SDK: Added a pointer-free `ActionDescriptor`, no-argument
  `ActionHandler`, structured execution result, and optional
  `Layer::registerActions()` hook. Descriptors separate the required display
  `label` from a stable, required, single-segment lower-camel `groupId`.
- Runtime boundary: Candidate preparation stages and validates each live
  instance's action table. Immutable composition snapshots copy descriptors but
  never handlers or element pointers. `Runtime::invokeCompositionAction()`
  dispatches by zero-based layer index and local action ID with structured
  bounds, kind, support, rejection, and execution failures.
- Built-in migration: Geodesic declares `subdivision.increment` and
  `subdivision.decrement`; Game of Life declares `simulation.randomize`, which
  preserves the existing immediate randomization behavior rather than the
  delayed/quantized `.reseed` parameter path. Host shortcuts now use the
  generic Runtime command. The zero-caller
  `Runtime::legacyCompositionElementForHost` seam was removed.
- Compatibility boundary: These actions are live-instance commands, not
  durable state. No expanded `.actions.` address, scene/preset value, package
  declaration, parameter manifest entry, MIDI/OSC target, mapping snapshot, or
  schema change is introduced.
- Deferred gate: Static/offline `ElementDescriptor` actions, package/catalog
  inspection, declaration/registration parity, and persisted action mappings
  remain SEAC-4 work. Geodesic subdivision and video status remain read-only
  inspection debt.
- Validation: Element SDK and RuntimeCore boundary checks pin the pointer-free
  descriptor and exact `groupId` field, optional registration hook,
  action-table validation, live snapshot projection, generic structured
  dispatch, concrete RuntimeCore isolation, exact built-in IDs/groups, host
  adapter semantics, and absence of those IDs from persisted JSON. The native
  RuntimeCore action scenario covers invalid IDs, empty labels, invalid group
  IDs, empty handlers, discovery/copy isolation, structured outcomes,
  per-layer scoping, replacement, clear, shutdown, and handler lifetime. Both
  focused validators, the Project Ops audit, all 17 public-app contracts, and
  `git diff --check` pass.

## 2026-07-27 - architecture - seac3_snapshot_registry_parameter_views

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Host boundary: Removed all cached derived Grid, Geodesic, Perlin, and Game of
  Life pointers plus `refreshLayerReferences()`. Slot selection now reads copied
  composition snapshots and carries the selected stable registry prefix.
- Parameter views: Grid/Geodesic HUD summaries, Perlin/Game of Life HUD
  metadata, core Grid/Geodesic OSC routes, Perlin/Game of Life MIDI bindings,
  Grid density cycling, and Game of Life pause now resolve live
  `ParameterRegistry` storage.
- Control compatibility: MIDI domains come from registered descriptor ranges.
  The established per-target snap/step and bool-mode policies remain explicit
  and unchanged.
- Remaining element seams: Geodesic subdivision status uses const
  `compositionElementForHost` inspection. Mutable
  `legacyCompositionElementForHost` access is allowed only inside
  `adjustGeodesicSubdivisionAtSlot()` and
  `randomizeGameOfLifeAtSlot()`, after slot snapshot/type validation.
- Compatibility: Public element IDs, parameter addresses, scene and mapping
  formats, keyboard controls, MIDI/OSC target behavior, rendering, and Element
  SDK headers remain unchanged.
- Validation: Runtime boundary validation now uses brace-balanced function
  extraction and semantic action allowlisting rather than a global call count.
  It rejects derived caches, concrete pointer-taking MIDI adapters, mutable
  access outside the two action bodies, and loss of registry range or
  snap/step semantics. Scene rollback validation no longer requires a retired
  pointer-cache refresh. BrowserFlow passes all 35 scenarios, including the new
  registry-view and MIDI-rebind contracts.
- Remaining gate: Replace the two mutable compatibility adapters with a
  declared action contract, then migrate Geodesic/video status off const
  element inspection and reduce the render-target seam.

## 2026-07-27 - architecture - seac3_slot_addressed_element_replacement

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Transaction: Added
  `Runtime::prepareCompositionElementReplacement(zeroBasedIndex, request)`.
  Runtime resolves the live element internally; the removed pointer-addressed
  API can no longer accept a host-selected mutable element.
- Validation boundary: Out-of-range, empty, effect, and overlay layers reject
  replacement before factory construction. Preparation leaves the live element
  and parameters unchanged; adoption remains the atomic commit point.
- Lifetime: After a successful adoption, the caller-held prepared result owns
  the retired element until parameter consumers, derived-pointer caches, and
  routes have been invalidated. Abort, cross-layer adoption, and stale prepared
  candidates preserve the current live element.
- Host boundary: Generic replacement no longer calls
  `legacyCompositionElementForHost`. Mutable legacy access remains only for
  optional Perlin/Game of Life post-install MIDI/randomize actions and
  `refreshLayerReferences()` derived-pointer caching.
- Compatibility: Public IDs, scenes, mappings, parameter addresses, rendering,
  Element SDK headers, and effect ABI policy remain unchanged.
- Validation: RuntimeCore covers bounds, empty/effect/overlay rejection without
  construction, prepare/abort isolation, commit, lifecycle progress,
  retired-element guard lifetime, cross-layer rejection, and stale-candidate
  rollback, including clear/reuse generation changes and candidates that
  outlive Runtime. Boundary validation pins the index-addressed API and the two
  remaining mutable compatibility areas.
- Remaining gate: Replace those two mutable consumers with narrow
  Runtime/parameter/action contracts. Typed descriptor/catalog ownership
  remains a later candidate rather than part of this slice.

## 2026-07-27 - architecture - seac3_immutable_composition_snapshot

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Query plane: Added pointer-free `CompositionLayerSnapshot` and
  `CompositionSnapshot` DTOs. Runtime returns the fixed eight-layer composition
  by value or one bounds-checked optional layer copy.
- Host boundary: Metadata consumers now read snapshots. The public live
  aggregate, permanent host alias, and public `CompositionLayer` accessor were
  removed.
- DTO contract: Snapshots contain only index, occupancy, element presence,
  composition kind, definition/label/type/prefix identity, active state,
  opacity, and coverage. They contain no element, FBO, parameter-registry,
  creator, ownership, or host pointers.
- Remaining seams: `compositionRenderTargetsForHost` exposes mutable render
  targets, `compositionElementForHost` supports read-only element inspection,
  and `legacyCompositionElementForHost` supports mutable legacy adapters. These
  are explicit host-only SEAC-3 debt.
- Compatibility: Public IDs, scenes, mappings, effect behavior, and public
  Element SDK headers remain unchanged. The snapshot DTOs are RuntimeCore
  query types, not an Effect SDK, stable native ABI, plug-in, or hot-load claim.
- Validation: RuntimeCore covers capacity/bounds, all composition kinds, copy
  isolation, mutation freshness, clear behavior, and query-without-
  construction. Boundary validation rejects live host aggregate access, pointer
  or resource-bearing DTOs, direct metadata reads outside snapshots, and public
  SDK leakage.
- Remaining gate: Move compositor and element-specific compatibility consumers
  behind narrower Runtime, parameter, and action contracts so the three named
  host seams can shrink or retire.

## 2026-07-26 - architecture - seac3_composition_control_plane

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Control plane: Added `CompositionKind`, `CompositionAssignment`,
  `CompositionMutationError`, and `CompositionMutationResult`. Runtime now
  owns transactional element adoption, effect/overlay assignment, active state,
  label, coverage, and whole-layer clear.
- Parameter ownership: Runtime registers the stable
  `console.layerN.opacity` parameter during element adoption, preserves its
  address and modifiers across replacement, and removes it with the exact layer
  ownership on clear or Runtime teardown.
- Host boundary at this checkpoint: `ofApp` read a const live Runtime
  composition view and no longer wrote assignment metadata directly. Mutable
  FBO access was isolated behind `CompositionRenderTargets`; remaining mutable
  type-specific adapters used the named `legacyCompositionElementForHost` seam.
  Read-only compatibility inspection still followed const element pointers
  exposed by the live view.
- Query status at this checkpoint: The const aggregate was a transitional live
  view, not the target immutable by-value snapshot. The render and
  legacy-element methods were host-only migration seams, not Element SDK
  services.
- Compatibility: Public IDs, scene/mapping addresses, effect behavior, and
  public Element SDK headers remain unchanged. No Effect SDK, stable native ABI,
  dynamic plug-in, or hot-loading claim is introduced.
- Validation: RuntimeCore covers composition bounds, render-target bounds,
  assignment validation and rollback, all three composition kinds, active,
  label, coverage, clear, and stable opacity/modifier preservation. Boundary
  validation rejects direct host metadata writes and public SDK leakage. Failed
  Runtime clears leave live MIDI/OSC mappings intact and propagate through host
  unload, reassignment, bulk-clear, and scene-publication paths.
- Remaining gate at this checkpoint (completed 2026-07-27): Replace the const
  live aggregate with an immutable by-value query model, then reduce or retire
  the remaining renderer and legacy-element host seams.

## 2026-07-26 - architecture - seac3_runtime_effect_coverage_policy

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Ownership: Added `CompositionCoverageWindow` and
  `Runtime::resolveEffectCoverage` as RuntimeCore's zero-based, half-open
  effect coverage-window policy. `drawConsole` now consumes the Runtime result.
- Host adapter: Removed the duplicate `PostEffectChain::CoverageWindow` and
  `resolveCoverageWindow` policy. `PostEffectChain` still owns concrete
  built-in effect defaults, coverage-mask parameters, FBO/shader execution,
  and type-specific application.
- Compatibility: Coverage behavior, public IDs, scenes, mappings, and public
  Element SDK headers remain unchanged. This compiler-matched source/static
  extraction introduces no public Effect SDK, stable binary ABI, native
  plug-in, or hot-loading claim.
- Validation: RuntimeCore now covers all-prior, nearest-layer, fractional,
  first-layer, negative, invalid-index, and half-open coverage behavior.
  BrowserFlow remains 33 scenarios after removal of its duplicated host-side
  coverage-policy scenario.
- Remaining gate at this checkpoint (completed by later entries): Replace the
  mutable composition-array alias with an immutable by-value query model and
  explicit Runtime assignment/layer controls, then isolate mutable FBO and
  legacy-element access in a named internal host render bridge.

## 2026-07-26 - architecture - seac3_scoped_element_type_registry

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Ownership: Removed the process-global `LayerFactory` singleton. The host,
  RuntimeCore tests, and package bench now own independent element type
  registries and inject them into Runtime.
- Isolation: RuntimeCore proves that two registries may resolve the same type
  ID to different creators and that a type registered in one Runtime cannot
  leak into another.
- Consumers: Scene validation uses non-constructing type lookup. Control &
  Mapping receives a narrow offline element-creator callback instead of
  reaching into a global registry; BrowserFlow proves injected hydration and
  the no-callback unhydrated case.
- Compatibility: The legacy `LayerFactory` source name and all established type
  IDs remain unchanged. This is scoped source/static registration, not dynamic
  discovery or a native plug-in ABI.
- Remaining gate: Add typed descriptor/package ownership, reduce the host-only
  mutable composition view, and move remaining effect/compositing behavior
  behind the spine.

## 2026-07-26 - architecture - seac3_transactional_element_replacement

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Transaction: Element setup now runs against an isolated parameter registry.
  Runtime prebuilds the next registry, ownership map, and prefix set, then
  commits those with the new element using no-fail swaps.
- Rollback: Setup failure, host-owned ID collision, abandoned preparation, and
  invalid adoption leave the live element, metadata, registry, opacity,
  modifiers, mappings, and FBOs unchanged.
- Ownership: Runtime rejects the layer-container-owned
  `console.layerN.opacity` address during every preparation, including an
  initial empty-layer install. Prepared candidates are destroyed before their
  private staging registry on every failure and expired-runtime path.
- Host: Visual replacements prepare and adopt before slot publication.
  Target-only MIDI rebinding preserves address-based MIDI/OSC map definitions;
  stable matching parameter IDs retain modifiers while new element defaults
  remain authoritative.
- Consumers: Registry replacement synchronously invalidates Control & Mapping
  row pointers and retired element/OSC references before fallible host
  publication. FX/UI-to-visual transitions also adopt before retiring the old
  host assignment.
- Compatibility: `setup()` receives staging storage and must not retain its
  address. `onParameterRegistryCommitted()` is the no-throw live-registry
  rebind hook. This changes the compiler-matched C++ vtable and does not promise
  a stable binary ABI.
- Remaining gate: Replace the process-global factory/catalog boundary with
  scoped runtime ownership and continue moving effect/compositing adapters out
  of the host.

## 2026-07-26 - architecture - seac3_runtime_core_composition_ownership

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Build: Added `SynaptomeRuntimeCore` as a first-class static-library solution
  target. The app now links Runtime and LayerFactory instead of compiling those
  sources directly.
- Ownership: Runtime now owns the fixed eight composition records, creative
  element pointers, and their three per-layer FBOs. Generic adoption, release,
  resize, update, and draw dispatch moved behind Runtime while the existing
  effect/compositing algorithm remains unchanged in the host.
- ABI: RuntimeCoreTest now links the shipping core against the same real
  openFrameworks surface instead of mixing real and stub type definitions.
- Safety: Restored exact cleanup of host-owned slot opacity and made MIDI
  namespace unbinding preserve layer 10 and textual siblings when clearing
  layer 1. Prepared ownership is opaque, composition adoption validates runtime
  provenance and canonical slot address, and explicit shutdown releases
  element/FBO resources before the graphics context closes.
- Remaining gate: Staged parameter registration and transactional
  same-address replacement are next; the global factory/catalog boundary also
  remains.

## 2026-07-26 - architecture - seac3_runtime_lifecycle_facade

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Summary: Added the first Runtime facade and moved generic element creation,
  configuration, setup, activation, exact registration ownership, failure
  cleanup, and release out of `ofApp`.
- Contract: Separated definition and instance identity, added structured error
  code/stage/identity context, reserved prefixes for zero-parameter elements,
  and rejected/rolled back out-of-namespace registrations.
- Safety: Parameter IDs are now unique across value kinds, exact lifecycle
  ownership prevents host parameters from being swept up, and namespace
  removal cannot confuse `console.layer1` with `console.layer10`.
- Validation: Added the focused RuntimeCore native test/profile and passed its
  native lifecycle contract plus the full Release app link.
- Remaining gate: Transactional replacement, composition ownership,
  update/render routing, and the linked runtime-core library remain SEAC-3
  work.

## 2026-07-26 - architecture - seac3_signal_bloom_build_boundary

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-3 in progress
- Summary: Added the transitional public Element SDK include root and shared
  build properties. Signal Bloom now builds as its own pinned-openFrameworks
  static library and as a separate stub compile contract. The host links the
  shipping target and calls a controlled registration unit shared with the
  lifecycle bench; neither `ofApp.cpp` nor the bench textually includes the
  concrete implementation.
- Compatibility: Public IDs, package activation, parameters, scenes, mappings,
  and rendered behavior are unchanged. The current `Layer` and parameter
  builder forwarders are explicitly transitional until SEAC-4.
- Validation: Signal Bloom SDK profile passed all fast, native, lifecycle, and
  app-link stages; public app passed 17 contracts; pytest passed 8 tests plus 2
  subtests; BrowserFlow passed 32 tests; Project Ops audits passed.
- Next: Introduce the Runtime facade and move generic composition-layer
  lifecycle ownership out of `ofApp`.

## 2026-07-26 - architecture - element_sdk_v1_boundary

- Request ID: `spine_element_architecture_convergence`
- Phase / Milestone: SEAC-2 complete; SEAC-3 ready
- Summary: Inventoried the monolithic host, build graph, element registration,
  lifecycle, services, parameters, package adapter, and fragmented test seams.
  Froze Element SDK v1 as a compiler-matched source/static-link contract, with
  runtime core owning catalogs, composition, state, and control; the host
  owning platform/UI adapters; and generated built-in registration as the only
  concrete-element bridge.
- Compatibility: Existing `Layer` implementations, public IDs,
  `console.layerN.*` addresses, scenes, mappings, and runtime behavior are
  unchanged. Dynamic native modules and raw `ofApp` loading are not promised.
- Next: Begin SEAC-3 with shared build properties, a Signal Bloom SDK
  compile-contract target, and the no-output-change runtime facade extraction.

## 2026-07-26 - roadmap - defer_dual_screen_start_seac

- Request IDs: `show_readiness_operator_stability`,
  `spine_element_architecture_convergence`
- Phase / Milestone: Priority-lane handoff
- Summary: The operator explicitly postponed dual-screen and full
  show-machine recovery rehearsal. The residual checks remain open under the
  deferred show-readiness request, while SEAC moved from planning into
  execution with the host/Element SDK dependency and registration inventory as
  its first active slice.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Project Ops request parity and repository audit.
- Follow-Up Actions: Publish the preserved baseline, then execute SEAC-2
  without enabling package discovery or changing public IDs.

## 2026-07-26 - architecture - spine_element_layer_model

- Request ID: spine_element_architecture_convergence
- Phase / Milestone: Architecture convergence before package expansion
- Summary: Defined Synaptome as a stable spine hosting creative elements in
  eight ordered composition layers. Added canonical definitions for element
  types, definitions, instances, packages, parameters, mappings, presets,
  scenes, machine profiles, and content assets; documented the honest
  openFrameworks compatibility ladder and the future SDK/runtime-core/host
  build split. Existing `Layer` code/schema names and public IDs remain
  compatibility surfaces.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Changed Markdown links, Project Ops repository audit,
  `show_readiness_operator_stability` request audit, and `git diff --check`.
- Follow-Up Actions: Reproduce the reported dual-screen hiccup and complete the
  show-safe rehearsal, then promote a focused contract/build-boundary request
  before resuming mapping-preset or discovery work.

## 2026-07-25 - mappings - circuit_lenia_editable_osc_defaults

- Phase / Milestone: Circuit Lenia live tuning
- Summary: Added seven semantic Circuit Lenia OSC defaults to the ordinary
  global mapping file for threshold, contour count, trace width, growth center,
  growth width, injection rate, and field scale. Each route uses the router's
  absolute mode and remains visible/editable in Control & Mapping. The Lenia
  renderer contains no OSC addresses or router dependency.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Mapping/config target checks, Circuit Lenia authoring contract,
  native mapping edit/export/import coverage, and all 29 BrowserFlow scenarios.
- Follow-Up Actions: Tune the unattractive initial surface through the visible
  parameter controls on the show display, then save one known-good scene and
  mapping snapshot.

## 2026-07-25 - visuals - circuit_lenia_view

- Phase / Milestone: Modular circuit cellular view
- Summary: Added `generative.circuitLenia` as an independent Browser asset and
  scene/MIDI/OSC namespace on the established deterministic Lenia simulation.
  A fixed catalog-selected circuit presentation converts the continuous field
  into hard, nested isocontour traces at `160x90` with nearest-neighbor scaling.
  Circuit threshold, contour count, and trace width are labeled controls;
  threshold and width are quick-access parameters. Organic Lenia retains its
  existing presentation and does not expose the circuit-only controls.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `0.13s` Circuit Lenia authoring contract, deterministic native
  lifecycle and organic-view isolation coverage, all 28 BrowserFlow scenarios,
  regenerated 65-entry catalog/parameter/scene/Console fixtures, all 17
  public-app contracts, and the incremental Release app build pass.
- Follow-Up Actions: Visually tune threshold, contour count, trace width, and
  palette against a representative mature Lenia organism on the show display.

## 2026-07-25 - visuals - circuit_algorithm_expansion

- Phase / Milestone: Modular circuit-family expansion
- Summary: Added Circuit Ant Tunnels and Circuit Flow Field as separate catalog
  assets on the shared `circuitTrace` runtime. Ant Tunnels uses a distinct
  pheromone corridor-routing rule; Flow Field computes a deterministic analytic
  vector and quantizes it through the shared eight-direction motion seam. Both
  retain the family's coarse `256x144` nearest-neighbor presentation, complete
  30-parameter defaults, owned deterministic seeds, and independent
  scene/MIDI/OSC namespaces without duplicating a layer class.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Five-model Circuit Trace contract, 10-asset modular-family
  contract, regenerated 64-entry catalog and parameter fixtures, deterministic
  variant lifecycle coverage, all 27 BrowserFlow scenarios, all 16 public-app
  contracts, and the incremental Release app build pass.
- Follow-Up Actions: Visually accept both defaults, then design circuit cellular
  variants as distinct runtimes rather than folding different state models into
  `circuitTrace`.

## 2026-07-25 - visuals - circuit_trace_pixel_language

- Phase / Milestone: Live visual acceptance correction
- Summary: Restored the Circuit Trace family to a deliberately coarse `256x144`
  canvas with nearest-neighbor scaling so pixels remain large and hard-edged.
  Retained the circular trace kernel and clean junction fixes, avoiding the
  earlier square-grid and plus-sign artifacts. Distributed Circuit Mycelium
  startup across a four-by-three colony layout so it develops across the full
  frame rather than remaining centered.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Circuit authoring contract, isolated native eight-direction test,
  modular-family contract, 62-entry catalog regression, and incremental Release
  app build all pass.
- Follow-Up Actions: Reload each circuit asset on the show machine to allocate
  its new field and visually approve pixel scale and full-frame composition.

## 2026-07-25 - visuals - circuit_trace_presentation_cleanup

- Phase / Milestone: Live visual acceptance correction
- Summary: Replaced the Circuit Trace family's `256x144` nearest-neighbor
  presentation with a `512x288` linearly sampled field and circular soft trace
  coverage. Removed forced start/branch via stamps, removed the drilled
  plus-in-square renderer, and defaulted optional via accents off. River's
  default width was reduced so merged channels remain traces rather than
  blocks. Eight-direction movement and all public parameter IDs remain intact.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Circuit authoring contract, modular-family contract, Release app
  build, all 26 BrowserFlow scenarios in `0.852s`, catalog/manifest checks, and
  diff check.
- Follow-Up Actions: Visually reload all three assets on the show display and
  tune only width, glow, density, and color if further projection-specific
  adjustment is needed.

## 2026-07-25 - visuals - cellular_fields_migration_slice

- Phase / Milestone: Legacy-to-modular workflow alignment
- Summary: Migrated Game of Life and Excitable Media as separate Cellular
  Fields runtimes. Game of Life now owns a persisted deterministic seed and
  reproducible reseed path; Excitable Media rebuilds correctly when a restored
  scene changes its seed. Both expose complete canonical scalar defaults,
  retain legacy configuration aliases, use shared descriptor infrastructure,
  and declare only a small speed-plus-primary-behavior quick-access set.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `0.18-0.20s` Cellular Fields fast profile, validator unit tests,
  Release app build, all 26 BrowserFlow scenarios, Hotkey test, all 16
  public-app contracts, catalog/manifest checks, and diff check.
- Follow-Up Actions: Live-review scene reload/restart behavior, then migrate
  Lenia and Reaction Diffusion without merging their distinct algorithms.

## 2026-07-25 - workflow - layer_authoring_and_live_selection

- Phase / Milestone: Layer iteration acceleration
- Summary: Added a staged, profile-driven layer authoring runner with
  sub-quarter-second static validation for Circuit Trace, Adaptive Trail, and
  Collective Motion; an isolated stub-based native target avoids rebuilding
  openFrameworks during algorithm checks. Added a shared parameter builder and
  migrated the established common registration blocks in Circuit Trace,
  AgentField, Flocking, and Signal Bloom without changing public parameter
  contracts. The Console Asset Browser now supports multi-token type-to-search,
  and `Ctrl+E` opens Control & Mapping on the focused Console layer's first
  quick-access parameter.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Four fast family profiles, eight validator/runner unit tests,
  isolated native authoring target, Release app build, all 26 BrowserFlow
  scenarios, and all 16 public-app contracts.
- Follow-Up Actions: Add honest render-independent native snapshots where the
  migrated algorithms expose a stable seam, then extend the profile pattern to
  Lenia and Reaction Diffusion.

## 2026-07-25 - visuals - modular_layer_family_migration

- Phase / Milestone: Legacy-to-modular workflow alignment
- Summary: Migrated Ant Tunnels, Slime Mold, and Physarum onto an aligned
  Adaptive Trail lifecycle and migrated Schooling and Murmuration onto an
  aligned Collective Motion lifecycle. The five existing assets retain their
  stable IDs, runtime types, registry prefixes, and established mapping
  suffixes while gaining owned deterministic seed/reseed behavior, complete
  catalog defaults, and consistent lifecycle metadata. Added a reusable
  migration guide and a validator covering eight assets across the Circuit
  Trace, Adaptive Trail, and Collective Motion runtimes.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release app project build, all 23 BrowserFlow scenarios,
  dedicated Circuit Trace and modular-family validators, and the expanded
  15-contract public-app gate.
- Follow-Up Actions: Live-review the migrated assets through scene
  save/reload/restart, then audit Cellular Fields one algorithm at a time.

## 2026-07-25 - visuals - circuit_trace_family

- Phase / Milestone: Modular generative layer family
- Summary: Added one shared `CircuitTraceLayer` with Circuit Slime, Circuit
  Mycelium, and Circuit River catalog profiles. The shared motion primitive
  limits every growth step to horizontal, vertical, or 45-degree diagonal
  movement, producing organic networks with PCB-like traces and vias. Each
  profile owns a stable registry prefix and the same 30 scene/MIDI/OSC-ready
  parameters, including deterministic seed and reseed behavior.
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release app project build, all 23 BrowserFlow scenarios, Hotkey
  test, all 16 public-app contracts, dedicated eight-direction/catalog
  validation, and `git diff --check`.
- Follow-Up Actions: Visually review and tune the three defaults on the show
  machine, then save one known-good performance scene.

## 2026-07-25 - runtime - scene_mapping_recovery_hardening

- Request ID: `show_readiness_operator_stability`
- Phase / Milestone: Pre-show operator stability
- Summary: MIDI/OSC mapping files and scene snapshots now validate into
  temporary state before replacing live routes. Explicit scene mapping
  snapshots override the global configuration, including an intentionally
  empty snapshot, while legacy scenes that omit mappings preserve the working
  global/live routes and slot assignments. Scene and assignment writes verify
  temporary JSON, retain a last-known-good backup, and restore it when
  promotion fails; mapping files use the same recovery policy. Scene snapshots
  retain the active mapping bank, and a low-frequency recovery autosave updates
  `scene-last.json` without falsely marking the named scene saved. Shared
  operator status now reports scene dirty/save/load state, mapping source and
  counts, unresolved targets, MIDI retry state, and OSC receiving/stale state.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release solution build, Hotkey test, all 22 BrowserFlow
  scenarios, scene/display recovery source contract, and scene-persistence
  fixture pass. The BrowserFlow gate now includes mapping
  save/mutate/restore/restart coverage for MIDI CC, buttons, OSC profiles,
  malformed snapshots, and unavailable MIDI hardware.
- Follow-Up Actions: Run the same sequence with the heaviest show scene and
  physical MIDI/OSC devices, then archive the known-good scene and mapping
  backups.

## 2026-07-25 - runtime - mirror_symmetry_and_quit_modal

- Request ID: `show_readiness_operator_stability`
- Phase / Milestone: Pre-show operator stability
- Summary: Mirror's basic horizontal and vertical modes now preserve one
  source half and reflect it pixel-for-pixel into the opposite half. The first
  Ctrl+Q opens a modal on the focused window with `QUIT: CTRL+Q` and
  `ESC: CANCEL`; a released-and-repressed Ctrl+Q confirms, Escape cancels, and
  unrelated input is consumed while the modal is active.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release solution build, Hotkey test, all 21 BrowserFlow
  scenarios, all 13 public-app contracts, and the operator-render contract.
- Follow-Up Actions: Verify the mirror source-half orientation and quit modal
  placement on the laptop/projector pair.

## 2026-07-25 - runtime - show_safe_window_and_quit_controls

- Request ID: `show_readiness_operator_stability`
- Phase / Milestone: Pre-show operator stability
- Summary: Controller windows now clamp restored position and size to the
  selected monitor's usable desktop. Ctrl+F targets the focused window rather
  than a process-global window, so fullscreen stays on the assigned screen.
  Ctrl+Q requires a release and second press within three seconds, with armed
  status visible in the System Status HUD. The Visual Studio solution now
  points at the configured openFrameworks installation path.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release solution build, Hotkey test, all 21 BrowserFlow
  scenarios, and all 13 public-app contracts. Dual-monitor
  placement/fullscreen behavior still requires show-machine visual rehearsal.
- Follow-Up Actions: Exercise controller window restore and repeated
  fullscreen transitions on the laptop/projector pair, then continue
  unsaved-change/save-result feedback.

## 2026-07-24 - runtime - target_size_unifont_operator_text

- Request ID: `show_readiness_operator_stability`
- Phase / Milestone: Pre-show operator stability
- Summary: Bundled GNU Unifont 17.0.05 under the SIL OFL 1.1 and replaced
  fractional enlargement of the built-in operator bitmap font with
  anti-aliased target-size rasterization. The shared renderer caches the
  current pixel size, uses its own metrics for clipping and ellipsis, and
  retains the built-in bitmap font as a failure fallback.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release x64 and BrowserFlow builds; all 21 BrowserFlow
  scenarios; all 13 public-app contracts; eight-second runtime launch smoke;
  2.22-second unchanged incremental Release build; operator-render contract
  including font checksum, target-size cache, shared metrics, and fallback.
- Follow-Up Actions: Visually verify scale extremes and the show-resolution
  layout, then continue dirty/save-result feedback and recovery rehearsal.

## 2026-07-24 - runtime - global_text_scale_and_lossless_mirror

- Request ID: `show_readiness_operator_stability`
- Phase / Milestone: Pre-show operator stability
- Summary: Promoted the existing menu text-size parameter into a global
  operator-interface scale across every app surface, including matching row
  spacing. Mirror's horizontal and vertical modes now flip the complete input
  frame while preserving sampled RGBA; quadrant and radial modes retain their
  kaleidoscopic behavior.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release x64 build; 2.42-second unchanged incremental build; all
  21 BrowserFlow scenarios; all 13 public-app contracts; operator-render
  contract; current parameter manifest.
- Follow-Up Actions: Visually verify the type-scale extremes and Mirror modes
  0/1 with the heaviest show scene, then finish dirty/save-result feedback and
  the recovery rehearsal.

## 2026-07-24 - runtime - scene_parameter_persistence_and_status

- Request ID: `show_readiness_operator_stability`
- Phase / Milestone: Pre-show operator stability
- Summary: Scene serialization now captures the bound live value for
  unmodulated float, bool, and string parameters while retaining the base value
  for modifier-owned parameters. The System Status HUD and Debug Terminal now
  report the same active scene and last scene-load outcome.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release x64 build; all 21 BrowserFlow scenarios; all 13
  public-app contracts; scene persistence and scene/display transaction gates;
  app-independence audit.
- Follow-Up Actions: Add authoritative unsaved-change and save-result state,
  then complete show-machine save/mutate/reload and recovery rehearsal.

## 2026-07-24 - runtime - show_day_render_path_cleanup

- Request ID: `show_day_render_path_cleanup`
- Phase / Milestone: Pre-show runtime performance
- Summary: Removed eight unused full-resolution layer-history render targets
  and their per-frame clears/copies, stopped clearing invisible slot buffers,
  and made post-effect scratch, mirror-history, and motion-history buffers
  allocate only when their effects are first used. Scene output and effect
  algorithms are unchanged.
- Request Doc: `docs/dev_playbook.md`
- Roadmap Entry: Operator-directed pre-show stabilization
- Validation: Clean Release x64 rebuild; all 20 BrowserFlow scenarios; Signal
  Bloom offscreen package bench; all 13 public-app contracts; strict extraction
  and app-independence gates; 2.46-second identical incremental build.
- Follow-Up Actions: Run the documented 60-second heaviest-show-scene visual
  and frame-time check on the show GPU before adding more visual load.

## 2026-07-24 - artist-sdk - labeled_parameter_selection

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Browser live parameter ownership
- Summary: Added one reusable labeled picker for live package parameters
  declared through static `options[]` or registered `optionsSource` metadata.
  Selection updates the existing registry value; provider revisions close stale
  pickers and unavailable current values remain unchanged until explicit
  replacement.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package/inspection gates, all 20
  BrowserFlow scenarios, Release x64 build, and 2.21-second identical
  incremental build.
- Follow-Up Actions: Add explicit package mapping-preset preview/apply/edit
  controls with conflict handling and rollback.

## 2026-07-24 - artist-sdk - package_preset_bank_selection

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Browser preset ownership
- Summary: Added labeled package preset-bank selection backed by stable IDs,
  persisted the choice in the ignored show-machine activation override, and
  applied it only to the next layer instantiation so active scene and mapping
  state remain authoritative.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package-only and combined gates, all
  19 BrowserFlow scenarios, Signal Bloom stub-backed lifecycle/draw-dispatch
  bench, Release x64 build,
  and 2.55-second identical incremental build.
- Follow-Up Actions: Promote named static/runtime option values into an
  explicit labeled dropdown before adding mapping-preset apply/edit controls.

## 2026-07-24 - artist-sdk - runtime_option_provider_resolution

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Browser option-provider ownership
- Summary: Added a revisioned runtime option-provider registry, registered the
  app-owned transport BPM choices, resolved them in read-only package
  inspection, and preserved/marked defaults that disappear from a provider.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package-only and combined
  catalog/manifest gates, all 18 BrowserFlow scenarios, Signal Bloom
  stub-backed lifecycle/draw-dispatch bench, Release x64 build, and 1.92-second
  identical incremental build.
- Follow-Up Actions: Add explicit package preset-bank selection with the
  locked value precedence before mapping-preset apply/edit controls.

## 2026-07-24 - artist-sdk - package_vertical_slice_convergence

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Package declaration convergence and dynamic-option fixture
- Summary: Added deterministic package-to-runtime-adapter generation, replaced
  the remaining Signal Bloom adapter duplication with a checkable output,
  added package-owned dynamic option metadata, encoded preservation of
  unavailable stored values, and rendered named choices plus unresolved
  provider state in read-only Browser inspection rows.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package-only and combined
  catalog/manifest gates, inspection schemas, all 18 BrowserFlow scenarios,
  Signal Bloom stub-backed lifecycle/draw-dispatch bench, Release x64 builds,
  2.14-second incremental
  rebuild, payload immutability assertion, and live-window Signal Bloom/Aurora
  Veil loading.
- Follow-Up Actions: Add an explicit runtime option-provider registry and
  resolve `transport.bpmMultipliers` while preserving unavailable stored
  values.

## 2026-07-19 - artist-sdk - package_vertical_slice

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Safe package vertical slice
- Summary: Added a focused package check command, manifest-only Browser
  inspection rows, disabled-by-default source-registered Signal Bloom
  activation, deterministic preset/override precedence, suggestion-only
  mappings, and a native stub-backed lifecycle/draw-dispatch bench.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `synaptome-layer check`; `LayerPackageBench.exe`; all 18
  `BrowserFlowTest` scenarios; package, catalog, manifest, schema, and
  public-app contract gates.
- Follow-Up Actions: Converge package/catalog/runtime declarations, then add
  one package-owned dynamic option source before further Browser promotion.

## 2026-07-19 - media - aurora_veil_public_media

- Request ID: `aurora_veil_public_media`
- Phase / Milestone: First reviewed public media asset
- Summary: Generated and reviewed one abstract aurora source, encoded it as a
  12-second 1920x1080 H.264 loop, recorded the prompt/encoding/license and
  SHA-256, added it to the manifest, and replaced the dangling default media
  layer reference without enabling folder scanning.
- Request Doc: `docs/project_ops/completed/aurora_veil_public_media.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python tools\media_catalog_regression.py --check`; `python
  tools\validate_configs.py synaptome\bin\data\config\videos.json`; `python
  tools\validate_configs.py --public-app`; FFmpeg stream inspection.
- Follow-Up Actions: Keep the catalog at one reviewed asset until a specific
  artistic requirement justifies another bounded intake request.

## 2026-07-18 - contracts - media_manifest_intake_contract

- Request ID: `media_manifest_intake_contract`
- Phase / Milestone: Safe media manifest intake complete
- Summary: Locked media discovery to explicit manifests, replaced the dangling
  `default-loop` reference with a valid empty baseline, separated public and
  operator-local roots, required stable IDs, revisions, SHA-256, provenance,
  generated-media metadata, redistribution permission, and replacement
  history, and added dependency-free semantic validation plus negative
  fixtures. The public-app report now covers 12 validated contracts.
- Request Doc: `docs/project_ops/completed/media_manifest_intake_contract.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python tools\media_catalog_regression.py --check`; package and
  combined catalog/manifest checks; Browser inspection payload check;
  `python tools\validate_configs.py --public-app`;
  `python tools\validate_synaptome_extraction_manifest.py --check --strict-review`.
- Follow-Up Actions: Open one bounded request for one reviewed redistributable
  asset, then add Browser visibility and runtime slot-load evidence without
  introducing folder scanning.

## 2026-07-18 - docs - roadmap_pre_media_gate

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Roadmap reconciliation and pre-media cleanup
- Summary: Made the Project Ops roadmap authoritative for priority, reconciled
  the active layer-package request with its task graph, labeled supporting and
  show-development roadmaps as active support, planned, parked, ready for
  closeout, or historical, and added a concrete safety gate for media policy,
  stable IDs, provenance, deterministic fixtures, and canonical contract
  preservation before more tracked media is generated. Updated the local
  request template to match the pinned Project Ops readiness contract.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Project Ops adapter and active-request audits; package,
  generated-layer, inspection-payload, combined catalog/manifest, canonical
  public-app contract, relative Markdown link, and `git diff --check`
  validation all passed.
- Follow-Up Actions: Finish one package-owned option-metadata slice, decide
  CG-08 media discovery/intake policy, record green validation evidence, then
  open one bounded media request.

## 2026-05-05 - contracts - osc_route_glob_regression
- Phase / Milestone: OSC contract hardening
- Summary: Added the OSC route glob validator to Synaptome's Project Ops validation ladder so built-in mesh-style OSC route coverage is part of the local administrative gate, not only the public contract report.
- Request Doc: `docs/project_ops/roadmap.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python tools\validate_osc_route_patterns.py`; `python tools\validate_configs.py --public-app`
- Follow-Up Actions: Keep route-pattern checks in sync with future mesh OSC contract revisions.

## 2026-05-05 - governance - schema_ownership_cleanup
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Replaced Synaptome's Project Ops adapter schema reference and local menu schema ID with repo-owned, versioned schema namespace IDs. Synaptome now consumes the Project Ops `v0.1.2` schema namespace while keeping Synaptome-owned schemas under the Synaptome `v0.1.0` namespace; Synaptome workflows and adapter metadata are pinned to Project Ops `v0.1.2`.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python -m json.tool .project_ops\config.json`; `python -m json.tool schemas\menu.schema.json`; `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `python tools\validate_configs.py --public-app`; repository-wide schema host scan passed with no raw GitHub or placeholder-local IDs; `git diff --check -- .project_ops/config.json schemas/menu.schema.json docs/project_ops/reports/changelog.md`.
- Follow-Up Actions: Keep future schema identity changes on repo-owned, versioned namespaces; do not reintroduce raw GitHub branch URLs or placeholder local schema hosts.

## 2026-05-05 - governance - project_ops_v0_1_1_pin
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Pinned Synaptome's Project Ops workflow checkouts, adapter schema URL, and adapter metadata to Project Ops `v0.1.1` instead of moving `main`.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `git diff --check`.
- Follow-Up Actions: Cut a new Project Ops tag before changing reusable audit/schema behavior consumed by Synaptome.

## 2026-05-05 - governance - project_ops_remote_request_audit
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Added a Synaptome Project Ops changed-request audit workflow that checks out `tensegrity-audio/project_ops` on GitHub Actions and runs `project_ops_request_audit.py` against changed `docs/project_ops/(in_progress|completed)/*.md` records. Added Project Ops request audit to contributor/local validation and pruned stale Tensegrity process-contract entries from Synaptome's full contract report so `validate_configs.py --contracts` is public-runtime-owned.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `python tools\validate_configs.py --contracts`; `python tools\validate_configs.py --public-app`; `python tools\validate_release_metadata.py`; `python tools\check_app_independence.py`; `python -m py_compile tools\validate_configs.py`.
- Follow-Up Actions: Push Synaptome so remote CI has both Project Ops adapter and request-audit coverage.

## 2026-05-05 - governance - project_ops_ci_audit
- Phase / Milestone: Project Ops CI adoption
- Summary: Updated Synaptome CI to check out `tensegrity-audio/project_ops` and run the reusable Project Ops adapter audit before public runtime validation.
- Request Doc: `docs/project_ops/roadmap.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; `python ..\project_ops\tools\project_ops_request_audit.py --repo ..\synaptome --request-id project_ops_compatibility`
- Follow-Up Actions: Use Project Ops request artifacts for future substantial runtime, contract, release, and docs work.

## 2026-05-04 - governance - project_ops_compatibility

- Phase / Milestone: Project Ops compatibility complete
- Summary: Added the namespaced `docs/project_ops/**` operating surface and updated `.project_ops/config.json` so Synaptome audits cleanly as a real Project Ops adopter without duplicating public runtime docs.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; `python ..\project_ops\tools\project_ops_request_audit.py --repo ..\synaptome --request-id project_ops_compatibility`; `python tools\validate_release_metadata.py`; `python tools\validate_configs.py --public-app`; `python tools\check_app_independence.py`
- Follow-Up Actions: Use Project Ops request artifacts for future substantial runtime, contract, release, and docs work.

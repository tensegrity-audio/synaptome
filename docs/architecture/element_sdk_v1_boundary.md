# Element SDK v1 Boundary

Status: Frozen architecture decision for SEAC-2, 2026-07-26.

Implementation status: SEAC-3 and SEAC-4 are complete. The public pointer-free
`Parameter.h` DTOs represent kinds, values, ranges, groups, ordered options,
option sources, quick-access order, aliases, and deprecation. All 23 shipping
types have construction-free declared parameter contracts covering 786
parameters, with exact live binding parity across 55 catalog assets. The public
minimum `ElementDescriptor` still
carries only stable type ID, the
closed `Visual`/`Effect` kind, and ordered pointer-free action descriptors. It
does not yet declare a display label, implementation/package version or owner,
capabilities/dependencies, parameters, resources, or persistence metadata.
Signal Bloom has a shipping static-library target, a separate stub compile-contract
target, public compatibility include paths, and a controlled built-in
registration entrypoint.
The first Runtime facade now owns compatibility element preparation/release,
distinct instance identity, prefix reservation, exact parameter registration
cleanup, namespace enforcement, and structured failure context.
`SynaptomeRuntimeCore` is now an independently linked static library and owns
the fixed composition records, element pointers, state/lifecycle/policy, and
generic resize/update/draw dispatch without owning GPU targets. Prepared
ownership cannot escape the facade,
adoption validates its source runtime and canonical `console.layerN` address,
and explicit Runtime shutdown releases elements while
`HostCompositionRenderer::releaseGraphicsResources()` releases host GPU targets
while the graphics context is live. Candidate setup now writes to an isolated
parameter registry; same-address adoption commits the registry, action table,
and element together,
while failed setup or commit leaves the live layer unchanged. Replacement
preparation now selects the current element by zero-based composition-layer
index inside Runtime; the host no longer obtains a mutable element pointer for
the generic replacement transaction. The caller-held prepared result retains the retired
action table and element after commit until host invalidation is complete.
`HostCompositionRenderer` privately owns per-slot and composite FBOs, performs
composition presentation, and reaches the concrete `PostEffectChain` only
through the narrow internal `HostCompositionEffects` interface. `ofApp`
delegates rendering and presentation, while persistence and mapping consumers
read copied snapshots. No raw mutable render target crosses Runtime. Element
type registration is now scoped per Runtime. Each registration atomically
contributes either a declared `ElementTypeContract` plus creator or a
legacy-setup-discovery record plus creator:
the legacy-named `LayerFactory` has no process-global singleton, the host and
each bench own an independent registry, scene validation uses non-constructing
type lookup, copied descriptor enumeration cannot mutate the registry, and
Control & Mapping receives only a narrow offline creator.
All host registration is now composed through the controlled
`BuiltinElements.cpp` aggregate instead of `ofApp.cpp`: 22 core creator
bindings live in that aggregate, while Signal Bloom is delegated to the narrow
package leaf registrar `registerSignalBloomElement()`, which is called by both
the aggregate and the package bench. This is controlled, handwritten source
registration, not generated registration or SEAC-8 completion.
Registration-only concrete includes and direct `TextLayerState` access have
left `ofApp`. Legacy Text parameter registration and font synchronization now
cross the host-only `BuiltinElementHostBindings` adapter. This isolates the
existing singleton from the application root; it does not replace that
singleton, make its parameters authoritative, or add an Element SDK service.
Runtime now owns the pure, zero-based effect coverage-window policy through
`CompositionCoverageWindow` and `Runtime::resolveEffectCoverage`; `drawConsole`
delegates to the host renderer, which consumes its half-open input range.
`PostEffectChain` remains the host-side concrete shader and parameter executor
behind `HostCompositionEffects`. This private host interface is not an Element
SDK effect contract, dynamic-loading surface, or ABI promise. Typed composition mutation now goes through
`CompositionAssignment`,
`CompositionMutationResult`, and explicit Runtime commands for element
adoption, effect/overlay assignment, active state, label, coverage, and clear.
Runtime also owns the canonical `console.layerN.opacity` registration and its
transactional replacement. The host observes composition through an immutable,
by-value snapshot, or one bounds-checked optional layer snapshot.
`CompositionLayerSnapshot` carries assignment identity, layer state, and
copied static action descriptors in their declared order; it
exposes no element, FBO, registry, creator, handler, or host pointers.
Runtime seeds a private handler table from that declaration, and the live
element binds handlers by action ID. Missing, undeclared, duplicate, or empty
bindings reject preparation before adoption.
`Runtime::invokeCompositionAction()` invokes one no-argument action by
zero-based composition-layer index and local action ID, with structured bounds,
kind, support, rejection, and execution failures. Read-only element inspection
has been replaced by the separate on-demand
`Runtime::compositionElementTelemetry()` query. Telemetry collection is not
part of the ordinary composition snapshot because it reads volatile live
values. Mutable FBO access is retired rather than exposed as a Runtime seam.
The host no longer caches derived Grid, Geodesic, Perlin, or Game of Life pointers:
ordinary HUD reads, MIDI/OSC binding, Grid density cycling, and Game of Life
pause resolve snapshot prefixes through `ParameterRegistry`. Geodesic
subdivisions are durable parameter state; video source label and capture
readiness are pointer-free live telemetry. The live aggregate, public
`CompositionLayer` accessors, and `compositionElementForHost` seam are gone.
Minimal type/kind/action descriptor authority is complete. Broader catalog,
parameter, package-owner/version, capability, and persistence metadata remain
outside this descriptor slice.

This document records the dependency inventory and the minimum Element SDK v1
contract that must exist before Synaptome moves code into new build targets.
It refines the canonical
[`synaptome_spine_element_model.md`](synaptome_spine_element_model.md) without
changing its public terminology.

The central decision is:

> Element SDK v1 is a compiler-matched, source/static-link extension boundary,
> not a stable native plug-in ABI.

It may use a deliberately small set of openFrameworks rendering types. It must
not expose the host application, operator UI, persistence implementation,
device adapters, or runtime-owned registries. Generated static registration is
the dependable installation path. Dynamic native modules remain a later,
evidence-based decision.

## Current-State Inventory

The current code has a useful element interface and isolated validation
scaffolding, but the physical dependency direction does not yet match the
architecture.

| Area | Current evidence | Boundary problem |
| --- | --- | --- |
| Application root | `synaptome/src/ofApp.cpp` still combines platform callbacks, mappings, scenes, devices, and UI wiring, but delegates element registration and composition rendering. | The host remains a broad service container even though creator and render-target ownership moved out. |
| Compile graph | `synaptome/Synaptome.vcxproj:497` compiles the host, runtime facilities, UI, and every visual implementation into one executable. | A small element change rebuilds and relinks the application. |
| Element API | `synaptome/src/visuals/Layer.h:3` includes `ofMain.h`, `ofJson`, and the concrete `ParameterRegistry`; its draw context exposes mutable `ofCamera&`. | The public seam is broad and tied to runtime internals. |
| Concrete dependencies | Registration-only concrete headers and creator lambdas now live in the handwritten `BuiltinElements.cpp` aggregate. Signal Bloom's package leaf registrar is shared by that aggregate and its bench. Legacy Text parameter/state access lives behind host-only `BuiltinElementHostBindings`. | Adding a built-in no longer adds a creator lambda or concrete state dependency to `ofApp`, but still edits the aggregate/project until SEAC-8 generation exists; the shared Text singleton remains internal compatibility debt. |
| Type special cases | Mutable element-specific action/status casts have been replaced by registered parameters, statically declared actions with live handler binding, and typed telemetry. GPU-target ownership and presentation are isolated in `HostCompositionRenderer`; the raw Runtime target adapter is retired. | SEAC-3 control/query/render ownership and SEAC-4 declaration/binding authority are closed; versioned state ownership is the SEAC-5 gap. |
| Lifecycle | `synaptome/src/visuals/Layer.h:23` has configure, setup, update, draw, resize, and enable methods, then relies on destruction. Runtime replacement is transactional. | There is still no typed setup result, explicit element teardown hook, or normalized health contract. |
| Composition | Runtime owns the fixed assignment/state/lifecycle records and coverage policy. `HostCompositionRenderer` owns per-slot/composite FBOs and presentation behind `HostCompositionEffects`; `ofApp::drawConsole` delegates. | The state and graphics owners are now explicit; real pixel/GL evidence remains a later confidence-suite gate. |
| Registry | At the SEAC-2 freeze, `LayerFactory` was process-global. It is now a Runtime-scoped registry with atomic declared records, validation, non-constructing lookup, and copied enumeration. Every shipping record carries `ElementTypeContract`; compatibility binding mode is explicit. It still has no package owner/version, unregister, or atomic package registration. | Type/action/parameter identity is isolated and inspectable; broader package ownership remains SEAC-7/SEAC-8 work. |
| Catalog | `LayerLibrary` owns legacy JSON loading, package activation, and preset merging; package tools generate a second legacy catalog representation. | The package manifest is not yet the runtime's typed source of truth. |
| Parameters | Public pointer-free DTOs and authoritative declarations now cover all 23 shipping types and 786 controls. Runtime publishes declaration metadata/defaults and enforces exact live ID/kind/storage parity. Signal Bloom uses explicit bind-only storage; 22 existing implementations use a checked setup-storage adapter whose metadata is discarded. | Direct bind-only migration remains cleanup. Versioned value ownership, provenance, and migrations belong to SEAC-5. |
| Services | Elements reach global services such as audio analysis, video catalog, and text state directly. | Isolated tests can still depend on hidden process state. |
| Inspection | Type/kind/action/group/parameter inspection is construction-free for every registration. Browser counts/groups, the machine catalog, compatibility manifest, compiled declarations, and human reference come from the same reviewed snapshot. | Dynamic option providers remain runtime services, and package ownership/version inspection belongs to later phases. |
| Tests | The package bench proves stub-backed element lifecycle/draw dispatch. Browser tests still use private app internals. `HostCompositionRendererTest` separately compiles the production renderer/Runtime sources against FBO/GL stubs. BrowserFlow passes 36 scenarios, and the operator reports successful Release and dual-screen use. | The renderer harness proves traversal/policy/failure behavior, not pixels, shader execution, or a real GL context; physical MIDI and a complete show-machine recovery rehearsal remain deferred. |

The inventory also identifies two state risks that later tasks must preserve:

- definition identity is currently reused as instance identity, while
  `console.layerN.*` is the effective runtime address;
- console persistence currently mixes portable composition concerns with
  monitor, focus, and other machine-local state.

## Frozen Dependency Direction

The target compile-time direction is:

```text
Element_<package> ----------------------+
  depends on ElementSDK                 |
                                        v
SynaptomeBuiltinElements ----------> RuntimeCore <---------- SynaptomeHost
  knows selected concrete types         ^                    owns platform/UI
                                        |
Element_<package>Bench -----------------+
  uses the real SDK test host
```

The allowed dependencies are:

### `SynaptomeElementSDK`

Public headers only. It may depend on:

- the C++ standard library surface selected for the supported compiler,
- narrow math/rendering types required by visual elements,
- the pinned openFrameworks source toolchain for v1.

It must not depend on:

- `ofApp`,
- Browser, Console, HUD, or other operator UI,
- MIDI, OSC, serial, audio-device, sensor, or window adapters,
- `LayerFactory`, `LayerLibrary`, filesystem discovery, or package activation,
- scene stores, machine profiles, or runtime persistence implementations,
- process-global Synaptome singletons.

### `SynaptomeRuntimeCore`

Depends on the Element SDK and owns:

- element registration, descriptors, compatibility checks, and catalog/package
  resolution;
- element lifecycle, instance ownership, and failure-safe replacement;
- the eight ordered composition records and composition/effect routing policy,
  including coverage-window resolution;
- parameter values, modifiers, options, banks, transport, telemetry, resource
  service implementations, and control target exposure;
- mapping resolution and provenance;
- presets, scenes, mapping banks, migrations, recovery, and transactional
  persistence;
- runtime commands, immutable query snapshots, health, diagnostics, and
  performance accounting.

Runtime core must not depend on concrete elements, `ofApp`, operator UI,
`HostCompositionRenderer`, or `HostCompositionEffects`. Concrete built-in
effect selection, default values, coverage-mask parameters, and shader
execution remain in the host's `PostEffectChain` adapter.

The current control-plane extraction exposes `CompositionKind`,
`CompositionAssignment`, `CompositionMutationError`, and
`CompositionMutationResult` inside RuntimeCore. These types describe the fixed
built-in composition kinds and mutation outcomes; they do not define an Element
SDK interface or a dynamically loadable effect contract.

The immutable query plane consists of `CompositionLayerSnapshot` and
`CompositionSnapshot`. A layer snapshot copies its zero-based index, occupancy,
element-presence flag, composition kind, definition/label/type/prefix identity,
active state, opacity, coverage, and copied pointer-free live action
descriptors. `Runtime::compositionSnapshot()` returns the fixed eight-layer aggregate by value;
`Runtime::compositionLayerSnapshot()` returns a bounds-checked optional copy.
Neither DTO carries executable or render-resource ownership.

Volatile element telemetry uses a separate on-demand query rather than adding
collection work to either composition snapshot. The query returns a
pointer-free `CompositionTelemetryResult`; it does not expose the live element
or turn telemetry into composition state.

### `SynaptomeHost`

The executable owns:

- openFrameworks process bootstrap and callbacks;
- physical windows and graphics contexts;
- monitor placement, focus, fullscreen, and OS quit behavior;
- platform event translation;
- construction of concrete filesystem, MIDI, OSC, serial, audio, and sensor
  adapters;
- `HostCompositionRenderer`, its per-slot/composite GPU targets, render
  traversal, latest-frame presentation, and preview drawing;
- concrete built-in effect execution through `PostEffectChain` behind the
  private `HostCompositionEffects` interface;
- Browser, Console, HUD, and other operator presentation/controllers.

The host calls a runtime facade and consumes runtime query models. It does not
own element creator bindings, registration-only concrete headers, or direct
built-in state references. `BuiltinElementHostBindings` privately owns the
legacy Text parameter and font-synchronization bridge and is excluded from
RuntimeCore and the public Element SDK.

### Element Targets

Each element package or cohesive implementation family builds as its own static
library. It depends only on:

- Element SDK,
- the pinned openFrameworks toolchain,
- dependencies and add-ons declared by that package.

Each element bench links the real element library and a reusable SDK test host.
It must not include implementation `.cpp` files, redefine private access, or
include the host source tree.

### Controlled Built-In Registration

The current SEAC-3 bridge is a handwritten `BuiltinElements.cpp` aggregate. It
is the only host composition-time unit that includes the selected concrete
element headers and creator lambdas. Signal Bloom supplies a narrower
`SignalBloomRegistration.cpp` package leaf; the aggregate and package bench
invoke that same leaf registrar, so the reference package has one creator
binding even though their top-level entrypoints differ.

This consolidation removes registration ownership from `ofApp`, but does not
yet make registration generated. SEAC-8 remains responsible for deriving the
aggregate records from validated descriptor/package metadata so a new element
does not require a handwritten aggregate or project edit.

## Element SDK v1 Contract

Names may receive normal C++ spelling adjustments during extraction, but the
following responsibilities and ownership rules are frozen.

### Identity

The SDK distinguishes:

- package ID and version;
- element type ID;
- element definition ID;
- element instance ID;
- composition layer ID/index;
- element-local parameter suffix;
- expanded runtime parameter address.

Definition ID must not double as instance ID. `console.layerN.*` remains the
compatibility runtime-address form. Folder paths, labels, timestamps, and local
absolute paths are never identity.

v1 must represent multiple instances correctly even if the current UI retains
temporary duplicate-assignment restrictions.

### Static Descriptor

An element type now has a minimal pure-data descriptor available without
construction or `setup()`. The implemented SEAC-4A record contains exactly:

- stable `typeId`;
- closed `ElementKind` (`Visual` or `Effect`);
- ordered pointer-free `ActionDescriptor` declarations.

`LayerFactory` validates the static action IDs, labels, group IDs, and
duplicates when it accepts the descriptor and creator. Lookup and enumeration
do not invoke the creator, and enumeration returns copies.

The broader Element Package target still needs display label,
implementation/package version and owner, compatible SDK/runtime range,
capabilities and dependencies, parameter declarations, deterministic/reset
support, render/resource requirements, lifecycle/test requirements, and
persistence metadata. Those fields are deliberately not present in the
SEAC-4A `ElementDescriptor`. Parameter and catalog authority belong to
SEAC-4B, package serialization to SEAC-7, and generated registration to
SEAC-8.

SEAC-4B1 adds a separate public `ElementTypeContract` companion rather than
expanding the minimal `ElementDescriptor`. It combines that descriptor with a
pointer-free `ParameterDeclarationSet`. This is a source/static-link
declaration surface, not package serialization.

Element kinds in v1 are `visual` and `effect`. Operator overlays remain
spine-owned UI modules, not creative elements. Existing effects may use a
runtime adapter until a focused effect interface is extracted, but they must
share the same identity, registration, diagnostics, and scene-validation path.

### Registration

One non-global runtime registry currently accepts either atomic registration
record:

```text
declared ElementTypeContract + creator
legacy minimal descriptor + explicit setup-discovery state + creator
```

Package/version ownership remains part of the later package registration
record, not the implemented SEAC-4B1 record.

Rules:

- every Runtime receives one explicitly owned registry; there is no
  process-global fallback;
- empty and duplicate IDs fail before activation;
- descriptor and creator registration cannot partially succeed;
- declaration state and type contracts can be looked up without construction
  and enumerated as independent copies;
- host and bench use the same controlled registration entrypoint;
- unregister/lifetime behavior is explicit even while built-ins remain
  process-lifetime static registrations;
- package manifests, runtime registration, tests, and docs do not each maintain
  independent handwritten type metadata.

`LayerFactory` and `LayerLibrary` therefore belong in runtime core, not the
public SDK.

### Lifecycle

The semantic lifecycle is:

```text
inspect descriptor
  -> create
  -> configure identity and definition data
  -> setup against scoped services
  -> register live actions
  -> activate
  -> update / render / resize
  -> deactivate
  -> shutdown
  -> destroy
```

Rules:

- descriptor inspection never constructs an element;
- setup reports success or structured failure and does not throw across the
  host boundary;
- the compatibility `setup(ParameterRegistry&)` receives a private staging
  registry, may only register IDs in the instance namespace, and must not
  retain the registry address;
- `{instancePrefix}.opacity` is reserved for the layer container and is
  rejected even though it is inside the instance namespace; element-local
  alpha controls use a behavior-specific suffix;
- an element that needs later registry lookup overrides
  `onParameterRegistryCommitted`; that hook is a trivial, no-throw pointer
  rebind and performs no allocation or device work;
- the old live instance remains active until a replacement has configured, set
  up, and registered a valid action table successfully;
- replacement preparation identifies the live element by zero-based
  composition-layer index; out-of-range, empty, effect, and overlay layers fail
  validation before the element factory is called;
- adoption rejects host-owned ID collisions before mutation, swaps the staged
  registry, action table, and element as one commit, preserves modifiers on
  matching stable parameter IDs, and leaves candidate defaults/base values
  authoritative;
- after adoption, the host synchronously invalidates pointer-bearing parameter
  views and retired element routes while the caller-held prepared result keeps
  the retired action table and element alive together;
- activation and deactivation are explicit;
- shutdown is explicit, idempotent, and releases registrations/resources owned
  by the instance;
- destruction is a final safety net, not the primary unload protocol;
- health and diagnostic messages identify package, type, definition, instance,
  and layer;
- all current `Layer` implementations remain supported through a compatibility
  adapter while the lifecycle is introduced.

### Scoped Services

Setup receives a narrow service context. v1 service interfaces cover:

- instance-scoped parameter binding;
- resource and package-relative asset resolution;
- structured logging and diagnostics;
- clock and transport snapshots;
- read-only shared audio-analysis data when declared;
- read-only media/catalog access when declared;
- deterministic seed/reset support;
- host-approved asynchronous work where required.

Elements do not receive `ofApp`, Browser/Console objects, persistence stores,
raw mapping routers, monitor/window control, or direct device ownership merely
for convenience. A capability must be declared before its optional service is
provided.

### Update And Transport

The update context replaces unrelated scalar arguments with one runtime
snapshot containing:

- bounded delta time and monotonic runtime time;
- transport running state;
- BPM, beat/bar phase, and source/confidence when available;
- speed/time-scale;
- discontinuity/reset indication.

The context is read-only. Elements do not select clock sources or mutate the
transport.

### Rendering And Graphics State

Visual rendering occurs on the host-approved render thread into a host-owned
target.

The render context provides:

- viewport and pixel density;
- time/transport snapshot;
- layer opacity as composition context;
- read-only camera/view information when applicable;
- approved resource access.

Rules:

- elements do not create competing app windows or switch the active window;
- elements do not retain host-owned FBOs, cameras, or transient context
  references;
- the spine establishes and restores framebuffer, viewport, matrices, style,
  blend, depth, scissor, shader, and texture-unit baselines around a render;
- the confidence suite detects graphics-state leakage;
- whole-layer active state, opacity, coverage, and ordering remain layer
  container responsibilities.

Element-local visibility controls must name the internal object or behavior they
affect. They do not redefine the layer's generic active/opacity controls.

### Parameters

SDK v1 separates declaration from storage binding.

SEAC-4 implements the pointer-free declaration DTO and authoritative
declaration path for every built-in. Signal Bloom demonstrates the explicit
bind-only form and registers:

- groups `example`, `exampleMotion`, `exampleTransform`, `exampleColor`, and
  `exampleModulation`, with stable IDs separate from display labels;
- 18 parameters in reviewed package order, including the explicit legacy
  `visible` compatibility declaration;
- current defaults, ranges/steps, units, and package descriptions;
- three ordered continuous `scale` options;
- the `transport.bpmMultipliers` option source with explicit value and label
  fields;
- no quick-access entries;
- the exact `alpha` to layer-container `opacity` deprecation.

`LayerFactory` validates declared pure data atomically before storing the
creator and records either explicit or setup-storage-adapter binding.
Signal Bloom's declaration is checked exactly against the package. The
catalog-wide reviewed snapshot generates declarations for the other built-ins,
and the live gate verifies exact ID/kind/storage parity for all catalog assets.
Runtime metadata and defaults come only from declarations.

One `ParameterDeclaration` surface carries:

- stable suffix ID and value kind;
- stable group ID and display label;
- default, range, step, and units;
- description;
- static options or an option-source ID;
- quick-access metadata;
- deprecation, alias, and replacement metadata.

The element binds implementation storage by suffix during scoped setup. The
runtime expands addresses, owns current/base/modifier/provenance state, and
derives Browser, preset, mapping, manifest, and documentation views.

Rules:

- IDs are unique across all value kinds;
- exact instance-owned IDs are removed through a registration token, not a
  broad unverified prefix deletion;
- declaration parity checks compare identity, kind, default, range, step,
  group, units, options, quick access, and deprecation;
- `active` and `opacity` are reserved layer-container concepts; the current
  compatibility runtime enforces the concrete `{instancePrefix}.opacity`
  reservation at preparation time;
- MIDI and OSC adapters target generic registered parameter IDs and actions;
  they never require concrete element casts.

SEAC-4 completed this representation and generation path. State provenance,
versioned persistence, and migrations are SEAC-5 work.

### Live Action Compatibility Contract

SEAC-4A separates static action metadata from live handler binding:

- `ActionDescriptor` contains only local `id`, `label`, `groupId`, and
  `description` strings;
- `ElementDescriptor::actions` is the authoritative ordered declaration for
  action metadata;
- `Layer::registerActions(ActionRegistrar&)` is an optional live-instance hook
  that binds an action ID to its handler but cannot redefine metadata;
- every handler is a no-argument command returning `Succeeded`, `Rejected`, or
  `Failed` with optional diagnostic text;
- action IDs are unique within one live element and use lower-camel dotted
  segments such as `subdivision.increment`;
- `label` is required display text; `groupId` is a required stable,
  single-segment lower-camel alphanumeric ID such as `geometry` or
  `simulation`, not a display label;
- empty IDs, duplicate IDs, invalid IDs, empty labels, and invalid group IDs
  reject factory registration before activation;
- missing declared bindings plus empty, duplicate, or undeclared live bindings
  reject candidate preparation before adoption;
- Runtime owns handler storage and copies the static descriptors, never
  handlers, into composition snapshots in canonical declaration order;
- `Runtime::invokeCompositionAction(zeroBasedIndex, actionId)` rejects
  out-of-range, empty, effect, overlay, and unsupported requests without a
  concrete element cast, and converts handler rejection, failure, or exception
  into `CompositionActionResult`;
- action handlers are destroyed before the element they may capture on every
  candidate and live retirement path, including failed or aborted preparation,
  replacement, clear, shutdown, Runtime expiry, and implicit Runtime
  destruction;
- adopted actions remain discoverable and invokable when their composition
  layer is inactive; inactive gates update/draw behavior, not commands;
- `Succeeded` means the command completed, `Rejected` means the element
  declined it in the current state, and `Failed` or a thrown exception means
  execution failed after it may have begun. Runtime does not retry or roll back
  handler-side mutation;
- invocation is synchronous on Runtime's owner thread and non-reentrant;
  external MIDI, OSC, device, or worker threads must queue invocation to that
  thread;
- a handler may mutate only its element-owned state. It must not re-enter
  Runtime composition mutation, clear or replace its slot, shut down or destroy
  Runtime, or invoke another composition action before returning.

These actions are commands, not state. They have no expanded
`console.layerN.actions.*` address in this checkpoint, do not enter
`ParameterRegistry`, and are not written into scenes, presets, mapping
snapshots, package manifests, or parameter manifests. MIDI/OSC action targets,
trigger/edge semantics, package serialization, and persisted action mappings
remain later gated work.

### Live Telemetry Compatibility Contract

The first element telemetry contract is a separate, on-demand Runtime query for
volatile observations:

- the public `Telemetry.h` defines
  `TelemetryValue = std::variant<bool, std::int64_t, double, std::string>`;
- `TelemetryEntry` contains only local `id`, display `label`, stable `groupId`,
  `description`, and one copied `TelemetryValue`;
- `TelemetrySink::add(TelemetryEntry)` receives entries from the optional const
  `Layer::collectTelemetry(TelemetrySink&) const` hook;
- telemetry IDs use unique lower-camel dotted segments, labels are nonempty,
  and group IDs use the same required single-segment lower-camel alphanumeric
  rule as live actions;
- `Runtime::compositionElementTelemetry(zeroBasedIndex)` returns
  `CompositionTelemetryResult`, whose structured errors distinguish
  `IndexOutOfRange`, `SlotEmpty`, `KindMismatch`, `ContractViolation`, and
  `CollectionFailure`;
- collection is synchronous on Runtime's owner thread, const, non-reentrant,
  bounded, and side-effect-free. It copies cached observations only and does
  not perform device I/O, graphics work, logging, composition mutation, or
  another action or telemetry query;
- Runtime contains collection exceptions and publishes no partial entries after
  a contract or collection failure;
- an adopted element remains queryable while its layer is inactive. Inactive
  gates update and draw behavior, not read-only telemetry;
- entries and results carry no callbacks, handlers, element pointers, device
  pointers, graphics resources, registries, or host/UI objects.

This is element-local runtime telemetry, not durable state and not normalized
health. Health is the lifecycle-level ready/degraded/failure assessment with
structured diagnostics; this telemetry slice only reports typed observations.
It is also distinct from `HudFeedRegistry` JSON payloads and HUD widget
`telemetry` feed names. The host may adapt a copied result into `hud.status` or
`hud.sensors`, but an element never publishes a HUD feed or names a widget.

Geodesic `subdivisions` is registered parameter state because actions change it
and scenes or presets may need to reproduce it. It is not telemetry.
`media.sourceLabel` is a string telemetry entry for webcam and video-clip
elements; webcam additionally reports the boolean
`media.captureInitialized`. Gain, mirror, loop, selected source, and other
declared controls remain parameter-authoritative and are not duplicated as
telemetry.

Telemetry has no expanded parameter address, is not a mapping target, and is
never written into scenes, presets, mapping snapshots, package manifests,
package capabilities, catalogs, parameter manifests, or ordinary composition
snapshots in this checkpoint. A static package capability declares a
requirement or service contract, such as camera access; a live telemetry value
reports the current observation and never satisfies or replaces that
capability.

### Packages And Offline Inspection

`layer.package.json` evolves into the canonical typed package declaration.
Legacy layer catalog JSON may remain as a generated compatibility artifact
until runtime core consumes the package type directly.

Offline inspection reads declarations only. It does not instantiate an element,
allocate graphics resources, open devices, or call setup.

Package mapping declarations remain suggestions with full transform and
provenance data. Nothing auto-applies a mapping, preset, or scene mutation.

## Runtime Facade

The host/runtime seam is a `Runtime` facade with the semantic surface:

```text
setup
update
resize
shutdown
submit typed command
read immutable snapshot
resolve composition policy
dispatch one element draw
```

Runtime owns composition state and generic element lifecycle/dispatch, not the
GPU composition buffers. `ofApp` delegates composition traversal and
presentation to `HostCompositionRenderer`, which requests copied state,
coverage policy, resize, and one element draw at a time. Neither the host nor
renderer receives a raw Runtime-owned output texture or target.

UI calls runtime commands and reads snapshots. It does not receive a raw
`ofApp*` as its service API.

`Runtime::resolveEffectCoverage` is an in-repository RuntimeCore composition
policy API. It is not part of the public Element SDK, does not define a public
effect interface, and does not change the compiler-matched source/static-link
compatibility decision.

Host metadata reads now use `Runtime::compositionSnapshot()` or
`Runtime::compositionLayerSnapshot()`. Writes use Runtime's transactional
adoption, assignment, active, label, coverage, and clear commands. No live
composition aggregate or public `CompositionLayer` accessor crosses the
boundary. Runtime owns no FBO and exposes no raw mutable target accessor.
`HostCompositionRenderer` privately owns per-slot/composite GPU targets,
consumes Runtime snapshots and coverage policy, calls Runtime's generic element
draw dispatch, and presents the latest frame or preview. Its only effect
dependency is `HostCompositionEffects`, implemented by `PostEffectChain`.
Mutable Geodesic subdivision and Game of Life
randomization register live actions and are invoked through
`Runtime::invokeCompositionAction()`, replacing the retired
`legacyCompositionElementForHost` seam. Their static descriptors declare the
canonical action metadata and order; their live hooks bind handlers by ID.
Those declarations are copied into the immutable layer snapshot for discovery.

The shipping SEAC-4A declaration set is:

| Type set | Ordered action declaration |
| --- | --- |
| 21 shipping visual types, including Signal Bloom | Empty |
| Geodesic | `subdivision.increment` / `Increase Subdivision` / `geometry` / `Increase geodesic subdivision by one, up to the current maximum.`; then `subdivision.decrement` / `Decrease Subdivision` / `geometry` / `Decrease geodesic subdivision by one, down to the current minimum.` |
| Game of Life | `simulation.randomize` / `Randomize Simulation` / `simulation` / `Immediately randomize the board using the current density.` |

Generic replacement continues to
use `Runtime::prepareCompositionElementReplacement()` and the two-phase
prepare/adopt transaction. There is no derived element-pointer cache or refresh
path. `firstConsoleElementOfType()` selects the first matching copied slot in
composition order; its registry prefix drives live parameter reads and writes,
HUD projection, and MIDI/OSC binding. Perlin and Game of Life MIDI ranges come
from registered descriptors while their established snap/step behavior remains
unchanged. Geodesic subdivisions resolve through the registered parameter;
media source labels and webcam capture readiness use the separate typed
telemetry query. `compositionElementForHost` and all host concrete status casts
are removed. Direct Text state access has also left `ofApp` through
`BuiltinElementHostBindings`; the shared singleton and its pre-adoption
configuration side effects remain SEAC-5 state-ownership debt. The
retired `CompositionRenderTargets`/`compositionRenderTargetsForHost` seam does
not remain as compatibility debt. SEAC-4A static type/kind/action declarations
and live binding parity are complete. SEAC-4 adds construction-free parameter
declarations and generated views for all 23 built-in types.

## Completed SEAC-3 Extraction Sequence

SEAC-3 preserved output and proceeded in this order:

1. Add shared MSBuild property sheets, public SDK include roots, and
   compatibility forwarding headers.
2. Add an SDK compile-contract target that builds Signal Bloom using only the
   allowed public include paths.
3. Introduce the runtime facade and move composition-layer ownership behind it
   without changing scene or render behavior.
4. Create `SynaptomeRuntimeCore`; move the type registry, catalog/package
   parsing, and parameter runtime ownership behind the new target.
5. Build Signal Bloom as the first real `Element_<package>` static library and
   link its existing bench to that library.
6. Consolidate current creator bindings in `SynaptomeBuiltinElements`; give
   Signal Bloom one package leaf registrar shared by the host aggregate and
   package bench; remove its registration-only concrete dependency from
   `ofApp`. Deterministic generation of the aggregate remains SEAC-8.
7. Replace element-specific host casts and MIDI helpers with parameter/action
   contracts.
8. Extract `HostCompositionRenderer`, move per-slot/composite GPU targets and
   presentation out of Runtime/`ofApp`, retire the raw target seam, and prove
   renderer policy/failure behavior in a dedicated stub-backed target.

`BuiltinElementHostBindings` completes direct application-root isolation for
the legacy Text bridge. The shared singleton and pre-adoption configuration
effects remain SEAC-5 debt. Broader service injection and cohesive
family targets remain later migration work; generated registration remains
SEAC-8.

The solution must stop hard-coding a machine-specific openFrameworks project
path. The authoring and package bench projects must become first-class solution
targets or share an equivalent reproducible command.

## Promotion Gates

No-output-change extraction is accepted only when:

- an SDK header compile check rejects UI, host, adapter, catalog I/O, and
  concrete-element dependencies;
- Signal Bloom builds through its element library, not by including its `.cpp`;
- host aggregate and package bench use the same package leaf registrar;
- runtime core has no concrete element or operator UI dependencies;
- `ofApp` has no registration-only concrete includes, casts, or direct
  built-in-state dependencies;
- built-in compatibility bindings are isolated behind a host-owned adapter
  excluded from RuntimeCore and the public Element SDK;
- invalid static descriptors reject atomically without replacing a prior
  registration;
- descriptor lookup and copied enumeration do not instantiate an element;
- declared and legacy parameter states are explicit, atomically stored, and
  construction-free to inspect;
- Signal Bloom's five groups and 18 ordered declarations match the reviewed
  package and current live setup registration;
- missing descriptors and non-visual descriptors reject at Runtime's
  descriptor stage before prefix reservation or construction;
- action snapshots preserve static declaration order and metadata while
  exposing no handlers or element pointers;
- missing, undeclared, duplicate, and empty live action bindings reject
  preparation; replacement remains atomic and action cleanup precedes element
  destruction;
- action invocation returns structured errors without concrete RuntimeCore
  element dependencies;
- on-demand telemetry returns typed pointer-free copies, contains collection
  and contract failures, and does not change or burden ordinary composition
  snapshots;
- no host read-only element pointer or concrete status cast remains;
- failed replacement preserves the prior live layer;
- setup/update/stub-backed draw dispatch/shutdown/reload pass through the real
  SDK seam;
- renderer traversal, effect coverage/order, fail-open behavior, target
  reuse/release, and allocation-status handling pass through the production
  renderer against controlled stubs;
- Release, public-app, BrowserFlow Release build, scene round-trip, and
  existing authoring profiles remain green.

`HostCompositionRendererTest` is policy and draw-dispatch evidence. It does not
prove pixels, shader execution, graphics-state containment in a real context,
or live projection. BrowserFlow execution and live dual-screen hardware
rehearsal were not run in this checkpoint and remain explicitly deferred.

## Explicitly Deferred

This decision does not promise or implement:

- loading an arbitrary raw openFrameworks `ofApp` folder;
- a stable binary ABI;
- native hot loading or unloading;
- automatic package discovery or activation;
- the complete Scene v2/state-provenance model;
- direct bind-only conversion of the 22 compatibility-adapted built-ins;
- scene, preset, or mapping migration to the declared parameter contract;
- display label, implementation/package version or owner, capabilities,
  dependencies, resources, and persistence fields in `ElementDescriptor`;
- serialization of the runtime descriptor into packages;
- generated registration;
- persisted MIDI/OSC action targets and action mappings;
- physical MIDI validation and a complete show-machine recovery rehearsal;
- real pixel, shader, and graphics-context confidence tests.

Those remain separate gated tasks. Parameter declaration and Runtime metadata
authority are complete; see
[`../contracts/builtin_element_parameter_contract.md`](../contracts/builtin_element_parameter_contract.md).
A raw openFrameworks experiment must first
be wrapped behind the lifecycle; it gains scenes, mappings, presets, and
Browser control only as it declares the corresponding stable contract.

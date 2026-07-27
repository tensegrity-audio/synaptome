# Element SDK v1 Boundary

Status: Frozen architecture decision for SEAC-2, 2026-07-26.

Implementation status: SEAC-3 in progress. Signal Bloom now has a shipping
static-library target, a separate stub compile-contract target, public
compatibility include paths, and a controlled built-in registration entrypoint.
The first Runtime facade now owns compatibility element preparation/release,
distinct instance identity, prefix reservation, exact parameter registration
cleanup, namespace enforcement, and structured failure context.
`SynaptomeRuntimeCore` is now an independently linked static library and owns
the fixed composition records, element pointers, per-layer FBOs, and generic
resize/update/draw dispatch. Prepared ownership cannot escape the facade,
adoption validates its source runtime and canonical `console.layerN` address,
and explicit shutdown releases elements and FBO resources while the graphics
context is live. Candidate setup now writes to an isolated parameter registry;
same-address adoption commits the registry and element together, while failed
setup or commit leaves the live layer unchanged. The host still adapts effect
compositing, persistence, mappings, and compatibility inspection through a
temporary internal view. Element type registration is now scoped per Runtime:
the legacy-named `LayerFactory` has no process-global singleton, the host and
each bench own an independent registry, scene validation uses non-constructing
type lookup, and Control & Mapping receives only a narrow offline creator.
Runtime now owns the pure, zero-based effect coverage-window policy through
`CompositionCoverageWindow` and `Runtime::resolveEffectCoverage`; `drawConsole`
consumes its half-open input range. `PostEffectChain` remains the host-side
concrete shader and parameter executor. This extraction adds no Element SDK
type, effect interface, dynamic-loading surface, or ABI promise. Typed
descriptor/catalog ownership is not yet complete.

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
| Application root | `synaptome/src/ofApp.cpp` combines platform callbacks, element registration, composition, mappings, scenes, devices, and UI wiring. | The host is also the runtime and service container. |
| Compile graph | `synaptome/Synaptome.vcxproj:497` compiles the host, runtime facilities, UI, and every visual implementation into one executable. | A small element change rebuilds and relinks the application. |
| Element API | `synaptome/src/visuals/Layer.h:3` includes `ofMain.h`, `ofJson`, and the concrete `ParameterRegistry`; its draw context exposes mutable `ofCamera&`. | The public seam is broad and tied to runtime internals. |
| Concrete dependencies | `synaptome/src/ofApp.h:23` begins a run of concrete element includes; registrations are hand-written at `synaptome/src/ofApp.cpp:1995`. | Adding an element edits the host composition root. |
| Type special cases | Element casts and element-specific bindings occur around `synaptome/src/ofApp.cpp:5254` and `synaptome/src/ofApp.cpp:5866`. | The generic factory contract is routinely bypassed. |
| Lifecycle | `synaptome/src/visuals/Layer.h:23` has configure, setup, update, draw, resize, and enable methods, then relies on destruction. | There is no setup result, explicit teardown, health, or transactional replacement. |
| Composition | `ConsoleSlot` at `synaptome/src/ofApp.h:326` owns assignment state, element ownership, coverage, and render targets; `drawConsole` starts at `synaptome/src/ofApp.cpp:4219`. | The reusable composition engine is embedded in the host. |
| Registry | At the SEAC-2 freeze, `LayerFactory` was process-global. It is now a Runtime-scoped type-to-creator map, but still has no descriptor, owner, version, enumeration, unregister, or atomic package registration. | Type creation is isolated; identity and package metadata still lack one typed authority. |
| Catalog | `LayerLibrary` owns legacy JSON loading, package activation, and preset merging; package tools generate a second legacy catalog representation. | The package manifest is not yet the runtime's typed source of truth. |
| Parameters | Package JSON, C++ `setup()`, and generated adapters repeat parameter metadata; `ParameterRegistry::Descriptor` at `synaptome/src/core/ParameterRegistry.h:24` is incomplete for the target contract. | Runtime binding and declaration metadata can drift. |
| Services | Elements reach global services such as audio analysis, video catalog, and text state directly. | Isolated tests can still depend on hidden process state. |
| Inspection | Browser/control inspection can instantiate an element and call `setup()` to discover parameters. | Inspection may allocate resources or touch devices. |
| Tests | The package bench and Browser tests include implementation `.cpp` files and private internals directly. | They test behavior, but not the promised include and linkage boundary. |

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
- the eight ordered composition layers, render targets, composition/effect
  routing policy, including coverage-window resolution, and graphics-state
  containment;
- parameter values, modifiers, options, banks, transport, telemetry, resource
  service implementations, and control target exposure;
- mapping resolution and provenance;
- presets, scenes, mapping banks, migrations, recovery, and transactional
  persistence;
- runtime commands, immutable query snapshots, health, diagnostics, and
  performance accounting.

Runtime core must not depend on concrete elements, `ofApp`, or operator UI.
During SEAC-3, concrete built-in effect selection, default values, coverage-mask
parameters, and shader execution remain in the host's `PostEffectChain`
adapter. Runtime core must not include that adapter.

### `SynaptomeHost`

The executable owns:

- openFrameworks process bootstrap and callbacks;
- physical windows and graphics contexts;
- monitor placement, focus, fullscreen, and OS quit behavior;
- platform event translation;
- construction of concrete filesystem, MIDI, OSC, serial, audio, and sensor
  adapters;
- Browser, Console, HUD, and other operator presentation/controllers.

The host calls a runtime facade and consumes runtime query models. It does not
include concrete element headers.

### Element Targets

Each element package or cohesive implementation family builds as its own static
library. It depends only on:

- Element SDK,
- the pinned openFrameworks toolchain,
- dependencies and add-ons declared by that package.

Each element bench links the real element library and a reusable SDK test host.
It must not include implementation `.cpp` files, redefine private access, or
include the host source tree.

### Generated Built-In Registration

`SynaptomeBuiltinElements` is a generated translation unit or small static
library. It is the only composition-time bridge that includes selected concrete
element headers. The host and isolated benches invoke the same registration
entrypoint.

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

An element type has a pure-data descriptor available without construction or
`setup()`. At minimum it contains:

- type ID, display label, implementation version, and element kind;
- owning package ID/version and compatible SDK/runtime range;
- capabilities and declared dependencies;
- parameter declarations and supported actions;
- deterministic/reset support;
- render/resource requirements;
- lifecycle and test requirements.

Element kinds in v1 are `visual` and `effect`. Operator overlays remain
spine-owned UI modules, not creative elements. Existing effects may use a
runtime adapter until a focused effect interface is extracted, but they must
share the same identity, registration, diagnostics, and scene-validation path.

### Registration

One non-global runtime registry accepts an atomic registration record:

```text
descriptor + creator + package/version owner
```

Rules:

- every Runtime receives one explicitly owned registry; there is no
  process-global fallback;
- empty and duplicate IDs fail before activation;
- descriptor and creator registration cannot partially succeed;
- registrations can be enumerated and attributed to their owner;
- host and bench use the same generated registration entrypoint;
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
- the old live instance remains active until a replacement has configured and
  set up successfully;
- adoption rejects host-owned ID collisions before mutation, swaps the staged
  registry and element as one commit, preserves modifiers on matching stable
  parameter IDs, and leaves candidate defaults/base values authoritative;
- after adoption, the host synchronously invalidates pointer-bearing parameter
  views and retired element routes before publishing layer metadata;
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

SEAC-4 implements this representation. SEAC-2 freezes its boundary and
ownership.

### Packages And Offline Inspection

`layer.package.json` evolves into the canonical typed package declaration.
Legacy layer catalog JSON may remain as a generated compatibility artifact
until runtime core consumes the package type directly.

Offline inspection reads declarations only. It does not instantiate an element,
allocate graphics resources, open devices, or call setup.

Package mapping declarations remain suggestions with full transform and
provenance data. Nothing auto-applies a mapping, preset, or scene mutation.

## Runtime Facade

The first host/runtime seam is a `Runtime` facade with the semantic surface:

```text
setup
update
render
resize
shutdown
submit typed command
read immutable snapshot
read output texture/target
```

`ofApp` can initially delegate to this facade while behavior remains unchanged.
The first runtime-owned aggregate should be the composition layer model
currently represented by `ConsoleSlot`, followed by generic element
create/configure/setup/update/render/clear behavior.

UI calls runtime commands and reads snapshots. It does not receive a raw
`ofApp*` as its service API.

`Runtime::resolveEffectCoverage` is an in-repository RuntimeCore composition
policy API. It is not part of the public Element SDK, does not define a public
effect interface, and does not change the compiler-matched source/static-link
compatibility decision.

## SEAC-3 Extraction Sequence

SEAC-3 must preserve output and proceed in this order:

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
6. Generate `SynaptomeBuiltinElements`; remove Signal Bloom's concrete include,
   compilation, and registration from the host.
7. Replace element-specific host casts and MIDI helpers with parameter/action
   contracts.
8. Inject audio, media/resource, logging, and path services; migrate singleton
   consumers incrementally.
9. Move remaining implementations by cohesive family and add their focused
   targets and benches to the solution.

The solution must stop hard-coding a machine-specific openFrameworks project
path. The authoring and package bench projects must become first-class solution
targets or share an equivalent reproducible command.

## Promotion Gates

No-output-change extraction is accepted only when:

- an SDK header compile check rejects UI, host, adapter, catalog I/O, and
  concrete-element dependencies;
- Signal Bloom builds through its element library, not by including its `.cpp`;
- host and bench use the same registration entrypoint;
- runtime core has no concrete element or operator UI dependencies;
- the host has no concrete element includes or casts for the migrated element;
- descriptor inspection does not instantiate the element;
- failed replacement preserves the prior live layer;
- setup/update/offscreen render/shutdown/reload pass through the real SDK seam;
- graphics-state containment and declaration parity pass;
- Release, public-app, BrowserFlow, scene round-trip, and existing authoring
  profiles remain green.

## Explicitly Deferred

This decision does not promise or implement:

- loading an arbitrary raw openFrameworks `ofApp` folder;
- a stable binary ABI;
- native hot loading or unloading;
- automatic package discovery or activation;
- the complete Scene v2/state-provenance model;
- the final parameter declaration implementation;
- live dual-screen hardware validation.

Those remain separate gated tasks. A raw openFrameworks experiment must first
be wrapped behind the lifecycle; it gains scenes, mappings, presets, and
Browser control only as it declares the corresponding stable contract.

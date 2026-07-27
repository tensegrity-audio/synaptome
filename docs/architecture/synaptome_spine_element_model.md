# Synaptome Spine, Element, And Layer Model

Status: Canonical architecture direction, accepted 2026-07-26.

This document defines Synaptome's public system model. It is the terminology
and boundary authority for future architecture, package, SDK, scene, mapping,
and build work. Existing `Layer` class names, `layer` JSON fields, file names,
and public parameter IDs remain compatibility surfaces until an explicit
migration provides aliases and fixtures.

Read with:

- [`synaptome_system_architecture.md`](synaptome_system_architecture.md) for the
  broader application map.
- [`synaptome_subsystem_anatomy.md`](synaptome_subsystem_anatomy.md) for current
  implementation anchors and gaps.
- [`synaptome_layer_system_roadmap.md`](synaptome_layer_system_roadmap.md) for
  the transitional package and SDK implementation sequence.
- [`../project_ops/roadmap.md`](../project_ops/roadmap.md) for active priority
  and promotion state.
- [`../project_ops/in_progress/spine_element_architecture_convergence.md`](../project_ops/in_progress/spine_element_architecture_convergence.md)
  for the ordered implementation milestones and promotion gates.

## System Definition

Synaptome is a stable live-performance host that loads contract-compatible
creative elements, gives them shared runtime services, and composes their state
into reproducible scenes.

The useful split is:

```text
spine
  hosts and coordinates
elements
  running in composition
layers
  saved together as
scenes
```

The spine makes an element operable, controllable, persistable, testable, and
composable. The element supplies the creative behavior.

## Canonical Terms

| Term | Definition |
| --- | --- |
| Spine | The stable Synaptome runtime: lifecycle, rendering, composition, parameters, transport, mappings, persistence, discovery, validation, operator surfaces, and diagnostics. |
| Element type | Executable creative behavior, such as a simulation, renderer, media player, or effect implementation. The current C++ `Layer` subclass is the first visual-element implementation seam. |
| Element definition | A stable, named catalog configuration backed by an element type. Multiple definitions may share one implementation while preserving distinct public identities and state models. |
| Element instance | One running copy of an element definition with instance-owned state and parameter values. |
| Layer | One ordered composition container in the live stack. A layer hosts at most one visual element instance and owns whole-layer active state, opacity, coverage, and composition placement. The current Console exposes eight layers. |
| Element family | An organizational family of related element definitions or implementations. This replaces the ambiguous public phrase `layer group`. |
| Element package | A distributable bundle containing manifest metadata, implementations or content, parameter declarations, defaults, presets, mapping suggestions, assets, compatibility requirements, and tests. |
| Parameter | A stable, typed, addressable value exposed by the spine. Browser edits, scenes, presets, mappings, and modifiers all use the same parameter contract. |
| Mapping | A visible route from an input source, through a declared transform, to a parameter or spine action. |
| Preset | A named parameter-value bundle for one element definition. A preset does not assign layers or store a complete performance. |
| Scene | A portable composition snapshot: element assignments across layers, explicit parameter values, layer state, and intentionally scene-owned mappings or banks. |
| Machine profile | Local devices, ports, monitor placement, local paths, and operator preferences. Machine state is not silently embedded into portable scene state. |
| Mode | A state-continuous behavior variation within one element. A change that replaces the core state model is another element definition, not a mode. |
| Content asset | Non-executable input such as an STL, video, image, shader, font, or data file consumed by a compatible element type. |

Public wording should say:

```text
Load an element into Layer 1.
Layer 1 hosts one element instance.
Save the eight-layer composition as a scene.
```

## Compatibility Naming

The current implementation uses `Layer` for creative implementations and
`slot` for composition containers. Migration should preserve behavior while
moving public language toward the canonical model:

| Current Name | Canonical Meaning | Migration Rule |
| --- | --- | --- |
| `Layer` C++ class | Visual element interface | Introduce an `Element` or `VisualElement` SDK name behind a compatibility alias before renaming implementations. |
| `LayerFactory` | Element type registry | Keep current type IDs stable; rename code only with focused build and factory fixtures. |
| `LayerLibrary::Entry` / layer asset | Element definition | Preserve asset IDs and catalog compatibility while schemas adopt the clearer term. |
| Console slot | Layer | Keep `console.layer{slot}` parameter IDs stable; the existing IDs already express the desired public composition model. |
| Layer group | Element family | Treat this as a documentation/catalog label migration, not a public ID rewrite. |
| Layer package | Element package | Existing schemas and commands may retain legacy filenames during the first contract version. |

No source symbol, JSON field, asset ID, registry prefix, scene target, MIDI
target, or OSC target is renamed merely to make the terminology look clean.

## Spine Responsibilities

The spine owns:

- application and element lifecycle,
- projection and offscreen render contexts,
- layer ordering, active state, opacity, coverage, and composition,
- parameter registration, metadata, values, modifiers, and provenance,
- transport, time, BPM, and shared runtime signals,
- MIDI, OSC, sensor, audio, hotkey, and device adapters,
- mapping transforms and conflict reporting,
- package inspection, compatibility validation, and controlled activation,
- preset, scene, mapping-bank, and machine-profile persistence,
- Browser, Console, HUD, projection, and operator diagnostics,
- resource ownership, failure containment, and performance evidence,
- versioned schemas, migrations, fixtures, and confidence gates.

The spine does not:

- dictate an element's creative algorithm,
- silently install mappings or replace operator state,
- hide input-source behavior inside an element,
- guarantee arbitrary C++ or arbitrary openFrameworks app compatibility,
- allow elements to own competing app shells or unmanaged operator windows,
- depend on a specific firmware or hardware implementation,
- treat every dropped file as executable code,
- promise native hot-loading without a tested dependency and ABI policy.

## Element Contract

A visual element receives services from the spine and returns behavior through
a bounded lifecycle. The present `Layer` interface exposes `configure`,
`setup`, `update`, `draw`, and resize hooks, then relies on object destruction
for cleanup. The target lifecycle must make unload explicit:

```text
configure
  -> register parameters / setup
  -> update
  -> draw
  -> unload
```

The target contract must make the following explicit:

- stable type, definition, instance, and parameter identities,
- declared parameters and private implementation state,
- setup and teardown ownership,
- viewport, camera, timing, transport, and opacity context,
- assets and path resolution,
- graphics-state containment,
- thread, device, network, and permission requirements,
- failure and health reporting,
- deterministic seed and reset behavior where applicable,
- serialization and migration responsibilities,
- setup-time, frame-time, memory, and allocation expectations.

An element may have no public parameters and still be hostable. It does not
become a fully controllable Synaptome instrument until it declares a stable
parameter surface.

During the current compiler-matched compatibility phase, `setup()` receives an
isolated staging registry. It may register only the element instance namespace
and must not retain that registry address. The concrete
`{instancePrefix}.opacity` address is reserved for the layer container and is
rejected even when an element registers it inside its namespace. Elements that
require later registry lookup receive the canonical live registry through the no-throw
`onParameterRegistryCommitted()` hook. A candidate is published only after
setup and registry validation succeed; failed setup or commit leaves the live
element, layer metadata, parameter state, mappings, and FBOs unchanged.

Whole-layer active state and opacity belong to the layer container. Elements
must not add a second generic owner for those concepts. Element-local
visibility parameters must name the internal object they affect.

## openFrameworks Compatibility

A raw openFrameworks `ofApp` folder is not an in-process Synaptome element.
It normally owns the app lifecycle, windows, event callbacks, build settings,
add-ons, data path, graphics state, threads, and devices that the spine already
owns.

Synaptome should publish an honest compatibility ladder:

| Level | Input | What Works |
| --- | --- | --- |
| 0: Reference experiment | Existing `ofApp` project | Use a migration guide or run externally and share video/texture output through a supported bridge. It is not directly loadable. |
| 1: Wrapped element | Creative state moved behind the element lifecycle | Runs in a layer and receives spine render/time context. Parameters may be empty. Source compilation is still required. |
| 2: Parametric element | Wrapped element plus declared parameters | Browser control, scene persistence, MIDI/OSC targetability, presets, and modifiers become possible. |
| 3: Cataloged definition | Parametric element plus stable manifest/catalog identity | Browser discovery, layer assignment, and scene references become stable. |
| 4: Performance instrument | Cataloged definition plus tested presets, mappings, and lifecycle | The element is ready for controlled live use. |
| 5: Element package | Complete bundle plus compatibility and bench evidence | It can be inspected and built independently of the full app. Installation still depends on its source/module strategy. |
| 6: Installable module | Versioned native module or generated registration | No hand-edit to the host composition root. Native loading still requires compatible compiler, openFrameworks, dependencies, and ABI. |

Data-only content follows a safer path:

```text
drop supported content file
  -> generic precompiled element type validates it
  -> stable element definition is generated
  -> operator explicitly activates it
```

An STL, video, image, or shader can become a true drop-in asset when a
precompiled element already knows how to load that content kind. The content
file is not itself executable element code.

## Identity And Addressing

Each identity has one job:

| Identity | Role |
| --- | --- |
| Package ID | Globally unique distribution and versioning identity. |
| Element type ID | Resolves to one executable implementation. |
| Element definition ID | Stable scene- and Browser-facing configured identity. |
| Element instance ID | Identifies one running copy. |
| Layer index/ID | Identifies its position in the live composition. |
| Parameter suffix | Stable element-local control name such as `speed`. |
| Runtime parameter ID | Expanded live address such as `console.layer1.speed`. |

Folder paths, absolute paths, timestamps, labels, and current layer numbers must
not generate package, type, or definition identity.

## Parameter Contract

Parameters are the common language between elements and spine services.

Parameter declarations must be structured data with:

- suffix ID,
- value kind,
- stable section/group ID,
- human label,
- default,
- range and step where applicable,
- units,
- description,
- options or a runtime option-source reference,
- quick-access metadata,
- deprecation and replacement metadata.

The group is not parsed from the display label. For example:

```json
{
  "id": "windSpeed",
  "kind": "float",
  "group": "motion",
  "label": "Cloud Wind Speed",
  "default": 0.4,
  "range": { "min": 0.0, "max": 2.0, "step": 0.01 },
  "units": "world/s"
}
```

The Browser may render this as `Motion: Cloud Wind Speed`, but persistence and
grouping use the structured fields.

Rules:

- Declare the public parameter surface once and generate or bind the other
  representations from it.
- Use normalized `0.0-1.0` colors and opacity for new contracts.
- Use degrees for public angles, seconds for duration, Hz for frequency, and
  explicit units for rates and distances.
- Use generic model targets. MIDI, OSC, audio, and sensor source names belong
  to mappings.
- Treat public IDs, kinds, units, and semantics as versioned API.
- Preserve legacy aliases until scene and mapping migration evidence exists.

## Control And Mapping Model

All control adapters feed one route model:

```text
physical or network input
  -> adapter source
  -> mapping transform
  -> parameter or spine action
```

- A device map translates physical controls into stable logical sources.
- MIDI and OSC are adapters, not separate parameter vocabularies.
- Mapping transforms own ranges, smoothing, deadband, inversion, blend, and
  relative/absolute behavior.
- Elements expose targets and do not read ordinary source-specific control
  state behind the Browser.
- Package mappings are suggestions until an operator previews and applies
  them.
- Conflicts must be visible, and application must support rollback.

## Defaults, Presets, Scenes, And Local State

Value precedence must be explicit and visible:

```text
element default
  -> selected preset
  -> explicit activation override
  -> scene value
  -> live operator edit
  -> active modifiers
```

The rightmost applicable source controls the live result. The operator should
be able to inspect the base value, live value, active modifiers, and value
origin.

State ownership:

| State | Owner |
| --- | --- |
| Immutable starting values | Element package/default contract |
| Named element look or behavior | Preset |
| Layer assignments and explicit performance state | Scene |
| Applied routes and active mapping bank | Scene or explicit operator mapping store, according to visible ownership |
| Device ports, monitor placement, local paths | Machine profile |
| Browser layout and local authoring preferences | Operator-local preferences |
| Telemetry, current frame, transient input | Runtime only |

Portable scenes must not silently absorb machine-local state.

## Package Contract

An element package should eventually declare:

- package identity and version,
- one or more element types and definitions,
- source, generated-registration, module, or template strategy,
- runtime and SDK compatibility range,
- compiler/ABI requirements for native modules,
- openFrameworks and add-on dependencies,
- platform, renderer, GPU, network, device, and permission capabilities,
- public parameters and defaults,
- modes and element families,
- bundled presets and preset banks,
- suggestion-only mapping presets,
- content assets and provenance,
- scene examples,
- static and runtime bench requirements,
- migration and deprecation metadata.

Unknown capabilities, unresolved dependencies, duplicate IDs, unsafe paths, or
contract drift fail inspection before activation.

## Build And Test Boundary

The target build shape is conceptually:

```text
synaptome-element-sdk
synaptome-runtime-core
synaptome-host
elements/<package>
```

The SDK contains only what element authors need. Runtime core owns spine
services. The host composes services and operator surfaces. Each native element
has a focused build and bench target rather than requiring every edit to
rebuild the host.

Confidence levels:

1. Static package and reference validation.
2. Parameter/default/preset/mapping manifest validation.
3. Registration and dependency resolution.
4. Isolated setup and declaration comparison.
5. Deterministic update and offscreen draw.
6. Blank-output, graphics-state, teardown, and reload checks.
7. Frame-time, memory, and allocation report.
8. Host integration, scene round-trip, and live projection acceptance.

No single level substitutes for the levels above or below it.

## Roadmap Alignment

The master roadmap should sequence architecture work in this order:

1. Complete the current show-safe rehearsal and close the dual-screen issue.
2. Freeze this terminology, identity, parameter, state-ownership, and
   compatibility model.
3. Extract the element SDK and runtime-core build seams.
4. Promote the current package scaffolding into Element Package v1.
5. Define strict scene, preset, mapping-bank, and machine-profile contracts.
6. Make the single-element confidence suite reusable across real elements.
7. Enable controlled package and data-only content discovery.
8. Consider native module loading only after dependency and ABI policy.
9. Resume biological, aurora, cosmic, and other content tracks as consumers of
   the stable spine.

Supporting roadmaps describe work inside one track. They do not redefine these
terms or become active without promotion in the master roadmap.

## Definition Of A Bulletproof Element

An element is ready for general use only when Synaptome can answer yes to all
of these:

- Can it be identified without relying on a local path?
- Can it be inspected without instantiating it?
- Can its dependencies and capabilities be checked before activation?
- Can it register exactly the public parameters it declares?
- Can defaults, presets, scenes, mappings, and modifiers be distinguished?
- Can it update and draw in isolation without leaking graphics state?
- Can it unload and reload without leaving resources or registrations behind?
- Can old scenes and mappings survive a compatible upgrade?
- Can an operator see and recover from failure?
- Can its performance cost be measured before a show?

That is the contract the spine offers and the discipline an element accepts.

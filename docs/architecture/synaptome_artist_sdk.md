# Synaptome Artist SDK And Compatibility Layer

Status: Supporting architecture draft; reviewed 2026-07-29. The validated
Package v1 plus controlled generated source-registration path is the honest
public baseline. SDK packaging is not an independently active workstream;
priority and promotion are owned by
[`../project_ops/roadmap.md`](../project_ops/roadmap.md). This document describes
what Synaptome can provide to openFrameworks artists and what gaps remain.

Terminology note: the canonical model now calls creative modules **elements**
and the eight ordered Console composition containers **layers**. See
[`synaptome_spine_element_model.md`](synaptome_spine_element_model.md). This
SDK document retains current `Layer` API names while the
compatibility-preserving Element SDK boundary is extracted.

Read with: [`synaptome_system_architecture.md`](synaptome_system_architecture.md),
[`synaptome_subsystem_anatomy.md`](synaptome_subsystem_anatomy.md),
[`synaptome_external_contracts.md`](synaptome_external_contracts.md),
[`synaptome_layer_system_roadmap.md`](synaptome_layer_system_roadmap.md),
[`synaptome_transport_reactivity.md`](synaptome_transport_reactivity.md), and
[`docs/contracts/contract_gaps.md`](../contracts/contract_gaps.md).

## Purpose

Synaptome should help artists who already know openFrameworks get live-performance infrastructure without rebuilding it for every project.

The artist-facing promise is:

```text
Bring an openFrameworks visual idea
  -> wrap it as a Synaptome layer
  -> declare parameters and metadata
  -> load it into slots
  -> control it from Browser, MIDI, OSC, sensors, scenes, and HUD
```

Synaptome should not replace openFrameworks. It should package the repetitive live-runtime work that many oF performance projects need.

## What Synaptome Gives An Artist

| Benefit | What Artist Gets | Current Evidence | Gap |
| --- | --- | --- | --- |
| Layer hosting | A visual can be loaded into an eight-slot Console. | `Layer`, `LayerFactory`, `LayerLibrary`, Console slots. | Public authoring guide and fixture-backed catalog. |
| Parameter UI | Declared values appear in the Browser with labels/ranges/groups. | `ParameterRegistry`, Browser rows. | Parameter vocabulary and generated manifest. |
| Mapping | Same parameters can be controlled by MIDI, OSC, devices, and sensors. | `MidiRouter`, `OscParameterRouter`, Device Mapper. | Target validation and mapping lifecycle docs. |
| Scene persistence | Composition-layer assignments and parameter values can be saved/reloaded; logical hardware control slots remain machine-profile state. | `ofApp::encodeSceneJson`, `ofApp::loadScene`. | Transaction-safe Scene load and round-trip fixtures. |
| Transport | Layers receive time, BPM, speed, and beat context. | `LayerUpdateParams`, `LayerDrawParams`, `transport.bpm`. | Public beat/phase contract and clock source plan. |
| Media | Video clips and webcams can be used as layers. | `VideoCatalog`, `VideoClipLayer`, `VideoGrabberLayer`. | Media discovery policy and media parameter vocabulary. |
| HUD feedback | Runtime state can be visible to the performer. | `HudRegistry`, `HudFeedRegistry`, overlays. | Feed/layout schema and stable widget authoring path. |
| Display separation | Operator UI can stay off projection output. | Projection/Control Window split, secondary display code. | Display/window schema and scene-load transaction. |
| Validation | Configs and contracts can be checked before a show. | `tools/validate_configs.py`, app-native tests. | Public contract tests with less implementation coupling. |

## Compatibility Levels

Synaptome should support a ladder, not one all-or-nothing integration path.

### Level 0: Existing oF Sketch As Reference

The artist has an existing `ofApp` sketch.

What Synaptome can provide:
- A checklist for separating update/draw logic from app lifecycle.
- Guidance on which values should become parameters.
- Guidance on which assets should become catalog metadata.

Current gap:
- The first migration guide now exists at
  [`layer_migration_workflow.md`](layer_migration_workflow.md); additional
  layer families still need to be moved through it.

### Level 1: Wrapped Layer

The artist moves visual logic into a `Layer` subclass.

Current API anchor:

```cpp
class Layer {
public:
    virtual void configure(const ofJson& config);
    virtual void setup(ParameterRegistry& registry) = 0;
    virtual void update(const LayerUpdateParams& params) = 0;
    virtual void draw(const LayerDrawParams& params) = 0;
};
```

What the artist gains:
- Runtime update/draw scheduling.
- Console slot hosting.
- Access to shared camera, viewport, time, BPM, speed, beat, and slot opacity.

Current gap:
- The API exists, but there is no stable public SDK package or authoring template.
- Registering a new `LayerFactory` type still requires source-level integration
  through a package leaf registrar plus the controlled built-in aggregate and
  project; the creator binding no longer belongs in `ofApp.cpp`, but a public
  no-source-edit extension mechanism does not exist yet.
- `LayerFactory::registerType()` now accepts one atomic static
  descriptor-plus-creator record and rejects invalid descriptors, missing
  creators, and duplicate type IDs before activation. Package ownership,
  serialized descriptors, and controlled generated registration are now
  proven by Signal Bloom.

### Level 2: Parametric Layer

The artist registers parameters in `setup(ParameterRegistry&)`.

What the artist gains:
- Browser controls.
- Scene persistence.
- MIDI/OSC/device-map targetability.
- HUD and mapping visibility.

Parameter rules for artists:
- Use stable dot-path suffixes such as `.visible`, `.opacity`, `.speed`, `.scale`, `.xInput`, `.yInput`, `.colorR`, `.gain`, `.mirror`, `.loop`.
- Treat IDs as public once scenes or mappings depend on them.
- Use descriptor labels/groups/ranges carefully; those are the UI contract.
- Avoid pointer lifetime surprises: registered parameter values must live as long as the layer instance.

Current gap:
- The parameter vocabulary now has a first public draft, but it is not yet enforced by SDK fixtures.
- The registry surface now has a generated manifest, but scene/MIDI/OSC/device-map/HUD targets are not yet semantically validated against it.

### Level 3: Cataloged Asset

The artist adds JSON metadata so the layer appears in the Browser.

Current asset fields:

| Field | Meaning |
| --- | --- |
| `id` | Stable asset ID used by scenes and Browser. |
| `label` | Human-facing name. |
| `category` | Browser grouping. |
| `type` | `LayerFactory` type string. |
| `registryPrefix` | Authoring prefix before slot rewrite. |
| `defaults` | Layer-specific JSON defaults. |
| `coverage` | Optional routing/coverage metadata. |
| `hudWidget` | Optional HUD widget metadata for HUD entries. |

What the artist gains:
- Browser discoverability.
- Slot assignment without editing the app loop.
- Scene persistence by asset ID.

Current gap:
- Layer asset schema is permissive.
- Golden fixture coverage now exists for catalog ingestion, but not for a minimal artist-authored source/catalog/scene example.
- FX, HUD widgets, media, and ordinary visuals are not yet documented as one coherent extension family.
- Browser asset inspection can instantiate layers and call `setup()`, so public layer authors need either a strict no-heavy-side-effects rule or Synaptome needs a manifest-only inspection path.

### Level 4: Mappable Performance Instrument

The artist declares parameters and can bind them through Browser learn flows, MIDI, OSC, device maps, host mic, or app-facing sensor inputs.

What the artist gains:
- A visual becomes an instrument.
- Control mappings can be reused.
- Sensors can modulate parameters without custom code in the layer.

Current gap:
- Mapping ownership is not explicit enough: global config, scene-local mappings, and operator-local preferences can overlap.
- Device-map and parameter-target validation now have advisory coverage, but strict SDK enforcement still needs cleanup/allowlist policy and fixtures.

### Level 5: Packaged Synaptome Extension

The artist ships:
- source layer class,
- package manifest with asset metadata, parameters, modes, options, presets,
  media, tests, and compatibility expectations,
- creator-only source registration file,
- optional presets/scenes,
- optional media files,
- parameter documentation generated from package declarations,
- validation fixture.

Target:
- A Synaptome extension should be installable and testable without touching
  `ofApp.cpp` or a handwritten host aggregate.

Current gap:
- Strict Package v1 and controlled generated registration exist, but there is
  no automatic extension loader.
- Runtime discovery, activation recovery, and native-module policy remain
  separate later gates.
- Broader built-in migration and public dependency guidance remain incomplete.
- Extension install and validation should eventually avoid editing the
  controlled host aggregate or project files.

First public decision:
- The first public Synaptome repo may ship with an honest controlled generated
  source-registration SDK path.
- Artist packages use a creator-only leaf, Package v1, catalog/scene fixtures,
  and validators instead of implying hot-loaded plugins exist.
- Synaptome may claim no project-list or host-aggregate edit for controlled
  source packages, but not runtime discovery or native plug-in loading.
- `LayerFactory::registerType()` fails loudly for invalid descriptors, missing
  creators, and duplicate type IDs, so malformed or colliding source
  registrations cannot silently replace implementations. Generated
  registration preflights the complete controlled type set before mutation.
  Discovery and transactional operator mappings remain later phases.

## Layer System Roadmap

The detailed roadmap for layer quality of life now lives in
[`synaptome_layer_system_roadmap.md`](synaptome_layer_system_roadmap.md).

The author-facing direction for individual layers lives in
[`synaptome_layer_design_standards.md`](../project_ops/synaptome_layer_design_standards.md).
In short, layers should evolve from C++-defined islands toward
self-describing modules with stable public parameters, visible mappings,
package metadata, presets, validation fixtures, and eventually single-layer
bench coverage.

That roadmap owns:

- layer package layout,
- folder-driven layer discovery,
- file-backed generated layers such as dropped STL models,
- package-declared parameters and manifest generation,
- static and dynamic option metadata,
- layer presets and preset banks,
- package OSC mapping presets that appear in the Browser mapping surface,
- single-layer validation and runtime bench work,
- controlled generated source registration now; discovery/module loading later.

The Artist SDK depends on that roadmap, but does not own all of its internal
implementation details.

## Runtime Systems Outside The Layer Roadmap

Some artist-facing capabilities feed layers but are not layer-system work.
BPM, beat detection, source confidence, onset/downbeat events, and transport
fallback policy live in
[`synaptome_transport_reactivity.md`](synaptome_transport_reactivity.md).

Layer packages may reference transport or beat sources in mapping presets, but
the runtime owns how those sources are produced.

## The Core Artist Library

This is the library Synaptome should make explicit.

| Library Piece | Current Code | Public Role |
| --- | --- | --- |
| Layer base class | `src/visuals/Layer.h` | Minimal oF-compatible module interface. |
| Layer update/draw params | `LayerUpdateParams`, `LayerDrawParams` | Shared time, BPM, speed, camera, viewport, beat, opacity context. |
| Parameter registry | `src/core/ParameterRegistry.h` | Typed controllable values and metadata. |
| Modifier math | `src/common/modifier.h` | Standard modulation behavior for OSC/MIDI/sensors. |
| Layer factory | `src/visuals/LayerFactory.*` | Type string to C++ layer creator. |
| Layer library | `src/visuals/LayerLibrary.*` | JSON catalog ingestion. |
| Media primitives | `VideoClipLayer`, `VideoGrabberLayer`, `VideoCatalog` | Reusable media layers and clip catalog. |
| Transport | `transport.bpm`, `globals.speed`, layer params | Shared performance time. |
| Device maps | `device_maps/*.json`, `DevicesPanel` | Data-driven controller roles. |
| Browser surface | `ControlMappingHubState` internally | Operator editing and mapping surface. |
| HUD feeds/widgets | `HudRegistry`, `HudFeedRegistry`, overlays | Performer feedback extension surface. |

## Parameter Vocabulary Starter

This should become a dedicated generated/reference doc, but the starter vocabulary is already visible.

| Family | Recommended Suffixes | Common Range | Meaning |
| --- | --- | --- | --- |
| Visibility | `.visible`, `.opacity`, `.alpha` | `0..1` or bool | Whether and how strongly a layer contributes. |
| Time | `.speed`, `.bpmSync`, `.bpmMultiplier`, `.paused` | varies | Local time behavior relative to global transport. |
| Input modulation | `.xInput`, `.yInput`, `.speedInput`, `.gainInput` | `0..1` | Common sensor/control entry points. |
| Transform | `.scale`, `.rotationDeg`, `.xBias`, `.yBias`, `.orbitRadius` | varies | Spatial placement and motion. |
| Color | `.colorR`, `.colorG`, `.colorB`, `.bgColorR`, `.trailAlpha` | `0..1` | Shared color controls. |
| Media | `.clip`, `.device`, `.gain`, `.mirror`, `.loop` | varies | Clip, webcam, and playback behavior. |
| FX | `.route`, `.coverage`, `.coverageMask`, `.threshold`, `.mix` | varies | Post-effect routing and effect intensity. |
| Transport | `transport.bpm`, `globals.speed` | BPM/speed ranges | Global time controls. |

Rules:
- Prefer reusable suffixes before inventing new names.
- Use labels and descriptions that make sense in the Browser.
- Avoid show-specific names for generic controls.
- Treat public parameter IDs like API: rename only with migration notes.

## Reusable Authoring Templates

The public SDK should teach artists by template, not just by source-code archaeology. A template is a small, copyable contract that says which parameter IDs, ranges, route shapes, scene fields, and Browser labels belong together.

Template families to make explicit:

| Template | Purpose | Example Shape |
| --- | --- | --- |
| Global transport | One shared performance clock that layers can opt into. | `transport.bpm`, `globals.speed`, `.bpmSync`, `.bpmMultiplier`. |
| Global show controls | Values that affect the whole runtime or a shared effect bus. | `fx.master`, global blackout/fade, global sensor gain. |
| Layer-local basics | Per-slot controls that every layer can expose consistently. | `.visible`, `.opacity`, `.speed`, `.scale`, `.rotationDeg`. |
| Layer-local color | Consistent color controls that can differ per layer and persist per scene. | `.colorR`, `.colorG`, `.colorB`, `.bgColorR`, `.bgColorG`, `.bgColorB`, optional palette/preset IDs. |
| Media playback | Consistent clip/webcam controls. | `.clip`, `.loop`, `.playbackSpeed`, `.gain`, `.mirror`, `.device`. |
| OSC routes | Predictable routes for external senders. | `/synaptome/global/bpm`, `/synaptome/slot/1/opacity`, `/synaptome/layer/<asset>/<param>`, `/sensor/<source>/<metric>`. |
| Sensor modifiers | A standard way to map audio/helper/device metrics into params. | source, metric, target parameter, min/max, smoothing, deadband, invert. |
| Scene persistence | What should save with a scene vs stay local to the machine/operator. | slot assets, layer-local params, mappings, banks, media IDs; not local monitor coordinates unless explicitly promoted. |

Important distinction:
- Global controls are shared knobs for the performance runtime.
- Layer-local controls use the same vocabulary but persist separately per slot/layer in a scene.
- A color template should make every layer easy to tint consistently, while still allowing each layer instance to carry its own scene-specific color values.
- OSC and MIDI templates should target the same parameter IDs the Browser shows; a route catalog should not become a second naming system.

## Killer Example Sketch

The first public example should demonstrate why Synaptome deserves to exist without depending on any private show scene.

The example should prove:
- An openFrameworks visual can be wrapped as a `Layer`.
- Parameters appear in the Browser with useful labels, groups, ranges, and defaults.
- Global BPM and layer-local speed both matter.
- A shared color template can tint multiple layers consistently while each layer keeps independent scene values.
- MIDI, OSC, host audio, and/or helper sensor inputs can map into the same parameter vocabulary.
- A video/media source can be loaded and controlled through the same runtime.
- The Console can combine the example across slots.
- HUD feedback can show the important live state.
- A saved scene reloads the full performance setup.

Working example criteria:
- It should be visually rich enough to be useful in a live set.
- It should be small enough for an artist to read in one sitting.
- It should avoid show-specific assets, private hardware, or hardcoded local paths.
- It should exercise templates instead of inventing one-off parameter names.

Possible shape:

```text
one generative signal layer
one media/video layer
one post-effect layer
global BPM + master intensity
per-layer palette/color params
host mic or OSC sensor modulation
MIDI map example
scene save/reload fixture
HUD summary
```

Validated baseline fixture:
- `docs/examples/artist_sdk/SignalBloomLayer.h`
- `docs/examples/artist_sdk/SignalBloomLayer.cpp`
- `docs/examples/artist_sdk/register_signal_bloom.cpp`
- `docs/examples/artist_sdk/signal_bloom.layer.json`
- `docs/examples/artist_sdk/signal_bloom.scene.json`
- `tools/testdata/artist_sdk/expected_artist_sdk_example.json`

Validation:

```powershell
python tools\validate_artist_sdk_example.py --check
```

This fixture proves the current honest path: a source-registered `Layer`
subclass, one package leaf registrar shared by the host aggregate and bench, a
Browser catalog entry, a saved scene with a Console slot, reusable parameter
suffixes, MIDI/OSC/sensor route targets, and a paired media layer. It is
intentionally not the final extension mechanism; controlled generated source
registration is implemented, while discovery and native loading remain later.

## Existing oF Code Migration Checklist

For a normal openFrameworks sketch:

1. Move visual state into a `Layer` subclass.
2. Move setup-only resources into `configure()` or `setup()`, keeping Browser offline hydration in mind.
3. Move per-frame logic into `update(const LayerUpdateParams&)`.
4. Move drawing into `draw(const LayerDrawParams&)`.
5. Replace private tweak constants with `ParameterRegistry` values.
6. Add a JSON catalog entry with stable `id`, `type`, `category`, and `registryPrefix`.
7. Add default parameter values to the layer config or constructor.
8. Load the layer into a Console slot.
9. Map parameters from Browser, MIDI, OSC, host mic, or app-facing sensor inputs.
10. Save a scene and confirm reload preserves the expected state.

## What Is Core vs Example

| Item | Core Synaptome | Example Content |
| --- | --- | --- |
| Element lifecycle and declaration interfaces | Yes, as public SDK headers | No |
| Scoped parameter declaration/binding interface | Yes, as public SDK headers | No |
| Element registry, catalog, and package loading | Yes, in runtime core rather than the public SDK | No |
| Generic media layers | Yes | Maybe with example media |
| Generic geometry/generative demo layers | Maybe minimal examples | Yes for style-specific variations |
| Specific STL models/videos/scenes | No | Yes |
| Device-map schema and one or two generic maps | Yes | Additional hardware packs |
| App OSC contract | Yes | Example sender fixtures |
| ESP32 firmware | No | Helper repo |
| Show-specific presets | No | Example pack |

## Current SDK Gaps To Close

| Gap | Current State | Target |
| --- | --- | --- |
| Public layer authoring guide | Architecture docs plus validated `docs/examples/artist_sdk/**` fixture. | Step-by-step guide expanded from the fixture. |
| Parameter vocabulary | Implicit in code and examples. | Versioned reference plus generated manifest. |
| Layer system roadmap | Layer package, folder discovery, file-backed generated layers, package params, options, presets, mapping presets, and bench work are larger than the SDK overview. | Use `synaptome_layer_system_roadmap.md` as the primary roadmap for improving the layer system. |
| Factory registration | Package v1 generates Signal Bloom's complete contract and build record; its leaf supplies only the creator, and the host/benches share one generated entrypoint. | Extend the controlled set without host/project edits; decide discovery and native loading separately. |
| Transport/reactivity contract | BPM and beat context exist, but clock source, confidence, onset/downbeat, and fallback policy are runtime concerns outside the layer roadmap. | Use `synaptome_transport_reactivity.md` for BPM, beat detection, and timing-source work. |
| Catalog fixture | Golden static `LayerLibrary` ingestion snapshot plus validated artist-authored source/catalog/scene fixture. | Use both fixtures to tighten the public authoring guide and future package seam. |
| Mapping lifecycle | Real Browser/MIDI/OSC flows. | Public docs for global/scene/local mapping ownership. |
| Media onboarding | Manifest-only `videos.json` contains one reviewed, provenance-complete public loop, `aurora-veil-r1`, assigned to the Browser-visible default clip layer. | Keep folder scanning deferred and require a bounded request for any additional asset. |
| Display stability | Works, but still coupled to scene-load work. | Transaction-backed display/window contract. |
| Public tests | Strong internal app tests. | Public SDK/contract fixtures that do not depend on private access. |
| Browser inspection | Package/generated metadata appears in a dedicated read-only Browser category; native coverage proves it does not instantiate or hydrate layers. | Keep inspection separate from activation while adding option/preset/mapping controls. |

## First Public SDK Slice

The smallest useful public SDK slice should include:

- the compatibility `Layer` interface and target `VisualElement` lifecycle,
- lifecycle, render, identity, capability, and service contexts,
- structured parameter declarations and scoped storage binding,
- one atomic element descriptor/creator registration record,
- `modifier.h`
- a minimal example layer,
- a minimal media layer example,
- a minimal device-map example,
- a generated parameter manifest command,
- a Browser-visible asset fixture,
- docs for mapping MIDI/OSC to parameters,
- docs for projection/control-window setup.

`LayerFactory`, `LayerLibrary`, package activation, and filesystem catalog
loading are runtime-core facilities. They are not author-facing SDK
dependencies. The frozen source/static-link boundary and extraction sequence
are documented in
[`element_sdk_v1_boundary.md`](element_sdk_v1_boundary.md).

That slice would explain why Synaptome deserves to exist without depending on any specific show scene.

The first vertical slice now exists and is converged: `synaptome-layer check`,
manifest-only Browser inspection, a package-generated runtime adapter,
default-off Signal Bloom activation, a native lifecycle bench, a scene fixture,
one package-owned dynamic option declaration, and one reviewed media loop.
The Browser now displays package named choices and resolves the app-owned
transport BPM provider as read-only metadata without rewriting defaults.
Unavailable defaults stay visible and preserved. Active packages now expose
labeled preset banks; the selected stable IDs persist operator-locally and
apply to the next layer load without rewriting the active scene or mappings.
Matching live parameters now render and edit those static/runtime choices
through one labeled picker while preserving unavailable provider values.
Remaining work is explicit mapping-preset apply/edit controls, factory
registration evolution, and projection/control-window tutorial polish.

Do not expand the example with more media by default. The safe next SDK slice
is an explicit mapping-preset preview/apply/edit flow with conflict handling and
rollback; folder discovery and no-source-edit installation remain future
mechanisms.

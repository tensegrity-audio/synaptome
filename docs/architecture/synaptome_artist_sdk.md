# Synaptome Artist SDK And Compatibility Layer

Status: Architecture draft with validated public SDK example fixture and first public registration decision. This document describes what Synaptome can provide to openFrameworks artists as a reusable library/runtime, and what gaps remain between the current app and that public SDK.

Read with: [`synaptome_system_architecture.md`](synaptome_system_architecture.md), [`synaptome_subsystem_anatomy.md`](synaptome_subsystem_anatomy.md), [`synaptome_external_contracts.md`](synaptome_external_contracts.md), and [`docs/contracts/contract_gaps.md`](../contracts/contract_gaps.md).

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
| Scene persistence | Slot assignments and parameter values can be saved/reloaded. | `ofApp::encodeSceneJson`, `ofApp::loadScene`. | Transaction-safe scene load and round-trip fixtures. |
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
- No migration guide exists yet.

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
- Registering a new `LayerFactory` type still requires source-level integration in app setup; a public extension mechanism does not exist yet.
- `LayerFactory::registerType()` should diagnose duplicate type names before this becomes a public plugin surface.

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
- factory registration,
- asset JSON,
- optional presets/scenes,
- optional media files,
- parameter documentation,
- validation fixture.

Target:
- A Synaptome extension should be installable and testable without touching `ofApp.cpp`.

Current gap:
- No plugin/package layout exists yet.
- Factory registration still requires source-level integration.
- Public dependency boundaries are not split from the current monolithic app.
- Extension install should eventually avoid editing `ofApp.cpp`.

First public decision:
- The first public Synaptome repo may ship with an honest source-registration SDK path.
- Artist examples should use a dedicated registration file/snippet, catalog JSON, scene fixture, and validator instead of implying hot-loaded plugins already exist.
- Synaptome must not claim no-source-edit installation until a generated registration, module manifest, or plugin/package loader is implemented and validated.
- `LayerFactory::registerType()` must fail loudly for empty or duplicate type IDs so package collisions cannot silently replace layer implementations.

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

This fixture proves the current honest path: a source-registered `Layer` subclass, a Browser catalog entry, a saved scene with a Console slot, reusable parameter suffixes, MIDI/OSC/sensor route targets, and a paired media layer. It is intentionally not the final extension mechanism; source registration remains explicit until Synaptome implements a generated registration or package loader.

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
| `Layer` interface | Yes | No |
| `ParameterRegistry` | Yes | No |
| `LayerFactory` and catalog loading | Yes | No |
| Generic media layers | Yes | Maybe with example media |
| Generic geometry/generative demo layers | Maybe minimal examples | Yes for style-specific variations |
| Specific STL models/videos/scenes | No | Yes |
| Device-map schema and one or two generic maps | Yes | Additional hardware packs |
| App OSC contract | Yes | Example sender fixtures |
| ESP32 firmware | No | Helper repo |
| Show-specific presets | No | Example pack |

## Current Gaps To Close

| Gap | Current State | Target |
| --- | --- | --- |
| Public layer authoring guide | Architecture docs plus validated `docs/examples/artist_sdk/**` fixture. | Step-by-step guide expanded from the fixture. |
| Parameter vocabulary | Implicit in code and examples. | Versioned reference plus generated manifest. |
| Layer package layout | No public extension folder structure. | Installable extension convention. |
| Factory registration | Source-level registration is the first public path; duplicate/empty type IDs now fail loudly. | Generated registration, plugin manifest, or module loader for no-source-edit installs. |
| Catalog fixture | Golden static `LayerLibrary` ingestion snapshot plus validated artist-authored source/catalog/scene fixture. | Use both fixtures to tighten the public authoring guide and future package seam. |
| Mapping lifecycle | Real Browser/MIDI/OSC flows. | Public docs for global/scene/local mapping ownership. |
| Media onboarding | `videos.json` manifest. | Decided manifest/folder-scan workflow. |
| Display stability | Works, but still coupled to scene-load work. | Transaction-backed display/window contract. |
| Public tests | Strong internal app tests. | Public SDK/contract fixtures that do not depend on private access. |
| Browser inspection | Offline hydration can call layer `setup()`. | Manifest-first inspection or explicit no-side-effect setup rule. |

## First Public SDK Slice

The smallest useful public SDK slice should include:

- `Layer.h`
- `ParameterRegistry.h`
- `modifier.h`
- `LayerFactory` and `LayerLibrary`
- a minimal example layer,
- a minimal media layer example,
- a minimal device-map example,
- a generated parameter manifest command,
- a Browser-visible asset fixture,
- docs for mapping MIDI/OSC to parameters,
- docs for projection/control-window setup.

That slice would explain why Synaptome deserves to exist without depending on any specific show scene.

Baseline now exists for the source/catalog/scene portion through `python tools\validate_artist_sdk_example.py --check`. Remaining slice work is packaging and tutorial polish, especially the factory registration mechanism, media onboarding policy, and projection/control-window setup guide.

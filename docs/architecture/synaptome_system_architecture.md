# Synaptome System Architecture

Status: Human-readable framework map for Synaptome as a public live-performance runtime.

Source: [`synaptome_subsystem_anatomy.md`](synaptome_subsystem_anatomy.md), [`synaptome_external_contracts.md`](synaptome_external_contracts.md), [`synaptome_artist_sdk.md`](synaptome_artist_sdk.md), [`docs/contracts/README.md`](../contracts/README.md), [`docs/contracts/contract_gaps.md`](../contracts/contract_gaps.md), and the current `synaptome` app source.

## Working Thesis

Synaptome is an openFrameworks-based runtime for modular, parameterized, sensor-reactive live audiovisual performance.

It exists so an audiovisual artist can build or load openFrameworks scenes without rebuilding the same show-control system every time: scene slots, parameter UI, MIDI mapping, OSC routing, sensor input, media browsing, HUD feedback, scene persistence, and validation.

Synaptome is the reusable app/runtime layer. Hardware helpers, private show content, governance scaffolding, generated hardware config, and product-specific experiments are outside the public runtime unless intentionally promoted as examples.

## Language Model

Public Synaptome language should be simple and consistent:

| Term | Meaning |
| --- | --- |
| Synaptome | The core live-performance runtime and eventual app/repo identity. |
| Browser | The operator workbench for browsing scenes, layers, media, parameters, mappings, device maps, and HUD controls. |
| Console | The eight-slot live deck where loadable visuals/scenes are assigned and performed. |
| HUD | The performer feedback surface for telemetry, status, layer summaries, routes, and sensor state. |
| Device Mapper | The Browser-facing surface for describing physical controls as reusable roles. |
| Projection | The audience-facing output window. |
| Control Window | The operator-facing window containing the three-band HUD, Console, and Browser layout. |

Legacy and implementation names should be handled this way:

| Name | Public Term | Notes |
| --- | --- | --- |
| `ControlMappingHubState` | Browser implementation | Internal class that currently implements much of the Browser and mapping surface. |
| Control Mapping Hub | Browser | Legacy public wording. Avoid it in new architecture language except when explaining history. |
| CMH | Browser | Legacy shorthand. Avoid in public docs. |
| `AssetBrowser` | Browser picker | Internal/specific picker for layer and asset selection inside the broader Browser workflow. |
| `ThreeBandLayout::workbench` | Browser band | Internal bottom-band name in `ThreeBandLayout`; the public layout is HUD, Console, Browser. |

## Architecture Layers

Synaptome should be explained as layered architecture, not as a list of UI modules. The Browser, Console, and HUD are important, but they are surfaces over a smaller core runtime model.

For the detailed subsystem-by-subsystem anatomy, including current code anchors, rules, relationships, and gaps, see [`synaptome_subsystem_anatomy.md`](synaptome_subsystem_anatomy.md).

For the public boundary contracts with MIDI controllers, OSC senders, helper repos, microphones, webcams, media, and displays, see [`synaptome_external_contracts.md`](synaptome_external_contracts.md).

For the artist-facing openFrameworks compatibility layer and first public SDK slice, see [`synaptome_artist_sdk.md`](synaptome_artist_sdk.md).

```text
Core model
  -> user-facing surfaces
  -> adapters and contracts
  -> content and extensions
```

### Core Runtime Model

These concepts are the center of the app. They should exist even if the user-facing surfaces change:

| Core Concept | What It Provides | Common Surfaces |
| --- | --- | --- |
| Parameters | Stable controllable values with type, label, range, defaults, groups, and modifiers. | Browser, HUD summaries, MIDI/OSC/device maps, scene files. |
| Scenes | Persisted performance state: slots, parameter values, mappings, HUD/window state, and runtime layout. | Browser scene list, Console slot state, app load/save. |
| Layers | Modular openFrameworks visuals that register parameters and draw into the runtime. | Browser catalog, Console slots, projection output. |
| Slots | Fixed live-performance containers for assigning, replacing, muting, and composing layers. | Console, Browser slot controls, scene persistence. |
| Media | Video, webcam, images, and future media entries with reusable playback/capture parameters. | Browser catalog, media layers, Console slots. |
| Devices | MIDI controllers, OSC sources, helper sensor inputs, hotkeys, and other control inputs. | Device Mapper, Browser mappings, HUD telemetry. |
| Mappings | Data-driven routes from physical/network inputs to parameter IDs or slot roles. | Browser, Device Mapper, MIDI/OSC routers. |
| Transport | Shared BPM, beat phase, and tempo-aware runtime state. | Browser parameter controls, layers/effects, HUD. |
| Contracts | Schemas, fixtures, and validators that let external devices and content talk to Synaptome safely. | Docs, tools, app-facing examples, app-native tests. |

### User-Facing Surfaces

These are how a performer touches the core model. They should not become the architecture boundary by themselves.

| Surface | Role |
| --- | --- |
| Browser | Browse, inspect, map, save/load, and edit runtime state. It is the main operator workbench for parameters, scenes, layers, media, mappings, and devices. |
| Console | Perform with eight slots. It answers "what is currently loaded and active?" |
| HUD | Monitor state without digging into controls. It answers "is the show healthy and responsive?" |
| Projection | Render the audience output without operator UI pollution. |
| Control Window | Keep operator control and feedback separate from projection output. |

The current Control Window should be described publicly as a three-band layout:

```text
HUD
Console
Browser
```

### Adapters And Contracts

Adapters connect the core model to the outside world. They should be replaceable as long as they preserve the public contracts:

| Adapter | Core Model Target |
| --- | --- |
| MIDI router | Parameters, slots, mappings, device roles. |
| OSC router | Parameters, sensor values, telemetry, mappings. |
| Device maps | Physical controls mapped to logical roles. |
| Helper/app OSC contract | External sensors/controllers mapped into Synaptome without helper source coupling. |
| Schemas and fixtures | Scene, layer, device, radio, and OSC compatibility. |

### Content And Extensions

Specific scenes, effects, shaders, videos, models, and show presets are content built on top of Synaptome. They prove the framework, but they are not the framework.

The useful public promise is: a new layer, media file, device map, or sensor source should become usable through the core model and surfaces without private edits to the app loop.

## Why Synaptome Deserves To Exist

openFrameworks is excellent for creative coding, rendering, media, devices, and low-level experimentation. It does not, by itself, give a performer a complete live runtime.

Synaptome fills that gap:

- A scene or layer can expose parameters once and have them appear in the Browser.
- A visual can be loaded into one of eight performance slots without editing the app loop.
- MIDI, OSC, hotkeys, and hardware device maps can target the same parameter IDs.
- Sensor data can arrive from any contract-compatible helper or device without app-specific helper source dependencies.
- The HUD, Console, Browser, and projection can share state while keeping performer controls off the audience output.
- Saved scenes and mappings let an artist prepare a show instead of hard-coding every state.
- Validation tools make the runtime safe enough to evolve before a show.

The value is not that Synaptome replaces openFrameworks. The value is that Synaptome turns openFrameworks work into a reusable live instrument.

## Blank-Slate Synaptome

If every specific scene, model, shader, video, and post effect were removed, Synaptome should still provide these framework primitives:

| Primitive | Artist Value | Current Evidence |
| --- | --- | --- |
| Performance shell | Launches the app, manages projection/control windows, keeps operator UI separate from output. | `synaptome/src/main.cpp`, `synaptome/src/ofApp.*`. |
| Three-band control window | Gives a stable operator layout: HUD, Console, Browser. | `ThreeBandLayout`, `MenuController`, HUD/console/browser states. |
| Eight-slot Console | Gives a fixed live deck for loading, replacing, muting, and composing visuals. | `ConsoleState::kSlotCount = 8`, `ofApp::consoleSlots`, `Ctrl+1..8` slot picker flow. |
| Layer authoring model | Lets a new visual become a loadable module with `setup`, `update`, `draw`, and declared parameters. | `src/visuals/Layer.h`, `LayerFactory`, `LayerLibrary`, `bin/data/layers/**/*.json`. |
| Parameter registry | Creates a shared vocabulary of controllable values with labels, groups, ranges, quick access, defaults, and modifiers. | `src/core/ParameterRegistry.h`, `src/common/modifier.h`. |
| Browser | Makes scenes, layers, media, parameters, mappings, device maps, and HUD controls browsable. | `ControlMappingHubState`, `AssetBrowser`, `LayerLibrary`, saved-scene callbacks. |
| Device Mapper | Describes physical controls as data so controllers map by role, not by hard-coded device code. | `DevicesPanel`, `bin/data/device_maps/*.json`, `docs/schemas/device_map.schema.json`. |
| HUD feedback | Gives the performer status, layer summaries, sensor telemetry, route state, and diagnostics. | `HudRegistry`, `HudFeedRegistry`, `OverlayManager`, `src/ui/overlays/*`. |
| Global transport | Gives scenes and effects a shared tempo reference. | `transport.bpm`, `LayerUpdateParams::bpm`, `LayerDrawParams::beat`. |
| Scene persistence | Saves and reloads performance state across slots, parameters, mappings, HUD/window state, and named scenes. | `ofApp::loadScene`, `ofApp::saveScene`, `encodeSceneJson`, `bin/data/layers/scenes/*.json`. |
| Sensor and OSC contracts | Allows external devices to drive parameters and HUD telemetry through stable messages. | `docs/osc_catalog.md`, `docs/contracts/README.md`, `synaptome/bin/data/config/osc-map.json`, public validators. |
| App-native tests | Lets the app framework be validated without PlatformIO or firmware builds. | `BrowserFlowTest`, `HotkeyTest`, `tools/run_control_hub_flow.py`, `tools/check_app_independence.py`. |

## Public Promise

As a public repo, Synaptome should make these claims understandable and testable:

1. Write or drop in a Synaptome-compatible openFrameworks layer, give it metadata and parameters, and it appears in the Browser.
2. Put that layer into one of eight Console slots, save the scene, and reload it later.
3. Expose parameters once, then control them from the Browser, MIDI, OSC, hotkeys, or sensor modifiers.
4. Add a MIDI controller map as JSON and bind physical knobs, faders, and buttons to logical roles.
5. Feed OSC sensor data from any helper or app-facing sender that speaks the contract, regardless of how the source hardware is implemented.
6. Use a Control Window and HUD to operate the show without polluting the projection output.
7. Validate configs, contracts, fixture output, and app-facing control flows before trusting a build live.

Some of this is already implemented. Some of it is still a target shape that needs explicit contracts before public packaging.

## What Is Synaptome

Synaptome should include:

- the openFrameworks app shell and composition root,
- the layer interface, layer factory, and layer asset catalog,
- the eight-slot Console runtime,
- scene save/load and scene schema support,
- the parameter registry and reusable parameter vocabulary,
- modifier support for MIDI, OSC, and automation-style inputs,
- MIDI, OSC, hotkey, and device-map adapters,
- the Browser, Console, HUD, Device Mapper, and overlay shell,
- media catalog and media layer primitives,
- app-facing schemas, fixtures, and validators,
- app-native tests for Browser, control, hotkey, and scene-facing flows,
- documentation for authoring layers, parameters, device maps, and app-facing OSC input.

## What Is Not Synaptome

These should be outside the Synaptome public runtime, even if they currently live in the same repo:

| Area | Current Paths | Relationship To Synaptome |
| --- | --- | --- |
| Helper firmware/source | External helper package | External bridge that turns hardware packets into app-facing OSC. |
| Product firmware | External firmware packages | Producers/controllers that speak shared contracts. |
| Radio schema package | External shared package | Shared contract package candidate, not app runtime source. |
| Specific show content | Current bespoke scenes, models, videos, style-specific assets | Examples or separate content packs unless intentionally bundled. |
| Project Ops framework | External `tensegrity-audio/project_ops` repo plus local `docs/project_ops/**` adapter docs | Process scaffolding; useful internally, not the app itself. |
| Historical payloads | Legacy firmware payload headers | Quarantined migration material. |

The rule remains:

```text
Synaptome may consume device data, but it must not depend on device implementation.
```

## Runtime Architecture

### App Shell

The current shell is `synaptome`, with `ofApp.cpp` acting as both composition root and too much runtime ownership. It wires openFrameworks lifecycle, windows, layer slots, parameter registration, UI states, MIDI/OSC routes, scene persistence, HUD, serial collector input, and tests.

This is the right runtime center, but it needs internal module boundaries before extraction:

- App composition: `main.cpp`, `ofApp.cpp`, `ofApp.h`.
- Core state: `ParameterRegistry`, `BankRegistry`, `ConsoleStore`.
- Runtime surfaces: projection draw, Console draw, Control Window draw, HUD/Browser layout.

### Layers

The reusable authoring surface is:

```cpp
class Layer {
public:
    virtual void configure(const ofJson& config);
    virtual void setup(ParameterRegistry& registry) = 0;
    virtual void update(const LayerUpdateParams& params) = 0;
    virtual void draw(const LayerDrawParams& params) = 0;
};
```

A layer can:

- receive JSON defaults,
- register parameters into the shared registry,
- update against elapsed time, speed, and global BPM,
- draw into a viewport/camera context,
- use an instance-specific registry prefix.

Current layer-to-slot flow:

```text
LayerLibrary asset id
  -> LayerFactory type
  -> configure(JSON defaults)
  -> rewrite registry prefix to console.layerN
  -> setup(ParameterRegistry)
  -> parameters appear in the Browser
  -> slot state persists into scene JSON
```

This is the heart of Synaptome as a framework: a scene author writes an openFrameworks layer once, then the runtime gives it a slot, parameter IDs, UI controls, mappings, and persistence.

### Parameters

`ParameterRegistry` is one of the most important Synaptome primitives. It supports:

- float, bool, and string parameters,
- metadata: id, label, group, units, description, range, quick access,
- default, base, and live values,
- snapshots,
- prefix removal for unloading layers,
- runtime modifiers.

`modifier.h` defines blend modes that let control inputs reshape values:

- additive,
- absolute,
- scale,
- clamp,
- toggle.

This is what makes a scene parametric instead of just compiled C++ art. It is also what lets the same parameter be edited from the Browser, a MIDI control, an OSC route, or a sensor stream.

### Parameter Vocabulary

A public Synaptome framework needs an explicit parameter vocabulary. Today it exists implicitly in repeated names and groups:

| Parameter Family | Examples Already Present | Public Meaning |
| --- | --- | --- |
| Visibility | `.visible`, `.alpha`, slot active state | Whether a layer contributes to output. |
| Motion and timing | `.speed`, `.bpmSync`, `.bpmMultiplier`, `transport.bpm` | Time behavior relative to global transport. |
| Transform | `.scale`, `.rotationDeg`, `.xBias`, `.yBias`, `.orbitRadius` | Spatial control for generative and media layers. |
| Color | `.colorR`, `.colorG`, `.colorB`, background color channels | Shared color control patterns. |
| Media | `.gain`, `.mirror`, `.clip`, `.loop`, `.device` | Video, webcam, and image behavior. |
| Sensor inputs | `.xInput`, `.yInput`, `.speedInput`, OSC source profiles | Modulation entry points. |
| Effects | `effects.*.route`, `effects.*.coverage`, effect-specific controls | Post-processing and routing. |

This should become a documented parameter library: a small set of recommended IDs, suffixes, groups, ranges, and semantics that new scenes can reuse.

### Console

The Console is the current performance deck:

- fixed eight-slot model,
- slot asset assignment,
- per-slot opacity and coverage,
- keyboard access through `Ctrl+1..8`,
- persistence through scene JSON and slot assignment config,
- slot inventory surfaced in the Browser and HUD.

The public Synaptome language should call this the Console or eight-slot Console. It is a primary feature, not just an internal UI.

### Browser

The Browser is the main operator workbench. It should be the front door for:

- layer assets,
- saved scenes,
- parameters and parameter groups,
- MIDI and OSC mappings,
- device maps,
- HUD widgets,
- media entries,
- templates and presets.

Current catalog sources:

- `bin/data/layers/**/*.json` for layers, HUD widgets, effects, media layer entries, and scene files,
- `config/videos.json` for video clips,
- `device_maps/*.json` for controller definitions,
- saved scene list callbacks from the app.

Important honesty for public docs: current media loading is catalog-driven through `videos.json`. The desired public workflow of "drop a video into a media folder and see it in the Browser" is a good Synaptome feature, but it is not fully contract-backed yet.

### Device Mapper

The Device Mapper is a core Synaptome idea.

Current device maps are JSON files such as `bin/data/device_maps/MIDI Mix 0.json`. They describe:

- device identity,
- port hints,
- columns/groups,
- roles like knob, fader, and button,
- MIDI binding metadata,
- labels and sensitivity.

This lets Synaptome understand a controller as an operator surface. The future public story should be:

```text
Add a JSON device map
  -> Synaptome sees the controller
  -> map roles to parameters
  -> save the mapping
```

That is separate from hardware helpers. MIDI devices, OSC sources, and radio/helper devices should all become inputs to the same parameter/control model.

### HUD

The HUD gives Synaptome its performer feedback:

- status,
- layer summaries,
- sensor telemetry,
- route state,
- menu mirror,
- diagnostics,
- current control context.

The HUD should monitor the runtime model without becoming the model itself. Parameters, devices, scenes, and slots remain core concepts; the HUD displays their state.

### Media Runtime

Synaptome already has the start of reusable media behavior:

- `VideoCatalog` reads configured clips from `config/videos.json`.
- `VideoClipLayer` exposes clip, gain, mirror, and loop controls.
- `VideoGrabberLayer` exposes webcam device, resolution, gain, and mirror controls.
- Media layer entries live under `bin/data/layers/media/`.

The public framework should separate:

- media discovery,
- media playback/capture layers,
- media parameter conventions,
- show-specific media files.

### Global Transport

The current app registers `transport.bpm` as a core parameter and passes BPM/beat data into layer update/draw params. Several layers and effects already use BPM sync ideas.

Public Synaptome should treat transport as a first-class runtime service:

- BPM,
- beat phase,
- tempo-synced layer behavior,
- future tap tempo, MIDI clock, Ableton Link, or OSC tempo inputs.

### Sensor And Helper Input

Synaptome should be able to consume sensor data from any source that speaks the app-facing contract.

Target helper flow:

```text
hardware or software source
  -> helper or sender-specific decode
  -> app-facing OSC
  -> Synaptome OSC/device/parameter adapters
  -> parameters, HUD, mappings, scenes
```

The helper and source hardware are not Synaptome. The app-facing OSC/device contracts are Synaptome boundary surfaces.

The public promise should be:

```text
If your hardware or software can emit the documented app-facing OSC contract, Synaptome can see it without importing your implementation source.
```

## Public Repo Boundary

The eventual public Synaptome repo should be shaped around these folders or packages:

| Boundary | Belongs In Public Synaptome | Notes |
| --- | --- | --- |
| Runtime app | Yes | openFrameworks app, project files, app-native tests. |
| Layer SDK | Yes | `Layer`, `LayerFactory`, layer authoring docs, examples. |
| Parameter core | Yes | Registry, modifiers, vocabulary, manifest tooling. |
| User-facing surfaces | Yes | Browser, Console, HUD, Device Mapper, overlay shell. |
| App contracts | Yes | Schemas, examples, fixtures, validation commands. |
| External contracts | Yes | MIDI, OSC, helper input, audio, video, display, and media boundary docs. |
| Artist SDK | Yes | Layer authoring surface, parameter vocabulary, extension packaging rules, examples. |
| Example scenes | Optional | Keep as examples/content packs, not the core identity. |
| Helper packages | Separate repos | Should depend on app-facing OSC/radio contracts. |
| Firmware devices | Separate repos | Should produce contract-compatible data. |
| Radio contract | Separate package or shared dependency | Current `common/` candidate. |
| Governance | Separate or internal docs | Not public app value unless generalized. |

## Main Runtime Flows

### Creating A New Layer

Target public flow:

```text
Write a Layer subclass
  -> register its factory type
  -> define a JSON catalog entry
  -> register parameters in setup()
  -> appear in the Browser
  -> load into a Console slot
  -> control via Browser/MIDI/OSC/sensors
  -> save in scenes
```

Current status: mostly implemented, but the authoring guide and parameter vocabulary are not explicit enough.

### Loading Media

Target public flow:

```text
Add media to a watched folder
  -> Synaptome catalogs it
  -> media entry appears in the Browser
  -> load into a Console slot
  -> gain/mirror/loop/playback parameters appear automatically
```

Current status: media layer primitives exist, but cataloging is currently `config/videos.json` based. Folder watch or scan behavior should become an explicit feature.

### Adding A MIDI Controller

Target public flow:

```text
Add device-map JSON
  -> Synaptome recognizes physical roles
  -> bind roles to parameters or slots
  -> save mapping
  -> reuse mapping across scenes
```

Current status: device-map schema, Device Mapper, MIDI router, and Browser Flow validation exist. Golden device-map fixtures still need to be stronger.

### Adding A Sensor Device

Target public flow:

```text
Build any sensor device or software sender
  -> emit the documented app-facing OSC contract
  -> Synaptome learns or routes the source
  -> map the source to parameters/HUD
```

Current status: public app OSC maps and target references are validated. Hardware decode, deployment config, and embedded catalog exchange belong to future helper packages.

### Performing A Scene

Target public flow:

```text
Open Synaptome
  -> load scene
  -> assign eight slots
  -> set BPM
  -> map controls
  -> watch HUD/telemetry
  -> perform with projection and control surfaces separated
  -> save scene/mapping state
```

Current status: strong. Scene load now has staged scene/display transaction handling, manual Control Window smoke, and lazy webcam timing smoke; the remaining hardening is app-native live-window transaction fixtures and display resilience polish.

## What Is Solid Now

- The runtime already has a real eight-slot Console model.
- Layer discovery and creation are data/catalog driven.
- Parameters have metadata, ranges, defaults, and modifiers.
- MIDI/OSC mapping and learning are real, not just planned.
- The Browser is a meaningful operator workbench, even though much of its implementation still lives in `ControlMappingHubState`.
- Device maps already express physical controllers as data.
- HUD widgets and telemetry feeds are structured.
- Global BPM exists as a core parameter and is passed to layers.
- App-facing OSC and target references have public fixtures.
- App-native validation no longer depends on PlatformIO.
- The app can be proven independent of firmware implementation directories.

## What Must Become Explicit

| Need | Why |
| --- | --- |
| Public layer authoring guide | Artists need to know how to make a visual appear in Synaptome. |
| Parameter vocabulary | Reusable scene parameters should feel consistent across layers. |
| Parameter manifest validator | Public parameter IDs need deprecation/version discipline. |
| Media auto-discovery or clear catalog policy | The public workflow should make adding clips easy. |
| Device-map golden fixtures | Controllers should be trusted as data, not code edits. |
| Live-window transaction fixtures and display resilience polish | Loading scenes must continue to avoid destabilizing control/projection windows. |
| Defaults vs local state policy | Public repos need clean committed examples and ignored operator state. |
| Synaptome/helper contract docs | Hardware builders need to know how to feed the runtime. |
| Example content boundary | Scenes and assets should demonstrate the framework without defining it. |

## Naming Map

| Name | Meaning | Current State | Target Use |
| --- | --- | --- | --- |
| `openFrameworks` | Creative coding framework dependency. | External framework plus current app folder naming confusion. | Framework dependency only, not app identity. |
| `Synaptome` | Canonical core app/repo identity. | Implemented locally for app folder, VS solution/project, and executable. | App/runtime/repo identity. |
| scenes/layers | Content hosted by the app. | JSON assets and saved scenes. | Content, not the app name. |

## Extraction Invariants

- Synaptome app validation must not require PlatformIO.
- Synaptome app source must not include firmware implementation directories.
- Helper and firmware repos communicate through app-facing contracts, not private includes.
- New device support should prefer data contracts, OSC routes, and device maps over `ofApp.cpp` edits.
- Generated files need check modes where practical.
- Scenes load through staged apply/publish/rollback and must continue to avoid closing/reopening control windows.
- Every public boundary change requires a contract pin, validation command, compatibility note, and stop condition.

## Next Architecture Work

Implementation sequencing should follow the public gap register in [`docs/contracts/contract_gaps.md`](../contracts/contract_gaps.md).

Near-term architecture-to-implementation children:

1. Parameter manifest and vocabulary.
2. Live-window transaction fixtures and display resilience polish.
3. Layer SDK golden fixture and authoring guide.
4. External contract schemas/fixtures for MIDI, OSC, audio, video, media, and displays.
5. Device-map logical slot fixture.
6. HUD/Console operator-surface validators.
7. Browser boundary and legacy language cleanup.
8. Rename compatibility plan.

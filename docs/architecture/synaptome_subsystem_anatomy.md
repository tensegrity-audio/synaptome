# Synaptome Subsystem Anatomy

Status: Detailed architecture companion. This document expands the high-level Synaptome architecture into subsystem-level definitions, rules, code anchors, relationships, and target gaps.

Read with: [`synaptome_system_architecture.md`](synaptome_system_architecture.md), [`synaptome_external_contracts.md`](synaptome_external_contracts.md), [`synaptome_artist_sdk.md`](synaptome_artist_sdk.md), [`docs/contracts/README.md`](../contracts/README.md), and [`docs/contracts/contract_gaps.md`](../contracts/contract_gaps.md).

## Purpose

This is the "synaptome" of Synaptome: a living map of the app's core concepts and how the code currently realizes them.

The goal is not to document every line literally. The goal is to make every important line of code findable inside a named subsystem, so the project can answer:

- what Synaptome is today,
- which parts are core framework,
- which parts are user-facing surfaces,
- which parts are adapters or contracts,
- which parts are content or show-specific examples,
- which pieces must be stabilized before public release.

## Architectural Spine

The useful mental model is:

```text
Layer declares parameters
  -> slot hosts layer
  -> scene persists slot and parameter state
  -> devices and mappings control parameters
  -> Browser edits and routes the model
  -> Console performs the model
  -> HUD reflects the model
  -> projection renders the model
  -> contracts let outside systems feed the model
```

Everything below should support or clarify that spine.

## Code Boundary Map

| Subsystem | Current Code Anchors | Current Role | Target Boundary |
| --- | --- | --- | --- |
| App shell | `synaptome/src/ofApp.*`, `main.cpp` | Composition root and too much runtime ownership. | Synaptome runtime host, with smaller services extracted internally. |
| Parameter runtime | `src/core/ParameterRegistry.h`, `src/common/modifier.h` | Shared controllable value registry. | Public core contract. |
| Banks | `src/core/BankRegistry.h` | Groups controls by global, scene, and layer scope. | Public control-bank contract. |
| Layer SDK | `src/visuals/Layer.h`, `LayerFactory.*`, `LayerLibrary.*` | Loadable visual/module contract and catalog. | Public layer authoring SDK. |
| Console and slots | `src/ui/ConsoleState.*`, `src/io/ConsoleStore.*`, `ofApp::consoleSlots` | Eight-slot performance deck and persistence. | Public live deck model. |
| Browser | `src/ui/ControlMappingHubState.h`, `src/ui/AssetBrowser.*` | Main operator workbench for parameters, mappings, assets, scenes, HUD, devices. | Public Browser surface; internal legacy names phased down. |
| Device Mapper | `src/ui/DevicesPanel.*`, `bin/data/device_maps/*.json` | Data-driven physical controller role mapping. | Public device-map contract. |
| MIDI adapter | `src/io/MidiRouter.*`, `bin/data/config/midi-map.json` | MIDI targets, learning, banks, takeover, OSC-source persistence. | Replaceable input adapter targeting parameter IDs. |
| OSC adapter | `src/io/OscParameterRouter.*`, `bin/data/config/osc-map.json` | OSC-to-parameter routing. | App-facing OSC contract. |
| Helper input | `src/io/SerialSlipOsc.*`, OSC fixtures | Consumes helper-provided OSC messages. | External helper repo plus stable app contract. |
| HUD and overlays | `src/ui/HudRegistry.*`, `HudFeedRegistry.*`, `overlays/*`, `OverlayManager.*` | Performer telemetry and layout. | Public operator feedback surface. |
| Media runtime | `src/media/VideoCatalog.*`, `VideoClipLayer.*`, `VideoGrabberLayer.*` | Clip/webcam catalog and layer primitives. | Public media extension contract. |
| Scene persistence | `ofApp::encodeSceneJson`, `ofApp::loadScene`, `ConsoleStore`, `docs/schemas/scene.schema.json` | Saves and reloads live state. | Transactional scene contract. |
| Tests and validators | `tools/validate_configs.py`, `tools/validate_parameter_targets.py`, `tools/run_control_hub_flow.py`, `BrowserFlowTest`, `HotkeyTest` | Regression and contract checks. | Public confidence suite independent of firmware builds. |

## Focused Audit Findings

The first subsystem audit pass surfaced these high-priority facts:

- `ofApp` is still the primary composition root and owns most live services. That is acceptable for a working app, but Synaptome's public architecture should name smaller internal services around parameters, scenes, slots, Browser state, adapters, HUD, and persistence.
- Browser is the public term. The implementation still carries legacy names such as `ControlMappingHubState`, `ControlHub`, `ui.hub.visible`, `overlay.hub.visible`, `control_hub_prefs.json`, and `tools/run_control_hub_flow.py`. These are compatibility/internal names, not language for public docs.
- Parameter IDs are the central contract, but duplicate ID enforcement, range semantics, pointer lifetime, deprecation policy, and generated manifests need hardening before public release.
- State boundaries are currently blended. Scene state, Console presentation state, window/display state, local operator preferences, mapping state, and runtime sensor snapshots all exist, but not every one has an explicit contract.
- Adapter contracts are real but drift-prone. MIDI maps, OSC maps, device maps, Serial SLIP frames, helper OSC examples, and schemas need to agree on target IDs, payload types, persistence ownership, and validation commands.
- Public packaging should treat runtime-written files such as local preferences, generated controller captures, transient scenes, webcam/device inventories, and show logs as local state unless they are explicitly promoted to examples.

## Core Model

### Parameters

What it is:
- A parameter is a stable, typed, addressable runtime value.
- Parameters are the primary shared language between layers, scenes, Browser controls, MIDI, OSC, device maps, HUD summaries, and persistence.

Current implementation:
- `ParameterRegistry` supports float, bool, and string params.
- Each param has a `Descriptor`: `id`, `label`, `group`, `units`, `description`, range, quick-access flag, and quick-access order.
- The registry stores a pointer to the live value, a default value, and a base value.
- Runtime modifiers can be attached to float and bool params.
- Duplicate IDs are intended to be rejected, but the policy is not yet uniformly enforced across all param types.
- `removeByPrefix()` removes dynamic layer params when a slot unloads.

Current rules:
- IDs must be globally unique inside the registry.
- IDs are dot-path strings such as `transport.bpm`, `ui.hud`, `effects.crt.coverage`, or `console.layer3.alpha`.
- Dynamic slot params should use `console.layerN.*`.
- Layer-local params should be registered using the layer registry prefix, then rewritten to the slot prefix when the layer is hosted.
- Scenes should persist base values, not arbitrary UI row state.
- Browser, MIDI, OSC, HUD, and device maps should target parameter IDs rather than private C++ fields.

Current reusable parameter families:

| Family | Common IDs / Suffixes | Meaning |
| --- | --- | --- |
| Transport | `transport.bpm`, `globals.speed` | Shared time and animation rate. |
| Visibility | `.visible`, `ui.hud`, `ui.console.visible`, `ui.hub.visible`, `ui.menu.visible` | Whether runtime surfaces or layers are visible. |
| Slot mix | `console.layerN.opacity`, `console.layerN.coverage` | Per-slot contribution and routing. |
| Timing | `.speed`, `.bpmSync`, `.bpmMultiplier`, `.paused` | Time behavior inside layers. |
| Transform | `.scale`, `.rotationDeg`, `.xBias`, `.yBias`, `.orbitRadius`, `.rotateX`, `.rotateY`, `.rotateZ` | Spatial control. |
| Color | `.colorR`, `.colorG`, `.colorB`, `.alpha`, `.bgColorR`, `.trailAlpha` | Visual appearance. |
| Sensor inputs | `.xInput`, `.yInput`, `.speedInput`, BioAmp sensor metrics | Modulation entry points. |
| Media | `.clip`, `.gain`, `.mirror`, `.loop`, `.device` | Video/webcam/image behavior. |
| Effects | `effects.*.route`, `effects.*.coverage`, `effects.*.coverageMask` | Post-effect routing and controls. |
| HUD/window | `hud.*`, `console.secondary_display.*`, `console.dual_display.mode` | Operator feedback and window state. |

Relationships:
- Layers declare parameters during `setup(ParameterRegistry&)`.
- Browser turns registry entries into rows and categories.
- MIDI maps and OSC maps store target parameter IDs.
- Device maps bind physical roles to parameter targets.
- Scenes persist parameter base values.
- HUD can summarize parameter state and feed updates.

Current gaps:
- Generated manifest and strict target validation now cover current public scene, MIDI, OSC, HUD, and device-map fixtures (`CG-01`); expand as new target-bearing contracts are promoted.
- A first public parameter vocabulary exists and is exercised by the artist SDK fixture; broader vocabulary/range enforcement remains follow-up (`CG-12`).
- Rename/deprecation rules for public parameter IDs are not defined.
- Some core params still carry legacy names such as `ui.hub.visible`; public language should be Browser, but compatibility must be planned.
- Range semantics are inconsistent: descriptor ranges exist, final modifier output is clamped, but direct base writes are not always range-enforced.
- The registry stores raw pointers to owner values, so layer unload/load and service lifetime rules must remain disciplined.
- String params have no modifier stack, so public docs should not imply every parameter type supports every mapping behavior.

Target:
- The generated parameter manifest should keep listing ID, type, group, label, range, owner, source file/catalog, and deprecation status where available.
- The public parameter vocabulary should become enforceable through layer SDK fixtures and semantic validators.
- Parameter ID changes should require compatibility notes and migration tooling.

### Modifiers

What it is:
- A modifier is a runtime transform that lets external inputs reshape a parameter without replacing the parameter contract.

Current implementation:
- `ParameterRegistry::RuntimeModifier` wraps `modifier::Modifier`.
- Supported blend modes include additive, absolute, scale, clamp, and toggle.
- Modifiers track input value, normalized input, before/after values, active/enabled state, clamp flags, and conflicts.

Rules:
- Modifiers apply to float and bool params.
- Base value remains distinct from modified live value.
- Multiple hard-setting modifiers can conflict.
- Modifiers should be owned by a source tag when possible so mappings can be cleared safely.

Relationships:
- OSC and MIDI routes can act like modifier sources.
- Browser should expose enough modifier state for operators to understand why a value moved.
- Scene persistence must decide whether it stores modifier definitions, current inputs, or only mappings.

Current gaps:
- Modifier ownership and persistence are not fully documented as a public contract.
- Parameter manifest should include modifier-capable targets.
- "Modifier" currently means more than one thing: `ParameterRegistry` runtime modifier stacks, MIDI route metadata, OSC route mappings, and scene mapping snapshots are related but not identical.
- Bool OSC and direct write paths do not always have the same conflict/status visibility as float modifier paths.

Target:
- Public docs should separate parameter modifiers, MIDI bindings, OSC routes, device roles, and scene mapping snapshots.
- Browser should make active modifier sources visible enough that a performer can explain why a parameter changed.

### Banks

What it is:
- A bank is a scoped collection of controls that changes which mappings or controls are active in a given performance context.

Current implementation:
- `BankRegistry` stores bank definitions and parent/child relationships.
- Default banks are configured in `ofApp`.
- Bank scope currently covers global, scene, and layer-oriented control groups.

Rules:
- A child bank can override a parent control by ID.
- Parent resolution is cycle-protected.
- Lookup should prefer the most specific active context before falling back to global controls.
- Empty or missing bank IDs must resolve safely during startup and scene load.

Relationships:
- Browser and mappings use banks to decide which controls are relevant.
- MIDI mappings can be bank-scoped.
- Scenes may persist or restore bank definitions and active-bank state.
- Layer authors should be able to declare useful bank groupings without editing router code.

Current gaps:
- Public bank authoring rules are not documented.
- Bank scope and layer-key naming conventions need a stable vocabulary.
- Bank definitions are not yet fixture-backed as a public contract.

### Scenes

What it is:
- A scene is a persisted snapshot of a performance state.

Current implementation:
- `ofApp::encodeSceneJson()` writes scene JSON.
- `ofApp::loadScene()` applies scene JSON.
- `docs/schemas/scene.schema.json` is permissive G0 validation.
- Saved scenes live under `synaptome/bin/data/layers/scenes/`.
- `scene-last.json` stores latest runtime state.

A scene currently covers:
- global parameters,
- console slots,
- per-slot layer/effect parameters,
- MIDI and OSC maps,
- HUD visibility/layout state,
- secondary display/window state,
- sensor snapshots in console presentation state,
- scene metadata/source/storage fields.

Rules:
- Scene loading must not expose half-applied live state to the Control Window or projection.
- Scene schema should follow runtime behavior, not the other way around.
- Scene files should target stable parameter IDs and layer asset IDs.

Relationships:
- Scenes restore Console slots.
- Slots instantiate Layers.
- Layers register Parameters.
- Parameters receive saved base values.
- Browser lists, loads, saves, and overwrites scenes.
- HUD and window state are part of scene safety, not decorative extras.

Current gaps:
- Scene load transaction implementation exists through staged apply/publish/rollback (`CG-03`, `CG-06`).
- Round-trip/live-window fixtures are missing.
- Window snapshot semantics are snapshot-validated but not fully contracted as scene-local vs operator-local policy.
- Scene persistence is partly generic and partly hard-coded through known fields and compatibility branches.
- Stale parameter or layer IDs can fail quietly because the current schema is permissive.
- Slot-scoped parameter IDs depend on slot number, so moving content across slots can change the mapping target identity.

Target:
- Scene load stages, validates, applies/publishes, and rolls back on failure.
- Scene round-trip fixtures should prove load/save compatibility.
- Scene schema should tighten as fixture coverage and public scene policy mature.

### Layers

What it is:
- A layer is a modular openFrameworks visual or runtime module that can be loaded into a slot.

Current implementation:
- `Layer` defines `configure`, `setup`, `update`, `draw`, `onWindowResized`, `setExternalEnabled`, and `isEnabled`.
- `LayerUpdateParams` passes `dt`, `time`, `bpm`, and global `speed`.
- `LayerDrawParams` passes camera, viewport, time, beat, and slot opacity.
- `LayerFactory` maps `type` strings to C++ creators.
- `LayerLibrary` loads JSON catalog entries.

Layer catalog entry shape:
- `id`,
- `label`,
- `category`,
- `type`,
- `registryPrefix`,
- `defaults`,
- optional coverage metadata,
- optional HUD widget metadata.

Rules:
- A new layer should not require edits throughout the app loop.
- A layer must register its public controls through `ParameterRegistry`.
- A layer's parameters must be stable enough to save in scenes and target from MIDI/OSC.
- A layer should receive show-specific defaults through JSON, not hard-coded one-off logic.
- A layer should not depend on firmware implementation details.

Relationships:
- Browser discovers layer catalog entries.
- Console slots host layer instances.
- LayerFactory creates the C++ instance for an entry's `type`.
- LayerLibrary supplies defaults and metadata.
- ParameterRegistry exposes layer controls.
- Scenes persist the resulting slot and parameter state.

Current gaps:
- No public layer authoring guide yet.
- Golden static layer-library ingestion and minimal public SDK example fixtures now exist (`tools/layer_catalog_regression.py`, `tools/validate_artist_sdk_example.py`); package/no-source-edit extension loading remains follow-up (`CG-16`).
- Some content-specific layers and assets need to be separated from core examples.
- Browser offline hydration can instantiate assets to inspect metadata, so layer setup must avoid heavy side effects where possible.
- `LayerFactory::registerType()` rejects empty or duplicate type IDs; public package discovery still needs operator-facing diagnostics.
- FX and UI/HUD assets are not yet authored through one completely uniform layer extension path.
- Default scenes contain some legacy/stale IDs that should be migrated before public examples are published.

Target:
- Public docs should make "write a Synaptome layer" straightforward.
- Example layers should demonstrate the framework without defining its identity.
- Layer catalog and SDK fixtures now prove representative built-in assets plus one authored example resolve into stable catalog, scene, and target entries.

### Slots

What it is:
- A slot is a fixed live-performance container in the Console.

Current implementation:
- The Console currently has eight slots.
- `ConsoleState::kSlotCount = 8`.
- `ofApp::consoleSlots` owns live slot runtime state.
- `ConsoleStore` persists slot inventory and presentation state.

Slot state includes:
- index,
- asset ID,
- label/display name,
- active state,
- opacity,
- coverage mode/columns,
- layer instance,
- layer registry prefix,
- associated params and mappings.

Rules:
- Slots are 1-based for public/operator identity.
- Slot parameter IDs should use `console.layerN.*`.
- Slot assignment should be data-driven through asset IDs.
- Loading/unloading a slot should add/remove parameter prefixes cleanly.
- Slot state should survive scene save/load.

Relationships:
- Console is the user-facing deck for slots.
- Browser can assign assets to slots and show slot inventory.
- Scenes persist slot assignments.
- Effects can route over slot coverage.
- MIDI/OSC mappings can target slot params.

Current gaps:
- Console layout and secondary display persistence are now snapshot-validated by `tools/validate_console_layout_contract.py --check` (`CG-06`).
- Slot/window state now participates in scene-load transaction stability; live-window fixture coverage remains follow-up.

Target:
- Slot assignment, parameter hydration, coverage, and display state should be transaction-safe and fixture-backed.

### ConsoleStore And Presentation State

What it is:
- ConsoleStore is the persistence boundary for live-deck state and operator presentation state that survives app launches or scene changes.

Current implementation:
- `ConsoleStore` persists console assignments, overlays, dual-display settings, secondary display data, controller focus, overlay layouts, and sensor snapshots.
- It writes through a temporary file and rename path.
- It currently accepts some compatibility migrations, including legacy overlay IDs.

Rules:
- ConsoleStore state is broader than slot inventory; it also captures how the operator sees and manages the show.
- Presentation state should not be confused with layer content.
- Runtime-local fields should be explicitly classified before a public repo publishes default config.
- Loaders should accept older compatible fields, but writers should emit the current version.

Relationships:
- Scenes and ConsoleStore overlap around slot assignments, window/layout state, HUD state, and sensor snapshots.
- Browser and Console both mutate parts of this state.
- Scene-load transaction work now stages scene-owned state; docs still need to lock which display pieces are scene-owned vs local.

Current gaps:
- `config/console.json` now has a strict static snapshot validator for stable persisted shape.
- Some fields still use legacy Browser implementation language such as `controlHubVisible`.
- The file has broad responsibilities, which makes public boundary language harder to explain.

### Devices

What it is:
- A device is an external control source such as MIDI hardware, OSC sender, helper sensor, hotkey source, or future control surface.

Current implementation:
- MIDI device behavior is represented by `MidiRouter`.
- Logical device maps live under `bin/data/device_maps/*.json`.
- `DevicesPanel` exposes the Device Mapper.
- OSC sources are tracked by `MidiRouter::OscSourceInfo` and `OscSourceProfile`.
- Helper/radio devices are external and should speak contracts into Synaptome.

Rules:
- Device support should start with data contracts, not source edits.
- Physical controls should map to logical roles.
- Logical roles should map to parameter IDs or slot actions.
- Device identity and port hints should be separate from show-specific bindings.

Relationships:
- Device maps describe hardware layout.
- Device Mapper edits role labels, sensitivity, and MIDI bindings.
- Browser uses device slots as mapping candidates.
- MIDI/OSC adapters deliver values into mappings and params.
- HUD can show device and sensor status.

Current gaps:
- Device-map logical slot fixture is now covered by `tools/device_map_regression.py --check` (`CG-04`).
- Device schema is permissive and needs stronger examples.

Target:
- A new controller should be addable by JSON map plus mapping, without touching `ofApp.cpp`.

### Mappings

What it is:
- A mapping connects an input source to a parameter or slot role.

Current implementation:
- `MidiRouter::CcMap` maps MIDI CC to float targets.
- `MidiRouter::BtnMap` maps MIDI notes/buttons to bool/action targets.
- `MidiRouter::OscMap` maps OSC patterns to targets.
- `BindingMetadata` carries channel, control ID, slot ID, device ID, and column ID.
- Browser exposes mapping columns and learn/edit flows.

Rules:
- Mapping targets should be stable parameter IDs or declared role IDs.
- Learn flows should produce persistent config, not hidden runtime-only behavior.
- Mapping ranges should preserve the meaning of the target parameter.
- Mappings should survive scene reload when appropriate.

Relationships:
- Browser shows mapping state beside parameters.
- Device Mapper creates/edits hardware role metadata.
- MIDI/OSC routers execute mappings at runtime.
- Scenes and config files persist mappings.

Current gaps:
- Mapping ownership between global config and scene-specific state needs explicit policy.
- The generated parameter manifest and strict target validator now give mapping stability a hard baseline for current persisted files.
- `MidiRouter` also owns OSC map/source profile persistence today, which blurs MIDI adapter and OSC adapter ownership.
- Mapping target existence is mostly checked at runtime, not as a semantic contract validation pass.
- MIDI map persistence lacks a strict public schema and versioning story.

Target:
- Mapping docs should distinguish device maps, MIDI map persistence, OSC source profiles, and scene-local mappings.

### Transport

What it is:
- Transport is shared time and tempo state for layers and effects.

Current implementation:
- `transport.bpm` is registered as a parameter.
- `globals.speed` is registered as a parameter.
- Layers receive `bpm`, `speed`, `time`, and beat data through update/draw params.

Rules:
- Transport values should be globally available and targetable.
- Layers should use transport inputs instead of inventing private BPM fields when possible.
- Future clock sources should drive the same transport model.

Relationships:
- Browser edits BPM and speed.
- MIDI/OSC can target transport params.
- Layers and effects consume transport in update/draw.
- HUD can display transport state.

Current gaps:
- Tap tempo, external clock, MIDI clock, and Link are future work.
- Transport contract should define phase/beat semantics, not just BPM.

Target:
- Transport should become a public runtime service.

## User-Facing Surfaces

### Browser

What it is:
- The Browser is the main operator workbench.
- It surfaces the core model: parameters, scenes, layers, slots, media, mappings, devices, HUD placement, and telemetry.

Current implementation:
- Public term: Browser.
- Internal implementation anchor: `ControlMappingHubState`.
- Asset picker anchor: `AssetBrowser`.
- Browser uses `MenuController::State`.
- Browser model contains parameter rows, tree nodes, categories, column visibility, slot options, HUD preferences, saved scene rows, and layout state.

Browser rules:
- Browser should not own the core model; it should inspect and mutate core services through explicit callbacks and registries.
- Browser rows should be derived from parameter, asset, scene, device, and HUD sources.
- Browser must use public language: Browser, Console, HUD. `Control Mapping Hub` and `CMH` are legacy/internal.
- Browser state such as tree expansion and visible columns is operator preference, not scene content unless explicitly contracted.
- Browser should make learn/edit flows visible and reversible.

Browser relationships:
- Reads `ParameterRegistry`.
- Reads `LayerLibrary`.
- Calls slot load/unload callbacks.
- Calls scene load/save callbacks.
- Reads/writes MIDI/OSC mapping state through routers.
- Reads/writes HUD visibility and placement.
- Emits telemetry through event callbacks.

Current gaps:
- Browser implementation is still a very large single header.
- The public Browser rules are not yet extracted into a guide.
- Some legacy config names still say `control_hub`.
- Browser currently owns or coordinates many behaviors at once: parameter rows, category trees, scene rows, layer/asset browsing, MIDI/OSC learn, Device Mapper flows, Console slot operations, HUD widget placement, telemetry, and sensor events.
- `AssetBrowser` is only the asset picker, not the whole Browser. Public language should avoid implying that the picker is the complete workbench.
- The internal third band is still named `workbench`; public docs should consistently call it the Browser band.

Target:
- Browser should become a clear surface over services, not a subsystem that owns too much behavior.

### Console

What it is:
- The Console is the live performance deck.

Current implementation:
- `ConsoleState` renders and navigates eight slots.
- `Ctrl+1..8` opens the asset picker for a slot.
- `Ctrl+U` unloads the focused slot.
- It displays slot labels, active/assigned state, opacity, and parameter previews.

Console rules:
- Console should answer "what is loaded and active?"
- Console should not be a general settings page.
- Console controls slots; Browser handles deeper inspection and mapping.
- Console must remain stable during scene load.

Relationships:
- Console reads slot state from `ofApp`.
- Console opens `AssetBrowser`.
- Console previews parameter activity through `ParameterRegistry`.
- Scenes and ConsoleStore persist slot state.

Current gaps:
- Console layout and secondary display persistence are contract-backed for current stable fields.
- Scene-load stability is covered by transaction work; live-window fixture coverage and resilience polish remain follow-up.

Target:
- Console should be a small, predictable live-performance surface with explicit slot semantics.

### HUD

What it is:
- The HUD is performer feedback.

Current implementation:
- `HudRegistry` owns HUD toggles and widget descriptors.
- `HudFeedRegistry` stores structured latest-feed payloads.
- `OverlayManager` places widgets in columns and bands.
- HUD widgets include controls, layers, status, sensors, telemetry, and menu mirror.

HUD rules:
- HUD displays the model; it should not become the model.
- HUD feed IDs should be stable if they are public.
- HUD layout is runtime-visible and may be scene/window state.
- HUD should not pollute projection output unless intentionally displayed.

Relationships:
- HUD reads layer, slot, sensor, and route state.
- Browser edits HUD visibility and placement.
- Scene persistence may snapshot HUD/window layout.
- Helper/sensor input can publish HUD telemetry.

Current gaps:
- HUD widget identity, declared feed IDs, preferences, and overlay layout snapshots are now covered by `tools/validate_hud_layout_contract.py --check` (`CG-05`).
- Window/display snapshots are covered by `tools/validate_console_layout_contract.py --check`, and scene loads now use staged apply/publish/rollback; an app-native live-window transaction fixture remains follow-up.
- HUD feed aliases and widget IDs should be normalized before they become public contract names.
- HUD persistence needs a clear split between scene-facing layout and local operator preference.

Target:
- HUD layout/feed state should be schema-backed and transaction-safe.

### Projection And Control Window

What it is:
- Projection is audience output.
- Control Window is operator output.

Current implementation:
- `ThreeBandLayoutManager` calculates HUD, Console, and workbench bands.
- Public language maps internal `workbench` to Browser.
- Secondary display state exists in console/store/app runtime.

Rules:
- Projection should be clean output.
- Control Window should contain HUD, Console, and Browser.
- Window state should be persisted without destabilizing scene load.

Current gaps:
- Manual Control Window smoke has passed; live close/reopen regression coverage remains follow-up.
- Secondary display snapshot shape is covered by `tools/validate_console_layout_contract.py --check`.
- Some persisted flags still use `controlHubVisible` and related legacy names even though the public surface is Browser.

Target:
- Window state is applied at a bounded scene/display transaction point.

## Adapters And Contracts

### MIDI

What it is:
- MIDI is an input adapter into the core model.

Current implementation:
- `MidiRouter` supports CC maps, button maps, learn, bank activation, soft takeover, captured controls, and persistence.

Rules:
- MIDI should target parameter IDs or logical control roles.
- Hardware ports and device maps should be distinct from target parameters.
- Soft takeover should prevent surprise value jumps.

Current gaps:
- Public mapping lifecycle needs documentation.
- The generated parameter manifest and strict target validator give MIDI targets a stability baseline for current persisted maps.
- MIDI target strings are plain parameter IDs today; a public validator should verify those targets exist.
- Soft takeover, banks, learn mode, button actions, and target metadata need public examples.

### OSC

What it is:
- OSC is an input adapter into parameters, sensor telemetry, and app state.

Current implementation:
- `OscParameterRouter` maps OSC patterns to float and bool routes.
- `MidiRouter` also stores OSC maps/source profiles for Browser editing.
- App-facing OSC fixtures validate route and mapping behavior.

Rules:
- OSC addresses should be contract-backed when public.
- OSC maps should target stable parameter IDs.
- Helper UI/catalog packets are outside Synaptome unless a future public helper package exposes them as app-facing OSC.

Current gaps:
- App OSC map fixture coverage is partial.
- App OSC catalog should become machine-readable.
- `config/osc-map.json` runtime shape and `docs/schemas/osc_map.schema.json` are not yet fully aligned.
- Built-in route defaults and external sensor metric names need a single source of truth.
- OSC mapping ownership is split between `OscParameterRouter` execution and `MidiRouter` Browser/source-profile persistence.

### Serial SLIP And Helper Input

What it is:
- Serial SLIP input is the app-side bridge for helper-originated OSC frames.
- It lets hardware/helper repos remain outside Synaptome while still feeding sensors, parameters, and telemetry into the app.

Current implementation:
- `SerialSlipOsc` parses SLIP-wrapped OSC frames on the host.
- Public fixtures validate representative app-facing OSC route behavior.
- Helper output can include sensor, system/control, and parameter-related OSC namespaces when documented by the app contract.

Rules:
- Synaptome should care about app-facing OSC, not private hardware transport details.
- Fixture expectations should match the helper's public app-facing output.
- Host parser support must cover every OSC payload type the helper contract promises.
- Hardware transport constants should live in a helper or radio-contract package, not the public app runtime.

Relationships:
- App-facing OSC can feed `OscParameterRouter`, HUD telemetry, Device Mapper source rows, and parameter modifiers.
- Contracts and fixtures are the boundary between Synaptome and helper/hardware repos.
- A hardware builder should be able to implement the documented app contract without reading Synaptome internals.

Current gaps:
- Host-side parser regression coverage is missing for complete Serial SLIP frame parsing.
- String OSC payloads from future helper packages need explicit host parser verification before becoming public app-facing payloads.
- `docs/osc_catalog.md` is the current prose catalog; a generated route catalog is still future work.
- Hardware transport constants belong in a helper or radio-contract package rather than this public app runtime.

### Schemas, Fixtures, And Validators

What it is:
- Validation is the contract layer that keeps Synaptome public and safe.

Current implementation:
- `tools/validate_configs.py --public-app` reports public app contract status.
- `tools/validate_parameter_targets.py --strict --contract-fixtures` checks target-bearing public fixtures.
- Layer, scene, HUD, Console, device-map, and artist SDK validators cover the promoted public fixtures.
- App-native Browser Flow and Hotkey tests exist.

Rules:
- App validation must not require PlatformIO.
- Helper/firmware validation may require its own tooling outside the public app runtime.
- Every extraction or rename should name its contract pin and validation commands.

Current gaps:
- Runtime scene/display transaction implementation, manual Control Window smoke, and lazy webcam timing smoke are in place; app-native staged apply/rollback fixtures with live windows remain follow-up.
- Existing validators shape-check and target-check the promoted public fixtures; coverage should expand as new media, hotkey, display, and external-route files become public contracts.
- Some schemas drift from runtime files, including OSC maps, MIDI maps, and hotkeys.
- App-native BrowserFlow is strong internal regression, but its direct `.cpp` inclusion and private-access test style make it less useful as a public contract test.

### Tests And Harnesses

What it is:
- Tests and harnesses are the confidence layer that keeps Synaptome usable before live performance and before public packaging.

Current implementation:
- Python validators cover config and contract shape checks.
- Public validators cover the app OSC map and target references.
- BrowserFlow exercises many internal app flows without requiring PlatformIO.
- Hotkey tests cover command routing and key mapping behavior.

Rules:
- App validation must stay independent from PlatformIO and firmware build tools.
- Helper/firmware validation may use its own tooling, but it should not be required to prove the app runtime.
- Every public contract should have either a schema fixture, a regression fixture, or an app-native harness.
- Tests should use public seams where possible; private/internal harnesses are allowed for coverage but should not be the only public proof.

Current gaps:
- Direct host-side parser tests for `SerialSlipOsc` frames and string payloads are missing.
- BrowserFlow still exposes implementation coupling.
- Hotkey schema/runtime naming has drifted in places.
- Scene persistence, HUD layout/feed, Console layout/window, layer catalog, artist SDK, and device-map fixtures are now part of the current public contract bundle; media onboarding and direct host parser fixtures remain follow-up.

## Content And Extensions

What it is:
- Scenes, show assets, effects, models, videos, and presets are content built on Synaptome.

Rules:
- Content should demonstrate Synaptome without defining Synaptome.
- Content should use public layer, parameter, media, and scene contracts.
- Show-specific assets should be optional examples or packs in a public repo.

Current content categories:
- Generative layers.
- Geometry layers and STL assets.
- Media layers.
- HUD widget entries.
- FX entries.
- Saved scenes.
- Text utility layer.

Target:
- Public Synaptome should include minimal example content plus clear extension docs.

## Current vs Target Summary

| Area | Current | Target | Next Strengthening Step |
| --- | --- | --- | --- |
| Parameters | Real registry, implicit vocabulary, raw pointer ownership. | Manifested, versioned public vocabulary with explicit range and lifetime rules. | Draft parameter vocabulary and generator. |
| Modifiers | Real float/bool modifier stacks, mixed route semantics. | Clear distinction between modifiers, mappings, bindings, and snapshots. | Document modifier ownership and persistence. |
| Banks | Global/scene/layer bank registry exists. | Public bank authoring and scoping contract. | Add bank fixture and naming rules. |
| Layers | Real SDK and fixture-backed catalog. | Public authoring guide and minimal SDK example. | Polish layer authoring guide and package/no-source-edit seam. |
| Scenes | Real staged save/load, permissive schema, partly hard-coded load rules. | Transaction-safe, round-trip tested scene contract. | Expand round-trip/live-window fixtures and tighten scene schema policy. |
| Slots | Eight-slot live deck works with static layout/window fixtures. | Stable slot contract with live-window fixture coverage. | Keep console/window snapshot validator current; add live-window fixture coverage. |
| ConsoleStore | Broad persistence for slots, overlays, windows, sensors. | Explicit split between scene state, local state, and presentation state. | Add console state schema and migration policy. |
| Browser | Powerful but large legacy implementation. | Public Browser over explicit services with legacy names hidden. | Write Browser rules and split service boundaries. |
| Console | Usable performance deck. | Small, predictable live surface over slot state. | Keep slot actions contract-backed. |
| HUD | Registry, feeds, overlays exist. | Schema-backed feedback/layout contract. | Baseline implemented with `tools/validate_hud_layout_contract.py --check`; future work can add feed payload schemas. |
| Devices | Device maps exist and logical slots are fixture-backed. | New controllers are data-first. | Extend fixture when target/action bindings become public. |
| MIDI/OSC | Real routing and learn flows, ownership split. | Public mapping lifecycle and route schemas. | Tie mappings to parameter manifest. |
| Serial/helper input | App-facing OSC examples exist. | Separate helper repo speaks a machine-readable app contract. | Add host parser regression and canonical OSC catalog. |
| Media | Clip/webcam layers and `videos.json`. | Clear discovery policy. | Decide manifest vs folder scan. |
| Tests | Strong internal harnesses and validators. | Public confidence suite independent of firmware builds. | Add semantic validators and public fixtures. |

## How This Guides Future Work

Before renaming or extracting anything, each change should answer:

1. Which core model concept does it touch?
2. Which user-facing surface exposes it?
3. Which adapter or contract feeds it?
4. Which content/example depends on it?
5. What is the validation command that proves the boundary still works?

That question set is the practical boundary between "Synaptome as a reusable framework" and "current private show/app contents."

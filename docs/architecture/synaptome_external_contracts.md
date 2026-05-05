# Synaptome External Contracts

Status: Architecture draft with first public extraction boundary decisions locked. This document defines the contracts Synaptome should expose to the outside world: MIDI controllers, OSC senders, helper repos, external displays, webcams, microphones, media files, and local runtime devices.

Read with: [`synaptome_system_architecture.md`](synaptome_system_architecture.md), [`synaptome_subsystem_anatomy.md`](synaptome_subsystem_anatomy.md), [`synaptome_artist_sdk.md`](synaptome_artist_sdk.md), and [`docs/contracts/contract_gaps.md`](../contracts/contract_gaps.md).

## Purpose

Synaptome becomes useful as a public runtime only if outside systems can talk to it without learning its private source tree.

The contract rule is:

```text
Outside system emits a documented signal
  -> Synaptome normalizes it into a device, mapping, media, display, or parameter event
  -> Browser, Console, HUD, scenes, and projection respond through the core model
```

The outside system should not need to know about `ofApp.cpp`, `ControlMappingHubState`, slot internals, or firmware-specific headers.

## Contract Design Rules

- Public contracts target stable concepts: parameters, slots, scenes, devices, media entries, transport, HUD feeds, and display state.
- Private implementation names must not leak into public docs except as compatibility notes.
- A new external device should be added through data, mappings, or documented messages before source edits.
- App validation must not require firmware build tools.
- Runtime-written local state is not a public example unless intentionally promoted.
- Every public contract needs an owner, a source file or schema, a fixture or example, and a validation command.

## First Public Extraction Boundary

Decision locked on 2026-04-29:
- The first public `synaptome` repo owns app-facing contracts only: parameter IDs, scenes, Browser/Console/HUD state, MIDI/device maps, app OSC routes, host audio, webcam/media/display adapters, public examples, and public app validators.
- Firmware/helper implementations stay outside Synaptome: ESP-NOW/radio decode, helper firmware, embedded UI/catalog exchange, generated radio headers, private deployment netmaps, and legacy payload quarantine.
- Radio/helper fixtures move to a future helper or `synaptome-radio-contract` package when that package exists. Until then they remain source-workspace evidence, not public Synaptome runtime dependencies.
- Public Synaptome may include sample OSC senders or captured app-facing OSC messages, but only after they no longer require firmware headers, generated radio config, deployment MACs, or helper source trees.

## Contract Map

| Outside World | Current Entry Point | Core Target | Public Contract We Need | Current Gap |
| --- | --- | --- | --- | --- |
| MIDI controllers | `MidiRouter`, `device_maps/*.json`, `midi-map.json` | Parameters, banks, slots, mappings | Device map plus MIDI mapping schema, strict target validation, learn lifecycle | Logical slots are fixture-backed; target/action binding semantics are still future policy; OSC metadata is mixed into MIDI router. |
| OSC senders | `OscParameterRouter`, `osc-map.json`, Browser OSC source rows | Parameters, modifiers, telemetry | Machine-readable OSC route catalog and target parameter manifest | Runtime config/schema drift and partial fixture coverage. |
| Helper repos | `SerialSlipOsc`, app OSC examples | Sensors, parameters, HUD telemetry | App-facing OSC contract independent of helper source | Host parser fixture gaps and route-catalog generation remain unresolved. |
| Microphones | `AudioInputBridge`, `config/audio.json` | Sensor metrics, OSC routes, parameter modifiers | Host audio sensor contract for level/peak and optional OSC publish | No explicit app contract entry/schema/fixture for audio config. |
| Webcams | `VideoGrabberLayer`, `media.webcam` asset | Media layer params, HUD layer metadata | Video input device contract and fallback policy | Device inventory is runtime log; no validator for device selection behavior. |
| Video/media files | `VideoCatalog`, `config/videos.json`, media layers | Media entries, layer params | Media discovery policy: manifest, folder scan, or both | Desired drop-in workflow is not implemented as a contract. |
| External monitors/projectors | `ThreeBandLayout`, secondary display params, `ConsoleStore` | Projection, Control Window, HUD/Console/Browser layout | Display/window state schema and transaction semantics | Console/display snapshot validation and staged scene/display transaction helpers are in place; app-native live-window fixtures and public monitor schema remain follow-up. |
| Hotkeys/keyboards | `HotkeyManager`, `KeyMappingUI` | Commands, surfaces, mappings | Command/action vocabulary and keymap schema | Schema/runtime field naming drift remains. |

## MIDI Controller Contract

What it should promise:
- A controller can be described as data.
- Physical controls map to logical roles.
- Logical roles map to parameter IDs, slot roles, or declared actions.
- Artists can bind controls in the Browser and reuse the map across scenes.

Current implementation anchors:
- `synaptome/src/io/MidiRouter.h`
- `synaptome/src/io/MidiRouter.cpp`
- `synaptome/src/ui/DevicesPanel.*`
- `synaptome/bin/data/device_maps/*.json`
- `synaptome/bin/data/config/midi-map.json`
- `docs/examples/midi_bank_example.json`

Contract fields that should become explicit:

| Field | Meaning |
| --- | --- |
| Device identity | User label, optional port hint, manufacturer/model hints, local alias. |
| Physical control | MIDI channel, CC/note/button number, input type, sensitivity, pickup behavior. |
| Logical role | Slot, column, knob/fader/button role, human label. |
| Target | Parameter ID, slot role, action ID, or bank control ID. |
| Range | Input min/max, output min/max, invert, deadband/smoothing where supported. |
| Bank | Global, scene, or layer-scoped bank ID. |
| Persistence | Whether binding is global config, scene-local mapping, or operator-local preference. |

Current gaps:
- MIDI/scene/OSC/HUD/device-map targets now pass strict public fixture validation; target/action binding semantics still need policy coverage.
- Device maps validate shape and logical slots, but not enough role semantics.
- `MidiRouter` also persists OSC source/mapping data, which blurs adapter ownership.
- Button/action semantics are narrower than the public language will need.
- Logical slot behavior now has a synthetic controller fixture plus the current MIDI Mix baseline.

## OSC Contract

What it should promise:
- Any OSC sender can address Synaptome through documented routes.
- Routes can drive parameters, modifiers, sensor telemetry, HUD feeds, or declared app actions.
- Route targets are stable parameter IDs or declared event names.

Current implementation anchors:
- `synaptome/src/io/OscParameterRouter.*`
- `synaptome/bin/data/config/osc-map.json` as a documented/validated config artifact
- `docs/examples/osc_map_example.json`
- `synaptome/src/ofApp.cpp` dynamic route rebuild and ingest code
- `synaptome/bin/data/config/osc-channels.txt` as source hint/sample address input

Contract fields that should become explicit:

| Field | Meaning |
| --- | --- |
| Address pattern | OSC address or wildcard pattern. |
| Payload type | Float, int, bool, string, or bundle support where applicable. |
| Source profile | Optional source identity such as helper, host mic, touch controller, or software sender. |
| Target | Parameter ID, modifier target, HUD feed, sensor metric, or action ID. |
| Range mapping | Input/output range, clamp, smoothing, deadband, relative-base behavior. |
| Ownership | Built-in route, global config route, scene route, or transient learn route. |

Current gaps:
- The audited runtime path appears to rebuild dynamic routes from Browser/MidiRouter OSC maps and built-ins; standalone `config/osc-map.json` is validated but not yet clearly loaded as the active startup source.
- No UDP OSC receiver contract was confirmed in the current audit; current helper ingest is Serial SLIP OSC and local audio can publish OSC outward.
- `config/osc-map.json` and `docs/schemas/osc_map.schema.json` do not fully describe the same shape.
- Built-in route defaults and external sensor metric names need a single source of truth.
- Bool and string handling needs the same contract clarity as float routes.
- OSC mapping execution and OSC source persistence are split between `OscParameterRouter` and `MidiRouter`.
- A machine-readable OSC route catalog is not yet generated.

## Helper Repo Contract

What it should promise:
- Synaptome consumes app-facing OSC, not firmware implementation details.
- A helper repo can decode any hardware protocol and emit the Synaptome app contract.
- The helper can live in a separate repo as long as fixtures prove the app-facing output.

Current implementation anchors:
- `synaptome/src/io/SerialSlipOsc.*`
- `docs/osc_catalog.md`
- `docs/examples/osc_map_example.json`
- `synaptome/src/io/SerialSlipOsc.*`
- `synaptome/src/io/OscParameterRouter.*`
- Future helper package fixtures for hardware decode and app-facing OSC output

External producer compatibility:

- `synaptome-mesh-osc v0.1.0` is an external producer contract owned by Synaptome Mesh / Tensegrity.
- Synaptome should treat it like any other OSC-producing helper: compatible through app-facing OSC messages, not through firmware headers, generated radio config, PlatformIO projects, or gateway source.
- The current Synaptome Mesh compatibility profile is optional and numeric-only. It maps selected mesh output events into Synaptome-facing routes; it is not a required runtime dependency.
- Equivalent OSC from TouchDesigner, Max/MSP, Python scripts, or other hardware bridges should remain valid when it targets the same Synaptome app-facing routes.

Contract boundary:

```text
Hardware/helper repo owns:
  hardware sensor reads
  ESP-NOW/radio transport
  helper decode
  embedded UI/catalog details

Synaptome owns:
  app-facing OSC addresses
  route-to-parameter semantics
  source identity in Browser/HUD
  fixtures proving expected app-visible messages
```

Current gaps:
- Host-side `SerialSlipOsc` parser needs direct regression coverage for all promised payload types; the current host ingest path is confirmed around float and int payloads, while helper output may include string OSC.
- Helper string OSC output and host parser support need explicit alignment before strings are public app-facing payloads.
- `docs/osc_catalog.md` should eventually be generated from the active app-facing route catalog.
- Hardware decode schemas are shared contract candidates, not Synaptome runtime source.

## Host Microphone Contract

What it should promise:
- A host microphone can become a local sensor source.
- Microphone metrics can be mapped the same way as external sensor metrics.
- Local audio capture can optionally publish OSC for other tools.

Current implementation anchors:
- `synaptome/src/io/AudioInputBridge.h`
- `synaptome/src/io/AudioInputBridge.cpp`
- `synaptome/bin/data/config/audio.json`
- OSC-style metric addresses such as `/sensor/host/localmic/mic-level` and `/sensor/host/localmic/mic-peak`

Contract fields that should become explicit:

| Field | Meaning |
| --- | --- |
| Device selection | Index, name contains, fallback behavior. |
| Audio config | Sample rate, buffer size, channels, enable flag. |
| Metrics | RMS/level, peak, optional future spectral features. |
| Address prefix | Public sensor namespace for local host audio. |
| Mapping target | Parameter modifier target or route target. |
| Publishing | Optional host/port for local OSC publish. |

Current gaps:
- Audio config is not yet listed as a first-class contract in `docs/contracts/README.md`.
- No schema or fixture validates `config/audio.json`.
- No public policy says whether host mic is local-only, scene-persisted, or operator preference.
- Spectral/audio feature parameters are not defined as a reusable library yet.

## Webcam And Video Device Contract

What it should promise:
- A webcam can be loaded as a media layer.
- Device selection, resolution, gain, mirror, and overlay diagnostics are parameters.
- If a device disappears or is busy, Synaptome should degrade visibly and recover predictably.

Current implementation anchors:
- `synaptome/src/visuals/VideoGrabberLayer.*`
- `synaptome/bin/data/layers/media/webcam_primary.json`
- `synaptome/bin/data/logs/webcam_devices.json`
- HUD layer metadata emitted from `ofApp`

Current runtime semantics:
- The primary webcam asset can defer hardware open until after scene publish. Scene load success means the camera layer is scheduled and parameterized; it does not guarantee that the camera has already opened or produced its first frame.
- Deferred camera setup failures should be logged and routed through webcam retry/fallback behavior rather than escaping the scene transaction after publish.

Current gaps:
- Webcam inventory is a runtime log, not a public contract.
- Device labels and indices are machine-dependent; stable matching policy is not documented.
- BrowserFlow has a native lazy-open/failure regression, but no public fixture validates real hardware selection/fallback behavior.
- Webcam layer parameters are useful, but not yet listed in a media parameter vocabulary.

## Media File Contract

What it should promise:
- Artists can add media and make it appear in the Browser.
- Media entries can be loaded into slots and controlled through consistent parameters.

Current implementation anchors:
- `synaptome/src/media/VideoCatalog.*`
- `synaptome/bin/data/config/videos.json`
- `synaptome/src/visuals/VideoClipLayer.*`
- `synaptome/bin/data/layers/media/clip_default.json`

Current status:
- Media discovery is manifest-based through `config/videos.json`.
- The desired public workflow of dropping media into a watched folder is a target, not a locked behavior.

Current gaps:
- Need a decision: explicit manifest, folder scan, or both.
- Need media path validation and package-copy rules.
- Need a fixture proving media entries become Browser-visible and slot-loadable.

## External Display Contract

What it should promise:
- Projection is audience output.
- Control Window is operator output.
- The Control Window contains HUD, Console, and Browser.
- External monitor/projector state can be persisted and restored without disrupting scene load.

Current implementation anchors:
- `synaptome/src/ui/ThreeBandLayout.h`
- `synaptome/src/ofApp.cpp` secondary display and monitor selection code
- `synaptome/src/io/ConsoleStore.h`
- `synaptome/bin/data/config/console.json`

Contract fields that should become explicit:

| Field | Meaning |
| --- | --- |
| Projection role | Audience output, no unintended operator UI. |
| Control Window role | Operator HUD, Console, Browser, diagnostics. |
| Monitor hint | Auto, primary, secondary, numeric index, or name fragment. |
| Geometry | X, Y, width, height, DPI scale, vsync. |
| Layout mode | Follow projection overlays or freeform controller layout. |
| Recovery | Watchdog/reconnect behavior when a monitor/window disappears. |
| Scene policy | Which display fields are scene-owned vs local preference. |

Current gaps:
- Secondary display/console state now has a static snapshot validator through `tools/validate_console_layout_contract.py --check`, and scene load applies state through staged scene/display transaction helpers.
- App-native live-window transaction fixtures remain follow-up so schema-valid scenes can be proven against active Control Window and Projection surfaces.
- Some persisted fields still carry legacy Browser implementation naming.
- Public docs should describe monitor setup without requiring the user to know openFrameworks window internals.
- Projection monitor selection is not yet a first-class contract; the main projection window is currently fixed in `main.cpp` while the controller monitor has richer selection and CLI override behavior.
- The scene/window policy is not locked: scenes currently persist console slots, params, globals, and mappings, while secondary display geometry/layout mostly lives in local console state.
- Single-screen mode can still mix operator controls into the projection; clean projection is reliable only when the operator surface is separated.
- Monitor failure behavior needs one documented policy; current docs and watchdog recovery behavior are not fully aligned.

## Synaptome vs Helper Repos vs Examples

| Area | Belongs In Synaptome | Belongs In Helper Repo | Belongs In Example Pack |
| --- | --- | --- | --- |
| App-facing OSC addresses | Yes | Consumed/emitted by helpers | May include sample senders. |
| Radio TLV decode | No, except shared contract docs/fixtures | Yes, helper or firmware package | No. |
| MIDI device maps | Yes, generic examples | Optional manufacturer packs | Controller-specific examples. |
| Host mic capture | Yes, as app input adapter | No | Example scenes using mic mappings. |
| Webcam/media layers | Yes, generic primitives | No | Sample media files and presets. |
| Specific show scenes | No, except minimal examples | No | Yes. |
| STL/video/style assets | No, except minimal examples | No | Yes. |
| openFrameworks layer SDK | Yes | No | Example layers. |
| Embedded device firmware | No | Yes | Maybe simulator/demo sender. |

## Gap Summary

| Gap | Why It Matters | Likely Follow-Up |
| --- | --- | --- |
| Parameter manifest | MIDI, OSC, scenes, and device maps need strict target validation. | Current persisted targets now pass `tools/validate_parameter_targets.py --strict`; extend that gate as new target-bearing files become public contracts. |
| External route catalog | Hardware/software senders need a clear OSC/MIDI target map. | Machine-readable OSC/device contract docs. |
| Audio config schema | Host mic should be a first-class local sensor source. | Add audio config schema and fixture. |
| Media discovery policy | Public users expect easy media onboarding. | Decide manifest vs folder scan, add fixture. |
| Display/window schema | Projector/control monitor stability is live-critical. | Baseline Console/display snapshot validator and scene transaction implementation now exist; add app-native live-window fixtures and a public monitor schema. |
| Device-map logical fixture | New controllers should avoid source edits. | Synthetic controller and MIDI Mix golden fixture now exist. |
| Helper repo boundary | Public packaging needs clean seams. | Extract helper contract only after fixture-backed dry run. |

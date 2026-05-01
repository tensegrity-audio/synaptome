# Synaptome Public Contract Index

Status: public app/runtime contract coverage is locked for the first Synaptome extraction slice. The current `python tools\validate_configs.py --public-app` report covers 10 public contracts.

This index names the contracts that the standalone Synaptome runtime owns: app configuration, scenes, layer assets, parameter IDs, mappings, HUD/Console state, and the public artist SDK fixture.

Fixture inventory: [fixtures.md](fixtures.md)

Known follow-up debt: [contract_gaps.md](contract_gaps.md)

Public extraction planning:
- [synaptome_public_extraction_manifest.json](synaptome_public_extraction_manifest.json) is the allowlist/exclusion manifest for a clean `synaptome` repo.
- `python tools\validate_synaptome_extraction_manifest.py --check --strict-review` validates that manifest against tracked files before extraction.
- `python tools\validate_configs.py --public-app` validates the public Synaptome app/runtime contract subset without helper, firmware, governance, or legacy payload checks.
- `.github/workflows/ci.yml` is the public app-only CI workflow included in the extraction payload.

Boundary decision: first public Synaptome owns app-facing contracts and examples. Hardware decode, embedded firmware, generated radio headers, private deployment netmaps, helper implementation source, and legacy payload quarantine are outside the public runtime payload unless rewritten as app-facing examples.

Architecture contract drafts:
- [Synaptome External Contracts](../architecture/synaptome_external_contracts.md) maps MIDI, OSC, helper repos, microphones, webcams, media files, displays, and hotkeys as outside-world boundaries.
- [Synaptome Artist SDK And Compatibility Layer](../architecture/synaptome_artist_sdk.md) maps the public layer/parameter/catalog surface for openFrameworks artists.

Parameter contract artifacts:
- [parameter_manifest.json](parameter_manifest.json) is the generated static parameter ID snapshot.
- [parameter_vocabulary.md](parameter_vocabulary.md) is the first public naming vocabulary for reusable Synaptome parameters.

Coverage commands:

```powershell
python tools\validate_configs.py --public-app
python tools\validate_parameter_targets.py --strict --contract-fixtures
```

Strict public contract mode reads committed fixtures from `tools/testdata/**` and examples under `docs/examples/**`. Live app-written files under `synaptome/bin/data/config/` and `synaptome/bin/data/layers/scenes/` remain runtime smoke state unless intentionally promoted into fixtures.

## Scope Labels

- `app-runtime`: openFrameworks runtime lifecycle, scene load, console/layer ownership, and window state.
- `app-contract`: persisted app data, schemas, parameter IDs, scene/layer/device-map contracts, OSC routes consumed by the app, and mapping fixtures.
- `artist-sdk`: public layer authoring surface, catalog metadata, source-registration example, and example scene fixture.
- `adapter-contract`: host-facing adapters such as MIDI, OSC, audio, webcam, media, display, and hotkey inputs.

## Contract Status Values

- `Current`: observed implementation source for active public behavior.
- `Draft`: intended public contract exists, but shape or ownership is not fully locked.
- `Missing Validator`: source exists, but no dedicated validation path covers it.
- `Missing Fixture`: validator or check exists, but fixture coverage is not yet sufficient.

## Public App Index

| Contract | Purpose | Primary Scope | Owner Role | Canonical Source | Consumers | Version Policy | Validator Command | Fixture Location | Migration Policy | Current Status | Extraction Blocker |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| App OSC map | Host OSC input routes into parameters and app behavior. | `app-contract` | App contract owner | `synaptome/bin/data/config/osc-map.json` | `MidiRouter`, Browser, app runtime, device adapters | Config has no explicit version yet; preserve backwards-compatible route entries. | `python tools\validate_configs.py --public-app` | `docs/examples/osc_map_example.json` | Route migrations need config compatibility notes and Browser validation. | Current; Validated | Partial: richer outside-world route fixtures remain future public-boundary work. |
| Parameter ID manifest | Stable parameter IDs consumed by scenes, mappings, OSC, MIDI, and UI. | `app-contract` | App contract owner | `docs/contracts/parameter_manifest.json` | Scene persistence, MIDI map, OSC map, Browser, HUD feeds | Manifest schema is v1; public ID renames require deprecation or migration notes. | `python tools\gen_parameter_manifest.py --check` | `docs/contracts/parameter_manifest.json`, `docs/examples/parameter_example.json` | Rename/deprecation policy required for any public parameter ID; strict target validation is tracked by Parameter target references. | Current; Validated | Partial: broader vocabulary/range policy remains follow-up. |
| Parameter target references | Strict check that persisted scenes, MIDI maps, OSC maps, audio modifiers, device maps, and HUD widget IDs point at known parameter IDs, Console slot templates, or layer catalog IDs. | `app-contract` | App contract owner | `docs/contracts/parameter_manifest.json`, committed runtime-state fixtures | Scene persistence, MIDI map, OSC map, Browser, HUD feeds | Strict validator must pass before release/extraction; intentional non-parameter catalog IDs must resolve through the layer catalog. | `python tools\validate_parameter_targets.py --strict --contract-fixtures` | `tools/testdata/runtime_state/config/*.json`, `tools/testdata/runtime_state/layers/scenes/*.json`, `tools/testdata/device_maps/*.json` | Target ID renames require fixture migration or explicit compatibility aliases; live app-written drift must not block the strict gate. | Current; Validated | No for current fixture set; extend coverage as new target-bearing files are promoted. |
| Layer asset catalog | JSON layer asset shape loaded by the visual layer library. | `app-contract` | App contract owner | `synaptome/bin/data/layers/**/*.json`, `synaptome/src/visuals/LayerLibrary.*`, `synaptome/src/ofApp.cpp` | Scene runtime, console/layer stack, Browser | Schema version pending; current assets are source examples. | `python tools\layer_catalog_regression.py --check` | `tools/testdata/layer_catalog/expected_catalog.json`, `synaptome/bin/data/layers/**/*.json` | Runtime layer types must map to factory registrations. | Current; Validated | Partial: plugin/source-registration policy remains pending. |
| Artist SDK example fixture | Minimal public source/catalog/scene bundle for an artist-authored layer. | `artist-sdk` | App contract owner | `docs/examples/artist_sdk/SignalBloomLayer.*`, `docs/examples/artist_sdk/register_signal_bloom.cpp`, `docs/examples/artist_sdk/signal_bloom.layer.json`, `docs/examples/artist_sdk/signal_bloom.scene.json` | Layer authoring docs, Browser catalog, Console scenes, MIDI/OSC/sensor mapping examples, public SDK packaging | Fixture schema v1; source registration remains explicit until the extension/package boundary is chosen. | `python tools\validate_artist_sdk_example.py --check` | `docs/examples/artist_sdk/**`, `tools/testdata/artist_sdk/expected_artist_sdk_example.json` | Parameter suffix, catalog ID, scene target, and registration changes must update the SDK snapshot and docs together. | Current; Validated | Partial: package layout and non-source-edit extension mechanism remain pending. |
| Scene persistence schema | Saved scene files and scene-last runtime state. | `app-runtime` | Runtime owner, app contract owner | `ofApp::encodeSceneJson`, `ofApp::loadScene`, committed scene fixtures | Runtime, console/layers, mappings, HUD/window state | Scene schema version is implicit; scene v2 contract is pending. | `python tools\validate_scene_persistence_contract.py --check` | `tools/testdata/scene_persistence/expected_scene_contract.json`, `tools/testdata/runtime_state/layers/scenes/*.json`, `tools/testdata/runtime_state/config/scene-last.json` | Scene transaction implementation and manual smoke have passed; use `--live` for local app-written smoke state. | Current; Validated | Partial: app-native/live-window fixture coverage remains follow-up, not a current extraction blocker. |
| Device-map schema | Logical device/control slot mapping for controllers and adapters. | `app-contract` | App contract owner | `synaptome/bin/data/device_maps/*.json`, `tools/testdata/device_maps/synthetic_controller.json` | Browser, MIDI/OSC mapping, device adapters | Schema version pending; logical role families are fixture-backed. | `python tools\device_map_regression.py --check` | `synaptome/bin/data/device_maps/MIDI Mix 0.json`, `tools/testdata/device_maps/synthetic_controller.json`, `tools/testdata/device_maps/expected_logical_slots.json` | New device support should be data/schema first; intentional role/binding changes must update the golden logical-slot fixture. | Current; Validated | No for current device-map logical slots; extend when target/action bindings become public. |
| MIDI mapping persistence | MIDI CC/button/OSC routing persistence for controls. | `app-contract` | App contract owner | `synaptome/bin/data/config/midi-map.json` | `MidiRouter`, Browser, parameter runtime | Config has no explicit version yet; preserve legacy mapping fields. | `python tools\validate_configs.py --public-app` | `docs/examples/midi_bank_example.json` | Add migration notes when target IDs or slot assignment keys change. | Current; Validated | Partial: depends on stable parameter IDs and device-map contract. |
| HUD layout/feed persistence | HUD widgets, feed IDs, overlay layout, and operator preferences. | `app-runtime` | UI shell owner | `HudRegistry`, `HudFeedRegistry`, `OverlayManager`, HUD layer assets, committed runtime-state fixtures | Projection/control windows, UI shell, scene load path | Snapshot schema v1; dynamic feed payloads and timestamps remain runtime-local. | `python tools\validate_hud_layout_contract.py --check` | `tools/testdata/hud_layout/expected_widgets.json`, `tools/testdata/runtime_state/config/control_hub_prefs.json`, `tools/testdata/runtime_state/config/console.json`, `synaptome/bin/data/layers/hud/*.json` | Widget ID/feed/layout changes must update the golden HUD fixture; use `--live` for local operator-state smoke checks. | Current; Validated | Partial: current HUD layout/feed fixtures are validated; app-native/live-window coverage can expand as follow-up. |
| Console layout/secondary display persistence | Console slot assignments and secondary display state. | `app-runtime` | Runtime owner, UI shell owner | `ConsoleStore`, committed runtime-state fixtures | Console renderer, projection/control windows, scene load path | Snapshot schema v1; monitor coordinates, timestamps, and sensor warm-start snapshots are validated as shape but excluded from golden churn. | `python tools\validate_console_layout_contract.py --check` | `tools/testdata/console_layout/expected_console_contract.json`, `tools/testdata/runtime_state/config/console.json`, `tools/testdata/runtime_state/config/ui/slot_assignments.json` | Scene transaction implementation and manual smoke have passed; use `--live` for local operator-state smoke checks. | Current; Validated | Partial: app-native/live-window fixture coverage remains follow-up, not a current extraction blocker. |

## Staging-Only Boundaries

Helper/radio decode, embedded UI exchange, firmware payload quarantine, private deployment maps, and governance workflow checks are not public Synaptome app contracts. In the source workspace, broader validation may still check those areas while they wait for their own package or helper repo. They must not become dependencies of the standalone public runtime.

## G0 Exit Notes

- The app extraction boundary is validated, and the final local public import has passed the extraction ladder; remote push still needs an empty public repo and remote CI.
- Browser/Control Panel validation is app-facing. The app-native runner is `synaptome/tests/BrowserFlowTest/BrowserFlowTest.vcxproj` plus `python tools\run_control_hub_flow.py`.
- Public docs should describe Synaptome as an openFrameworks runtime package, not as a replacement for Visual Studio or openFrameworks.
- Scene/display transaction work has landed; remaining public-release polish is reviewed branch/fresh extraction plus live-window fixture and display-resilience follow-up.

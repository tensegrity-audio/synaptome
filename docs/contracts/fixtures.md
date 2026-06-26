# Synaptome Public Fixture Inventory

Status: public app/runtime fixture coverage is locked for the first Synaptome extraction slice.

This inventory tracks the fixtures that make public contract validation reproducible in a standalone Synaptome repo. Helper/radio decode fixtures and embedded firmware evidence are intentionally excluded from the first public payload unless rewritten as app-facing examples.

Extraction scope is tracked separately in [`synaptome_public_extraction_manifest.json`](synaptome_public_extraction_manifest.json). Public validation starts with `python tools\validate_synaptome_extraction_manifest.py --check --strict-review` and `python tools\validate_configs.py --public-app`.

## Current Public Fixtures

| Contract | Fixture Path | Validator | Coverage | Status |
| --- | --- | --- | --- | --- |
| App OSC map | `docs/examples/osc_map_example.json`, `synaptome/bin/data/config/osc-map.json`, `tools/validate_osc_route_patterns.py` | `python tools\validate_configs.py --public-app`, `python tools\validate_osc_route_patterns.py` | Schema, current app route validation, and built-in mesh route glob matching. | Current |
| Signal Control receive and OSC input selection | `docs/examples/osc_input_example.json`, `tools/testdata/runtime_state/config/osc-input.json`, `tools/testdata/signal_control/expected_receive_contract.json` | `python tools\validate_signal_control_receive_contract.py --check`, `python tools\validate_configs.py --public-app` | Browser receive-mode config shape, Router UDP default endpoint, Signal Control scalar telemetry routes, waveform route/sample-count rules, and freshness thresholds. | Current |
| Parameter ID manifest | `docs/contracts/parameter_manifest.json`, `docs/examples/parameter_example.json` | `python tools\gen_parameter_manifest.py --check`, `python tools\validate_configs.py --public-app` | Generated static snapshot of core, sensor, effect, layer asset, and console slot parameter ID patterns. | Current |
| Parameter target references | `tools/testdata/runtime_state/config/*.json`, `tools/testdata/runtime_state/layers/scenes/*.json`, `tools/testdata/device_maps/*.json` | `python tools\validate_parameter_targets.py --strict --contract-fixtures`, `python tools\validate_configs.py --public-app` | Strict semantic scan for committed runtime-state target IDs against the generated parameter manifest, Console slot templates, and layer catalog IDs. | Current |
| MIDI mapping persistence | `docs/examples/midi_bank_example.json`, `synaptome/bin/data/config/midi-map.json` | `python tools\validate_configs.py --public-app` | Schema and current MIDI route validation. | Current |
| Device-map schema | `synaptome/bin/data/device_maps/MIDI Mix 0.json`, `tools/testdata/device_maps/synthetic_controller.json`, `tools/testdata/device_maps/expected_logical_slots.json` | `python tools\device_map_regression.py --check`, `python tools\validate_configs.py --public-app` | Strict logical-slot snapshot for current MIDI Mix mapping plus a synthetic controller covering role families, sensitivity range, MIDI binding shape, and duplicate-binding protection. | Current |
| Layer asset catalog | `tools/testdata/layer_catalog/expected_catalog.json`, `synaptome/bin/data/layers/**/*.json` except scenes | `python tools\layer_catalog_regression.py --check`, `python tools\validate_configs.py --public-app` | Golden static catalog snapshot mirroring `LayerLibrary` JSON ingestion, category sorting, HUD/FX/runtime classification, and LayerFactory type resolution. | Current |
| Artist SDK example fixture | `docs/examples/artist_sdk/**`, `tools/testdata/artist_sdk/expected_artist_sdk_example.json` | `python tools\validate_artist_sdk_example.py --check`, `python tools\validate_configs.py --public-app` | Static source/catalog/scene proof for an artist-authored `Layer`, factory registration snippet, Browser catalog metadata, Console scene slot, media pairing, reusable parameter suffixes, and MIDI/OSC/sensor route targets. | Current |
| HUD layout/feed persistence | `tools/testdata/hud_layout/expected_widgets.json`, `tools/testdata/runtime_state/config/control_hub_prefs.json`, `tools/testdata/runtime_state/config/console.json`, `synaptome/bin/data/layers/hud/*.json` | `python tools\validate_hud_layout_contract.py --check`, `python tools\validate_configs.py --public-app` | Static HUD contract for widget asset identity, declared feed IDs, Browser HUD preferences, and Console overlay layout snapshots. Dynamic feed payloads, timestamps, and Browser selection state are intentionally excluded. | Current |
| Console layout/secondary display persistence | `tools/testdata/console_layout/expected_console_contract.json`, `tools/testdata/runtime_state/config/console.json`, `tools/testdata/runtime_state/config/ui/slot_assignments.json` | `python tools\validate_console_layout_contract.py --check`, `python tools\validate_configs.py --public-app` | Static Console/display contract for eight slots, layer asset references, overlay flags, display preference shape, slot assignment shape, and HUD placement shape. | Current |
| Scene persistence schema | `tools/testdata/scene_persistence/expected_scene_contract.json`, `tools/testdata/runtime_state/layers/scenes/*.json`, `tools/testdata/runtime_state/config/scene-last.json` | `python tools\validate_scene_persistence_contract.py --check`, `python tools\validate_configs.py --public-app` | Static scene-owned summary for committed scene fixtures and scene-last: slot bounds, layer asset references, scalar/modifier value shape, canonical JSON stability, effect/global keys, and bank targets. Runtime staged apply/rollback and local app-written scene drift are intentionally excluded. | Current |

## Draft Layer Package Fixtures

| Contract | Fixture Path | Validator | Coverage | Status |
| --- | --- | --- | --- | --- |
| Layer package manifest | `docs/examples/layer_packages/signal_bloom/layer.package.json`, `docs/examples/layer_packages/signal_bloom/presets/*.json` | `python tools\validate_layer_packages.py --check` | Draft package metadata, package-owned parameters, preset values, visible OSC mapping preset metadata, source registration references, and bench metadata references. | Draft |
| Package-derived catalog snapshot | `tools/testdata/layer_packages/expected_package_catalog.json` | `python tools\layer_package_catalog_regression.py --check` | Browser-like package catalog output generated from package metadata without changing current runtime loading. | Draft |
| Package-derived parameter manifest snapshot | `tools/testdata/layer_packages/expected_package_parameter_manifest.json` | `python tools\layer_package_parameter_manifest.py --check` | Package-scoped public parameter IDs and Console slot templates expanded from package suffix declarations. | Draft |
| Combined package/runtime catalog snapshot | `tools/testdata/layer_packages/expected_combined_layer_catalog.json` | `python tools\layer_catalog_regression.py --include-packages --check` | Draft compatibility view proving package catalog entries can sit beside the current legacy layer catalog without ID conflicts. | Draft |
| Combined package/runtime parameter manifest snapshot | `tools/testdata/layer_packages/expected_combined_parameter_manifest.json` | `python tools\gen_parameter_manifest.py --include-packages --check` | Draft compatibility view proving package-declared parameters can sit beside the current generated manifest without ID conflicts. | Draft |

## Missing Or Partial Public Fixtures

| Contract | Needed Fixture | Reason |
| --- | --- | --- |
| Scene/display runtime transaction | App-native staged-load/rollback fixture with control/projection windows active. | Required to prove schema-valid scenes load without exposing partial live state. |
| External device/display contract map | Contract fixtures for MIDI, broader gateway OSC, audio, video, display, and hotkey boundaries. | Required before Synaptome can publish a clear outside-world compatibility surface. Signal Control receive is now covered, but full synaptome_mesh gateway fanout remains Tensegrity-owned router work. |
| Host audio input contract | `config/audio.json` schema fixture plus expected `/sensor/host/localmic/*` metrics. | Required before local mic becomes a public audio-reactive input contract. |
| Media onboarding | Fixture proving media entries become Browser-visible and slot-loadable. | Required before the public workflow can promise manifest or folder-scan behavior. |

## Fixture Rules

- Fixtures must use the current public app contract shape unless explicitly marked historical in their own docs.
- Generated outputs should be checked with read-only commands before public extraction.
- `tools/testdata/runtime_state/**` is the committed source of truth for strict app runtime-state contracts.
- Live app-written files under `synaptome/bin/data/config/` and `synaptome/bin/data/layers/scenes/` are runtime smoke state unless intentionally copied into a fixture update.
- Missing fixture status is acceptable while a boundary is still draft or runtime-owned. Unknown fixture ownership is not.

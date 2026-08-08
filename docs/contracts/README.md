# Synaptome Public Contract Index

Status: public app/runtime contract coverage is locked for the first Synaptome extraction slice. The current `python tools\validate_configs.py --public-app` report covers 23 public contracts.

This index names the contracts that the standalone Synaptome runtime owns: app configuration, scenes, layer assets, parameter IDs, mappings, HUD/Console state, and the public artist SDK fixture.

Fixture inventory: [fixtures.md](fixtures.md)

Known follow-up debt index: [contract_gaps.md](contract_gaps.md)

Public extraction planning:
- [synaptome_public_extraction_manifest.json](synaptome_public_extraction_manifest.json) is the allowlist/exclusion manifest for a clean `synaptome` repo.
- `python tools\validate_synaptome_extraction_manifest.py --check --strict-review` validates that manifest against tracked files before extraction.
- `python tools\validate_configs.py --public-app` validates the public Synaptome app/runtime contract subset without helper, firmware, governance, or legacy payload checks.
- `.github/workflows/ci.yml` is the public app-only CI workflow included in the extraction payload.

Boundary decision: first public Synaptome owns app-facing contracts and examples. Hardware decode, embedded firmware, generated radio headers, private deployment netmaps, helper implementation source, and legacy payload quarantine are outside the public runtime payload unless rewritten as app-facing examples.

External producer note: Synaptome Mesh's first producer contract is `synaptome-mesh-osc v0.1.0`, owned outside this repo. Synaptome remains compatible with it by accepting app-facing OSC input routes; this repo must not import Synaptome Mesh firmware, gateway, generated config, or private deployment identity.

Cross-repo stream routing note: [Signal Control Integration](signal_control_integration.md) defines the split between Synaptome receive/runtime work and Tensegrity desktop router/OBS work.

Architecture contract drafts:
- [Element SDK v1 Boundary](../architecture/element_sdk_v1_boundary.md) freezes
  the compiler-matched SDK boundary and the implemented minimal
  type/kind/action descriptor contract.
- [Synaptome External Contracts](../architecture/synaptome_external_contracts.md) maps MIDI, OSC, helper repos, microphones, webcams, media files, displays, and hotkeys as outside-world boundaries.
- [Synaptome Artist SDK And Compatibility Layer](../architecture/synaptome_artist_sdk.md) maps the public layer/parameter/catalog surface for openFrameworks artists.
- [Synaptome Layer System Roadmap](../architecture/synaptome_layer_system_roadmap.md) owns package, folder discovery, generated layer assets, presets, mapping presets, validation, and bench follow-up.
- [Synaptome Transport And Reactivity Contract](../architecture/synaptome_transport_reactivity.md) owns BPM, beat detection, transport source, confidence, onset/downbeat, and mapping-visible timing follow-up.
- [Synaptome Public Runtime Contract Roadmap](../architecture/synaptome_public_runtime_contract_roadmap.md) owns non-layer scene, HUD, console/display, media, device, external boundary, and host audio follow-up.

Parameter contract artifacts:
- [Built-In Element Parameter Contract](builtin_element_parameter_contract.md)
  is the authoritative maintainer workflow and compatibility-adapter policy.
- [builtin_element_parameters.json](builtin_element_parameters.json) is the
  reviewed, type-level source snapshot for all 23 built-in element parameter
  contracts.
- [element_parameter_catalog.json](element_parameter_catalog.json) is the
  generated machine-readable catalog view consumed by tooling and inspection
  surfaces.
- [Element Parameter Reference](../element_parameter_reference.md) is the
  generated human-readable reference for the same declarations.
- `python tools\gen_builtin_element_contracts.py --check` verifies the reviewed
  snapshot, compiled Runtime payload, catalog view, and documentation stay
  identical.
- [parameter_manifest.json](parameter_manifest.json) is the generated static parameter ID snapshot.
- [parameter_vocabulary.md](parameter_vocabulary.md) is the first public naming vocabulary for reusable Synaptome parameters.

State ownership and provenance:

- [State Ownership And Provenance Contract](state_ownership_and_provenance.md)
  freezes the SEAC-5 ownership matrix, value-origin vocabulary, independent
  artifact version rules, legacy Scene v1/current Scene v2 classification,
  omitted-versus-empty semantics, and machine/preference boundary. The
  side-effect-free Scene compatibility reader, pointer-free value-origin
  inspection, mapping-bank v1 route reader/writer, machine-profile v1, and
  preferences-v1 aggregate are implemented. Preferences v1 owns
  Browser/HUD, hotkeys, local package activation/next-load preset, and the
  active mapping-bank choice. Mapping routes remain in mapping-bank v1;
  custom global bank definitions use the independent, recoverable
  bank-definitions-v1 operator store and are omitted by new Scene writers.

Draft layer package artifacts:

These are draft layer-system contracts. They prove package metadata can be
validated and compared with current runtime surfaces, but they do not mean the
app runtime loads packages yet.

- [layer_package.schema.json](../schemas/layer_package.schema.json) is the draft package manifest schema.
- [layer_preset.schema.json](../schemas/layer_preset.schema.json) is the draft layer preset schema.
- [Signal Bloom package fixture](../examples/layer_packages/signal_bloom/layer.package.json) is the first package-declared layer example.
- `python tools\layer_package_discovery.py` reports the fixture package root, future runtime package root, and legacy/package coexistence rule.
- `python tools\validate_layer_packages.py --check` validates current package fixtures.
- `python tools\layer_package_catalog_regression.py --check` validates the package-derived catalog snapshot.
- `python tools\layer_package_parameter_manifest.py --check` validates the package-derived parameter manifest snapshot.
- `python tools\layer_catalog_regression.py --include-packages --check` validates the draft combined package/runtime catalog snapshot.
- `python tools\gen_parameter_manifest.py --include-packages --check` validates the draft combined package/runtime parameter manifest snapshot.

Draft generated-layer artifacts:

These are draft file-backed layer contracts. They prove a content file can be
expanded into stable catalog-style metadata, but they do not mean the app
runtime scans STL/model folders yet.

- [generated_layer_template.schema.json](../schemas/generated_layer_template.schema.json) is the draft template schema for generated content-backed layers.
- [generated_layer_sidecar.schema.json](../schemas/generated_layer_sidecar.schema.json) is the optional per-content metadata sidecar schema.
- [STL generated-layer fixture](../examples/generated_layers/stl_models/generated_layer.template.json) is the first template-backed content example.
- `python tools\generated_layer_catalog_regression.py --check` validates generated catalog output, static and dynamic option metadata, and legacy layer asset ID conflicts.

Draft Browser inspection artifacts:

These are draft pre-UI contracts. They prove package and generated-layer
metadata can be combined for inspection without requiring the Browser to load,
instantiate, scan, or mutate anything.

- [layer_browser_inspection_payload.schema.json](../schemas/layer_browser_inspection_payload.schema.json) is the draft read-only inspection payload schema.
- [expected_layer_browser_inspection_payload.json](../../tools/testdata/layer_browser_inspection/expected_layer_browser_inspection_payload.json) is the current package/generated-layer inspection snapshot.
- `python tools\layer_browser_inspection_payload.py --check` validates the generated payload against the golden snapshot and semantic read-only rules.
- `python tools\validate_configs.py tools\testdata\layer_browser_inspection\expected_layer_browser_inspection_payload.json` validates the snapshot against the schema.

Media intake artifacts:

- [media_catalog.md](media_catalog.md) locks current discovery to explicit
  manifests and defines public versus operator-local roots, provenance,
  replacement, and promotion rules.
- [media_catalog.schema.json](../schemas/media_catalog.schema.json) requires
  stable IDs, hashes, provenance, redistribution state, and layer-default
  references for every future clip.
- [media_catalog_example.json](../examples/media_catalog_example.json) is the
  valid empty pre-media baseline; it contains no binary media.
- `python tools\media_catalog_regression.py --check` validates the canonical
  one-asset catalog, empty public example, and negative semantic fixtures.

Layer package activation artifacts:

- [layer_package_activation.md](layer_package_activation.md) defines the
  read-only inspection boundary, disabled-by-default allowlist, value merge
  order, and suggestion-only mapping policy.
- [layer_package_activation.schema.json](../schemas/layer_package_activation.schema.json)
  validates the runtime activation file.
- `python tools\synaptome_layer.py check <layer.package.json>` is the focused
  author command.
- `python tools\synaptome_layer.py runtime-adapter <layer.package.json>
  --output <optional-layer.json> --check` rejects hand-maintained runtime
  adapter drift.
- `python tools\signal_bloom_runtime_contract.py` rejects drift between the
  package, its preset files, the reviewed optional catalog adapter, mapping
  ownership, and committed default-off activation.
- `LayerPackageBench` inspects Signal Bloom's copied `Visual` descriptor with
  an empty ordered action declaration, proves enumeration-copy isolation before
  creation, then registers all 18 parameters, advances 240 frames, and issues a
  stub-backed draw call. This is source-registration evidence, not package
  descriptor serialization or package/runtime parameter parity, and it does
  not prove framebuffer pixels or real GL behavior.

Coverage commands:

```powershell
python tools\validate_configs.py --public-app
python tools\gen_builtin_element_contracts.py --check
python tools\validate_parameter_targets.py --strict --contract-fixtures
python tools\validate_portable_machine_state.py --check
python tools\validate_modular_layer_families.py
python tools\validate_layer_packages.py --check
python tools\layer_package_catalog_regression.py --check
python tools\layer_package_parameter_manifest.py --check
python tools\layer_catalog_regression.py --include-packages --check
python tools\gen_parameter_manifest.py --include-packages --check
python tools\generated_layer_catalog_regression.py --check
python tools\layer_browser_inspection_payload.py --check
python tools\validate_configs.py tools\testdata\layer_browser_inspection\expected_layer_browser_inspection_payload.json
python tools\media_catalog_regression.py --check
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
| Built-in element parameter declarations | Construction-free type-level authority for parameter IDs, kinds, groups, defaults, ranges, options, aliases, deprecations, and live storage parity. | `app-contract`, `artist-sdk`, `app-runtime` | Runtime owner, app contract owner | `docs/contracts/builtin_element_parameters.json`, compiled generated Runtime payload | Runtime preparation, Browser, catalog/reference generation, scenes, presets, MIDI/OSC mappings | Stable IDs and kinds are compatibility boundaries; metadata or default changes require reviewed regeneration and fixture/migration review. | `python tools/gen_builtin_element_contracts.py --check`; app `--validate-builtin-element-contracts <layers-root>` | All 55 committed layer assets plus RuntimeCore adapter failure cases | Preserve type-level ID/kind surfaces across asset variants; use aliases or explicit migrations for public renames. | Current; Validated for 23 types and 786 parameters | No for built-in declaration or runtime value-origin inspection; persisted artifact versioning remains SEAC-5. |
| Shared Text configuration transaction | Prevent the host-global Text compatibility store from leaking candidate defaults before composition adoption. | `app-runtime`, `adapter-contract` | Runtime owner, app contract owner | `TextLayer`, `TextLayerState::Snapshot`, `Layer::onParameterRegistryCommitted` | Runtime preparation/replacement, Browser host parameters, Scene `overlay.text.*` values | Candidate configuration is copied and fallible; setup does not touch live shared state; the no-throw adoption hook publishes strings by swap and scalars by assignment only after Runtime staging succeeds. | `BrowserFlowTest`; Release app build | Native abandonment/adoption snapshot scenario plus production `TextLayer` compilation | Preserve all `overlay.text.*` IDs and shared semantics. Instance-owned migration requires aliases and Scene fixtures; this transaction does not claim per-instance state. | Current; Validated | Partial: the singleton is intentionally retained as compatibility state. |
| App OSC map | Host OSC input routes into parameters and app behavior, including built-in mesh-style sensor routes. | `app-contract` | App contract owner | `synaptome/bin/data/config/osc-map.json`, `synaptome/src/ofApp.cpp`, `synaptome/src/io/OscParameterRouter.*` | `MidiRouter`, Browser, app runtime, device adapters | Config has no explicit version yet; preserve backwards-compatible route entries. Built-in route patterns use `OscParameterRouter` glob syntax, not raw regex. | `python tools\validate_configs.py --public-app`, `python tools\validate_osc_route_patterns.py` | `docs/examples/osc_map_example.json`, `tools/validate_osc_route_patterns.py` | Route migrations need config compatibility notes and Browser validation. Regex-looking `.*` route segments are rejected for built-in mesh routes. | Current; Validated | Partial: richer outside-world route fixtures remain future public-boundary work. |
| Generic OSC ingress and Synaptome Mesh compatibility | Preserve all well-formed OSC as typed observations while applying only finite single numeric scalars to the existing mapping engine; normalize the producer-owned Mesh v0.1 route pair into one Synaptome-facing route. | `app-contract`, `adapter-contract` | App contract owner | `OscIngressMessage`, `SerialSlipOsc`, `ofApp::observeOscIngressMessage` | Direct serial, Router UDP, Browser input diagnostics, HUD feeds, OSC learn/router | Consumer profile `synaptome-mesh-v1` targets producer contract `synaptome-mesh-osc v0.1.0`. Raw address, payload shape, transport, and endpoint remain provenance. | `python tools\validate_osc_ingress_contract.py`; `BrowserFlowTest` | `tools/testdata/osc_ingress/synaptome_mesh_v0_1_0_consumer_capture.json` | Keep the Mesh sender generic. Strip `/synaptome_mesh` only into the derived canonical address, alias `/sensor/hr/<id>/heart-bpm` to `/bpm`, and suppress only an immediate identical legacy/namespaced pair from the same origin. New payload-to-parameter semantics require an explicit typed mapping contract. | Current; Validated | Partial: non-scalar messages are observable but not yet selectable as typed mapping sources. |
| Machine-profile v1 OSC, physical-MIDI input, and control-slot persistence | Own machine-local OSC transport/endpoints, one physical MIDI input binding, and logical control-slot assignments without constraining payloads or absorbing routes, scenes, UI preferences, or telemetry. | `app-runtime`, `app-contract`, `adapter-contract` | Runtime owner, app contract owner | `MachineProfileDocument`, `MidiRouter`, `ofApp::loadMachineProfileSettings`, `ofApp::saveMachineProfileMidiInputs`, `ControlMappingHubState`, `DevicesPanel`, `docs/schemas/machine_profile.schema.json` | Direct serial, Router UDP, Browser input selector, physical MIDI input, Device Mapper, logical controller slots, machine recovery | Exact integer `schemaVersion: 1` is current. The OSC section is required; optional `midi` and `controlSlots` are independently authoritative when present. Missing version is invalid; future primary profiles reject without backup or fragmented-legacy downgrade. | `python tools\validate_machine_profile_contract.py --check`; `BrowserFlowTest` | `docs/examples/machine_profile_example.json`, `tools/testdata/machine_profile/*.json` | Legacy OSC, MIDI selector, and slot files are copied only when a valid, unblocked profile omits the corresponding canonical owner. Present-empty MIDI disables input. A configured port resolves only by one exact case-sensitive name; hints, substrings, indices, and port-zero fallback are never canonical. Slot and MIDI edits publish recoverably and restore runtime state on failure. | Current; Validated for OSC, physical-MIDI input, and logical-slot lanes | Partial: audio, video, display, and paths remain on named legacy readers until their transactional endpoint seams land. |
| Operator preferences v1 | Own independently authoritative local Browser/HUD, hotkey, package activation/next-load preset, and active mapping-bank choices without absorbing routes, bank definitions, Scene state, or machine endpoints. | `app-runtime`, `app-contract`, `adapter-contract` | UI shell owner, app contract owner | `PreferencesDocument`, `ControlMappingHubState`, `HotkeyManager`, `LayerLibrary`, `ofApp`, `docs/schemas/preferences.schema.json` | Browser/HUD, hotkey editor, package activation/preset selection, active mapping bank | Exact integer `schemaVersion: 1` is current. Present sections are authoritative; omitted sections delegate to their named legacy reader. Invalid or future canonical input blocks writes and legacy takeover. | `python tools\validate_preferences_contract.py --check`; `BrowserFlowTest` | `docs/examples/preferences_example.json`, `tools/testdata/preferences/*.json` | Writes validate the complete candidate, persist before adoption, preserve unrelated sections, and roll persisted/live state back on failure. | Current; Validated | No for the five current sections; new preference lanes require explicit schema and adapter coverage. |
| Portable machine-state separation | Keep portable Scene, layer, and package JSON free of local physical webcam selection and machine-specific filesystem paths. | `app-contract`, `artist-sdk`, `adapter-contract` | App contract owner | `tools/validate_portable_machine_state.py`, `tools/testdata/portable_machine_state/classification.json` | Layer assets, Scenes, package/catalog fixtures, public extraction | Classification schema v1 explicitly separates portable globs from local and legacy-compatibility fixtures. Portable webcam assets use semantic/runtime selection rather than `deviceName` or `deviceIndex`; portable string values cannot be absolute local paths. | `python tools\validate_portable_machine_state.py --check` | `tools/testdata/portable_machine_state/cases.json` plus the classified portable artifact set | Local machine profiles and explicitly named legacy fixtures are allowed only through catalog classification; new portable roots must be added to the catalog. | Current; Validated | No for the classified artifact set; expand the catalog when new portable stores are introduced. |
| Signal Control receive and OSC input selection | Synaptome Browser receive-mode selection plus Signal Control host-audio scalar/waveform routes into Router UDP. | `app-contract`, `adapter-contract` | App contract owner | Machine-profile v1 OSC section, legacy `synaptome/bin/data/config/osc-input.json` compatibility reader, `docs/contracts/signal_control_integration.md` | Signal Control, Synaptome Browser, `AudioAnalysisBus`, audio-reactive layers | Machine-profile schema v1 owns new endpoint writes. The legacy OSC-input schema v1 remains read-only compatibility input. Keep `directSerial` and `routerUdp` modes; `/sensor/host/<source>/waveform` allows 64, 128, or 256 float samples. | `python tools\validate_signal_control_receive_contract.py --check`; `python tools\validate_machine_profile_contract.py --check` | `docs/examples/osc_input_example.json`, `docs/examples/machine_profile_example.json`, `tools/testdata/signal_control/expected_receive_contract.json` | Route or freshness changes must update the fixture/schema/docs and Tensegrity producer docs together. Endpoint mode changes publish recoverably through the local machine profile. | Current; Validated | No for the current host-audio receive path; typed non-scalar mapping semantics remain a separate follow-up. |
| Parameter ID manifest | Stable parameter IDs consumed by scenes, mappings, OSC, MIDI, and UI. | `app-contract` | App contract owner | `docs/contracts/parameter_manifest.json` | Scene persistence, MIDI map, OSC map, Browser, HUD feeds | Manifest schema is v1; public ID renames require deprecation or migration notes. | `python tools\gen_parameter_manifest.py --check` | `docs/contracts/parameter_manifest.json`, `docs/examples/parameter_example.json` | Rename/deprecation policy required for any public parameter ID; strict target validation is tracked by Parameter target references. | Current; Validated | Partial: broader vocabulary/range policy remains follow-up. |
| Parameter target references | Strict check that persisted scenes, MIDI maps, OSC maps, audio modifiers, device maps, and HUD widget IDs point at known parameter IDs, Console slot templates, or layer catalog IDs. | `app-contract` | App contract owner | `docs/contracts/parameter_manifest.json`, committed runtime-state fixtures | Scene persistence, MIDI map, OSC map, Browser, HUD feeds | Strict validator must pass before release/extraction; intentional non-parameter catalog IDs must resolve through the layer catalog. | `python tools\validate_parameter_targets.py --strict --contract-fixtures` | `tools/testdata/runtime_state/config/*.json`, `tools/testdata/runtime_state/layers/scenes/*.json`, `tools/testdata/device_maps/*.json` | Target ID renames require fixture migration or explicit compatibility aliases; live app-written drift must not block the strict gate. | Current; Validated | No for current fixture set; extend coverage as new target-bearing files are promoted. |
| Layer asset catalog | JSON layer asset shape loaded by the visual layer library. | `app-contract` | App contract owner | `synaptome/bin/data/layers/**/*.json`, `synaptome/src/visuals/LayerLibrary.*`, `synaptome/src/ofApp.cpp` | Scene runtime, console/layer stack, Browser | Schema version pending; current assets are source examples. | `python tools\layer_catalog_regression.py --check` | `tools/testdata/layer_catalog/expected_catalog.json`, `synaptome/bin/data/layers/**/*.json` | Runtime layer types must map to `ElementDescriptor::typeId`, never display labels. | Current; Validated | Partial: plugin/source-registration policy remains pending. |
| Modular layer family lifecycle | Shared-runtime families preserve public identity, own deterministic randomness, expose complete defaults, and separate model identity from live modes. | `app-contract`, `artist-sdk` | Runtime owner, app contract owner | `CircuitTraceLayer`, `AgentFieldLayer`, `FlockingLayer`, family catalog assets | Scenes, mappings, Browser, layer authors | Asset IDs, runtime types, registry prefixes, and parameter suffixes remain stable; new randomness must use persisted seed/reseed state. | `python tools\validate_modular_layer_families.py` | Ten canonical assets under `synaptome/bin/data/layers/generative` | Migrate in place; retain aliases for old config shapes and avoid novelty assets where presets suffice. | Current; Validated | Partial: additional legacy families still need migration and live visual review. |
| Cellular Fields lifecycle | Distinct cellular algorithms share deterministic lifecycle and descriptor infrastructure without being collapsed into one runtime. | `app-contract`, `artist-sdk` | Runtime owner, app contract owner | `GameOfLifeLayer`, `ExcitableMediaLayer`, their catalog assets | Scenes, mappings, Browser, layer authors | Existing IDs/types/prefixes/suffixes stay stable; canonical defaults match registration while legacy aliases remain readable. | `python tools\validate_cellular_fields.py` | `game_of_life.json`, `excitable_media.json` | Keep algorithm identities separate and retain compatibility aliases. | Current; Validated | Partial: Lenia and Reaction Diffusion remain to migrate. |
| Circuit Lenia presentation | Shared Lenia simulation with a fixed catalog-selected hard-isocontour circuit view. | `app-contract`, `artist-sdk` | Runtime owner, app contract owner | `LeniaLayer`, `lenia.json`, `circuit_lenia.json`, committed default route fixture | Scenes, mappings, Browser, layer authors | Organic Lenia retains its presentation; Circuit Lenia owns a stable ID/prefix, complete circuit defaults, and seven ordinary editable OSC routes. | `python tools\validate_circuit_lenia.py` | Organic and Circuit Lenia catalogs, `tools/testdata/circuit_lenia/default_mapping_routes.json`, plus BrowserFlow lifecycle and mapping-snapshot coverage | Share simulation state and lifecycle; keep circuit-only controls out of the organic UI and all OSC addresses out of the renderer. | Current; Validated | Partial: live visual acceptance and default tuning remain. |
| Artist SDK example fixture | Minimal public source/catalog/scene bundle for an artist-authored layer. | `artist-sdk` | App contract owner | `docs/examples/artist_sdk/**`, `docs/examples/layer_packages/signal_bloom/**`, generated registration outputs | Layer authoring docs, Browser catalog, Console scenes, mappings, Package v1 authoring | The readable fixture retains an explicit registration example; the shipping package leaf is creator-only and its complete Runtime contract is generated from Package v1. | `python tools\validate_artist_sdk_example.py --check`; `python tools\generate_element_package_registrations.py --check` | Artist SDK and Signal Bloom package fixtures | Type, parameter, creator-symbol, catalog, scene, or registration-set changes must update generated output and snapshots together. | Current; Validated | Partial: broader public guide and representative migration remain SEAC-12. |
| Layer package activation and controlled discovery | Read-only package/content discovery plus explicit catalog promotion and operator-owned preset/mapping transactions. | `artist-sdk`, `app-runtime` | Runtime owner, app contract owner | Package v1, controlled registration, `controlled_package_discovery_v1.py`, `LayerLibrary::publishDiscoveredCandidate`, `PackageControlTransactions` | Browser, catalog, Console, package authors | Schema v1; discovery/activation default false, scan is construction-free, and only exact registered types publish. Presets/mappings remain separate. | `python -m pytest tests\test_controlled_package_discovery_v1.py -q`; BrowserFlow | Signal Bloom, generated STL, Browser discovery/activation, confidence and package benches | Preserve path-independent IDs/signatures; fail collisions closed; retain prior inspection on refresh failure; never mutate active composition during scan/removal/replacement. | Current; Validated | Native loading remains SEAC-11 and is not required by implemented fixtures. |
| Media catalog intake | Explicit, provenance-aware video onboarding without folder scanning or undocumented binaries. | `app-contract`, `adapter-contract` | App contract owner | `synaptome/bin/data/config/videos.json`, `docs/schemas/media_catalog.schema.json`, `docs/contracts/media_catalog.md` | `VideoCatalog`, media layers, Browser-facing catalog metadata, artists and operators | Schema version 1; committed media requires a stable ID, revision, SHA-256, redistribution permission, and replacement history when revised. | `python tools\media_catalog_regression.py --check` | `docs/examples/media_catalog_example.json`, `tools/testdata/media_catalog/invalid_catalog_cases.json` | Preserve IDs only for the same conceptual asset; increment revision and record the previous hash; remove defaults/references before deletion. | Current; Validated | No for manifest intake and Browser-visible default-layer loading; folder scanning remains deferred. |
| Scene persistence schema | Saved Scene v1/v2 files and scene-last runtime state, including explicit mapping-snapshot ownership. | `app-runtime` | Runtime owner, app contract owner | `SceneStateDocument`, `ofApp::encodeSceneJson`, `ofApp::loadScene`, committed scene fixtures | Runtime, console/layers, mappings, HUD/window state | Missing or explicit Scene v1 is normalized in memory; writers emit Scene v2. Router and active-bank omission preserve their live/global owners. Legacy slot-assignment fields remain readable and validated, but normal load preserves the machine profile and new named/autosave writers omit them. New saves retain the active mapping bank and emit mapping-bank route v1. | `python tools\validate_scene_persistence_contract.py --check` | `tools/testdata/scene_persistence/expected_scene_contract.json`, `tools/testdata/scene_persistence/slot_assignment_ownership_cases.json`, `tools/testdata/runtime_state/layers/scenes/*.json`, `tools/testdata/runtime_state/config/scene-last.json` | Unsupported future Scene or embedded mapping versions reject before application and cannot silently downgrade through backup. Importing legacy slot assignments requires a separately explicit action. | Current; Validated | Partial: live-window/show-machine fixture coverage remains follow-up, not a current extraction blocker. |
| Scene and mapping recovery | Transactional scene publication, recoverable machine-profile control-slot and physical-MIDI writes, and non-destructive MIDI/OSC mapping import. | `app-runtime`, `adapter-contract` | Runtime owner, app contract owner | `ofApp::buildSceneApplyPlan`, `ofApp::publishScenePlan`, `writeJsonRecoverably`, `ofApp::saveMachineProfileMidiInputs`, `ControlMappingHubState::importSlotAssignmentSnapshotTransactional`, `MappingBankDocument`, `MidiRouter::importMappingSnapshot`, `MidiRouter::publishInputBindingSnapshot`, `DevicesPanel` | Scene runtime, Browser, HUD, MIDI/OSC routing, physical MIDI input, controller slot assignments | Malformed input or failed route/profile publication restores working assignments, routes, and the prior MIDI binding. Future primaries reject without activating older backups. Scene and machine-profile writes verify temporary JSON and retain a last-known-good backup until commit. | `python tools\validate_scene_display_transaction.py`; `python tools\validate_mapping_bank_contract.py --check`; `python tools\validate_machine_profile_contract.py --check` | Legacy/current mapping-bank fixtures, `tools/testdata/machine_profile/midi_binding_cases.json`, and native version, slot transaction, MIDI resolver/reconnect/UI rollback, explicit-empty, publication-failure, save/reload, backup, and no-downgrade scenarios in `BrowserFlowTest` | Any change to ownership, precedence, resolver, rollback, discriminator, or write promotion must update the source gate and native recovery scenarios together. | Current; Validated | Partial: live physical-MIDI hardware and physical dual-window restart rehearsal remain follow-up. |
| Device-map schema | Logical device/control slot mapping for controllers and adapters. | `app-contract` | App contract owner | `synaptome/bin/data/device_maps/*.json`, `tools/testdata/device_maps/synthetic_controller.json` | Browser, MIDI/OSC mapping, device adapters | Schema version pending; logical role families are fixture-backed. | `python tools\device_map_regression.py --check` | `synaptome/bin/data/device_maps/MIDI Mix 0.json`, `tools/testdata/device_maps/synthetic_controller.json`, `tools/testdata/device_maps/expected_logical_slots.json` | New device support should be data/schema first; intentional role/binding changes must update the golden logical-slot fixture. | Current; Validated | No for current device-map logical slots; extend when target/action bindings become public. |
| Mapping-bank v1 route persistence | Owner-scoped MIDI CC/button and OSC applied-route persistence using the actual flat `MidiRouter` vocabulary. | `app-contract`, `app-runtime` | Runtime owner, app contract owner | `MappingBankDocument`, `MidiRouter`, `docs/schemas/mapping_bank_route_snapshot.schema.json` | Scene and operator mapping stores, Browser, parameter runtime | Missing `schemaVersion` is legacy unversioned input; exact integer v1 is current; invalid/future versions reject before mutation. Canonical writers emit all four route arrays. The `{version,bank,mappings}` public example remains a distinct interchange artifact. | `python tools\validate_mapping_bank_contract.py --check`; `python tools\validate_configs.py --public-app` | `tools/testdata/mapping_bank/*.json`, legacy embedded Scene router fixtures, `BrowserFlowTest` | Preserve all legacy route fields in memory; do not rewrite sources on read or fall back from a future primary. Standalone `device`/`deviceIndex` remain read-only legacy selection only while a valid canonical profile omits `midi`; canonical route saves omit them once the machine-profile owner is active. | Current; Validated | Partial: live hardware evidence remains follow-up; global bank definitions and active-bank choice have independent owners. |
| Operator global bank definitions v1 | Strict custom global-bank/control definitions independent from applied routes, active-bank choice, and Scene ownership. | `app-contract`, `app-runtime` | Runtime owner, app contract owner | `BankDefinitionsDocument`, `BankRegistry`, `ofApp`, `docs/schemas/bank_definitions.schema.json` | App startup, legacy Scene compatibility, bank registry | Exact integer `schemaVersion: 1` is current. Canonical presence wins; absent canonical input delegates to legacy Scene global banks, while present empty selects the built-in Home fallback. Invalid/future canonical input blocks legacy takeover and publication. | `python tools\validate_bank_definitions_contract.py --check`; `BrowserFlowTest` | `docs/examples/bank_definitions_example.json`, `tools/testdata/bank_definitions/*.json` | Reads never auto-migrate or rewrite legacy Scene banks. Whole-document publication is recoverable and rolls persisted/live state back on failure. The publication surface is currently an app seam, not an operator editor. | Current; Validated | Partial: an operator-facing global-bank editor remains follow-up. |
| HUD layout/feed persistence | HUD widgets, feed IDs, overlay layout, and operator preferences. | `app-runtime` | UI shell owner | `HudRegistry`, `HudFeedRegistry`, `OverlayManager`, HUD layer assets, committed runtime-state fixtures | Projection/control windows, UI shell, scene load path | Snapshot schema v1; dynamic feed payloads and timestamps remain runtime-local. | `python tools\validate_hud_layout_contract.py --check` | `tools/testdata/hud_layout/expected_widgets.json`, `tools/testdata/runtime_state/config/control_hub_prefs.json`, `tools/testdata/runtime_state/config/console.json`, `synaptome/bin/data/layers/hud/*.json` | Widget ID/feed/layout changes must update the golden HUD fixture; use `--live` for local operator-state smoke checks. | Current; Validated | Partial: current HUD layout/feed fixtures are validated; app-native/live-window coverage can expand as follow-up. |
| Console layout/secondary display persistence | Composition-layer Console slots and secondary display state. | `app-runtime` | Runtime owner, UI shell owner | `ConsoleStore`, committed runtime-state fixtures | Console renderer, projection/control windows, scene load path | Snapshot schema v1; monitor coordinates, timestamps, and sensor warm-start snapshots are validated as shape but excluded from golden churn. Logical hardware control slots are machine-profile state, not Console layout. | `python tools\validate_console_layout_contract.py --check` | `tools/testdata/console_layout/expected_console_contract.json`, `tools/testdata/runtime_state/config/console.json` | Scene transaction implementation and manual smoke have passed; use `--live` for local operator-state smoke checks. | Current; Validated | Partial: app-native/live-window fixture coverage remains follow-up, not a current extraction blocker. |

## Staging-Only Boundaries

Helper/radio decode, embedded UI exchange, firmware payload quarantine, private deployment maps, and governance workflow checks are not public Synaptome app contracts. In the source workspace, broader validation may still check those areas while they wait for their own package or helper repo. They must not become dependencies of the standalone public runtime.

## G0 Exit Notes

- The app extraction boundary is validated, and the final local public import has passed the extraction ladder; remote push still needs an empty public repo and remote CI.
- Browser/Control Panel validation is app-facing. The app-native runner is `synaptome/tests/BrowserFlowTest/BrowserFlowTest.vcxproj` plus `python tools\run_control_hub_flow.py`.
- Public docs should describe Synaptome as an openFrameworks runtime package, not as a replacement for Visual Studio or openFrameworks.
- Scene/display transaction work has landed; remaining public-release polish is reviewed branch/fresh extraction plus live-window fixture and display-resilience follow-up.

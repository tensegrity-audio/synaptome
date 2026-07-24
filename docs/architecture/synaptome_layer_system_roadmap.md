# Synaptome Layer System Roadmap

Status: Active supporting architecture; reviewed 2026-07-19. Current priority
and execution state are owned by
[`../project_ops/roadmap.md`](../project_ops/roadmap.md). This document owns the
implementation sequence for packages, folder discovery, generated layer
assets, package-declared parameters, layer presets, visible mapping presets,
Browser integration, validation, and single-layer testing.

Read with: [`synaptome_artist_sdk.md`](synaptome_artist_sdk.md),
[`synaptome_biological_layer_roadmap.md`](synaptome_biological_layer_roadmap.md),
[`../project_ops/synaptome_layer_design_standards.md`](../project_ops/synaptome_layer_design_standards.md),
[`../project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`](../project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md).

## Purpose

The layer system should make Synaptome feel like a real creative instrument
rather than a hand-wired C++ app.

The practical goal is:

```text
put a layer package or supported content file in the right folder
  -> Synaptome discovers it
  -> validates what it declares
  -> shows it in the Layer Browser
  -> exposes standard parameters, options, presets, and mapping presets
  -> can save, restore, map, and test it safely
```

This roadmap is about layer lifecycle and quality of life: how layers are
authored, discovered, inspected, controlled, preset, validated, and tested.
The author-facing evolution target for individual layers is defined in
[`synaptome_layer_design_standards.md`](../project_ops/synaptome_layer_design_standards.md):
layers should move from handcrafted runtime islands toward self-describing
creative modules without churning stable public IDs.

## What Belongs Here

- Layer package layout and `layer.package.json`.
- Folder-driven Browser organization.
- File-backed generated layer assets such as dropped STL files.
- Package-declared parameter surfaces.
- Static dropdown options and dynamic option source references.
- Layer-local presets and preset banks.
- Package-supplied OSC/audio/control mapping presets that appear in the Browser
  mapping surface.
- Layer catalog and parameter manifest generation from package metadata.
- Source registration now, generated registration or loaders later.
- Single-layer static validation and runtime/offscreen bench.

## What Does Not Belong Here

These systems can feed layers, but they are not layer-system work:

- BPM, beat detection, clock source selection, onset/downbeat confidence, and
  transport fallback behavior.
- Host audio capture policy.
- MIDI/OSC device ownership outside layer mapping presets.
- Display/window/HUD persistence except where a layer package declares a
  layer-specific preview or HUD capability.
- Full plugin/binary module loading policy beyond the layer package seam.

Those topics belong in the transport/reactivity, external contract, runtime, or
public SDK docs. Layer packages may reference those systems; they should not own
their implementation.

## Current Starting Point

Synaptome already has:

- C++ `Layer` subclasses.
- `LayerFactory` source registration.
- `LayerLibrary` JSON catalog ingestion.
- Browser-visible parameters through `ParameterRegistry`.
- Generated `docs/contracts/parameter_manifest.json`.
- Layer catalog regression fixtures.
- Scene persistence and target validation.
- A minimal artist SDK example fixture.

The weak spot is that layer truth is still scattered across C++ code, layer JSON
files, runtime parameter registration, generated manifests, scenes, mappings,
and Browser behavior. The package roadmap pulls those declarations into one
inspectable layer system.

Current package scaffold:

- `docs/schemas/layer_package.schema.json`
- `docs/schemas/layer_preset.schema.json`
- `docs/examples/layer_packages/signal_bloom/layer.package.json`
- `docs/examples/layer_packages/signal_bloom/presets/*.json`
- `tools/layer_package_discovery.py`
- `tools/validate_layer_packages.py`
- `tools/layer_package_catalog_regression.py`
- `tools/layer_package_parameter_manifest.py`
- `tools/testdata/layer_packages/expected_package_catalog.json`
- `tools/testdata/layer_packages/expected_package_parameter_manifest.json`
- `tools/testdata/layer_packages/expected_combined_layer_catalog.json`
- `tools/testdata/layer_packages/expected_combined_parameter_manifest.json`
- `docs/schemas/generated_layer_template.schema.json`
- `docs/schemas/generated_layer_sidecar.schema.json`
- `docs/examples/generated_layers/stl_models/generated_layer.template.json`
- `docs/examples/generated_layers/stl_models/tetrahedron.stl`
- `docs/examples/generated_layers/stl_models/tetrahedron.generated_layer.json`
- `tools/generated_layer_catalog_regression.py`
- `tools/testdata/generated_layers/expected_generated_layer_catalog.json`
- `docs/schemas/layer_browser_inspection_payload.schema.json`
- `tools/layer_browser_inspection_payload.py`
- `tools/testdata/layer_browser_inspection/expected_layer_browser_inspection_payload.json`

The package contract now has one bounded runtime seam. Tooling produces draft
package-only snapshots, opt-in combined catalog/manifest snapshots, generated
content snapshots, and a schema-checked read-only Browser inspection payload.
The Browser consumes that payload without instantiating packages, one reviewed
source-registered package can be activated explicitly, and a focused
offscreen lifecycle bench exists. Automatic package discovery/loading remains
a future phase.

## Where We Stand

The safe foundation is now in place. Synaptome can describe a layer package,
validate it, generate package-derived catalog/parameter outputs, and compare
those outputs beside the current runtime catalog and parameter manifest.

What is real today:

- A package folder can declare a layer through `layer.package.json`.
- Package and preset JSON shapes have draft schemas.
- The Signal Bloom fixture proves package metadata, parameters, presets,
  preset banks, visible OSC mapping presets, source-registration references,
  and bench metadata can live together in one package folder.
- Tooling can validate the package and fail early on missing files, duplicate
  IDs, invalid preset values, invalid ranges, or mapping targets that do not
  match package parameters.
- Tooling can generate package-only catalog and parameter snapshots.
- Opt-in combined checks can prove package-derived catalog/parameter entries do
  not collide with the current legacy runtime surfaces.
- A draft STL/model generated-layer template can turn a docs/example `.stl`
  file into a stable catalog-style entry with standard model parameters and no
  legacy asset ID conflicts.
- A draft read-only Browser inspection payload can combine package and
  generated-layer metadata without layer instantiation, scene mutation, runtime
  scanning, or canonical manifest changes.
- The Browser renders that payload as non-editable inspection rows.
- Static named option metadata now has one generated-layer fixture
  (`materialMode`) that flows into the generated catalog and inspection
  payload.
- Dynamic option-source metadata now has one generated-layer fixture
  (`materialPreset`) that names a future runtime provider without resolving it
  or rendering it in the Browser.
- Signal Bloom declares `transport.bpmMultipliers`; the app registers that
  provider, Browser inspection resolves its choices, and provider revisions
  invalidate cached rows.
- Missing provider defaults are marked and preserved without changing package
  metadata or stored values.
- Signal Bloom has explicit default-off source registration and a focused
  native lifecycle/offscreen bench.
- Active packages expose ordered preset banks as labeled Browser choices.
  Selection persists stable IDs in the ignored show-machine override and
  affects the next layer instantiation without mutating current scene or
  mapping state.

What is only draft tooling:

- `--include-packages` is a compatibility check, not runtime behavior.
- `synaptome/bin/data/layer_packages` is named as the future runtime package
  root, but the app does not yet scan it.
- Package-derived parameters are not yet part of the canonical
  `docs/contracts/parameter_manifest.json`.
- Package-derived catalog entries are not automatically activated by the
  Browser; inspection remains separate from activation.
- Generated-layer template output is a docs/tools fixture only; it does not
  mean Synaptome scans model folders at runtime.
- Static and resolved dynamic option metadata is visible in read-only Browser
  inspection; editable dropdown behavior is not implemented yet.
- The generated-layer `materialPreset` provider remains intentionally
  unresolved because generated content is still fixture-only.

What is not implemented yet:

- Runtime or Browser scanning of STL/model/media folders.
- Browser controls for package presets, preset banks, mapping presets, static
  dropdown options, or dynamic option providers.
- Runtime package loading or generated registration.

Current breakage risk is low because the new work is mostly schemas, fixtures,
and opt-in checks. Risk rises when package outputs start changing Browser or
runtime behavior.

| Work Area | Current Risk | Why |
| --- | --- | --- |
| Static package schemas, fixtures, and validators | Low | They do not change runtime loading and fail in tools first. |
| File-backed generated layer fixtures | Medium | Generated IDs, folder rules, and standard parameters need to stay stable. |
| Read-only inspection payload | Low | It is schema-checked fixture output and cannot claim runtime loading, instantiation, or scene mutation. |
| Browser package/preset/mapping UI | Medium-High | UI state, scenes, and operator edits can conflict if merge rules are vague. |
| Canonical manifest/catalog integration | Medium-High | Package metadata, runtime registration, scenes, and mappings must not drift. |
| Runtime package loading or generated registration | High | This touches installation, registration, C++ build/runtime seams, and compatibility policy. |

## Safety Strategy

The end goal is still simple: drop a supported package or content file into the
right folder and have Synaptome discover, validate, browse, map, preset, save,
restore, and test it safely. The way to get there is deliberately slow: every
new capability must be visible in static tooling before it changes runtime
behavior.

Rules for minimizing breakage:

- Keep current runtime behavior canonical until a new path has fixtures,
  validators, package-only snapshots, and combined compatibility snapshots.
- Keep `--include-packages` opt-in until Browser/runtime loading is explicitly
  implemented and tested.
- Do not move package-derived parameters into
  `docs/contracts/parameter_manifest.json` until package declarations can be
  compared against runtime parameter registration.
- Do not rename public IDs without a migration or alias policy.
- Do not let generated IDs depend on absolute local paths.
- Do not hide package-supplied OSC/audio/control mappings inside parameters;
  they must appear as editable mapping rows.
- Introduce Browser behavior as read-only inspection first, then opt-in
  activation, then default behavior after scene/mapping compatibility is proven.
- Runtime package loading or generated registration only comes after static
  package checks, combined checks, Browser inspection, and a single-layer bench
  exist.

Promotion ladder:

| Step | Capability | Runtime Impact | Gate Before Next Step |
| --- | --- | --- | --- |
| 0 | Draft package schemas, fixture, validator | None | Package fixture validates. Done. |
| 1 | Package-only catalog/parameter snapshots | None | Package snapshots are stable. Done. |
| 2 | Combined package/runtime compatibility snapshots | None | No package/runtime ID conflicts. Done. |
| 3 | File-backed generated-layer template fixture | None | A dropped-file fixture produces stable generated IDs and parameters. Done for one STL fixture. |
| 4 | Schema-checked read-only inspection payload | None | Package and generated-layer metadata can be inspected without runtime loading, instantiation, or scene mutation. Done for current fixtures. |
| 5 | Browser read-only package/generated-layer inspection UI | Read-only UI only | Browser can display inspection data without loading or mutating scenes. |
| 6 | Browser opt-in package activation | Controlled UI path | Scene save/load and target validation pass with package-derived entries. |
| 7 | Canonical manifest/catalog integration | Contract change | Runtime registration agrees with package declarations or has explicit exceptions. |
| 8 | Runtime package root scanning | Runtime discovery | Disabled-by-default smoke path, strict conflict handling, no source-edit promise. |
| 9 | Generated registration or loader evolution | Runtime/build behavior | Single-layer bench and compatibility policy are in place. |

## Tracked Layer Gaps

These gaps are folded into this roadmap. `docs/contracts/contract_gaps.md`
indexes them, but this document owns their meaning and next actions.

| ID | Gap | Current State | Next Action |
| --- | --- | --- | --- |
| CG-02 | Layer asset golden fixtures | Layer catalog regression mirrors `LayerLibrary` ingestion and checks factory registrations; the artist SDK fixture proves a minimal source/catalog/scene path. | Use the catalog and SDK fixtures to drive package fixture design and the public layer authoring guide. |
| CG-07 | Public parameter vocabulary | A first reusable vocabulary exists and the artist SDK fixture enforces selected suffix/type families; broader range/unit policy remains advisory. | Tighten suffix/range/unit rules after package fixtures and real examples agree. |
| CG-11 | Artist SDK compatibility slice | The first public path is honest source registration with a validated source/catalog/scene fixture. | Keep source-registration language honest while package tooling evolves toward generated registration or a loader. |
| CG-12 | Layer package layout and schema | Draft schemas, a Signal Bloom fixture, shared package discovery roots, and static validation now exist. | Extend the explicit package roots toward template-backed content folders and runtime Browser loading. |
| CG-13 | Manifest generation from package parameters | Package-derived parameter manifest snapshots expand package suffixes through `registryPrefix`; `gen_parameter_manifest.py --include-packages` now writes/checks a draft combined manifest without changing the canonical manifest. | Decide when package-derived entries become part of the canonical `parameter_manifest.json` rather than a draft combined gate. |
| CG-14 | Dropdown option metadata and dynamic providers | Matching live package parameters use one labeled picker for static `options[]` and registered `optionsSource` choices. Selection updates the existing registry value, provider revisions close stale pickers, and unavailable current values remain preserved until explicit replacement. | Reuse the same metadata/picker path on the next packaged layer and keep raw numeric/string editing available only where no choices are declared. |
| CG-15 | Layer preset package contract | Package-owned suffix-based presets and ordered banks are schema-validated. The Browser persists a stable bank/preset selection in the operator-local override and applies it on the next layer load with tested precedence and rollback. | Keep current scene values authoritative; consider live preset application only after value provenance and transactional rollback exist. |
| CG-16 | Single-layer package validator and runtime bench | Signal Bloom now passes a focused static check and native lifecycle/offscreen bench; full package-vs-runtime descriptor comparison and pixel/non-blank output checks remain. | Compare every runtime descriptor against package declarations, then add optional rendered-output assertions. |
| CG-17 | Folder-driven discovery and file-backed generated layers | Current catalog behavior relies on explicit layer JSON entries; STL-style dropped files do not yet have a standard generated asset/template path. | Define discovery roots, folder-to-Browser rules, stable generated IDs, sidecar overrides, and template schemas for file-backed generated layers. |
| CG-18 | Package OSC mapping presets | Packages can ship validated suffix-based OSC/audio/control mapping suggestions, and activation records but never applies the chosen mapping preset. Editable Browser mapping rows do not exist yet. | Add explicit apply/edit controls with slot expansion, conflict preview, and rollback while retaining scene/operator ownership. |

## Roadmap

### Phase 1: Package Contract

Create the first real package shape without changing runtime loading behavior.

Deliverables:

- `docs/schemas/layer_package.schema.json`. Done for the first draft.
- One minimal package fixture. Done for Signal Bloom.
- Static validation for required IDs, metadata, defaults, parameters, modes,
  presets, media references, test metadata, and compatibility expectations.
  First validator is in place.
- Static package-derived catalog and parameter manifest snapshots. Done for the
  Signal Bloom fixture.
- Clear coexistence rules for current `*.layer.json` assets. Done for the draft
  tooling: legacy assets remain canonical unless `--include-packages` is used,
  and duplicate IDs are treated as combined-check conflicts.

Success means Synaptome can inspect a package folder and say whether the package
is structurally valid before touching the layer implementation.

### Phase 2: Folder Discovery

Make folder placement meaningful.

Deliverables:

- Configured layer package roots. Current policy:
  `docs/examples/layer_packages` is the tracked fixture root, and
  `synaptome/bin/data/layer_packages` is the future app/runtime install root.
- Folder-to-Browser category/group/label defaults.
- Conflict rules between folder-derived organization and manifest overrides.
- Stable asset/package IDs that do not depend on local absolute paths.

Success means package folders can be reorganized for Browser usability without
breaking scene or mapping IDs.

### Phase 3: File-Backed Generated Layers

Support content drops that automatically become Browser entries through a
template.

Primary example:

```text
drop model.stl into a configured STL folder
  -> Synaptome creates a generated model-layer catalog entry
  -> the STL viewer/importer layer supplies the implementation
  -> the generated asset gets standard model parameters
```

The dropped STL file is not its own C++ layer. The reusable STL viewer is the
layer; the file is content that instantiates it.

Standard model parameters should include visibility, opacity, scale, position,
rotation, material color, solid/wireframe mode, lighting, spin, normals, and
reload/recenter actions where relevant.

Deliverables:

- Template schema for file-backed generated assets. Done for the first STL
  template draft.
- Stable generated content IDs. Done for one docs/example STL fixture.
- Optional sidecar manifest support for label, tags, physical scale, and
  defaults. Done for the first sidecar draft.
- A fixture proving one dropped file becomes a validated catalog entry. Done
  for `tetrahedron.stl`.

Still pending:

- Runtime/Browser folder scanning.
- Broader content kinds beyond STL.
- Thumbnail/media preview metadata.
- Generated-layer parameter manifest output, if needed before Browser
  inspection.

### Phase 4: Package Parameters And Manifest Generation

Make package parameter declarations the public source of truth.

Deliverables:

- Parameter suffix declarations in package metadata.
- Type, range, units, default, label, description, deprecation metadata.
- Static `options[]` for named choices.
- `optionsSource` references for runtime-owned choices.
- Manifest generation that expands suffixes through `registryPrefix`.
- Comparison against runtime registration when a runtime seam exists.

Success means public parameters are not hand-edited into generated manifests and
do not silently drift away from what the layer code registers.

Current scaffold status: `tools/layer_package_parameter_manifest.py` can already
expand package-declared suffixes into package-scoped public IDs and Console slot
templates for the fixture. `tools/gen_parameter_manifest.py --include-packages`
can now create a draft combined manifest for compatibility testing, while the
default `tools/gen_parameter_manifest.py --check` remains the canonical current
runtime manifest.

Generated-layer option metadata has also started: the STL template fixture
declares one static `materialMode` string option list and one dynamic
`materialPreset` `optionsSource` provider reference. Both now appear in the
generated-layer catalog snapshot and the read-only Browser inspection payload.

### Phase 5: Layer Presets And Preset Banks

Make layer-local value bundles first-class.

Deliverables:

- `docs/schemas/layer_preset.schema.json`.
- Bundled preset files.
- Ordered preset banks for fast performance toggling.
- Operator-local user preset policy.
- Scene merge behavior for selected preset vs explicit edited values.

Success ultimately means a performer can quickly switch a layer between named
parameter states without changing scenes or hiding an algorithm swap.

Current runtime seam: the Browser exposes labeled preset banks for active
packages and persists stable IDs in the operator-local activation override.
The choice applies to the next layer instantiation. Opt-in activation merges
package defaults, the selected preset, and explicit activation parameters in
that order; existing scene load then wins. Live mutation of a running layer is
intentionally deferred until parameter provenance and rollback are explicit.

### Phase 6: Visible Mapping Presets

Let packages suggest default OSC/audio/control reactions without hiding them.

Deliverables:

- Package mapping preset metadata.
- Target suffix expansion into the selected console slot.
- OSC pattern, range, smoothing, deadband, blend, and relative/absolute fields.
- Conflict behavior when a scene or operator map already uses the source.
- Browser rows showing package-supplied mappings as editable mappings.

Success means a layer can load with useful reactive defaults, while the user can
see, disable, retarget, or edit those mappings.

Current ownership rule: mappings are suggestions and activation records but
does not apply them. Scene/operator mappings retain ownership until an explicit
Browser apply/edit flow exists.

### Phase 7: Browser Integration

Make the Browser consume the package system directly.

Deliverables:

- Read-only inspection payload. Done for current package and generated-layer
  fixtures.
- Inspection payload schema. Done for the current draft payload.
- Static and dynamic option metadata fixtures. Done for one generated STL
  template.
- Package-discovered catalog sections.
- Named dropdown rendering for `options[]`. Done on matching live package
  parameters.
- Dynamic option rendering for registered `optionsSource` providers. Done on
  matching live package parameters with unavailable-value preservation and
  stale-picker cancellation.
- Preset-bank controls. Done for active packages as operator-local, next-load
  selection with stable IDs and rollback on persistence failure.
- Mapping-preset activation controls.
- Manifest-first inspection so browsing a layer does not require expensive or
  side-effectful setup.

Current status: the Browser consumes the runtime inspection payload as a
separate read-only category, resolves registered option providers, decorates
matching live parameter rows with labeled choices, and exposes an
active-package preset picker through host callbacks. Native coverage proves
inspection rows carry no live parameter pointers, labeled selection updates the
existing registry value, unavailable values are preserved, and preset
persistence does not mutate inspection, catalog, scene, or mapping state.

Success means the Browser becomes an authoring and performance surface for the
package contract, not a loose reflection of runtime state.

### Phase 8: Static Package Command

Add a fast command for one package.

Target shape:

```powershell
synaptome-layer-check --package layers/generative/schooling/layer.package.json
```

It should validate structure, references, IDs, parameters, options, presets,
mapping presets, generated manifest output, catalog output, and compatibility
metadata.

Success means authors can check a package without launching the app.

Implemented first shape:

```powershell
python tools\synaptome_layer.py check docs\examples\layer_packages\signal_bloom\layer.package.json
```

### Phase 9: Runtime/Offscreen Bench

Add the deeper test after the runtime seam exists.

Target shape:

```powershell
synaptome-layer-bench --package layers/generative/schooling/layer.package.json --preset default --frames 120
```

The bench should instantiate one layer, configure it, register parameters in an
isolated registry, compare runtime registration against package declarations,
update deterministically, draw offscreen, and report crashes or blank output.

Success means one layer can prove that it works without launching the full
performance UI.

Implemented lifecycle seam: `LayerPackageBench` creates Signal Bloom through
`LayerFactory`, registers 18 parameters, advances 240 frames, draws into an
offscreen framebuffer, verifies scene-value precedence, and rejects duplicate
factory registration. Pixel/non-blank image comparison remains future bench
depth.

### Phase 10: Registration Evolution

Keep source registration honest until a better mechanism exists.

Current public path:

```text
source files
  -> register_<packageId>.cpp
  -> LayerFactory::registerType(...)
```

Future options:

- generated registration from discovered packages,
- a centralized module manifest,
- a plugin/package loader with explicit dependency and binary compatibility
  rules.

Success means layer installation eventually avoids hand-editing `ofApp.cpp`
without pretending Synaptome has hot-loaded plugins before it does.

## Immediate Next Step

The safe vertical slice now includes a generated optional runtime adapter,
revisioned app-owned dynamic option resolution, explicit labeled live parameter
selection with unavailable-value preservation, and labeled operator-local
preset selection for the next layer load. Release, incremental, native-flow,
contract, and offscreen bench gates pass.

1. Add mapping-preset preview/apply/edit controls with slot expansion, conflict
   handling, and rollback;
   never auto-apply package mappings.
2. Keep scanning and generated/plugin registration disabled until duplicate,
   dependency, rollback, and ABI policy are tested.

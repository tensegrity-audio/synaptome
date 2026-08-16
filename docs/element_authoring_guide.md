# Synaptome Element Authoring Guide

This guide is the supported public path from an openFrameworks experiment to a
validated Synaptome element. It uses only public SDK headers, contracts,
fixtures, and commands; editing `ofApp.cpp` is not part of the workflow.

## 1. Choose The Smallest Extension Boundary

Use a built-in type when the implementation belongs in the host. Use a
validated source package when you own C++ source and can rebuild Synaptome.
Use data-only content when a bounded precompiled template already supports the
content family. Use an external process or reviewed transport bridge when the
experiment needs isolation or independent deployment.

Synaptome v1 does not load native element DLLs, hot-reload code, or install a
raw `ofApp` project. The decision and future reversal criteria are in
[`Native Module Policy v1`](architecture/native_module_policy_v1.md).

Reference implementations:

- source package: [Signal Bloom](examples/layer_packages/signal_bloom/layer.package.json);
- data-only content: [generated tetrahedron](examples/generated_layers/stl_models/tetrahedron.generated_layer.json);
- simple built-in: [`GridLayer`](../synaptome/src/visuals/GridLayer.h);
- content-backed built-in: [`StlModelLayer`](../synaptome/src/visuals/StlModelLayer.h);
- stateful built-in: [`LeniaLayer`](../synaptome/src/visuals/LeniaLayer.h).

## 2. Wrap A Raw openFrameworks Sketch

Move creative state out of the sketch application and into a `Layer` subclass.
The spine owns windows, events, devices, composition, persistence, and final
presentation. The element owns its algorithm, instance storage, and resources.

Implement these lifecycle hooks:

```cpp
class MyElement final
    : public Layer,
      public synaptome::element::ParameterBindable {
public:
    void configure(const ofJson& definition) override;
    void bindParameters(
        synaptome::element::ParameterBinder& binder) override;
    void setup(ParameterRegistry& registry) override;
    void update(const LayerUpdateParams& params) override;
    void draw(const LayerDrawParams& params) override;
};
```

`configure()` adopts definition/content defaults into owned storage.
`bindParameters()` binds every declared parameter exactly once. `setup()`
prepares resources and must not redeclare parameter metadata or reset values.
`update()` advances deterministic state. `draw()` uses the provided camera,
viewport, time, beat, and slot opacity and restores graphics state before
returning. Destruction must release owned CPU/GPU resources.

Do not retain a `ParameterBinder`, registry pointer, render target, host UI,
Runtime object, or host-owned GPU object. The frozen dependency boundary is
documented in
[`Element SDK v1`](architecture/element_sdk_v1_boundary.md).

## 3. Freeze Public Identity First

Choose stable values for:

- element type ID: implementation identity, such as `example.signalBloom`;
- definition/asset ID: configured presentation, such as
  `examples.signal_bloom`;
- registry prefix: persisted parameter namespace;
- parameter suffixes: local IDs such as `speed`, `visible`, or `seed`.

The full target is `<registryPrefix>.<parameterSuffix>`. Scenes, presets,
mappings, Browser controls, OSC, and MIDI may persist it. Never rename a
published identity for cosmetic cleanup. Add a documented alias or migration
and fixtures when a change is unavoidable.

## 4. Declare, Configure, Then Bind

Static declarations own parameter ID, kind, group, label, default, range,
step, units, options, quick access, aliases, and deprecation. Instance code
binds storage only:

```cpp
void MyElement::bindParameters(
    synaptome::element::ParameterBinder& binder) {
    binder.bind("visible", visible_);
    binder.bind("speed", speed_);
    binder.bind("seed", seed_);
}

void MyElement::setup(ParameterRegistry& registry) {
    (void)registry;
    // Allocate resources; do not call registry.add*().
}
```

Runtime applies authoritative declaration defaults before `configure()`, so a
definition can override them without setup resetting the result. Missing,
extra, duplicate, wrong-kind, or aliased-storage bindings fail preparation
before active composition changes.

## 5. Author Element Package v1

Start from the
[Signal Bloom package](examples/layer_packages/signal_bloom/layer.package.json)
and its creator-only
[source leaf](examples/layer_packages/signal_bloom/source/SignalBloomRegistration.cpp).
Declare package/type/definition identity, compatibility, dependencies,
capabilities, source inventory, parameter declarations, assets, presets,
mapping suggestions, confidence profile, fixtures, and migrations.

Validate one package:

```powershell
python tools\validate_layer_packages.py `
  docs\examples\layer_packages\signal_bloom\layer.package.json
```

Add the reviewed package to
[`element_package_registration_set_v1.json`](contracts/element_package_registration_set_v1.json),
then generate and verify the shared Runtime/build records:

```powershell
python tools\generate_element_package_registrations.py
python tools\generate_element_package_registrations.py --check
```

Controlled generation requires no package-specific edit to `ofApp.cpp`, the
built-in aggregate, the solution, or project source lists. A normal app rebuild
is required.

## 6. Definitions, Content, Presets, And Mappings

A definition selects one registered type and supplies stable label, category,
registry prefix, defaults, and content configuration. Multiple definitions may
share one type; organic Lenia and Circuit Lenia are the reference pattern.

Data-only families must use a reviewed bounded template backed by an exact
precompiled type. The generated tetrahedron uses
[`generated_layer.template.json`](examples/generated_layers/stl_models/generated_layer.template.json)
and `stlModel`; this does not imply arbitrary media-folder discovery.

Package presets are explicit parameter-value transactions. Package mappings
are suggestions only: operators preview and apply them separately. Neither is
activated merely because a package or content candidate was discovered.

## 7. Discover And Activate Explicitly

Controlled discovery is local, disabled by default, and construction-free.
Configure only bounded roots, produce a snapshot, inspect candidate identity,
signature, provenance, diagnostics, and exact type availability, then activate
an `available` candidate in Browser:

```powershell
python tools\controlled_package_discovery_v1.py `
  --config synaptome\bin\data\config\package-discovery.json `
  --prior synaptome\bin\data\config\package-discovery.snapshot.json `
  --output synaptome\bin\data\config\package-discovery.snapshot.json
```

Activation publishes a catalog definition; it does not assign a Console slot,
apply a preset/mapping, or replace a running instance. See
[`Controlled Package Discovery v1`](contracts/controlled_package_discovery_v1.md).

## 8. Prove Persistence And Provenance

Exercise declaration defaults, definition overrides, a preset, a Browser edit,
and a mapping edit as distinct origins. Assign the definition to a Console
slot, save a Scene, clear the slot, restore the Scene, and verify the exact
definition, prefix, values, and origins return. Portable Scenes must not contain
absolute local paths; local roots and activation choices remain local state.

For content-backed elements, remove or change the source while inactive and
active. Refresh may report unavailable/replacement state but must not destroy
or mutate the running instance. Failed prepare or restore must leave the prior
composition intact.

## 9. Add A Confidence Profile

Copy the nearest profile under
[`tools/element_confidence_profiles`](../tools/element_confidence_profiles):
Grid for a simple visual, `stl-model` for bounded content, or Lenia for
deterministic state and owned graphics resources. Add a dedicated contract
project when native source coverage is new.

Run the complete profile:

```powershell
python tools\run_element_confidence.py --profile grid --tier ci
python tools\run_element_confidence.py --profile stl-model --tier ci
python tools\run_element_confidence.py --profile lenia --tier ci
python tools\run_element_confidence.py `
  --package docs\examples\layer_packages\signal_bloom\layer.package.json `
  --tier ci
```

The complete tier checks construction-free declarations, declaration/live
parity, deterministic repetitions, hidden-context drawing, graphics-state
restoration, teardown, 200 reload cycles, memory growth, and timing. Renderer
and performance comparisons require a reviewed baseline for the same machine
and renderer class; an absent baseline is reported as a skip, never invented.

## 10. Validate The Host And Release Build

Run the public/static gates:

```powershell
python tools\gen_builtin_element_contracts.py --check
python tools\gen_parameter_manifest.py --include-packages --check
python tools\validate_parameter_targets.py --strict --contract-fixtures
python tools\validate_configs.py --public-app
python tools\validate_element_sdk_boundary.py
python tools\check_app_independence.py
python -m pytest tests\test_controlled_package_discovery_v1.py -q
```

Build and run `RuntimeCoreTest`, `LayerPackageBench`, and `BrowserFlowTest`, then
build `Release|x64` from both the physical checkout and supported
openFrameworks junction. In the real host, verify non-blank output, expected
presentation, controls, scene restore, removal/failure isolation, and no new
first-party warning or material performance regression.

## 11. Compatibility And Release Checklist

Before review:

- all public IDs, kinds, defaults, ranges, options, actions, and paths are
  unchanged or carry an explicit migration;
- bindings exactly match static declarations and setup preserves configured
  values;
- resources release cleanly and drawing restores graphics state;
- definitions, presets, mappings, Scenes, and provenance round-trip;
- discovery remains construction-free and activation remains explicit;
- complete confidence, native host, public contract, and Release gates pass;
- live visual acceptance is recorded on the target renderer;
- package version, compatibility, dependencies, provenance, and release notes
  describe the actual change.

The remaining legacy setup adapters are cleanup candidates, not permission to
copy their metadata-registration pattern into new elements.

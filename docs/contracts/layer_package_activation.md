# Layer Package Activation Contract

Status: Current opt-in source-registration contract, established 2026-07-19.

Synaptome can inspect draft package metadata without loading a layer. Runtime
activation is a separate, explicit allowlist in
`synaptome/bin/data/config/layer-packages.json`.

## Safety Boundary

- The top-level `enabled` flag defaults to `false`.
- Every package also has its own `enabled` flag. Both flags must be true.
- Activation references a reviewed catalog entry under
  `synaptome/bin/data/layers-optional/`; it does not scan folders.
- The layer implementation must already be compiled and registered with
  `LayerFactory`. This is still source registration, not plugin loading.
- Browser inspection reads `layer-package-inspection.json` directly and does
  not instantiate, configure, or call `setup()` on inspected packages.
- Browser inspection displays static choices and resolves registered
  dynamic-provider choices without rewriting stored defaults.
- Preset-bank selection is stored in the ignored show-machine local override
  and applies only when that package layer is next instantiated. It does not
  rewrite a running layer, scene, or mapping.
- Static and registered runtime option choices decorate matching live package
  parameters. Choosing a label updates the existing registry base/live value;
  missing provider values remain unchanged and visible as unavailable until
  the operator explicitly selects a valid replacement.

## Value And Mapping Precedence

Parameter values merge in this order:

```text
package defaults -> selected package preset -> activation parameters -> scene values
```

The rightmost value wins. Package mapping presets are suggestions. They remain
disabled by default and are never silently installed by activation; existing
scene and operator mappings retain ownership.

## Signal Bloom Example

The committed config is deliberately disabled. To run the vetted example
locally, set both `enabled` fields to `true`. An active package exposes its
ordered preset bank in the Browser. Choosing a labeled preset writes its stable
`presetBank` and `preset` IDs to `layer-packages.local.json`; the selected
preset and optional `parameters` object are merged on the next layer load
before the existing scene-load path applies saved values.

## Validation

```powershell
python tools\synaptome_layer.py check docs\examples\layer_packages\signal_bloom\layer.package.json
python tools\synaptome_layer.py runtime-adapter docs\examples\layer_packages\signal_bloom\layer.package.json --output synaptome\bin\data\layers-optional\examples.signal_bloom.json --check
python tools\signal_bloom_runtime_contract.py
python tools\validate_configs.py synaptome\bin\data\config\layer-packages.json synaptome\bin\data\layers-optional\examples.signal_bloom.json
msbuild synaptome\tests\LayerPackageBench\LayerPackageBench.vcxproj /p:Configuration=Release /p:Platform=x64
synaptome\tests\LayerPackageBench\x64\Release\LayerPackageBench.exe
```

The runtime-adapter command deterministically derives identity, defaults,
presets, preset-bank labels, and suggestion-only mapping metadata from the
reviewed package. The 20-scenario BrowserFlow native suite additionally proves
disabled activation is a no-op, explicit activation merges values in the
documented order, preset selection uses stable IDs and rolls back on
persistence failure, labeled static/runtime choices update the existing live
parameter while preserving unavailable values, mapping suggestions are not
auto-applied, and inspection/catalog/mapping state remains unchanged. The layer
bench compares all 18 runtime IDs, kinds, and float ranges against the package
and dispatches lifecycle/draw calls through openFrameworks stubs. Before
creation it also inspects Signal Bloom's registered `Visual` descriptor,
verifies its empty ordered action declaration, and proves that mutating copied
enumeration cannot change the registry. The package does not serialize that
runtime descriptor, so this is source-registration inspection rather than
package/full-descriptor parity. The bench does not create a graphics context,
execute a real FBO/GL render, read pixels, or detect blank output.

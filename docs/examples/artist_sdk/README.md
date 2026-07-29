# Artist SDK Example Fixture

Status: Public SDK fixture for the first source/catalog/scene authoring slice.
The readable example still compiles through the public SDK contract target.
Its Package v1 counterpart is the shipping source and is compiled through the
generated package build target.

Validator:

```powershell
python tools\validate_artist_sdk_example.py --check
```

This fixture shows the smallest honest public path for an openFrameworks artist:

```text
SignalBloomLayer source
  -> controlled built-in type registration
  -> Browser catalog entry
  -> Console scene slot
  -> parameters targeted by MIDI/OSC/sensor mappings
  -> media layer paired in the same saved scene
```

The shipping source is contained under
`docs/examples/layer_packages/signal_bloom/source`. Its header matches this
fixture, both implementations use the transitional
`<synaptome/element/compat/...>` include surface, and their declared parameter
surfaces are checked together.

## Files

| File | Role |
| --- | --- |
| `SignalBloomLayer.h` / `SignalBloomLayer.cpp` | Minimal readable `Layer` subclass with no external assets or heavy setup side effects. |
| `register_signal_bloom.cpp` | Readable pre-generation registration example; the shipping package leaf is creator-only. |
| `signal_bloom.layer.json` | Browser-visible layer asset metadata. |
| `signal_bloom.scene.json` | Scene fixture showing globals, Console slots, media controls, MIDI/OSC routes, and scene persistence shape. |

## Generated Shipping Registration

Element Package v1 owns the shipping type, actions, groups, parameters, and
binding mode. The package leaf supplies only:

```cpp
std::unique_ptr<Layer>
synaptomeCreateElementPackage_examples_signal_bloom();
```

`tools/generate_element_package_registrations.py` validates the explicit
registration set and emits the complete Runtime contract, creator binding, and
opt-in build source list. `ofApp.cpp`, the built-in aggregate, the solution,
and project source lists have no Signal Bloom-specific entry. This remains
controlled compile-time source registration, not automatic discovery,
hot-loaded plug-ins, or persisted action mapping.

Run the complete focused boundary check with:

```powershell
python tools\validate_layer_authoring.py signal-bloom-sdk --native --incremental-app
```

For a multi-asset example backed by one configurable runtime type, see
[`circuit_trace_family.md`](circuit_trace_family.md). It uses three standard
catalog manifests because the current package schema is intentionally
single-asset.

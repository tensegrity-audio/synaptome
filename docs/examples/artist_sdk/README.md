# Artist SDK Example Fixture

Status: Public SDK fixture for the first source/catalog/scene authoring slice.
The example now compiles through the transitional public SDK include root and
its shipping static-library target. Generated registration and no-source-edit
installation remain later package seams.

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

The runtime mirror remains under `synaptome/src/visuals`, but it is compiled by
`Element_SignalBloom.vcxproj` and linked into the host rather than compiled
directly by `Synaptome.vcxproj`. The public fixture and shipping mirror remain
byte-identical. Both use the transitional
`<synaptome/element/compat/...>` include surface.

## Files

| File | Role |
| --- | --- |
| `SignalBloomLayer.h` / `SignalBloomLayer.cpp` | Minimal readable `Layer` subclass with no external assets or heavy setup side effects. |
| `register_signal_bloom.cpp` | Current source-registration requirement for the factory type. |
| `signal_bloom.layer.json` | Browser-visible layer asset metadata. |
| `signal_bloom.scene.json` | Scene fixture showing globals, Console slots, media controls, MIDI/OSC routes, and scene persistence shape. |

## Current Registration Requirement

Until generated registration exists, a type still needs one controlled static
registration record:

```cpp
factory.registerType("example.signalBloom", []() {
    return std::make_unique<SignalBloomLayer>();
});
```

The host and isolated lifecycle bench now call the same
`registerBuiltinElements()` entrypoint; `ofApp.cpp` no longer knows the Signal
Bloom concrete class. Adding a new type still requires updating the controlled
registration unit until SEAC-8 generates it. This is source/static
registration, not hot-loaded plug-ins.

Run the complete focused boundary check with:

```powershell
python tools\validate_layer_authoring.py signal-bloom-sdk --native --incremental-app
```

For a multi-asset example backed by one configurable runtime type, see
[`circuit_trace_family.md`](circuit_trace_family.md). It uses three standard
catalog manifests because the current package schema is intentionally
single-asset.

# Artist SDK Example Fixture

Status: Public SDK fixture for the first source/catalog/scene authoring slice. Source registration is the first public path; no-source-edit plugin loading is a future package seam.

Validator:

```powershell
python tools\validate_artist_sdk_example.py --check
```

This fixture shows the smallest honest public path for an openFrameworks artist:

```text
SignalBloomLayer source
  -> LayerFactory type registration
  -> Browser catalog entry
  -> Console scene slot
  -> parameters targeted by MIDI/OSC/sensor mappings
  -> media layer paired in the same saved scene
```

The fixture is not installed into the runtime project yet. It is a public contract example that proves the shape before Synaptome chooses the final extension/package mechanism.

## Files

| File | Role |
| --- | --- |
| `SignalBloomLayer.h` / `SignalBloomLayer.cpp` | Minimal readable `Layer` subclass with no external assets or heavy setup side effects. |
| `register_signal_bloom.cpp` | Current source-registration requirement for the factory type. |
| `signal_bloom.layer.json` | Browser-visible layer asset metadata. |
| `signal_bloom.scene.json` | Scene fixture showing globals, Console slots, media controls, MIDI/OSC routes, and scene persistence shape. |

## Current Registration Requirement

Until the public extension seam exists, a layer type still has to be registered in app source:

```cpp
factory.registerType("example.signalBloom", []() {
    return std::make_unique<SignalBloomLayer>();
});
```

The final public package mechanism must preserve this contract without requiring artists to edit unrelated app internals. Until then, public docs should present this as source registration, not hot-loaded plugins.

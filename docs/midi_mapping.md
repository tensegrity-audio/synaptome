# MIDI Mapping Guide

MIDI input is one of Synaptome's public control surfaces. Mappings connect physical controller messages to stable parameter IDs used by scenes, layers, HUD widgets, and Console slots.

## Concepts

- **Parameter**: a stable addressable value such as `console.layer.opacity` or a layer-specific control.
- **Mapping**: a MIDI binding that writes a normalized value into a parameter.
- **Device map**: a controller layout file that gives physical controls logical roles.
- **Learn flow**: the Browser/Control UI flow that captures the next MIDI message and binds it to the selected parameter.

## Files

| File | Purpose |
| --- | --- |
| `synaptome/bin/data/config/midi-map.json` | Default MIDI mapping example. |
| `synaptome/bin/data/device_maps/MIDI Mix 0.json` | Public controller-map example. |
| `docs/examples/midi_bank_example.json` | Small documented mapping fixture. |
| `tools/testdata/device_maps/synthetic_controller.json` | Regression fixture for logical slots. |

## Validation

```powershell
python tools\validate_configs.py --public-app
python tools\device_map_regression.py --check
python tools\validate_parameter_targets.py --strict --contract-fixtures
```

The public app contract gate checks MIDI mappings against the parameter manifest and layer catalog so stale targets fail before publication.

## Mapping Shape

A mapping records the physical source and target parameter. Exact schema details live in `docs/schemas/midi_bank.schema.json`.

```json
{
  "bank": "home",
  "mappings": [
    {
      "device": "MIDI Mix 0",
      "channel": 1,
      "type": "cc",
      "number": 16,
      "target": "console.layer.opacity"
    }
  ]
}
```

## Public Rules

- Target parameter IDs must exist in the generated parameter manifest or in a validated fixture-owned target set.
- Device maps should expose logical slots instead of requiring users to memorize raw CC numbers.
- Local learned mappings are app-written state until intentionally promoted as examples.
- Firmware/radio controller mappings belong to future helper packages unless represented as app-facing MIDI or OSC examples.

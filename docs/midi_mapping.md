# MIDI Mapping Guide

MIDI input is one of Synaptome's public control surfaces. Applied routes
connect controller messages or OSC sources to stable parameter IDs used by
scenes, layers, HUD widgets, and Console slots.

## Concepts

- **Parameter**: a stable addressable value such as `console.layer.opacity` or a layer-specific control.
- **Mapping**: an applied MIDI or OSC route that writes into a parameter.
- **Device map**: a controller layout file that gives physical controls logical roles.
- **Mapping-bank route snapshot**: the actual versioned `MidiRouter` state.
  A Scene owns it when embedded at `mappings.router`; otherwise the operator
  mapping store owns it.
- **Learn flow**: the Browser/Control UI flow that captures the next MIDI message and binds it to the selected parameter.

## Files

| File | Purpose |
| --- | --- |
| `synaptome/bin/data/config/midi-map.json` | Operator-local runtime route store; legacy unversioned input remains accepted. |
| `synaptome/bin/data/device_maps/MIDI Mix 0.json` | Public controller-map example. |
| `docs/schemas/machine_profile.schema.json` | Canonical machine-local physical MIDI input owner. |
| `docs/examples/machine_profile_example.json` | Sanitized exact-port binding example. |
| `tools/testdata/machine_profile/midi_binding_cases.json` | Omission, empty, exact resolution, reconnect, ambiguity, and route-filtering fixture. |
| `tools/testdata/mapping_bank/canonical_v1.json` | Canonical mapping-bank v1 route fixture. |
| `tools/testdata/mapping_bank/legacy_unversioned.json` | Legacy runtime-reader fixture. |
| `docs/examples/midi_bank_example.json` | Browser/interchange example; it is not the runtime route format. |
| `tools/testdata/device_maps/synthetic_controller.json` | Regression fixture for logical slots. |

## Validation

```powershell
python tools\validate_configs.py --public-app
python tools\validate_mapping_bank_contract.py --check
python tools\validate_machine_profile_contract.py --check
python tools\device_map_regression.py --check
python tools\validate_parameter_targets.py --strict --contract-fixtures
```

The public app contract gate checks MIDI mappings against the parameter manifest and layer catalog so stale targets fail before publication.

## Runtime Mapping-Bank v1 Route Shape

The runtime snapshot is the existing flat `MidiRouter` vocabulary plus a root
`schemaVersion`. Exact canonical-writer details live in
`docs/schemas/mapping_bank_route_snapshot.schema.json`.

```json
{
  "schemaVersion": 1,
  "cc": [
    {
      "num": 16,
      "channel": 1,
      "target": "console.layer1.opacity",
      "bank": "home",
      "out": [0.0, 1.0]
    }
  ],
  "buttons": [],
  "oscSources": [],
  "osc": []
}
```

Missing `schemaVersion` is legacy unversioned input and is upgraded only in a
copied in-memory document. Writers emit v1. Non-integer, non-positive, and
future versions reject before route mutation. A future standalone primary is
not allowed to activate an older backup.

Outer ownership remains independent: an omitted Scene `mappings.router`
preserves live/operator routes, while a present snapshot—including an empty
v1 snapshot—is authoritative. `mappings.activeBank` and bank definitions are
not duplicated inside this router snapshot.

The separate `docs/schemas/midi_bank.schema.json` describes the older
`{version, bank, mappings}` Browser/interchange example. `MidiRouter` does not
consume that artifact, and the compatibility reader rejects it rather than
silently clearing routes.

## Physical MIDI Input Ownership

The local machine profile owns the selected physical MIDI input independently
from route persistence:

```json
{
  "midi": {
    "inputs": [
      {
        "deviceProfileId": "MIDI Mix 0",
        "portName": "MIDI Mix 0"
      }
    ]
  }
}
```

The optional v1 `midi.inputs` array accepts at most one row. Its precedence and
resolution rules are:

- Omitted `midi` preserves the current binding and is the only valid,
  unblocked profile state that permits legacy standalone `device` or
  `deviceIndex` selection.
- Present `midi` with `inputs: []` explicitly disables MIDI input.
- One row resolves only when exactly one enumerated input port equals
  `portName`, including case. Zero matches remain unavailable; multiple exact
  matches remain ambiguous.
- Device-map `portHints`/`ports`, substring matching, numeric indices, and
  port-zero fallback are never canonical startup resolution.
- Unresolved binding does not delete applied routes or logical control-slot
  assignments. Device-qualified routes apply only when their device profile
  matches the active canonical input; unqualified compatibility routes remain
  available.
- In Device Mapper, `A` explicitly publishes the selected observed port name
  through recoverable machine-profile persistence. A failed write restores the
  prior binding.

Device-map hints may associate an observed port with a roster row so an
operator can select it. That advisory discovery step does not silently choose
startup hardware; explicit binding materializes the exact observed
`portName`.

## Public Rules

- Target parameter IDs must exist in the generated parameter manifest or in a validated fixture-owned target set.
- Device maps should expose logical slots instead of requiring users to memorize raw CC numbers.
- Top-level `device`/`deviceIndex` in the standalone file are machine-local
  compatibility input only while a valid, unblocked machine profile omits
  `midi`. Canonical route saves omit them once that owner is active, and Scene
  route export never owns endpoint selection.
- Current learned routes may retain resolved channel/number and logical
  device/column/slot identity. They are owner-scoped state, not a promise that
  every route snapshot is portable across machines.
- Local learned mappings are app-written state until intentionally promoted as examples.
- Firmware/radio controller mappings belong to future helper packages unless represented as app-facing MIDI or OSC examples.

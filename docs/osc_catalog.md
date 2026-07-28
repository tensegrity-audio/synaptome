# OSC Catalog

OSC is a public Synaptome app-facing control surface. The first public repo documents and validates OSC messages that the openFrameworks runtime can receive or emit without requiring firmware, helper source, generated radio headers, or deployment netmaps.

## App Parameter Routes

The default app OSC map lives at:

```text
synaptome/bin/data/config/osc-map.json
```

The Synaptome-side OSC input selector persists its receive mode here:

```text
synaptome/bin/data/config/osc-input.json
```

`mode` is either `directSerial` for the existing serial collector or `routerUdp` for the Signal Control router path. The default router endpoint is `udp://127.0.0.1:9000`. Both inputs now produce the same transport-neutral typed observation envelope.

Every well-formed OSC message is retained in bounded ingress diagnostics with
its raw and canonical addresses, typetags, payload summary, transport, source
endpoint, and timestamp. This is intentionally broader than parameter
mapping. The existing learn/router path accepts exactly one finite OSC
`int32`, `int64`, `float32`, or `float64` argument. Strings, bools, blobs,
vectors, nil, impulse, MIDI, timetags, and other valid shapes remain visible
without being silently coerced into a float. A typed mapping consumer for any
of those shapes must define its argument and conversion semantics explicitly.

`routerUdp` also accepts B3 external host waveform packets at `/sensor/host/<source>/waveform`. Those multi-float packets publish into `AudioAnalysisBus` and are intentionally not expanded into scalar history rows.

Example route fixture:

```text
docs/examples/osc_map_example.json
docs/examples/osc_input_example.json
```

Validate with:

```powershell
python tools\validate_configs.py --public-app
python tools\validate_osc_route_patterns.py
python tools\validate_osc_ingress_contract.py
python tools\validate_signal_control_receive_contract.py --check
python tools\validate_parameter_targets.py --strict --contract-fixtures
```

## Sensor-Like App Inputs

Synaptome treats several incoming OSC paths as sensor or modulation sources. Public examples should target stable parameter IDs or documented sensor IDs.

Typical app-facing families:

```text
/parameter/<parameter-id>
/sensor/host/localmic/<metric>
/sensor/bioamp/<metric>
/sensor/matrix/<device-id>/mic-level
/sensor/matrix/<device-id>/mic-peak
/sensor/deck/<device-id>/deck-scene
/control/<action>
```

The exact accepted routes are owned by `synaptome/src/io/OscParameterRouter.*` and the committed OSC map fixtures.
Built-in route patterns use `OscParameterRouter`'s current private glob
syntax. `*` matches zero or more characters and `?` matches one character;
both currently may cross `/` boundaries. Regex-looking `.*` is a literal dot
followed by the glob wildcard and does not match normal Mesh device IDs such
as `0x0101`. This grammar is not the complete OSC address-pattern grammar and
must be versioned before its semantics change.

Host audio publishes scalar metrics for modulation:

```text
/sensor/host/localmic/mic-level
/sensor/host/localmic/mic-peak
/sensor/host/localmic/mic-bass
/sensor/host/localmic/mic-mids
/sensor/host/localmic/mic-highs
```

The live waveform is not represented as a scalar OSC route. It is captured by the host audio bridge and rendered by the `Sensors > Audio Waveform` layer so OBS can capture the Synaptome output window directly.

Signal Control may also send host audio telemetry into Synaptome's `routerUdp` input:

```text
/sensor/host/<source>/mic-level
/sensor/host/<source>/mic-peak
/sensor/host/<source>/mic-bass
/sensor/host/<source>/mic-mids
/sensor/host/<source>/mic-highs
```

The default stream host source is `zoom`, so the first producer smoke sends routes such as:

```text
/sensor/host/zoom/mic-level
```

B3 adds visual-rate waveform packets:

```text
/sensor/host/<source>/waveform    64, 128, or 256 float args, normalized -1.0..1.0
```

Waveform packets bypass the scalar OSC path. Synaptome combines the latest scalar telemetry for the source with the waveform packet and publishes one `AudioAnalysisBus::Snapshot` for `Sensors > Audio Waveform`.

Tensegrity Signal Control currently produces these packets with optional waveform output enabled. Its default is source `zoom`, `128` samples, and `30 Hz` to `udp://127.0.0.1:9000`.

B3 receive expectations:

- Use `128` waveform samples at `30 Hz` by default.
- Clamp waveform sample values to `[-1.0, 1.0]`.
- Keep single-float telemetry visible to OSC learn, mapping, HUD telemetry, and history.
- Keep multi-float waveform packets out of scalar history.
- Show external source label, telemetry freshness, waveform freshness, and sample count in `OSC > Input`.

The receive/input contract is fixture-backed by:

```text
tools/testdata/signal_control/expected_receive_contract.json
```

## Synaptome Mesh Gateway Via Signal Control

The synaptome_mesh gateway contract is producer-owned by the `synaptome_mesh` repo as `synaptome-mesh-osc v0.1.0`. When OBS and Synaptome both need the gateway stream, the Signal Control Tkinter app should ingest the gateway OSC first, show user-facing route/payload/status details, and fan out compatible OSC to Synaptome's Router UDP input plus mapped state to OBS widgets.

Mesh v0.1 emits every event as an ordered pair: the legacy address first, then
the same payload at `/synaptome_mesh` plus that address. Synaptome owns the
consumer adapter:

- Preserve the producer's raw address, typed payload, transport, and endpoint.
- Derive the unprefixed route as the canonical app address.
- Suppress only an immediate identical legacy/namespaced pair from the same
  origin, within 25 ms. Repeated later samples still dispatch.
- Translate `/sensor/hr/<device-id>/heart-bpm` to Synaptome's established
  `/sensor/hr/<device-id>/bpm` route.
- Pass every other Mesh numeric route through unchanged.
- Retain Mesh string/status/control payloads as diagnostics; do not interpret
  `/control/rx` as a Synaptome action or `/param/rx` as a parameter ID.

This adapter runs in Synaptome for both direct SLIP serial and Router UDP.
Signal Control may mirror it per destination, but its raw ingress and other
fanout outputs should remain unchanged. The Mesh gateway does not need to
reformat its versioned producer contract for Synaptome.

Direct SLIP serial now parses OSC 1.x numeric, string/symbol, bool, blob,
char, MIDI, timetag, RGBA, nil, and impulse argument shapes into the common
envelope. Frames over 64 KiB and malformed or truncated payloads are rejected
before observation or routing.

The consumer-owned compatibility fixture is:

```text
tools/testdata/osc_ingress/synaptome_mesh_v0_1_0_consumer_capture.json
```

The full cross-repo route contract lives in `docs/contracts/signal_control_integration.md`.

## Public Boundary

Included in first public Synaptome:

- app OSC map schema and examples
- parameter-target validation
- BrowserFlow OSC ingest regression
- generic typed OSC observation and Mesh v0.1 receiver compatibility
- host-local audio/sensor style routes when represented as app inputs

Not included in first public Synaptome:

- firmware TLV decode implementation
- helper ESP-NOW bridge source
- generated radio config headers
- private deployment netmaps
- embedded UI catalog exchange fixtures

Those belong to future helper or radio-contract packages. If a future package emits OSC into Synaptome, it should publish sample app-facing OSC captures that do not require private firmware artifacts.

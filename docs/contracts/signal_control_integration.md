# Signal Control Integration Contract

Status: Draft coordination contract for the Synaptome and Tensegrity repos. Last alignment check: 2026-05-17 after the live Signal Control -> Synaptome route pass and Tkinter desktop decision.

This document is mirrored in both repositories:

- Synaptome: `docs/contracts/signal_control_integration.md`
- Tensegrity: `docs/contracts/signal_control_integration.md`

Keep the two copies aligned until this contract moves into a shared package or published protocol document.

## Current Alignment Snapshot

As of 2026-05-17, both repo copies of this contract should remain identical. The live operator route check was reported passing on 2026-05-15: Signal Control routed host audio telemetry/waveform into Synaptome with `OSC > Input > Router UDP`.

Synaptome has completed its current receive-side slice:

- Browser `OSC > Input` rows exist for mode, serial status, router UDP port, last message, and reconnect/rebind.
- `config/osc-input.json` persists the input mode, defaults to `directSerial`, and stores the router target as `127.0.0.1:9000`.
- Direct serial remains the working fallback path for existing OSC learn, parameter routing, HUD telemetry, and history.
- `routerUdp` binds an `ofxOscReceiver`, shows listening status in the Browser, and routes single numeric OSC messages through the existing scalar OSC path.
- `routerUdp` now accepts `/sensor/host/<source>/waveform` multi-float packets and publishes external host audio snapshots into Synaptome's existing render-facing `AudioAnalysisBus`.
- Browser `OSC > Input` rows report external source label, telemetry freshness, waveform freshness, and waveform sample count.
- `docs/examples/osc_input_example.json`, `docs/schemas/osc_input.schema.json`, and `tools/testdata/signal_control/expected_receive_contract.json` now fixture the receive/input contract.
- `tools/validate_signal_control_receive_contract.py --check` validates the Signal Control receive fixture and the live `config/osc-input.json` shape.

Tensegrity has advanced its side of the split in `media/obs/signal_frame/`:

- The current OBS signal-frame control panel has a web panel plus a Tkinter desktop shell over the same local API. Tkinter is now the chosen final desktop stack; packaging and polish remain.
- It can select an audio input and drive the existing OBS waveform socket at `ws://127.0.0.1:8787/ws/audio`.
- It can optionally send single-float Synaptome OSC telemetry to `udp://127.0.0.1:9000` with source labels such as `zoom`.
- It can optionally send `/sensor/host/<source>/waveform` OSC packets with `64`, `128`, or `256` float samples.
- Those waveform packets currently reuse the OBS visual downsample path, including Signal Control gain.
- It now has a first Tkinter/API gateway router slice for synaptome_mesh serial/SLIP ingest, UDP ingest for bridge/testing workflows, raw OSC preservation, Synaptome UDP fanout, OBS/log fanout, and user-facing route/payload/status rows.
- It has destination health in the desktop shell and web panel packet status for scalar and waveform sends; the desktop shell now also shows freshness/error detail for the audio OSC destination and gateway router destinations.
- It still needs live synaptome_mesh gateway validation, replay fixtures around captured SLIP streams, and explicit OBS widget mappings beyond the current router log/counter model.

The immediate shared handoff is:

```text
Signal Control audio input
  -> OBS waveform WebSocket
  -> /sensor/host/<source>/mic-* single-float OSC
  -> /sensor/host/<source>/waveform multi-float OSC
  -> Synaptome routerUdp input on udp://127.0.0.1:9000
```

The next shared slice is Tensegrity-side router ingest and user-facing destination health, because the basic Signal Control -> Synaptome audio route has already passed live.

## Goal

Build one Tkinter desktop Signal Control panel in the Tensegrity repo that owns live signal routing for OBS and Synaptome:

- Zoom or other host audio can drive OBS widgets and Synaptome audio-reactive visuals.
- Hardware or helper OSC can route to Synaptome and OBS at the same time.
- Synaptome can still run without Signal Control by using direct serial OSC and local mic capture.
- OBS can still run without Synaptome by using direct audio capture.

The operator should see one main desktop control surface for stream routing, plus a small Synaptome-side input selector for the app's own receive mode.

## Repository Ownership

| Area | Synaptome repo owns | Tensegrity repo owns |
| --- | --- | --- |
| Desktop panel | No desktop panel implementation. | Native Tkinter Signal Control desktop app, currently prototyped in `media/obs/signal_frame/` and later packageable. |
| OBS integration | Stable output/telemetry contracts only. | OBS overlay widget WebSocket, layout controls, meters, stream operator controls. |
| OSC input | Synaptome receiver modes: direct serial and router UDP. | Router service that ingests serial or UDP OSC and forwards to configured destinations. |
| Audio capture | Local mic capture for Synaptome-only operation. | Zoom/host audio capture for stream operation. |
| External audio into Synaptome | Receive telemetry and waveform from router. | Produce telemetry and waveform from captured audio. |
| Mapping/runtime | Apply OSC/audio data to parameters, modifiers, HUD feeds, and audio-reactive layers. | Route, meter, transform, and forward signals without knowing Synaptome internals. |
| Public contracts | App-facing receive contracts, schemas, fixtures, and Browser docs. | Producer/router contracts, desktop app config, OBS setup docs, packaging. |

## Operator Surfaces

Signal Control desktop panel:

- Select Zoom or other host audio input.
- Show peak/RMS/waveform meters.
- Route audio samples to the OBS waveform WebSocket.
- Route audio telemetry and waveform packets to Synaptome.
- Ingest hardware/helper OSC from serial or UDP.
- Forward OSC to Synaptome and OBS widgets.
- Show destination health and recent messages.

Synaptome Mesh gateway ingest:

- The synaptome_mesh gateway emits OSC over serial/SLIP today under the `synaptome-mesh-osc v0.1.0` contract.
- In router mode, Signal Control should ingest the gateway stream first so the same messages can be inspected, counted, and routed to both OBS and Synaptome.
- Signal Control should preserve every OSC message it receives from the gateway, including legacy routes and `/synaptome_mesh/...` aliases, and it should not silently drop string/status/control messages just because Synaptome's current app-facing mapping path mostly consumes numeric routes.
- Forwarding to Synaptome should send OSC to Synaptome's Router UDP input on `udp://127.0.0.1:9000`. Synaptome currently applies numeric scalar app routes and host waveform packets; string or richer gateway messages may require a Synaptome follow-up if they need to become Browser history, HUD feeds, or mappings.
- OBS-facing routing is Tensegrity-owned. It may map gateway messages into widget state, labels, counters, meters, or logs without requiring Synaptome internals.

Synaptome Browser:

- `OSC > Input` submenu with `Direct Serial` and `Router UDP` modes.
- Show active serial port or UDP listen port.
- Show recent OSC address/value and receiver status.
- Show active external audio source and waveform freshness.

## Default Local Ports

All ports are configurable. Defaults should avoid both apps binding the same UDP port.

| Purpose | Default | Owner |
| --- | --- | --- |
| OBS audio widget WebSocket | `ws://127.0.0.1:8787/ws/audio` | Signal Control |
| Synaptome routed OSC input | `udp://127.0.0.1:9000` | Synaptome receiver |
| Signal Control OSC ingest from Synaptome telemetry | `udp://127.0.0.1:9001` | Signal Control |
| Hardware/helper OSC ingest | Configurable serial or UDP | Signal Control in router mode; Synaptome in direct mode |
| Synaptome Mesh gateway OSC ingest | Gateway serial can still go direct only for Synaptome-only fallback. | Signal Control owns gateway ingest/fanout when OBS and Synaptome both need the same OSC stream. |

## Signal Flows

### Stream Mode

```text
Zoom audio device
  -> Signal Control desktop app
      -> OBS waveform WebSocket
      -> Synaptome audio telemetry OSC
      -> Synaptome audio waveform OSC

Hardware/helper OSC
  -> Signal Control router
      -> Synaptome UDP input
      -> OBS widgets or internal panel state
```

### Synaptome Direct Mode

```text
Hardware/helper serial OSC
  -> Synaptome direct serial input
      -> Synaptome MIDI/OSC learn
      -> Synaptome parameter routing
      -> Synaptome HUD/feed history
```

### Synaptome-Owned Mic Mode

```text
Synaptome local mic capture
  -> Synaptome audio-reactive runtime
  -> Synaptome OSC telemetry publish
      -> Signal Control
          -> OBS waveform widget, synthetic first and true waveform later
```

## Synaptome Receive Contract

Signal Control sends host audio as app-facing OSC using this route shape:

```text
/sensor/host/<source>/<metric>
```

Source segment rules:

- Default Zoom/host source is `zoom`.
- Source ids should use lowercase ASCII letters, digits, `_`, or `-`.
- Synaptome should treat unknown source ids as labels, not as a validation failure.

Telemetry routes:

| Route | Argument | Synaptome field |
| --- | --- | --- |
| `/sensor/host/<source>/mic-level` | float `0.0..1.0` | `level` / RMS envelope |
| `/sensor/host/<source>/mic-peak` | float `0.0..1.0` | `peak` |
| `/sensor/host/<source>/mic-bass` | float `0.0..1.0` | `bass` |
| `/sensor/host/<source>/mic-mids` | float `0.0..1.0` | `mids` |
| `/sensor/host/<source>/mic-highs` | float `0.0..1.0` | `highs` |

Waveform route:

```text
/sensor/host/<source>/waveform    64, 128, or 256 float args, normalized -1.0..1.0
```

Waveform packet rules:

- Packets are visual-rate telemetry, not audio-rate streaming.
- Initial target is `30 Hz` and `128` samples per packet.
- Each argument is a mono sample normalized to `[-1.0, 1.0]`.
- The current Tensegrity prototype sends the same gain-adjusted, clipped visual samples used by the OBS waveform socket.
- Synaptome should clamp out-of-range sample values.
- The first implementation does not require timestamp arguments; receive time is enough for freshness.

Synaptome should treat telemetry and waveform as one external audio source and map it into the existing render-facing bus:

```cpp
struct ExternalAudioFrame {
    std::string sourceId;
    float level;
    float peak;
    float bass;
    float mids;
    float highs;
    std::vector<float> samples;
    double receiveTime;
};
```

The external frame should publish to the existing `AudioAnalysisBus::Snapshot` used by `AudioWaveformLayer`:

```cpp
AudioAnalysisBus::Snapshot {
    valid;
    frame;
    sampleRate;
    channels;
    level;
    peak;
    bass;
    mids;
    highs;
    sourceLabel;
    waveform;
}
```

Synaptome processing expectations:

- Single-float telemetry continues through the existing scalar OSC path so learn, mappings, HUD telemetry, and history keep working.
- The same telemetry also updates the external audio source state for `<source>`.
- Multi-float waveform packets bypass `ingestOscMessage(address, float)` and should not be expanded into hundreds of scalar history entries.
- When a waveform packet arrives, Synaptome combines it with the latest telemetry values for that source and publishes one `AudioAnalysisBus::Snapshot`.
- Until a real sample rate is negotiated, external waveform snapshots may use `sampleRate = 0` and `channels = 1`.
- Initial UI freshness thresholds: telemetry fresh at `<= 2000 ms`, waveform fresh at `<= 1000 ms`, stale after `5000 ms`.

Browser status expectations for `OSC > Input` after B3:

- Current input mode and router bind state.
- Last scalar OSC message.
- External audio source label.
- Telemetry freshness.
- Waveform freshness.
- Waveform sample count.

## Synaptome Work Plan

1. Done: Add Browser/config state for `directSerial` and `routerUdp` modes.
2. Done: Add UDP OSC receive support through `ofxOscReceiver`.
3. Done: Route both direct serial and UDP single-float messages into existing OSC learn, parameter routing, HUD telemetry, and history.
4. Done: Add `OSC > Input` Browser submenu rows for mode, serial status, UDP port, last message, and reconnect/rebind.
5. Done: Add external host audio source state backed by the existing `AudioAnalysisBus`.
6. Done: Add multi-float waveform OSC handling for `/sensor/host/<source>/waveform`.
7. Done: Publish external snapshots so `Sensors > Audio Waveform` can render router audio without opening the same audio device.
8. Done: Add Browser rows for external source freshness and waveform sample count.
9. Done: Add fixtures/docs for the receive routes and validation coverage for config shape.
10. Future if needed: Accept and surface forwarded non-numeric gateway OSC payloads beyond the current numeric scalar/waveform app path.

## Tensegrity Work Plan

1. In progress: Promote the current OBS web control panel into a native Tkinter desktop Signal Control app; Tkinter is the chosen stack, final packaging is pending.
2. In progress: Keep or port the current audio capture behavior for direct OBS waveform mode.
3. In progress: Add router-owned audio capture for Zoom/host audio.
4. Done for the current producer slice: Add OSC output to Synaptome for single-float telemetry and waveform packets.
5. In progress: Add OSC/serial input router for hardware/helper data. First synaptome_mesh gateway ingest/fanout slice has landed with serial/SLIP ingest, UDP ingest mode, raw OSC preservation, Synaptome UDP fanout, and OBS/log fanout.
6. In progress: Add destination rows for Synaptome, OBS widgets, and optional logging/sniffing; scalar/waveform packet counters and first freshness/error rows now exist, with explicit OBS widget mappings still pending.
7. In progress: Keep OBS waveform WebSocket payload compatible with the existing overlay.
8. In progress: Add a config file for router inputs, destinations, ports, source labels, and waveform options.

## Acceptance Checks

- Synaptome can receive hardware OSC directly over serial with Signal Control closed.
- Synaptome can receive hardware OSC over UDP from Signal Control router mode.
- OBS waveform can run from direct Zoom capture without Synaptome.
- Signal Control can capture Zoom audio once and feed OBS waveform samples plus Synaptome telemetry and waveform packets.
- Synaptome can receive both telemetry and waveform from Signal Control without opening the same audio device.
- Synaptome receives `/sensor/host/zoom/waveform` with 128 floats and `Sensors > Audio Waveform` updates from `AudioAnalysisBus`.
- Multi-float waveform packets do not enter the scalar OSC history as one row per sample.
- Signal Control can ingest every OSC message emitted by the synaptome_mesh gateway stream, show recent route/payload/status information, and fan out compatible OSC to Synaptome plus mapped state to OBS.
- Signal Control can receive Synaptome local mic telemetry on port `9001` and drive OBS without opening the mic.
- Signal Control shows scalar packet count, waveform packet count, and destination target for the Synaptome destination.
- Signal Control shows first latest send error/freshness detail for the Synaptome audio destination and gateway router destinations.
- No two processes need to bind the same UDP or WebSocket port.

## Live Operator Check

Status: Passed manually on 2026-05-15 for the Signal Control host-audio route into Synaptome Router UDP. Keep the checklist below as the repeatable show-night smoke.

Run this after both repos are on current builds:

```text
Signal Control:
  OSC Out: enabled
  Target: udp://127.0.0.1:9000
  Source: zoom
  Waveform: enabled
  Waveform samples: 128

Synaptome:
  OSC > Input: Router UDP
  UDP Port: 9000
```

Pass criteria:

- Signal Control scalar and waveform packet counters increase.
- Synaptome `Last OSC` updates from `/sensor/host/zoom/mic-*`.
- Synaptome external source/freshness rows and waveform sample count update.
- `Sensors > Audio Waveform` renders the routed host waveform.

## Open Decisions

- Final packaging shape for the Tkinter Signal Control desktop app.
- Whether Synaptome should also accept a WebSocket waveform stream later after the first OSC waveform implementation.
- Whether waveform source selection belongs only in Synaptome Browser or also in scene/config persistence.
- Whether Signal Control should edit Synaptome config files directly or only display expected Synaptome settings.

# Signal Control Integration Contract

Status: Draft coordination contract for the Synaptome and Tensegrity repos.

This document is mirrored in both repositories:

- Synaptome: `docs/contracts/signal_control_integration.md`
- Tensegrity: `docs/contracts/signal_control_integration.md`

Keep the two copies aligned until this contract moves into a shared package or published protocol document.

## Goal

Build one desktop Signal Control panel in the Tensegrity repo that owns live signal routing for OBS and Synaptome:

- Zoom or other host audio can drive OBS widgets and Synaptome audio-reactive visuals.
- Hardware or helper OSC can route to Synaptome and OBS at the same time.
- Synaptome can still run without Signal Control by using direct serial OSC and local mic capture.
- OBS can still run without Synaptome by using direct audio capture.

The operator should see one main desktop control surface for stream routing, plus a small Synaptome-side input selector for the app's own receive mode.

## Repository Ownership

| Area | Synaptome repo owns | Tensegrity repo owns |
| --- | --- | --- |
| Desktop panel | No desktop panel implementation. | Native Signal Control desktop app, likely under `apps/signal_control/`. |
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

Synaptome Browser:

- `OSC > Input` submenu with `Direct Serial` and `Router UDP` modes.
- Show active serial port or UDP listen port.
- Show recent OSC address/value and receiver status.
- Later: show active external audio source and waveform freshness.

## Default Local Ports

All ports are configurable. Defaults should avoid both apps binding the same UDP port.

| Purpose | Default | Owner |
| --- | --- | --- |
| OBS audio widget WebSocket | `ws://127.0.0.1:8787/ws/audio` | Signal Control |
| Synaptome routed OSC input | `udp://127.0.0.1:9000` | Synaptome receiver |
| Signal Control OSC ingest from Synaptome telemetry | `udp://127.0.0.1:9001` | Signal Control |
| Hardware/helper OSC ingest | Configurable serial or UDP | Signal Control in router mode; Synaptome in direct mode |

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

Signal Control may send host audio as app-facing OSC using a source id path segment.

Recommended Zoom source prefix:

```text
/sensor/host/zoom
```

Telemetry routes:

```text
/sensor/host/zoom/mic-level   float 0.0..1.0 RMS/envelope
/sensor/host/zoom/mic-peak    float 0.0..1.0 peak
/sensor/host/zoom/mic-bass    float 0.0..1.0 low-band energy
/sensor/host/zoom/mic-mids    float 0.0..1.0 mid-band energy
/sensor/host/zoom/mic-highs   float 0.0..1.0 high-band energy
```

Waveform route:

```text
/sensor/host/zoom/waveform    64, 128, or 256 float args, normalized -1.0..1.0
```

Waveform packets are visual-rate telemetry, not audio-rate streaming. Initial target is 30 Hz and 128 samples per packet.

Synaptome should treat telemetry and waveform as one external audio source:

```cpp
struct ExternalAudioFrame {
    std::string sourceId;
    float rms;
    float peak;
    float bass;
    float mids;
    float highs;
    std::vector<float> samples;
    double captureTime;
};
```

Single-float telemetry can continue through the existing OSC/message path. Multi-float waveform packets need a richer receive path than `ingestOscMessage(address, float)`.

## Synaptome Work Plan

1. Add an OSC input manager with `directSerial` and `routerUdp` modes.
2. Add UDP OSC receive support through `ofxOscReceiver`.
3. Route both direct serial and UDP single-float messages into existing OSC learn, parameter routing, HUD telemetry, and history.
4. Add `OSC > Input` Browser submenu rows for mode, serial status, UDP port, last message, and reconnect/rebind.
5. Add an external audio bus for telemetry and waveform sources.
6. Add multi-float waveform OSC handling for `/sensor/host/<source>/waveform`.
7. Let audio-reactive layers choose local mic or external audio source.
8. Add fixtures/docs for the receive routes and validation coverage for config shape.

## Tensegrity Work Plan

1. Promote the current OBS web control panel into a native desktop Signal Control app.
2. Keep or port the current audio capture behavior for direct OBS waveform mode.
3. Add router-owned audio capture for Zoom/host audio.
4. Add OSC output to Synaptome for telemetry and waveform packets.
5. Add OSC/serial input router for hardware/helper data.
6. Add destination rows for Synaptome, OBS widgets, and optional logging/sniffing.
7. Keep OBS waveform WebSocket payload compatible with the existing overlay.
8. Add a config file for router inputs, destinations, ports, and source labels.

## Acceptance Checks

- Synaptome can receive hardware OSC directly over serial with Signal Control closed.
- Synaptome can receive hardware OSC over UDP from Signal Control router mode.
- OBS waveform can run from direct Zoom capture without Synaptome.
- Signal Control can capture Zoom audio once and feed both OBS waveform samples and Synaptome telemetry.
- Synaptome can receive both telemetry and waveform from Signal Control without opening the same audio device.
- Signal Control can receive Synaptome local mic telemetry on port `9001` and drive OBS without opening the mic.
- No two processes need to bind the same UDP or WebSocket port.

## Open Decisions

- Final desktop stack for Signal Control. Current recommendation: Python plus PySide6/Qt, packaged later with PyInstaller.
- Whether Synaptome waveform input should be OSC-only at first or also accept a WebSocket stream later.
- Whether waveform source selection belongs only in Synaptome Browser or also in scene/config persistence.
- Whether Signal Control should edit Synaptome config files directly or only display expected Synaptome settings.

# Synaptome Transport And Reactivity Contract

Status: Planned dependency; reviewed 2026-07-18. No detector or clock-source
implementation is currently active. This document keeps BPM, beat detection,
clock source selection, onset/downbeat events, confidence, and fallback policy
separate from the layer package roadmap.

Read with: [`synaptome_layer_system_roadmap.md`](synaptome_layer_system_roadmap.md)
and [`synaptome_external_contracts.md`](synaptome_external_contracts.md).

## Purpose

Layers should be able to react to time, BPM, beats, audio, sensors, MIDI, and
OSC without each layer inventing its own clock or audio detector.

The runtime should own the timing and source truth. Layers should receive those
signals through shared update params, public transport parameters, or visible
mapping sources.

## What Belongs Here

- BPM ownership.
- Manual tap tempo.
- MIDI clock or external clock input.
- OSC-provided clock, beat, or onset messages.
- Host-audio-derived beat detection.
- Beat phase, bar phase, downbeat, onset, and confidence.
- Fallback behavior when a source disappears or confidence drops.
- Mapping-visible transport/reactivity sources such as `/beat/onset`.

## What Does Not Belong Here

- Layer package layout.
- Layer parameter naming rules.
- Layer preset file shape.
- Folder-driven layer discovery.
- File-backed generated assets such as STL drops.

Layer packages can reference transport and reactivity sources in mapping
presets, but the runtime owns how those sources are produced.

## Target Beat Surface

The public transport surface should answer:

- What is the current BPM?
- Where are we inside the beat?
- Where are we inside the bar?
- Did an onset happen this frame?
- Did a downbeat happen this frame?
- How confident is the current detector/source?
- What source owns the clock right now?
- What happens when that source becomes unavailable?

Likely public values:

- `transport.bpm`
- `transport.beatPhase`
- `transport.barPhase`
- `transport.beatIndex`
- `transport.onset`
- `transport.downbeat`
- `transport.confidence`
- `transport.source`

## Source Policy

The runtime should support multiple clock/reactivity sources, but only one
should own the authoritative beat state at a time unless the UI explicitly
blends them.

Possible sources:

- manual BPM and tap tempo,
- MIDI clock,
- OSC clock or onset routes,
- host-audio beat detection,
- external app or router-provided timing.

The Browser should show the active source and confidence where practical.

## Mapping Relationship

Beat and onset events should feed the same mapping system as OSC, MIDI, and
sensors. A layer package may suggest a mapping such as:

```text
/beat/onset -> pulse
```

But the layer package does not own beat detection. It only declares that, if a
beat/onset source exists, this layer has a useful default mapping for it.

## Tracked Transport Gap

This gap is folded into this roadmap. `docs/contracts/contract_gaps.md` indexes
it, but this document owns its meaning and next actions.

| ID | Gap | Current State | Next Action |
| --- | --- | --- | --- |
| CG-19 | Beat detection and transport source contract | Runtime exposes BPM/beat context, but Synaptome does not yet have a stable beat detection contract covering source selection, onset/downbeat events, phase, confidence, fallback behavior, and mapping exposure. | Define transport beat fields, clock source policy for manual/MIDI/OSC/host-audio inputs, confidence/fallback semantics, and mapping-visible beat/onset sources. Add fixtures and runtime tests once the detector path is chosen. |

## Roadmap

1. Define the public transport fields and source IDs.
2. Decide source priority and fallback behavior.
3. Add manual BPM/tap and external clock source policy.
4. Add host-audio beat/onset detection only after the source contract is clear.
5. Surface beat/onset values as mapping-visible sources.
6. Add fixtures and runtime tests for confidence, fallback, and scene reload.

Resume condition: promote a focused transport request when beat-reactive
mapping presets need to move from draft metadata into canonical runtime
behavior. Do not make layer packages responsible for detecting beats.

# Synaptome Parameter Vocabulary

Status: First public vocabulary draft, paired with `docs/contracts/parameter_manifest.json`.

Validator: `python tools\gen_parameter_manifest.py --check`

## Purpose

Synaptome parameters are the stable names that scenes, layers, MIDI maps, OSC routes, device maps, the Browser, the Console, and the HUD use to talk about controllable runtime state.

The manifest is generated from current app registrations and layer catalog assets. This vocabulary explains which names should be reused by future layers instead of inventing near-duplicates.

## ID Shape

Use dotted identifiers:

```text
family.subject.detail
```

Examples:

```text
transport.bpm
globals.speed
effects.ascii.coverage
generative.oscilloscope.speed
console.layer{slot}.opacity
```

Rules:
- Use ASCII letters, digits, and underscores in each segment.
- Use lowerCamelCase for multi-word suffixes already used by the app, such as `bpmMultiplier`, `faceOpacity`, and `sampleDensity`.
- Use the same suffix for the same concept across layers.
- Treat a public parameter rename as a migration event. Add an alias or scene migration note before removing the old ID.

## Runtime Families

| Family | Meaning | Examples |
| --- | --- | --- |
| `transport` | Shared clock and timing state. | `transport.bpm` |
| `globals` | Global runtime multipliers. | `globals.speed` |
| `camera` | Projection camera controls. | `camera.dist` |
| `fx` / `effects` | Master and post-effect controls. | `fx.master`, `effects.crt.glow` |
| `ui` | Operator surface visibility and sizing. | `ui.hud`, `ui.menu_text_size` |
| `console` | Console slot and control-window state. | `console.secondary_display.enabled`, `console.layer{slot}.opacity` |
| `hud` | HUD widget visibility and feeds. | `hud.controls`, `hud.sensors` |
| `overlay` | Shared overlay/text controls. | `overlay.text.content`, `overlay.text.size` |
| `sensors` | Local or external sensor values mirrored into parameters. | `sensors.bioamp.rms` |
| `generative`, `geometry`, `media` | Layer asset families from the catalog. | `generative.perlin.scale`, `media.webcam.primary.device` |

## Reusable Layer Suffixes

| Suffix | Type | Recommended Range / Values | Meaning |
| --- | --- | --- | --- |
| `visible` | bool | `true` / `false` | Layer or surface visibility. |
| `opacity` | float | `0.0` to `1.0` | Slot or layer opacity before composition. |
| `alpha` | float | `0.0` to `1.0` | Visual element alpha inside a layer. |
| `speed` | float | Layer-specific; document units. | Primary animation speed. |
| `bpmSync` | bool | `true` / `false` | Whether the layer follows global BPM. |
| `bpmMultiplier` | float | Common values: `0.25`, `0.5`, `1`, `2`, `4`, `8`. | Beat-relative speed multiplier. |
| `gain` | float | `0.0` and up, usually `0.0` to `2.0`. | Media/video/audio amplitude multiplier. |
| `mirror` | bool | `true` / `false` | Horizontal mirror for camera/video sources. |
| `loop` | bool | `true` / `false` | Media loop behavior. |
| `scale` | float | Layer-specific. | Spatial or texture scale. |
| `rotateX`, `rotateY`, `rotateZ`, `rotationDeg` | float | Degrees. | Rotation controls. |
| `colorR`, `colorG`, `colorB` | float | `0.0` to `255.0` or normalized if documented. | Primary color channels. |
| `bgColorR`, `bgColorG`, `bgColorB` | float | `0.0` to `255.0` or normalized if documented. | Background color channels. |
| `lineOpacity`, `faceOpacity` | float | `0.0` to `1.0` | Geometry wire/face composition. |
| `xInput`, `yInput`, `speedInput` | float | Input selector values. | Sensor/modulation input selectors. |
| `reseed` | bool | Momentary/latched bool. | Request a new random seed. |
| `autoReseed` | bool | `true` / `false` | Automatically reseed on timing rules. |
| `autoReseedEveryBeats` | float | Beats. | Auto-reseed period. |
| `route` | float | Integer-like route selector. | Post-effect routing target. |
| `coverage` | float | Integer-like column count or normalized coverage, documented per effect. | Effect coverage window. |
| `coverageMask` | bool | `true` / `false` | Enable effect coverage mask behavior. |

## Console Slot Patterns

Live loaded layers use slot-scoped IDs:

```text
console.layer{slot}.{suffix}
```

where `{slot}` is currently `1` through `8`.

Examples:

```text
console.layer1.speed
console.layer4.opacity
console.layer8.colorR
```

Layer catalog assets also expose asset-scoped IDs in the manifest:

```text
generative.perlin.speed
geometry.grid.faceOpacity
media.webcam.primary.device
```

Use asset-scoped IDs for Browser/catalog inspection and slot-scoped IDs for live scene state.

## Deprecation Policy

- Do not delete a public parameter ID without a migration note.
- Prefer aliases or scene migration tooling when an ID changes.
- Keep suffix semantics stable across layer families.
- If the same suffix needs different units, document the units in the layer authoring guide and manifest metadata.

## Current Limits

- The manifest validates the generated ID surface, templates, and duplicates.
- `tools/validate_parameter_targets.py --strict` validates scene, MIDI, OSC, HUD, audio, and device-map targets against the generated manifest, Console slot templates, and layer catalog IDs.
- Some catalog assets, such as HUD widgets and post-effect catalog entries, use global/effect parameters rather than `Layer::setup` parameters. They are listed separately in the manifest.

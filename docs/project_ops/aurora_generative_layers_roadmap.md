# Aurora Generative Layers Roadmap

Status: Draft show-development roadmap.
Created: 2026-06-18.

This roadmap captures the aurora visual track for Synaptome. The core idea is to make waveform and spectrum data the primary animation source, not just a light modulation input. Aurora layers should feel like music stretched into luminous atmospheric curtains.

## Current Runtime Leverage

Synaptome can support waveform-driven aurora work now through:

- `AudioAnalysisBus::Snapshot` fields: `level`, `peak`, `bass`, `mids`, `highs`, and `waveform`.
- Local mic waveform snapshots from `AudioInputBridge`.
- Signal Control waveform packets at `/sensor/host/<source>/waveform` with 64, 128, or 256 float samples.
- Signal Control scalar routes for `mic-level`, `mic-peak`, `mic-bass`, `mic-mids`, and `mic-highs`.
- Browser, MIDI, OSC, and sensor mappings into registered layer parameters.

The current system does not yet expose a full reusable FFT spectrum to layers. Local mic analysis computes broad bass/mids/highs bands with Goertzel-style probes, while external Signal Control currently sends waveform plus broad band telemetry. This is enough for show-ready aurora curtains, but richer FFT bins would unlock more detailed spectral texture.

## Visual Direction

Aurora should be driven by three related structures:

- **Curtain ribbons:** waveform samples become vertical displacement along long horizontal bands.
- **Spectral shimmer:** highs and future FFT bins add thin glints, edge flicker, and fine striations.
- **Magnetic drift:** noise fields and low-frequency energy bend the curtains into slow flowing arcs.

Audio mapping baseline:

| Input | Visual Response |
| --- | --- |
| Waveform | Curtain displacement, ribbon edge shape, historical trailing bands. |
| Bass | Curtain height, vertical lift, large magnetic swell. |
| Mids | Fold strength, horizontal drift, ribbon thickness. |
| Highs | Shimmer, sparkle density, fine striation, edge flicker. |
| Peak | Bright flare sweep or sudden curtain ignition. |
| Level | Overall opacity, glow, bloom, and density. |

## Priority Ranking

| Rank | Layer | Fit | Primary Reason |
| ---: | --- | --- | --- |
| 1 | Waveform Aurora Curtains | Excellent | Uses existing waveform packets directly and has immediate visual identity. |
| 2 | Multi-Curtain Spectral Veil | Excellent | Extends waveform curtains with bass/mids/highs split behavior. |
| 3 | Aurora Flow Field | Good | Uses current flow/noise strengths for slow magnetic drift and atmospheric layering. |
| 4 | FFT Spectral Curtain | Good after bus upgrade | Needs reusable spectrum bins, then becomes the most musically precise aurora layer. |
| 5 | Aurora Particle Rain | Medium | Good accent layer, but weaker as the main aurora image. |
| 6 | Magnetosphere Field Lines | Medium | Good science-inspired overlay, better paired with curtains than standalone. |

## Layer Concepts

### Waveform Aurora Curtains

First target layer. It should read `AudioAnalysisBus` directly and render one or more luminous ribbons.

Parameters:

- `visible`
- `alpha`
- `curtainCount`
- `sampleDensity`
- `waveformGain`
- `verticalScale`
- `curtainHeight`
- `foldStrength`
- `flowSpeed`
- `noiseScale`
- `trailDecay`
- `lineThickness`
- `glowAmount`
- `shimmerAmount`
- `magneticTilt`
- `bassLift`
- `midsFold`
- `highsSparkle`
- `peakFlash`
- `colorR`, `colorG`, `colorB`
- `color2R`, `color2G`, `color2B`
- `bgColorR`, `bgColorG`, `bgColorB`

Implementation notes:

- Use waveform samples as the base spline.
- Keep a short waveform history to draw layered trailing curtains.
- Smooth waveform input separately from visual trail decay.
- Let each curtain use a phase offset and noise offset so repeated bands do not look copied.
- Make color palettes favor green, cyan, violet, pale blue, and optional sunset-pink variants.

### Multi-Curtain Spectral Veil

Second target layer or an expanded mode of the first layer.

Parameters:

- `bandSplitMode`
- `lowCurtainGain`
- `midCurtainGain`
- `highCurtainGain`
- `bandSeparation`
- `edgeSparkle`
- `saturation`
- `temperature`

Audio behavior:

- Bass controls lower, broader curtains.
- Mids control the main folded curtain body.
- Highs control upper shimmer lines and sparkling edge points.

### Aurora Flow Field

Atmospheric support layer that can sit behind curtains.

Parameters:

- `fieldScale`
- `fieldStrength`
- `flowSpeed`
- `curlAmount`
- `turbulence`
- `density`
- `trailFade`
- `paletteRate`
- `backgroundAlpha`

Audio behavior:

- Bass increases vertical lift.
- Mids increase field strength and curl.
- Highs increase fine turbulence and sparkle.

### FFT Spectral Curtain

Future upgraded layer once spectrum bins are exposed.

Additional data needed:

```cpp
std::vector<float> spectrum;
std::vector<float> logSpectrum;
float spectralCentroid;
float spectralFlux;
```

Audio behavior:

- Low spectrum bins shape the lower curtain body.
- Mid bins create folds and drifting veils.
- High bins create fine striations.
- Spectral centroid shifts color temperature.
- Spectral flux triggers flare and shimmer events.

## Roadmap Phases

### Phase 0: Waveform Prototype

Goal: build an aurora layer from the current `AudioAnalysisBus` without changing the audio contract.

Tasks:

- Create `AuroraCurtainLayer` as a normal Synaptome `Layer`.
- Read `AudioAnalysisBus::Snapshot` directly.
- Draw smoothed waveform ribbons with history trails.
- Register Browser-visible parameters.
- Add a catalog asset under `synaptome/bin/data/layers/generative/`.
- Add a show preset scene or fixture once the layer behavior is stable.

Acceptance:

- Layer reacts to local mic waveform.
- Layer reacts to Signal Control Router UDP waveform packets.
- Layer remains visually active with no audio by using time/noise fallback.
- Bass/mids/highs visibly affect different parts of the curtain.

### Phase 1: Performance Controls

Goal: make the layer playable in a live set.

Tasks:

- Add palette controls for green/cyan/violet and sunset variants.
- Add peak-triggered flare sweep.
- Add optional mirrored curtains.
- Add audio smoothing, response curve, and deadband parameters.
- Add sensible default mappings for `mic-bass`, `mic-mids`, `mic-highs`, and `mic-peak`.

Acceptance:

- Operator can tune reactivity without editing code.
- Visual response remains stable across quiet passages and loud drops.
- No external audio route is required for the layer to render.

### Phase 2: Spectrum Bus Upgrade

Goal: expose reusable spectrum data to audio-reactive layers.

Tasks:

- Add optional `spectrum` and `logSpectrum` vectors to `AudioAnalysisBus::Snapshot`.
- Add derived features: `spectralCentroid` and `spectralFlux`.
- Decide whether Signal Control should send spectrum bins, or whether Synaptome should derive spectrum from waveform packets when possible.
- Update docs/contracts if spectrum becomes part of the public receive path.
- Keep broad `bass`, `mids`, and `highs` fields for backwards-compatible mappings.

Acceptance:

- Existing audio waveform layer continues to work.
- Aurora can use spectrum bins when present and fallback to broad bands when absent.
- Contract docs and fixtures clearly distinguish waveform packets from spectrum packets.

### Phase 3: Spectral Aurora Pack

Goal: ship the aurora vibe as a small layer pack.

Layers:

1. **Aurora Curtains**
   - Main waveform-driven curtain layer.

2. **Aurora Veil**
   - Softer background gas/flow layer.

3. **Spectral Shimmer**
   - Thin high-frequency particle or line accent layer.

4. **Magnetosphere Lines**
   - Optional field-line overlay for sci-fi or cosmic sets.

Acceptance:

- Each layer appears in the Browser catalog.
- Each layer can be loaded into a Console slot and saved in a scene.
- Each layer has defined audio/sensor mapping recipes.
- The pack can run together without frame-rate instability.

## External Library Candidates

| Library | Use | Recommendation |
| --- | --- | --- |
| ofxFft | Full FFT spectrum for local audio analysis. | Investigate first because the project already references ofxFft include paths in Visual Studio. |
| FastNoiseLite | Smooth atmospheric drift, turbulence, domain warp. | Good fit for aurora curtains and veil motion. |
| ofxFlowTools | Fluid-like aurora gas and optical-flow-driven motion. | Optional spike after waveform curtains are working. |

## Recommended Next Step

Build **Waveform Aurora Curtains** first using the current `AudioAnalysisBus`. It gives the strongest aurora identity with the least contract risk. Then add spectrum support only after the waveform layer proves what extra FFT detail would actually improve.

# Aurora Curtain Layer Implementation Plan

## Goal

Update `AuroraCurtainLayer` so it reads as a photographic, atmospheric aurora instead of white oscilloscope waveforms rising from the bottom of the frame.

The current layer is close architecturally: it already has audio input, waveform history, curtain meshes, trails, shimmer, striations, and peak flashes. The core issue is that the visible geometry is still driven too directly by the raw waveform. This makes the output look like vertical white waveform graphics rather than aurora curtains.

The desired result is a sound-reactive arctic night scene:

- A dark sky with faint atmosphere
- Large translucent green and cyan aurora sheets
- Occasional violet or magenta secondary color
- Bright but soft folded ridges
- Vertical falling light rays
- Slow persistent glow and trails
- Audio-reactive modulation that feels physical, not like an audio visualizer
- Compatibility with a later `ConstellationStarfieldLayer` and `ArcticSeaIcebergLayer`

Reference images show the aurora as broad atmospheric veils spanning the upper and middle sky, not as waveform lines starting at the bottom of the viewport.

## Existing files

Primary files:

```text
src/layers/AuroraCurtainLayer.cpp
src/layers/AuroraCurtainLayer.h
```

Important existing functions:

```cpp
void AuroraCurtainLayer::updateAudioState(float dt, float timeSeconds);
float AuroraCurtainLayer::targetSampleFor(float normalizedIndex, float timeSeconds) const;
glm::vec2 AuroraCurtainLayer::curtainPoint(...);
void AuroraCurtainLayer::drawCurtain(...);
void AuroraCurtainLayer::drawFlash(float width, float height, float alpha) const;
```

Important existing members:

```cpp
std::vector<float> waveform_;
std::vector<float> targetWaveform_;
std::deque<std::vector<float>> history_;
float level_;
float peak_;
float bass_;
float mids_;
float highs_;
float flash_;
```

## Diagnosis

### Current problem 1: waveform is visible as geometry

In `updateAudioState()`, the layer currently copies raw waveform samples into `targetWaveform_`:

```cpp
targetWaveform_[static_cast<std::size_t>(i)] = snapshotHasWaveform
    ? sampleBuffer(snapshot.waveform, t)
    : targetSampleFor(t, timeSeconds);
```

Then `curtainPoint()` uses those samples directly as vertical displacement:

```cpp
const float sample = sampleBuffer(samples, t);
const float waveformLift = sample * paramWaveformGain_ * paramVerticalScale_ * height * 0.14f * (0.70f + level_ * audio * 0.38f);
const float y = baseY - waveformLift + fold * height * 0.045f + tilt;
```

This makes the aurora edge trace the audio waveform. A waveform naturally reads as a waveform, not as aurora.

### Current problem 2: additive blending pushes colors to white

The layer draws all histories, all curtains, mirror curtains, glow lines, edge lines, striations, sparkles, and flash using additive blending:

```cpp
ofEnableBlendMode(OF_BLENDMODE_ADD);
```

The current edge alpha is also high:

```cpp
const float edgeAlpha = alpha * trail * (0.86f + idleLift + level_ * audio * 0.18f + flash_ * 0.16f);
```

With multiple overlapping green, blue, and white-tinted layers, the result saturates to white.

### Current problem 3: composition starts too low

Current `baseY`:

```cpp
const float baseY = height * (0.74f - curtainNorm * 0.28f) - bassLift + historyAge * height * 0.005f;
```

This anchors the curtain edge low in the frame. The aurora should mostly live in the sky, with the bottom third reserved for sea, ice, mountains, reflections, or foreground elements.

## Core implementation principle

Do not render the raw waveform as the aurora shape.

Instead, treat audio as a hidden modulation field:

```text
raw waveform -> absolute amplitude -> blurred energy field -> subtle modulation of curtain folds, opacity, depth, shimmer, and rays
```

The aurora should be shaped mostly by slow noise fields and broad folds. Audio should energize those fields, not draw itself directly.

## Sound-reactive mapping

Use audio bands this way:

```text
bass_  -> curtain height, global lift, horizon glow, sea reflection later
mids_  -> fold strength, magnetic twisting, slow billowing
highs_ -> shimmer, vertical ray density, star twinkle later
peak_  -> soft bloom pulse, never a hard white flash
level_ -> overall alpha, persistence, glow strength
```

Avoid this:

```text
waveform sample -> visible y position
```

Prefer this:

```text
abs(waveform sample) + FFT bands + noise -> energy field -> alpha, depth, shimmer, ray density, minor y offset
```

## Step 1: Rename the mental model from waveform to energy field

This can be done in two phases.

### Phase 1: Minimal code churn

Keep the existing member names:

```cpp
waveform_
targetWaveform_
history_
```

But change their contents so they store `0..1` aurora energy instead of `-1..1` raw waveform.

### Phase 2: Cleanup after behavior is correct

Optionally rename later:

```cpp
waveform_       -> energyField_
targetWaveform_ -> targetEnergyField_
ensureWaveformSize() -> ensureEnergyFieldSize()
targetSampleFor() -> targetEnergyFor()
```

Do not do the rename first. First get the visual behavior correct.

## Step 2: Replace raw waveform with a smoothed aurora energy field

In `updateAudioState()`, replace this loop:

```cpp
const float denom = static_cast<float>(std::max(1, sampleCount - 1));
for (int i = 0; i < sampleCount; ++i) {
    const float t = static_cast<float>(i) / denom;
    targetWaveform_[static_cast<std::size_t>(i)] = snapshotHasWaveform
        ? sampleBuffer(snapshot.waveform, t)
        : targetSampleFor(t, timeSeconds);
}
```

with this:

```cpp
const float denom = static_cast<float>(std::max(1, sampleCount - 1));
for (int i = 0; i < sampleCount; ++i) {
    const float t = static_cast<float>(i) / denom;

    // Use absolute waveform amplitude as subtle energy only.
    // Do not preserve signed waveform shape in the visible geometry.
    const float rawWave = snapshotHasWaveform
        ? std::abs(sampleBuffer(snapshot.waveform, t))
        : 0.0f;

    // Multi-scale slow fields create the photographic curtain structure.
    const float slowField =
        0.42f * ofNoise(t * 1.35f, timeSeconds * 0.035f, 1.0f) +
        0.34f * ofNoise(t * 3.80f, timeSeconds * 0.060f, 2.0f) +
        0.24f * ofNoise(t * 9.50f, timeSeconds * 0.110f, 3.0f);

    // FFT bands change the field without drawing hard audio shapes.
    const float spectralBias = hasAudio_
        ? bass_ * 0.34f +
          mids_ * (0.28f + 0.20f * ofNoise(t * 4.0f, timeSeconds * 0.080f, 4.0f)) +
          highs_ * 0.14f
        : 0.18f;

    const float audioRipple = rawWave * 0.10f;

    targetWaveform_[static_cast<std::size_t>(i)] = ofClamp(
        slowField * 0.78f + spectralBias * paramAudioAmount_ + audioRipple,
        0.0f,
        1.0f);
}
```

Rationale:

- `std::abs()` removes the oscilloscope up/down trace.
- Multi-scale noise creates broad atmospheric structure.
- FFT bands alter intensity and motion.
- Raw waveform contributes only a small ripple.
- Values become `0..1`, which is easier for alpha, depth, and ray density.

## Step 3: Update `targetSampleFor()` to return energy, not signed waveform

Current fallback:

```cpp
float AuroraCurtainLayer::targetSampleFor(float normalizedIndex, float timeSeconds) const {
    const float slow = std::sin(normalizedIndex * TWO_PI * 2.0f + timeSeconds * (0.38f + paramFlowSpeed_ * 0.12f));
    const float drift = signedNoise(normalizedIndex * 5.0f, timeSeconds * 0.08f, 2.1f);
    const float scalarEnergy = hasAudio_ ? ofClamp(level_ * 0.75f + bass_ * 0.25f, 0.0f, 1.0f) : 0.35f;
    return ofClamp((slow * 0.14f + drift * 0.20f) * (0.65f + scalarEnergy), -1.0f, 1.0f);
}
```

Replace with:

```cpp
float AuroraCurtainLayer::targetSampleFor(float normalizedIndex, float timeSeconds) const {
    const float t = normalizedIndex;
    const float scalarEnergy = hasAudio_
        ? ofClamp(level_ * 0.55f + bass_ * 0.30f + mids_ * 0.15f, 0.0f, 1.0f)
        : 0.35f;

    const float slowField =
        0.44f * ofNoise(t * 1.40f, timeSeconds * 0.030f, 11.0f) +
        0.36f * ofNoise(t * 4.20f, timeSeconds * 0.055f, 12.0f) +
        0.20f * ofNoise(t * 10.0f, timeSeconds * 0.090f, 13.0f);

    return ofClamp(slowField * (0.72f + scalarEnergy * 0.35f), 0.0f, 1.0f);
}
```

## Step 4: Reposition the aurora into the sky

In `curtainPoint()`, replace the low frame base position:

```cpp
const float baseY = height * (0.74f - curtainNorm * 0.28f) - bassLift + historyAge * height * 0.005f;
```

with sky placement:

```cpp
const float skyTop = height * 0.10f;
const float skyBottom = height * 0.62f;
const float baseY =
    ofLerp(skyBottom, skyTop, curtainNorm * 0.72f)
    - bassLift
    + historyAge * height * 0.012f;
```

This places the controlling ridge of the aurora in the upper and middle sky.

Recommended normalized composition:

```text
0.00 to 0.12: dark sky and stars
0.12 to 0.62: aurora curtains
0.50 to 0.72: horizon glow, distant mountains, low atmospheric haze
0.68 to 1.00: arctic sea, icebergs, reflections
```

## Step 5: Reduce direct waveform lift

In `curtainPoint()`, current lift is too large and signed:

```cpp
const float sample = sampleBuffer(samples, t);
const float waveformLift = sample * paramWaveformGain_ * paramVerticalScale_ * height * 0.14f * (0.70f + level_ * audio * 0.38f);
```

After Step 2, `sample` is an energy value in `0..1`. Replace with:

```cpp
const float energy = ofClamp(sampleBuffer(samples, t), 0.0f, 1.0f);
const float energyLift =
    energy * paramWaveformGain_ * paramVerticalScale_ * height * 0.050f *
    (0.65f + level_ * audio * 0.28f);
```

Then replace:

```cpp
const float y = baseY - waveformLift + fold * height * 0.045f + tilt;
```

with:

```cpp
const float y = baseY - energyLift + fold * height * 0.050f + tilt;
```

The key change is that energy only nudges the aurora. It no longer draws a waveform silhouette.

## Step 6: Make the curtain broad and translucent

In `drawCurtain()`, adjust alpha calculations.

Current values:

```cpp
const float veilAlpha = alpha * trail * (0.48f + idleLift * 0.34f + level_ * audio * 0.20f + flash_ * 0.10f);
const float lowerVeilAlpha = alpha * trail * (0.20f + idleLift * 0.18f + bass_ * audio * 0.08f + flash_ * 0.04f);
const float striationAlpha = alpha * trail * (0.30f + idleLift * 0.18f + highs_ * audio * 0.18f + flash_ * 0.08f);
const float edgeAlpha = alpha * trail * (0.86f + idleLift + level_ * audio * 0.18f + flash_ * 0.16f);
```

Replace with:

```cpp
const float veilAlpha = alpha * trail *
    (0.62f + idleLift * 0.22f + level_ * audio * 0.16f + bass_ * audio * 0.10f);

const float lowerVeilAlpha = alpha * trail *
    (0.24f + idleLift * 0.12f + bass_ * audio * 0.10f + mids_ * audio * 0.05f);

const float striationAlpha = alpha * trail *
    (0.24f + idleLift * 0.12f + highs_ * audio * 0.20f + flash_ * 0.04f);

const float edgeAlpha = alpha * trail *
    (0.20f + idleLift * 0.16f + highs_ * audio * 0.10f + flash_ * 0.04f);
```

Intent:

- Veil becomes dominant.
- Edge becomes subtle.
- High frequencies affect shimmer and striations, not the whole curtain.
- Peak flash stops whitening the whole mesh.

## Step 7: Use alpha blending for the veil, additive blending for light accents

Right now all curtain drawing happens under additive blending in `draw()`.

A better model:

- Draw broad mesh veils with alpha blending.
- Draw internal glow, rays, subtle edge, sparkle, and flash with additive blending.

Minimal implementation path:

1. In `draw()`, keep the existing loop structure.
2. Inside `drawCurtain()`, switch blend mode locally before drawing each component.

Recommended order inside `drawCurtain()`:

```cpp
ofEnableBlendMode(OF_BLENDMODE_ALPHA);
veil.draw();

ofEnableBlendMode(OF_BLENDMODE_ADD);
innerGlow.draw();
lowerGlow.draw();
striations.draw();
glow.draw();
edge.draw();
```

Then remove or reduce the global assumption that everything is additive.

In `draw()`, this is acceptable:

```cpp
ofEnableBlendMode(OF_BLENDMODE_ALPHA);
drawHorizonGlow(width, height, alpha);

for (...) {
    drawCurtain(...);
}

ofEnableBlendMode(OF_BLENDMODE_ADD);
drawFlash(width, height, alpha);
```

## Step 8: Reduce line widths and edge dominance

Current edge glow line is too thick:

```cpp
glLineWidth(std::max(1.0f, paramLineThickness_ * (1.7f + paramGlowAmount_ * 1.15f)));
```

Replace with:

```cpp
glLineWidth(std::max(1.0f, paramLineThickness_ * (0.70f + paramGlowAmount_ * 0.28f)));
```

Current final edge line:

```cpp
glLineWidth(std::max(0.5f, paramLineThickness_));
edge.draw();
```

Replace with:

```cpp
glLineWidth(std::max(0.5f, paramLineThickness_ * 0.55f));
edge.draw();
```

Or, for the most photographic look, omit the final `edge.draw()` for all but the most recent history frame:

```cpp
if (historyIndex == 0) {
    edge.draw();
}
```

## Step 9: Make vertical rays more aurora-like

Current ray end follows the curtain mesh direction:

```cpp
const glm::vec2 rayEnd = topPoint + (bottomPoint - topPoint) * rayDepth;
```

This can work, but it often reads as geometric striping. Aurora rays should feel mostly vertical, with slight magnetic sway.

Replace with:

```cpp
const float verticalSway =
    signedNoise(t * 6.0f, timeSeconds * 0.080f, curtainNorm * 4.0f) *
    width * 0.018f;

const glm::vec2 rayStart = topPoint;
const glm::vec2 rayEnd = topPoint + glm::vec2(
    verticalSway,
    (mirror ? -1.0f : 1.0f) * depth * rayDepth * (0.74f + bass_ * audio * 0.24f));
```

Then replace:

```cpp
striations.addVertex(glm::vec3(topPoint.x, topPoint.y, 0.0f));
striations.addColor(rayTop);
striations.addVertex(glm::vec3(rayEnd.x, rayEnd.y, 0.0f));
striations.addColor(rayBottom);
```

with:

```cpp
striations.addVertex(glm::vec3(rayStart.x, rayStart.y, 0.0f));
striations.addColor(rayTop);
striations.addVertex(glm::vec3(rayEnd.x, rayEnd.y, 0.0f));
striations.addColor(rayBottom);
```

Also increase ray variation based on energy:

```cpp
const float energy = ofClamp(sampleBuffer(samples, t), 0.0f, 1.0f);
const float rayDepth = ofClamp(0.30f + streak * 0.56f + energy * 0.16f, 0.22f, 1.0f);
```

## Step 10: Add horizon glow

The reference images have a green glow band near the horizon. Add this to make the scene feel photographic and to bridge the aurora into the planned arctic sea layer.

Add declaration in `AuroraCurtainLayer.h`:

```cpp
void drawHorizonGlow(float width, float height, float alpha) const;
```

Add implementation in `AuroraCurtainLayer.cpp`:

```cpp
void AuroraCurtainLayer::drawHorizonGlow(float width, float height, float alpha) const {
    ofMesh glow;
    glow.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

    const float y0 = height * 0.52f;
    const float y1 = height * 0.80f;

    const float glowEnergy = hasAudio_
        ? ofClamp(0.30f + bass_ * 0.45f + level_ * 0.20f, 0.0f, 1.0f)
        : 0.34f;

    ofFloatColor top = colorFrom(
        paramColorR_ * 0.45f,
        paramColorG_ * 0.78f,
        paramColorB_ * 0.55f,
        alpha * (0.08f + glowEnergy * 0.12f));

    ofFloatColor bottom = colorFrom(
        paramBgR_,
        paramBgG_,
        paramBgB_,
        0.0f);

    glow.addVertex(glm::vec3(0.0f, y0, 0.0f));
    glow.addColor(top);
    glow.addVertex(glm::vec3(0.0f, y1, 0.0f));
    glow.addColor(bottom);
    glow.addVertex(glm::vec3(width, y0, 0.0f));
    glow.addColor(top);
    glow.addVertex(glm::vec3(width, y1, 0.0f));
    glow.addColor(bottom);

    glow.draw();
}
```

Call it in `draw()` after the background rectangle and before curtains:

```cpp
if (paramBgAlpha_ > 0.0f) {
    setColor(colorFrom(paramBgR_, paramBgG_, paramBgB_, paramBgAlpha_ * params.slotOpacity));
    ofDrawRectangle(0.0f, 0.0f, width, height);
}

ofEnableBlendMode(OF_BLENDMODE_ALPHA);
drawHorizonGlow(width, height, alpha);
```

## Step 11: Tune default parameters

Current defaults in `AuroraCurtainLayer.h` are aggressive and line-heavy.

Replace defaults with these starting values:

```cpp
float paramCurtainCount_ = 3.0f;
float paramSampleDensity_ = 220.0f;
float paramWaveformGain_ = 0.38f;
float paramVerticalScale_ = 0.34f;
float paramCurtainHeight_ = 0.48f;
float paramFoldStrength_ = 1.15f;
float paramFlowSpeed_ = 0.12f;
float paramNoiseScale_ = 2.8f;
float paramTrailDecay_ = 0.20f;
float paramLineThickness_ = 2.0f;
float paramGlowAmount_ = 1.35f;
float paramShimmerAmount_ = 0.65f;
float paramMagneticTilt_ = -0.12f;
float paramBassLift_ = 0.56f;
float paramMidsFold_ = 1.05f;
float paramHighsSparkle_ = 1.10f;
float paramPeakFlash_ = 0.38f;
float paramAudioAmount_ = 0.82f;
float paramAudioSmoothing_ = 0.56f;
float paramBgAlpha_ = 0.18f;

float paramBgR_ = 0.003f;
float paramBgG_ = 0.007f;
float paramBgB_ = 0.018f;

float paramColorR_ = 0.06f;
float paramColorG_ = 0.95f;
float paramColorB_ = 0.42f;

float paramColor2R_ = 0.42f;
float paramColor2G_ = 0.22f;
float paramColor2B_ = 0.85f;
```

Notes:

- Lower `paramLineThickness_` and `paramGlowAmount_` reduce white saturation.
- Higher `paramAudioSmoothing_` makes audio response more aurora-like.
- Lower `paramPeakFlash_` prevents beat hits from becoming white flashes.
- Green primary with violet secondary matches the references.

## Step 12: Adjust color mixing to avoid white

Current ray and sparkle colors lerp toward pure white:

```cpp
ofFloatColor rayTop = topColor.getLerped(ofFloatColor(1.0f, 1.0f, 1.0f, 1.0f), 0.18f + streak * 0.20f);
```

Reduce white mixing:

```cpp
ofFloatColor paleGreen = colorFrom(0.72f, 1.0f, 0.78f, 1.0f);
ofFloatColor rayTop = topColor.getLerped(paleGreen, 0.10f + streak * 0.12f);
```

For sparkles, avoid full white:

```cpp
ofFloatColor spark = colorB.getLerped(colorFrom(0.72f, 1.0f, 0.84f, 1.0f), 0.35f);
```

## Step 13: Soften `drawFlash()`

Current `drawFlash()` draws a vertical rectangle and a line over most of the viewport:

```cpp
ofDrawRectangle(x - bandWidth * 0.5f, 0.0f, bandWidth, height);
ofDrawLine(x, height * 0.06f, x, height * 0.94f);
```

This reads more like a scanner bar than aurora. Replace with a soft atmospheric bloom band limited to the aurora sky region:

```cpp
void AuroraCurtainLayer::drawFlash(float width, float height, float alpha) const {
    if (flash_ <= 0.002f || paramPeakFlash_ <= 0.0f) {
        return;
    }

    const float x = ofClamp(flashPhase_, 0.0f, 1.0f) * width;
    const float bandWidth = width * (0.08f + flash_ * 0.035f);
    const float y0 = height * 0.10f;
    const float y1 = height * 0.66f;

    ofMesh bloom;
    bloom.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);

    const ofFloatColor center = colorFrom(0.50f, 1.0f, 0.70f, ofClamp(alpha * flash_ * 0.13f, 0.0f, 0.30f));
    const ofFloatColor edge = colorFrom(0.30f, 0.75f, 0.55f, 0.0f);

    bloom.addVertex(glm::vec3(x - bandWidth, y0, 0.0f));
    bloom.addColor(edge);
    bloom.addVertex(glm::vec3(x - bandWidth, y1, 0.0f));
    bloom.addColor(edge);

    bloom.addVertex(glm::vec3(x, y0, 0.0f));
    bloom.addColor(center);
    bloom.addVertex(glm::vec3(x, y1, 0.0f));
    bloom.addColor(center);

    bloom.addVertex(glm::vec3(x + bandWidth, y0, 0.0f));
    bloom.addColor(edge);
    bloom.addVertex(glm::vec3(x + bandWidth, y1, 0.0f));
    bloom.addColor(edge);

    bloom.draw();
}
```

Call this under additive blend, but with the reduced alpha above.

## Step 14: Prevent mirrored curtain from confusing the scene

`paramMirror_` currently draws a reflected secondary curtain. For the arctic scene, turn this off by default:

```cpp
bool paramMirror_ = false;
```

Do not use mirror as a fake reflection. The future `ArcticSeaIcebergLayer` should own water reflection so it can be clipped to the sea surface and distorted by waves.

## Step 15: Planned companion layers

Do not put stars or sea geometry into `AuroraCurtainLayer`. Keep this layer focused on atmospheric aurora. Build the full scene using separate layers.

### Layer stack

```text
1. ConstellationStarfieldLayer
2. AuroraCurtainLayer
3. ArcticSeaIcebergLayer
```

### `ConstellationStarfieldLayer`

Responsibilities:

- Dark starfield behind aurora
- Slow parallax drift
- Occasional constellation lines
- High frequencies create subtle twinkle
- Bass or aurora bloom slightly dims stars so the aurora feels bright

Suggested audio mapping:

```text
highs_ -> twinkle intensity
mids_  -> constellation line fade-ins
bass_  -> slight dimming during aurora bloom
level_ -> global sky atmosphere
```

### `ArcticSeaIcebergLayer`

Responsibilities:

- Occupy lower 25 to 35 percent of frame
- Dark ocean plane or stylized arctic water
- Icebergs passing through foreground
- Subtle green aurora reflections
- Distant horizon haze
- Optional low-poly mountains or ice shelf silhouette

Suggested audio mapping:

```text
bass_  -> wave amplitude and slow camera bob
mids_  -> iceberg drift variation
highs_ -> small water sparkles
level_ -> reflection strength
peak_  -> soft ripple bloom
```

This layer should occlude the bottom of the aurora so the aurora no longer appears to originate from the bottom edge of the screen.

## Acceptance criteria

The implementation is successful when:

- The layer no longer reads as white waveform bars.
- The main visible form is broad, translucent green/cyan aurora curtains.
- Purple or violet appears as a secondary atmospheric accent, not as white saturation.
- Audio clearly affects the aurora, but the raw waveform is not visually traceable.
- Bass creates lift, scale, and glow.
- Mids create folding and slow billowing.
- Highs create shimmer and ray activity.
- Peaks create soft bloom, not a scanner line or white flash.
- The aurora mostly lives above the lower third of the frame.
- The bottom third is visually available for a future arctic sea and iceberg foreground.
- Default parameters look usable without requiring heavy manual tweaking.

## Implementation checklist

1. Change `updateAudioState()` so `targetWaveform_` stores a `0..1` aurora energy field.
2. Change `targetSampleFor()` so fallback animation returns slow positive energy instead of signed waveform values.
3. Update `curtainPoint()` to place the aurora in the sky and reduce direct vertical audio displacement.
4. Rename local `sample` usage to `energy` where possible for clarity.
5. Increase veil dominance and reduce edge alpha in `drawCurtain()`.
6. Draw broad veil with alpha blending.
7. Draw glow, rays, edge, sparkles, and flash with additive blending.
8. Reduce edge line width and optionally draw hard edge only for `historyIndex == 0`.
9. Make striations mostly vertical with slight magnetic sway.
10. Add `drawHorizonGlow()` and call it before curtains.
11. Replace current default parameters with the tuned defaults in this document.
12. Reduce lerps toward pure white in ray and sparkle colors.
13. Replace `drawFlash()` scanner-bar behavior with a soft sky-limited bloom.
14. Leave starfield and arctic sea for separate layers.

## Suggested commit message

```text
Refactor aurora curtains from waveform visualizer to atmospheric energy field
```

## Suggested PR summary

```text
This update changes AuroraCurtainLayer so audio drives a smoothed aurora energy field rather than directly drawing waveform geometry. It repositions the curtains into the sky, reduces additive white saturation, emphasizes translucent veils over hard edges, adds vertical aurora rays, introduces a horizon glow, and retunes defaults for a photographic green/violet aurora look. The result remains sound reactive while leaving the lower frame available for a future arctic sea and iceberg foreground layer.
```

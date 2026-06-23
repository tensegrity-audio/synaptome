# SolarSystemLayer Scientific Orrery Redesign

## Purpose

This document describes how to revise `SolarSystemLayer` so it reads as a physically grounded, low-poly scientific orrery rather than a decorative or kitschy space scene.

The desired aesthetic is:

- Low-poly geometry, but physically motivated motion and lighting
- Slower, more astronomical movement
- A sun that visibly radiates energy and acts as the dominant light source
- Planetary dynamics based on observed orbital data
- Audio reactivity that modulates plausible solar and atmospheric phenomena, not orbital mechanics
- A calmer, more desaturated, observational color palette

The current layer already contains useful observed exoplanet data and strong visual building blocks. The main task is to remove stylized shortcuts that break the physical read and replace them with coherent scientific behavior.

## Files to modify

- `src/layers/SolarSystemLayer.h`
- `src/layers/SolarSystemLayer.cpp`

Exact paths may vary depending on the codebase layout, but the relevant files are the current `SolarSystemLayer` implementation and header.

## Current problems

### 1. The system mixes unrelated observed planets

`resetSystem()` selects a real host system, then appends extra planets from unrelated host systems using `paramObservedDiversity_`.

Current pattern:

```cpp
const int targetExtra = static_cast<int>(std::round(paramObservedDiversity_ * 5.0f));
for (int i = 0; i < targetExtra; ++i) {
    const auto& data = kObservedPlanets[static_cast<std::size_t>(randInt(rng, 0, static_cast<int>(kObservedPlanets.size()) - 1))];
    if (randRange(rng, 0.0f, 1.0f) <= paramObservedDiversity_) {
        selected.push_back(&data);
    }
}
```

This creates a visually rich system, but it breaks the feeling of a coherent gravitational system.

### 2. Orbital speeds are visually mapped and randomized

Current pattern:

```cpp
body.speed = logMap(data.periodDays, 0.55f, 180000.0f, 1.85f, 0.08f) * randRange(rng, 0.82f, 1.18f);
```

This makes planets feel like animated beads. The relationship between observed orbital period and visual motion should be preserved.

### 3. Audio warps planet orbit radius

Current `orbitPointFor(const Body...)` samples waveform data and adds it to the orbit radius:

```cpp
const float sample = (hasWaveform_ ? waveformSampleFor(wrap01(waveformPhase)) : 0.0f);
const float waveformOffset = sample * paramWaveformAmount_ * radiusScale * (0.22f + level_ * paramAudioAmount_);
const float radius = body.orbit * radiusScale + extraRadius + waveformOffset;
```

This makes the solar system look like a waveform visualizer. Audio should not move planets off their orbital paths in the realistic mode.

### 4. Planet lighting uses a fixed arbitrary direction

`drawLowPolySphere()` currently uses a fixed light vector:

```cpp
const glm::vec3 light = safeNormalize(glm::vec3(-0.38f, -0.62f, 0.70f));
```

Planets should be lit by the star at the center. This is critical for realism because it creates terminators and makes the sun feel physically present.

### 5. Planet colors are too saturated and fantasy-like

`planetColorFor()` currently introduces teal, violet, rose, cobalt, hot pink, and other high-saturation colors. This is visually fun, but it pushes the scene toward arcade or toy aesthetics.

### 6. Trails and waveform belt read as neon visualizer elements

`drawBodyTrail()` lerps trails toward each planet color, and `drawWaveformBelt()` draws an explicitly audio-like cyan belt. These should become subtle orbital persistence and heliospheric field visualization.

## Design goals

1. Preserve the low-poly look.
2. Make the motion obey observed relative orbital periods.
3. Keep planets on stable elliptical paths.
4. Make the star the dominant visual and lighting source.
5. Make audio reactivity physically plausible.
6. Reduce saturation and visual gimmicks.
7. Keep all changes compatible with the existing parameter registry system.

## Scientific basis to encode

Use the following principles as implementation constraints:

- Orbital period controls orbital speed. Short-period planets move faster than long-period planets.
- Semi-major axis controls orbit size.
- Eccentricity controls ellipse shape.
- The central star sits at one focus of the ellipse, not the center of a decorative oval.
- The star emits light outward, so planet shading should depend on position relative to the origin.
- Solar wind, corona, and magnetic-field-like visual effects are plausible places for audio reactivity.
- Audio should not directly change observed orbital radius, orbital period, eccentricity, or semi-major axis unless a non-realistic mode is explicitly enabled.

## Parameter changes

Update the default values in `SolarSystemLayer.h` to make the default mode calmer and more scientific.

### Current defaults to change

```cpp
bool paramShowOrbits_ = false;
bool paramShowWaveformBelt_ = true;
float paramOrbitSpeed_ = 0.18f;
float paramStarSize_ = 0.052f;
float paramStarGlow_ = 1.35f;
float paramStarRadiance_ = 1.15f;
float paramPlanetSize_ = 0.78f;
float paramObservedDiversity_ = 0.65f;
float paramPlanetVariation_ = 0.72f;
float paramOrbitAlpha_ = 0.32f;
float paramTrailAlpha_ = 0.52f;
float paramTrailLength_ = 0.34f;
float paramAudioAmount_ = 1.0f;
float paramBassScale_ = 0.55f;
float paramMidsSpeed_ = 0.85f;
float paramHighsSparkle_ = 1.15f;
float paramWaveformAmount_ = 0.055f;
```

### Recommended defaults

```cpp
bool paramShowOrbits_ = true;
bool paramShowTrails_ = true;
bool paramShowMoons_ = true;
bool paramShowRings_ = true;
bool paramShowAsteroids_ = true;
bool paramShowComets_ = true;
bool paramShowWaveformBelt_ = false;

float paramOrbitSpeed_ = 0.012f;
float paramStarSize_ = 0.070f;
float paramStarGlow_ = 2.4f;
float paramStarRadiance_ = 2.6f;
float paramPlanetSize_ = 0.62f;
float paramObservedDiversity_ = 0.0f;
float paramPlanetVariation_ = 0.28f;

float paramOrbitAlpha_ = 0.12f;
float paramOrbitThickness_ = 0.85f;
float paramTrailAlpha_ = 0.18f;
float paramTrailLength_ = 0.12f;
float paramTrailSteps_ = 36.0f;

float paramAudioAmount_ = 0.65f;
float paramBassScale_ = 0.75f;
float paramMidsSpeed_ = 0.10f;
float paramHighsSparkle_ = 0.45f;
float paramWaveformAmount_ = 0.0f;
```

### Rename misleading parameter labels

In `setup()`, several labels currently imply audio affects orbital mechanics. Rename them to clarify the new behavior.

Current labels:

```cpp
registerFloat(registry, prefix + ".bassScale", &paramBassScale_, paramBassScale_, "Bass Orbit Scale", 0.0f, 1.5f, 0.01f);
registerFloat(registry, prefix + ".midsSpeed", &paramMidsSpeed_, paramMidsSpeed_, "Mids Speed Lift", 0.0f, 3.0f, 0.01f);
registerFloat(registry, prefix + ".waveformAmount", &paramWaveformAmount_, paramWaveformAmount_, "Waveform Orbit Warp", 0.0f, 0.20f, 0.001f);
```

Recommended labels:

```cpp
registerFloat(registry, prefix + ".bassScale", &paramBassScale_, paramBassScale_, "Bass Solar Bloom", 0.0f, 1.5f, 0.01f);
registerFloat(registry, prefix + ".midsSpeed", &paramMidsSpeed_, paramMidsSpeed_, "Mids Solar Wind", 0.0f, 3.0f, 0.01f);
registerFloat(registry, prefix + ".waveformAmount", &paramWaveformAmount_, paramWaveformAmount_, "Waveform Field Ripple", 0.0f, 0.20f, 0.001f);
```

## Implementation steps

## Step 1: Make observed host systems coherent by default

In `resetSystem()`, remove or disable cross-host planet injection for the default scientific mode.

### Replace this block

```cpp
const int targetExtra = static_cast<int>(std::round(paramObservedDiversity_ * 5.0f));
for (int i = 0; i < targetExtra; ++i) {
    const auto& data = kObservedPlanets[static_cast<std::size_t>(randInt(rng, 0, static_cast<int>(kObservedPlanets.size()) - 1))];
    if (randRange(rng, 0.0f, 1.0f) <= paramObservedDiversity_) {
        selected.push_back(&data);
    }
}
```

### With either nothing, or an explicit non-realistic mode

```cpp
// Scientific mode: preserve one observed host system.
// Do not mix planets from unrelated systems here.
```

If keeping the feature is important, add a new boolean such as:

```cpp
bool paramHybridSystemMode_ = false;
```

Then only run cross-host injection when that mode is enabled.

```cpp
if (paramHybridSystemMode_) {
    const int targetExtra = static_cast<int>(std::round(paramObservedDiversity_ * 5.0f));
    for (int i = 0; i < targetExtra; ++i) {
        const auto& data = kObservedPlanets[static_cast<std::size_t>(randInt(rng, 0, static_cast<int>(kObservedPlanets.size()) - 1))];
        if (randRange(rng, 0.0f, 1.0f) <= paramObservedDiversity_) {
            selected.push_back(&data);
        }
    }
}
```

Acceptance criteria:

- With default parameters, all visible planets belong to the selected `sourceSystem_`.
- The selected system name remains meaningful.
- No unrelated host planets are added unless an explicit hybrid/fantasy mode is turned on.

## Step 2: Replace randomized visual speed with period-based speed

In `resetSystem()`, replace the current speed assignment.

### Replace

```cpp
body.speed = logMap(data.periodDays, 0.55f, 180000.0f, 1.85f, 0.08f) * randRange(rng, 0.82f, 1.18f);
```

### With

```cpp
const float referencePeriodDays = 365.25f;
body.speed = referencePeriodDays / std::max(0.1f, data.periodDays);
```

This keeps the visual system compressed in scale, but preserves relative orbital period. Very short-period planets will still move faster, while long-period planets will barely move.

Important: keep the global `paramOrbitSpeed_` low. Suggested default is `0.012f`.

Acceptance criteria:

- Inner planets move faster than outer planets.
- Outer planets appear nearly static over short viewing periods.
- No random multiplier is applied to observed orbital period.
- Changing `paramOrbitSpeed_` scales the whole system uniformly.

## Step 3: Remove audio warping from body orbits

In `orbitPointFor(const Body&...)`, remove waveform radius modulation.

### Replace

```cpp
const float sample = (hasWaveform_ ? waveformSampleFor(wrap01(waveformPhase)) : 0.0f);
const float waveformOffset = sample * paramWaveformAmount_ * radiusScale * (0.22f + level_ * paramAudioAmount_);
const float radius = body.orbit * radiusScale + extraRadius + waveformOffset;
```

### With

```cpp
const float radius = body.orbit * radiusScale + extraRadius;
```

Also update call sites if `waveformPhase` is no longer needed. The least invasive option is to keep the function signature for now, mark `waveformPhase` unused, and remove the internal waveform effect.

```cpp
(void)waveformPhase;
const float radius = body.orbit * radiusScale + extraRadius;
```

Acceptance criteria:

- Planet positions do not visibly wobble with waveform input.
- Audio changes radiance, field lines, atmosphere, or star effects, but not orbital radius.
- Setting `paramWaveformAmount_` to a nonzero value does not affect planet orbit paths.

## Step 4: Implement physically correct ellipse geometry for bodies

Replace the approximate ellipse in `orbitPointFor(const Body&...)` with a Kepler-style ellipse where the star is at one focus.

### Replace current body orbit math

```cpp
const float eccentricity = ofClamp(body.eccentricity * (0.55f + paramEccentricity_ * 1.35f), 0.0f, 0.86f);
const float semiMajor = radius * (1.0f + eccentricity * 0.32f);
const float semiMinor = semiMajor * std::sqrt(std::max(0.04f, 1.0f - eccentricity * eccentricity));
glm::vec3 local(std::cos(angle) * semiMajor - eccentricity * semiMajor * 0.42f,
                0.0f,
                std::sin(angle) * semiMinor);
```

### With

```cpp
const float e = ofClamp(body.eccentricity * paramEccentricity_, 0.0f, 0.86f);
const float a = radius;
const float b = a * std::sqrt(std::max(0.001f, 1.0f - e * e));

// Treat input angle as mean anomaly M.
const float M = angle;
float E = M;
for (int i = 0; i < 5; ++i) {
    E = E - (E - e * std::sin(E) - M) / std::max(0.0001f, 1.0f - e * std::cos(E));
}

glm::vec3 local(a * (std::cos(E) - e),
                0.0f,
                b * std::sin(E));
```

Keep the existing inclination and yaw rotation after this:

```cpp
local = rotateX(local, body.inclination);
local = rotateY(local, body.orbitYaw);
return local;
```

Acceptance criteria:

- Elliptical orbits have the star at one focus.
- Eccentric systems visibly accelerate near periapsis if using mean anomaly with Kepler solve.
- `paramEccentricity_ = 0` produces circular orbits.
- `paramEccentricity_ = 1` uses the observed eccentricity without exaggerated decorative scaling.

## Step 5: Make the sun the lighting source for planets

Add a new sphere drawing function that accepts a local light direction.

### Header addition

In `SolarSystemLayer.h`, add:

```cpp
void drawLowPolySphereLit(float radius,
                          const ofFloatColor& color,
                          float alpha,
                          float seed,
                          int rings,
                          int segments,
                          bool wireframe,
                          const glm::vec3& lightDirLocal) const;
```

The existing `drawLowPolySphere()` can remain as a wrapper that calls the lit version with the old fixed direction for non-planet uses.

### Implementation strategy

Move the body of `drawLowPolySphere()` into `drawLowPolySphereLit()` and replace:

```cpp
const glm::vec3 light = safeNormalize(glm::vec3(-0.38f, -0.62f, 0.70f));
```

With:

```cpp
const glm::vec3 light = safeNormalize(lightDirLocal);
```

Then make the original function call the new one:

```cpp
void SolarSystemLayer::drawLowPolySphere(float radius,
                                         const ofFloatColor& color,
                                         float alpha,
                                         float seed,
                                         int rings,
                                         int segments,
                                         bool wireframe) const {
    drawLowPolySphereLit(radius,
                         color,
                         alpha,
                         seed,
                         rings,
                         segments,
                         wireframe,
                         glm::vec3(-0.38f, -0.62f, 0.70f));
}
```

### Planet draw call change

In `draw()`, when drawing each body, compute the light direction from planet position back to the star at origin.

Add before drawing the planet sphere:

```cpp
const glm::vec3 sunDirectionLocal = safeNormalize(-position);
```

Replace the planet sphere call:

```cpp
drawLowPolySphere(planetRadius,
                  ofFloatColor(planetColor.r, planetColor.g, planetColor.b, 1.0f),
                  alpha,
                  body.seed,
                  4,
                  8,
                  false);
```

With:

```cpp
drawLowPolySphereLit(planetRadius,
                     ofFloatColor(planetColor.r, planetColor.g, planetColor.b, 1.0f),
                     alpha,
                     body.seed,
                     4,
                     8,
                     false,
                     sunDirectionLocal);
```

Important note: if the planet is rotated after translation, make sure the light direction is transformed consistently. The simplest first pass is to compute `sunDirectionLocal` before the planet spin rotations and call the lit sphere before applying spin. If keeping planet spin rotation, the terminator may rotate with the mesh unless you inverse-transform the light vector. That is acceptable for a first implementation, but the final preferred behavior is a stable sun-facing terminator.

Acceptance criteria:

- Planets have a clear sun-facing lit side and a darker far side.
- The terminator direction changes with orbital position.
- The sun appears to be the source of illumination.

## Step 6: Strengthen solar radiance and corona

The sun should dominate the scene. Keep the low-poly star core, but make the surrounding corona more luminous and radial.

Current `drawStarRadiance()` already draws rays and corona shells. Rework it so audio affects the corona, not orbital mechanics.

Recommended behavior:

```cpp
const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
const float bassLuminosity = 1.0f + bass_ * paramBassScale_ * audio * 0.65f;
const float midWind = 1.0f + mids_ * paramMidsSpeed_ * audio * 0.35f;
const float highSpark = highs_ * paramHighsSparkle_ * audio;
```

Use these values as follows:

- `bassLuminosity`: scale corona alpha and outer radius
- `midWind`: increase ray curl or ray length subtly
- `highSpark`: add tiny coronal points or short line glints near the corona edge

Avoid using audio to scale `radiusScale` in a way that changes planet orbit spacing.

In `draw()`, change:

```cpp
const float radiusScale = minDim * 0.40f * paramScale_ * paramOrbitSpread_ * (1.0f + bassBloom * 0.24f);
```

To:

```cpp
const float radiusScale = minDim * 0.40f * paramScale_ * paramOrbitSpread_;
```

Then apply bass to the star only:

```cpp
const float starRadius = minDim * paramStarSize_ *
    (1.0f + bass_ * paramBassScale_ * audio * 0.22f + pulseEnvelope_ * 0.20f);
```

Acceptance criteria:

- Bass makes the sun breathe or bloom.
- Orbit spacing does not expand and contract with bass.
- The star feels like an energy source, not a decorative ball.

## Step 7: Replace waveform belt with heliosphere field lines

The existing `drawWaveformBelt()` reads as a visible music visualizer. Replace or rename it conceptually as a heliosphere field effect.

### Header rename option

Replace:

```cpp
void drawWaveformBelt(float radiusScale, float alpha, float timeSeconds) const;
```

With:

```cpp
void drawHeliosphereField(float radiusScale, float alpha, float timeSeconds) const;
```

Also replace `paramShowWaveformBelt_` with a clearer name if possible:

```cpp
bool paramShowHeliosphereField_ = true;
```

If compatibility matters, keep the old parameter ID but change the label and behavior.

### Desired visual behavior

- Field lines originate near the star.
- Lines travel outward through the inner system.
- Lines have a slight spiral or Parker-spiral-like curve.
- Waveform controls small radial pressure ripples along the lines.
- Bass controls brightness.
- Mids control spiral/curl strength.
- Highs control tiny sparks or bright knots.

### Example implementation sketch

```cpp
void SolarSystemLayer::drawHeliosphereField(float radiusScale, float alpha, float timeSeconds) const {
    if (alpha <= 0.0f) {
        return;
    }

    const float audio = hasAudio_ ? paramAudioAmount_ : 0.0f;
    const float fieldAlpha = alpha * (0.045f + bass_ * paramBassScale_ * audio * 0.10f);
    const int lines = 48;
    const int steps = 40;
    const float inner = radiusScale * 0.045f;
    const float outer = radiusScale * 0.92f;
    const float curl = 0.18f + mids_ * paramMidsSpeed_ * audio * 0.18f;

    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_LINES);

    for (int l = 0; l < lines; ++l) {
        const float base = static_cast<float>(l) / static_cast<float>(lines) * TWO_PI;
        const float lineSeed = static_cast<float>(l) * 0.137f + seedState_ * 0.0001f;

        glm::vec3 prev;
        bool hasPrev = false;

        for (int s = 0; s < steps; ++s) {
            const float pct = static_cast<float>(s) / static_cast<float>(steps - 1);
            const float sample = hasWaveform_ ? waveformSampleFor(wrap01(pct + lineSeed)) : 0.0f;
            const float r = ofLerp(inner, outer, pct) + sample * paramWaveformAmount_ * radiusScale * 0.08f;
            const float angle = base + pct * curl * TWO_PI + timeSeconds * 0.015f;
            const float y = std::sin(base * 2.0f + pct * 5.0f + timeSeconds * 0.11f) * radiusScale * 0.018f;

            glm::vec3 p(std::cos(angle) * r, y, std::sin(angle) * r);
            p = rotateX(p, 0.10f * std::sin(base));

            if (hasPrev) {
                const float fade = (1.0f - pct) * (1.0f - pct);
                ofFloatColor c = sourceStarColor_.getLerped(ofFloatColor(0.70f, 0.86f, 1.0f, 1.0f), 0.25f);
                c.a = fieldAlpha * fade * (0.55f + std::abs(sample) * 0.80f);
                mesh.addVertex(prev);
                mesh.addColor(c);
                mesh.addVertex(p);
                mesh.addColor(c);
            }

            prev = p;
            hasPrev = true;
        }
    }

#ifndef TARGET_OPENGLES
    glLineWidth(std::max(1.0f, paramOrbitThickness_ * 0.55f));
#endif
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    glDepthMask(GL_FALSE);
    mesh.draw();
    glDepthMask(GL_TRUE);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
}
```

Acceptance criteria:

- The effect reads as solar wind or magnetic field lines, not a waveform ring.
- Waveform is still visible, but only as subtle pressure ripple.
- The field radiates from the star.
- The field does not intersect the visual logic of planet orbits too aggressively.

## Step 8: Desaturate planet colors

Rewrite `planetColorFor()` to use observationally plausible, lower-saturation palettes.

### Desired classes

| Planet type | Radius range | Palette |
|---|---:|---|
| Rocky | `< 1.45 Earth radii` | basalt, dusty tan, iron oxide, muted blue-gray |
| Super-Earth / mini-Neptune | `1.45 to 4.3` | pale cyan, gray-blue, muted methane haze |
| Ice giant | `4.3 to 9.5` | desaturated cyan, gray teal, soft blue |
| Gas giant | `> 9.5` | cream, ochre, tan, gray-brown, muted orange |

### Implementation guidance

Remove or greatly reduce this outlier block:

```cpp
const std::array<ofFloatColor, 8> observedOutliers = {
    ofFloatColor(0.88f, 0.98f, 0.58f, 1.0f),
    ofFloatColor(0.95f, 0.32f, 0.68f, 1.0f),
    ofFloatColor(0.28f, 0.98f, 0.78f, 1.0f),
    ofFloatColor(0.92f, 0.46f, 0.20f, 1.0f),
    ofFloatColor(0.54f, 0.68f, 1.0f, 1.0f),
    ofFloatColor(0.78f, 0.92f, 1.0f, 1.0f),
    ofFloatColor(0.66f, 0.50f, 0.34f, 1.0f),
    ofFloatColor(0.36f, 0.42f, 0.46f, 1.0f)
};
```

Replace color variation with subtle albedo variation:

```cpp
const float albedoJitter = randRange(rng, 0.86f, 1.08f);
result.r = ofClamp(result.r * albedoJitter, 0.04f, 0.88f);
result.g = ofClamp(result.g * albedoJitter, 0.04f, 0.88f);
result.b = ofClamp(result.b * albedoJitter, 0.04f, 0.88f);
```

Then pull saturated colors back toward neutral gray:

```cpp
const ofFloatColor neutral(0.52f, 0.52f, 0.50f, 1.0f);
result = result.getLerped(neutral, 0.18f);
```

Acceptance criteria:

- No planet should read as neon, candy, or arcade.
- Hot rocky planets can have ember-like accents, but not glowing pink or saturated orange bodies.
- Saturated color should be reserved mostly for the star, corona, and subtle plasma effects.

## Step 9: Make trails subtle and observational

In `drawBodyTrail()`, reduce trail color intensity and stop strongly tinting trails by planet color.

### Replace

```cpp
const ofFloatColor trailColor = colorFrom(paramTrailR_, paramTrailG_, paramTrailB_, 1.0f)
                                    .getLerped(body.color, 0.14f + body.atmosphere * 0.10f);
```

### With

```cpp
const ofFloatColor baseTrail(0.72f, 0.78f, 0.86f, 1.0f);
const ofFloatColor trailColor = baseTrail.getLerped(body.color, 0.04f);
```

Also reduce the audio influence in alpha.

### Replace

```cpp
alpha * paramTrailAlpha_ * fade * (0.72f + level_ * paramAudioAmount_ * 0.55f)
```

### With

```cpp
alpha * paramTrailAlpha_ * fade * (0.82f + level_ * paramAudioAmount_ * 0.10f)
```

Acceptance criteria:

- Trails read as faint orbital persistence or long exposure traces.
- Trails do not dominate the planets.
- Trails do not look like neon UI strokes.

## Step 10: Reduce or constrain generated moons, rings, asteroids, and comets

Random moons and rings currently add spectacle. For scientific mode, they should be quieter and less frequent.

Suggested changes:

- Reduce default `paramMoonSize_` from `0.8f` to `0.45f`.
- Reduce default `paramMoonSpeed_` from `1.45f` to `0.35f`.
- Make rings less saturated and less opaque.
- Reduce comet density by default, or disable comets by default if they feel theatrical.
- Keep asteroids subtle and mostly non-glowing.

Recommended defaults:

```cpp
float paramMoonSize_ = 0.45f;
float paramMoonSpeed_ = 0.35f;
float paramAsteroidDensity_ = 0.65f;
float paramCometDensity_ = 0.25f;
```

Optional:

```cpp
bool paramShowComets_ = false;
```

Acceptance criteria:

- Moons do not zip around planets.
- Rings look dusty and translucent.
- Comets are rare accents, not a constant fantasy element.
- Asteroids add scale and texture without becoming confetti.

## Audio modulation model

Use this model for all audio reactivity in the scientific mode.

| Audio feature | Should modulate | Should not modulate |
|---|---|---|
| Bass | Solar corona bloom, star radius, field brightness | Orbit radius, planet position |
| Low mids | Solar wind density, heliosphere line thickness | Orbital period |
| Mids | Field curl, atmospheric band contrast | Global orbit speed except very subtly |
| Highs | Coronal sparks, starfield scintillation, tiny glints | Planet scale, orbit shape |
| Waveform | Heliosphere pressure ripple | Planet orbit path |
| Beat envelope | Solar pulse or corona brightness | Semi-major axis, eccentricity |

## Recommended update loop change

Current `update()` changes orbit speed with mids and pulse:

```cpp
const float audioLift = hasAudio_ ? paramAudioAmount_ : 0.0f;
const float speedLift = 1.0f + mids_ * paramMidsSpeed_ * audioLift + pulseEnvelope_ * 0.18f * audioLift;
orbitTime_ += dt * paramOrbitSpeed_ * speedLift;
```

For scientific mode, reduce this substantially.

Recommended:

```cpp
const float audioLift = hasAudio_ ? paramAudioAmount_ : 0.0f;
const float speedLift = 1.0f + mids_ * paramMidsSpeed_ * audioLift * 0.04f;
orbitTime_ += dt * paramOrbitSpeed_ * speedLift;
```

Or remove speed lift entirely:

```cpp
orbitTime_ += dt * paramOrbitSpeed_;
```

Preferred default: keep a tiny 4 percent maximum feel-based lift if the performance needs it, but do not let the audience see orbit speed obviously pump with music.

## Suggested implementation order

1. Change parameter defaults.
2. Disable cross-host planet mixing by default.
3. Replace period speed logic.
4. Remove audio radius warping from body orbit points.
5. Replace body ellipse math with Kepler-style focus-offset ellipse.
6. Stop bass from scaling `radiusScale`.
7. Add sun-relative lighting for planets.
8. Rework `drawWaveformBelt()` into `drawHeliosphereField()`.
9. Desaturate `planetColorFor()`.
10. Reduce trails, moons, rings, asteroids, and comets.
11. Tune camera and scale after the dynamics feel right.

## Visual acceptance checklist

The revised layer should satisfy the following:

- The system reads as an orrery or scientific visualization, not a toy mobile.
- The sun is visually dominant and clearly radiates light.
- Planets are visibly lit by the sun.
- Orbits are stable, elliptical, and not warped by audio.
- Inner planets move faster than outer planets.
- Motion is slow enough to feel astronomical.
- Audio reactivity is present but lives in corona, solar wind, atmospheric shimmer, and starfield scintillation.
- The color palette is muted and observational.
- Trails and orbit guides are faint enough to support structure without feeling like neon UI.
- The low-poly style remains visible, but the dynamics feel physically grounded.

## Testing checklist

Test the following cases:

### No audio input

Expected:

- Layer still looks complete.
- Sun radiance remains active but calm.
- No waveform field jitter appears unless simulated fallback is desired.
- Planets move slowly and predictably.

### Strong bass input

Expected:

- Sun blooms.
- Corona brightens.
- Heliosphere field becomes subtly more visible.
- Planet orbit spacing does not change.

### Strong mids input

Expected:

- Field lines curl or breathe subtly.
- Orbit speed does not visibly pump.

### Strong highs input

Expected:

- Coronal sparks or starfield scintillation increase.
- Planet sizes and orbits remain stable.

### `paramEccentricity_ = 0`

Expected:

- All body orbits become circular.

### `paramEccentricity_ = 1`

Expected:

- Observed eccentricities are visible but not exaggerated beyond the data.

### `paramObservedDiversity_ = 0`

Expected:

- Only one observed host system is used.

### Optional hybrid mode enabled

Expected:

- Cross-host mixing is allowed only when an explicitly named non-scientific mode is enabled.

## Notes for future improvements

These are not required for the first pass, but they would make the layer stronger later:

1. Add labels for host system and planet names as an optional overlay.
2. Add a true anomaly or eccentric anomaly debug mode.
3. Add a small scientific HUD showing selected host, stellar temperature, and planet count.
4. Add actual solar-system mode using Mercury through Neptune as a separate data table.
5. Add star color and luminosity scaling from stellar temperature and radius.
6. Add optional habitable-zone band based on stellar luminosity.
7. Add a debug toggle that draws the ellipse focus and periapsis direction.

## Summary

The core change is conceptual: planets should behave like observed bodies in a gravitational system, while audio should modulate energy emitted by the star and its surrounding field. Keep the low-poly visual language, but make the physics legible.

The highest-impact changes are:

1. Stop mixing unrelated planets by default.
2. Use observed period directly for speed.
3. Remove waveform orbit warping.
4. Use proper ellipse math with the star at one focus.
5. Light planets from the star.
6. Replace waveform belt with heliosphere field lines.
7. Desaturate planets and reduce neon trails.

Once those are implemented, the layer should read much less kitschy while remaining performative and audio-reactive.

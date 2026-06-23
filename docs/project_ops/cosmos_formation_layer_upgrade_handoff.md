# CosmosFormationLayer Upgrade Handoff

Status: first implementation pass applied on 2026-06-20. `GrowthCell`/`GrowthSample` now use dark/gas/stellar/temperature/compression channels, peaks and beats trigger local ignition, star formation is persistent, star cores render opaque, and draw scale uses a simple cosmological scale factor. The next large step is persistent `HaloState` identity if the current field-first look still feels too cluster-seeded.

Control note: `haloAlpha` currently controls both the diffuse growth-field gas and the density halo atmosphere. Split this into a separate gas/field alpha only if live tuning needs independent fades.

## Goal

Improve `CosmosFormationLayer` from a beautiful cluster-driven particle layer into a more scientifically inspired, persistent galaxy-formation performance piece.

Do **not** rewrite the layer from scratch. The current code already has the right foundation:

- `MatterNode` particles
- `Cluster` attractor-like structures
- `GrowthCell` field state with dark density, gas density, stellar density, temperature, spin, bloom, and compression
- lifecycle controls through `formationAge`, `formationTime`, `gravityDelay`, and `expansionRate`
- audio-driven local ignition, pressure waves, turbulence, sparkle, waveform warp, and beat/peak impulses

The main change is this:

```text
Current feel:
seeded clusters pull particles into pre-arranged galaxy-like structures

Target feel:
density fields evolve, halos emerge, gas cools, stars persist, and clusters gain identity over time
```

---

## Existing code anchors

Primary files:

```text
CosmosFormationLayer.h
CosmosFormationLayer.cpp
```

Current important structs:

```cpp
struct MatterNode {
    glm::vec2 pos;
    glm::vec2 prev;
    glm::vec2 vel;
    std::size_t cluster;
    float angle;
    float orbitRadius;
    float speed;
    float size;
    float brightness;
    float heat;
    float seed;
    float spin;
    float birthDelay;
    float mass;
};

struct Cluster {
    glm::vec2 basePos;
    float radius;
    float strength;
    float spin;
    float seed;
    float heat;
};

struct GrowthCell {
    float darkDensity;
    float gasDensity;
    float stellarDensity;
    float temperature;
    float spin;
    float bloom;
    float compression;
};
```

Current key functions:

```cpp
resetMatter();
triggerBang(float strength, bool collapseMatter);
resetGrowthField();
updateGrowthField(float dt, float timeSeconds, float transportSpeed);
updateMatter(float dt, float timeSeconds, float transportSpeed);
sampleGrowthField(const glm::vec2& point) const;
drawGrowthField(...);
drawDensityHalos(...);
drawMatter(...);
```

---

## Priority 1: Expand `GrowthCell` into physical channels

Status: first pass implemented.

### Problem

`GrowthCell` currently combines everything into generic `density`, `heat`, `spin`, and `bloom`.

That works, but it limits the ability to show:

- dark structure
- glowing gas
- cooling
- irreversible star formation
- old stellar regions versus new starbirth

### Change

Replace or extend `GrowthCell`:

```cpp
struct GrowthCell {
    float darkDensity = 0.0f;      // invisible gravitational structure
    float gasDensity = 0.0f;       // luminous diffuse material
    float stellarDensity = 0.0f;   // persistent formed stars
    float temperature = 0.0f;      // color and cooling state
    float spin = 0.0f;             // local angular tendency
    float bloom = 0.0f;            // recent starbirth / ignition
    float compression = 0.0f;      // local collapse indicator
};
```

Update `GrowthSample` to match:

```cpp
struct GrowthSample {
    float darkDensity = 0.0f;
    float gasDensity = 0.0f;
    float stellarDensity = 0.0f;
    float temperature = 0.0f;
    float spin = 0.0f;
    float bloom = 0.0f;
    float compression = 0.0f;
    glm::vec2 flow{ 0.0f, 0.0f };
};
```

### Implementation notes

In `resetGrowthField()`:

- initialize a hot gas core
- initialize a lower-amplitude dark-density fluctuation field
- initialize `stellarDensity = 0`

Example:

```cpp
cell.darkDensity = seedNoise * 0.18f + bell(r, 0.0f, 0.11f) * 0.35f;
cell.gasDensity = bell(r, 0.0f, 0.065f) * (0.45f + seedNoise * 0.35f);
cell.stellarDensity = 0.0f;
cell.temperature = cell.gasDensity;
cell.spin = p.x * p.y >= 0.0f ? 1.0f : -1.0f;
cell.bloom = cell.gasDensity * 0.8f;
cell.compression = 0.0f;
```

In `updateGrowthField()`:

- use `darkDensity` to shape collapse
- use `gasDensity` for luminous gas
- cool `temperature` as lifecycle advances
- convert dense, cool gas into `stellarDensity`

---

## Priority 2: Add irreversible star formation

Status: first pass implemented.

### Problem

The current layer has heat and bloom, but it does not clearly accumulate permanent stellar structure. This weakens the sense of cosmic history.

### Change

Inside `updateGrowthField()`, after computing updated gas and temperature, add star formation:

```cpp
const float dense = smoothStep(0.28f, 0.85f, next.gasDensity);
const float cool = 1.0f - smoothStep(0.35f, 0.85f, next.temperature);
const float collapsing = smoothStep(0.02f, 0.18f, next.compression);
const float starForm = dense * cool * collapsing * dt * (0.08f + highs_ * audioDrive * 0.06f);

const float converted = std::min(next.gasDensity, starForm);
next.gasDensity -= converted;
next.stellarDensity = ofClamp(next.stellarDensity + converted * 1.8f, 0.0f, 1.0f);
next.bloom = std::max(next.bloom, converted * 8.0f);
```

### Acceptance criteria

- Mature regions stay visible even after gas cools.
- Highs increase sparkle/starbirth, but only where gas is already eligible.
- Peaks do not create stars everywhere.
- The scene gets richer over time instead of cycling visually.

---

## Priority 3: Add persistent halo identity

### Problem

`Cluster` is currently created up front in `resetMatter()`. Matter nodes are assigned to clusters at birth. This can make the final structure feel pre-arranged.

### Change

Keep `Cluster` for now, but add a second structure called `HaloState`:

```cpp
struct HaloState {
    int id = 0;
    glm::vec2 center{ 0.0f, 0.0f };
    glm::vec2 velocity{ 0.0f, 0.0f };
    float mass = 0.0f;
    float gasMass = 0.0f;
    float stellarMass = 0.0f;
    float radius = 0.12f;
    float spin = 0.0f;
    float age = 0.0f;
    float temperature = 0.0f;
    float starFormationRate = 0.0f;
    float lastMergerTime = -1000.0f;
};
```

Add members:

```cpp
std::vector<HaloState> halos_;
int nextHaloId_ = 1;
float haloDetectAccumulator_ = 0.0f;
```

Add functions:

```cpp
void updateHalos(float dt, float timeSeconds);
void detectHaloCandidates(std::vector<HaloState>& candidates) const;
void matchHalos(const std::vector<HaloState>& candidates, float dt);
```

Call from `update()` after `updateGrowthField()` and before `updateMatter()`:

```cpp
updateGrowthField(dt, params.time, std::max(0.25f, transportSpeed));
updateHalos(dt, params.time);
updateMatter(dt, params.time, std::max(0.25f, transportSpeed));
updatePressureWaves(dt);
```

### First implementation

Do not overbuild. Detect halos from dense field regions:

```text
for each growth cell:
    mass = darkDensity + gasDensity * 0.6 + stellarDensity * 1.4
    if mass is local maximum above threshold:
        create candidate halo
```

Match candidates to existing halos by nearest center within a radius. If no match, create a new halo ID. If multiple candidates match one existing halo, keep strongest and treat the others as merger activity later.

### Acceptance criteria

- Halos persist for many seconds.
- A halo keeps the same identity as it drifts.
- Spin is derived from local particle/field motion, not only random seed.
- Future rendering can draw halo age, gas mass, and stellar mass.

---

## Priority 4: Use halos to guide motion, not only seeded clusters

### Problem

`updateMatter()` currently builds targets from `clusters_`:

```cpp
for (std::size_t clusterIndex = 0; clusterIndex < clusters_.size(); ++clusterIndex) {
    const Cluster& cluster = clusters_[clusterIndex];
    const glm::vec2 cpos = clusterPositions[clusterIndex];
    ...
}
```

This is the biggest source of the “pre-arranged cluster” feeling.

### Change

Phase this in gradually:

1. Keep `clusters_` for early bootstrapping and fallback.
2. Add `halos_` as an additional influence.
3. As halo detection stabilizes, reduce direct cluster pull and increase halo pull.

Example inside `updateMatter()`:

```cpp
for (const HaloState& halo : halos_) {
    const glm::vec2 delta = halo.center - node.pos;
    const float dist2 = glm::dot(delta, delta);
    const float basin = std::max(0.08f, halo.radius * paramClusterSoftness_);
    const float weight = halo.mass * std::exp(-dist2 / std::max(0.012f, basin * basin));

    if (weight <= 0.0001f) {
        continue;
    }

    const glm::vec2 targetDir = safeNormalize(delta, radial);
    const glm::vec2 tangent(-targetDir.y, targetDir.x);

    force += targetDir * paramGravity_ * gravityPhase * weight * 0.035f;
    force += tangent * halo.spin * paramClusterSwirl_ * shearPhase * weight * 0.025f;
}
```

Then reduce the old cluster target influence:

```cpp
const float seededClusterInfluence = ofLerp(1.0f, 0.35f, smoothStep(0.30f, 1.0f, lifecycle));
```

Apply that multiplier to the current `clusters_` force contribution.

### Acceptance criteria

- Early stage can still bloom attractively.
- Mature structure is increasingly guided by detected density/halo state.
- The layer feels less like particles are moving toward known attractor points.

---

## Priority 5: Add a simple cosmological scale factor

Status: first pass implemented.

### Problem

Matter currently launches outward from a tiny origin:

```cpp
node.pos = originDir * originRadius + ...
node.vel = launchDir * paramExpansionForce_ * node.speed * ...
```

This reads like an explosion, not expansion of space.

### Change

Add internal state:

```cpp
float scaleFactor_ = 1.0f;
float cosmicTemperature_ = 1.0f;
```

Update during `update()`:

```cpp
const float lifecycle = ofClamp(age_ / std::max(0.01f, paramFormationTime_), 0.0f, 1.0f);
scaleFactor_ = ofLerp(0.18f, 1.0f, smoothStep(0.0f, 1.0f, lifecycle));
cosmicTemperature_ = ofLerp(1.0f, 0.16f, smoothStep(0.0f, 1.0f, lifecycle));
```

Use `cosmicTemperature_` in field cooling:

```cpp
next.temperature = ofLerp(next.temperature, cosmicTemperature_ * targetHeat, dt * (0.12f + paramCoolingRate_ * 0.20f));
```

Use `scaleFactor_` in drawing, not necessarily in simulation at first:

```cpp
const float cosmicScale = ofLerp(0.35f, 1.0f, scaleFactor_);
const glm::vec2 screenPos = center + node.pos * scale * cosmicScale;
```

### Acceptance criteria

- Early formation feels compressed and hot.
- Mature formation feels expanded and cool.
- The layer can still be played with `formationAge`.

---

## Priority 6: Split audio events into Ignite, Bang, and New Universe

Status: first pass implemented.

### Problem

`triggerBang()` currently resets age, clears waves, resets the growth field, and may collapse matter. This is useful manually, but too destructive for musical peaks.

### Change

Add three concepts:

```cpp
void triggerIgnition(float strength, const glm::vec2& origin);
void triggerBang(float strength, bool collapseMatter);
void resetMatter(); // New Universe / reseed only
```

`triggerIgnition()` should:

- add a pressure wave
- add bloom/temperature to nearby `GrowthCell`s
- avoid resetting `age_`
- avoid clearing `growthField_`
- avoid collapsing matter

Example:

```cpp
void CosmosFormationLayer::triggerIgnition(float strength, const glm::vec2& origin) {
    triggerPressureWave(strength, origin);

    for (int y = 0; y < kGrowthRows; ++y) {
        for (int x = 0; x < kGrowthCols; ++x) {
            GrowthCell& cell = growthField_[y * kGrowthCols + x];
            const glm::vec2 p = growthFieldPoint(x, y);
            const float r = glm::length(p - origin);
            const float influence = bell(r, 0.0f, 0.12f) * strength;
            cell.bloom = std::max(cell.bloom, influence);
            cell.temperature = ofClamp(cell.temperature + influence * 0.35f, 0.0f, 1.0f);
        }
    }
}
```

Change `updateBeatState()` so peaks call `triggerIgnition()` instead of only pressure/bang energy:

```cpp
if (hasAudio_ && peak_ >= paramPeakBangThreshold_ && timeSeconds - lastPeakTime_ >= 0.12f) {
    float impulse = peak_ * paramPeakImpulse_;
    glm::vec2 origin = chooseIgnitionOrigin(timeSeconds, lifecycle, peak_);
    triggerIgnition(impulse, origin);
    lastPeakTime_ = timeSeconds;
}
```

### Acceptance criteria

- Peaks create local starbirth/pressure events.
- Peaks never restart the universe.
- Manual `bang` still gives the performer a dramatic reset.
- `reseed` remains the only full new-universe action.

---

## Priority 7: Make rendering emphasize gas and field before particles

Status: first pass implemented; tune after live visual review.

### Problem

The draw stack is good, but particles can still dominate visually.

Current draw order:

```cpp
drawBackground(...);
drawGrowthField(...);
drawPressureWaves(...);
drawDensityHalos(...);
drawCore(...);
drawTrails(...);
drawMatter(...);
```

Keep this order, but rebalance visual weight.

### Changes

In `drawGrowthField()`:

- use `gasDensity` for diffuse gas
- use `temperature` for hot/cool color
- use `stellarDensity` for mature dense knots
- use `bloom` for recent starbirth

Suggested visibility expression:

```cpp
const float gasVisible = cell.gasDensity * 0.62f;
const float starsVisible = cell.stellarDensity * 0.85f;
const float birthVisible = cell.bloom * 0.55f;
const float visible = ofClamp(gasVisible + starsVisible + birthVisible, 0.0f, 1.0f);
```

In `drawMatter()`:

- keep star cores opaque
- reduce count dominance by making many nodes smaller/fainter
- let field/halo glow carry the scale

Suggested star alpha logic:

```cpp
const float coreAlpha = alpha * ofClamp(0.55f + node.brightness * 0.45f, 0.0f, 1.0f);
```

Avoid making mature star cores transparent. Use size, glow, and color for variation.

### Acceptance criteria

- From across the room, the viewer sees gas, halos, and clusters first.
- Individual particles read as stars embedded in structure, not sprites in empty space.
- Mature scenes look quieter but still alive.

---

## Priority 8: Add only low-risk libraries for now

Do not add particle-mesh gravity, HDF5, OpenVDB, or REBOUND yet.

Recommended optional library only:

```text
FastNoiseLite
```

Use it for:

- gas clumping
- sub-grid texture
- bloom variation
- turbulence detail

Do not use it for:

- main cluster placement
- global swirl
- fake gravity
- replacing the growth field

---

## Suggested implementation order

1. Done: extend `GrowthCell` and `GrowthSample`.
2. Done: update `resetGrowthField()`, `sampleGrowthField()`, and `drawGrowthField()` for gas/star/temperature channels.
3. Done: add irreversible star formation in `updateGrowthField()`.
4. Done: add `scaleFactor_` and `cosmicTemperature_`.
5. Done: add `triggerIgnition()` and route peaks/beats to it.
6. Done: tune rendering so gas/field contribute more and star cores stay opaque.
7. Next if needed: add `HaloState` and simple halo detection.
8. Next if needed: add halo influence to `updateMatter()` while reducing old `clusters_` influence over lifecycle.
9. Only then consider FastNoiseLite for texture.

---

## Final success criteria

The revised layer should satisfy these:

- Galaxies feel like they emerge from density fields, not predefined targets.
- Gas expands, cools, and condenses.
- Stars form irreversibly and persist.
- Halos have identity and survive over time.
- Swirl emerges near collapsed halos, not globally everywhere.
- Audio creates bounded local events.
- Peaks do not reset the universe.
- The visual reads as massive scale, not particles in a fish tank.
- The existing Synaptome parameter and scene workflow remains intact.

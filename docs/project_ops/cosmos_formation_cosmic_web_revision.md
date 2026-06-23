# CosmosFormationLayer Cosmic Web Revision Handoff

Status: single-mode cosmic-web implementation applied on 2026-06-21. We intentionally did not add `structureMode`; the current artistic direction replaces the orbital/super-galaxy behavior with one DESI-style universe-map look. The seed density has been raised beyond the old 16-node ceiling: the default now uses 64 web anchor seeds plus generated satellite nodes for a much richer topology. Radius mapping now scales independently across the viewport so the web can spread across wide screens instead of being limited by the shorter screen dimension. The current engine pass promotes luminosity and audio excitation into the growth field itself, so dense nodes glow because the simulation state says they are energetic rather than because a separate overlay was added.

Latest pass: latent gravitational formation is now part of the same engine. Web nodes begin as low-strength seeds instead of immediate attractors, then gain `activation`, `audioCharge`, and `effectiveMass` as surrounding matter density, field compression, and audio accumulate over time. Filament edges also carry `activation`, `audioCharge`, `conductivity`, and `massFlow`, so stable axon-like structures can slowly emerge, brighten, and start steering matter instead of appearing as a representative overlay. Audio is additive rather than purely momentary: bass pushes long-horizon node growth, mids/highs excite ridge conductivity and shimmer, and beat pulses nudge accretion without resetting the system.

New tuning controls:

- `Gravity Emergence`: how quickly latent web seeds become real gravitational wells.
- `Filament Memory`: how long activated ridges stay coherent after matter and audio pass through them.
- `Audio Accretion`: how much audio contributes to long-term node and filament formation.

Default motion has been slowed again: `Evolution Speed`, `Gravity`, `Cluster Swirl`, and `Cluster Drift` are intentionally conservative, while `Glow Persistence`, `Field Luminosity`, and `Filament Memory` are higher so the image reads as a universe condensing from darkness rather than a swarm chasing fixed nuclei.

Organic-field revision: the web now uses full-screen blue-noise-style seed placement instead of macro clumps squeezed into the center of the frame. Filament paths are measured and drawn as curved organic paths with deterministic wobble, and the growth field receives an additional warped Voronoi-style ridge pass so axon structure can arise between neighboring web sites instead of only along straight nearest-neighbor chords. Matter spawning now keeps a diffuse screen-wide star dust population alive while the field slowly condenses it into brighter nuclei and ridges. `Audio: Glow` and `Audio: Twinkle` expose direct audio mappings for luminosity, halo lift, star shimmer, and glow radius.

## Goal

The current `CosmosFormationLayer` still reads as an orbital cluster system: matter settles into a distinct ring, stars sweep through the interior, and motion appears driven by shifting gravitational points.

The new target is a **cosmic web / universe-map look** inspired by DESI-style large-scale structure references:

```text
dense nodes
+ branching filaments
+ dark voids
+ hierarchical clustering
+ subtle local swirl near nodes only
```

The layer should no longer prioritize a clean annulus, galaxy disks, or particles orbiting obvious centers. It should read like a massive universe map with neuron-like structure.

## Current diagnosis

The visual result is still dominated by these existing assumptions:

1. `resetMatter()` places clusters on a ring.
2. `MatterNode` instances are born with cluster assignments.
3. `updateMatter()` builds orbit-lane targets around clusters.
4. `updateGrowthField()` still contains broad disk and spiral-band bias.
5. Rendering allows moving star particles to dominate over the field/web structure.

The implementation goal is to shift the layer from:

```text
clusters + orbit lanes + swirl
```

to:

```text
nodes + filaments + voids
```

## Existing code anchors

Primary files:

```text
CosmosFormationLayer.h
CosmosFormationLayer.cpp
```

Important current structs:

```cpp
struct MatterNode;
struct Cluster;
struct GrowthCell;
struct GrowthSample;
struct PressureWave;
```

Important current functions:

```cpp
resetMatter();
resetGrowthField();
updateGrowthField(float dt, float timeSeconds, float transportSpeed);
updateMatter(float dt, float timeSeconds, float transportSpeed);
sampleGrowthField(const glm::vec2& point) const;
drawGrowthField(...);
drawDensityHalos(...);
drawMatter(...);
```

## Priority 1: Remove the annulus cluster seeding

### Problem

`resetMatter()` currently places `Cluster` positions around a ring:

```cpp
const float ordinal = clusterState_ <= 1 ? 0.0f : (static_cast<float>(i) + 0.5f) / static_cast<float>(clusterState_);
const float angle = static_cast<float>(i) * 2.3999632f + centered(rng) * 0.18f;
const float ring = clusterState_ <= 1 ? 0.0f : ofLerp(0.22f, paramClusterSpread_, std::sqrt(ordinal));

Cluster cluster;
cluster.basePos = glm::vec2(std::cos(angle), std::sin(angle)) * ring;
cluster.basePos.y *= 0.76f;
```

This directly causes the visual to settle into one obvious ring.

### Change

Replace ring placement with hierarchical distributed peak placement.

Use a small number of macro-regions, then scatter clusters around them. This creates uneven large-scale structure and voids.

```cpp
std::vector<glm::vec2> macroCenters;
const int macroCount = std::max(1, std::min(9, std::max(2, clusterState_ / 8)));

for (int i = 0; i < macroCount; ++i) {
    glm::vec2 p(centered(rng), centered(rng));
    p *= ofLerp(0.25f, 0.72f, unit(rng));
    p.y *= 0.78f;
    macroCenters.push_back(p);
}

clusters_.clear();
clusters_.reserve(static_cast<std::size_t>(clusterState_));

for (int i = 0; i < clusterState_; ++i) {
    const glm::vec2 macro = macroCenters[static_cast<std::size_t>(i % macroCenters.size())];

    glm::vec2 jitter(centered(rng) * 0.20f, centered(rng) * 0.15f);
    if (unit(rng) < 0.25f) {
        jitter *= 2.0f; // occasional outlier node
    }

    Cluster cluster;
    cluster.basePos = macro + jitter;
    cluster.radius = ofLerp(0.045f, 0.135f, unit(rng));
    cluster.strength = ofLerp(0.30f, 0.85f, unit(rng));
    cluster.spin = (unit(rng) < 0.5f ? -1.0f : 1.0f) * ofLerp(0.10f, 0.75f, unit(rng));
    cluster.seed = unit(rng) * 10000.0f;
    cluster.heat = ofLerp(0.12f, 0.42f, unit(rng));
    clusters_.push_back(cluster);
}
```

### Acceptance criteria

- No single ring is visible after settlement.
- The system contains uneven voids and concentrations.
- Cluster layout feels like a lumpy survey map, not an orbital diagram.
- Default seed density is high enough to read as a universe-map graph, not a handful of attractors.
- `Cosmos Radius` can spread the web across the full viewport, including wide projection outputs.
- Dense field regions emit persistent luminosity from compression, stellar density, and audio excitation.
- `Evolution Speed` slows web drift, field evolution, matter transport, and simulation-time visual noise together.

## Priority 2: Add explicit cosmic web state

### Problem

The current field has density, heat, spin, and bloom, but no direct concept of filaments, ridges, or voids.

### Change

Extend `GrowthCell` and `GrowthSample` with web-specific channels.

```cpp
struct GrowthCell {
    float darkDensity = 0.0f;
    float gasDensity = 0.0f;
    float stellarDensity = 0.0f;
    float temperature = 0.0f;
    float spin = 0.0f;
    float bloom = 0.0f;
    float compression = 0.0f;

    // New cosmic web channels
    float ridge = 0.0f;        // filament strength
    float node = 0.0f;         // dense intersection strength
    float voidness = 0.0f;     // low-density space strength
    glm::vec2 filamentDir{0.0f, 0.0f};
};
```

```cpp
struct GrowthSample {
    float darkDensity = 0.0f;
    float gasDensity = 0.0f;
    float stellarDensity = 0.0f;
    float temperature = 0.0f;
    float spin = 0.0f;
    float bloom = 0.0f;
    float compression = 0.0f;

    float ridge = 0.0f;
    float node = 0.0f;
    float voidness = 0.0f;
    glm::vec2 gradient{0.0f, 0.0f};
    glm::vec2 filamentDir{0.0f, 0.0f};
    glm::vec2 flow{0.0f, 0.0f};
};
```

### Acceptance criteria

- `sampleGrowthField()` can return field gradient and filament direction.
- `updateMatter()` can push matter along filaments instead of orbit lanes.
- Rendering can draw ridges and nodes directly.

## Priority 3: Build persistent web nodes and filament edges

### Problem

A universe-map look needs visible topology: nodes connected by branching filaments. Pure orbiting particles and generic noise will not produce that reliably.

### Change

Add persistent web structures.

In `CosmosFormationLayer.h`:

```cpp
struct WebNode {
    int id = 0;
    glm::vec2 pos{0.0f, 0.0f};
    glm::vec2 vel{0.0f, 0.0f};
    float mass = 0.0f;
    float radius = 0.10f;
    float heat = 0.0f;
    float age = 0.0f;
};

struct FilamentEdge {
    int a = -1;
    int b = -1;
    float strength = 1.0f;
    float age = 0.0f;
};

std::vector<WebNode> webNodes_;
std::vector<FilamentEdge> filamentEdges_;
int nextWebNodeId_ = 1;
float webRebuildAccumulator_ = 0.0f;
```

Add function declarations:

```cpp
void rebuildCosmicWeb(float timeSeconds);
void seedWebNodesFromClusters();
void buildFilamentEdges();
void paintCosmicWebIntoField(float dt);
float distanceToSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, float& t) const;
```

### First implementation

Do not require full Delaunay yet. Use nearest-neighbor plus minimum-spanning-tree-like behavior.

For each node:

1. Find its 2 or 3 nearest other nodes.
2. Add edges if they do not already exist.
3. Prefer short and medium edges.
4. Avoid connecting everything to everything.

```cpp
void CosmosFormationLayer::buildFilamentEdges() {
    filamentEdges_.clear();

    for (std::size_t i = 0; i < webNodes_.size(); ++i) {
        std::vector<std::pair<float, int>> nearest;

        for (std::size_t j = 0; j < webNodes_.size(); ++j) {
            if (i == j) continue;

            const float d2 = glm::distance2(webNodes_[i].pos, webNodes_[j].pos);
            nearest.push_back({d2, static_cast<int>(j)});
        }

        std::sort(nearest.begin(), nearest.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.first < rhs.first;
                  });

        const int connectCount = std::min(3, static_cast<int>(nearest.size()));
        for (int n = 0; n < connectCount; ++n) {
            const int a = static_cast<int>(i);
            const int b = nearest[n].second;
            if (a > b) continue;

            FilamentEdge edge;
            edge.a = a;
            edge.b = b;
            edge.strength = ofClamp(1.0f / std::sqrt(std::max(0.001f, nearest[n].first)), 0.15f, 1.0f);
            filamentEdges_.push_back(edge);
        }
    }
}
```

### Acceptance criteria

- The web graph is sparse.
- It creates branching structures.
- It avoids a complete mesh.
- The graph persists long enough to be visually legible.

## Priority 4: Paint filaments into the growth field

### Problem

Even if nodes exist, the field will not look like the references unless filaments are visible and affect motion.

### Change

In `updateGrowthField()`, after base field update, call:

```cpp
paintCosmicWebIntoField(dt);
```

Implementation sketch:

```cpp
void CosmosFormationLayer::paintCosmicWebIntoField(float dt) {
    if (growthField_.empty()) return;

    for (const FilamentEdge& edge : filamentEdges_) {
        if (edge.a < 0 || edge.b < 0) continue;
        if (edge.a >= static_cast<int>(webNodes_.size())) continue;
        if (edge.b >= static_cast<int>(webNodes_.size())) continue;

        const glm::vec2 a = webNodes_[edge.a].pos;
        const glm::vec2 b = webNodes_[edge.b].pos;
        const glm::vec2 ab = b - a;
        const glm::vec2 dir = safeNormalize(ab);

        for (int y = 0; y < kGrowthRows; ++y) {
            for (int x = 0; x < kGrowthCols; ++x) {
                GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
                const glm::vec2 p = growthFieldPoint(x, y);

                float t = 0.0f;
                const float d = distanceToSegment(p, a, b, t);

                // Thin ridges with soft skirts.
                const float core = std::exp(-(d * d) / 0.0018f);
                const float skirt = std::exp(-(d * d) / 0.012f) * 0.28f;
                const float alongFade = smoothStep(0.0f, 0.12f, t) * (1.0f - smoothStep(0.88f, 1.0f, t));
                const float filament = (core + skirt) * alongFade * edge.strength;

                cell.ridge = std::max(cell.ridge, filament);
                cell.darkDensity = ofClamp(cell.darkDensity + filament * 0.08f, 0.0f, 1.5f);
                cell.gasDensity = ofClamp(cell.gasDensity + filament * 0.035f, 0.0f, 1.5f);

                cell.filamentDir = safeNormalize(cell.filamentDir + dir * filament, dir);
            }
        }
    }

    for (const WebNode& node : webNodes_) {
        for (int y = 0; y < kGrowthRows; ++y) {
            for (int x = 0; x < kGrowthCols; ++x) {
                GrowthCell& cell = growthField_[static_cast<std::size_t>(y * kGrowthCols + x)];
                const glm::vec2 p = growthFieldPoint(x, y);
                const float r = glm::length(p - node.pos);

                const float nodeInfluence = std::exp(-(r * r) / std::max(0.002f, node.radius * node.radius));
                cell.node = std::max(cell.node, nodeInfluence);
                cell.darkDensity = ofClamp(cell.darkDensity + nodeInfluence * 0.12f, 0.0f, 1.5f);
                cell.gasDensity = ofClamp(cell.gasDensity + nodeInfluence * 0.06f, 0.0f, 1.5f);
                cell.temperature = std::max(cell.temperature, node.heat * nodeInfluence);
            }
        }
    }
}
```

### Acceptance criteria

- The field itself contains visible filaments.
- Filaments connect dense nodes.
- Voids remain dark.
- Particles appear embedded in the web, not orbiting outside it.

## Priority 5: Rewrite matter motion to follow web ridges

### Problem

Current `updateMatter()` is dominated by cluster targets and tangential orbital motion.

Problematic pattern:

```cpp
weightedTarget += (cpos + laneDir * laneRadius) * weight;
weightedTangent += safeNormalize(glm::vec2(-delta.y, delta.x), ...) * cluster.spin * weight;
```

This creates orbit lanes and interior sweep behavior.

### Change

Make field/web motion primary. Cluster orbiting should become secondary or disabled for the cosmic-web preset.

Add these force components after `GrowthSample field = sampleGrowthField(node.pos);`:

```cpp
const glm::vec2 gradientDir = safeNormalize(field.gradient, radial);
const float collapseStrength = ofClamp(field.darkDensity + field.gasDensity + field.node, 0.0f, 1.5f);

force += gradientDir * paramGravity_ * gravityPhase * collapseStrength * 0.045f;
force += field.filamentDir * field.ridge * (0.05f + mids_ * audioDrive * 0.025f);
force += radial * paramVoidPressure_ * field.voidness * 0.035f;
```

Then localize swirl:

```cpp
const float localSwirl = field.node * smoothStep(0.35f, 0.90f, field.darkDensity + field.stellarDensity);
const glm::vec2 tangent(-gradientDir.y, gradientDir.x);

force += tangent * paramClusterSwirl_ * localSwirl * 0.035f;
```

Reduce old cluster lane influence:

```cpp
const float seededClusterInfluence = ofLerp(0.75f, 0.10f, smoothStep(0.20f, 0.90f, lifecycle));
```

Apply `seededClusterInfluence` to all old cluster-target and cluster-tangent force terms. If the web looks good, allow a preset/default that sets this near zero.

### Acceptance criteria

- Matter travels along ridges.
- Dense nodes accumulate stars.
- Swirl appears near nodes, not across the whole structure.
- There is no obvious interior orbital sweep.

## Priority 6: Remove broad disk and spiral-band dominance

### Problem

`updateGrowthField()` currently contains disk and spiral terms like:

```cpp
const float spiral = spiralBand(theta * 4.0f - std::log(r + 0.045f) * 5.4f - globalTurn, 3.3f);
const float broadDisk = std::exp(-(r * r) / 0.82f);
```

This biases the whole layer toward galaxy-disk structure.

### Change

For the cosmic-web mode, demote these to optional background texture only.

Replace dominant density contribution:

```cpp
broadDisk * (0.10f + spiral * 0.28f) * settling
```

with something much weaker:

```cpp
broadDisk * (0.025f + spiral * 0.035f) * settling
```

Let `paintCosmicWebIntoField()` produce the main large-scale structure.

### Acceptance criteria

- No dominant spiral or disk is visible in cosmic-web mode.
- The web graph determines the composition.
- Spiral motion is only a local node behavior or a separate preset.

## Priority 7: Rebalance default parameters

### Problem

Current defaults encourage visible orbital behavior:

```cpp
float paramClusterSwirl_ = 1.15f;
float paramClusterDrift_ = 0.82f;
float paramShear_ = 0.95f;
float paramExpansionForce_ = 0.72f;
```

### Change

For the cosmic-web preset/default, reduce these:

```cpp
float paramClusterSwirl_ = 0.20f;
float paramClusterDrift_ = 0.18f;
float paramShear_ = 0.15f;
float paramExpansionForce_ = 0.45f;
float paramTrailAlpha_ = 0.025f;
```

Compensate visually with stronger field rendering:

```cpp
float paramHaloAlpha_ = 0.32f;
float paramMatterGlow_ = 2.2f;
float paramPressureAmount_ = 0.18f;
```

### Acceptance criteria

- Motion feels slower and larger.
- Particles do not visibly whirl around attractors.
- Field, web, and density carry the image.

## Priority 8: Render cosmic web first, particles second

### Problem

The reference images read as large-scale density maps, not particle emitters.

### Change

In `drawGrowthField()`:

- Draw `ridge` as fine white/cyan/pink filament density.
- Draw `node` as luminous intersection glow.
- Draw `voidness` by preserving darkness, not by drawing black shapes.
- Draw particles as texture and star cores after the field.

Suggested intensity expression:

```cpp
const float filamentVisible = cell.ridge * 0.85f;
const float nodeVisible = cell.node * 0.95f;
const float gasVisible = cell.gasDensity * 0.35f;
const float starVisible = cell.stellarDensity * 0.60f;
const float birthVisible = cell.bloom * 0.45f;

const float visible = ofClamp(
    filamentVisible +
    nodeVisible +
    gasVisible +
    starVisible +
    birthVisible,
    0.0f,
    1.0f
);
```

In `drawMatter()`:

- Use opaque cores only for selected bright stars.
- Make most matter nodes tiny texture points.
- Reduce trails unless explicitly performing a high-energy mode.

Suggested matter alpha:

```cpp
const float structure = field.ridge * 0.6f + field.node * 0.9f + field.stellarDensity;
const float coreAlpha = alpha * ofClamp(0.18f + structure * 0.82f, 0.0f, 1.0f);
```

### Acceptance criteria

- Still frame reads like a cosmic web map.
- Motion adds life but is not required for the structure to be visible.
- Voids are legible.
- Filaments remain visible even when particles are sparse.

## Priority 9: Mode Flag Decision

Decision: skip the mode flag for now.

The previous super-galaxy look and the new cosmic-web look are different artistic modes, but preserving both would add complexity while the current goal is to make one strong layer.

### Implementation Choice

Make `CosmosFormationLayer` itself the cosmic-web layer:

- ring cluster seeding is replaced by hierarchical distributed seeding
- persistent web nodes and sparse filament edges define the composition
- broad disk/spiral density is demoted to faint background texture
- old orbit-lane matter motion is replaced with ridge/gradient following
- field/web rendering is primary and particles become supporting texture

### Acceptance criteria

- The layer has one coherent visual identity.
- UI and defaults load directly into the cosmic-web look.
- Future mode work is only reconsidered after the single look is approved.

## Suggested implementation order

1. Done: replace ring cluster seeding with hierarchical distributed seeding.
2. Done: add `WebNode` and `FilamentEdge`.
3. Done: build a sparse nearest-neighbor filament graph.
4. Done: extend `GrowthCell` and `GrowthSample` with `ridge`, `node`, `voidness`, `gradient`, and `filamentDir`.
5. Done: paint filaments and nodes into the growth field.
6. Done: update `sampleGrowthField()` to return gradient and filament direction.
7. Done: add ridge-following forces in `updateMatter()`.
8. Done: remove old orbit-lane cluster forces from matter motion.
9. Done: reduce global disk/spiral influence in `updateGrowthField()`.
10. Done: rebalance defaults for the cosmic-web look.
11. Done: rebalance rendering so field/web structure dominates particles.
12. Next: tune live against the actual projector/output feed.

## Fastest path to visible improvement

If time is limited, implement only these four changes first:

1. Remove ring cluster seeding.
2. Add web nodes and filament edges.
3. Paint filaments into `GrowthCell`.
4. Draw ridges/nodes strongly in `drawGrowthField()`.

This should immediately move the image toward the DESI / universe-map references.

## Final success criteria

The revised layer should satisfy:

- No distinct single ring.
- No obvious orbital sweep through the interior.
- Dense nodes appear at web intersections.
- Filaments connect nodes in branching structures.
- Voids remain dark and legible.
- Motion is slow and massive.
- Swirl exists only near dense nodes or in non-web mode.
- Audio adds local ignition and shimmer without destroying the web.
- A still frame reads as a cosmic web map.
- Particles support the field instead of defining the whole image.

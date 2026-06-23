# 3D Arctic Aurora Scene Roadmap

Status: active direction for showable arctic aurora scene work.
Created: 2026-06-21.
Updated: 2026-06-21 for centered polar-world scene revision.

## Goal

Build one cohesive, orbitable 3D arctic aurora scene that is public-showable today:

```text
dark arctic sky
+ audio-reactive glowing 3D aurora volume
+ centered circular sea disk at Y=0
+ real 3D floating icebergs intersecting the waterline
+ cylindrical/spherical aurora light floating above the water
```

The current separate-layer prototype reads as flat 2D scenic cutouts. The original integrated layer also read too much like a rectangular stage: a sea plane in one direction, horizon helper quads, and aurora sheets at the back. This roadmap pivots the show path toward a centered polar world that can be orbited with the shared camera.

## Key Decision

Create a new single layer:

```text
ArcticAuroraSceneLayer
```

Do not keep trying to make the public scene from:

```text
ConstellationStarfieldLayer
+ AuroraCurtainLayer
+ ArcticSeaIcebergLayer
```

Those can remain experimental/catalog layers for now, but the showable arctic aurora scene should be one layer with one depth buffer and one world-space composition.

## Camera Decision

Use the existing Synaptome camera passed through `LayerDrawParams::camera`.

Pattern to follow:

```cpp
params.camera.begin();
ofEnableDepthTest();
glDepthFunc(GL_LEQUAL);
glDepthMask(GL_TRUE);
glClear(GL_DEPTH_BUFFER_BIT);

// Draw 3D scene here.

ofDisableDepthTest();
params.camera.end();
```

This matches existing 3D layers such as `GridLayer`, `GeodesicLayer`, `StlModelLayer`, and `SolarSystemLayer`.

Do not create a private camera unless the shared app camera proves impossible for framing. The needed unit is a dedicated 3D scene layer, not a dedicated camera.

## Audio Scope

Audio modulates the aurora only.

Audio may affect:

```text
aurora brightness
aurora bloom/glow envelope
aurora fold depth
aurora vertical ray density
aurora shimmer
aurora reflection intensity on water
```

Audio must not affect:

```text
sea wave amplitude
iceberg drift
iceberg scale
iceberg bobbing
camera motion
star positions
```

The sea and icebergs should feel physically present and stable. They can have slow procedural idle motion, but not audio-driven motion.

## Files

Primary implementation files:

```text
synaptome/src/visuals/ArcticAuroraSceneLayer.h
synaptome/src/visuals/ArcticAuroraSceneLayer.cpp
synaptome/bin/data/layers/generative/arctic_aurora_scene.json
synaptome/bin/data/layers/scenes/arctic-aurora-3d.json
```

Integration files:

```text
synaptome/src/ofApp.h
synaptome/src/ofApp.cpp
synaptome/Synaptome.vcxproj
synaptome/Synaptome.vcxproj.filters
docs/contracts/parameter_manifest.json
tools/testdata/layer_catalog/expected_catalog.json
tools/testdata/console_layout/expected_console_contract.json
tools/testdata/scene_persistence/expected_scene_contract.json
```

Factory type:

```text
arcticAuroraScene
```

Catalog asset id:

```text
generative.arcticAuroraScene
```

Registry prefix:

```text
generative.arcticAuroraScene
```

## World Layout

Use a simple world-space coordinate system with the app camera:

```text
x = horizontal
y = vertical
z = depth
```

Current polar-world layout:

```text
water disk center: 0, 0, 0
water plane y:    0
water diameter:   1500

iceberg y:     waterline +/- small non-audio offset
iceberg x/z:   polar radius/angle positions inside the disk

aurora radius: 560 to 980
aurora y:      120 to 640
aurora angle:  curved arc around the disk

stars z:       -2200 or screen-space background
```

The aurora should float above the disk as curved world-space light, not sit on the screen as a backdrop.

## Render Order

The layer can draw a 2D sky gradient first, then draw 3D geometry using the existing camera.

Recommended order:

1. Screen-space sky gradient and distant stars.
2. `params.camera.begin()`.
3. Enable depth test and clear depth buffer.
4. Draw far aurora glow shells with depth writes off.
5. Draw main aurora translucent ribbon volumes with depth writes off.
6. Draw aurora vertical rays and bright folds additive.
7. Draw water plane opaque/translucent with depth writes on.
8. Draw aurora reflection meshes on/just above water with depth writes off.
9. Draw far-to-near iceberg meshes with depth test on.
10. Draw iceberg rim lights additive with depth writes off.
11. Disable depth test and end camera.

Important blend/depth rules:

```text
opaque sea/iceberg faces: alpha or normal blend, depth writes on
aurora translucent volume: additive/alpha blend, depth writes off
aurora glow shells: additive blend, depth writes off
water reflections: additive blend, depth writes off, very low alpha
iceberg rims: additive blend, depth writes off
```

## Data Structures

### Audio State

Use `AudioAnalysisBus::Snapshot`:

```cpp
bool hasAudio_;
float level_;
float peak_;
float bass_;
float mids_;
float highs_;
float auroraEnergy_;
float auroraPulse_;
std::vector<float> energyField_;
std::vector<float> targetEnergyField_;
```

Convert raw waveform to hidden energy:

```text
abs(waveform)
+ broad bass/mids/highs terms
+ slow multi-scale noise
-> 0..1 energy field
```

Never use signed waveform samples as visible vertex position.

### Aurora Curtain

```cpp
struct AuroraCurtain {
    float z = 0.0f;
    float xOffset = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    float phase = 0.0f;
    float colorMix = 0.0f;
};
```

Each curtain is a 3D ribbon surface:

```text
columns: 96 to 180
vertical segments: 8 to 18
mesh mode: triangles
```

The mesh should have:

```text
horizontal folds
vertical curtain falloff
depth ripple
soft alpha gradient
bright fold ridges
thin vertical rays
expanded glow shells
```

### Water Plane

```cpp
struct WaterGrid {
    int cols;
    int rows;
    ofMesh surface;
    ofMesh highlights;
};
```

The sea is a real tessellated plane:

```text
x spans left/right
z spans near/far
y has small procedural displacement
```

Water motion is slow procedural idle only. Audio does not change wave amplitude.

### Icebergs

```cpp
struct Iceberg {
    glm::vec3 pos;
    float scale;
    float yaw;
    float seed;
    ofMesh aboveWater;
    ofMesh belowWater;
    ofMesh rimLines;
};
```

Generate low-poly irregular meshes:

```text
top jagged cap
middle faceted body
small underwater mass below waterline
cyan rim edges
subtle magenta shadow side
```

First-pass geometry does not need physically perfect triangulation. It just needs actual 3D vertices/faces with clear front/side/top faces.

## Implementation Milestones Today

### Milestone 1: Minimal 3D Scene Layer

Status: complete in first implementation slice.

Implemented:

- Added `ArcticAuroraSceneLayer.h/.cpp`.
- Registered factory type `arcticAuroraScene`.
- Added Browser asset `generative.arcticAuroraScene`.
- Added scene preset `arctic-aurora-3d.json`.
- Uses the existing `LayerDrawParams::camera` path with depth testing and depth-buffer clear.
- Draws screen-space sky/stars, a world-space 3D water plane, depth-separated aurora guide meshes, and depth-tested placeholder iceberg meshes.
- Keeps audio modulation scoped to aurora energy/glow/folds/reflection highlights.

Target duration: 45-75 minutes.

Tasks:

- Add `ArcticAuroraSceneLayer.h/.cpp`.
- Register `arcticAuroraScene`.
- Add catalog JSON.
- Add simple scene preset.
- Draw sky, basic 3D water plane, and placeholder 3D iceberg boxes.

Acceptance:

- Layer loads from Browser.
- Scene preset loads.
- Existing camera controls affect the scene.
- Water reads as perspective geometry, not screen bands.

### Milestone 2: Real Sea Plane

Status: complete in second implementation slice.

Implemented:

- Increased the water mesh density for a smoother world-space sea plane.
- Reworked water coloring with near/far depth falloff and horizon color blending.
- Kept water displacement procedural and independent of audio.
- Added broken water highlight strokes instead of rigid screen/grid bands.
- Added additive aurora reflection receiver strips on the water surface.
- Added a world-space horizon mist band tied to the far edge of the sea plane.
- Added `waterReflection` and `waterHorizonFog` controls.

Target duration: 45-60 minutes.

Tasks:

- Replace placeholder water with tessellated mesh.
- Add slow non-audio wave displacement.
- Add horizon fog band and subtle water highlights.
- Add reflection receiver strips on water surface.

Acceptance:

- Water recedes into depth.
- Water remains stable under audio.
- The scene has a convincing horizon relationship.

### Milestone 3: Real 3D Icebergs

Status: complete in third implementation slice.

Implemented:

- Replaced the per-frame placeholder iceberg draw with persistent generated meshes.
- Each iceberg now has an irregular waterline ring, shoulder ring, jagged top ring, and tapered underwater ring.
- Added separate above-water, below-water, and rim-line meshes per iceberg.
- Added varied face colors for top, front, shadow, underwater mass, and cold side tint.
- Kept `icebergScale` as a live draw-time scale so it can be tuned without reseeding geometry.
- Kept iceberg geometry independent of audio.
- Maintained far-to-near placement with shared camera depth testing.

Revision:

- Reworked the iceberg generator toward Delaunay-style low-poly facets from controlled silhouette/interior points.
- Made the visible above-water ice flatter, broader, and subtler.
- Moved the sharper/spikier geometry into the submerged mass below the waterline.
- Disabled glowing edge/rim styling by default.
- Reduced the default iceberg count and disabled the fake reflection strip by default so the ice reads as physical mass floating in the water.

Target duration: 60-90 minutes.

Tasks:

- Generate irregular low-poly iceberg meshes.
- Add above-water and below-water pieces.
- Add cyan rim-line mesh and magenta side tint.
- Place icebergs at multiple `z` depths.
- Draw far-to-near with depth test.

Acceptance:

- Icebergs read as floating 3D objects.
- Near icebergs occlude far geometry.
- Icebergs do not pulse with audio.

### Milestone 4: 3D Aurora Volume

Status: complete in emissive aurora implementation slice.

Implemented:

- Replaced the placeholder aurora guide with a depth-layered emissive aurora volume.
- Added broad additive glow shells around each curtain so the aurora reads as light, not just geometry.
- Added brighter translucent curtain cores with x/y/z fold displacement.
- Added vertical ray/filament meshes whose density and intensity react to audio highs and peaks.
- Added a hidden waveform-derived energy field that modulates local aurora brightness without drawing the raw waveform.
- Added `auroraBloom`, `auroraFoldStrength`, `auroraRayDensity`, and `auroraCurtainCount` controls.
- Raised default glow/bloom/audio values so the loaded scene starts from a visibly luminous aurora.

Revision:

- Converted the aurora from planar back-wall sheets into cylindrical curtain shells around the centered world.
- Converted `u` in the aurora mesh from screen-left/screen-right into angle around the disk.
- Replaced the large planar air-glow rectangle with curved additive atmospheric shell meshes.
- Kept the successful glow and bright fold-edge treatment.
- Made vertical ray lines a true optional path and set the default ray density to `0.0` to avoid pink scratch artifacts.
- Converted aurora reflection and water mist helpers to radial meshes so they cannot produce rectangular horizon boxes.

### Milestone 4B: Centered Polar World Revision

Status: complete in orbitable-world implementation slice.

Implemented:

- Replaced the rectangular/trapezoid sea plane with a radial disk mesh centered at world origin.
- Set the default waterline to `Y=0`.
- Repositioned icebergs with polar radius/angle placement around the disk.
- Kept iceberg geometry local to the waterline so above-water and submerged mass intersect the disk naturally.
- Reinterpreted existing aurora depth controls as inner/outer cylindrical radii without renaming parameter IDs.
- Removed the rectangular horizon-mist wall and replaced it with a radial edge mist ring.
- Updated catalog and scene defaults for centered orbitable framing.

### Milestone 4C: Aurora Audio Reactivity Tuning

Status: complete in audio-reactivity tuning slice.

Implemented:

- Kept the established aurora glow/fold/edge visual language.
- Boosted incoming audio analysis with perceptual gain curves before smoothing.
- Lowered default smoothing so the aurora reacts faster to live input.
- Increased audio contribution to emissive shell brightness, curtain core alpha, fold radius, vertical lift, shimmer, and glowing fold-edge line width.
- Preserved audio scope: reactivity is focused on the aurora instead of sea or iceberg geometry.

### Milestone 4D: Full-Ring Environment Pass

Status: complete in spatial/environment implementation slice.

Implemented:

- Expanded the aurora from a partial arc into a full-circle ring when the configured arc length reaches the scene circumference.
- Removed endpoint darkening for full-ring auroras so the curtain does not appear stuck in one quadrant.
- Enlarged the ocean disk defaults so the aurora sits above water instead of off to the side.
- Added procedural top/moonlight coloring to the water surface.
- Made the water surface more translucent and stopped it from writing depth so submerged iceberg mass remains visible.
- Brightened the underwater iceberg mesh.
- Added slow deterministic non-audio iceberg orbit, bob, and yaw drift.
- Increased star count and extended star placement through the full screen.

### Milestone 4E: Independent Partial Aurora Arcs

Status: complete in rotating-arc refinement slice.

Implemented:

- Removed the aurora full-ring fallback from the main curtain draw path.
- Reinterpreted `auroraWidth` as per-curtain arc length so ribbons leave open water visible.
- Gave each aurora curtain an independent orbital center, angular speed, radius layer, and width breathing.
- Mapped audio to modest increases in curtain rotation speed and arc width while preserving the existing glow/fold look.
- Moved broad atmospheric glow shells onto each partial ribbon so the halo travels with the curtain instead of becoming a static backdrop.
- Increased moonlit water color and default water highlight so the sea reads as a visible disk under the aurora.

### Milestone 4F: Shared Polar Aurora Flow

Status: complete in magnetosphere-flow refinement slice.

Implemented:

- Added one shared polar flow sampler for the aurora system instead of independent large-scale wobble per curtain.
- Kept the ribbons roughly concentric while letting each curtain slide around its own ring.
- Sampled broad world-angle lobes, coherent polar noise, and polar audio energy so multiple rings bend under related forces when they occupy the same sector.
- Applied the same flow field to atmosphere halos, glow shells, curtain cores, edge lines, and optional rays so the visual layers move together.
- Reduced older layer-local radial wobble so it reads as fine texture riding the larger magnetic flow.

Target duration: 90-120 minutes.

Tasks:

- Add multiple aurora curtain surfaces at different depths.
- Build energy field from audio.
- Create vertical segmented ribbons with x/y/z folds.
- Add expanded additive glow shells.
- Add thin vertical ray meshes.
- Add water reflection intensity driven by aurora energy.

Acceptance:

- Aurora floats above and behind water.
- Aurora has visible depth layers.
- Aurora glows and pulses with audio.
- Raw waveform is not traceable.

### Milestone 5: Polish Defaults and Validate

Target duration: 30-45 minutes.

Tasks:

- Tune default colors, scale, and depth.
- Set scene preset to use only `generative.arcticAuroraScene`.
- Regenerate catalog/manifest fixtures.
- Run contract checks.
- Attempt MSBuild only if the local build process is not wedged.

Acceptance:

- Defaults are showable without manual tuning.
- Static validators pass.
- Any native build failure is documented clearly.

## Parameter Set

Minimum useful parameters:

```text
visible
alpha
sceneScale
sceneOffsetY
sceneOffsetZ

waterWidth
waterNearZ
waterFarZ
waterLevel
waterWaveIdle
waterHighlight

icebergCount
icebergScale
icebergSpread
icebergRimLight

auroraCurtainCount
auroraWidth
auroraHeight
auroraBaseY
auroraDepthNear
auroraDepthFar
auroraFoldStrength
auroraGlow
auroraRayDensity
auroraReflection

audioAmount
audioSmoothing
bassGlow
midsFold
highsRays
peakPulse

skyR, skyG, skyB
waterR, waterG, waterB
auroraR, auroraG, auroraB
aurora2R, aurora2G, aurora2B
iceRimR, iceRimG, iceRimB
iceAccentR, iceAccentG, iceAccentB
```

## Audio Mapping

Audio should feel like energy moving through the aurora field.

```text
level -> aurora global emissive alpha and reflection intensity
bass  -> broad glow envelope and slow curtain lift
mids  -> fold depth, curtain twist, lateral billow
highs -> vertical ray density and fine shimmer
peak  -> soft aurora pulse envelope
waveform abs -> subtle local brightness ripples only
```

No audio mapping for sea or iceberg geometry.

## Visual Acceptance Criteria

The scene is acceptable when:

- Sea is a real perspective plane.
- Icebergs are actual 3D meshes with visible top/side/front planes.
- Aurora is visibly behind/above the sea in world space.
- Aurora is layered in depth, not a single flat wall.
- Aurora glows even without full post-process bloom.
- Audio clearly affects aurora brightness/folds/rays.
- Raw waveform is not visible.
- Sea and icebergs stay physically stable under audio.
- The still frame is not embarrassing to show publicly.

## Technical Acceptance Criteria

- Uses existing `LayerDrawParams::camera`.
- Uses `ofEnableDepthTest()` and clears depth buffer inside the layer draw.
- Uses depth writes for solid sea/ice geometry.
- Disables depth writes for translucent aurora/glow.
- Keeps implementation CPU/ofMesh-based for today.
- Does not require new shader or post-processing infrastructure for the first pass.
- Leaves a path to FBO bloom later.

## Today Fallbacks

If native MSBuild keeps timing out:

- Continue to run static validators.
- Keep compile-risk low by following known `GridLayer`/`SolarSystemLayer` patterns.
- Avoid shader code and template-heavy utilities.
- Avoid changing global render pipeline.
- Document that native build was attempted and timed out.

If aurora still reads too flat:

- Add more depth-separated curtain layers.
- Increase z separation.
- Add glow shells at multiple scales.
- Increase vertical segmentation.
- Make water reflections visibly follow perspective on the sea plane.

If icebergs still read flat:

- Add explicit top face color.
- Add dark underside mesh below water.
- Add side face color variation based on normal direction.
- Add rim line mesh along top edges and vertical front edges.

## Cleanup From Previous Prototype

The earlier separate layers can remain in the catalog temporarily:

```text
generative.constellationStarfield
generative.auroraCurtains
generative.arcticSeaIcebergs
```

But the show preset should stop using them. Replace the public arctic scene preset with:

```text
generative.arcticAuroraScene
```

After `ArcticAuroraSceneLayer` is approved, decide whether to:

- keep old layers as experimental components,
- hide them from the Browser,
- or remove them before merging.

## Suggested Commit Message

```text
Add integrated 3D arctic aurora scene layer
```

## Suggested PR Summary

```text
Adds an integrated ArcticAuroraSceneLayer that uses the existing Synaptome camera and depth buffer to render a cohesive 3D arctic scene. The layer draws a tessellated sea plane, volumetric low-poly icebergs, and depth-separated aurora curtain meshes. Audio modulation is scoped to the aurora energy field, driving glow, fold depth, shimmer, vertical rays, and reflections without moving the sea or icebergs.
```

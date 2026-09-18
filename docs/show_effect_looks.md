# Show Effect Looks

These six saved scenes provide curated settings for the existing effect
processors. They contain an algorithmic source in layer 1 and its effect in
layer 2. They require no rebuild and introduce no processor, shader, host, or
runtime changes.

## Load And Build Live

1. Open the Browser's **Scenes > Saved** section.
2. Select a `show-fx-*` scene and press Enter. This loads a complete two-layer
   scene and replaces the current composition, so save your current scene
   first if you want to keep it.
3. Replace layer 1 with another visual to reuse the loaded effect settings.
   Keep the effect in layer 2. Its coverage is one upstream layer.
4. Add further sources and effects in the six free layers. Increase the
   effect's Coverage to process more preceding layers, or use 0 for all
   preceding layers.
5. Save As a new scene after making your changes.

The FX Browser still lists the established processors. These are named
effect-look scenes, not six new effect implementations or independently
stackable instances of the same processor. Loading a bare FX Browser entry
does not recall one of these looks. Effect parameters are shared by processor
type, so use at most one instance of each processor in a scene.

| Saved scene | Source and effect | Intended appearance | Useful live controls |
| --- | --- | --- | --- |
| `show-fx-01-river-prism` | River Formation + Mirror | Eight-fold cyan river symmetry with retained line detail | Mirror Angle, Zoom, Mix; source River Width |
| `show-fx-02-circuit-etching` | Circuit Mycelium + Dither | Coarse jade traces with ordered color quantization | Dither Cell Size, Mode; source Glow |
| `show-fx-03-cellular-glyphs` | Lenia + ASCII | Growing green cellular tissue described by stable Braille glyphs | ASCII Block Size, Gamma, Color Mode; source Edge Glow |
| `show-fx-04-phosphor-bloom` | Flow Field + CRT | Cyan-green flowing filaments with a restrained phosphor halo | CRT Glow, Softness, Scanline; source Curl Amount |
| `show-fx-05-chromatic-drift` | Perlin Noise + CRT | Slowly flowing color fields with separated edges and mild tracking movement | CRT RGB Misalignment, Per-Channel Offset, Tracking Wobble |
| `show-fx-06-tidal-afterimage` | Murmuration + Motion Extract | Short cyan-to-violet echoes reveal the flock's changing direction | Motion Mix, Threshold, Fade; source Point Size |

## MIDI, OSC, And Sound

All settings use the existing parameter registry and normal MIDI/OSC Learn
flow. The scenes omit `mappings`, bank selection, global tempo, camera values,
hardware selection, and window preferences. They do not install a replacement
mapping snapshot.

The existing scene loader clears the old composition first, which removes
routes addressed to its visual layers and clears parameter modifiers and
scene-local banks. Global and effect routes can remain live and alter the
loaded look. Save your current performance scene before loading a starter,
then apply the new source mappings and save your finished scene with its
mapping snapshot. These content files do not change that loader behavior.

For the bundled layout, source parameters use `console.layer1.*`. Effects
retain their canonical `effects.*` addresses, regardless of the layer number.
The following are suggested mapping ranges, not automatically applied routes.

| Look | Suggested audio source | Target | Suggested output range |
| --- | --- | --- | --- |
| River Prism | `/sensor/host/localmic/mic-bass` | `effects.mirror.zoom` | 0.95 to 1.35 |
| Circuit Etching | `/sensor/host/localmic/mic-highs` | `console.layer1.glow` | 0.35 to 0.85 |
| Cellular Glyphs | `/sensor/host/localmic/mic-mids` | `effects.ascii.gamma` | 0.65 to 1.05 |
| Phosphor Bloom | `/sensor/host/localmic/mic-level` | `effects.crt.glow` | 0.15 to 0.45 |
| Chromatic Drift | `/sensor/host/localmic/mic-highs` | `effects.crt.rgbMisalignment` | 0.08 to 0.38 |
| Tidal Afterimage | `/sensor/host/localmic/mic-level` | `effects.motion.mix` | 0.35 to 0.75 |

Choose the observed source in Control & Mapping, set an absolute output range,
then learn the corresponding MIDI or OSC input. Set the input range against
the actual audio level meter and add smoothing there. Replace `localmic` with
the observed external source, such as `zoom`, when using external host audio.
Save your applied mappings with your performance scene. No element contains
hidden audio polling or direct OSC address handling.

Each OSC address has one shared source profile. These absolute ranges replace
that address's profile, including any relative Scale profile applied by a new
element's host-mic suggestion. Preview conflicts before applying. Use separate
derived source addresses when an effect and an element need different output
transforms simultaneously, or retain the shared relative profile and adjust
their base parameter values instead.

Motion's established `fadeBeats` ID is labeled **Motion Fade (frames)** in the
UI. Its current implementation holds at most six sampled frames and is not
beat synchronized. This look starts at 4; use 0 to 5 for meaningful adjustment.
Its legacy color controls use 0 to 255 and Tail Opacity uses 0 to 100 percent.

## Validation And Extension Boundary

The JSON schemas, referenced assets, complete effect parameter sets, parameter
IDs, and declared source ranges are checked. No Windows show-machine render,
physical MIDI test, projection rehearsal, or frame-rate measurement is claimed
by these static checks. Let each simulation develop and audition the effect
on the show computer before saving your performance version.

The existing effect path is deliberately unchanged. It hardcodes processor
dispatch in `PostEffectChain`; the FX installation branch in `ofApp` does not
apply asset `defaults`, and route synchronization recognizes canonical FX
asset IDs. Creating renamed FX assets would therefore provide unreliable
recall. The saved-scene path applies the full registry-addressed settings in
each FX slot's `parameters` and survives ordinary scene serialization,
including Dither Mode, which is omitted by the legacy top-level effect fields.

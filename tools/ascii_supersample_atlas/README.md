# ASCII Supersample Atlas Baker

Offline tool that produces prebaked glyph atlases + descriptor LUTs for the
`effects.asciiSupersample` shader. This replicates the runtime font analyzer so
Release builds can ship known-good mask data instead of rebuilding atlases on
the operator’s machine.

## Usage

```powershell
cd <repo>
python tools/ascii_supersample_atlas/bake_precomputed_atlas.py `
  --data-root synaptome/bin/data `
  --font VCR_OSD_MONO_1.001.ttf
```

Arguments:

- `--data-root` (default `synaptome/bin/data`): points at the
  openFrameworks data directory.
- `--fonts-dir` (default `<data-root>/fonts`): where `.ttf`/`.otf` files live.
- `--output-dir` (default `<data-root>/fonts/ascii_supersample/prebaked`):
  prebaked atlas destination.
- `--manifest` (default `<data-root>/fonts/ascii_supersample/manifest.json`):
  output manifest path.
- `--font`: repeat per filename to bake a subset of fonts; omit to bake every
  `.ttf/.otf` under `--fonts-dir`.
- `--font-size`: font size used during rendering (default `48`).

Each baked font produces:

- `atlas.png`: RGBA atlas containing all printable ASCII glyphs.
- `atlas.lut.json`: descriptor data (2x3 tap averages + means + rects).
- `manifest.json`: updated with metadata, checksums, and descriptor-uniqueness
  stats so validation can prove every glyph differs.

## Notes

- The script depends on Pillow (`pip install pillow`).
- Glyph descriptors follow the 2x3 sampling strategy outlined in
  `docs/roadmap/in_progress/ascii_supersampling.md`.
- Packaging rules should include the `prebaked/` directory + manifest so the
  runtime prefers prebaked atlases before falling back to on-boot font baking.

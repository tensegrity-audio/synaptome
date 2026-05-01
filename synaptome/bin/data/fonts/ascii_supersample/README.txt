Supersample atlases now ship as prebaked assets alongside the Release build.

Run `python tools/ascii_supersample_atlas/bake_precomputed_atlas.py` from the
repo root to regenerate the assets inside `prebaked/` plus `manifest.json`.
Each prebaked atlas contains:

- `atlas.png` (RGBA glyph coverage for printable ASCII)
- `atlas.lut.json` (descriptor data + glyph rect metadata)

At runtime, the supersample effect prefers these prebaked assets and only falls
back to on-boot atlas baking when an entry is missing from the manifest.

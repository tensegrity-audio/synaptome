# Third Party Notices

The MIT License in `LICENSE` applies to Synaptome source code and documentation owned by Tensegrity Audio unless a file says otherwise.

## Runtime Dependencies

- openFrameworks: MIT License. Synaptome depends on an external openFrameworks SDK install; the SDK is not bundled in this repository.
- nlohmann/json 3.11.3: MIT License. Bundled at `thirdparty/nlohmann/json.hpp`.

## Bundled Asset Notes

- `synaptome/bin/data/fonts/VCR_OSD_MONO_1.001.ttf` and the derived atlas files under `synaptome/bin/data/fonts/ascii_supersample/` are third-party font assets used by the default text and supersample examples. Public font listings identify this font as free to use, but the font assets are not relicensed by Synaptome.
- `synaptome/bin/data/models/*.stl` contains bundled example model assets referenced by the current geometry catalog. Treat these as example content assets, not as part of the MIT-licensed source code.

Before adding new bundled fonts, media, models, or show assets, include their source and license/provenance here or keep them outside the public repo.

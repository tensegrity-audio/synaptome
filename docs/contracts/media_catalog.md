# Synaptome Media Catalog Contract

Status: Current manifest-only intake contract, established 2026-07-18.

Synaptome discovers video clips only through
`synaptome/bin/data/config/videos.json`. Folder scanning, file watching, and
automatic Browser registration are not part of the current contract.

## Safe Baseline

The catalog contains one reviewed, redistributable example clip:
`aurora-veil-r1`. It replaced the former dangling `default-loop` reference only
after the file, stable ID, SHA-256, prompt, encoding settings, license, and
redistribution state were recorded. The public example fixture remains empty
to prove that zero-asset catalogs are still valid.

## Roots

- `synaptome/bin/data/media/public/**`: tracked, redistributable runtime media.
- `synaptome/bin/data/media/local/**`: operator-local media; ignored by source
  control and forbidden in the committed catalog.
- `docs/examples/media_catalog_example.json`: public catalog-shape example;
  contains no binary media.
- `tools/testdata/media_catalog/**`: semantic contract fixtures; contains no
  production media.

## Clip Rules

Every cataloged clip must declare:

- a stable lowercase `id`;
- a label and relative path under `../media/`;
- a filename stem equal to the stable ID;
- a monotonically increasing revision;
- the exact SHA-256 of the referenced file;
- loop/prewarm behavior;
- creator, source, license, redistribution permission, and notes.

Generated media must additionally record the tool, model, prompt, and settings.
Do not put dates or generation-run identifiers into the stable clip ID. Those
belong in provenance or source history.

## Replacement And Deletion

- Preserve an ID only when the replacement is the same conceptual asset.
- Increment `revision`, record the previous SHA-256 and a reason, then update
  the current SHA-256.
- Use a new ID when the asset's meaning or intended use changes.
- Remove layer defaults and scene/preset references before deleting an asset.
- Never overwrite an absent or restricted asset merely because a manifest path
  already exists.

## Validation

```powershell
python tools\media_catalog_regression.py --check
python tools\validate_configs.py synaptome\bin\data\config\videos.json
python tools\validate_configs.py --public-app
```

The dedicated validator checks schema shape, manifest-only discovery, stable
IDs and paths, duplicate IDs/paths, file existence, SHA-256, generated-media
provenance, redistribution permission, replacement history, and layer-default
references. Negative fixtures prove the most important rejection paths.

Current reviewed asset:

| ID | Runtime path | Revision | SHA-256 | Intended use |
| --- | --- | --- | --- | --- |
| `aurora-veil-r1` | `../media/public/aurora-veil-r1.mp4` | 1 | `fdddea31e9509dc90d5bd6a045c1cd5cdf415fee03a050f17715f54c03a6ec8f` | Default public media-layer loop and projection-safe SDK example. |

## Promotion Rule

Each media addition must be one bounded request containing one content family,
one public destination, one manifest entry, provenance, and validation
evidence. Package/runtime work and media are committed at separate rollback
boundaries even when one user-directed cleanup pass authorizes both.

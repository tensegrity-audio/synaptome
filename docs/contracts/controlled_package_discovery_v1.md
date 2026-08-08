# Controlled Package Discovery v1

Status: SEAC-10 implementation contract

Configuration schema:
[`../schemas/controlled_package_discovery.schema.json`](../schemas/controlled_package_discovery.schema.json)

Pure scanner and snapshot model:
[`../../tools/controlled_package_discovery_v1.py`](../../tools/controlled_package_discovery_v1.py)

## Boundary

Discovery reads only enabled roots in strict machine-local configuration.
Absent configuration and the checked-in configuration are disabled. Roots
are never inferred from the working directory, home directory, drives, media
folders, scenes, or manifests.

Refresh produces a complete immutable snapshot. Scanning never calls a
creator, `setup()`, a parameter/action API, a scene loader, a mapping
publisher, or Runtime adoption. Runtime code loading is outside this contract.
A source package is `available` only when package, implementation, type,
definition, registry-prefix, and normalized descriptor signature exactly
match controlled generated registration. STL content is `available` only
through `templates.model.stl` and the precompiled `stlModel` type record.

## Configuration

The closed v1 document contains `schemaVersion`, `enabled`, and `roots`.
Every root has a local ID, a kind (`packages` or `generated-stl`), a path, and
an optional enable flag. Relative paths resolve from the configuration
directory. Root paths are local diagnostics and never portable identities.
Directory symlinks are not followed; Package v1 and generated-content readers
enforce referenced-file containment and exact case.

The retained runtime intake root is
`synaptome/bin/data/layer_packages`. The package and generated-STL docs roots
are selected explicitly by fixtures/tests. No general media-library discovery
is claimed.

## Snapshot, Identity, And Conflict Policy

The v1 snapshot records generation, enabled/stale state,
`constructionFree: true`, `activationRequired: true`, diagnostics, counts, and
a deterministic candidate list. Candidates include all stable identities and
versions, signature, status, exact type availability, local provenance,
diagnostics, inspectable declarations, prior status/signature, and a catalog
descriptor reserved for explicit activation.

Statuses are `available`, `inspection-only`, `invalid`, `incompatible`,
`duplicate`, `unavailable`, and `pending-replacement`. Only `available` is
directly activatable; `pending-replacement` requires its distinct explicit
acceptance action and replaces only the matching catalog record. A stale
snapshot is never activatable.

A generated sidecar may declare `contentId` and `contentVersion`; otherwise
identity derives from template namespace plus normalized root-relative path.
Absolute paths never enter IDs or signatures. Package signatures cover
canonical manifest data plus validated declared files. Content signatures
cover template identity/version, content ID, template and sidecar data, and
content bytes.

Overlapping roots reaching one physical manifest are coalesced with all root
provenances. Distinct candidates claiming one package, type, definition,
registry-prefix, template, or content ID are all `duplicate`; there is no
root-order winner. Canonical catalog collisions fail closed. An exact
controlled registration backing its package is implementation evidence, not a
second claimant.

## Refresh, Replacement, And Removal

Same identity/version with changed bytes becomes `duplicate`. A compatible
higher version used by an activation becomes `pending-replacement`. A removed
candidate remains visible as `unavailable` with its last signature. An
unexpected scan/publication failure retains the prior generation and
candidates, marks the snapshot stale, and appends a failure diagnostic.
Refresh never changes active layers.

## Inspect-Before-Activate

Browser renders status, identity/version, signature, provenance, diagnostics,
refresh, and activation. Rows remain unbound read-only metadata. Activation is
enabled only for a current, enabled, exact-type `available` candidate.

The host re-reads the current snapshot and checks the type against the live
`LayerFactory`, closing the inspect/activate time-of-check gap. It first writes
only candidate ID, signature, type ID, definition ID, and enabled state to the
ignored local activation document, then validates and publishes the catalog
descriptor. Failed catalog publication restores the prior activation
document. Publication does not construct or assign a layer; later assignment
uses the existing Runtime prepare/adopt transaction. Presets and mappings
remain separate SEAC-9 transactions.

## Operator Workflow

1. Review and enable only bounded roots in `package-discovery.json`.
2. Produce the local snapshot:

   ```powershell
   python tools\controlled_package_discovery_v1.py `
     --config synaptome\bin\data\config\package-discovery.json `
     --prior synaptome\bin\data\config\package-discovery.snapshot.json `
     --output synaptome\bin\data\config\package-discovery.snapshot.json
   ```

3. In Browser, open **Controlled Discovery**, refresh, inspect, then activate
   only an available candidate.
4. Assign the activated definition through Console and invoke preset/mapping
   transactions separately.

## Evidence And SEAC-11 Input

Focused gates:

```powershell
python -m pytest tests\test_controlled_package_discovery_v1.py -q
python tools\generated_layer_catalog_regression.py --check
python tools\layer_browser_inspection_payload.py --check
.\synaptome\tests\BrowserFlowTest\x64\Release\BrowserFlowTest.exe
```

The matrix covers strict/default-off roots, package and STL success,
root-independent identities/signatures, overlap coalescing, hostile inputs,
missing registered types, collisions, drift/replacement/removal, stale
retention, construction isolation, and activation rollback.

The reference source package is satisfied by generated registration and the
STL slice by an existing precompiled built-in. No implemented fixture requires
native loading, unloading, ABI policy, or hot reload. SEAC-11 therefore starts
with no native loader and requires a concrete unsatisfied workflow to justify
one.

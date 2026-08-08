# Controlled Package Discovery v1: SEAC-10 Execution Handoff

Status: Complete

Owner task: SEAC-10

## Outcome

Synaptome may inspect explicitly configured package roots and supported
data-only content roots while discovery is enabled, but discovery itself never
loads code, creates an element, changes a composition layer, applies a preset
or mapping, or rewrites portable show state.

An operator can inspect every candidate and its exact status in the Browser.
Only a separate, explicit activation transaction may publish a valid candidate
whose implementation type is already present in the controlled Runtime
registration set.

The promotion gate is strict: malformed, duplicate, incompatible, refreshed,
removed, or unavailable content must not change the active show state.

## Existing Inputs

SEAC-10 extends these implemented surfaces rather than replacing them:

- Element Package v1 supplies stable package, type, asset, definition,
  parameter, action, preset, mapping, dependency, compatibility, and migration
  identities.
- The generated registration set proves which source element types were
  validated and compiled into this host build.
- Package control transactions provide explicit preview, apply, conflict,
  persistence, and rollback behavior for presets and mappings.
- `tools/layer_package_discovery.py` names the tracked fixture root and the
  future runtime package root. It is currently tooling-only.
- `layer-package-inspection.json` and the Browser provide a construction-free,
  read-only inspection seam.
- The generated-layer template and sidecar fixtures prove one STL content file
  can become stable catalog metadata through a precompiled element type.
- `layer-packages.json` is an explicit, default-off activation allowlist. It is
  not a folder-scanning contract.

## Frozen Boundaries

### Discovery Is Not Activation

Discovery builds immutable candidate metadata in isolation. It must not call a
creator, `setup()`, element actions, parameter writes, scene loads, mapping
publication, or package preference publication.

Activation remains default-off and explicit. A candidate can be visible in the
Browser without being activatable. Inspection, activation eligibility,
activation preference, catalog availability, layer assignment, and a running
element are distinct states and must not be represented by one boolean.

### No Runtime Code Loading

SEAC-10 does not load DLLs, shared libraries, object files, scripts, or package
source. A source package is activatable only when its exact stable type and
package record agree with the controlled generated registration already
compiled into the host.

Data-only discovery may bind supported content to an already registered,
precompiled element type through a validated template. The content file is not
an element implementation. Native module value, ABI, signing, unloading, and
hot-reload policy belong to SEAC-11.

### Local Roots Stay Local

Discovery roots are operator/machine configuration. They are absent or
disabled by default and must not be copied into scenes, mapping banks, package
manifests, generated registration records, or other portable show documents.
Absolute paths may appear in local diagnostics only.

The first implementation must preserve the current known roots:

- `docs/examples/layer_packages` for tracked fixtures and tests;
- `synaptome/bin/data/layer_packages` for default-off runtime package intake.

Additional package or content roots require explicit local configuration.
There is no unbounded drive, home-directory, media-library, or recursive
symlink traversal.

## Candidate Model

One refresh produces a complete immutable discovery snapshot. Each candidate
records stable identity, declared version, content signature, root/provider
provenance, local diagnostic path, validation diagnostics, implementation
availability, and one status:

| Status | Meaning | Activatable |
| --- | --- | --- |
| `available` | Valid, compatible, unique, and backed by an exact registered type. | Yes, through an explicit transaction |
| `inspection-only` | Valid metadata whose source/type is not activatable in this build. | No |
| `invalid` | JSON, schema, containment, signature, migration, template, or sidecar validation failed. | No |
| `incompatible` | SDK, Runtime, dependency, capability, package, or content version is unsupported. | No |
| `duplicate` | A stable identity conflicts with another candidate or controlled catalog/registration identity. | No |
| `unavailable` | A declared file, provider, type, dependency, or content item is missing at refresh time. | No |
| `pending-replacement` | A valid new version could replace an existing explicit activation after operator review. | No automatic replacement |

Diagnostics may retain the last known valid signature and prior status, but
stale metadata must never be presented as currently available.

## Stable Identity And Signatures

- Package identity is `packageId`; package version is `packageVersion`.
- Runtime implementation identity is `element.id`.
- Scene/catalog definition identity is `asset.id`; `asset.type` must equal the
  expected registered type, and `asset.registryPrefix` remains the public
  parameter namespace.
- Preset, mapping, action, parameter, migration, template, and content IDs use
  their existing declared contracts.
- No stable ID, route ID, preference key, scene field, or content identity may
  contain or hash a machine-local absolute path.
- Package signatures use the canonical validated manifest plus every declared
  package file. Data-only signatures use the template identity/version,
  declared stable content identity, sidecar data, and content bytes.
- Two distinct byte/signature sets claiming the same identity and version are
  a conflict, not an implicit update.

If a content drop has no explicit sidecar ID, its derived ID may use a
normalized root-relative path plus template namespace. Moving or renaming that
file changes the derived ID. A sidecar-provided stable ID is required when
identity must survive reorganization; the absolute root never participates.

## Discovery Pipeline

Every refresh follows the same side-effect-free order:

1. Read the enabled local root list and resolve only paths contained by those
   roots.
2. Enumerate package manifests and explicitly supported template/content
   patterns in deterministic normalized relative-path order.
3. Parse and validate manifest, template, sidecar, and referenced files without
   creating an element.
4. Normalize supported contract versions and reject future or ambiguous
   versions before publication.
5. Validate contained paths, exact filename case, declared signatures,
   dependencies, capabilities, compatibility ranges, and migrations.
6. Compare package declarations with the generated registration record or
   resolve data-only content to an exact precompiled type.
7. Detect collisions across the entire candidate set, the controlled
   registration set, the canonical legacy catalog, and generated definitions.
8. Publish one immutable inspection snapshot atomically.

A scan or publication failure leaves the prior inspection snapshot readable,
marks it stale with the failed refresh diagnostics, and leaves Runtime,
catalog, scenes, mappings, preferences, registry values, and active layers
unchanged.

## Conflict Policy

There is no first-found, last-found, root-priority, or newest-timestamp winner.

- Distinct candidates claiming the same package, type, asset/definition,
  registry-prefix, template, or generated-content ID are all `duplicate`.
- A discovered candidate cannot shadow a controlled built-in/generated
  registration or canonical legacy catalog entry.
- The same physical manifest reached through overlapping roots is coalesced
  into one candidate with multiple local root provenances before collision
  checks.
- Aliases and migrations are accepted only through their declared contracts;
  they do not excuse two live owners of one current identity.
- Duplicate diagnostics identify every claimant without exposing local paths
  in portable state.

The active show remains authoritative while a conflict exists.

## Replacement, Refresh, And Deletion

### Replacement

- A higher compatible `packageVersion` with the same stable identities and a
  new signature becomes `pending-replacement`.
- The same identity/version with a different signature becomes `duplicate`.
- A lower, incompatible, or migration-incomplete version is not eligible.
- Replacement never hot-swaps a running element or rewrites an activation
  record. The operator must inspect and explicitly accept it through the
  activation transaction.
- If replacement requires a Runtime type not compiled into the current build,
  it remains `inspection-only` or `unavailable`; SEAC-10 does not load it.

### Refresh

Refresh replaces only the inspection snapshot. It does not automatically
activate newly found content, reload active content, remove catalog entries
used by a running layer, apply changed defaults, or resolve duplicates by
arrival time.

### Deletion And Unavailability

- A removed inactive candidate becomes `unavailable` for the current snapshot
  and cannot be newly activated.
- A removed package/content item already used by a running layer is not
  automatically unloaded. Its adopted Runtime state continues until an
  explicit replace, clear, shutdown, or scene transaction says otherwise.
- A later reload or activation that requires missing content fails before
  adoption and preserves the existing layer.
- Last-known metadata may remain visible for diagnosis, clearly labeled stale
  and unavailable.

## Inspect-Before-Activate Browser Flow

The Browser must expose:

- package/content label plus stable IDs and versions;
- current status, compatibility, registered-type availability, signature, and
  actionable diagnostics;
- local source/root provenance as machine-local diagnostic information;
- declared parameters, groups, options, presets, mappings, capabilities,
  dependencies, tests, and content references;
- an explicit refresh action;
- an activation action enabled only for `available` candidates;
- a replacement comparison and explicit acceptance action for
  `pending-replacement`.

Selecting or expanding a row is inspection only. Presets and mappings remain
suggestions until their existing SEAC-9 transactions are explicitly invoked.
Activation must use recoverable preference/catalog publication and the
existing Runtime prepare/adopt transaction; any validation, write, creation,
setup, or adoption failure restores the prior working state.

## Data-Only Content Slice

The first supported content slice is the existing generated STL template and
sidecar contract:

```text
configured content root
  -> validated template + supported extension
  -> validated content file and optional sidecar
  -> stable generated definition
  -> exact precompiled STL-capable element type
  -> Browser inspection
  -> explicit activation/assignment
```

The implementation must not claim arbitrary media-folder discovery. Each
content family needs a versioned template, bounded extensions, an exact
precompiled type, stable identity rules, containment checks, fixtures, and
negative coverage. A malformed sidecar, missing content file, unsupported
extension, or unavailable element type produces a non-activatable candidate.

## Implementation Slices

1. **SEAC-10A — Contract and root policy:** add a strict local discovery
   configuration and pure snapshot DTO; preserve default-off behavior.
2. **SEAC-10B — Package scanner:** reuse Element Package v1 normalization,
   signatures, registration parity, and whole-snapshot collision analysis.
3. **SEAC-10C — Data-only scanner:** promote the STL template/sidecar fixture
   through an exact precompiled-type availability check.
4. **SEAC-10D — Browser inspection:** render statuses, diagnostics, refresh,
   local provenance, and replacement comparisons without construction.
5. **SEAC-10E — Explicit activation:** adapt available candidates, or a
   separately reviewed and explicitly accepted replacement, into the existing
   recoverable allowlist/catalog and Runtime transaction.
6. **SEAC-10F — Lifecycle policy:** prove refresh, replacement, deletion,
   unavailability, rollback, and active-state isolation.
7. **SEAC-10G — Promotion evidence:** run contract, fixture, BrowserFlow,
   native, confidence, extraction, junction, physical Release, and live
   projection gates.

## Required Test Matrix

At minimum, automated coverage must prove:

- absent config, disabled config, missing root, empty root, and valid root;
- one valid package and one valid generated STL candidate;
- malformed JSON/schema, future version, wrong case, path escape, symlink
  escape, missing declared file, changed signature, and invalid sidecar;
- missing/incompatible SDK, Runtime, dependency, capability, template, content
  family, or registered type;
- duplicate package, type, asset/definition, registry prefix, template, and
  generated-content IDs, including legacy catalog and controlled registration
  collisions;
- overlapping-root coalescing without path-derived identity;
- same-version content drift, compatible higher-version replacement,
  incompatible replacement, refresh, and deletion while inactive and active;
- inspection and refresh invoke no creator and mutate no Runtime, registry,
  scene, mapping, preference, catalog, or active composition state;
- explicit activation succeeds only after inspection and rolls back on failed
  validation, preference write, catalog publication, creation, setup, or
  Runtime adoption;
- package presets/mappings never auto-apply;
- active layers survive every failed or hostile discovery case;
- the physical repository and Windows junction checkout produce equivalent
  Release results.

Promotion also requires the existing Element Package v1, registration,
catalog, parameter-manifest, Browser inspection, mapping-bank, public-app,
BrowserFlow, RuntimeCore, LayerPackageBench, Grid confidence, Signal Bloom
confidence, extraction-manifest, and Release build gates.

## Stop Conditions

Stop SEAC-10 promotion if:

- discovery requires arbitrary runtime code/module loading;
- a stable public identity requires a local absolute path;
- root configuration must enter portable show state;
- one malformed or colliding candidate prevents inspection of unrelated valid
  candidates or changes active state;
- refresh, deletion, or replacement implicitly changes a running layer;
- a missing registered type is guessed, substituted, or loaded dynamically;
- package/content validation and Runtime activation cannot share one exact
  descriptor/type identity;
- rollback cannot restore the prior catalog, preference, Runtime, scene,
  mapping, registry, and active composition state;
- physical and junction builds disagree.

Any native-code requirement is recorded for SEAC-11 rather than quietly
widening this phase.

## Promotion Checklist

SEAC-10 is complete only when:

- [x] Discovery configuration is strict, local, explicit, and default-off.
- [x] Package and supported data-only scanners publish deterministic immutable
      candidate snapshots.
- [x] IDs and signatures are independent of local absolute roots.
- [x] Duplicate, replacement, refresh, deletion, and unavailable-content
      policies above are executable and covered.
- [x] Browser inspection precedes every activation and is construction-free.
- [x] Activation accepts only exact precompiled/registered types and is fully
      recoverable.
- [x] Hostile discovery leaves the active show unchanged.
- [x] The full automated, physical/junction Release, and carried-forward
      live-projection gates
      pass.
- [x] Contracts, fixtures, operator guidance, Project Ops, and the SEAC-11
      input are updated with observed evidence.

## SEAC-11 Input

SEAC-11 receives evidence from actual controlled discovery: which valid
packages remain inspection-only because their type is not compiled in, what
operator workflow generated registration cannot satisfy, and whether restart
cost or distribution friction justifies a native module mechanism.

That decision must not reinterpret successful data-only discovery as dynamic
code loading, and it must not weaken the stable identity, inspection,
collision, activation, rollback, or active-state guarantees frozen here.

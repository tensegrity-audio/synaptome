# Element Package v1: SEAC-7 Execution Handoff

Status: Implemented; retained as the frozen execution contract
Owner task: SEAC-7
Prepared: 2026-07-29
Canonical request:
[`../project_ops/completed/spine_element_architecture_convergence.md`](../project_ops/completed/spine_element_architecture_convergence.md)

Predecessor evidence:
[`element_confidence_suite_v1_handoff.md`](element_confidence_suite_v1_handoff.md)

State and ownership authority:
[`../contracts/state_ownership_and_provenance.md`](../contracts/state_ownership_and_provenance.md)

Existing draft schema and reference fixture:

- [`../schemas/layer_package.schema.json`](../schemas/layer_package.schema.json)
- [`../examples/layer_packages/signal_bloom/layer.package.json`](../examples/layer_packages/signal_bloom/layer.package.json)

## Outcome

SEAC-7 promotes the reviewed package scaffolding into one versioned Element
Package v1 document and proves, before activation, that its serialized type and
parameter surface matches the construction-free Runtime contract.

The package becomes the portable bundle description for:

- package, type, and definition identity;
- package and implementation versions;
- SDK/runtime compatibility requirements;
- capabilities and dependencies;
- type descriptor, actions, groups, and parameter declarations;
- definitions and definition-level defaults;
- source and package-relative assets;
- bundled presets and preset banks;
- mapping suggestions;
- confidence-test profiles and fixtures;
- aliases, deprecations, and migrations.

This phase serializes and validates the frozen contract. It does not make
packages automatically discoverable, generate registration records, mutate
operator mappings, or promise native module loading.

## Frozen Reference Scope

Signal Bloom remains the package-backed reference because its reviewed fixture,
bind-only declaration, presets, visible mapping suggestions, package adapter,
and SEAC-6 confidence profile already exist.

Grid remains a built-in comparison fixture. It proves that the serializer and
parity model can describe a simple shipping type without requiring every
built-in to migrate into a distributable package during SEAC-7.

Do not expand this phase into catalog-wide package conversion. One complete
package and one built-in comparison are sufficient to freeze the schema and
parity rules.

## Required Contract Decisions

Before implementation changes runtime behavior, freeze these points in the
schema and validator:

1. `schemaVersion` is the document-format version and is independent of
   `packageVersion` and implementation version.
2. Package IDs, type IDs, definition IDs, registry prefixes, action IDs, group
   IDs, parameter IDs, preset IDs, and mapping-preset IDs are stable identities,
   not labels or paths.
3. All file references are package-relative, normalized, contained by the
   package root, and case-consistent. Absolute paths and traversal outside the
   package fail validation.
4. Compatibility requirements use explicit supported ranges. Unknown required
   capabilities or unresolved required dependencies fail before activation.
5. Type declarations serialize the existing `ElementTypeContract`; the JSON
   vocabulary must not introduce a second semantic authority.
6. Definition defaults may override declaration defaults according to the
   frozen value-precedence chain, but may not change parameter kind or metadata.
7. Package mappings remain suggestions and never become live routes without
   the explicit SEAC-9 preview/apply transaction.
8. Migrations are source-immutable, ordered, deterministic transformations.
   Unsupported future versions and ambiguous migration paths fail closed.
9. A missing optional section and a present empty section retain the meanings
   frozen by SEAC-5.
10. Package inspection and parity validation are construction-free and
    side-effect-free.

## Serialized Descriptor Parity

The parity gate compares a normalized package view with the copied
construction-free Runtime record for the same type. It must compare:

- type ID and `ElementKind`;
- ordered actions, including IDs, labels, and groups;
- ordered parameter groups;
- ordered parameter IDs and kinds;
- defaults, ranges, steps, units, visibility, and descriptions;
- inline options and dynamic option-source declarations;
- quick-access ordering;
- aliases and deprecation metadata;
- declared binding mode where applicable.

Normalization may account only for explicitly documented JSON/C++ vocabulary
differences. It must not discard unknown fields, reorder declared arrays, round
values loosely, or accept a package/runtime mismatch because live setup happens
to succeed.

A mismatch fails before creator invocation, prefix reservation, parameter
publication, preset application, mapping preview, or scene mutation. Diagnostics
must name the package, type, JSON path, Runtime field, expected value, and
observed value.

## Schema Evolution And Compatibility

Element Package v1 readers:

- accept exactly schema v1;
- reject missing, malformed, or future schema versions unless a separately
  documented legacy input exists;
- do not rewrite the package during validation or activation;
- reject duplicate identities within the package and conflicts with the
  activation set;
- validate referenced preset, asset, test, and migration files before exposing
  the package as activatable;
- preserve unknown future package versions as unsupported input rather than
  silently interpreting them as v1.

The current `layer.package.json` field names are compatibility inputs. The
implementation may retain them in Element Package v1 or provide an explicit
normalizer, but it must not rename stable public identities or invalidate the
existing Signal Bloom fixture without a reviewed migration.

## Dependency And Capability Boundary

SEAC-7 declares and validates dependencies; SEAC-8 owns generated or otherwise
controlled registration.

Dependency records must distinguish:

- required versus optional;
- package, source, static-library, asset, and runtime-service requirements;
- stable dependency identity from local resolution path;
- compatible version/range from the version actually resolved.

Capabilities must be closed, documented values rather than free-form
permission claims. Machine-local selections such as physical MIDI ports,
webcams, displays, audio devices, and absolute content roots remain outside the
portable package.

Versioned inter-package graph rules must be deterministic. Missing required
dependencies, duplicate providers, incompatible versions, and cycles fail
preflight with stable diagnostics. Optional absence is reported without
changing the declared package surface.

## Confidence-Suite Integration

Extend the SEAC-6 JSON report rather than creating a competing report format.
For a package run, add:

- package schema and package version;
- normalized package identity;
- resolved compatibility and dependency records;
- serialized/runtime descriptor signatures;
- field-level parity result;
- migration path, if any;
- referenced asset, preset, mapping, and test inventory.

The canonical Signal Bloom command remains:

```powershell
python tools\run_element_confidence.py `
  --package docs\examples\layer_packages\signal_bloom\layer.package.json `
  --tier ci
```

Package validation and descriptor parity become required preflight checks for
that command. Existing lifecycle, graphics, reload, memory, timing, and host
integration evidence remain required and must not be weakened.

## Implementation Order

1. Inventory the draft package schema against `ElementTypeContract`, state
   ownership, and the SEAC-6 report; record every missing or duplicate field.
2. Freeze the Element Package v1 schema, normalized in-memory model, version
   reader, ID grammar, path rules, compatibility ranges, capability vocabulary,
   dependency graph, and migration grammar.
3. Upgrade Signal Bloom and its referenced fixtures without changing its stable
   IDs or effective reviewed defaults.
4. Implement pure load/normalize/validate logic with stable JSON-path
   diagnostics and no Runtime mutation.
5. Export or adapt the construction-free Runtime contract into the same
   normalized comparison model.
6. Add exact package/runtime parity and negative fixtures for field drift,
   duplicate IDs, unsafe paths, future versions, missing references,
   incompatible dependencies, and cycles.
7. Integrate package preflight and parity into the existing confidence runner
   and report.
8. Run the separate host/public-contract promotion ladder and record the
   evidence before handing off to SEAC-8.

## Required Validation

At minimum, run:

```powershell
python tools\validate_layer_packages.py --check
python tools\synaptome_layer.py check `
  docs\examples\layer_packages\signal_bloom\layer.package.json
python tools\validate_artist_sdk_example.py
python tools\validate_element_sdk_boundary.py
python tools\validate_configs.py --public-app

python tools\run_element_confidence.py --profile grid --tier ci
python tools\run_element_confidence.py `
  --package docs\examples\layer_packages\signal_bloom\layer.package.json `
  --tier ci

python tools\run_control_hub_flow.py --dual-screen-phase2
python tools\check_app_independence.py
```

Build the Release host through both the physical checkout and the supported
openFrameworks junction. Build at least one repository-backed native test
through the junction as well. This preserves the root-selection invariant
documented in
[`../build_env.md`](../build_env.md#troubleshooting-mixed-junction-and-physical-paths).

## Promotion Checklist

SEAC-7 is complete only when:

- Element Package v1 has one versioned schema and one canonical normalized
  model;
- Signal Bloom validates as the complete reference package;
- Grid remains a passing built-in comparison fixture;
- package/runtime descriptor parity passes before activation;
- every negative parity, version, dependency, duplicate, reference, migration,
  and path fixture fails with a stable diagnostic;
- package mapping declarations remain unapplied suggestions;
- portable packages reject machine-local state and unsafe paths;
- both canonical SEAC-6 confidence commands still pass;
- BrowserFlow, public configuration, SDK/runtime boundaries, app independence,
  and physical/junction Release builds pass;
- the handoff to SEAC-8 identifies a validated package/type record that can
  drive controlled registration without changing the package contract.

## Explicitly Deferred At This Gate

- generated registration and removal of handwritten aggregate/project-list
  edits: SEAC-8;
- transactional mapping preview/apply/edit/remove behavior: SEAC-9;
- default-on or automatic package/content discovery: SEAC-10;
- native binary module and ABI policy: SEAC-11;
- catalog-wide representative migration and public authoring closure: SEAC-12.

Closure note: SEAC-8 through SEAC-12 are now complete. Generated registration,
transactional mappings, default-off controlled discovery, Native Module Policy
v1, representative bind-only migration, and the public authoring guide are
maintained by their final contracts and closure evidence rather than this
earlier package handoff.

Stop instead of widening the schema if parity requires constructing an element,
if portable metadata needs machine-local state, if package validation mutates
operator state, if registration mechanics leak into the serialized contract,
or if an existing scene/mapping identity would change without an explicit
migration.

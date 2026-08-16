# Representative Element Migration v1: SEAC-12 Execution Handoff

Status: Complete

Owner task: SEAC-12

Primary roadmap:
[`../project_ops/completed/spine_element_architecture_convergence.md`](../project_ops/completed/spine_element_architecture_convergence.md)

Predecessor decision:
[`Native Module Policy v1`](native_module_policy_v1.md) (completed through the
[`SEAC-11 handoff`](native_module_decision_v1_handoff.md))

## Goal

Close the spine/element architecture milestone by migrating three deliberately
different built-in element types to the authoritative bind-only path, proving
their complete lifecycle and persistence behavior, and publishing a public
authoring guide that another artist can follow without private host knowledge.

This is not a catalog-wide rewrite. It is a representative proof that the
spine supports simple visuals, content-backed elements, and complex persistent
simulations through one honest contract.

## Reference Set

| Reference | Role | Why selected | Required proof |
| --- | --- | --- | --- |
| `GridLayer` / `grid` | Simple generative visual | Existing confidence profile, deterministic update/draw surface, broad parameter set, and no content dependency. | Replace `LegacySetupAdapter` registration with explicit `ParameterBinder` storage; preserve every public ID/default/range and the Grid confidence baseline. |
| `StlModelLayer` / generated tetrahedron | Content-backed element | Exercises bounded file content, data-only discovery, exact precompiled type availability, local provenance, and missing-content handling. | Bind declared storage explicitly; activate and restore the tetrahedron through its generated-content identity; prove missing/changed content fails without disturbing active state. |
| `LeniaLayer` / Lenia and Circuit Lenia | Complex persistent simulation | Deterministic seed/reseed lifecycle, shared simulation with two presentations, transport interaction, GPU resources, and nontrivial scene state. | Bind declared storage explicitly; prove deterministic state signatures, organic/circuit presentation preservation, scene round-trip, teardown/reload, and performance containment. |

Signal Bloom remains the already-migrated bind-only Element Package v1 and
generated-registration authoring example. It is a carried-forward control, not
one of the three migrations.

## Frozen Compatibility Surface

Do not change without an explicit migration fixture and release note:

- type IDs: `grid`, `stlModel`, and `lenia`;
- every existing asset ID, definition ID, registry prefix, parameter suffix,
  kind, range, default, option, alias, action, and catalog path;
- `console.layer{slot}` addressing and eight-slot composition semantics;
- Lenia/Circuit Lenia shared simulation identity and distinct presentation;
- generated tetrahedron content/template identity and signature rules;
- scene, preset, mapping-bank, machine-profile, and preference ownership;
- slot opacity ownership and active composition transaction boundaries.

Metadata cleanup is not permission to rename a public target.

## Target Implementation Shape

For each reference type:

```text
authoritative static declaration
  -> configure definition/content defaults into owned storage
  -> explicit ParameterBinder binding only
  -> Runtime exact declaration/binding parity
  -> setup resources without redeclaring parameter metadata
  -> deterministic update/draw/release
```

After the three migrations, the declared compatibility-adapter count should
fall from 22 to 19. Do not migrate unrelated types merely to reduce that
number.

## Execution Slices

### SEAC-12A — Baseline And Fixture Freeze

- Inventory the three Runtime descriptors, declarations, catalog definitions,
  saved-scene targets, presets, mappings, actions, and confidence evidence.
- Record current visual/performance baselines before changing setup/binding.
- Add focused negative fixtures for missing/extra/wrong-kind bindings and
  content loss where coverage is absent.
- Prove all current public gates pass before migration.

### SEAC-12B — Grid Bind-Only Migration

- Move all Grid parameter registration to explicit declaration-backed storage
  binding.
- Keep setup responsible only for graphics/resource preparation.
- Preserve configured values across prepare/adopt, reload, and scene restore.
- Run the existing Grid confidence profile and host integration tests.

### SEAC-12C — STL Content-Backed Migration

- Move `StlModelLayer` to explicit declaration-backed storage binding.
- Keep content-path resolution and mesh loading outside parameter declaration.
- Add an STL confidence profile or equivalent focused native bench covering
  valid ASCII/binary intake as applicable, missing content, deterministic
  normalization, draw containment, teardown/reload, and cache behavior.
- Prove generated tetrahedron discovery, activation, Console assignment, scene
  save/restore, removal/unavailability, and failed-refresh isolation.
- Do not generalize this slice into arbitrary media-folder discovery.

### SEAC-12D — Lenia Stateful Migration

- Move `LeniaLayer` to explicit declaration-backed storage binding.
- Preserve configured seed before setup and prevent binding from resetting
  definition, preset, Browser, or Scene values.
- Add a Lenia confidence profile or equivalent focused native bench covering
  deterministic state signatures, reseed action behavior, update/draw,
  graphics-state containment, resource release, and repeated reload.
- Exercise both organic Lenia and Circuit Lenia catalog definitions and prove
  presentation selection does not fork simulation identity or parameter
  surface.
- Prove scene round-trip restores the declared durable state without
  serializing transient GPU resources or live telemetry.

### SEAC-12E — Public Authoring Guide

Create `docs/element_authoring_guide.md` as the public, task-oriented entry
point. It must cover:

1. Choosing built-in, generated source package, data-only content, or external
   bridge strategy.
2. Wrapping a raw openFrameworks sketch as an element.
3. Stable type/definition/registry-prefix/parameter identity.
4. Lifecycle and service boundaries.
5. Static declarations and bind-only storage.
6. Element Package v1 authoring and controlled registration.
7. Definitions, assets, presets, and suggestion-only mappings.
8. Controlled discovery and explicit activation.
9. Scene round-trip and provenance expectations.
10. Confidence profiles, host validation, Release build, and live visual
    acceptance.
11. Compatibility, versioning, deprecation, and release checklist.
12. The v1 no-native-loader policy and future reversal gate.

The guide must link runnable commands and the Signal Bloom, tetrahedron, Grid,
and Lenia reference artifacts. It must not require reading `ofApp.cpp` or
private host implementation to complete the supported workflow.

### SEAC-12F — Architecture Closure

- Run the complete validation matrix below.
- Perform one live visual review of all three references in the real host.
- Verify representative scenes and mappings still resolve stable targets.
- Update Artist SDK, subsystem/system architecture, layer-system roadmap,
  contract index, Project Ops, and changelog status.
- Record remaining `LegacySetupAdapter` types as cleanup, not an unfinished
  architecture gate.
- Mark the 12-step spine/element convergence milestone complete.

## Required Test Matrix

### Static And Contract

```powershell
python tools\gen_builtin_element_contracts.py --check
python tools\gen_parameter_manifest.py --check
python tools\gen_parameter_manifest.py --include-packages --check
python tools\validate_parameter_targets.py --strict --contract-fixtures
python tools\validate_configs.py --public-app
python tools\validate_element_sdk_boundary.py
python tools\check_app_independence.py
```

### Discovery And Package

```powershell
python -m pytest tests\test_controlled_package_discovery_v1.py -q
python tools\generate_element_package_registrations.py --check
python tools\generated_layer_catalog_regression.py --check
python tools\layer_browser_inspection_payload.py --check
```

### Native Lifecycle And Confidence

- Build and run `RuntimeCoreTest`, `LayerPackageBench`, and `BrowserFlowTest`.
- Run the Grid complete confidence profile.
- Run the carried-forward Signal Bloom complete confidence profile.
- Add and run complete STL and Lenia profiles, or document equivalent focused
  benches that satisfy the same report contract and thresholds.
- Exercise at least 200 prepare/setup/update/draw/release cycles for each new
  profile with bounded memory growth and no graphics-state leakage.
- Compare frame-time and allocation evidence to reviewed pre-migration
  baselines; do not invent a weaker threshold to make a regression pass.

### Host And Persistence

- Load all three references into Console slots in one session.
- Edit representative parameters through Browser and at least one mapping
  ingress.
- Save and restore a scene containing all three definitions.
- Verify definition, preset, Browser, and Scene provenance remains distinct.
- Remove or invalidate the generated STL source while inactive and active;
  refresh must not replace or destroy the running instance.
- Restart the host and verify supported durable state restores without local
  path leakage into portable documents.

### Build And Visual

- Build `Release|x64` from the physical checkout and supported openFrameworks
  junction with zero errors.
- Run the app with representative Grid, tetrahedron, Lenia, and Circuit Lenia
  views.
- Confirm non-blank output, expected presentation, stable interaction, and no
  material performance regression against the reviewed baseline.
- Existing third-party warnings remain separately tracked; new first-party
  warnings introduced by the migration fail promotion.

## Stop Conditions

Stop promotion if:

- a stable public ID or persisted target changes without an explicit migration;
- bind-only setup overwrites configured, preset, Browser, or Scene values;
- one reference needs private host access or an element-specific `ofApp.cpp`
  branch;
- the STL slice broadens into unbounded folder/media discovery;
- Lenia deterministic signatures or organic/circuit presentation behavior
  drift unexpectedly;
- a failed content refresh, scene restore, or element preparation mutates the
  prior active composition;
- the isolated bench and real host exercise different lifecycle or descriptor
  paths;
- graphics state, resources, memory, or frame time regress beyond the reviewed
  confidence threshold;
- the authoring guide promises native loading, hot reload, or raw `ofApp`
  installation;
- the guide cannot be completed using only public docs, SDK headers, fixtures,
  and commands.

## Promotion Checklist

SEAC-12 and the architecture milestone are complete only when:

- [x] Grid uses explicit bind-only storage with unchanged public behavior.
- [x] StlModel uses explicit bind-only storage and the generated tetrahedron
      passes discovery, lifecycle, content-failure, and scene-round-trip gates.
- [x] Lenia uses explicit bind-only storage and both presentations pass
      deterministic lifecycle, reload, persistence, and visual gates.
- [x] Static declarations and every live catalog definition retain exact
      parameter parity.
- [x] Grid, Signal Bloom, STL, and Lenia confidence evidence passes.
- [x] Host integration, BrowserFlow, scene, mapping, and controlled-discovery
      gates pass.
- [x] Physical and junction Release builds pass with zero errors.
- [x] The public element authoring guide is complete and runnable without
      private host knowledge.
- [x] Remaining compatibility adapters and deferred show/device work are
      explicitly routed without weakening this gate.
- [x] The physical Release host stays running for the required live review.
      On 2026-08-16 the operator built and launched from Visual Studio and
      confirmed Grid, STL Tetra, organic Lenia, and Circuit Lenia load in the
      real host. The committed `geometry.stl_tetra` definition supplies the
      live tetrahedron presentation; generated-fixture discovery, activation,
      content failure, and rollback remain covered by the focused automated
      gates.
- [x] Project Ops and architecture roadmaps mark SEAC-12 and the 12-step
      convergence milestone complete.

Implementation and validation evidence is recorded in
[`../project_ops/reports/seac_12_representative_migration.md`](../project_ops/reports/seac_12_representative_migration.md).

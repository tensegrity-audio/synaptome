# Layer Package And Compatibility Bench Scaffolding

State Summary
- Phase: INTAKE
- Status: Draft
- Steps Complete: 3 / 10
- Progress: Draft package/preset schemas, shared package discovery roots, a Signal Bloom package fixture, package-derived snapshots, opt-in combined catalog/manifest checks, one STL/model generated-layer template fixture, static and dynamic option metadata fixtures, and a schema-checked read-only Browser inspection payload now exist without changing runtime loading behavior.
- Last Step Outcome: 2026-06-28 - Added one generated-layer `optionsSource` metadata fixture and proved it flows into generated catalog plus inspection snapshots without adding provider lookup, Browser UI, or runtime scanning.
- Next Step: Expand only in tiny, schema-checked slices, such as one package-owned option metadata example or one additional generated content kind, then consider read-only Browser UI behind an explicit draft path.
- Dependencies / Overlap: `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`, `docs/contracts/parameter_manifest.json`, `docs/schemas/layer_asset.schema.json`, `tools/layer_catalog_regression.py`, `tools/gen_parameter_manifest.py`.
- Primary Scope: contracts
- Secondary Scopes: artist-sdk, docs, tests, runtime
- Blocking Issues / Unknowns: Runtime bench shape depends on an app-native or offscreen layer test seam; generated registration/module loading remains a later architecture decision; beat-reactive mapping presets depend on the separate transport/reactivity contract.
- Resume From: Intake; keep the read-only inspection payload passing, then add the smallest next metadata expansion without touching Browser/runtime behavior.

## Current State In Plain English

This ticket has moved the layer package idea from a roadmap note into a checked
draft contract. We can now point tooling at a package folder, validate the
package, generate package-derived catalog and parameter outputs, and compare
those draft outputs beside the current runtime catalog and manifest.

The app still does not load packages at runtime. Existing `*.layer.json` layer
assets remain the canonical runtime catalog. The new package outputs are
deliberately draft-only unless a command is run with `--include-packages`.

What is real:

- Draft package and preset schemas.
- One working Signal Bloom package fixture.
- Static package validation.
- Package-only catalog and parameter manifest snapshots.
- Shared package discovery roots.
- Opt-in combined package/runtime catalog and parameter manifest checks.
- One static STL/model generated-layer template fixture with standard model
  parameters and zero legacy asset ID conflicts.
- One schema-checked read-only Browser inspection payload combining package and
  generated-layer fixture metadata.
- One generated-layer static option fixture (`materialMode`) that flows through
  the generated catalog and inspection payload.
- One generated-layer dynamic option-source fixture (`materialPreset`) that
  names a future runtime provider without resolving it.

What is not real yet:

- Runtime scanning of `synaptome/bin/data/layer_packages`.
- Browser use of package-discovered catalog entries.
- Runtime or Browser scanning of STL/model/media folders.
- Read-only Browser UI for package/generated-layer inspection.
- Package preset-bank controls in the Browser.
- Package mapping preset activation in the mapping surface.
- Runtime/offscreen single-layer bench.

Risk profile:

- Low risk now, because the work is mostly schemas, fixtures, and tools.
- Low risk for the inspection payload itself, because it is read-only fixture
  output and schema validation prevents it from claiming runtime loading,
  instantiation, or scene mutation.
- Medium risk for broader file-backed/generated-layer expansion, because IDs
  and folder rules must stay stable.
- Medium-high risk when package data starts feeding Browser state, scenes,
  mappings, or the canonical parameter manifest.
- High risk only when package loading/generated registration touches runtime
  installation and C++ registration behavior.

## Safety-First Delivery Rules

- Default runtime behavior remains canonical until a replacement path has
  schema coverage, fixtures, package-only snapshots, and combined compatibility
  snapshots.
- `--include-packages` remains a draft compatibility mode until Browser/runtime
  loading has its own staged tests.
- Every new generated ID must be deterministic and independent of absolute
  local paths.
- Public parameter IDs, layer asset IDs, scene targets, and mapping targets must
  never be renamed without an alias or migration plan.
- Browser work starts read-only, then opt-in, then default only after scene and
  mapping validation pass.
- Package OSC/audio/control defaults must remain visible mapping rows, not
  hidden parameter side effects.
- Runtime package loading, generated registration, or loader/plugin behavior
  waits until static checks, combined checks, Browser inspection, and a
  single-layer bench are in place.

## Next Baby Steps

1. Done: draft a generated-layer template shape for dropped STL/model files.
2. Done: add one tiny docs/example content fixture and optional sidecar
   metadata.
3. Done: generate a catalog-style snapshot for that fixture.
4. Done: prove no generated ID conflicts with legacy layer assets.
5. Done: define a read-only inspection payload that combines package and
   generated content metadata without adding Browser code.
6. Done: add a schema for that inspection payload and validate the snapshot
   against it.
7. Done: add one static generated-layer option fixture and prove it reaches the
   generated catalog plus inspection payload.
8. Done: add one generated-layer `optionsSource` fixture and prove it reaches
   the generated catalog plus inspection payload.
9. Next: add only tiny metadata expansions, such as one package-owned option
   metadata fixture or one more generated content kind, while keeping the
   inspection payload deterministic.
10. Then: only after that payload is stable, consider read-only Browser
   inspection behind an explicit draft path.

## Milestone Synthesis

- Milestone ID: layer-package-compat-bench
- Goal: Make Synaptome layer packages and template-backed content folders declared, validated, discoverable, preset-aware, mapping-aware, and eventually testable in isolation without touching `ofApp.cpp`.
- Non-Goals: Do not promise hot-loaded plugins, binary module loading, or no-source-edit installation before generated registration or package loading is implemented.
- Owner: Project Ops / Synaptome runtime.

## Roadmap Overlap Review

- Existing roadmap entries checked: `docs/project_ops/roadmap.md`, `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`.
- Related active requests: N/A.
- Duplicate risk: Low.
- Merge / split decision: Promote the previous backlog line "Layer Package And File-Driven Browser Organization" into this request; keep the layer standards doc author-facing and place detailed architecture debt in the layer-system roadmap.

## Complexity

- Level: High.
- Drivers: Package schema design, folder discovery, file-backed generated assets, legacy asset coexistence, generated manifest changes, Browser dropdown behavior, OSC mapping presets, dynamic runtime providers, preset storage, and runtime/offscreen bench requirements.
- Confidence: Medium.

## Intake

- User Request: Split layer standards from architecture work; keep standards author-facing, move package/SDK/loading/test-bench architecture into roadmaps, keep `contract_gaps.md` as an index, and open a Project Ops request for layer package scaffolding.
- Context: Synaptome has validated layer catalog and artist SDK fixtures, but package metadata is not yet the source of truth and runtime registration still requires source integration.
- Acceptance Signal: Package architecture is tracked in Project Ops, the layer-system roadmap owns the target architecture and gap detail, the contract gap index routes readers to the right roadmap, and future implementation can close gaps with schemas, fixtures, validators, Browser behavior, and runtime tests.

## Form

- Problem Statement: Layer authors need a stable package contract, and Synaptome needs validation scaffolding before packages, dropped content files, OSC mapping defaults, preset banks, and layer tests can become installable, inspectable, mappable, preset-aware, and testable in isolation.
- Constraints: Keep current source-registration language honest; do not add `contracts/README.md` entries until schemas and validators exist; preserve scene and mapping compatibility through stable IDs.
- Inputs Needed: Decide the first minimal package fixture and how it coexists with current `*.layer.json` assets.

## Analysis

- Touch Map: `docs/schemas/**`, `docs/contracts/**`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/project_ops/**`, `tools/gen_parameter_manifest.py`, `tools/layer_catalog_regression.py`, `LayerLibrary`, `LayerFactory`, Browser catalog discovery, Browser option rendering, OSC mapping surface, preset storage, and future runtime bench code.
- Risks: Prematurely claiming installable plugins; breaking legacy layer assets; duplicating parameter declarations between runtime and package metadata; generating unstable IDs for dropped files; hiding OSC defaults outside the mapping surface; making Browser option providers silently rewrite stored scene values.
- Alternatives Considered: Leave all guidance in the layer standards doc. Rejected because architecture and debt tracking need separate homes.

## Plan

- Steps:
  1. Add `layer_package.schema.json` and one minimal package fixture.
  2. Add folder discovery rules for explicit packages and template-backed content folders.
  3. Add a static package validator for IDs, required fields, referenced files, parameters, modes, presets, media, mapping presets, and tests.
  4. Add a file-backed generated-layer fixture, such as an STL file that becomes a Browser-visible model layer through a standard template.
  5. Extend package parameter declarations with static `options[]` and `optionsSource` metadata.
  6. Extend parameter manifest generation and layer catalog regression to consume package metadata.
  7. Add package OSC mapping preset metadata and validation for suffix expansion, OSC pattern/range/smoothing/deadband/blend fields, and conflict policy.
  8. Add `layer_preset.schema.json`, bundled preset banks, fixtures, and validation.
  9. Add Browser support for static dropdown options, runtime-owned dynamic option providers, mapping preset activation, and preset-bank toggling.
  10. Add a single-layer static validation command, then a runtime/offscreen bench when the test seam exists.
- Validation Plan: Start with schema and fixture validators; keep `python tools\gen_parameter_manifest.py --check`, `python tools\layer_catalog_regression.py --check`, package-only checks, combined `--include-packages` checks, and `python tools\validate_configs.py --public-app` passing as the package surface expands.
- Rollback / Stop Conditions: Stop if the package shape requires scene ID migration without a compatibility path, if generated IDs depend on absolute paths, if combined checks find package/runtime ID conflicts, or if docs imply no-source-edit installation before registration/loading support exists.

## Task Graph

| Task ID | Description | Status |
| --- | --- | --- |
| LPB-1 | Add package schema and minimal fixture. | Done |
| LPB-2 | Add folder discovery rules for explicit packages and template-backed content folders. | In Progress |
| LPB-3 | Add static package validator. | Done |
| LPB-4 | Add file-backed generated-layer fixture, starting with an STL/model drop template. | Done |
| LPB-5 | Add package parameter declarations, static options, and dynamic option source metadata. | In Progress |
| LPB-6 | Generate manifest/catalog entries from package metadata. | In Progress |
| LPB-7 | Add package OSC mapping preset metadata and validation. | Planned |
| LPB-8 | Add layer preset schema, preset banks, fixtures, and validation. | Planned |
| LPB-9 | Add Browser dropdown rendering, dynamic option providers, mapping preset activation, and preset-bank toggling. | Planned |
| LPB-10 | Add static package command and later runtime/offscreen single-layer bench. | Planned |

## Execution

- 2026-06-25 - Opened request and linked it to Artist SDK architecture and contract gap tracking.
- 2026-06-25 - Expanded scope for folder-discovered packages, file-backed generated layers, visible OSC mapping presets, and preset banks.
- 2026-06-25 - Split beat/transport into `docs/architecture/synaptome_transport_reactivity.md`; kept this request focused on the layer-system roadmap.
- 2026-06-26 - Added first package/preset schemas, Signal Bloom package fixture, preset bank, visible OSC mapping preset, bench metadata fixture, and static package validator.
- 2026-06-26 - Added package-derived catalog and parameter manifest regression tools with golden snapshots under `tools/testdata/layer_packages/`.
- 2026-06-26 - Added shared package discovery roots plus opt-in combined catalog/parameter manifest checks using `--include-packages`.
- 2026-06-26 - Added draft generated-layer template/sidecar schemas, an STL docs/example fixture, generated catalog snapshot, and legacy-ID conflict validation.
- 2026-06-26 - Added a read-only Browser inspection payload, golden snapshot, and schema validation for package/generated-layer fixture metadata without Browser/runtime behavior changes.
- 2026-06-26 - Narrowed the generated-layer snapshot so it still checks legacy ID conflicts but no longer fails when unrelated runtime layer asset counts change.
- 2026-06-26 - Added `materialMode` static option metadata to the generated STL template and proved it flows into generated catalog and inspection snapshots.
- 2026-06-28 - Added `materialPreset` dynamic `optionsSource` metadata to the generated STL template and proved it flows into generated catalog and inspection snapshots without provider lookup or Browser UI.

## Validation

- Passed: `python tools\validate_layer_packages.py --check`
- Passed: `python tools\layer_package_catalog_regression.py --check`
- Passed: `python tools\layer_package_parameter_manifest.py --check`
- Passed: `python tools\generated_layer_catalog_regression.py --check`
- Passed: `python tools\layer_browser_inspection_payload.py --check`
- Passed: `python tools\validate_configs.py tools\testdata\layer_browser_inspection\expected_layer_browser_inspection_payload.json`
- Passed: `python tools\validate_configs.py docs\examples\generated_layers\stl_models\generated_layer.template.json docs\examples\generated_layers\stl_models\tetrahedron.generated_layer.json docs\examples\layer_packages\signal_bloom\layer.package.json docs\examples\layer_packages\signal_bloom\presets\default.json docs\examples\layer_packages\signal_bloom\presets\bright.json docs\examples\layer_packages\signal_bloom\presets\calm.json tools\testdata\layer_browser_inspection\expected_layer_browser_inspection_payload.json`
- Current dirty-worktree caveat: `python tools\layer_catalog_regression.py --include-packages --check` and `python tools\gen_parameter_manifest.py --include-packages --check` are implemented but currently fail because unrelated runtime layer/catalog/manifest edits have not been promoted to their snapshots. Do not refresh those runtime-dependent snapshots as part of this non-runtime slice.
- Not Run: Runtime/offscreen bench; no runtime layer package loader or bench seam exists yet.
- Manual Evidence: `validate_layer_packages.py` checks the package fixture, parameter declarations, preset values, mapping preset targets, source registration file references, and bench metadata reference. The package catalog and package parameter tools prove the same fixture can produce stable Browser-like catalog output and package-scoped parameter IDs. The generated-layer tool proves one STL fixture becomes stable catalog-style metadata with static and dynamic option metadata and without legacy ID conflicts. The inspection payload tool proves package and generated metadata can be combined for future Browser inspection without runtime loading, layer instantiation, scene mutation, provider lookup, or canonical manifest changes.

## Doc Sync

- Roadmap updated: Yes.
- Changelog updated: No.
- Related docs updated: Yes.
- Links checked: Manual relative-path review.

## Post-Mortem

- Lessons: Keep author standards, architecture targets, gap tracking, and stable contract indexes separate so each document can be stricter about its own job.
- Follow-ups: Add schemas/fixtures before promoting package contracts into `docs/contracts/README.md`.

## Notes

- This request intentionally preserves source registration as the honest first public SDK path.

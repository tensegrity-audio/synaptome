# Layer Package And Compatibility Bench Scaffolding

State Summary
- Phase: INTAKE
- Status: Draft
- Steps Complete: 2 / 10
- Progress: Draft package/preset schemas, shared package discovery roots, a Signal Bloom package fixture, static validation, package-derived snapshots, and opt-in combined catalog/manifest checks now exist without changing runtime loading behavior.
- Last Step Outcome: 2026-06-26 - Added `tools/layer_package_discovery.py`, `--include-packages` paths for catalog/parameter manifest generators, and combined golden snapshots for package/runtime compatibility checks.
- Next Step: Define the file-backed generated-layer template shape first, then add one docs/example STL/model fixture and generated snapshot without changing Browser/runtime loading.
- Dependencies / Overlap: `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`, `docs/contracts/parameter_manifest.json`, `docs/schemas/layer_asset.schema.json`, `tools/layer_catalog_regression.py`, `tools/gen_parameter_manifest.py`.
- Primary Scope: contracts
- Secondary Scopes: artist-sdk, docs, tests, runtime
- Blocking Issues / Unknowns: Runtime bench shape depends on an app-native or offscreen layer test seam; generated registration/module loading remains a later architecture decision; beat-reactive mapping presets depend on the separate transport/reactivity contract.
- Resume From: Intake; start the file-backed generated-layer template fixture while keeping package/runtime combined checks draft-only.

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

What is not real yet:

- Runtime scanning of `synaptome/bin/data/layer_packages`.
- Browser use of package-discovered catalog entries.
- Automatic STL/model/media folder drops.
- Package preset-bank controls in the Browser.
- Package mapping preset activation in the mapping surface.
- Runtime/offscreen single-layer bench.

Risk profile:

- Low risk now, because the work is mostly schemas, fixtures, and tools.
- Medium risk for the next file-backed/generated-layer fixture, because IDs and
  folder rules must stay stable.
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

1. Draft a generated-layer template shape for dropped STL/model files.
2. Add one tiny docs/example content fixture and optional sidecar metadata.
3. Generate a package-only/catalog-style snapshot for that fixture.
4. Add a combined compatibility snapshot proving no ID conflicts with legacy
   layer assets.
5. Keep the Browser and runtime untouched.
6. Only after those checks pass, add read-only Browser inspection of generated
   package/content metadata.

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
| LPB-4 | Add file-backed generated-layer fixture, starting with an STL/model drop template. | Planned |
| LPB-5 | Add package parameter declarations, static options, and dynamic option source metadata. | Planned |
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

## Validation

- Passed: `python tools\validate_layer_packages.py --check`
- Passed: `python tools\layer_package_catalog_regression.py --check`
- Passed: `python tools\layer_package_parameter_manifest.py --check`
- Passed: `python tools\layer_catalog_regression.py --include-packages --check`
- Passed: `python tools\gen_parameter_manifest.py --include-packages --check`
- Not Run: Runtime/offscreen bench; no runtime layer package loader or bench seam exists yet.
- Manual Evidence: `validate_layer_packages.py` checks the package fixture, parameter declarations, preset values, mapping preset targets, source registration file references, and bench metadata reference. The package catalog and package parameter tools prove the same fixture can produce stable Browser-like catalog output and package-scoped parameter IDs. The opt-in combined checks prove those draft package outputs can sit beside the current runtime catalog/manifest without ID conflicts.

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

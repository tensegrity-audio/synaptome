# Layer Package And Compatibility Bench Scaffolding

State Summary
- Request ID: layer_package_compatibility_bench_scaffolding
- Phase: EXECUTION
- Status: Paused behind show and spine contract
- Steps Complete: 8 / 10 (2 additional steps in progress)
- Progress: The Browser now provides reusable labeled dropdowns on matching live package parameters for both static `options[]` and registered `optionsSource` choices, alongside the operator-local package preset-bank picker. Selections update the existing live/base parameter value, while unavailable provider values are preserved until the operator explicitly replaces them.
- Last Step Outcome: 2026-07-24 - Added reusable live labeled parameter selection, including `Half Time` / `Normal` / `Double Time` for BPM and `Compact` / `Default` / `Full` for Scale, with provider-revision cancellation, unavailable-value preservation, telemetry, and scene/mapping/inspection isolation coverage.
- Next Step: After the show-safe checkpoint, rebaseline the package schema and Browser mapping work against the canonical element contract and SEAC-7/SEAC-9; do not resume mapping UI independently.
- Dependencies / Overlap: `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`, `docs/contracts/parameter_manifest.json`, `docs/schemas/layer_asset.schema.json`, `tools/layer_catalog_regression.py`, `tools/gen_parameter_manifest.py`.
- Primary Scope: contracts
- Secondary Scopes: artist-sdk, docs, tests, runtime
- Blocking Issues / Unknowns: Generated registration/module loading remains a later architecture decision; mapping-preset editing still needs explicit Browser ownership; beat-reactive mappings depend on the separate transport/reactivity contract.
- Impact / Priority Notes: Establishes deterministic package and generated-content contracts before more tracked media, Browser activation, or runtime discovery increases compatibility risk.
- Priority Score: N/A
- Priority Lane: Deferred
- Ready State: Ready
- Ready Gate: Met for read-only inspection, vetted opt-in source registration, operator-local next-load preset selection, and explicit labeled live parameter selection; automatic discovery and mapping activation remain out of scope.
- Project Ops / Roadmap Updates (timestamped): 2026-06-25 - Promoted layer package scaffolding from backlog. 2026-07-18 - Reconciled the request with the current Project Ops readiness contract and pre-media gate. 2026-07-18 - Completed the bounded package-owned static option slice and paused before runtime promotion. 2026-07-24 - Converged the reviewed runtime adapter, added package-owned dynamic option metadata and unavailable-value policy, completed the live-window smoke, rendered read-only option/provider state in the Browser, resolved the app-owned transport provider with missing-value preservation, added operator-local next-load preset-bank selection, and promoted static/runtime choices into explicit live labeled dropdowns. 2026-07-26 - Paused independent mapping UI work and subordinated package continuation to the canonical spine/element request.
- Resume From: Execution; define the preview/apply/rollback boundary for package mapping presets.

## Current State In Plain English

This ticket has moved the layer package idea from a roadmap note into a checked
draft contract. We can now point tooling at a package folder, validate the
package, generate package-derived catalog and parameter outputs, and compare
those draft outputs beside the current runtime catalog and manifest.

The app does not scan or dynamically load packages. Existing `*.json` layer
assets remain canonical. One vetted package can be promoted through an
explicit, disabled-by-default allowlist because its C++ source is compiled and
registered; this is not plugin loading.

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
- Browser consumption of that payload as non-editable inspection rows without
  layer instantiation or offline registry hydration.
- `python tools/synaptome_layer.py check <layer.package.json>` for focused
  author feedback.
- Disabled-by-default Signal Bloom activation with defaults -> preset ->
  activation override -> scene value precedence.
- Suggestion-only package mappings that activation never silently applies.
- A native stub-backed Signal Bloom lifecycle/draw-dispatch bench.
- One generated-layer static option fixture (`materialMode`) that flows through
  the generated catalog and inspection payload.
- One generated-layer dynamic option-source fixture (`materialPreset`) that
  names a future runtime provider without resolving it.
- One package-owned dynamic option-source fixture (`bpmMultiplier`) plus a
  machine-readable preserve-and-mark-unavailable policy.
- A generated Signal Bloom runtime adapter that removes hand-maintained
  package/default/preset/mapping duplication.
- A revisioned runtime option-provider registry whose app-owned
  `transport.bpmMultipliers` choices resolve in read-only Browser rows.
- Missing provider values remain marked and preserved while the inspection
  payload and stored defaults remain unchanged.
- Active packages expose ordered, labeled preset banks in the Browser.
- Preset selection writes stable bank/preset IDs to the ignored show-machine
  activation override and is applied when that layer is next instantiated.
- Native coverage proves preset selection does not mutate the inspection
  payload, current catalog entry, active scene, MIDI map, or OSC map.
- Matching live package parameters render their current option label instead of
  a raw value and open a shared dropdown for static or runtime-provided choices.
- Labeled selection commits through the existing parameter registry, so scenes,
  MIDI, OSC, and direct edits continue to share one value rather than parallel
  package-specific state.
- Provider revisions close stale pickers; removed current values stay visible
  as unavailable until an explicit valid selection replaces them.

What is not real yet:

- Runtime scanning of `synaptome/bin/data/layer_packages`.
- Automatic Browser/runtime package discovery.
- Runtime or Browser scanning of STL/model/media folders.
- Live preset application to an already-running layer; the current safe
  behavior applies a selected preset on the next layer load.
- Package mapping preset activation in the mapping surface.
- Generated registration or binary/plugin loading.

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
9. Done: add one package-owned option-metadata fixture while keeping the
   inspection payload deterministic and canonical runtime outputs unchanged.
10. Done in separate request: complete the pre-media safety gate in
    [`../roadmap.md`](../roadmap.md), including media roots, stable IDs,
    provenance, and the explicit-manifest-versus-folder-scan policy. Only after
    that gate is recorded should a separate request add more tracked media.
11. Done: expose the inspection payload in the Browser as read-only rows.
12. Done: add a focused package check command and single-layer lifecycle bench.
13. Done: add disabled-by-default source-registered activation and lock value
    plus mapping ownership semantics.
14. Done: generate the reviewed optional runtime adapter from package metadata.
15. Done: add one package-owned `optionsSource` fixture and preserve-on-
    unavailable payload policy.
16. Done: render static choices and unavailable dynamic-option state in the
    Browser without resolving providers or rewriting stored values.
17. Done: add an explicit provider registry and resolve
    `transport.bpmMultipliers` with unavailable-value coverage.
18. Done: add explicit labeled package preset-bank selection with operator-local
    persistence and next-load application while preserving the locked defaults
    -> preset -> explicit override -> scene precedence.
19. Done: make named static/runtime choices editable through a shared labeled
    dropdown while preserving unavailable values and active scene ownership.
20. Next: add an explicit mapping-preset preview/apply/edit flow; package mappings must
    remain suggestions until the operator acts.

## Milestone Synthesis

- Milestone ID: layer-package-compat-bench
- Milestone Name: Layer Package And Compatibility Bench Scaffolding
- Milestone Type: Contract and validation scaffolding
- Source Requests: layer_package_compatibility_bench_scaffolding
- Outcome Statement (Done When): Package and generated-content metadata can be validated and inspected deterministically, a static package command exists, and a stub-backed lifecycle/draw-dispatch bench is implemented without overstating real graphics evidence.
- KPI / Success Signal: Package-only, combined compatibility, inspection, canonical catalog/manifest, and public-app checks pass with stable IDs and no unintended runtime behavior change.
- Target Window: Before the next tracked generated-media or show-content request.
- Dependency Gates: Stable public IDs, deterministic fixture roots, current runtime catalog compatibility, and a documented media intake/discovery policy.
- Contract Surfaces: Layer packages, presets, generated-layer templates and sidecars, Browser inspection payload, layer catalog, parameter manifest, scenes, and mappings.
- Risk Posture: Low for schemas and fixtures; medium-high for Browser/canonical integration; high for runtime scanning or loading.
- Goal: Make Synaptome layer packages and template-backed content folders declared, validated, discoverable, preset-aware, mapping-aware, and eventually testable in isolation without touching `ofApp.cpp`.
- Non-Goals: Do not promise hot-loaded plugins, binary module loading, or no-source-edit installation before generated registration or package loading is implemented.
- Owner: Project Ops / Synaptome runtime.

## Roadmap Overlap Review

- Existing roadmap entries checked: `docs/project_ops/roadmap.md`, `docs/project_ops/synaptome_layer_design_standards.md`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/architecture/synaptome_artist_sdk.md`, `docs/contracts/contract_gaps.md`.
- Related active requests: N/A.
- Duplicate risk: Low.
- Merge / split decision: Promote the previous backlog line "Layer Package And File-Driven Browser Organization" into this request; keep the layer standards doc author-facing and place detailed architecture debt in the layer-system roadmap.
- Priority conflict: None; show-development roadmaps are parked until the pre-media gate is complete.

## Prioritization

- Policy Source: Explicit user request to clean roadmap state and reach a safe point before generating more media.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Due Date / Timing Driver: Complete the pre-media gate before the next tracked generated-media request.
- Sort Key: 2026-07-18-pre-media-safety
- Override: User-directed safety cleanup takes precedence over new show-content work.

## Definition Of Ready

- Ready State: Ready
- Ready Date: 2026-07-18
- Ready Owner: Codex / maintainer
- Ready Exceptions: Runtime package scanning, automatic Browser activation, preset application, and mapping activation are not implied by readiness for static contract slices.
- Decision Links: N/A; media discovery policy is complete, while preset and mapping ownership remain the next Browser boundaries.

## Complexity

- Level: High.
- Predicted Count: 12
- Count Drivers: 3 contract surfaces, 3 generated outputs, 2 runtime compatibility boundaries, 2 Browser/mapping boundaries, 1 media policy dependency, and 1 future test seam.
- Drivers: Package schema design, folder discovery, file-backed generated assets, legacy asset coexistence, generated manifest changes, Browser dropdown behavior, OSC mapping presets, dynamic runtime providers, preset storage, stub-backed lifecycle/draw-dispatch requirements, and future real-GL/pixel requirements.
- Confidence: Medium.

## Intake

- User Request: Split layer standards from architecture work; keep standards author-facing, move package/SDK/loading/test-bench architecture into roadmaps, keep `contract_gaps.md` as an index, and open a Project Ops request for layer package scaffolding.
- Context: Synaptome has validated layer catalog and artist SDK fixtures, but package metadata is not yet the source of truth and runtime registration still requires source integration.
- Acceptance Signal: Package architecture is tracked in Project Ops, the layer-system roadmap owns the target architecture and gap detail, the contract gap index routes readers to the right roadmap, and future implementation can close gaps with schemas, fixtures, validators, Browser behavior, and runtime tests.

## Form

- Problem Statement: Layer authors need a stable package contract, and Synaptome needs validation scaffolding before packages, dropped content files, OSC mapping defaults, preset banks, and layer tests can become installable, inspectable, mappable, preset-aware, and testable in isolation.
- User / Operational Value: Artists gain a predictable authoring and validation path while operators keep stable scenes, mappings, IDs, and canonical runtime behavior.
- Change Type: Contracts, fixtures, documentation, validation tooling, then separately promoted runtime integration.
- Execution Mode: Strict gated
- Acceptance Criteria: Deterministic package/generated-layer inspection, stable IDs, passing compatibility checks, explicit media intake policy, and no implied runtime loading before its promotion gates exist.
- Constraints: Keep current source-registration language honest; do not add `contracts/README.md` entries until schemas and validators exist; preserve scene and mapping compatibility through stable IDs.
- Must Not Change: Canonical runtime loading, public IDs, existing scene/mapping targets, or stored option values without an approved migration and runtime request.
- Allowed To Change: Draft schemas, fixtures, package-only and inspection snapshots, validators, and roadmap/request documentation within the bounded slice.
- Inputs Needed: Decide the first media discovery/intake policy before adding
  tracked media; current `*.layer.json` assets remain canonical until a
  separately approved promotion changes that rule.

## Analysis

- Touch Map: `docs/schemas/**`, `docs/contracts/**`, `docs/architecture/synaptome_layer_system_roadmap.md`, `docs/project_ops/**`, `tools/gen_parameter_manifest.py`, `tools/layer_catalog_regression.py`, `LayerLibrary`, `LayerFactory`, Browser catalog discovery, Browser option rendering, OSC mapping surface, preset storage, and future runtime bench code.
- Risks: Prematurely claiming installable plugins; breaking legacy layer assets; duplicating parameter declarations between runtime and package metadata; generating unstable IDs for dropped files; hiding OSC defaults outside the mapping surface; making Browser option providers silently rewrite stored scene values.
- Alternatives Considered: Leave all guidance in the layer standards doc. Rejected because architecture and debt tracking need separate homes.

## Design Alignment

- Guiding Principles Affected: Stable public contracts, explicit operator-visible mappings, deterministic validation, and honest extension boundaries.
- Systems / Elements / Processes Used: Layer package schemas, generated-layer fixtures, Browser inspection payload, Project Ops promotion gates, catalog/manifest regression, and public-app validation.
- Alignment Rationale: Static evidence must precede Browser or runtime behavior so authoring improvements do not destabilize scenes, mappings, or public IDs.
- Design Alignment Log Update: N/A; this request applies existing architecture principles and does not introduce a new design principle.
- Student-Facing Explanation: Describe packages as checked metadata plus source registration today, with runtime discovery and plugin loading clearly identified as future capabilities.

## Plan

- Steps: Follow the ordered task graph below; each promotion keeps runtime behavior canonical until its validation gate passes.
  1. Add `layer_package.schema.json` and one minimal package fixture.
  2. Add folder discovery rules for explicit packages and template-backed content folders.
  3. Add a static package validator for IDs, required fields, referenced files, parameters, modes, presets, media, mapping presets, and tests.
  4. Add a file-backed generated-layer fixture, such as an STL file that becomes a Browser-visible model layer through a standard template.
  5. Extend package parameter declarations with static `options[]` and `optionsSource` metadata.
  6. Extend parameter manifest generation and layer catalog regression to consume package metadata.
  7. Add package OSC mapping preset metadata and validation for suffix expansion, OSC pattern/range/smoothing/deadband/blend fields, and conflict policy.
  8. Add `layer_preset.schema.json`, bundled preset banks, fixtures, and validation.
  9. Add Browser support for static dropdown options, runtime-owned dynamic option providers, mapping preset activation, and preset-bank toggling.
  10. Add a single-layer static validation command and a stub-backed
      lifecycle/draw-dispatch bench; defer real-GL/pixel checks until that test
      seam exists.
- Validation Plan: Start with schema and fixture validators; keep `python tools\gen_parameter_manifest.py --check`, `python tools\layer_catalog_regression.py --check`, package-only checks, combined `--include-packages` checks, and `python tools\validate_configs.py --public-app` passing as the package surface expands.
- Rollback / Stop Conditions: Stop if the package shape requires scene ID migration without a compatibility path, if generated IDs depend on absolute paths, if combined checks find package/runtime ID conflicts, or if docs imply no-source-edit installation before registration/loading support exists.

## Task Graph

| Task ID | Description | Status |
| --- | --- | --- |
| LPB-1 | Add package schema and minimal fixture. | Done |
| LPB-2 | Add folder discovery rules for explicit packages and template-backed content folders. | In Progress |
| LPB-3 | Add static package validator. | Done |
| LPB-4 | Add file-backed generated-layer fixture, starting with an STL/model drop template. | Done |
| LPB-5 | Add package parameter declarations, static options, and dynamic option source metadata. | Done |
| LPB-6 | Generate manifest/catalog entries from package metadata. | Done |
| LPB-7 | Add package OSC mapping preset metadata and validation. | Done |
| LPB-8 | Add layer preset schema, preset banks, fixtures, and validation. | Done |
| LPB-9 | Add Browser dropdown rendering, dynamic option providers, mapping preset activation, and preset-bank toggling. | In Progress |
| LPB-10 | Add static package command and stub-backed single-layer lifecycle/draw-dispatch bench; keep real-GL/pixel evidence as follow-up. | Done |

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
- 2026-07-18 - Reconciled the request with its task graph, moved it from intake
  to execution, and linked the pre-media safety gate without expanding runtime
  scope.
- 2026-07-18 - Added `bpmMultiplier` named options owned by the Signal Bloom
  package and regenerated package catalog/manifest, combined compatibility,
  and read-only inspection snapshots.
- 2026-07-19 - Promoted the inspection payload into non-editable Browser rows,
  added the focused `synaptome-layer check` command, compiled Signal Bloom as
  an honest source-registration example behind a disabled allowlist, locked
  value/mapping merge behavior, and added native Browser plus layer-bench
  coverage.
- 2026-07-24 - Added `synaptome-layer runtime-adapter` so the reviewed optional
  runtime catalog is generated from Signal Bloom package/default/preset/mapping
  data; added a package-owned `transport.bpmMultipliers` option source and
  machine-readable preserve-on-unavailable policy; completed the live-window
  Signal Bloom/Aurora Veil smoke.
- 2026-07-24 - Rendered static choices and unresolved dynamic-provider state in
  read-only Browser inspection rows; added a native assertion that rebuilding
  the Browser model does not mutate the inspection payload or stored defaults.
- 2026-07-24 - Added a revisioned runtime option-provider registry, registered
  the app-owned transport BPM choices, resolved them in read-only Browser
  inspection, and proved a removed default remains marked and preserved.
- 2026-07-24 - Added an active-package preset-bank picker using labels backed by
  stable IDs. Selection persists in the ignored show-machine override and
  applies to the next layer instantiation; failed writes roll back and no
  current scene, inspection payload, or MIDI/OSC mapping is mutated.
- 2026-07-24 - Decorated matching live package parameters with reusable labeled
  choices from static metadata or registered runtime providers. Explicit
  selection updates the existing parameter value; unavailable values remain
  untouched, and provider revisions close stale pickers.

## Validation

- Passed: `python tools\validate_layer_packages.py --check`
- Passed: `python tools\layer_package_catalog_regression.py --check`
- Passed: `python tools\layer_package_parameter_manifest.py --check`
- Passed: `python tools\generated_layer_catalog_regression.py --check`
- Passed: `python tools\layer_browser_inspection_payload.py --check`
- Passed: `python tools\layer_catalog_regression.py --include-packages --check`
- Passed: `python tools\gen_parameter_manifest.py --include-packages --check`
- Passed: `python tools\validate_configs.py --public-app`
- Passed: `python ..\project_ops\tools\project_ops_audit.py --repo .`
- Passed: `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id layer_package_compatibility_bench_scaffolding`
- Passed: `python tools\validate_configs.py tools\testdata\layer_browser_inspection\expected_layer_browser_inspection_payload.json`
- Passed: `python tools\validate_configs.py docs\examples\generated_layers\stl_models\generated_layer.template.json docs\examples\generated_layers\stl_models\tetrahedron.generated_layer.json docs\examples\layer_packages\signal_bloom\layer.package.json docs\examples\layer_packages\signal_bloom\presets\default.json docs\examples\layer_packages\signal_bloom\presets\bright.json docs\examples\layer_packages\signal_bloom\presets\calm.json tools\testdata\layer_browser_inspection\expected_layer_browser_inspection_payload.json`
- Passed: `python tools\synaptome_layer.py check docs\examples\layer_packages\signal_bloom\layer.package.json`
- Passed: `python tools\synaptome_layer.py runtime-adapter docs\examples\layer_packages\signal_bloom\layer.package.json --output synaptome\bin\data\layers-optional\examples.signal_bloom.json --check`
- Passed: `LayerPackageBench.exe` (18 parameters, 240 updates, stub-backed draw
  dispatch; no pixel or real-GL claim).
- Passed: `BrowserFlowTest.exe` (19 scenarios, including read-only inspection,
  opt-in activation, and preset-bank selection/rollback/immutability).
- Passed: `BrowserFlowTest.exe` (20 scenarios, adding static/runtime labeled
  parameter selection, provider-revision cancellation, unavailable-value
  preservation, telemetry, and mapping/inspection immutability).
- Passed: Release x64 app build after the labeled-picker shared-header change
  (50.45 seconds) followed by an identical 2.21-second incremental build.
- Passed: Release x64 app build after the preset/Browser shared-header change
  (about 59 seconds) followed by an identical 2.55-second incremental build.
- Passed: Release x64 app build after the provider/Browser header change (44.82
  seconds) followed by an identical 1.92-second incremental build.
- Passed: Release x64 build (182.25-second one-time rebuild) followed by an
  identical 2.17-second incremental build.
- Manual Evidence: Signal Bloom and Aurora Veil both load in a live window.
- Manual Evidence: Inspection rows carry no runtime parameter pointers and leave offline layer storage empty. Activation is a no-op under the committed disabled config; an explicit test activation merges the bright preset and an overriding speed value while leaving the mapping preset unapplied.

## Doc Sync

- Roadmap updated: Yes.
- Changelog updated: Yes; added the 2026-07-18 roadmap/pre-media reconciliation entry.
- Related docs updated: Yes.
- Links checked: Yes; all relative Markdown targets in changed docs resolve.

## Post-Mortem

- Lessons: Keep author standards, architecture targets, gap tracking, and stable contract indexes separate so each document can be stricter about its own job.
- Follow-ups: Add schemas/fixtures before promoting package contracts into `docs/contracts/README.md`.

## Notes

- This request intentionally preserves source registration as the honest first public SDK path.

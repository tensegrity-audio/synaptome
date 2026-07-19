# Media Manifest Intake Contract

State Summary
- Request ID: media_manifest_intake_contract
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 5 / 5
- Progress: Synaptome now has a validated manifest-only, zero-asset media baseline with explicit roots, stable IDs, SHA-256, provenance, generated-media metadata, replacement rules, and negative fixtures.
- Last Step Outcome: 2026-07-18 - Removed the dangling default clip, added the media schema/policy/validator/fixtures, and passed the public-app contract suite with 12 validated contracts.
- Next Step: Open one bounded request for one reviewed redistributable asset; do not combine it with folder scanning, Browser activation, or package loading.
- Dependencies / Overlap: `docs/project_ops/roadmap.md`, `docs/architecture/synaptome_public_runtime_contract_roadmap.md`, `docs/contracts/media_catalog.md`, `docs/schemas/media_catalog.schema.json`, `synaptome/bin/data/config/videos.json`, `tools/media_catalog_regression.py`.
- Primary Scope: contracts
- Secondary Scopes: docs, tests, runtime
- Blocking Issues / Unknowns: None for safe manifest intake; the first actual asset still needs creative selection, redistribution review, and provenance.
- Impact / Priority Notes: Prevents undocumented or missing media from entering the public runtime and establishes a reproducible gate before generated media work resumes.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: Met; the request was limited to contract, validation, and empty-baseline changes with no media generation or folder scanning.
- Project Ops / Roadmap Updates (timestamped): 2026-07-18 - Promoted the pre-media policy gap, implemented manifest-only intake, validated it, and closed the request in one bounded cleanup pass.
- Resume From: Phase COMPLETE, State Complete, Next Action create a new request from `docs/project_ops/in_progress/_REQUEST_TEMPLATE.md` for one reviewed asset.

## Milestone Synthesis

- Milestone ID: media-manifest-intake
- Milestone Name: Safe Media Manifest Intake
- Milestone Type: Contract and validation
- Source Requests: media_manifest_intake_contract
- Outcome Statement (Done When): The committed media catalog can be empty safely, every future clip requires reproducible provenance and file integrity, invalid references are rejected, and no documentation implies folder scanning.
- KPI / Success Signal: Dedicated media validation and the public-app contract suite pass with zero dangling clips and 12 validated public contracts.
- Target Window: Before the next generated-media request.
- Dependency Gates: Existing `VideoCatalog` manifest behavior, public/local root policy, validation without optional dependencies, and Project Ops roadmap synchronization.
- Contract Surfaces: `videos.json`, media catalog schema/policy/example, source-control roots, public contract index, runtime architecture docs, and validation tooling.
- Risk Posture: Low; the runtime parser still consumes an explicit manifest and the committed catalog is empty.
- Goal: Make media generation safe to resume through explicit, provenance-aware manifest entries.
- Non-Goals: Generate media, implement folder scanning, add Browser UI, change `VideoCatalog` loading behavior, or promise slot-load evidence without an approved asset.
- Owner: Codex / maintainer.

## Roadmap Overlap Review

- Existing roadmap entries checked: `docs/project_ops/roadmap.md`, CG-08 in `docs/architecture/synaptome_public_runtime_contract_roadmap.md`, and media gaps in SDK/system/external architecture docs.
- Related active requests: layer_package_compatibility_bench_scaffolding.
- Duplicate risk: Low.
- Merge / split decision: Keep media intake separate from layer-package activation and future Browser/runtime discovery work.
- Priority conflict: None; parked show roadmaps remain parked until an asset request is explicitly promoted.

## Prioritization

- Policy Source: Explicit user request to reach a safe point before generating more media.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Due Date / Timing Driver: Before any new tracked generated-media work.
- Sort Key: 2026-07-18-pre-media-contract
- Override: User-directed safety work precedes content generation.

## Definition Of Ready

- Ready State: Ready
- Ready Date: 2026-07-18
- Ready Owner: Codex / maintainer
- Ready Exceptions: No binary media or native runtime build was required for an empty manifest contract.
- Decision Links: N/A; manifest-only was selected as the conservative policy matching current runtime behavior.

## Complexity

- Level: Medium
- Predicted Count: 8
- Count Drivers: Runtime config, schema, policy doc, example, negative fixtures, validator, contract index, and architecture/roadmap synchronization.
- Drivers: Path safety, file integrity, provenance, license/redistribution state, replacement semantics, optional Python dependencies, and runtime compatibility.
- Confidence: High

## Intake

- User Request: Make the documented pre-media cleanup and safety gate happen.
- Context: The prior catalog referenced a missing `default-loop.mp4`; discovery policy, roots, provenance, and replacement behavior were undecided.
- Acceptance Signal: An empty manifest is valid, the dangling entry is gone, future clips require hashes/provenance, rejection fixtures pass, and the public contract report is green.

## Form

- Problem Statement: Synaptome could validate the shape of `videos.json` but could not prove that committed media existed, was reproducible, was redistributable, or had safe replacement semantics.
- User / Operational Value: Artists can generate or import media later without polluting the repository or silently breaking Browser/runtime references.
- Change Type: Contract, validation, docs, fixture, and safe runtime configuration cleanup.
- Execution Mode: Strict gated
- Acceptance Criteria: Manifest-only policy, explicit roots, empty safe baseline, stable IDs, file hashes, provenance, replacement rules, negative fixtures, and green public validation.
- Constraints: Do not generate media, add folder scanning, or change the C++ media loader.
- Must Not Change: Runtime manifest loading model, existing public layer/parameter IDs, Browser activation state, or package loading behavior.
- Allowed To Change: `videos.json`, media schemas/docs/fixtures/validator, ignore rules, contract index, architecture status, and roadmap state.
- Inputs Needed: N/A

## Analysis

- Touch Map: Media config and `VideoCatalog` read behavior; schemas/examples; public contracts and fixtures; roadmap/architecture docs; extraction manifest; validation tools; `.gitignore`.
- Risks: Optional `jsonschema` dependency, accidentally requiring a nonexistent binary, committing operator-local media, unstable generated filenames, or implying folder scanning.
- Alternatives Considered: Generate a replacement for `default-loop`; rejected because its provenance and intended identity were unknown. Keep the dangling entry; rejected because it was not a safe baseline. Add folder scanning now; rejected as an unrelated runtime feature.

## Design Alignment

- Guiding Principles Affected: Honest runtime contracts, deterministic validation, stable public IDs, explicit ownership, and safe-by-default public examples.
- Systems / Elements / Processes Used: `VideoCatalog`, explicit JSON manifests, schema and semantic validators, public contract reporting, fixtures, and Project Ops promotion gates.
- Alignment Rationale: The runtime continues using its implemented manifest seam while future content gains reproducibility and compatibility evidence.
- Design Alignment Log Update: N/A; this applies established public-contract and safety principles.
- Student-Facing Explanation: A media file is not part of Synaptome merely because it exists in a folder; it becomes public only through a checked manifest entry that identifies the exact file and its origin.

## Plan

- Steps: Lock manifest-only policy; define schema and roots; replace the dangling catalog with an empty baseline; add semantic and negative validation; synchronize contracts/roadmaps and validate.
- Validation Plan: Run media, config, package, combined catalog/manifest, inspection, public-app, Project Ops, extraction, link, and diff checks.
- Rollback / Stop Conditions: Stop if the change requires a generated binary, C++ loader modification, runtime scanning, or a public ID migration.

## Task Graph

| Task ID | Description | Status |
| --- | --- | --- |
| media-manifest-intake-T1 | Select and document manifest-only discovery plus roots. | Complete |
| media-manifest-intake-T2 | Add schema, empty public example, and provenance/replacement rules. | Complete |
| media-manifest-intake-T3 | Remove the dangling catalog entry and ignore operator-local media. | Complete |
| media-manifest-intake-T4 | Add dependency-free semantic validation and negative fixtures. | Complete |
| media-manifest-intake-T5 | Synchronize public contracts/roadmaps and run validation. | Complete |

## Execution

- 2026-07-18 - Added package-owned named BPM multiplier options and regenerated draft package/inspection compatibility snapshots.
- 2026-07-18 - Replaced the missing `default-loop` catalog with a schema-versioned empty manifest-only baseline.
- 2026-07-18 - Added media schema, policy, example, negative fixtures, local-media ignore rule, and a validator that works without optional `jsonschema` installation.
- 2026-07-18 - Promoted media intake into the public contract report and synchronized architecture/roadmap documentation.

## Validation

- Passed: `python tools\media_catalog_regression.py --check`
- Passed: `python tools\validate_configs.py synaptome\bin\data\config\videos.json docs\examples\media_catalog_example.json`
- Passed: `python tools\validate_layer_packages.py --check`
- Passed: `python tools\layer_package_catalog_regression.py --check`
- Passed: `python tools\layer_package_parameter_manifest.py --check`
- Passed: `python tools\layer_catalog_regression.py --include-packages --check`
- Passed: `python tools\gen_parameter_manifest.py --include-packages --check`
- Passed: `python tools\layer_browser_inspection_payload.py --check`
- Passed: `python tools\validate_configs.py --public-app` (`validated=12`)
- Passed: `python tools\validate_synaptome_extraction_manifest.py --check --strict-review`
- Not Run: Native openFrameworks build/runtime playback; no binary media or C++ loader behavior changed.
- Manual Evidence: The canonical and public example catalogs contain zero clips/layers; negative fixtures reject duplicate IDs, missing generated provenance, dangling defaults, and absent media binaries.

## Doc Sync

- Roadmap updated: Yes
- Changelog updated: Yes
- Related docs updated: Yes
- Links checked: Yes

## Post-Mortem

- Lessons: An explicit empty catalog is a stronger public baseline than a plausible-looking reference to an absent asset. Contract validators should not require optional packages when a deterministic fallback is practical.
- Follow-ups: Add exactly one reviewed redistributable asset in a new request, then prove Browser visibility and runtime slot loading.

## Notes

- No media was generated or imported by this request.

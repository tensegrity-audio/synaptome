# Algorithmic Show Content

State Summary
- Request ID: algorithmic_show_content
- Phase: EXECUTION
- Status: In progress
- Steps Complete: 3 / 4
- Progress: Twelve compiled source packages, 36 catalog looks and six effect-look scenes implemented; portable validation and operator guide complete.
- Last Step Outcome: 2026-09-17 - Verified declarations, deterministic models, native adapters and recorded geometry; corrected reseed persistence before delivery.
- Next Step: Build Release/x64 on the show machine, rehearse planned combinations and confirm physical MIDI/audio/projector behavior.
- Dependencies / Overlap: Element SDK v1, source registration, parameter manifests, existing effect chain, show_readiness_operator_stability.
- Primary Scope: contracts
- Secondary Scopes: tests, docs
- Blocking Issues / Unknowns: Windows Release build, projector rendering and physical MIDI cannot run in this Linux workspace.
- Impact / Priority Notes: Content for the operator's next-day visuals show; preserve runtime behavior.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Ready State: Ready
- Ready Gate: User authorized additive content; existing source-package boundary supports implementation. Target-machine acceptance remains a final validation gate.
- Project Ops / Roadmap Updates (timestamped): 2026-09-17 - Promoted content creation for the next-day show without resuming architecture work.
- Resume From: Phase EXECUTION, State In progress, Next Action record target-machine acceptance using docs/show_content_guide.md.

## Milestone Synthesis

- Milestone ID: algorithmic_show_content
- Milestone Name: Algorithmic show content pack
- Milestone Type: feature
- Source Requests: algorithmic_show_content
- Outcome Statement (Done When): Twelve distinct programmatic models and six usable effect looks can be selected and controlled using existing interfaces, with validation evidence and an explicit target-machine handoff.
- KPI / Success Signal: 12 bound parameter contracts, deterministic finite models, 36 catalog looks, 6 effect scenes, no handwritten host/runtime edits.
- Target Window: 2026-09-18 show
- Dependency Gates: Package validation, generated registration, catalog/parameter validation, native content tests, Windows visual acceptance.
- Contract Surfaces: New show.* identities and parameters, ordinary layer definitions, package metadata and generated registration only.
- Risk Posture: Medium; bounded isolated content, but no target GPU or physical controller in this workspace.
- Goal: More playable algorithmic content for live scene composition.
- Non-Goals: Runtime refactoring, new effect processor architecture, native loading, device changes.
- Owner: Codex; operator owns target-machine acceptance.

## Roadmap Overlap Review

- Existing roadmap entries checked: Project Ops roadmap; biological, aurora, cosmic, layer-system and spine-element roadmaps; authoring standards.
- Related active requests: show_readiness_operator_stability
- Duplicate risk: Low
- Merge / split decision: Separate content delivery from runtime stability and architecture requests.
- Priority conflict: User explicitly prioritized show content over architecture.

## Prioritization

- Policy Source: User request
- Priority Score: N/A
- Priority Lane: Fast-Track
- Due Date / Timing Driver: 2026-09-18 show
- Sort Key: 2026-09-18-algorithmic-show-content
- Override: User prioritizes additive content; no runtime changes.

## Definition Of Ready

- Ready State: Ready
- Ready Date: 2026-09-17
- Ready Owner: Codex
- Ready Exceptions: No exception to final target-machine validation; report it pending.
- Decision Links: docs/element_authoring_guide.md; docs/project_ops/synaptome_layer_design_standards.md

## Complexity

- Level: High
- Predicted Count: 18
- Count Drivers: Twelve models and six effect looks.
- Drivers: Numerical stability, parameter consistency, additive integration, visual usefulness.
- Confidence: Medium

## Intake

- User Request: Create about twelve new algorithmic elements and six effects for tomorrow, following naming, parameters, MIDI/OSC and sound reactivity rules; do not change runtime.
- Context: Existing Lenia, river, cellular, low-poly and scientific content establishes the style.
- Acceptance Signal: Selectable, interesting, bounded content with working generic controls and honest validation.

## Form

- Problem Statement: The show needs new content within the existing architecture.
- User / Operational Value: More contrasting scenes and live control options.
- Change Type: Content, contracts, tests and docs.
- Execution Mode: Assisted
- Acceptance Criteria: Distinct models, correct bindings and defaults, visible mapping suggestions, bounded stepping, deterministic seeds, additive integration, documented show usage.
- Constraints: No runtime redesign, no host behavior change, no new C++ dependencies.
- Must Not Change: ofApp, Runtime, HostCompositionRenderer, effect processor implementation, MIDI/OSC routing, machine-local preferences.
- Allowed To Change: Content packages, ordinary catalog/scene files, generated registration/build records, metadata-generation tooling, tests, docs.
- Inputs Needed: Operator's Windows/GPU/MIDI smoke result after local validation.

## Analysis

- Touch Map: packages/showcase, layers/generative/showcase, layers/scenes/show-fx-*, tools, tests, generated records and documentation.
- Risks: Actual GPU behavior unavailable; existing effect install ignores defaults; existing scene loading can clear retired slot mappings.
- Alternatives Considered: New effect shaders require host changes. Existing effect settings therefore ship as saved look scenes using canonical processor IDs.

## Design Alignment

- Guiding Principles Affected: Model-first rendering, stable parameter ownership, visible generic mappings, deterministic instance state.
- Systems / Elements / Processes Used: Element Package v1, controlled source registration, ordinary catalog definitions and scenes.
- Alignment Rationale: Adds creative models without altering host ownership.
- Design Alignment Log Update: This request records the additive boundary and effect-look decision.
- Student-Facing Explanation: N/A

## Plan

- Steps: Audit documentation; implement content; validate native/static behavior; document and rehearse.
- Validation Plan: Package and registry checks, generated metadata freshness, native model and adapter tests, public contracts, portable-state gate and Windows handoff.
- Rollback / Stop Conditions: Keep changes isolated on content/algorithmic-show-pack; do not merge or claim show readiness without target-machine acceptance.

## Task Graph

| Task ID | Description | Status |
| --- | --- | --- |
| algorithmic_show_content-T1 | Audit docs, code and constraints | Complete |
| algorithmic_show_content-T2 | Implement isolated models and effect looks | Complete |
| algorithmic_show_content-T3 | Validate metadata, native behavior and integration | Complete |
| algorithmic_show_content-T4 | Record show guide and target-machine acceptance | In Progress |

## Execution

- 2026-09-17 - Read the repository documentation and current roadmaps. Existing public validation passes after installing test-only Python dependencies. Implemented the six saved effect-look scenes without changing effect dispatch.
- 2026-09-17 - Delivered twelve source-contained bind-only models, 236 declared parameters and 36 ordinary catalog looks. Common visible host-mic suggestions share source profiles and preserve a nonzero silent baseline. Generated registration uses the existing extension path.
- 2026-09-17 - Reviewed CPU captures of actual drawing geometry. Native checks caught and corrected a reseed persistence issue in Wave Tank, Chladni Plate and Phase Lattice; all actions now restart the declared seed without changing the persisted base.
- 2026-09-17 - Completed docs/show_content_guide.md and docs/show_effect_looks.md. Documented existing scene-load route retirement, per-processor effect ownership and machine-local preset activation limits without changing host behavior.

## Validation

- Passed: 24 public contracts; all twelve package declarations, 36 presets and twelve mapping suggestions; generated registration, catalog and parameter freshness; SDK boundary and portable-state checks.
- Passed: 61 Python tests plus two subtests; strict extraction, release metadata, built-in OSC patterns, app independence and this request's Project Ops audits.
- Passed: Twelve pure model suites and twelve real-adapter CPU suites, including deterministic replay, bounded stepping, finite visible geometry, parameter binding, transport/local pause, reseed persistence, slot opacity and graphics-state restoration. Existing LayerPackageBench remains green.
- Passed: AddressSanitizer and UndefinedBehaviorSanitizer runs of models and adapters; leak detection disabled because the container does not support it.
- Not Run: Windows Release build and hardware/projector acceptance, unavailable in Linux.
- Existing documentation mismatch: The contributing guide's literal `project_ops_compatibility` audit command targets a completed request without an active roadmap entry, including on the original branch. The repository audit and current `algorithmic_show_content` request audit both pass; no unrelated historical roadmap entry was recreated.
- Manual Evidence: Reviewed the twelve-panel contact sheet recorded from native adapter drawing geometry. These are CPU geometry/color captures, not OpenGL screenshots or a frame-rate benchmark.

## Doc Sync

- Roadmap updated: Yes
- Changelog updated: Yes
- Related docs updated: Yes
- Links checked: Repository-local guide links verified.

## Post-Mortem

- Lessons: Older documents contain superseded authoring patterns; follow canonical bind-only and mapping ownership contracts.
- Follow-ups: Windows show-machine smoke and physical controller check.

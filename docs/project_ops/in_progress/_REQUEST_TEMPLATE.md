# Synaptome Request Artifact Template

State Summary
- Request ID: <lowercase_filename_stem>
- Phase: INTAKE
- Status: Draft
- Steps Complete: 0 / 0
- Progress: <one-sentence current state>
- Last Step Outcome: <YYYY-MM-DD> - <what just happened>
- Next Step: <specific next action>
- Dependencies / Overlap: <related docs, requests, code areas, or N/A>
- Primary Scope: <runtime | contracts | docs | release | tests | governance>
- Secondary Scopes: <configured scope labels or N/A>
- Blocking Issues / Unknowns: <none or list>
- Impact / Priority Notes: <why this matters or N/A>
- Priority Score: <integer or N/A>
- Priority Lane: <Fast-Track | Standard | Review | Deferred | N/A>
- Ready State: Not Ready
- Ready Gate: <one-line blocker, exception, or met>
- Project Ops / Roadmap Updates (timestamped): <YYYY-MM-DD> - <state sync note>
- Resume From: Phase INTAKE, State Draft, Next Action <command or artifact>

## Milestone Synthesis

- Milestone ID: <stable_id>
- Milestone Name: <human-readable name>
- Milestone Type: <feature | contract | docs | validation | governance | other>
- Source Requests: <request IDs or this request ID>
- Outcome Statement (Done When): <observable completion condition>
- KPI / Success Signal: <measurable evidence>
- Target Window: <date, release, event, or explicit dependency window>
- Dependency Gates: <required decisions, contracts, tests, or N/A>
- Contract Surfaces: <public IDs, schemas, files, APIs, or N/A>
- Risk Posture: <low, medium, high, and why>
- Goal: <outcome>
- Non-Goals: <what is intentionally out of scope>
- Owner: <human, agent, team, or N/A>

## Roadmap Overlap Review

- Existing roadmap entries checked: <paths or headings>
- Related active requests: <IDs or N/A>
- Duplicate risk: <Low | Medium | High>
- Merge / split decision: <decision>
- Priority conflict: <conflict or None>

## Prioritization

- Policy Source: <user request, roadmap policy, release need, incident, or other>
- Priority Score: <integer or N/A>
- Priority Lane: <Fast-Track | Standard | Review | Deferred | N/A>
- Due Date / Timing Driver: <date, event, dependency, or N/A>
- Sort Key: <stable ordering key>
- Override: <override reason or N/A>

## Definition Of Ready

- Ready State: Not Ready
- Ready Date: <YYYY-MM-DD or N/A>
- Ready Owner: <human, agent, team, or N/A>
- Ready Exceptions: <accepted exceptions or N/A>
- Decision Links: <RFC/decision IDs or N/A>

Set both Ready State fields to `Ready` only after every required field is
resolved and decision blockers are closed or explicitly excepted. A request
must not enter `EXECUTION` before then.

## Complexity

- Level: <Low | Medium | High>
- Predicted Count: <derived surface/step count>
- Count Drivers: <what contributes to the count>
- Drivers: <risk, scope, validation, dependencies>
- Confidence: <Low | Medium | High>

## Intake

- User Request: <verbatim or summarized request>
- Context: <relevant background>
- Acceptance Signal: <how we will know this is done>

## Form

- Problem Statement: <what needs to change>
- User / Operational Value: <why the change matters>
- Change Type: <docs, contract, runtime, validation, release, governance, or mix>
- Execution Mode: <Strict gated | Assisted | Audit-only | Autonomous docs>
- Acceptance Criteria: <observable conditions>
- Constraints: <technical, product, privacy, or timing constraints>
- Must Not Change: <protected surfaces>
- Allowed To Change: <authorized surfaces>
- Inputs Needed: <unknowns or N/A>

## Analysis

- Touch Map: <files, docs, tools, systems, public interfaces, privacy paths>
- Risks: <risks or N/A>
- Alternatives Considered: <alternatives or N/A>

## Design Alignment

- Guiding Principles Affected: <principles or N/A>
- Systems / Elements / Processes Used: <architecture surfaces or N/A>
- Alignment Rationale: <why this direction fits>
- Design Alignment Log Update: <entry ID/update or N/A>
- Student-Facing Explanation: <plain-language explanation or N/A>

## Plan

- Steps: <ordered implementation or documentation steps; do not leave blank>
- Validation Plan: <commands, reviews, manual checks>
- Rollback / Stop Conditions: <conditions>

## Task Graph

| Task ID | Description | Status |
| --- | --- | --- |
| <request_id-T1> | <description> | Planned |

## Execution

- <YYYY-MM-DD> - <what changed>

## Validation

- Passed: <commands or checks>
- Not Run: <commands and reason>
- Manual Evidence: <summary or N/A>

## Doc Sync

- Roadmap updated: No
- Changelog updated: No
- Related docs updated: No
- Links checked: No

## Post-Mortem

- Lessons: <summary or N/A>
- Follow-ups: <items or N/A>

## Notes

- Keep the request State Summary identical to its master-roadmap entry when
  roadmap parity is required.

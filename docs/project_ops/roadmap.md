# Synaptome Project Ops Roadmap

This roadmap is the Project Ops request index for Synaptome. It is not the public runtime feature documentation and does not replace `docs/release_policy.md`.

## In Progress

No active Project Ops requests are open.

## Completed

- **Project Ops Compatibility** ([Request Doc](completed/project_ops_compatibility.md))
State Summary
- Phase: COMPLETE
- Status: Complete
- Steps Complete: 4 / 4
- Progress: Synaptome now has a namespaced Project Ops operating surface under `docs/project_ops/**`.
- Last Step Outcome: 2026-05-04 - Project Ops audit passed cleanly for Synaptome.
- Next Step: Use the request template for future public-runtime work.
- Dependencies / Overlap: `.project_ops/config.json`, `docs/project_ops/README.md`, `docs/project_ops/roadmap.md`, `docs/project_ops/reports/changelog.md`, `docs/project_ops/governance/README.md`, `docs/contributing.md`, `docs/release_policy.md`.
- Blocking Issues / Unknowns: None.
- Impact / Priority Notes: Gives Synaptome a Project Ops-compatible administrative surface without importing Tensegrity history or replacing public runtime docs.
- Project Ops / Roadmap Updates (timestamped): 2026-05-04 - Added namespaced Project Ops operating surface and passed adapter audit. 2026-05-05 - Synaptome CI now checks out the external Project Ops repo before public runtime validation.
- Resume From: Complete; future work starts from a new request artifact.
Request Doc: docs/project_ops/completed/project_ops_compatibility.md

## Backlog

- Use request artifacts from `docs/project_ops/in_progress/_REQUEST_TEMPLATE.md` when Project Ops-managed work begins.
- Project Ops audit now runs in Synaptome CI through the external `tensegrity-audio/project_ops` checkout.
- Add release-note templates only if Synaptome starts publishing release notes beyond `docs/release_policy.md`.

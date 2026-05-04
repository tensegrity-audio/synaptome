# Synaptome Project Ops Roadmap

This roadmap is the Project Ops request index for Synaptome. It is not the public runtime feature documentation and does not replace `docs/release_policy.md`.

## In Progress

No active Project Ops requests are open.

## Completed

- **Project Ops Compatibility** ([Request Doc](completed/project_ops_compatibility.md))
  - Phase: COMPLETE
  - Status: Complete
  - Summary: Added the namespaced `docs/project_ops/**` operating surface and aligned `.project_ops/config.json` so Synaptome audits cleanly as a Project Ops adopter without duplicating public runtime docs.
  - Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; public runtime validation ladder.

## Backlog

- Use request artifacts from `docs/project_ops/in_progress/_REQUEST_TEMPLATE.md` when Project Ops-managed work begins.
- Decide later whether to add Project Ops audit to Synaptome CI.
- Add release-note templates only if Synaptome starts publishing release notes beyond `docs/release_policy.md`.

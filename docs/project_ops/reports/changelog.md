# Synaptome Project Ops Changelog

This changelog records Project Ops and administrative workflow changes. Product release versioning remains governed by `docs/release_policy.md`.

## 2026-05-05 - contracts - osc_route_glob_regression
- Phase / Milestone: OSC contract hardening
- Summary: Added the OSC route glob validator to Synaptome's Project Ops validation ladder so built-in mesh-style OSC route coverage is part of the local administrative gate, not only the public contract report.
- Request Doc: `docs/project_ops/roadmap.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python tools\validate_osc_route_patterns.py`; `python tools\validate_configs.py --public-app`
- Follow-Up Actions: Keep route-pattern checks in sync with future mesh OSC contract revisions.

## 2026-05-05 - governance - schema_ownership_cleanup
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Replaced Synaptome's Project Ops adapter schema reference and local menu schema ID with repo-owned, versioned schema namespace IDs. Synaptome now consumes the Project Ops `v0.1.2` schema namespace while keeping Synaptome-owned schemas under the Synaptome `v0.1.0` namespace; Synaptome workflows and adapter metadata are pinned to Project Ops `v0.1.2`.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python -m json.tool .project_ops\config.json`; `python -m json.tool schemas\menu.schema.json`; `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `python tools\validate_configs.py --public-app`; repository-wide schema host scan passed with no raw GitHub or placeholder-local IDs; `git diff --check -- .project_ops/config.json schemas/menu.schema.json docs/project_ops/reports/changelog.md`.
- Follow-Up Actions: Keep future schema identity changes on repo-owned, versioned namespaces; do not reintroduce raw GitHub branch URLs or placeholder local schema hosts.

## 2026-05-05 - governance - project_ops_v0_1_1_pin
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Pinned Synaptome's Project Ops workflow checkouts, adapter schema URL, and adapter metadata to Project Ops `v0.1.1` instead of moving `main`.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `git diff --check`.
- Follow-Up Actions: Cut a new Project Ops tag before changing reusable audit/schema behavior consumed by Synaptome.

## 2026-05-05 - governance - project_ops_remote_request_audit
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Added a Synaptome Project Ops changed-request audit workflow that checks out `tensegrity-audio/project_ops` on GitHub Actions and runs `project_ops_request_audit.py` against changed `docs/project_ops/(in_progress|completed)/*.md` records. Added Project Ops request audit to contributor/local validation and pruned stale Tensegrity process-contract entries from Synaptome's full contract report so `validate_configs.py --contracts` is public-runtime-owned.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `python tools\validate_configs.py --contracts`; `python tools\validate_configs.py --public-app`; `python tools\validate_release_metadata.py`; `python tools\check_app_independence.py`; `python -m py_compile tools\validate_configs.py`.
- Follow-Up Actions: Push Synaptome so remote CI has both Project Ops adapter and request-audit coverage.

## 2026-05-05 - governance - project_ops_ci_audit
- Phase / Milestone: Project Ops CI adoption
- Summary: Updated Synaptome CI to check out `tensegrity-audio/project_ops` and run the reusable Project Ops adapter audit before public runtime validation.
- Request Doc: `docs/project_ops/roadmap.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; `python ..\project_ops\tools\project_ops_request_audit.py --repo ..\synaptome --request-id project_ops_compatibility`
- Follow-Up Actions: Use Project Ops request artifacts for future substantial runtime, contract, release, and docs work.

## 2026-05-04 - governance - project_ops_compatibility

- Phase / Milestone: Project Ops compatibility complete
- Summary: Added the namespaced `docs/project_ops/**` operating surface and updated `.project_ops/config.json` so Synaptome audits cleanly as a real Project Ops adopter without duplicating public runtime docs.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; `python ..\project_ops\tools\project_ops_request_audit.py --repo ..\synaptome --request-id project_ops_compatibility`; `python tools\validate_release_metadata.py`; `python tools\validate_configs.py --public-app`; `python tools\check_app_independence.py`
- Follow-Up Actions: Use Project Ops request artifacts for future substantial runtime, contract, release, and docs work.

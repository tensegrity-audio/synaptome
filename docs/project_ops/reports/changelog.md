# Synaptome Project Ops Changelog

This changelog records Project Ops and administrative workflow changes. Product release versioning remains governed by `docs/release_policy.md`.

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

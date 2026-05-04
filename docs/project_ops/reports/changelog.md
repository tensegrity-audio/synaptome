# Synaptome Project Ops Changelog

This changelog records Project Ops and administrative workflow changes. Product release versioning remains governed by `docs/release_policy.md`.

## 2026-05-04 - governance - project_ops_compatibility

- Phase / Milestone: Project Ops compatibility complete
- Summary: Added the namespaced `docs/project_ops/**` operating surface and updated `.project_ops/config.json` so Synaptome audits cleanly as a real Project Ops adopter without duplicating public runtime docs.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; `python tools\validate_release_metadata.py`; `python tools\validate_configs.py --public-app`; `python tools\check_app_independence.py`
- Follow-Up Actions: Use Project Ops request artifacts for future substantial runtime, contract, release, and docs work.

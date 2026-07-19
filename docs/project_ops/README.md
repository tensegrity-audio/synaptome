# Synaptome Project Ops

Synaptome uses Project Ops for project administration, request tracking, roadmap hygiene, and validation handoff.

Project Ops provides reusable operating structure; Synaptome pins the reusable framework at `v0.1.2`. Synaptome owns its public runtime architecture, release policy, contracts, roadmap, changelog, and validation evidence.

## Local Operating Files

- `.project_ops/config.json` defines Synaptome's Project Ops paths, scope labels, privacy posture, and validation commands.
- `docs/project_ops/roadmap.md` is the single source of truth for current
  priority, active/completed Project Ops work, supporting-roadmap state, and
  the next safe promotion gate.
- `docs/project_ops/synaptome_layer_design_standards.md` records canonical visual-layer model, mode, manifest, parameter, Browser grouping, and validation standards.
- `docs/project_ops/in_progress/_REQUEST_TEMPLATE.md` defines the request artifact shape for future work.
- `docs/project_ops/reports/changelog.md` records meaningful project-operations changes.
- `docs/project_ops/governance/README.md` records local Project Ops policy.
- `docs/release_policy.md` remains the source of truth for product naming and versioning.
- `docs/contributing.md` remains the source of truth for contribution workflow and validation.

## Boundary

This directory is the administrative Project Ops namespace. It must not replace public runtime docs under `README.md`, `docs/readme.md`, `docs/architecture/**`, `docs/contracts/**`, or `docs/examples/**`.

Project Ops compatibility must not import Tensegrity umbrella history, private roadmap state, Mycelium firmware decisions, or local deployment evidence.

## Operating Loop

1. Create a request from `docs/project_ops/in_progress/_REQUEST_TEMPLATE.md`.
2. Keep the request State Summary current.
3. Mirror active request state into `docs/project_ops/roadmap.md`.
4. Record meaningful outcomes in `docs/project_ops/reports/changelog.md`.
5. Run the validation commands listed in `.project_ops/config.json`.
6. Update related public runtime docs before closeout.
7. Move completed request artifacts to `docs/project_ops/completed/`.

## Roadmap Hierarchy

1. `docs/project_ops/roadmap.md` answers **what is active and what happens
   next**.
2. `docs/project_ops/in_progress/**` records execution state and validation
   evidence for promoted work.
3. `docs/architecture/*roadmap.md` answers **how a subsystem should evolve**.
4. Show-development roadmaps preserve visual direction but are parked unless
   the master roadmap links an active request.

Do not infer priority from document length, phase numbering, or the word
"roadmap" in a filename.

# Synaptome Project Ops Governance

This folder records Synaptome-specific Project Ops policy. Reusable process machinery lives in Project Ops; Synaptome owns the decisions that apply to the public runtime.

## Adoption Mode

Synaptome adopts Project Ops as a manual-template and audit-compatible workflow. CI enforcement is not implied by this directory until a later request explicitly adds it.

## Public Boundary

- Product naming and versioning follow `docs/release_policy.md`.
- Contribution workflow follows `docs/contributing.md`.
- Public runtime architecture stays under `docs/architecture/**`.
- Public contracts stay under `docs/contracts/**`.
- Project Ops request history stays under `docs/project_ops/**`.
- Firmware, Mycelium, and device-network decisions stay outside Synaptome unless they are expressed as generic OSC/MIDI/app-facing examples.

## Privacy

Synaptome is public by default. Private/local evidence must stay under configured private paths such as `.local/**`, `docs/private/**`, or private runtime config folders.

## Validation

The normal public-runtime validation ladder is recorded in `.project_ops/config.json` and `docs/contributing.md`.

Required checks for runtime-facing changes:

```powershell
python tools\validate_synaptome_extraction_manifest.py --check --strict-review
python tools\validate_release_metadata.py
python tools\validate_osc_route_patterns.py
python tools\validate_configs.py --public-app
python tools\check_app_independence.py
python ..\project_ops\tools\project_ops_audit.py --repo .
```

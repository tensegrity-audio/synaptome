# Synaptome Validation Playbook

This playbook is the public validation reference for the Synaptome openFrameworks runtime.

## Quick Regression Sweep

Run from the repo root:

```powershell
python tools\validate_synaptome_extraction_manifest.py --check --strict-review
python tools\validate_configs.py --public-app
python tools\check_app_independence.py
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'synaptome\tests\BrowserFlowTest\BrowserFlowTest.vcxproj' `
  /p:Configuration=Release /p:Platform=x64 /m /v:minimal /clp:Summary
python tools\run_control_hub_flow.py --dual-screen-phase2
```

Expected signals:

- extraction manifest has 0 review-gated and 0 unclassified files
- public app contracts pass
- app independence reports no firmware implementation references
- BrowserFlowTest builds with 0 errors
- BrowserFlow reports all 16 scenarios passed
- generated outputs remain ignored

## Harness Catalog

| Harness | Command | Coverage |
| --- | --- | --- |
| Public manifest | `python tools\validate_synaptome_extraction_manifest.py --check --strict-review` | Ensures only public runtime files are included in the extraction payload. |
| Public app contracts | `python tools\validate_configs.py --public-app` | Validates app-facing contracts: parameters, scenes, layer catalog, device maps, MIDI/OSC maps, HUD/Console fixtures. |
| App independence | `python tools\check_app_independence.py` | Catches firmware/helper implementation references in public app source. |
| BrowserFlowTest build | `msbuild synaptome\tests\BrowserFlowTest\BrowserFlowTest.vcxproj /p:Configuration=Release /p:Platform=x64` | Builds the native app-facing regression harness. |
| BrowserFlow summary | `python tools\run_control_hub_flow.py --dual-screen-phase2` | Runs Browser, Console, MIDI, OSC, webcam, HUD feed, and dual-screen routing checks. |

## Generated Artifacts

BrowserFlow writes JSON summaries under `tests/artifacts/` and runtime webcam inventory under `synaptome/bin/data/logs/`. These are ignored by default. Promote a generated file only when it becomes a committed fixture with a validator.

## Manual Smoke Checks

Use these when touching runtime behavior not fully covered by BrowserFlow:

- Launch `synaptome\bin\Synaptome.exe`.
- Load a public scene from `synaptome\bin\data\layers\scenes`.
- Confirm the Browser/Control window remains open during scene load.
- Confirm a webcam-heavy scene publishes before camera open work blocks the UI.
- Save and reload a scene only when intentionally testing persistence.

## Adding A Public Contract

When adding a new public surface:

1. Add or update the schema/fixture.
2. Add a validator or extend an existing one.
3. Wire the validator into `python tools\validate_configs.py --public-app` if it belongs to the first public runtime.
4. Update docs and examples.
5. Rerun the quick regression sweep.

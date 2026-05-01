# Contributing To Synaptome

Synaptome changes should keep the public openFrameworks runtime buildable, testable, and understandable for artists.

## Workflow

- Use focused branches for features, fixes, or docs.
- Keep unrelated refactors out of migration and contract-hardening changes.
- Update docs and fixtures when public behavior changes.
- Do not commit generated build outputs, runtime logs, local Visual Studio user files, or app-written local state unless a fixture intentionally promotes them.
- Keep product/repo naming and version metadata aligned with `docs/release_policy.md`.

Suggested commit style:

```text
oF - describe the app/runtime change
docs - describe the documentation change
tools - describe validator/tooling change
```

## Required Local Checks

Run the public validation ladder before publication or before asking for review on runtime-facing changes:

```powershell
python tools\validate_synaptome_extraction_manifest.py --check --strict-review
python tools\validate_release_metadata.py
python tools\validate_configs.py --public-app
python tools\check_app_independence.py
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'synaptome\tests\BrowserFlowTest\BrowserFlowTest.vcxproj' `
  /p:Configuration=Release /p:Platform=x64 /m /v:minimal /clp:Summary
python tools\run_control_hub_flow.py --dual-screen-phase2
```

Use `python tools\validate_configs.py --public-app` as the public repo gate.

## Code Guidelines

- Prefer existing openFrameworks and Synaptome patterns over new abstractions.
- Keep parameter IDs stable once they are exposed in scenes, MIDI maps, OSC maps, or docs.
- Route user-facing controls through the existing Browser/Console/HUD contracts.
- Keep layer authoring examples honest: first public Synaptome uses source registration. No-source-edit plugin loading is future work.
- Make invalid public registrations fail loudly. Empty or duplicate layer type IDs should not silently replace implementations.

## Documentation Guidelines

- Public docs should describe Synaptome, not private source-workspace staging details.
- Firmware/helper/radio internals should be described only as future package boundaries unless they are rewritten as app-facing examples.
- Examples should include the command that validates them.
- Avoid documenting local machine paths except as replaceable examples using `<YOU>` or a clearly marked placeholder.

## Issue Reports

Please include:

- OS and Visual Studio version
- openFrameworks version and install path
- Build command or validation command
- The first error, plus any relevant warning family
- Whether the app was launched from `synaptome\bin\Synaptome.exe` or through Visual Studio

## CI

Public CI runs:

- strict extraction manifest validation
- public app contract validation
- app-independence audit
- whitespace checks
- BrowserFlowTest build
- BrowserFlow harness summary

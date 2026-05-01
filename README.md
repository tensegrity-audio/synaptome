# Synaptome

Layer-based openFrameworks runtime for live modular visuals with MIDI/OSC mapping, audio/sensor reactivity, operator HUD and Browser controls, and projection output.

Current public baseline: **Synaptome v0.1.0**.

Synaptome is an openFrameworks live-performance runtime for building modular, parameterized, sensor-reactive visual scenes. The app source lives under `synaptome/`; the openFrameworks SDK stays outside this repo.

## Requirements

- Windows 10 or 11, 64-bit
- Visual Studio 2022 with the Desktop development with C++ workload
- openFrameworks 0.12.x Visual Studio package
- Python 3.11+ for validation tools

## Install openFrameworks

Download the Windows Visual Studio package from:

```text
https://openframeworks.cc/download/
```

Extract it to:

```text
C:\Users\<YOU>\Documents\openFrameworks
```

You should have:

```text
C:\Users\<YOU>\Documents\openFrameworks\
  addons\
  apps\
  libs\
  projectGenerator\
```

## Clone Synaptome

Use any local workspace. Example:

```text
C:\Users\<YOU>\Documents\Code\synaptome\
  synaptome\
    Synaptome.sln
    Synaptome.vcxproj
    src\
    bin\
```

If the repo is not exposed through the openFrameworks `apps\myApps` folder, set `SYNAPTOME_REPO_ROOT` before building:

```powershell
$env:SYNAPTOME_REPO_ROOT = "C:\Users\<YOU>\Documents\Code\synaptome"
```

## Optional openFrameworks Junction

Some openFrameworks workflows expect apps under `Documents\openFrameworks\apps\myApps`. Close Visual Studio and the running app before changing junctions.

```powershell
New-Item -ItemType Junction `
  -Path "C:\Users\<YOU>\Documents\openFrameworks\apps\myApps\synaptome" `
  -Target "C:\Users\<YOU>\Documents\Code\synaptome\synaptome"
```

Verify it:

```powershell
Get-Item "C:\Users\<YOU>\Documents\openFrameworks\apps\myApps\synaptome" |
  Format-List FullName,Attributes,LinkType,Target
```

## Build The App

Open either:

```text
C:\Users\<YOU>\Documents\Code\synaptome\synaptome\Synaptome.sln
```

or the junctioned solution:

```text
C:\Users\<YOU>\Documents\openFrameworks\apps\myApps\synaptome\Synaptome.sln
```

Set:

```text
Configuration: Release
Platform: x64
Startup project: Synaptome
```

Build with Visual Studio, or from PowerShell:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'synaptome\Synaptome.sln' `
  /t:Synaptome /p:Configuration=Release /p:Platform=x64 /m /v:minimal /clp:Summary
```

The Release executable is:

```text
synaptome\bin\Synaptome.exe
```

## Validate

From the repo root:

```powershell
python tools\validate_synaptome_extraction_manifest.py --check --strict-review
python tools\validate_configs.py --public-app
python tools\check_app_independence.py
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'synaptome\tests\BrowserFlowTest\BrowserFlowTest.vcxproj' `
  /p:Configuration=Release /p:Platform=x64 /m /v:minimal /clp:Summary
python tools\run_control_hub_flow.py --dual-screen-phase2
```

Expected current signals:

- Public extraction manifest reports no review-gated or unclassified files.
- Public app contracts report `validated=10`.
- App independence audit reports no firmware implementation references.
- BrowserFlowTest builds with warnings tracked as cleanup, but 0 errors.
- BrowserFlow reports all 16 scenarios passed.
- Generated test outputs and runtime logs remain ignored by Git.

## Docs

- [Docs Index](docs/readme.md)
- [Build Environment](docs/build_env.md)
- [Validation Playbook](docs/dev_playbook.md)
- [Contributing](docs/contributing.md)
- [Release Policy](docs/release_policy.md)
- [Artist SDK](docs/architecture/synaptome_artist_sdk.md)
- [External Contracts](docs/architecture/synaptome_external_contracts.md)
- [MIDI Mapping](docs/midi_mapping.md)
- [OSC Catalog](docs/osc_catalog.md)

## License

Synaptome source code and documentation are licensed under the MIT License. See [LICENSE](LICENSE).

Bundled third-party libraries and asset notes are tracked in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Troubleshooting

- If Visual Studio cannot find openFrameworks libraries, confirm `C:\Users\<YOU>\Documents\openFrameworks` exists or set `OPENFRAMEWORKS_ROOT`.
- If the app cannot find data, launch it from `synaptome\bin\Synaptome.exe` or through Visual Studio with the Synaptome project as startup.
- If validation sees live-state drift, remember that strict fixtures live under `tools/testdata/runtime_state/**`; mutable app-written state under `synaptome/bin/data/**` is local runtime evidence unless intentionally promoted.

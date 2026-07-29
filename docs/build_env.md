# Synaptome Build Environment

This guide covers the public Synaptome app/runtime build. Firmware, helper, and radio-contract packages are intentionally outside the first public Synaptome repo.

## Required Tools

| Tool | Purpose |
| --- | --- |
| Visual Studio 2022 with Desktop development with C++ | Builds the openFrameworks app and native validation harnesses. |
| openFrameworks 0.12.x Visual Studio package | Provides the app framework, addons, and libraries. |
| Python 3.11+ | Runs schema, contract, and extraction validators. |
| Git | Source checkout and publication workflow. |

## Expected Layout

Recommended local layout:

```text
C:\Users\<YOU>\Documents\
  openFrameworks\
    addons\
    apps\
    libs\
  Code\
    synaptome\
      README.md
      docs\
      synaptome\
        Synaptome.sln
        Synaptome.vcxproj
        Directory.Build.props
        src\
      tools\
      tests\
```

The app may also be exposed to openFrameworks through a junction:

```text
C:\Users\<YOU>\Documents\openFrameworks\apps\myApps\synaptome -> C:\Users\<YOU>\Documents\Code\synaptome\synaptome
```

## Build Roots

`synaptome/Directory.Build.props` resolves paths in this order:

- `SYNAPTOME_REPO_ROOT`, when set
- the repo root relative to `synaptome/Directory.Build.props`
- the app root from the path used to open app/runtime projects; repository
  test projects retain the physical root because their sources live under
  `tests/`, keeping junction and physical include paths from being mixed
- `OPENFRAMEWORKS_ROOT`, when set
- the default `C:\Users\<YOU>\Documents\openFrameworks`

Useful PowerShell overrides:

```powershell
$env:SYNAPTOME_REPO_ROOT = "C:\Users\<YOU>\Documents\Code\synaptome"
$env:OPENFRAMEWORKS_ROOT = "C:\Users\<YOU>\Documents\openFrameworks"
```

Reload Visual Studio after changing environment variables or build-root props.

## Troubleshooting Mixed Junction And Physical Paths

The supported openFrameworks junction and the physical checkout are two names
for the same files, but MSVC does not always treat them as one include
namespace. If one translation unit reaches a header through both names,
`#pragma once` may not suppress the second copy. The resulting failure appears
as a large burst of duplicate declarations or definitions, commonly:

- `C2011`: type redefinition
- `C2084`: function already has a body
- errors in otherwise unrelated shared headers such as
  `ParameterValueOrigin.h` or `modifier.h`

This was encountered on 2026-07-29 after opening the app through:

```text
C:\Users\<YOU>\Documents\openFrameworks\apps\myApps\synaptome
```

while generated include roots still pointed at the physical checkout.

`Directory.Build.props` prevents that split as follows:

- app and runtime projects use the path namespace through which their project
  was opened;
- repository-backed test projects use the physical repository namespace,
  matching the sources they compile from `tests/`.

Do not replace those roots with one unconditional physical or junction path.
Doing so fixes one project class while recreating the collision in the other.
After changing checkout or junction layout, close Visual Studio, remove only
the affected project's normal intermediate output if necessary, reopen the
project, and rebuild.

To reproduce the junction-sensitive app check directly:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'C:\Users\<YOU>\Documents\openFrameworks\apps\myApps\synaptome\Synaptome.vcxproj' `
  /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:minimal /clp:Summary
```

The expected result is a linked `Synaptome.exe` with no `C2011` or `C2084`
errors. The repository test projects must also build after this check.

## Build

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'synaptome\Synaptome.sln' `
  /t:Synaptome /p:Configuration=Release /p:Platform=x64 /m /v:minimal /clp:Summary
```

## Validate

```powershell
python tools\validate_synaptome_extraction_manifest.py --check --strict-review
python tools\validate_configs.py --public-app
python tools\check_app_independence.py
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
  'synaptome\tests\BrowserFlowTest\BrowserFlowTest.vcxproj' `
  /p:Configuration=Release /p:Platform=x64 /m /v:minimal /clp:Summary
python tools\run_control_hub_flow.py --dual-screen-phase2
```

These commands are the public publication ladder. They intentionally avoid PlatformIO and firmware helpers.

## Generated Outputs

Generated files are ignored:

- `synaptome/bin/Synaptome.*`
- `synaptome/bin/data/logs/`
- `synaptome/obj/`
- `synaptome/tests/**/x64/`
- `tests/artifacts/*.json`
- `__pycache__/`

Do not commit local build output or app-written runtime logs.

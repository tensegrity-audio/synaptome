# Built-In Element Parameter Contract

Status: Authoritative maintainer guide after SEAC-4, 2026-07-27.

Synaptome has one reviewed parameter declaration for each built-in element
type. That declaration is the authority for parameter identity, kind, display
metadata, defaults, grouping, ranges, options, aliases, and deprecation
metadata. Runtime storage must match it exactly, but does not get to redefine
it.

This contract currently covers 23 built-in element types and 786 parameters.
The live validator has confirmed exact binding parity across all 55 committed
catalog assets.

## Authoritative And Generated Artifacts

| Artifact | Role | Edit directly? |
| --- | --- | --- |
| [`builtin_element_parameters.json`](builtin_element_parameters.json) | Reviewed type-level source snapshot | Yes, after reviewing an intentional runtime export |
| [`element_parameter_catalog.json`](element_parameter_catalog.json) | Machine-readable catalog view | No; generated |
| [`../element_parameter_reference.md`](../element_parameter_reference.md) | Human-readable reference | No; generated |
| `synaptome/src/runtime/BuiltinElementParameterContracts.generated.inc` | Compiled Runtime declarations | No; generated |
| [`parameter_manifest.json`](parameter_manifest.json) | Compatibility ID manifest used by persistence and mappings | No; generated |

`tools/gen_builtin_element_contracts.py` keeps the reviewed snapshot, compiled
payload, catalog, and reference identical. `tools/gen_parameter_manifest.py`
keeps compatibility manifests synchronized.

## Rules

- Each element type exposes one stable parameter surface. Catalog assets and
  presets may select different starting values, but they may not change a
  parameter's ID, kind, group, range, unit, options, or meaning.
- The static declaration owns Runtime metadata and declared defaults.
- A live element must bind exactly the declared IDs and kinds. Missing,
  duplicate, extra, or wrong-kind bindings fail preparation transactionally.
- New elements should use explicit bind-only storage through
  `ParameterBinder`.
- Twenty-two legacy built-ins currently use `LegacySetupAdapter`. Their
  `setup()` methods are permitted to initialize storage and resources, but
  metadata emitted during setup is discarded and checked against the static
  declaration.
- In the compatibility adapter, setup must pass the configured storage value
  as the registry default. `ParameterRegistry::add*` writes its supplied
  default into live storage; passing the static default would overwrite asset,
  preset, or scene configuration.
- Stable public IDs are compatibility boundaries for scenes, presets, MIDI,
  OSC, Browser controls, and automation. Renaming one requires an explicit
  alias or migration, updated fixtures, and release notes.

## Updating A Built-In Contract

Run these commands from the repository root in PowerShell.

First build the Release app, then export a fresh reviewed snapshot:

```powershell
$repo = (Resolve-Path .).Path
& .\synaptome\bin\Synaptome.exe `
  --export-builtin-element-contracts `
  "$repo\docs\contracts\builtin_element_parameters.json" `
  "$repo\synaptome\bin\data\layers"
```

The exporter launches an openFrameworks window and graphics context, enumerates
available models and webcam devices, and may update ignored local runtime logs.
It does not open a webcam during contract capture. This is a developer tool,
not an operator workflow.

Review the snapshot diff before generating anything. In particular, inspect
changes to IDs, kinds, group IDs, order, defaults, ranges, steps, units,
descriptions, options, aliases, and deprecations. Do not accept a capture
blindly: a difference is either an intentional public-contract change or a
runtime bug.

After review, regenerate all derived views:

```powershell
python tools\gen_builtin_element_contracts.py
python tools\gen_parameter_manifest.py --write
python tools\gen_parameter_manifest.py --include-packages --write
```

Then prove the generated tree is current:

```powershell
python tools\gen_builtin_element_contracts.py --check
python tools\gen_parameter_manifest.py --check
python tools\gen_parameter_manifest.py --include-packages --check
```

Rebuild the Release app and validate every built-in type against every
committed catalog asset:

```powershell
$repo = (Resolve-Path .).Path
& .\synaptome\bin\Synaptome.exe `
  --validate-builtin-element-contracts `
  "$repo\synaptome\bin\data\layers"
```

The expected live summary is 23 types and 55 assets with no parity failures.
Also run the focused native suites appropriate to the change:

```powershell
& .\synaptome\tests\RuntimeCoreTest\x64\Release\RuntimeCoreTest.exe
& .\synaptome\tests\LayerPackageBench\x64\Release\LayerPackageBench.exe
& .\synaptome\tests\BrowserFlowTest\x64\Release\BrowserFlowTest.exe
python tools\validate_element_sdk_boundary.py
python tools\validate_parameter_targets.py --strict --contract-fixtures
```

Build the test executables first if they are not present or are stale. A
parameter change is complete only when the reviewed source, generated views,
compiled declaration, compatibility manifests, live bindings, and relevant
persistence fixtures agree.

## Scope Boundary

This contract makes built-in element parameters authoritative and
construction-free. The broader architecture now provides versioned state
ownership through SEAC-5, package serialization through SEAC-7, generated
registration through SEAC-8, and package control transactions through SEAC-9.
It does not yet provide:

- automatic package discovery (SEAC-10);
- a stable binary plugin ABI or support for dropping in an arbitrary raw
  openFrameworks `ofApp` folder.

An openFrameworks experiment must still be wrapped as an element and declare a
stable Synaptome contract before it gains scenes, presets, Browser inspection,
or MIDI/OSC mapping.

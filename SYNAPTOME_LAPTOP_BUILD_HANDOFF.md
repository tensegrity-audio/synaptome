# Synaptome Laptop Build and Launch Handoff

## Goal

Diagnose why the full Synaptome openFrameworks application does not build or
run in Visual Studio. This is a build and launch investigation, not a request
to continue Phase 10 or change layer-registration behavior.

## What We Know

The last two relevant commits are:

```text
2243392 Fix show media paths and local package opt-in
1f03b23 Fix show build path and parallelize Release compilation
```

`1f03b23` was an attempted repair for a real layout problem. It moved the
Signal Bloom implementation that the app compiles from
`docs/examples/artist_sdk/` into `synaptome/src/visuals/`, so the app no longer
depends on source files outside its own application directory.

Do not revert this as a first response. Reverting restores the old fragile
source path and does not address the older openFrameworks solution-path issue.

## Known Structural Risks

### 1. The solution requires the app to be under openFrameworks

`synaptome/Synaptome.sln` hard-codes the openFrameworks library project as:

```text
..\\..\\..\\libs\\openFrameworksCompiled\\project\\vs\\openframeworksLib.vcxproj
```

That is valid only when the project directory is:

```text
<OPENFRAMEWORKS_ROOT>\\apps\\myApps\\synaptome
```

The project file itself recognizes `OPENFRAMEWORKS_ROOT`, but the solution's
project reference does not. A standalone clone may therefore show an unloaded
or missing `openframeworksLib` project even when the environment variable is
set.

### 2. Extra add-ons are compiled directly

`Synaptome.vcxproj` directly includes source files from:

```text
addons\\ofxFft
addons\\ofxGui
addons\\ofxMidi
addons\\ofxOsc
addons\\ofxVectorGraphics
```

The local openFrameworks installation must contain each of those directories.
Missing `ofxFft` or `ofxVectorGraphics` is enough to cause compile errors.

### 3. The app is not covered by the current CI build

GitHub Actions builds `BrowserFlowTest`, not `synaptome/Synaptome.vcxproj`.
Passing CI therefore does not prove the full graphics application can build or
launch on Windows.

### 4. A separate runtime media-path bug was fixed later

`2243392` resolves relative media paths against `config/videos.json`. This is
not expected to block compilation, but an older checkout can launch with
missing media or empty video layers depending on the working directory.

## First Pass: Establish a Correct Build Layout

1. Pull the current branch and record the revision:

   ```powershell
   git pull --ff-only
   git log -3 --oneline
   ```

   Confirm the history contains `1f03b23` and `2243392`, or later commits.

2. Confirm the openFrameworks root contains the expected folders:

   ```powershell
   $of = "$env:USERPROFILE\Documents\openFrameworks"
   Test-Path "$of\libs\openFrameworksCompiled\project\vs\openframeworksLib.vcxproj"
   Test-Path "$of\addons\ofxFft\src\ofxFft.cpp"
   Test-Path "$of\addons\ofxVectorGraphics\src\ofxVectorGraphics.cpp"
   ```

   Every command should return `True`.

3. Place the app at the layout the solution requires. If the repo is elsewhere,
   make a junction. Close Visual Studio first.

   ```powershell
   New-Item -ItemType Junction `
     -Path "$env:USERPROFILE\Documents\openFrameworks\apps\myApps\synaptome" `
     -Target "C:\PATH\TO\synaptome\synaptome"
   ```

   The target is the inner `synaptome` app directory, the one containing
   `Synaptome.sln`, not the repository root.

4. Open the solution through the junction, then set:

   ```text
   Configuration: Release
   Platform: x64
   Startup project: Synaptome
   ```

   Do not select ARM64 or ARM64EC for this first diagnosis.

## Capture the Decisive Error

In Visual Studio:

1. Run **Build > Rebuild Solution**.
2. Open **View > Output**, select **Build** in the dropdown.
3. Copy from the first line containing `error` through the final build summary.
4. Also copy the first warning that names a missing file, import, header, or
   library. The first error is usually the cause; the rest are often cascade
   errors.

If you prefer PowerShell, run this from the repository root and save the log:

```powershell
$msbuild = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
& $msbuild "synaptome\Synaptome.sln" `
  /t:Synaptome /p:Configuration=Release /p:Platform=x64 `
  /m /v:diagnostic /clp:Summary 2>&1 |
  Tee-Object -FilePath "synaptome-build.log"
```

If Visual Studio is Professional or Enterprise, replace `Community` in the
path. Send back `synaptome-build.log`, or at minimum the first error block plus
the final summary.

## Error-to-Cause Map

| First error or symptom | Most likely cause | Next action |
| --- | --- | --- |
| `openframeworksLib.vcxproj` cannot be found or loads as unavailable | The solution is opened outside the required `openFrameworks/apps/myApps` layout. | Create/use the junction above, reopen the solution. |
| Missing `ofxFft.cpp`, `ofxFft.h`, or `ofxVectorGraphics.cpp` | The required add-on is absent from the local openFrameworks install. | Install or restore the exact add-on version expected by the project. |
| `cannot open source file` under `docs/examples/artist_sdk/SignalBloomLayer...` | Checkout predates `1f03b23`, or project files are stale/conflicted. | Pull current main and confirm project references `src/visuals/SignalBloomLayer.cpp`. |
| Build succeeds but app immediately exits or shows no media | Runtime data/working-directory issue, possibly an older checkout before `2243392`. | Launch `synaptome/bin/Synaptome.exe` directly and collect the console/log output. |
| Build succeeds but Debugger cannot start | Wrong startup project, wrong configuration/platform, or stale output directory. | Set `Synaptome` as startup project, use `Release|x64`, then clean and rebuild. |

## Scope for a Fix

Once the first actual error is captured, make the smallest correction that
addresses it. Do not combine this work with dynamic package discovery, generated
registration, plugin loading, or other Phase 10 work.

The likely durable repository fixes are:

1. Make the `.sln` openFrameworks project reference resolve from
   `OPENFRAMEWORKS_ROOT`, or document and enforce the junction-only layout.
2. List and provision all non-core add-ons as explicit dependencies.
3. Add a Windows CI job that builds the real `Synaptome.vcxproj` using the
   documented openFrameworks layout.

## Evidence to Return

Return these items after the laptop test:

- `git log -3 --oneline`
- The three `Test-Path` outputs
- The first compiler/linker error block and final build summary
- If it builds: the console output or app log from the failed launch
- The exact on-disk locations of the repository and openFrameworks root

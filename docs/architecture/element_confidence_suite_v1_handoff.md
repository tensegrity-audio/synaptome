# Element Confidence Suite v1: SEAC-6 Execution Handoff

Status: Implemented; retained as the frozen execution contract
Owner task: SEAC-6
Canonical request:
[`../project_ops/in_progress/spine_element_architecture_convergence.md`](../project_ops/in_progress/spine_element_architecture_convergence.md)

Implementation and operating guide:
[`element_confidence_suite_v1.md`](element_confidence_suite_v1.md)

## Outcome

SEAC-6 delivers one reusable runner that can prove an element's declared
surface, isolated lifecycle, deterministic behavior, rendered output,
graphics-state containment, teardown/reload behavior, and performance evidence
without launching the Synaptome host.

This phase generalizes existing evidence; it does not replace it:

- `RuntimeCoreTest` owns Runtime lifecycle and transaction behavior.
- `LayerPackageBench` owns the current Signal Bloom SDK/registration seam.
- `HostCompositionRendererTest` owns renderer traversal and policy against
  controlled stubs.
- BrowserFlow and the Release host remain the separate integration tier.

Stub-backed draw dispatch is not pixel or real-graphics-context evidence.
The implemented suite adds a real hidden/offscreen graphics-context tier for
promotion.

## Frozen Scope

The first reusable suite covers two reference elements:

| Role | Element | Reason |
| --- | --- | --- |
| Simple built-in | `grid` / `GridLayer` | No external media or device dependency; exercises legacy setup adaptation and deterministic geometry/draw behavior. |
| Stateful SDK/package example | `example.signalBloom` / Signal Bloom | Uses the real public Element SDK, bind-only declared parameters, shared leaf registration, transport-driven updates, and the existing reviewed package fixture. |

Do not substitute a webcam, microphone, media-file, or machine-device-dependent
element for either promotion fixture. Additional elements may opt in after the
two frozen fixtures pass.

Dependency resolution in SEAC-6 means resolving and reporting the source,
assets, static libraries, project references, registry entry, and test profile
needed to build and run the selected element. Missing and duplicate
requirements must fail before setup. Versioned inter-package dependency graph
semantics, cycle policy, and package serialization belong to SEAC-7.

## Canonical Runner And Profiles

Implement this repository-root command:

```powershell
python tools\run_element_confidence.py --profile grid --tier ci
python tools\run_element_confidence.py `
  --package docs\examples\layer_packages\signal_bloom\layer.package.json `
  --tier ci
```

The runner must be non-interactive, return nonzero for a failed required check,
print a concise human summary, and write a machine-readable JSON report under
`artifacts/element-confidence/`. Generated reports are build artifacts and
must not become public fixtures.

Add internal runner profiles under `tools/element_confidence_profiles/`.
These profiles configure the test harness and do not become Element Package v1
or widen the public package contract.

Frozen deterministic inputs:

| Input | Grid | Signal Bloom |
| --- | --- | --- |
| Viewport | 640 x 360 | 1280 x 720 |
| Update frames | 60 | 120 |
| Fixed step | 1 / 60 second | 1 / 60 second |
| BPM | 120 | 120 |
| Transport speed | 1.0 | 1.0 |
| Seed | 1001 | 1001 |
| Determinism repetitions | 2 | 2 |

Signal Bloom's values intentionally match
`docs/examples/layer_packages/signal_bloom/tests/layer_test.json`. The suite
must inject time, transport, viewport, seed, and generic input values; it must
not read wall-clock time, live OSC/MIDI, audio, webcam, display selection, or
operator-local files.

## Test Tiers

### Tier 0: Static And Dependency

- Run the applicable authoring, SDK-boundary, and package validators.
- Resolve every declared file, asset, project reference, library, registry
  entry, and profile input before construction.
- Fail missing and duplicate requirements with stable diagnostics.
- Prove that checking one element does not compile or register unrelated
  element implementations except controlled shared Runtime/SDK support.

### Tier 1: Native Contract And Lifecycle

- Compile against the real Element SDK and Runtime seam.
- Inspect the static type/action/parameter declaration before construction.
- Run setup or bind-only adoption, then compare live IDs, kinds, ranges,
  defaults, options, actions, and binding mode with the static declaration.
- Update with the frozen inputs and produce a deterministic state signature.
- Repeat an identical run and require identical state signatures.
- Shut down cleanly and prove registry/action invalidation.

### Tier 2: Real Offscreen Graphics

- Create a hidden real graphics context without constructing `ofApp`.
- Render the frozen frames into an FBO and read back the final pixels.
- Require at least two distinct RGBA values and at least 0.1 percent nonblack
  pixels. A profile may set a stricter threshold.
- Emit a SHA-256 pixel signature. Exact pixel-signature comparison is required
  only against a reviewed baseline with the same renderer/vendor/driver class;
  cross-GPU CI uses the nonblank and state checks.
- Capture and restore framebuffer binding, viewport, matrices, style/color,
  blend, depth, scissor, active shader/program, active texture unit, and bound
  textures around every element draw.
- Fail any graphics-state leak and report the leaking state names.

`HostCompositionRendererTest` remains the policy test; Tier 2 is the missing
real-context evidence. A stub counter cannot satisfy Tier 2.

### Tier 3: Reload And Performance Evidence

- Warm up 20 create/setup/update/draw/shutdown cycles.
- Run 200 additional reload cycles in one process.
- Require zero crashes, setup failures, stale registry/action entries,
  unreleased harness-owned graphics targets, or graphics-state leaks.
- Record process working set after warmup and after every measured cycle.
- Fail if final working set exceeds the warm baseline by more than 16 MiB or
  if the least-squares growth slope exceeds 64 KiB per reload.
- Record update and draw median, p95, and maximum duration separately.
- Performance timings are evidence, not a universal GPU benchmark. The first
  accepted run establishes a machine/renderer-class baseline; later runs on
  the same class fail above 20 percent p95 regression unless the baseline
  change is reviewed and recorded.

### Tier 4: Host Integration

This remains separate from the isolated command:

- existing RuntimeCore, renderer-policy, BrowserFlow, and public-contract
  checks pass;
- the Release app builds with zero warnings and zero errors;
- the host's construction-free declaration for both fixtures matches the bench;
- no Phase 6 implementation adds element-specific logic to `ofApp.cpp`.

Live projection, physical MIDI, webcam, microphone, and show-machine recovery
remain manual/deferred evidence and do not silently become CI dependencies.

## Report Contract

Every JSON report must include:

- schema version, UTC timestamp, commit, configuration, platform, and tier;
- element type, package/profile ID, resolved dependency list, and binding mode;
- declaration and live-surface comparison result;
- deterministic input values and both state signatures;
- graphics renderer/vendor/version when Tier 2 runs;
- nonblank metrics and pixel signature;
- reload count, warm/final working set, and fitted growth slope;
- update/draw median, p95, and maximum timings;
- per-check pass, fail, skip, duration, and diagnostic;
- overall result.

A skipped required check fails `--tier ci`. Unsupported optional
pixel-baseline comparison must be reported as skipped without hiding the
required nonblank/GL-containment result.

## Required Existing Baseline

Before and after implementation, run:

```powershell
python tools\validate_artist_sdk_example.py
python tools\validate_element_sdk_boundary.py
python tools\validate_runtime_core_boundary.py

msbuild synaptome\tests\ElementSdkCompileContract\ElementSdkCompileContract.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m /v:minimal
msbuild synaptome\tests\RuntimeCoreTest\RuntimeCoreTest.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64 /m /v:minimal
msbuild synaptome\tests\HostCompositionRendererTest\HostCompositionRendererTest.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64 /m /v:minimal
msbuild synaptome\tests\LayerPackageBench\LayerPackageBench.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m /v:minimal

.\synaptome\tests\RuntimeCoreTest\x64\Release\RuntimeCoreTest.exe
.\synaptome\tests\HostCompositionRendererTest\x64\Release\HostCompositionRendererTest.exe
.\synaptome\tests\LayerPackageBench\x64\Release\LayerPackageBench.exe
python tools\validate_configs.py --public-app
```

The existing `tools/layer_authoring_profiles/runtime-core.json` and
`tools/layer_authoring_profiles/signal-bloom-sdk.json` remain valid entry
points. The new runner may orchestrate them but must not fork their contract
logic into a second authority.

## Implementation Order

1. Add the report model, profile loader, dependency preflight, and runner CLI.
2. Extract reusable lifecycle/declaration checks from `LayerPackageBench`
   without weakening its current executable.
3. Add Grid and Signal Bloom adapters and deterministic state signatures.
4. Add the hidden real-context offscreen executable and GL-state guard.
5. Add reload/memory/timing collection and baseline comparison.
6. Wire the isolated `ci` tier into CI, then run the separate host integration
   tier.

## Promotion Checklist

SEAC-6 is complete only when:

- both canonical runner commands pass from a clean checkout;
- Grid and Signal Bloom pass every applicable required tier;
- the real-context tier proves nonblank output and GL containment;
- 200 measured reloads satisfy the cleanup and memory-growth gates;
- the host and bench expose the same declarations;
- existing RuntimeCore, renderer-policy, LayerPackageBench, BrowserFlow,
  public-contract, and clean Release-build gates remain green;
- command, profile, report, baseline-review, and failure-diagnostic behavior
  are documented;
- no Package v1 serialization, generated registration, discovery, or
  operator-owned mapping mutation is pulled forward from SEAC-7 through
  SEAC-10.

Stop and record the evidence instead of weakening a gate if real graphics
output cannot be made deterministic enough for a universal signature, a
fixture requires live machine state, the bench and host declarations differ,
or teardown requires host-owned singleton cleanup. Hardware-specific pixel and
performance baselines may be scoped by renderer class; correctness,
nonblank-output, GL containment, and lifecycle cleanup may not be waived.

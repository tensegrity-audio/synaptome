# Element Confidence Suite v1

Status: Implemented for the frozen Grid and Signal Bloom fixtures

The Element Confidence Suite proves one element without constructing
`ofApp`. It is an isolated confidence gate, not a replacement for RuntimeCore,
renderer-policy, BrowserFlow, Release-host, or manual show-machine evidence.

## Commands

Run from the repository root on Windows with openFrameworks 0.12.x,
Visual Studio/MSBuild, and Python 3.11 or newer:

```powershell
python tools\run_element_confidence.py --profile grid --tier ci
python tools\run_element_confidence.py `
  --package docs\examples\layer_packages\signal_bloom\layer.package.json `
  --tier ci
```

`--profile` selects an internal harness profile. `--package` reads the package
ID and selects its one exact matching profile. The two options are mutually
exclusive. Profiles under `tools/element_confidence_profiles/` are test
configuration and do not widen Element Package v1.

The available tiers are cumulative:

| Tier argument | Evidence |
| --- | --- |
| `static` | Validators and dependency preflight. |
| `contract` | Static/live declaration parity, frozen updates, two state signatures, and teardown invalidation. |
| `graphics` | Contract plus hidden real-context FBO rendering, pixel metrics/signature, and graphics-state containment. |
| `reload` | Graphics plus 20 warmups, 200 measured reloads, working-set gates, and update/draw timings. |
| `ci` | All required isolated tiers. |

A required failure or skip returns nonzero. Optional renderer-class pixel and
performance baseline comparisons may be skipped without hiding the required
nonblank, containment, lifecycle, or memory gates.

## Dependency Preflight

Preflight resolves the selected element's source, registration leaf, project
files, authoring profile, package source files, presets, and test profile
before construction. It uses stable diagnostic prefixes:

- `dependency.missing`
- `dependency.duplicate-id`
- `dependency.duplicate-target`
- `dependency.registry-count`

The Grid and Signal Bloom native projects each register exactly one element
type. Checking one fixture does not compile the other element implementation
or the aggregate built-in registration source.

## Reports

Each run writes one timestamped JSON report beneath
`artifacts/element-confidence/`. Use `--report PATH` to choose a deterministic
output path. Reports include commit/configuration/platform identity, resolved
dependencies, binding mode, declaration/live comparison, frozen inputs, two
SHA-256 state signatures, graphics identity and pixel SHA-256, nonblank
metrics, reload memory evidence, update/draw timing summaries, every check,
and the overall result.

The artifact directory is ignored by Git. Reports are evidence and must not be
promoted into public package fixtures.

## Graphics Evidence

The native harness creates a hidden GLFW/OpenGL 3.2 context and an RGBA FBO
without constructing `ofApp`. It requires at least two RGBA values and at
least 0.1 percent nonblack pixels.

Every draw is surrounded by a guard that captures and restores:

- draw/read framebuffer bindings and viewport;
- model-view, projection, and texture matrices;
- openFrameworks style and color;
- blend, depth, and scissor state;
- active shader/program;
- active texture unit; and
- 2D, rectangle, and cube-map bindings on every combined texture unit.

Any state that still differs after restoration is reported by name and fails
the required graphics check.

## Reload And Performance Gates

The harness warms 20 create/setup/update/draw/shutdown cycles, then measures
200 more in the same process. Every cycle checks composition/action and
parameter-registry invalidation plus graphics-state containment.

The run fails when:

- final working set is more than 16 MiB above the warm baseline; or
- least-squares growth exceeds 64 KiB per reload.

Update and draw median, p95, and maximum durations are evidence. They become a
regression gate only when a reviewed baseline matches the same platform and
renderer class.

## Reviewed Baselines

Pass `--baseline PATH` to compare a reviewed baseline. A matching entry is
exact on every field it declares in `rendererClass`; more than one matching
entry is a configuration failure. Pixel signatures must match exactly.
Update and draw p95 values may be at most 20 percent above their reviewed
values.

Example:

```json
{
  "schemaVersion": 1,
  "entries": [
    {
      "id": "grid-show-machine-amd-2026-07",
      "profileId": "grid",
      "rendererClass": {
        "system": "Windows",
        "machine": "AMD64",
        "vendor": "ATI Technologies Inc.",
        "renderer": "AMD Radeon(TM) 890M Graphics",
        "version": "3.2.0 Core Profile Context 26.6.4.260624"
      },
      "pixelSignature": "<64 lowercase SHA-256 characters>",
      "timings": {
        "updateP95Ms": 0.002,
        "drawP95Ms": 3.6
      },
      "review": {
        "approvedBy": "<reviewer>",
        "approvedUtc": "2026-07-28T00:00:00Z",
        "sourceCommit": "<full commit>",
        "reason": "<why this renderer-class baseline was accepted>"
      }
    }
  ]
}
```

To establish or change a baseline, retain the source report as review
evidence, copy its exact renderer identity, pixel signature, and p95 timings,
record the approving commit/reviewer/reason, and rerun with `--baseline`.
Do not weaken the nonblank, graphics-containment, cleanup, or memory gates to
accept a baseline.

## CI And Integration Boundary

The `element-confidence` CI job runs on a Windows x64 runner labeled
`element-confidence`. That runner must have the repository's supported
openFrameworks/Visual Studio toolchain and a real OpenGL 3.2-capable graphics
context. Hosted stub or counter-only drawing cannot satisfy this job.

Host integration remains separate. Before promotion, run the RuntimeCore,
HostCompositionRenderer, LayerPackageBench, BrowserFlow, public-contract, and
clean Release-host gates from the execution handoff. Live projection,
physical MIDI, webcam, microphone, and show-machine recovery remain manual or
deferred evidence.

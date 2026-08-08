# SEAC-10 Remote Diagnostics Ledger

Status: active handoff record for diagnosing the final controlled-discovery
operator path from another machine. Update this file with a timestamp, commit,
machine context, exact action, observed result, and attached artifact whenever
a symptom is reproduced or resolved.

## Current Finish-Line Issues

| ID | State | Symptom / evidence | SEAC-10 relevance | Next remote diagnostic |
| --- | --- | --- | --- | --- |
| SEAC10-DIAG-01 | Open evidence gap | Automated discovery and BrowserFlow gates pass, but the current runtime log contains no `ControlledDiscovery` refresh or activation record. Success and no-op paths are therefore not reconstructable remotely. | Direct: the final operator path is not represented in durable evidence. | Run the Release app at the commit under test, enable configured roots, refresh, inspect, activate one package and one STL candidate, and preserve the snapshot, activation file, and complete run log. Record the visible candidate IDs/statuses and whether active composition changed. |
| SEAC10-DIAG-02 | Open isolation risk | Manual runs rewrote tracked operator-facing state (`config/midi-map.json` and named scenes) and created `.bak` files. These files are deliberately excluded from the SEAC-10 commit because they are machine/session state, not discovery evidence. | Indirect but high diagnostic risk: committing or comparing this state can obscure the discovery-only diff and make remote reproduction non-deterministic. | Reproduce from a clean checkout or copied data directory. Capture dirty-state output before and after the run. Do not promote maps, scenes, backups, logs, or `.vs` state without a separate reviewed fixture change. |
| SEAC10-DIAG-03 | Open observability issue | The available runtime log is dominated by per-update `hud.feed.updated` notices, making warnings and discovery events difficult to find in a remote log review. | Indirect: relevant refresh/publication failures can be buried in high-volume unrelated output. | Preserve the full log, then attach a filtered extract for `ControlledDiscovery`, `warning`, and `error`; include surrounding lines and timestamps rather than only the final tail. Consider rate-limiting the feed notice in a separate observability change. |
| SEAC10-DIAG-04 | Open environment gap | The focused suite passes 16 tests and skips the directory-symlink case when Windows link creation is unavailable. Physical and junction Release builds pass, but this skipped containment case is not positive evidence on the current machine. | Direct to root-containment policy. | Run the focused test from an elevated or Developer Mode machine that can create symlinks and record the full 17-test result plus Windows version and privilege context. |
| SEAC10-DIAG-05 | Deferred operator evidence | Live physical MIDI and the complete show-machine recovery rehearsal remain untested. The existing log also reports zero MIDI ports. | Not a discovery implementation failure, but it limits confidence that activation/refresh work remains isolated during a representative live session. | Repeat refresh/activation/removal and failed-refresh cases on the show machine with the normal MIDI/audio/display setup; record that mappings, active layers, and output survive unchanged. |
| SEAC10-DIAG-06 | Needs classification | The available shutdown log ends with `ofFile: remove(): file does not exist` and `Failed to publish canonical preferences`. The log predates a captured SEAC-10 operator run, so causality is unproven. | Unknown; track separately unless it reproduces during controlled discovery. | Record the source commit and clean-data baseline, reproduce shutdown once without discovery and once after refresh/activation, then compare paths and warnings. Do not attribute this to SEAC-10 without that comparison. |

## Required Capture Bundle

For a remote diagnosis, attach these artifacts from the same run and identify
the exact commit:

```powershell
git rev-parse HEAD
git status -sb
python -m pytest tests\test_controlled_package_discovery_v1.py -q
python tools\validate_configs.py --public-app
```

- `synaptome/bin/data/config/package-discovery.json`, with machine-local roots
  redacted only if necessary;
- `synaptome/bin/data/config/package-discovery.snapshot.json` before and after
  refresh;
- `synaptome/bin/data/config/package-discovery-activations.json` before and
  after activation or replacement acceptance;
- the complete run log plus a filtered `ControlledDiscovery|warning|error`
  extract;
- a screenshot or transcription of the Controlled Discovery Browser rows,
  including status, candidate ID, signature, provenance, and diagnostics;
- `git status -sb` after exit so runtime-written portable files are visible.

Do not attach `.vs` databases or operator scene/map backups to the PR. If one
of those files is essential to reproduction, copy the smallest redacted input
into a separately reviewed diagnostic artifact and explain why it is needed.

## Known-Good Gates

As of 2026-08-08 on the local Phase 10 worktree:

- focused discovery: `16 passed, 1 skipped` (Windows symlink capability);
- public app contracts: `validated=24`;
- `git diff --check`: passes, with line-ending conversion warnings only;
- previously recorded Phase 10 evidence: BrowserFlow 51 scenarios plus physical
  and junction Release host/BrowserFlow builds with zero errors.

These gates prove the contract and automated lifecycle behavior. They do not
replace the capture bundle for an operator-only or machine-specific symptom.

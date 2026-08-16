# SEAC-10 Remote Diagnostics Ledger

Status: closed evidence record. The final controlled-discovery operator path
passed on 2026-08-15 at commit `0511bf9` in an isolated detached Windows 11
25H2 worktree. Retain this ledger as SEAC-10 promotion evidence.

## Current Finish-Line Issues

| ID | State | Symptom / evidence | SEAC-10 relevance | Next remote diagnostic |
| --- | --- | --- | --- | --- |
| SEAC10-DIAG-01 | Resolved | The isolated Release app refreshed an enabled, non-stale, construction-free generation-2 snapshot with two available candidates. Operator screenshots show the Browser rows and both Tetrahedron Fixture and Signal Bloom running in slots 1 and 2. The local activation document records both exact candidate IDs, type/definition IDs, and signatures. | Direct promotion evidence complete. | None for SEAC-10. Success-path log notices remain optional observability cleanup; the snapshot, activation document, and operator capture are authoritative evidence. |
| SEAC10-DIAG-02 | Resolved | The run used detached worktree `synaptome-phase10-validation`. Only its intentional discovery config/snapshot edits were tracked; runtime activation, log, console, scene-last, and backup outputs remained ignored. The primary worktree's pre-existing operator files were unchanged. | Isolation procedure proved effective. | Reuse an isolated worktree or copied data directory for future operator captures. |
| SEAC10-DIAG-03 | Reclassified cleanup | The captured run could be filtered successfully, but refresh/activation success is represented primarily by the snapshot, activation document, and UI rather than dedicated runtime notices. | Observability quality, not a discovery correctness or promotion blocker. | Track success notices or HUD-feed rate limiting only in a separate observability change. |
| SEAC10-DIAG-04 | Resolved | After enabling Windows Developer Mode on Windows 11 25H2 build 26200.9168, the focused suite completed `17 passed` with the directory-symlink containment case exercised. | Direct containment evidence complete. | None. |
| SEAC10-DIAG-05 | Moved to show readiness | Live physical MIDI and the complete show-machine recovery rehearsal remain untested. | Explicitly outside SEAC-10 implementation and promotion. | Continue under `show_readiness_operator_stability`; do not reopen controlled discovery for this evidence. |
| SEAC10-DIAG-06 | Classified non-discovery | The isolated run reproduced `ofFile: remove(): file does not exist` during missing `scene-last.json` recovery, not controlled discovery. It did not reproduce `Failed to publish canonical preferences`. Both discovered candidates activated and ran successfully. | Not caused by SEAC-10. | Route any repeated missing-scene recovery message to persistence/observability cleanup. |

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

As of 2026-08-15 on the isolated Phase 10 worktree:

- focused discovery: `17 passed` with Windows directory-symlink containment;
- public app contracts: `validated=24`;
- `git diff --check`: passes, with a line-ending conversion warning only;
- BrowserFlow: all 51 scenarios pass, including controlled discovery;
- isolated physical Release host and BrowserFlow builds complete with zero
  errors; the existing junction Release build also remains green;
- the generation-2 operator snapshot contains two available/activatable
  candidates, and the activation document records both
  `package:examples.signal_bloom` and
  `content:examples.generated_models.tetrahedron.content`;
- operator capture shows both activated definitions running concurrently in
  slots 1 and 2 without unexpected composition mutation.

These gates and the same-run operator capture close the SEAC-10 promotion
evidence. Future machine-specific symptoms begin as new diagnostics rather
than reopening this completed phase.

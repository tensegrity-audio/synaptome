# Show Readiness And Operator Stability

State Summary
- Request ID: show_readiness_operator_stability
- Phase: EXECUTION
- Status: Deferred by operator
- Steps Complete: 14 / 18
- Progress: Core persistence, mapping recovery, operator status, render, controller-window, and quit-safety work is implemented and validated; the remaining dual-screen and full recovery rehearsal are explicitly deferred.
- Last Step Outcome: 2026-07-26 - The operator postponed dual-screen testing and authorized spine/element architecture execution without treating the residual display check as a blocker.
- Next Step: When show validation resumes, reproduce the dual-screen hiccup, record its exact trigger, and complete the heaviest-scene and device-recovery rehearsal.
- Dependencies / Overlap: `docs/project_ops/roadmap.md`, `docs/architecture/synaptome_spine_element_model.md`, scene persistence, window/monitor placement, MIDI/OSC mappings, Browser, Console, and HUD.
- Primary Scope: runtime
- Secondary Scopes: tests, contracts, docs
- Blocking Issues / Unknowns: Exact dual-screen trigger is not yet recorded and the complete show-machine recovery sequence remains unproven; the operator accepted this as deferred validation risk.
- Impact / Priority Notes: Residual show-machine validation remains important but no longer blocks the spine/element architecture roadmap.
- Priority Score: N/A
- Priority Lane: Deferred
- Ready State: Ready
- Ready Gate: Core implementation and automated checks are complete enough to preserve; remaining acceptance requires later access to the show-machine display and device setup.
- Project Ops / Roadmap Updates (timestamped): 2026-07-24 - Opened as the active show blocker. 2026-07-26 - Recorded operator live evidence. 2026-07-26 - Deferred dual-screen and full recovery rehearsal and promoted the spine/element architecture request.
- Resume From: Phase EXECUTION, State Deferred by operator, Next Action reproduce the dual-screen hiccup when show-machine validation resumes.

- Started: 2026-07-24
- Owner: Synaptome runtime

## Milestone Synthesis

- Milestone ID: show-readiness-operator-stability
- Milestone Name: Show Readiness And Operator Stability
- Milestone Type: validation
- Source Requests: show_readiness_operator_stability
- Outcome Statement (Done When): The heaviest show scene and real control/display setup survive save, mutation, reload, restart, device loss, and recovery without ambiguous state or an unrecoverable operator path.
- KPI / Success Signal: One complete show-machine rehearsal passes with archived known-good scene and mapping backups and no unresolved display blocker.
- Target Window: Before package discovery, mapping expansion, or new visual scope resumes.
- Dependency Gates: Release build, scene/mapping transaction coverage, actual show displays, and actual MIDI/OSC devices.
- Contract Surfaces: Scenes, mappings, layer assignments, window/monitor state, Browser, Console, HUD, MIDI, and OSC.
- Risk Posture: High because failures affect live projection, control recovery, and saved show state.
- Goal: Make the current app predictable and recoverable during a live show.
- Non-Goals: New element algorithms, automatic package discovery, new media families, or native module loading.
- Owner: Synaptome runtime

## Roadmap Overlap Review

- Existing roadmap entries checked: `docs/project_ops/roadmap.md`, element workflow, element package scaffolding, and show-content review lanes.
- Related active requests: layer_package_compatibility_bench_scaffolding
- Duplicate risk: Low
- Merge / split decision: Keep this request focused on show-machine stability; architecture and package expansion remain separate follow-up work.
- Priority conflict: None; this request explicitly precedes package and content expansion.

## Prioritization

- Policy Source: User-directed show-safe checkpoint and operator live-test evidence.
- Priority Score: N/A
- Priority Lane: Fast-Track
- Due Date / Timing Driver: Before the next live show and before architecture/package scope resumes.
- Sort Key: 2026-07-24-show-blocker
- Override: Show-machine reliability takes precedence over new feature breadth.

## Definition Of Ready

- Ready State: Ready
- Ready Date: 2026-07-24
- Ready Owner: Synaptome runtime
- Ready Exceptions: The dual-screen trigger may be discovered during execution because reproducing it requires the show-machine display setup.
- Decision Links: `docs/project_ops/roadmap.md` and `docs/architecture/synaptome_spine_element_model.md`

## Complexity

- Level: High
- Predicted Count: 18
- Count Drivers: Persistence, mappings, status, navigation, displays, performance, recovery, devices, and live acceptance checks.
- Drivers: Cross-surface state ownership, real hardware, multi-window behavior, failure rollback, and show-machine-only evidence.
- Confidence: High

## Intake

- User Request: Bring the existing app to a show-safe operating point before expanding package, media, or visual scope.
- Context: Automated coverage was strong, but scene round-trip, operator feedback, display behavior, and recovery still needed live proof.
- Acceptance Signal: The actual show configuration completes the documented rehearsal and archives a known-good recoverable state.

## Form

- Problem Statement: The operator must be able to trust what is loaded, modified, saved, mapped, displayed, and recoverable during a performance.
- User / Operational Value: A show can continue or recover without guessing which surface owns state or whether a command succeeded.
- Change Type: runtime, contracts, tests, and docs
- Execution Mode: Strict gated
- Acceptance Criteria: Complete the request acceptance list and task graph on the show machine.
- Constraints: Preserve public IDs and current scene/mapping compatibility; do not expand content or discovery scope.
- Must Not Change: Established asset IDs, parameter targets, scene compatibility, or operator mappings without migration evidence.
- Allowed To Change: Runtime waste, persistence transactions, operator status, navigation, display handling, recovery, tests, and documentation.
- Inputs Needed: Exact dual-screen reproduction sequence and access to the show displays and MIDI/OSC devices.

## Analysis

- Touch Map: `ofApp`, scene encoding/loading, MIDI/OSC routing, window placement, Browser, Console, HUD, schemas, fixtures, native flows, and show-machine files.
- Risks: Display placement or focus regression, scene or mapping loss, stale operator status, device recovery failure, and performance regression.
- Alternatives Considered: Continue feature work and rely only on automated tests; rejected because display, device, projection, and operator recovery require live evidence.

## Design Alignment

- Guiding Principles Affected: Stable spine services, explicit ownership, transactional persistence, visible control state, and recoverable failure.
- Systems / Elements / Processes Used: Scene service, mapping adapters, composition layers, operator surfaces, display placement, validation fixtures, and live rehearsal.
- Alignment Rationale: Show readiness is the first practical proof that the spine can host and recover a layered composition safely.
- Design Alignment Log Update: 2026-07-26 spine/element/layer model records this request as the first promotion gate.
- Student-Facing Explanation: Prove the existing instrument survives realistic mistakes and hardware changes before adding more instruments.

## Plan

- Steps: Remove runtime waste; correct scene persistence; unify operator status; harden mappings and writes; stabilize windows and quit behavior; reproduce the dual-screen issue; rehearse the heaviest scene and device recovery; archive known-good state.
- Validation Plan: Run Release, public-app, BrowserFlow, Hotkey, native persistence/mapping tests, then the documented show-machine rehearsal.
- Rollback / Stop Conditions: Stop promotion if a display becomes unreachable, live state is lost on a rejected load, mappings are silently replaced, or the heaviest scene misses its frame-time budget.

## Task Graph

| Task ID | Description | Status |
| --- | --- | --- |
| SROS-1 | Remove output-preserving render waste. | Done |
| SROS-2 | Persist visible values while preserving modifier-owned bases. | Done |
| SROS-3 | Unify operator text, scene, mapping, and device status. | Done |
| SROS-4 | Make scene, mapping, and assignment writes transactional and recoverable. | Done |
| SROS-5 | Stabilize controller windows, fullscreen, Mirror, and quit behavior. | Done |
| SROS-6 | Reproduce and close the dual-screen hiccup. | Deferred |
| SROS-7 | Rehearse heaviest-scene save/reload/restart and device recovery. | Planned |
| SROS-8 | Archive known-good scene and mapping state and close the request. | Planned |

## Outcome

The operator can load, adjust, save, reload, navigate, and recover the show
without guessing which surface owns state or whether a command succeeded.

## Scope

- Scene parameter round-trip correctness.
- Active scene, unsaved-change, load, save, and failure visibility.
- Browser, Console, and HUD label/action consistency.
- Keyboard and controller navigation without modal dead ends.
- Show-machine performance and recovery rehearsal.

Package mapping expansion, automatic package discovery, new tracked media, and
new visual algorithms remain paused until this checkpoint is complete.

## Acceptance

- The heaviest planned scene survives save → mutate → reload with its visible
  unmodulated values, modifier bases/stacks, slot assignments, mappings, and
  effect routes restored.
- Browser and HUD can consume one authoritative scene-status model.
- Every show-critical action reports success or actionable failure.
- Escape/back behavior exits every operator modal predictably.
- Restart, missing device, and rejected scene load leave a usable prior state.
- The show scene stays within the measured frame-time budget on the show GPU.

## Progress

- 2026-07-24: Removed unused full-resolution history buffers, per-frame
  clears/copies, and eager disabled-effect allocations.
- 2026-07-24: Scene serialization now reads the bound live value when a
  parameter is unmodulated and retains the base value when modifiers exist.
- 2026-07-24: Added native float/bool/string persistence coverage; BrowserFlow
  now runs 21 scenarios.
- 2026-07-24: Promoted `ui.menu_text_size` to the app-wide operator text
  scale and propagated it through HUD, Console, Browser, auxiliary menus,
  scene-load status, and the controller window with scaled row spacing.
- 2026-07-24: Replaced Mirror's destructive half-frame duplication in its two
  basic modes with full-frame horizontal/vertical RGBA flips. Quadrant and
  radial modes remain explicitly artistic.
- 2026-07-24: Added a show-critical operator-render contract, completed a
  Release build, passed all 21 BrowserFlow scenarios and 13 public-app
  contracts, and measured a 2.42-second unchanged incremental Release build.
- 2026-07-24: Replaced fractional enlargement of the built-in bitmap font with
  bundled GNU Unifont 17.0.05 rasterized at the requested app text size.
  Font loading is cached by pixel size, ellipsis calculations use the same
  rendered metrics, and the old bitmap renderer remains a hard fallback.
  Release and BrowserFlow builds pass, all 21 BrowserFlow scenarios and 13
  public-app contracts pass, the runtime survived an eight-second launch
  smoke, and the unchanged incremental Release build remains 2.22 seconds.
- 2026-07-25: Clamped restored controller-window bounds to the selected
  monitor's work area so its title bar and resize controls remain reachable.
  Ctrl+F now toggles the specifically focused Console or Controller window,
  preserving the controller's assigned monitor, while Ctrl+Q requires two
  distinct presses within three seconds and reports the armed state in the
  System Status HUD. Corrected the solution-level openFrameworks reference so
  `Synaptome.sln` resolves the same installation as the app project. The
  Release solution build, Hotkey test, all 21 BrowserFlow scenarios, and all
  13 public-app contracts pass.
- 2026-07-25: Corrected Mirror modes 0/1 to the intended half-screen symmetry:
  horizontal mode preserves the left half and reflects it into the right;
  vertical mode preserves the first vertical half and reflects it into the
  other. Replaced the quit HUD warning with a focused-window modal showing
  `QUIT: CTRL+Q` and `ESC: CANCEL`; the modal consumes unrelated input and
  remains open until Escape cancels or a released second Ctrl+Q confirms.
- 2026-07-25: Hardened scene and control-mapping recovery. MIDI/OSC mapping
  documents are parsed and validated into temporary state before replacing a
  working router configuration. A scene-owned `mappings.router` snapshot
  explicitly replaces the current routes, including an intentionally empty
  snapshot; a legacy scene that omits it preserves the loaded global mapping
  file. This historical slot-assignment policy was superseded on 2026-07-28:
  logical hardware slots are now machine-profile owned, legacy Scene fields
  preserve that owner during normal load, and new Scene/autosave writers omit
  them. Machine-profile assignment writes verify a `.tmp` JSON document,
  preserve the previous file as `.bak`, and restore it if promotion fails.
  Mapping files
  follow the same verified backup/recovery path; rejected primary scenes and
  mappings can load their last-known-good backup without discarding live state.
- 2026-07-25: Added operator-visible `SAVED`, `MODIFIED`, load/save results,
  mapping source, MIDI/OSC mapping counts, and unresolved-target counts to the
  shared status feed, HUD, Debug Terminal, and saved-scene Browser rows. Added
  a native save/mutate/restore/restart mapping scenario covering MIDI CC,
  MIDI buttons, OSC ranges/smoothing/deadband/blend/relative mode, malformed
  input, and unavailable MIDI hardware. Scene snapshots now retain the active
  mapping bank, while a 30-second recovery autosave updates `scene-last.json`
  without clearing the named scene's `MODIFIED` state. MIDI disconnection is
  shown as retrying and OSC input is labeled waiting, receiving, or stale.
- 2026-07-26: The operator reported that the previous night's app test exposed
  no major issue other than a dual-screen hiccup. This is positive live
  evidence, but the display issue still needs a reproducible description and
  a focused retest before the show-safe checkpoint can close.

## Deferred Resume

The operator explicitly postponed the remaining live display and recovery
rehearsal on 2026-07-26. These checks remain open acceptance work but do not
block SEAC architecture execution.

1. Reproduce the dual-screen hiccup and record the exact window, monitor,
   fullscreen, focus, and restart sequence that triggers it.
2. Retest the corrected display path on the show machine.
3. Run the full save -> mutate -> reload -> restart rehearsal with the heaviest
   scene and the actual MIDI/OSC devices.
4. Audit show-critical navigation and recovery for modal dead ends.
5. Visually verify GNU Unifont at the text-scale extremes and operator show
   resolution, plus Mirror half-screen symmetry with the heaviest composition.
6. Archive the verified show scene and mapping backup before generating or
   importing more media.

# Show Readiness And Operator Stability

- Request ID: `show_readiness_operator_stability`
- Status: In Progress
- Priority Lane: Show Blocker
- Started: 2026-07-24
- Owner: Synaptome runtime

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

## Next

1. Add authoritative active-scene and dirty/load/save status.
2. Surface the same status in Browser and HUD feeds.
3. Audit show-critical navigation and recovery.
4. Run the show-machine save/reload and performance rehearsal.

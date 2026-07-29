# Package Control Transactions v1: SEAC-9 Handoff

Status: Implemented

Owner task: SEAC-9

## Outcome

Package presets and mapping suggestions are now operator-owned transactions.
Signal Bloom can be assigned to a Console layer, previewed without changing
base ownership or persisted routes, explicitly applied, edited, disabled,
removed, and rolled back.

Nothing in package validation, catalog loading, generated registration, or
Browser inspection applies a preset or route automatically.

## Preset Transaction

- Moving through an active package preset bank previews resolved values in the
  live layer while preserving the prior base value and provenance.
- Enter commits the resolved base values with `preset` provenance and publishes
  the selected package preference.
- Escape restores the pre-preview live values.
- `R` restores the prior base/live/origin state and the prior persisted package
  selection.
- A failed preference write restores the complete prior working state.
- The Browser Ownership column exposes base, live, active/declared modifier
  counts, and base origin for each parameter.

## Mapping Transaction

- Package mapping targets are explicit parameters or actions. Action routes
  require rising, falling, or both-edge trigger semantics and a finite
  threshold.
- Preview expands each local target against the assigned layer instance, shows
  current target, exact-route, and shared-source conflicts, and performs no
  route write.
- Enter applies a conflict-free preview. `X` is the explicit conflict-override
  action. Existing operator routes are never silently claimed.
- `E` edits the first package-owned route from the latest observed OSC source.
  `D` disables routes without deleting them. `U` removes package-owned routes.
  `R` publishes the prior route snapshot.
- Persisted routes retain stable package, preset, mapping-index, layer-index,
  package-version, and route identities.
- Route publication stages and validates the whole mapping bank. A failed write
  restores the prior live snapshot.
- Action routes dispatch through Runtime's existing slot-addressed action
  invocation plane and do not serialize live handlers.

## Acceptance Evidence

- Element Package v1, catalog, Browser inspection, and mapping-bank validators
  pass with the explicit target/action vocabulary.
- Focused native coverage proves layer-instance expansion, operator target,
  exact-route, and shared-source conflicts; explicit replacement; route
  provenance; parameter and action dispatch; failed-write restoration; preset
  preview/cancel/apply/rollback; and Browser apply/edit/disable/remove/rollback.
- BrowserFlow passes all 50 scenarios.
- Python coverage passes all 39 tests plus two subtests.
- Physical and junction Release host builds are the final integration gates.

## Stable Boundaries

SEAC-9 does not make packages discoverable, default-enable package activation,
change stable package/type/definition/parameter/action/preset/mapping
identities, or introduce a native binary module ABI.

## SEAC-10 Input

Execution contract:
[Controlled Package Discovery v1](controlled_package_discovery_v1_handoff.md).

SEAC-10 may rely on:

- one validated and generated package with transactional controls;
- inspection metadata that carries complete presets and mapping suggestions;
- stable local-to-layer target expansion and package route provenance;
- recoverable publication that preserves the active show state on failure.

Controlled discovery must remain default-off and inspect-before-activate.
Malformed, duplicate, incompatible, missing, refreshed, or removed content must
not change the active show state.

# Generated Element Registration v1: SEAC-8 Handoff

Status: Implemented

Owner task: SEAC-8

Implementation contract:
[`../contracts/generated_element_package_registration_v1.md`](../contracts/generated_element_package_registration_v1.md)

## Outcome

Signal Bloom now ships from its contained Element Package v1 source through a
deterministic controlled registration set. Its complete Runtime descriptor is
generated from the validated normalized package. The package leaf supplies
only the creator.

The previous shipping mirror, handwritten `SignalBloomRegistration` bridge,
dedicated `Element_SignalBloom` project, solution entry, and package-specific
host/test source-list entries are retired.

## Acceptance Evidence

- A clean generation check validates the complete controlled set and rejects
  stale output.
- Duplicate controlled identities, unsafe set paths, unresolved required
  package dependencies, creator-symbol drift, and stale output have focused
  negative tests.
- `BuiltinElements.cpp` calls one stable generated-package entrypoint.
- The host and both reference benches use the generic opt-in generated build
  target.
- Signal Bloom's generated Runtime descriptor matches its Package v1
  descriptor exactly before creator invocation.
- The Layer Package Bench constructs and exercises the generated registration.
- The public catalog still resolves all 23 Runtime factory types.
- The package remains default-off for runtime catalog activation; generation
  does not enable discovery or mutate mappings.
- Focused Python coverage passes all 39 tests plus two subtests.
- Grid and Signal Bloom complete CI confidence profiles pass; Signal Bloom's
  generated descriptor signature remains
  `39e6e7934d09689bd1a952e2d040f3a36ebdf595a47446778d1745a2fcdb00de`.
- All 24 public-app contracts, app independence, and all 49 BrowserFlow
  scenarios pass.
- Release host builds pass from both the physical checkout and the
  openFrameworks junction with zero errors. Existing third-party
  openFrameworks/add-on warnings remain outside this change.

## Stable Identities

SEAC-8 does not change Signal Bloom's package, type, definition,
registry-prefix, parameter, preset, mapping, schema, package-version, or
implementation-version identities.

## SEAC-9 Input

SEAC-9 may rely on:

- one validated package and generated Runtime declaration;
- construction-free package/Runtime parity;
- the explicit suggestion-only mapping inventory;
- stable parameter/action targets and provenance rules;
- a confidence report containing generated-registration evidence.

SEAC-9 must not make mapping suggestions apply automatically. Preview,
conflict comparison, explicit apply/edit/disable/remove, persistence, and
rollback remain transactional operator actions.

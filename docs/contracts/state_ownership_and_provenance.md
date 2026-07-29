# State Ownership And Provenance Contract

Status: SEAC-5 implementation contract, promotion gate passed 2026-07-28. The first
executable slice—the side-effect-free Scene v1/v2 compatibility reader—is
implemented. The second executable slice adds nonserialized parameter-value
origins and pointer-free inspection snapshots. The third versions the actual
`MidiRouter` route snapshot as mapping-bank v1 while retaining legacy
unversioned reads. The fourth introduces strict machine-profile v1 and moves
new OSC transport/endpoint writes behind it while retaining the fragmented
OSC compatibility reader. The fifth adds transactional logical control-slot
assignments and removes machine-local assignments from new Scene and autosave
writes. The sixth adds the optional single-input physical MIDI lane,
exact-unique port resolution, and recoverable explicit rebinds. Broader
machine and preference lanes remain in progress. The seventh makes the
shared Text compatibility boundary transactional: candidate defaults remain
private through preparation and publish only when Runtime adopts the element.

This contract defines which Synaptome artifact may own each kind of state, how
the runtime explains the origin of a value, and how versioned readers preserve
current scenes and mappings without absorbing machine-local state.

Read with:

- [Synaptome Spine And Element Model](../architecture/synaptome_spine_element_model.md)
- [Built-In Element Parameter Contract](builtin_element_parameter_contract.md)
- [Layer Package Activation Contract](layer_package_activation.md)
- [Scene schema](../schemas/scene.schema.json)
- [Mapping-bank v1 route schema](../schemas/mapping_bank_route_snapshot.schema.json)
- [Global bank-definitions v1 schema](../schemas/bank_definitions.schema.json)

## Non-Negotiable Invariants

- A portable artifact must not silently acquire physical device names, ports,
  monitor geometry, absolute local paths, UI layout, or transient telemetry.
- A missing optional state section means **preserve the current owner**. A
  present empty section means **the artifact explicitly owns an empty value**.
- Loading is read-only. Compatibility migration happens in memory and does not
  rewrite the source file until the operator explicitly saves.
- Unsupported future versions, ambiguous ownership, invalid value kinds, and
  unresolved required migrations fail before live state is changed.
- A failed load restores the previous composition, parameters, mapping routes,
  active bank, and persisted local snapshots.
- Stable asset IDs, type IDs, parameter IDs, registry prefixes, action IDs,
  and `console.layer{slot}` addresses are compatibility boundaries.
- Package mapping declarations are suggestions. They never become live,
  scene-owned, or operator-owned routes without an explicit preview/apply
  action.

## Value Precedence And Origin

Starting and live values resolve in this order:

```text
element declaration default
  -> element-definition default
  -> selected preset
  -> activation override
  -> scene value
  -> live operator edit
  -> active modifiers
```

The rightmost applicable source controls the result. An element declaration
owns the type-level default and metadata. A catalog asset or future package
definition may choose a different starting value without changing the
parameter's ID, kind, or metadata.

For every addressable parameter, the spine must be able to report:

- `baseValue`: the value before modifiers;
- `liveValue`: the value after active modifiers;
- `baseOrigin`: one of `element-default`, `definition-default`, `preset`,
  `activation-override`, `scene`, or `operator-edit`;
- `originId`: the stable asset, preset, activation, or scene identity when the
  origin is an artifact;
- `artifactVersion`: the version actually read for that origin;
- `migrationTrail`: the ordered compatibility migrations and aliases applied;
- `modifiers`: the active modifier identities in evaluation order.

`migrationTrail` explains how a value was interpreted; it is not itself a
higher-precedence value source. Telemetry and current-frame data never receive
a persisted value origin.

## Ownership Matrix

| State | Portability | Owner and scope | Canonical authority | Version boundary | Missing-on-load behavior | Write authority |
| --- | --- | --- | --- | --- | --- | --- |
| Element declaration default | Portable, package/type level | Element type contract | Reviewed static parameter declaration | Built-in declaration snapshot v1 today | Contract error; no guessed default | Maintainer-reviewed contract generation |
| Element-definition default | Portable, one stable asset/definition | Element definition | Catalog config today; Element Package v1 later | Definition/package version | Fall back to element declaration default | Package/definition author |
| Bundled preset | Portable, one element definition | Package preset | Stable `packageId`, `assetId`, and `presetId` plus suffix-keyed values | Preset v1 | No preset layer is applied | Package author |
| User preset | Operator-local unless explicitly exported | Operator preset store | Versioned local preset with the same definition/suffix rules | User-preset v1 | Preserve current base value | Operator |
| Activation override and selected package preset | Machine/operator local | Explicit package activation record | Default-off activation plus local override | Activation v1 today | Package remains disabled or uses committed activation | Operator |
| Layer assignment, layer active state, opacity, coverage, explicit parameter values | Portable | Scene | Stable layer index, asset ID, and full instance parameter addresses | Scene v2; unversioned input is legacy v1 | Omitted section preserves the current owner | Operator scene save |
| Scene bank definitions | Portable only when intentionally included | Scene | `banks.scene` and layer-scoped bank definitions | Scene v2 | Preserve current external banks; clear prior scene/layer banks during a successful scene replacement according to current compatibility behavior | Operator scene save |
| Global bank definitions | Operator-local | Independent global-bank definition store | `config/bank-definitions.json` with stable bank/control IDs | Bank-definitions v1 | An absent canonical file delegates to legacy Scene input; a present empty document selects the built-in Home fallback; invalid/future input blocks legacy takeover | Operator through the app publication seam |
| Applied parameter/action routes | Scene when explicitly embedded; otherwise operator mapping store | Visible owner selected at save/apply time | Versioned router snapshot with stable logical targets | Mapping-bank v1 is current; missing `schemaVersion` is legacy unversioned input | Omitted route snapshot preserves live/operator routes; present empty snapshot clears them | Operator |
| Active mapping bank | Local | Operator preferences | Stable bank ID | Preferences v1 `mappings.activeBank`; legacy Scene field is read-only compatibility input | Omission delegates to the legacy Scene/current runtime selection; canonical present value wins and is published recoverably | Operator |
| Logical device/control topology | Portable adapter definition | Device profile | Stable logical device, control, column, and slot IDs without local ports | Device-profile v1 target; current device-map schema is permissive | Profile remains unavailable | Adapter author |
| Logical control-slot assignments | Machine profile by default; portable only after an explicit logical-only export | Machine profile | Stable assignment key to device-profile ID, logical slot ID, and analog kind | Machine-profile v1 `controlSlots`; the unversioned slot file is compatibility input | Omission preserves current assignments; explicit empty clears them | Operator |
| Physical device, MIDI port, OSC listen/send port, audio device, webcam choice | Local | Machine profile or named legacy adapter | Logical source ID to physical endpoint | Machine-profile v1 is current for OSC, physical MIDI input, and logical control slots; audio/video remain classified compatibility lanes pending machine-profile v2 | A present MIDI row resolves only to one exact enumerated port name; missing, renamed, or non-unique ports remain unresolved. Present empty disables; omission alone may delegate to the named legacy reader | Operator/machine administrator |
| Monitor identity, window geometry, DPI, VSync, local content paths | Local | Named display/path compatibility adapter pending a future machine-profile version | Local display/path configuration | Console v3 and existing local configuration remain classified compatibility stores | Preserve local state or use a documented safe default; portable assets reject absolute local paths | Operator/machine administrator |
| Browser/HUD layout, filters, collapsed state, authoring choices, hotkeys | Local | Operator preferences | Versioned preference document | Preferences v1 is current for Browser/HUD, hotkeys, package activation/preset selection, and active mapping bank | Omitted sections delegate to their named legacy reader; present sections are authoritative | Operator |
| Recovery autosave and Console recovery snapshot | Local recovery state | Runtime on behalf of the operator | `scene-last.json` and Console store | Source artifact version | Missing recovery state starts from named/default scene and local preferences | Runtime, transactionally |
| Telemetry, current frame, device inventory, timestamps, logs, health | Non-portable/transient | Runtime | Live services only | Not persisted as performance state | Recompute | Runtime only |

## Artifact Compatibility Ledger

This table records the implementation observed when SEAC-5A was frozen. It is
not permission to copy current ambiguity into new formats.

| Artifact | Current evidence | Compatibility classification | Required next contract |
| --- | --- | --- | --- |
| Built-in declarations | `builtin_element_parameters.json` has `schemaVersion: 1`; Runtime publishes declaration-owned metadata/defaults and initializes `element-default` origins | Versioned and authoritative | Implemented for value-origin reporting without duplicating declaration truth |
| Package presets | The generated runtime adapter now retains package schema/revision and per-preset schema versions; activation still flattens effective values but carries a suffix-keyed origin sidecar into Runtime | Draft v1; runtime provenance is preserved, but future versions are not yet safely rejected | Accept exactly preset v1 and reject atomically on drift |
| Named scenes and recovery autosave | Writer emits `scene.schemaVersion: 2`; `SceneStateDocument` classifies/normalizes before `buildSceneApplyPlan`; old fixtures may omit metadata or contain machine-local slot assignments | Missing version is legacy Scene v1; explicit `1` is legacy v1; explicit `2` is current | Implemented: invalid/future versions reject before planning; future versions do not fall back to an older backup; legacy slot assignments remain readable but normal load preserves the machine owner; new named/autosave writes omit them |
| Scene/operator router snapshot | `MappingBankDocument` classifies the flat `MidiRouter` snapshot; canonical export emits root `schemaVersion: 1` plus complete `cc`, `buttons`, `oscSources`, and `osc` arrays | Missing version is legacy unversioned input; exact v1 is current | Implemented: normalize copies before parsing, reject invalid/future versions before mutation, never downgrade a future primary through backup, and keep Scene omission distinct from explicit empty |
| Operator global bank definitions | `BankDefinitionsDocument` owns strict custom global-bank/control definitions independently from routes and active-bank choice | Current bank-definitions v1; legacy Scene global banks remain copied compatibility input only when the canonical file is absent | Implemented: canonical-first startup, strict reference/cycle/future rejection, recoverable whole-document publication, and rollback. Reads do not auto-migrate legacy Scene banks, and publication is currently an app seam rather than an operator editor. |
| Public MIDI bank example | `midi_bank.schema.json` uses `{version, bank, mappings}` | Versioned interchange/example shape, not the `MidiRouter` file shape | Kept distinct: the runtime reader rejects this vocabulary instead of silently identifying it with mapping-bank v1 |
| OSC map | Runtime config and public schema use different route vocabulary in places | Operator/machine-local adapter config | Version the runtime shape and migrate through an explicit adapter contract |
| Console store | Reader accepts legacy input; writer emits `version: 3`; one file mixes recovery composition, display, overlay, and preference state | Local compatibility store, not a portable scene | Split by ownership behind compatibility readers before changing the existing file |
| Operator preferences | Strict `preferences.json` v1 owns independently present Browser/HUD, hotkey, package activation/preset-selection, and active-bank sections; unversioned stores remain section-specific compatibility inputs | Current v1 with legacy copied reads | Implemented: canonical-first startup, strict future rejection, section-preserving recoverable publication, and rollback |
| Slot assignments | Strict machine-profile v1 now owns canonical `controlSlots`; `config/ui/slot_assignments.json` remains an absent-section compatibility source and legacy scenes may still contain snapshots | Current machine-local v1 with legacy copied reads | Implemented transactionally; a future logical-only portable export must be explicit and must not reintroduce silent Scene ownership |
| Device maps | Current files mix reusable control topology with local `portHints`/`ports` and are rewritten directly; Device Mapper uses those fields only as advisory discovery metadata | Draft device profile plus machine-local binding in one permissive document | Separate portable device-profile v1 from local machine bindings; keep hints non-authoritative and add recoverable writes |
| Machine profile | Strict `machine-profile.json` v1 owns local UDP/serial selection, one exact physical MIDI input binding, and logical control-slot assignments; audio, video, display, and path state remain classified legacy/local lanes | Current v1 for OSC, `midi`, and `controlSlots` | Legacy `midi-map` selectors remain copied input only when a valid, unblocked profile omits `midi`; new independently authoritative sections require machine-profile v2 plus a v1 normalizer |

## Version Reader Rules

Each artifact owns its own integer version. A field called `version` in one
artifact has no relationship to `schemaVersion` in another.

Readers must:

1. Parse into an isolated document.
2. Classify a missing version as that artifact's named legacy version, never
   as the newest version. A newly introduced aggregate with no historical
   unversioned shape, such as machine-profile v1, rejects a missing version
   and names the fragmented compatibility sources separately.
3. Reject non-integer, non-positive, and unsupported future versions.
4. Apply deterministic, ordered, side-effect-free migrations in memory.
5. Resolve declared aliases before validating required targets and kinds.
6. Validate ownership, IDs, kinds, values, ranges, and references against the
   current declarations.
7. Build a complete apply plan and rollback snapshot.
8. Commit once; if publication fails, restore the prior working state.

Writers emit only the current canonical version. They do not overwrite a
legacy source merely because it was read successfully. Canonical JSON
formatting is allowed to change on an explicit save; public identities,
effective values, routes, absence semantics, and ownership must not.

For Scene v2 specifically:

- `scene.schemaVersion` is the discriminator.
- Missing or explicit `1` input follows the legacy Scene v1 reader.
- Explicit `2` input follows the Scene v2 reader.
- A future version fails before composition preparation or mapping import.
- `mappings.router` and `mappings.activeBank` retain independent presence
  semantics. Omission preserves live state. Presence is authoritative,
  including an empty snapshot.
- `mappings.slotAssignments` is legacy compatibility input. Normal Scene load
  validates but does not adopt it, and new named Scene and recovery-autosave
  writers omit it. Explicit logical-only import/export, if added later, must
  be a separately named operator action.

## Preset Rules

- Preset values are parameter suffixes scoped to exactly one stable element
  definition. They are expanded only after the target instance prefix is
  known.
- Presets do not assign layers, choose physical devices, install mappings, or
  store a whole performance.
- Preset v1 means exactly `schemaVersion: 1`; an unknown version does not
  degrade to v1.
- Package, asset, and preset IDs plus the source package revision remain
  available as runtime provenance after merging.
- Unknown IDs, wrong kinds, invalid values, unresolved aliases, or descriptor
  drift reject the whole preset application.
- Selecting an active package preset in the Browser previews live values while
  preserving base ownership. Explicit apply publishes preset provenance and
  the operator-local package preference; cancel, failed writes, and rollback
  restore the prior working state.
- A scene's explicit value wins over a preset and is sufficient for playback.
  A preset reference may explain provenance but must not make scene recovery
  depend on an unavailable optional package without an explicit dependency
  error.

## Mapping And Bank Rules

- A bank groups controls; a route maps an input source to a parameter or
  action target. They are related but independently owned.
- Canonical routes use stable public targets and retain logical source
  identity when available. Current learned routes may also contain resolved
  channel/number state and are owner-scoped rather than universally portable;
  physical endpoint resolution belongs to the machine profile.
- Source-specific runtime modifiers follow the same rule. Persisted modifier
  data must retain its owner/provenance so scene replacement clears only
  scene-owned modifiers and can rebuild adapter-owned routes deterministically.
- Legacy routes containing physical labels remain readable through an
  explicit compatibility path, but unresolved labels are visible and are not
  guessed.
- Global banks and routes stay operator-owned unless the operator explicitly
  copies them into a scene. Scene/layer banks may be scene-owned.
- Import is atomic. An invalid entry cannot partially replace working routes.
- Package mapping presets remain suggestions with package provenance until an
  explicit operator action creates scene-owned or operator-owned routes.

## Machine And Preference Boundaries

Machine-profile v1 owns the implemented OSC, physical-MIDI-input, and
logical-control-slot lanes and will own the remaining physical environment
facts as their transactional seams land:

- input/output device and port selection;
- logical-source-to-physical-device bindings;
- monitor identity, geometry, DPI, VSync, and window placement;
- machine-local asset roots and content paths;
- logical control-slot assignments to local hardware.

Device-profile v1 is separate and portable. It describes reusable logical
control topology without authoritative `portHints`, local selected port names,
or machine bindings. Current permissive device maps may retain `portHints` and
`ports` as advisory Device Mapper discovery metadata only. Machine profiles
bind those logical identities to the hardware present on one machine. Current
`deviceProfileId`, `slotId`, and MIDI `portName` values preserve exact nonblank
machine-binding values, including shipped spaced names such as `MIDI Mix 0`;
portable device-profile normalization is a separate future contract.

Preferences v1 will own operator UI choices:

- Browser/HUD layout, visibility, filters, and collapsed state;
- authoring and inspection choices;
- local hotkey customizations;
- local package activation and selected next-load preset.

The current Console and Browser stores may remain compatibility containers
while readers are extracted. Their mixed contents do not redefine ownership.
Migration must split data according to this contract without copying physical
or preference state into a named scene.

Portable scene paths must be data-relative or stable content references.
Windows absolute paths are legacy machine-local input and cannot be emitted by
a canonical portable writer. Camera and audio selection likewise use semantic
roles such as `camera.front` or `audio.primary`; a raw device index or name is
resolved and retained by the machine profile, never guessed on another
machine.

Physical MIDI v1 deliberately retains the exact enumerated input-port name in
machine state. Resolution succeeds only when exactly one current port equals
that name. Zero or multiple exact matches remain unresolved; canonical startup
must not guess from a device-map hint, substring, numeric index, or port-zero
fallback.

## Legacy Text Boundary

The `overlay.text.*` parameters are currently host-global, scene-owned
compatibility values backed by the shared `TextLayerState` singleton. They are
not evidence that two Text element instances have independent state.

Until that adapter is retired:

- scene persistence continues to use the stable `overlay.text.*` IDs;
- preparing a candidate Text element must not mutate the shared live values
  before adoption commits;
- failed or abandoned preparation must leave the shared values unchanged;
- documentation and snapshots must not claim per-instance Text state;
- migration to instance-owned Text values requires explicit aliases and scene
  fixtures for the existing global IDs.

The compatibility implementation now captures candidate defaults in a copied
`TextLayerState::Snapshot`. `TextLayer::setup()` neither attaches to nor
normalizes the live singleton. Runtime's no-throw
`onParameterRegistryCommitted()` hook publishes the staged snapshot by swaps
only after all fallible composition staging succeeds. Destruction, preparation
failure, and abandoned replacement therefore discard candidate Text values.
Font discovery and resource loading resume after adoption. This closes the
pre-adoption leak; it does not claim per-instance Text ownership or change any
`overlay.text.*` ID.

## Ordered Implementation Slices

- [x] Add a pure Scene version classifier/normalizer before
  `buildSceneApplyPlan`: missing/1 -> legacy v1 migration, 2 -> current
  validation, unsupported future -> non-destructive rejection.
- [x] Pin legacy/current classification in the scene golden, restrict the
  compatibility schema to v1/v2, and add native invalid/future,
  source-immutability, and omitted-versus-empty coverage.
- [x] Preserve value-origin information through default, preset, activation,
   scene, operator-edit, and modifier resolution; expose copied base/live
   values, artifact version/revision, migration trail, and ordered active
   modifier owner tags without serializing the runtime DTO.
- [x] Version the actual runtime mapping snapshot as mapping-bank v1, keep a
   copied legacy-unversioned reader, emit complete canonical route arrays,
   reject the unrelated public interchange shape, validate embedded snapshots
   before Scene apply, and prevent future-version backup downgrade.
- [x] Introduce strict machine-profile v1 for OSC transport/endpoints behind
   the legacy `osc-input.json` reader; publish explicit Browser changes
   recoverably and keep payload/Mesh compatibility outside the profile.
- [x] Extend machine-profile v1 with strict, transactional `controlSlots`;
   retain the legacy slot file only when that canonical section is absent;
   validate-but-preserve legacy Scene inputs; omit assignments from new named
   Scene and recovery-autosave writes.
- [x] Extend machine-profile v1 with an optional max-one physical MIDI input;
   make present empty disable, omission-only legacy delegation, exact-unique
   resolution, unresolved route/assignment preservation, active-profile route
   filtering, and recoverable explicit rebind/rollback executable.
- [x] Add strict preferences v1 behind section-specific legacy readers for
   Browser/HUD, hotkeys, package activation/preset selection, and active bank;
   preserve unrelated sections and roll back failed writes. Keep audio,
   webcam, display, and path state as named compatibility lanes for a future
   machine-profile v2 rather than widening strict v1.
- [x] Stage shared Text configuration so preparation/adoption is
   transactional; preserve the singleton and stable global IDs until a
   separately reviewed instance-owned migration supplies compatibility aliases
   and Scene fixtures.

## Promotion Gate

SEAC-5 cannot close until:

- legacy unversioned scenes and mappings restore the same effective state;
- canonical current artifacts reject unsupported versions and ambiguous
  ownership before mutation;
- omitted and explicitly empty owned sections have distinct tested behavior;
- default, preset, activation, scene, live-edit, and modifier origins are
  inspectable;
- portable fixtures contain no physical machine state;
- machine/profile and preference writes are recoverable and independently
  versioned;
- failed loads and migrations preserve the prior composition and routes;
- the shared Text configuration path cannot leak candidate state before
  adoption.

All criteria above are executable and passing as of 2026-07-28. The
portable-state gate covers classified Scene/layer/package JSON and deliberately
does not misrepresent the named audio, webcam, display, and path compatibility
adapters as implemented machine-profile v1 sections.

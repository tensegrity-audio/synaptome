# Element Package v1 Contract

Status: SEAC-7 implementation contract

Canonical schema:
[`../schemas/layer_package.schema.json`](../schemas/layer_package.schema.json)

Canonical pure reader and normalized model:
[`../../tools/element_package_v1.py`](../../tools/element_package_v1.py)

Reference package:
[`../examples/layer_packages/signal_bloom/layer.package.json`](../examples/layer_packages/signal_bloom/layer.package.json)

## Scope

Element Package v1 is a portable description of one element type and its
package-owned definition. Reading, normalizing, validating, inventorying, and
comparing a package are source-immutable and construction-free. The document
does not register a creator, reserve a registry prefix, publish parameters,
apply presets or mapping suggestions, or mutate a scene.

The retained `layer.package.json`, `asset`, `parameters`, `mappingPresets`, and
related names are v1 compatibility vocabulary. Their normalized meaning is:

- `packageId` identifies the bundle;
- `element.id` identifies the Runtime type;
- `asset` is the package definition and owns its registry prefix and optional
  definition-level default overrides;
- `parameterGroups` plus `parameters` serialize the Runtime
  `ParameterDeclarationSet`;
- `element.actions` serializes the Runtime `ElementDescriptor` action order;
- `mappingPresets` contains suggestion-only declarations.

## Inventory And Closed Gaps

The pre-SEAC-7 draft already represented package/definition identity, source
files, parameter basics, presets, banks, mapping suggestions, media, one test
reference, and informal compatibility flags. It did not represent or strictly
validate the following Runtime or package fields:

| Missing or duplicate surface | v1 authority |
| --- | --- |
| Runtime type ID and kind | `element.id`, `element.kind` |
| Binding mode | `element.bindingMode` |
| Ordered actions | `element.actions` |
| Ordered parameter groups and parameter group membership | `parameterGroups`, `parameters[].groupId` |
| Visibility, quick access, aliases, exact option-source vocabulary | `parameters[]` |
| Definition-level overrides | `asset.defaults` |
| Independent implementation version | `implementationVersion` |
| SDK/runtime supported and resolved versions | `compatibility` |
| Closed capability vocabulary | `capabilities` |
| Required/optional typed dependencies and graph edges | `dependencies` |
| Test inventory | `tests.confidenceProfile`, `tests.fixtures` |
| Package aliases and deprecations | `aliases`, `deprecations` |
| Ordered source-immutable migrations | `migrations` |
| Confidence report package evidence | the SEAC-6 report's `package` object |

`asset.type` is a reference to `element.id`; it is not a second descriptor
authority. A mismatch fails validation.

## Version Rules

- `schemaVersion` is the document format and must be the integer `1`.
- `packageVersion` versions the portable package independently. This reader
  supports the reviewed pre-1.0 line `>=0.1.0 <1.0.0`.
- `implementationVersion` versions the source/static implementation
  independently.
- SDK, Runtime, and dependency compatibility use conjunctions of explicit
  semantic-version comparisons such as `>=0.1.0 <1.0.0`.
- The validator compares each declared range with the version actually
  resolved. It does not infer a range from the checkout.
- Unsupported schema or package majors fail without rewriting or downgrading
  the source.

## Identity And Path Rules

Stable package, type, definition, registry-prefix, action, group, parameter,
preset, mapping-preset, dependency, and migration identities use the schema's
closed ID grammar. Labels and filesystem paths are not identities.

Every serialized file reference:

- is relative to the directory containing `layer.package.json`;
- uses `/`, is already normalized, and contains no `.` or `..` segment;
- resolves beneath that package directory;
- matches on-disk case;
- names a file before the package is exposed as activatable.

Absolute paths, Windows separators, traversal, missing required files, and
case drift fail with a JSON-path diagnostic. Signal Bloom therefore carries
its declared source inside the package instead of referring outside it.

## Normalized Descriptor

The only parity model has these ordered fields:

```text
typeId
kind
bindingMode
actions[]
parameterGroups[]
parameters[]
```

Each normalized parameter contains its ID, kind, group ID, label, default,
optional range and step, units, description, visibility, inline options,
optional option source, optional quick-access order, aliases, and optional
deprecation.

JSON numbers belonging to `float` declarations are converted exactly once to
IEEE-754 binary32 because `ParameterValue` and `ParameterRange` use C++
`float`. This is a documented vocabulary conversion, not an epsilon
comparison: parity then uses exact values. Object member order is irrelevant;
declared array order is authoritative. Unknown or missing object fields,
different array lengths/order, and any value drift fail.

The native confidence harness copies the construction-free
`ElementTypeContract` from `LayerFactory`, serializes that copy, and exits
without invoking the stored creator. The confidence runner compares it with
the normalized package. Only a passing comparison permits the lifecycle
harness to invoke the creator.

Each parity diagnostic names:

- package ID;
- normalized JSON path;
- Runtime field path;
- expected package value;
- observed Runtime value.

## Defaults, Optional Sections, And Suggestions

Declaration defaults own type-level starting values. `asset.defaults` may
override values for that definition but cannot add parameters or change a
kind. Later preset, activation, scene, operator, and modifier values retain
the precedence frozen in
[`state_ownership_and_provenance.md`](state_ownership_and_provenance.md).

For optional state-bearing sections, omission means the package makes no
claim and a present empty array/object means the package explicitly owns an
empty value. The confidence inventory records each as `omitted`,
`present-empty`, or `present`. The complete Signal Bloom fixture writes empty
inventories explicitly.

Every mapping preset must declare `applyMode: "suggestion-only"`. Validation
and confidence reporting inventory those suggestions but never publish a live
route. Preview/apply remains SEAC-9.

## Dependencies And Capabilities

The v1 capability vocabulary is closed:

- `graphics.opengl`;
- `transport.read`;
- `parameters.dynamic-options`.

Dependencies distinguish package, source, static-library, asset, and
runtime-service requirements. Each record declares required/optional status,
a stable dependency ID, a compatible range, the resolved version, and exactly
one local path or provider identity. Graph edges use `dependsOn`.

Preflight rejects duplicate IDs/providers, incompatible versions, missing
required paths, unresolved edges, and cycles. A missing optional path is
reported as `optional-absent` without changing the normalized declaration.

## Migrations

A migration record contains a stable ID, one older source version, one newer
target version, and one contained transformation file. The v1 validator does
not execute or write migrations. It rejects non-forward edges and more than
one outgoing edge for a source version, which makes the selected path ordered
and deterministic. Unsupported or ambiguous inputs fail closed.

## Confidence Report Extension

Package runs extend the SEAC-6 report with a `package` object containing:

- schema, package, and implementation versions;
- normalized package/type/definition/prefix identity;
- resolved compatibility and declared dependency records;
- serialized and Runtime descriptor signatures;
- exact field-level parity status and diagnostics;
- migration path;
- source, asset, preset, mapping, test, migration, and Runtime descriptor
  inventory.

The report schema version remains `1`; this is an additive SEAC-6 report
extension rather than a competing report.

## SEAC-8 Registration Input

The controlled-registration phase may consume this validated record without
changing the package contract:

```json
{
  "packageId": "examples.signal_bloom",
  "packageVersion": "0.1.0",
  "implementationVersion": "0.1.0",
  "typeId": "example.signalBloom",
  "kind": "visual",
  "bindingMode": "bind-only",
  "definitionId": "examples.signal_bloom",
  "registryPrefix": "examples.signal_bloom",
  "sourceRegistration": "source/register_signal_bloom.cpp"
}
```

SEAC-8 owns how this validated record drives controlled or generated
registration. Element Package v1 does not serialize project-list edits,
factory mutation, discovery policy, native-module loading, or activation
transactions.

# Generated Element Package Registration v1

Status: SEAC-8 implementation contract

Controlled registration set:
[`element_package_registration_set_v1.json`](element_package_registration_set_v1.json)

Generator:
[`../../tools/generate_element_package_registrations.py`](../../tools/generate_element_package_registrations.py)

Package authority:
[`element_package_v1.md`](element_package_v1.md)

## Purpose

Generated registration turns a reviewed, validated Element Package v1 record
into compile-time Runtime registration without adding package-specific entries
to `ofApp.cpp`, `BuiltinElements.cpp`, the Visual Studio solution, or a project
source list.

This is controlled source registration. It is not runtime folder discovery,
dynamic loading, a native plug-in ABI, or permission to activate an arbitrary
package found on disk.

## Controlled Input

`element_package_registration_set_v1.json` is an explicit allowlist of
repository-relative `layer.package.json` paths. It uses exact integer
`schemaVersion: 1`, contains no discovery roots or enable flags, and is the
only package-selection input to generation.

The generator processes the complete set before writing output:

1. Resolve every normalized path beneath the repository.
2. Run the pure Element Package v1 validator.
3. Reject duplicate package, definition, type, or registry-prefix identities.
4. Reject required package dependencies absent from the controlled set.
5. Require `source.strategy: source-registration`.
6. Resolve the contained registration source and compile sources.
7. Require the deterministic package creator symbol.
8. Sort records by stable package ID.

Any failure leaves checked-in generated output untouched.

## Creator Boundary

The contained registration source supplies only a creator:

```cpp
std::unique_ptr<Layer>
synaptomeCreateElementPackage_examples_signal_bloom();
```

The symbol is derived deterministically from `packageId` by replacing
non-alphanumeric characters with `_`. Generation rejects collisions. The leaf
does not declare type identity, actions, parameter groups, parameters,
binding mode, or compatibility metadata.

The creator is stored during registration and is not invoked by package
validation, generation, registration-record inspection, or descriptor parity.

## Generated Authority

The generator emits:

- `GeneratedElementPackageRegistrations.h`, the inspection and registration
  API;
- `GeneratedElementPackageRegistrations.cpp`, normalized metadata records,
  complete `ElementTypeContract` construction, creator bindings, and Runtime
  preflight;
- `GeneratedElementPackages.targets`, the opt-in MSBuild source list with
  package-specific object paths.

The generated Runtime contract contains the package's ordered actions,
parameter groups, parameters, kinds, defaults, exact binary32 values, ranges,
steps, units, descriptions, visibility, options, option sources, quick-access
order, aliases, deprecations, and binding mode. Signal Bloom has no second
handwritten descriptor authority.

`--check` compares every checked-in output byte-for-byte with fresh generation
and reports a unified stale-output diff.

## Build Integration

`synaptome/Directory.Build.targets` imports the generated target once.
Only projects setting:

```xml
<SynaptomeEnableGeneratedElementPackages>true</SynaptomeEnableGeneratedElementPackages>
```

compile the aggregate and controlled package source. The host, Signal Bloom
confidence contract, and Layer Package Bench opt in. RuntimeCore and unrelated
tests do not.

Each generated source receives a package-specific object path. Paths are
rooted through `SynaptomeRepoRoot`, preserving the physical/junction namespace
rules in `Directory.Build.props`.

Adding another validated source package to the controlled set regenerates the
aggregate and target. It does not edit an application or test project list.

## Runtime Ordering

`registerGeneratedElementPackages()`:

1. checks every generated type ID against the destination factory before any
   generated record mutates it;
2. constructs the generated `ElementTypeContract`;
3. registers that contract with the creator-only leaf and generated binding
   mode;
4. verifies that the expected type was published.

The Element Confidence Suite runs generated-output validation before the
native build. It then copies the Runtime descriptor without invoking the
creator and compares it exactly with the normalized package. Lifecycle and
graphics work remain blocked on parity.

The confidence report's additive `package.registration` object records the
registration set, generated outputs, complete validated record, creator
symbol, descriptor signature, and validation status.

## Commands

Generate after an intentional controlled-set or package change:

```powershell
python tools\generate_element_package_registrations.py --write
```

Verify a clean checkout:

```powershell
python tools\generate_element_package_registrations.py --check
python tools\run_element_confidence.py `
  --package docs\examples\layer_packages\signal_bloom\layer.package.json `
  --tier ci
```

## Deferred Boundaries

- Transactional preset and mapping preview/apply/edit/remove belongs to
  SEAC-9.
- Default-off package and data-only content discovery belongs to SEAC-10.
- Native binary module and ABI policy is resolved by
  [`Native Module Policy v1`](../architecture/native_module_policy_v1.md): v1
  has no native runtime loader.
- Catalog-wide representative migration and the final public authoring guide
  were completed by SEAC-12; retain the generated-registration boundary and
  use the public authoring guide for current workflow instructions.

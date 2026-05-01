# Synaptome Release Policy

This repo uses two forms of the name intentionally:

- **Synaptome** is the product, app, runtime, UI title, executable, solution, and project name.
- `synaptome` is the repository, folder, package, path, and command identity.

Use this sentence as the canonical pattern:

```text
Clone the `synaptome` repo to build Synaptome.
```

## Version Numbers

Synaptome uses semantic versioning in the form `MAJOR.MINOR.PATCH`.

- The root `VERSION` file stores the plain version, for example `0.1.0`.
- Git release tags use a leading `v`, for example `v0.1.0`.
- Public docs and UI labels may use `Synaptome v0.1.0`.

## Version Meaning

- Patch releases, such as `v0.1.1`, are for fixes, docs, validation, and build hygiene.
- Minor releases, such as `v0.2.0`, are for meaningful public runtime, contract, SDK, or workflow additions.
- `v1.0.0` should wait until the public artist workflow is stable: clone, build, add a layer, map controls, save/reload a scene, and validate.

## Initial Public Baseline

`v0.1.0` is the initial public runtime baseline. It includes the standalone public repo import, MIT license, third-party notices, app-only validation, BrowserFlow harness, and first source-registration artist SDK fixture.

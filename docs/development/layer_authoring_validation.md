# Fast layer-family validation

Use the layer-authoring runner while creating or migrating a modular layer
family. Its default stage is static and does **not** build openFrameworks:

```powershell
python tools\validate_layer_authoring.py circuit-trace
python tools\validate_layer_authoring.py adaptive-trail
python tools\validate_layer_authoring.py collective-motion
python tools\validate_layer_authoring.py cellular-fields
python tools\validate_layer_authoring.py circuit-lenia
python tools\validate_layer_authoring.py signal-bloom-sdk
```

The fast stage checks the family source contract, registered parameter
suffixes, catalog defaults and stable IDs, model identity, deterministic seed
rules, project entries, and factory wiring. Commands run in order and stop on
the first failure, so a catalog typo does not trigger a compiler.

When the static contract is clean, add the headless native gate:

```powershell
python tools\validate_layer_authoring.py circuit-trace --native
```

This incrementally builds the isolated `LayerAuthoringTest`, which uses the
repository's openFrameworks stubs rather than compiling `openframeworksLib`.
The target contains only family-owned, render-independent native contracts.
Broader BrowserFlow coverage remains part of integration rather than the
per-edit authoring loop.

The Signal Bloom SDK profile is the first build-boundary profile:

```powershell
python tools\validate_layer_authoring.py signal-bloom-sdk --native
```

It compiles the public example through SDK-only compatibility includes using
the headless stubs, checks the deterministic package-registration outputs,
builds the lifecycle bench against the generated aggregate and contained
creator, and runs that bench. Add `--incremental-app` to prove the host compiles
and links the generated package sources.

The runtime lifecycle profile checks the first host/runtime ownership seam:

```powershell
python tools\validate_layer_authoring.py runtime-core --native
```

It builds a focused target against the same real openFrameworks header and ABI
surface as the shipping `SynaptomeRuntimeCore` library. The contract verifies
distinct definition and instance identity, lifecycle ordering, structured
setup failures, exact parameter ownership, prefix reservation for
zero-parameter elements, out-of-namespace registration rollback, composition
ownership/provenance, canonical slot addressing, generic update/draw/resize
routing, staged setup, failed/successful same-address replacement,
host-registration collision rollback, matching-ID modifier preservation,
live-registry rebinding, idempotent shutdown, and safe release. Add
`--incremental-app` to prove the full host still compiles and links through the
library. It also proves scoped type-registry isolation: two Runtime instances
may bind the same type ID to different test creators, and neither can resolve a
type registered only in the other.

`Layer::setup(ParameterRegistry&)` receives a private staging registry, not the
canonical live registry. Author code must only register its own namespace and
must not retain that address. The layer container owns
`{instancePrefix}.opacity`; an element that needs internal alpha control must
use a behavior-specific suffix instead. If later registry lookup is
unavoidable, override
`onParameterRegistryCommitted(ParameterRegistry&) noexcept` only to retain the
committed registry pointer. Full effect/compositing ownership and a scoped
typed descriptor/catalog remain SEAC-3 work. The compatibility factory itself
is already non-global and explicitly owned per app, Runtime test, or bench.

Circuit Trace currently has an isolated eight-direction native contract:

```powershell
python tools\validate_layer_authoring.py circuit-trace --native
```

Adaptive Trail and Collective Motion do not yet expose render-independent
simulation snapshots, so their profiles intentionally have no native stage.
For those families, the exact fast commands above are the honest per-edit
gate; use `--incremental-app` for compiled integration instead of presenting a
static or rendering test as simulation coverage.

The first Cellular Fields slice validates Game of Life and Excitable Media
together while preserving their separate `gameOfLife` and `excitableMedia`
runtime identities. Neither currently exposes a render-independent state
signature, so this profile also stops at the fast static contract before the
explicit incremental-app tier.

Before handing the layer to the app, request the final incremental application
build:

```powershell
python tools\validate_layer_authoring.py circuit-trace --incremental-app
```

The profile builds required project references, including the element and
runtime-core libraries. MSBuild reuses current outputs, so openFrameworks is
not rebuilt unless its inputs changed or its library output is missing.

Use both flags for the complete authoring handoff. `--dry-run` prints the exact
commands, `--keep-going` is available for audits, and `--list` shows profiles.

## Adding another family

Add `tools/layer_authoring_profiles/<family>.json` with these stages:

- `fast`: family-specific source/catalog/default/factory validator commands;
- `native`: a narrow headless build followed by named native tests;
- `incrementalApp`: the final app project build.

Commands are argument arrays and run without a shell. Use `{python}` for the
active Python interpreter and `{msbuild}` to locate Visual Studio through
`PATH` or `vswhere`. Keep cheap, deterministic checks in `fast`; do not put an
application or openFrameworks build there.

The speed advantage is structural: the normal edit loop reads a handful of
source and JSON files and launches no compiler. The optional native tier
compiles the stubbed test target only. The openFrameworks-dependent app build
is reserved for the handoff tier instead of being paid after every parameter
or catalog edit.

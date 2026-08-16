# SEAC-12 Representative Migration Evidence

Date: 2026-08-15

Status: Complete; architecture milestone promoted.

## Frozen Baseline

- Runtime catalog: 23 built-in types and 786 declared parameters.
- Compatibility setup adapters before migration: 22.
- Reference surfaces: Grid 22 parameters, STL Model 15 parameters, Lenia 37
  parameters. Type IDs remain `grid`, `stlModel`, and `lenia`.
- Carried control: Signal Bloom remains the generated-registration,
  bind-only Element Package v1 example.
- No parameter ID, kind, range, default, definition ID, registry prefix,
  catalog identity, scene address, or mapping target changed.

## Implementation Result

- Grid, STL Model, and Lenia now register with explicit `ParameterBinder`
  storage. Their `setup()` methods no longer redeclare parameter metadata.
- The declared compatibility-adapter count is 19. Those remaining adapters
  are cleanup and do not reopen the architecture contract.
- STL mesh loading is deferred until an active graphics context, keeps
  CPU-owned mesh storage, accepts bounded absolute fixture paths, and retains
  the prior instance when content preparation fails.
- Lenia preserves one simulation identity across organic and circuit
  presentations. Deterministic signatures and durable scene values remain
  distinct from transient pixels and graphics resources.
- The public workflow is documented in
  [`../../element_authoring_guide.md`](../../element_authoring_guide.md).

## Confidence Evidence

The complete CI tier passed for all four references:

| Profile | Result | Report |
| --- | --- | --- |
| Grid | 7 pass, 0 fail, 2 baseline skips | `artifacts/element-confidence/grid-20260815T213028535115Z.json` |
| Signal Bloom | 12 pass, 0 fail, 2 baseline skips | `artifacts/element-confidence/examples.signal_bloom-20260815T215244016959Z.json` |
| STL Model | 8 pass, 0 fail, 2 baseline skips | `artifacts/element-confidence/stl-model-20260815T214205889847Z.json` |
| Lenia | 8 pass, 0 fail, 2 baseline skips | `artifacts/element-confidence/lenia-20260815T215918626492Z.json` |

Grid, STL Model, and Lenia each completed at least 200 lifecycle cycles with
the committed memory and timing thresholds. The two skips are explicitly
labeled reviewed-renderer-baseline comparisons; no weaker baseline or
threshold was invented.

Real openFrameworks graphics-context captures were reviewed and are non-blank:

- `artifacts/element-confidence/grid.png`: warped cyan grid presentation;
- `artifacts/element-confidence/stl-model.png`: normalized orange tetrahedron;
- `artifacts/element-confidence/lenia.png`: green circuit-lattice presentation.

## Contract, Host, And Build Results

- Built-in contracts, canonical/combined manifests, strict parameter targets,
  all 24 public-app contracts, the Element SDK boundary, and app independence
  pass.
- All 17 controlled-discovery tests and all generated registration, generated
  catalog, and Browser payload checks pass.
- `RuntimeCoreTest`, `LayerPackageBench`, and all 51 `BrowserFlowTest`
  scenarios pass. BrowserFlow covers explicit binding parity, the tetrahedron
  content/refresh transaction, Lenia presentation and deterministic scene
  behavior, Browser edits, mapping targets, and composition rollback.
- `Release|x64` builds from the physical checkout and supported openFrameworks
  junction complete with zero errors.

## Live Acceptance And Promotion

On 2026-08-16 the operator built and launched Synaptome from Visual Studio and
confirmed Grid, organic Lenia, Circuit Lenia, and the committed **STL Tetra**
definition load in the real host. `STL Tetra` is the 3D Browser catalog entry
backed by `models/lowpoly_tetra.stl`; it exercises the same migrated
`stlModel` implementation and visible tetrahedron presentation.

The separately named **Tetrahedron Fixture** is the default-off controlled
discovery example. Its exact generated-content identity, absolute content
path, activation transaction, missing/changed-content behavior, scene
round-trip, and failed-refresh isolation pass the controlled-discovery,
BrowserFlow, and STL confidence gates. An attempt to assign the unactivated
discovery inspection row correctly did not create a Console layer; it was not
an STL render failure.

This live evidence closes the physical-host gate. SEAC-12 and the complete
12-step spine/element convergence milestone are promoted.

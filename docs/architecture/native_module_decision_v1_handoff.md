# Native Module Decision v1: SEAC-11 Execution Handoff

Status: Complete

Owner task: SEAC-11

Primary roadmap:
[`../project_ops/completed/spine_element_architecture_convergence.md`](../project_ops/completed/spine_element_architecture_convergence.md)

## Goal

Publish an evidence-based native-module policy without implementing a loader
that no observed workflow requires. A decision to retain generated/static
registration is a complete and successful SEAC-11 outcome.

SEAC-11 is a decision gate, not permission to start a DLL, hot-reload, or
plugin-framework project.

## Outcome

The evidence review retained the default decision. The authoritative current
policy is [`Native Module Policy v1`](native_module_policy_v1.md): Synaptome v1
has no native runtime loader. No schema, ABI, loader, placeholder API, or build
project was added.

## Decision Starting Point

The default decision is:

> Synaptome v1 does not load native element modules at runtime. Source elements
> use validated Element Package v1 plus controlled generated registration and
> a normal application rebuild. Supported data-only content uses bounded,
> versioned templates backed by exact precompiled element types. Arbitrary
> experiments remain source migrations or external-process integrations.

This decision may change only if the evidence review below identifies a
concrete valid workflow that cannot be served by generated registration,
data-only discovery, or a safer external bridge.

## Evidence From SEAC-10

The closed controlled-discovery evidence establishes:

- `package:examples.signal_bloom` validates, registers, discovers, activates,
  and runs through generated source registration;
- `content:examples.generated_models.tetrahedron.content` discovers,
  activates, and runs through the precompiled `stlModel` type;
- both definitions run concurrently without discovery mutating the prior
  active composition;
- no valid reviewed package remained `inspection-only` because native code was
  unavailable;
- no observed operator workflow required unloading, hot reload, or avoiding a
  planned application restart;
- all 17 focused discovery tests, 24 public contracts, 51 BrowserFlow
  scenarios, and physical/junction Release builds pass without a native
  loader.

The authoritative evidence record is
[`../project_ops/reports/seac_10_remote_diagnostics.md`](../project_ops/reports/seac_10_remote_diagnostics.md).

## Required Decision Review

Answer each question with repository evidence, not hypothetical convenience.

| Question | Current evidence | Decision consequence |
| --- | --- | --- |
| Is any valid reviewed package blocked solely because its type is not compiled in? | No. | Do not build a loader. |
| Does generated registration require host-source or per-package project editing? | No; the controlled set generates the aggregate and build records. | Retain generated registration. |
| Is restart-free code replacement an accepted show requirement? | No observed requirement. | Do not promise hot reload or unloading. |
| Can a module boundary avoid openFrameworks, compiler, CRT, STL, allocator, exception, and GPU-context ABI exposure? | No frozen boundary proves this. | Reject a C++ plugin ABI. |
| Is crash containment required for arbitrary experiments? | Potentially, but in-process loading cannot provide it. | Prefer a process boundary. |
| Is a data-only family blocked by the current template model? | No reviewed fixture is blocked. | Add future bounded templates independently. |

## Supported Extension Policy

SEAC-11 must publish this capability ladder clearly:

1. Built-in element type compiled with the host.
2. Validated source package in the controlled registration set.
3. Supported data-only content discovered through a precompiled type.
4. External process or transport bridge for experiments needing isolation or
   independent deployment.
5. Native runtime module only after a future proposal satisfies the reversal
   gate below.

An external bridge may use an explicitly versioned transport such as OSC,
Spout, NDI, shared memory, or another reviewed boundary. Naming a possible
transport does not add it to the public contract.

## Reversal Gate For A Future Native Loader

A later proposal may reopen the decision only when it supplies all of:

- one reviewed package that is valid except for unavailable native code and
  cannot reasonably enter the controlled build;
- a versioned narrow ABI that does not expose openFrameworks classes, C++
  standard-library containers, exceptions, allocators, or host-owned GPU
  objects across the boundary;
- exact compiler, architecture, CRT, SDK, openFrameworks, add-on, dependency,
  renderer, and capability compatibility rules;
- package signing/provenance and local trust policy;
- load, initialization, failure, restart, update, replacement, and removal
  policy;
- crash and graphics-state containment analysis;
- deterministic inspection before code execution;
- rollback behavior that preserves registry, catalog, Runtime, scene, mapping,
  preference, and active composition state;
- focused hostile-module tests and a supported distribution workflow.

If those requirements imply a host-process ABI or cannot contain faults, the
proposal must compare an external process bridge and explain why it is not the
safer boundary.

## Execution Slices

1. **SEAC-11A — Evidence inventory:** confirm the SEAC-10 candidate/status
   inventory contains no reviewed native-code blocker.
2. **SEAC-11B — Boundary decision:** record the no-loader decision and the
   supported extension ladder in the architecture and Artist SDK documents.
3. **SEAC-11C — Future reversal policy:** publish the complete reversal gate
   without adding schemas, runtime code, or placeholder module APIs.
4. **SEAC-11D — Roadmap promotion:** update Project Ops, contract gap notes,
   and SEAC-12 input; mark SEAC-11 complete.

No code spike is part of these slices. A spike becomes authorized only if
SEAC-11A finds a concrete unsatisfied workflow and the reversal gate is
accepted before implementation.

## Validation

Run the carried-forward gates to prove the decision changes no behavior:

```powershell
python -m pytest tests\test_controlled_package_discovery_v1.py -q
python tools\validate_configs.py --public-app
python tools\generate_element_package_registrations.py --check
python tools\check_app_independence.py
git diff --check
```

Documentation review must also confirm that no file claims arbitrary native
plugin loading, raw `ofApp` loading, hot reload, or runtime DLL installation.

## Stop Conditions

Stop and return to design review if:

- the decision is based only on hypothetical future convenience;
- a proposed API exposes host C++ or openFrameworks objects across a binary
  boundary;
- inspection would execute module code;
- failure or replacement could mutate the active show before validation;
- the work adds placeholder loader code despite choosing no loader;
- an external process boundary is dismissed without comparison;
- the supported compiler/dependency/distribution matrix has no owner.

## Promotion Checklist

SEAC-11 is complete only when:

- [x] The discovery evidence inventory is recorded and contains no unresolved
      reviewed native-code workflow.
- [x] The no-native-loader v1 decision is published as current policy.
- [x] Generated source registration, data-only templates, and external bridges
      are described without overstating their capabilities.
- [x] The future reversal gate is explicit and testable.
- [x] No speculative module schema, ABI, loader, or project is added.
- [x] Public contracts and validation remain green.
- [x] Project Ops advances to SEAC-12.

## SEAC-12 Input

SEAC-12 may rely on the existing SDK/runtime boundary, generated registration,
controlled discovery, and the no-loader policy. Representative migrations
must prove the public source/package/content paths rather than creating a
private module seam.

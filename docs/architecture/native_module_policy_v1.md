# Native Module Policy v1

Status: Current architecture policy

Decision date: 2026-08-15

Decision owner: Synaptome runtime and Artist SDK

Evidence:
[`../project_ops/reports/seac_10_remote_diagnostics.md`](../project_ops/reports/seac_10_remote_diagnostics.md)

## Decision

Synaptome v1 does not load native element modules at runtime. Source elements
use validated Element Package v1, controlled generated registration, and a
normal application rebuild. Supported data-only content uses bounded,
versioned templates backed by exact precompiled element types. Arbitrary
experiments remain source migrations or external-process integrations.

This is a supported architecture outcome, not an unfinished loader. Synaptome
does not claim runtime DLL installation, native plug-in loading, unloading,
hot reload, or direct loading of a raw openFrameworks `ofApp` project.
Inspection and controlled discovery never execute candidate code.

## Evidence Inventory

The SEAC-10 reviewed inventory contains no unresolved native-code workflow:

| Candidate | Reviewed result | v1 path |
| --- | --- | --- |
| `package:examples.signal_bloom` | Validates, registers, discovers, activates, and runs. | Element Package v1 plus controlled generated source registration. |
| `content:examples.generated_models.tetrahedron.content` | Discovers, activates, and runs through an exact compiled type. | `templates.model.stl` plus the precompiled `stlModel` type. |

Both candidates ran concurrently without discovery changing the prior active
composition. No valid reviewed candidate remained `inspection-only` solely
because native code was unavailable. No observed operator workflow required
unloading, restart-free replacement, or avoidance of a planned rebuild and
restart. The decision preserves the passing 17-test focused discovery suite,
24 public contracts, 51 BrowserFlow scenarios, and physical/junction Release
build evidence.

Generated registration does not require a package author to edit `ofApp.cpp`,
the handwritten built-in aggregate, the solution, or project source lists.
The controlled registration set generates the aggregate Runtime contract and
build records. Discovery remains default-off and admits a source package only
when it exactly matches that compiled record.

## Supported Extension Ladder

Choose the first applicable boundary:

1. A built-in element type compiled with the host.
2. A validated source package in the controlled generated-registration set.
3. Supported data-only content discovered through a bounded template and an
   exact precompiled type.
4. An external process or transport bridge when an experiment needs crash
   isolation, independent deployment, or a separately owned runtime.
5. A native runtime module only after a future proposal passes the complete
   reversal gate below.

An external bridge may use an explicitly versioned transport such as OSC,
Spout, NDI, shared memory, or another reviewed boundary. These are examples,
not additions to the current public contract. A proposal must still define
ownership, compatibility, failure, security, and lifecycle behavior for its
chosen transport.

## Why v1 Rejects An In-Process C++ Plug-in ABI

No reviewed boundary proves that a loaded element can avoid exposing or
depending on openFrameworks classes, compiler and C++ ABI details, the CRT and
standard library, allocators and exceptions, add-on versions, threads, or
host-owned GPU objects and graphics context state. In-process loading also
does not provide crash containment for arbitrary experiments. A process
boundary is therefore the preferred comparison when isolation or independent
deployment is the actual requirement.

## Future Reversal Gate

A later native-loader proposal may reopen this decision only when it supplies
all of the following reviewable evidence:

- one reviewed package that is valid except for unavailable native code and
  cannot reasonably enter the controlled build;
- a versioned narrow ABI that exposes no openFrameworks classes, C++
  standard-library containers, exceptions, allocators, or host-owned GPU
  objects across the boundary;
- exact compiler, architecture, CRT, SDK, openFrameworks, add-on, dependency,
  renderer, and capability compatibility rules;
- package signing/provenance and local trust policy;
- load, initialization, failure, restart, update, replacement, and removal
  policy;
- crash and graphics-state containment analysis;
- deterministic inspection before any candidate code executes;
- rollback behavior preserving registry, catalog, Runtime, scene, mapping,
  preference, and active composition state;
- focused hostile-module tests; and
- a supported distribution workflow with a named owner.

If the proposed boundary still relies on a host-process C++ ABI or cannot
contain faults, it must compare an external-process bridge and explain with
evidence why that is not the safer boundary. Hypothetical convenience alone
does not reopen the decision.

## Change Control

Element Package v1, generated registration, controlled discovery, and
data-only template schemas remain independently versioned contracts. This
policy adds no module manifest fields, ABI declarations, loader API,
placeholder project, or transport promise. Reversing it requires a new
architecture decision and must not mutate active show state before complete
inspection and validation.

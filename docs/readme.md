# Synaptome Docs

Synaptome is a public openFrameworks performance spine for hosting creative
elements in ordered visual layers. These docs describe the reusable app/runtime
surface, not private source-workspace details or helper implementations.

## Start Here

- [Root setup guide](../README.md)
- [Build environment](build_env.md)
- [Validation playbook](dev_playbook.md)
- [Contributing](contributing.md)
- [Release policy](release_policy.md)

## Architecture

- [Spine, element, and layer model](architecture/synaptome_spine_element_model.md)
- [System architecture](architecture/synaptome_system_architecture.md)
- [Subsystem anatomy](architecture/synaptome_subsystem_anatomy.md)
- [External contracts](architecture/synaptome_external_contracts.md)
- [Artist SDK](architecture/synaptome_artist_sdk.md)

## Roadmaps

- [Current priority and pre-media safety gate](project_ops/roadmap.md)
- [Spine and element architecture convergence](project_ops/in_progress/spine_element_architecture_convergence.md)
- [Element-system sequence](architecture/synaptome_layer_system_roadmap.md)
- [Public runtime contract sequence](architecture/synaptome_public_runtime_contract_roadmap.md)
- [Transport and reactivity dependency](architecture/synaptome_transport_reactivity.md)
- [Biological element direction](architecture/synaptome_biological_layer_roadmap.md)

The Project Ops roadmap is authoritative for what is active. Architecture and
show-development roadmaps describe possible sequence but do not establish
priority by themselves.

## Contracts And Examples

- [Contract index](contracts/README.md)
- [Contract gaps](contracts/contract_gaps.md)
- [Fixtures](contracts/fixtures.md)
- [Signal Control integration](contracts/signal_control_integration.md)
- [Media catalog intake](contracts/media_catalog.md)
- [MIDI mapping](midi_mapping.md)
- [OSC catalog](osc_catalog.md)
- [Artist SDK example](examples/artist_sdk/README.md)

## Public Boundary

The first public Synaptome repo owns app-facing runtime contracts: scenes,
element definitions and their compatibility layer catalogs, parameters,
Browser/Console/HUD state, MIDI maps, OSC maps, host audio,
webcam/media/display adapters, examples, and validation tools.

Firmware implementations, helper decode code, deployment netmaps, generated radio headers, embedded UI catalog exchange, and legacy payload quarantine remain outside this repo. They belong in future helper or radio-contract packages.

# Synaptome Docs

Synaptome is a public openFrameworks runtime for live visual performance. These docs describe the reusable app/runtime surface, not private source-workspace details or helper implementations.

## Start Here

- [Root setup guide](../README.md)
- [Build environment](build_env.md)
- [Validation playbook](dev_playbook.md)
- [Contributing](contributing.md)
- [Release policy](release_policy.md)

## Architecture

- [System architecture](architecture/synaptome_system_architecture.md)
- [Subsystem anatomy](architecture/synaptome_subsystem_anatomy.md)
- [External contracts](architecture/synaptome_external_contracts.md)
- [Artist SDK](architecture/synaptome_artist_sdk.md)

## Contracts And Examples

- [Contract index](contracts/README.md)
- [Contract gaps](contracts/contract_gaps.md)
- [Fixtures](contracts/fixtures.md)
- [MIDI mapping](midi_mapping.md)
- [OSC catalog](osc_catalog.md)
- [Artist SDK example](examples/artist_sdk/README.md)

## Public Boundary

The first public Synaptome repo owns app-facing runtime contracts: scenes, layer catalogs, parameters, Browser/Console/HUD state, MIDI maps, OSC maps, host audio, webcam/media/display adapters, examples, and validation tools.

Firmware implementations, helper decode code, deployment netmaps, generated radio headers, embedded UI catalog exchange, and legacy payload quarantine remain outside this repo. They belong in future helper or radio-contract packages.

# Synaptome Project Ops Changelog

This changelog records Project Ops and administrative workflow changes. Product release versioning remains governed by `docs/release_policy.md`.

## 2026-07-24 - runtime - scene_parameter_persistence_and_status

- Request ID: `show_readiness_operator_stability`
- Phase / Milestone: Pre-show operator stability
- Summary: Scene serialization now captures the bound live value for
  unmodulated float, bool, and string parameters while retaining the base value
  for modifier-owned parameters. The System Status HUD and Debug Terminal now
  report the same active scene and last scene-load outcome.
- Request Doc: `docs/project_ops/in_progress/show_readiness_operator_stability.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Release x64 build; all 21 BrowserFlow scenarios; all 13
  public-app contracts; scene persistence and scene/display transaction gates;
  app-independence audit.
- Follow-Up Actions: Add authoritative unsaved-change and save-result state,
  then complete show-machine save/mutate/reload and recovery rehearsal.

## 2026-07-24 - runtime - show_day_render_path_cleanup

- Request ID: `show_day_render_path_cleanup`
- Phase / Milestone: Pre-show runtime performance
- Summary: Removed eight unused full-resolution layer-history render targets
  and their per-frame clears/copies, stopped clearing invisible slot buffers,
  and made post-effect scratch, mirror-history, and motion-history buffers
  allocate only when their effects are first used. Scene output and effect
  algorithms are unchanged.
- Request Doc: `docs/dev_playbook.md`
- Roadmap Entry: Operator-directed pre-show stabilization
- Validation: Clean Release x64 rebuild; all 20 BrowserFlow scenarios; Signal
  Bloom offscreen package bench; all 13 public-app contracts; strict extraction
  and app-independence gates; 2.46-second identical incremental build.
- Follow-Up Actions: Run the documented 60-second heaviest-show-scene visual
  and frame-time check on the show GPU before adding more visual load.

## 2026-07-24 - artist-sdk - labeled_parameter_selection

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Browser live parameter ownership
- Summary: Added one reusable labeled picker for live package parameters
  declared through static `options[]` or registered `optionsSource` metadata.
  Selection updates the existing registry value; provider revisions close stale
  pickers and unavailable current values remain unchanged until explicit
  replacement.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package/inspection gates, all 20
  BrowserFlow scenarios, Release x64 build, and 2.21-second identical
  incremental build.
- Follow-Up Actions: Add explicit package mapping-preset preview/apply/edit
  controls with conflict handling and rollback.

## 2026-07-24 - artist-sdk - package_preset_bank_selection

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Browser preset ownership
- Summary: Added labeled package preset-bank selection backed by stable IDs,
  persisted the choice in the ignored show-machine activation override, and
  applied it only to the next layer instantiation so active scene and mapping
  state remain authoritative.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package-only and combined gates, all
  19 BrowserFlow scenarios, Signal Bloom offscreen bench, Release x64 build,
  and 2.55-second identical incremental build.
- Follow-Up Actions: Promote named static/runtime option values into an
  explicit labeled dropdown before adding mapping-preset apply/edit controls.

## 2026-07-24 - artist-sdk - runtime_option_provider_resolution

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Browser option-provider ownership
- Summary: Added a revisioned runtime option-provider registry, registered the
  app-owned transport BPM choices, resolved them in read-only package
  inspection, and preserved/marked defaults that disappear from a provider.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package-only and combined
  catalog/manifest gates, all 18 BrowserFlow scenarios, Signal Bloom offscreen
  bench, Release x64 build, and 1.92-second identical incremental build.
- Follow-Up Actions: Add explicit package preset-bank selection with the
  locked value precedence before mapping-preset apply/edit controls.

## 2026-07-24 - artist-sdk - package_vertical_slice_convergence

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Package declaration convergence and dynamic-option fixture
- Summary: Added deterministic package-to-runtime-adapter generation, replaced
  the remaining Signal Bloom adapter duplication with a checkable output,
  added package-owned dynamic option metadata, encoded preservation of
  unavailable stored values, and rendered named choices plus unresolved
  provider state in read-only Browser inspection rows.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: All 13 public-app contracts, package-only and combined
  catalog/manifest gates, inspection schemas, all 18 BrowserFlow scenarios,
  Signal Bloom offscreen bench, Release x64 builds, 2.14-second incremental
  rebuild, payload immutability assertion, and live-window Signal Bloom/Aurora
  Veil loading.
- Follow-Up Actions: Add an explicit runtime option-provider registry and
  resolve `transport.bpmMultipliers` while preserving unavailable stored
  values.

## 2026-07-19 - artist-sdk - package_vertical_slice

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Safe package vertical slice
- Summary: Added a focused package check command, manifest-only Browser
  inspection rows, disabled-by-default source-registered Signal Bloom
  activation, deterministic preset/override precedence, suggestion-only
  mappings, and a native headless/offscreen lifecycle bench.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `synaptome-layer check`; `LayerPackageBench.exe`; all 18
  `BrowserFlowTest` scenarios; package, catalog, manifest, schema, and
  public-app contract gates.
- Follow-Up Actions: Converge package/catalog/runtime declarations, then add
  one package-owned dynamic option source before further Browser promotion.

## 2026-07-19 - media - aurora_veil_public_media

- Request ID: `aurora_veil_public_media`
- Phase / Milestone: First reviewed public media asset
- Summary: Generated and reviewed one abstract aurora source, encoded it as a
  12-second 1920x1080 H.264 loop, recorded the prompt/encoding/license and
  SHA-256, added it to the manifest, and replaced the dangling default media
  layer reference without enabling folder scanning.
- Request Doc: `docs/project_ops/completed/aurora_veil_public_media.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python tools\media_catalog_regression.py --check`; `python
  tools\validate_configs.py synaptome\bin\data\config\videos.json`; `python
  tools\validate_configs.py --public-app`; FFmpeg stream inspection.
- Follow-Up Actions: Keep the catalog at one reviewed asset until a specific
  artistic requirement justifies another bounded intake request.

## 2026-07-18 - contracts - media_manifest_intake_contract

- Request ID: `media_manifest_intake_contract`
- Phase / Milestone: Safe media manifest intake complete
- Summary: Locked media discovery to explicit manifests, replaced the dangling
  `default-loop` reference with a valid empty baseline, separated public and
  operator-local roots, required stable IDs, revisions, SHA-256, provenance,
  generated-media metadata, redistribution permission, and replacement
  history, and added dependency-free semantic validation plus negative
  fixtures. The public-app report now covers 12 validated contracts.
- Request Doc: `docs/project_ops/completed/media_manifest_intake_contract.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python tools\media_catalog_regression.py --check`; package and
  combined catalog/manifest checks; Browser inspection payload check;
  `python tools\validate_configs.py --public-app`;
  `python tools\validate_synaptome_extraction_manifest.py --check --strict-review`.
- Follow-Up Actions: Open one bounded request for one reviewed redistributable
  asset, then add Browser visibility and runtime slot-load evidence without
  introducing folder scanning.

## 2026-07-18 - docs - roadmap_pre_media_gate

- Request ID: `layer_package_compatibility_bench_scaffolding`
- Phase / Milestone: Roadmap reconciliation and pre-media cleanup
- Summary: Made the Project Ops roadmap authoritative for priority, reconciled
  the active layer-package request with its task graph, labeled supporting and
  show-development roadmaps as active support, planned, parked, ready for
  closeout, or historical, and added a concrete safety gate for media policy,
  stable IDs, provenance, deterministic fixtures, and canonical contract
  preservation before more tracked media is generated. Updated the local
  request template to match the pinned Project Ops readiness contract.
- Request Doc: `docs/project_ops/in_progress/layer_package_compatibility_bench_scaffolding.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: Project Ops adapter and active-request audits; package,
  generated-layer, inspection-payload, combined catalog/manifest, canonical
  public-app contract, relative Markdown link, and `git diff --check`
  validation all passed.
- Follow-Up Actions: Finish one package-owned option-metadata slice, decide
  CG-08 media discovery/intake policy, record green validation evidence, then
  open one bounded media request.

## 2026-05-05 - contracts - osc_route_glob_regression
- Phase / Milestone: OSC contract hardening
- Summary: Added the OSC route glob validator to Synaptome's Project Ops validation ladder so built-in mesh-style OSC route coverage is part of the local administrative gate, not only the public contract report.
- Request Doc: `docs/project_ops/roadmap.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python tools\validate_osc_route_patterns.py`; `python tools\validate_configs.py --public-app`
- Follow-Up Actions: Keep route-pattern checks in sync with future mesh OSC contract revisions.

## 2026-05-05 - governance - schema_ownership_cleanup
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Replaced Synaptome's Project Ops adapter schema reference and local menu schema ID with repo-owned, versioned schema namespace IDs. Synaptome now consumes the Project Ops `v0.1.2` schema namespace while keeping Synaptome-owned schemas under the Synaptome `v0.1.0` namespace; Synaptome workflows and adapter metadata are pinned to Project Ops `v0.1.2`.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python -m json.tool .project_ops\config.json`; `python -m json.tool schemas\menu.schema.json`; `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `python tools\validate_configs.py --public-app`; repository-wide schema host scan passed with no raw GitHub or placeholder-local IDs; `git diff --check -- .project_ops/config.json schemas/menu.schema.json docs/project_ops/reports/changelog.md`.
- Follow-Up Actions: Keep future schema identity changes on repo-owned, versioned namespaces; do not reintroduce raw GitHub branch URLs or placeholder local schema hosts.

## 2026-05-05 - governance - project_ops_v0_1_1_pin
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Pinned Synaptome's Project Ops workflow checkouts, adapter schema URL, and adapter metadata to Project Ops `v0.1.1` instead of moving `main`.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `git diff --check`.
- Follow-Up Actions: Cut a new Project Ops tag before changing reusable audit/schema behavior consumed by Synaptome.

## 2026-05-05 - governance - project_ops_remote_request_audit
- Phase / Milestone: Project Ops compatibility hardening
- Summary: Added a Synaptome Project Ops changed-request audit workflow that checks out `tensegrity-audio/project_ops` on GitHub Actions and runs `project_ops_request_audit.py` against changed `docs/project_ops/(in_progress|completed)/*.md` records. Added Project Ops request audit to contributor/local validation and pruned stale Tensegrity process-contract entries from Synaptome's full contract report so `validate_configs.py --contracts` is public-runtime-owned.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo .`; `python ..\project_ops\tools\project_ops_request_audit.py --repo . --request-id project_ops_compatibility`; `python tools\validate_configs.py --contracts`; `python tools\validate_configs.py --public-app`; `python tools\validate_release_metadata.py`; `python tools\check_app_independence.py`; `python -m py_compile tools\validate_configs.py`.
- Follow-Up Actions: Push Synaptome so remote CI has both Project Ops adapter and request-audit coverage.

## 2026-05-05 - governance - project_ops_ci_audit
- Phase / Milestone: Project Ops CI adoption
- Summary: Updated Synaptome CI to check out `tensegrity-audio/project_ops` and run the reusable Project Ops adapter audit before public runtime validation.
- Request Doc: `docs/project_ops/roadmap.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; `python ..\project_ops\tools\project_ops_request_audit.py --repo ..\synaptome --request-id project_ops_compatibility`
- Follow-Up Actions: Use Project Ops request artifacts for future substantial runtime, contract, release, and docs work.

## 2026-05-04 - governance - project_ops_compatibility

- Phase / Milestone: Project Ops compatibility complete
- Summary: Added the namespaced `docs/project_ops/**` operating surface and updated `.project_ops/config.json` so Synaptome audits cleanly as a real Project Ops adopter without duplicating public runtime docs.
- Request Doc: `docs/project_ops/completed/project_ops_compatibility.md`
- Roadmap Entry: `docs/project_ops/roadmap.md`
- Validation: `python ..\project_ops\tools\project_ops_audit.py --repo ..\synaptome`; `python ..\project_ops\tools\project_ops_request_audit.py --repo ..\synaptome --request-id project_ops_compatibility`; `python tools\validate_release_metadata.py`; `python tools\validate_configs.py --public-app`; `python tools\check_app_independence.py`
- Follow-Up Actions: Use Project Ops request artifacts for future substantial runtime, contract, release, and docs work.

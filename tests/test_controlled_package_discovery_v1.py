from __future__ import annotations

import json
import shutil
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import controlled_package_discovery_v1 as discovery  # noqa: E402


PACKAGE_FIXTURE = ROOT / "docs/examples/layer_packages/signal_bloom"
CONTENT_FIXTURE = ROOT / "docs/examples/generated_layers"


def config(*roots: discovery.DiscoveryRoot, enabled: bool = True):
    return discovery.DiscoveryConfig(enabled=enabled, roots=tuple(roots))


def package_root(path: Path, root_id: str = "packages"):
    return discovery.DiscoveryRoot(root_id, "packages", path)


def content_root(path: Path, root_id: str = "content"):
    return discovery.DiscoveryRoot(root_id, "generated-stl", path)


def fixture_config() -> discovery.DiscoveryConfig:
    return config(package_root(PACKAGE_FIXTURE.parent), content_root(CONTENT_FIXTURE))


def by_kind(snapshot: discovery.DiscoverySnapshot, kind: str):
    return next(item for item in snapshot.candidates if item.kind == kind)


def test_absent_disabled_missing_empty_and_valid_roots(tmp_path: Path) -> None:
    assert discovery.load_config(None) == discovery.DiscoveryConfig()
    disabled = discovery.refresh(config(package_root(tmp_path), enabled=False))
    assert not disabled.enabled
    assert disabled.candidates == ()

    missing = discovery.refresh(config(package_root(tmp_path / "missing")))
    assert missing.candidates == ()
    assert missing.diagnostics[0]["code"] == "discovery.root-missing"

    empty = tmp_path / "empty"
    empty.mkdir()
    assert discovery.refresh(config(package_root(empty))).candidates == ()

    valid = discovery.refresh(fixture_config())
    assert {item.kind for item in valid.candidates} == {"package", "generated-stl"}
    assert {item.status for item in valid.candidates} == {"available"}
    assert all(item.activatable for item in valid.candidates)


@pytest.mark.parametrize(
    "document,message",
    [
        ([], "root must be an object"),
        ({"schemaVersion": 2, "enabled": False, "roots": []}, "schemaVersion 1"),
        ({"schemaVersion": 1, "enabled": "yes", "roots": []}, "must be boolean"),
        (
            {
                "schemaVersion": 1,
                "enabled": False,
                "roots": [{"id": "x", "kind": "anything", "path": "."}],
            },
            "packages or generated-stl",
        ),
        (
            {
                "schemaVersion": 1,
                "enabled": False,
                "roots": [
                    {"id": "x", "kind": "packages", "path": "."},
                    {"id": "x", "kind": "packages", "path": "."},
                ],
            },
            "duplicate discovery root",
        ),
    ],
)
def test_config_is_closed_and_versioned(
    tmp_path: Path, document: object, message: str
) -> None:
    with pytest.raises(discovery.DiscoveryConfigError, match=message):
        discovery.parse_config(document, base=tmp_path)


def test_signatures_and_ids_do_not_depend_on_absolute_root(tmp_path: Path) -> None:
    first_packages = tmp_path / "machine-a/packages"
    second_packages = tmp_path / "machine-b/packages"
    first_content = tmp_path / "machine-a/content"
    second_content = tmp_path / "machine-b/content"
    shutil.copytree(PACKAGE_FIXTURE.parent, first_packages)
    shutil.copytree(PACKAGE_FIXTURE.parent, second_packages)
    shutil.copytree(CONTENT_FIXTURE, first_content)
    shutil.copytree(CONTENT_FIXTURE, second_content)
    first = discovery.refresh(
        config(package_root(first_packages), content_root(first_content))
    )
    second = discovery.refresh(
        config(package_root(second_packages), content_root(second_content))
    )
    assert [(x.candidate_id, x.signature) for x in first.candidates] == [
        (x.candidate_id, x.signature) for x in second.candidates
    ]
    assert all(str(tmp_path) in item.local_path for item in first.candidates)
    for candidate in first.candidates:
        assert str(tmp_path) not in candidate.candidate_id
        assert str(tmp_path) not in candidate.signature


def test_overlapping_roots_coalesce_one_physical_manifest() -> None:
    snapshot = discovery.refresh(
        config(
            package_root(PACKAGE_FIXTURE.parent, "outer"),
            package_root(PACKAGE_FIXTURE, "inner"),
        )
    )
    assert len(snapshot.candidates) == 1
    assert snapshot.candidates[0].root_ids == ("inner", "outer")
    assert snapshot.candidates[0].status == "available"


def test_distinct_duplicate_claimants_all_fail_closed(tmp_path: Path) -> None:
    root = tmp_path / "packages"
    shutil.copytree(PACKAGE_FIXTURE, root / "one")
    shutil.copytree(PACKAGE_FIXTURE, root / "two")
    snapshot = discovery.refresh(config(package_root(root)))
    assert len(snapshot.candidates) == 2
    assert {item.status for item in snapshot.candidates} == {"duplicate"}
    assert all(
        any(d["code"] == "discovery.duplicate" for d in item.diagnostics)
        for item in snapshot.candidates
    )


def test_malformed_future_escape_missing_and_wrong_case_are_isolated(
    tmp_path: Path,
) -> None:
    root = tmp_path / "packages"
    shutil.copytree(PACKAGE_FIXTURE, root / "valid")
    shutil.copytree(PACKAGE_FIXTURE, root / "hostile")
    shutil.copytree(PACKAGE_FIXTURE, root / "future")
    hostile_path = root / "hostile/layer.package.json"
    hostile = json.loads(hostile_path.read_text(encoding="utf-8"))
    hostile["packageId"] = "hostile.escape"
    hostile["element"]["id"] = "hostile.escape"
    hostile["asset"]["id"] = "hostile.escape"
    hostile["asset"]["type"] = "hostile.escape"
    hostile["asset"]["registryPrefix"] = "hostile.escape"
    hostile["source"]["files"][0] = "../escape.cpp"
    hostile["presets"][0]["path"] = "presets/DEFAULT.json"
    hostile_path.write_text(json.dumps(hostile), encoding="utf-8")
    future_path = root / "future/layer.package.json"
    future = json.loads(future_path.read_text(encoding="utf-8"))
    future["packageId"] = "future.package"
    future["element"]["id"] = "future.package"
    future["asset"]["id"] = "future.package"
    future["asset"]["type"] = "future.package"
    future["asset"]["registryPrefix"] = "future.package"
    future["schemaVersion"] = 2
    future_path.write_text(json.dumps(future), encoding="utf-8")
    malformed_dir = root / "malformed"
    malformed_dir.mkdir()
    (malformed_dir / "layer.package.json").write_text("{", encoding="utf-8")

    snapshot = discovery.refresh(config(package_root(root)))
    statuses = [item.status for item in snapshot.candidates]
    assert "available" in statuses
    assert "invalid" in statuses
    assert "incompatible" in statuses
    assert all(item.status != "available" or item.registered_type_available
               for item in snapshot.candidates)


def test_symlinked_directory_cannot_escape_a_discovery_root(
    tmp_path: Path,
) -> None:
    root = tmp_path / "root"
    outside = tmp_path / "outside"
    root.mkdir()
    shutil.copytree(PACKAGE_FIXTURE, outside)
    try:
        (root / "escape").symlink_to(outside, target_is_directory=True)
    except OSError as exc:
        pytest.skip(f"directory symlinks are unavailable: {exc}")
    snapshot = discovery.refresh(config(package_root(root)))
    assert snapshot.candidates == ()


def test_missing_registered_type_is_visible_but_not_activatable() -> None:
    packages, types = discovery.controlled_registrations()
    package_snapshot = discovery.refresh(
        config(package_root(PACKAGE_FIXTURE.parent)),
        registrations=({}, types),
    )
    package = by_kind(package_snapshot, "package")
    assert package.status == "inspection-only"
    assert not package.activatable

    content_snapshot = discovery.refresh(
        config(content_root(CONTENT_FIXTURE)),
        registrations=(packages, {}),
    )
    content = by_kind(content_snapshot, "generated-stl")
    assert content.status == "unavailable"
    assert not content.activatable


def test_same_version_drift_replacement_and_deletion_lifecycle(
    tmp_path: Path,
) -> None:
    root = tmp_path / "content"
    shutil.copytree(CONTENT_FIXTURE, root)
    cfg = config(content_root(root))
    initial = discovery.refresh(cfg)
    original = by_kind(initial, "generated-stl")

    stl = root / "stl_models/tetrahedron.stl"
    stl.write_bytes(stl.read_bytes() + b"\n")
    drift = discovery.refresh(cfg, prior=initial)
    changed = by_kind(drift, "generated-stl")
    assert changed.status == "duplicate"
    assert changed.previous_signature == original.signature

    sidecar_path = root / "stl_models/tetrahedron.generated_layer.json"
    sidecar = json.loads(sidecar_path.read_text(encoding="utf-8"))
    sidecar["contentVersion"] = "1.1.0"
    sidecar_path.write_text(json.dumps(sidecar), encoding="utf-8")
    replacement = discovery.refresh(
        cfg,
        prior=initial,
        active_versions={
            original.candidate_id: (original.content_version, original.signature)
        },
    )
    replacement_candidate = by_kind(replacement, "generated-stl")
    assert replacement_candidate.status == "pending-replacement"
    assert replacement_candidate.registered_type_available

    stl.unlink()
    removed = discovery.refresh(cfg, prior=initial)
    stale_candidate = by_kind(removed, "generated-stl")
    assert stale_candidate.status == "unavailable"
    assert stale_candidate.signature == original.signature
    assert any(d["code"] == "discovery.removed" for d in stale_candidate.diagnostics)


def test_failed_refresh_keeps_prior_snapshot_readable() -> None:
    prior = discovery.refresh(fixture_config())

    def fail(*args, **kwargs):
        raise RuntimeError("hostile provider")

    stale = discovery.refresh_atomically(fixture_config(), prior=prior, scanner=fail)
    assert stale.stale
    assert stale.generation == prior.generation
    assert stale.candidates == prior.candidates
    assert stale.diagnostics[-1]["code"] == "discovery.refresh-failed"


def test_activation_requires_inspection_and_rolls_back_every_publication_stage() -> None:
    snapshot = discovery.refresh(fixture_config())
    candidate = by_kind(snapshot, "package")
    original_activation = {"schemaVersion": 1, "operator": "unchanged"}
    original_catalog = ({"id": "legacy"},)

    events: list[str] = []

    def persist(document):
        events.append("persist")
        return True

    def publish(document):
        events.append("catalog")
        return True

    def prepare(document):
        events.append("prepare")
        return {"prepared": document["id"]}

    def adopt(prepared):
        events.append("adopt")
        return True

    success = discovery.activate_candidate(
        snapshot,
        candidate.candidate_id,
        original_activation,
        original_catalog,
        persist_activation=persist,
        publish_catalog=publish,
        prepare_runtime=prepare,
        adopt_runtime=adopt,
    )
    assert success.ok
    assert events == ["persist", "catalog", "prepare", "adopt"]
    assert success.catalog[-1]["id"] == candidate.definition_id
    assert "mappingPresets" in candidate.metadata

    for failing_stage in ("persist", "catalog", "prepare", "adopt"):
        live = {"activation": dict(original_activation), "catalog": original_catalog}

        def staged_persist(document, stage=failing_stage):
            if stage == "persist" and document != original_activation:
                return False
            live["activation"] = dict(document)
            return True

        def staged_publish(document, stage=failing_stage):
            if stage == "catalog" and tuple(document) != original_catalog:
                return False
            live["catalog"] = tuple(document)
            return True

        def staged_prepare(document, stage=failing_stage):
            return None if stage == "prepare" else {"prepared": True}

        def staged_adopt(prepared, stage=failing_stage):
            return stage != "adopt"

        result = discovery.activate_candidate(
            snapshot,
            candidate.candidate_id,
            original_activation,
            original_catalog,
            persist_activation=staged_persist,
            publish_catalog=staged_publish,
            prepare_runtime=staged_prepare,
            adopt_runtime=staged_adopt,
        )
        assert not result.ok
        assert result.rollback_succeeded
        assert live["activation"] == original_activation
        assert live["catalog"] == original_catalog


def test_replacement_requires_explicit_acceptance_and_replaces_catalog_entry() -> None:
    initial = discovery.refresh(fixture_config())
    current = by_kind(initial, "generated-stl")
    replacement = discovery.DiscoverySnapshot(
        initial.generation + 1,
        True,
        False,
        (
            discovery._replace(
                current,
                status="pending-replacement",
                content_version="1.1.0",
                signature="replacement-signature",
            ),
        ),
    )
    prior_catalog = (
        {"id": "legacy"},
        {"id": current.definition_id, "version": "1.0.0"},
    )

    def succeed(*args):
        return True

    blocked = discovery.activate_candidate(
        replacement,
        current.candidate_id,
        {},
        prior_catalog,
        persist_activation=succeed,
        publish_catalog=succeed,
        prepare_runtime=lambda catalog: catalog,
        adopt_runtime=succeed,
    )
    assert not blocked.ok

    accepted = discovery.activate_candidate(
        replacement,
        current.candidate_id,
        {},
        prior_catalog,
        persist_activation=succeed,
        publish_catalog=succeed,
        prepare_runtime=lambda catalog: catalog,
        adopt_runtime=succeed,
        accept_replacement=True,
    )
    assert accepted.ok
    assert [item["id"] for item in accepted.catalog].count(
        current.definition_id
    ) == 1
    assert accepted.catalog[-1]["id"] == current.definition_id


def test_invalid_duplicate_and_stale_candidates_never_reach_runtime(tmp_path: Path) -> None:
    snapshot = discovery.refresh(fixture_config())
    available = by_kind(snapshot, "package")
    blocked = discovery.DiscoverySnapshot(
        snapshot.generation,
        snapshot.enabled,
        snapshot.stale,
        (discovery._replace(available, status="duplicate"),),
    )
    invoked = {"count": 0}

    def forbidden(*args):
        invoked["count"] += 1
        return True

    result = discovery.activate_candidate(
        blocked,
        available.candidate_id,
        {},
        (),
        persist_activation=forbidden,
        publish_catalog=forbidden,
        prepare_runtime=forbidden,
        adopt_runtime=forbidden,
    )
    assert not result.ok
    assert invoked["count"] == 0

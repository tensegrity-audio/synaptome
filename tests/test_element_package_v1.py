from __future__ import annotations

import copy
import json
import shutil
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import element_package_v1 as package_v1

REPO_ROOT = Path(__file__).resolve().parents[1]
REFERENCE_ROOT = (
    REPO_ROOT
    / "docs"
    / "examples"
    / "layer_packages"
    / "signal_bloom"
)


def fixture(tmp_path: Path) -> Path:
    destination = tmp_path / "signal_bloom"
    shutil.copytree(REFERENCE_ROOT, destination)
    return destination / "layer.package.json"


def mutate(path: Path, callback: object) -> package_v1.ValidationResult:
    document = json.loads(path.read_text(encoding="utf-8"))
    callback(document)  # type: ignore[operator]
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    return package_v1.validate_package(path)


def codes(result: package_v1.ValidationResult) -> set[str]:
    return {diagnostic.code for diagnostic in result.diagnostics}


def test_reference_package_normalizes_without_mutation() -> None:
    path = REFERENCE_ROOT / "layer.package.json"
    before = path.read_bytes()
    result = package_v1.validate_package(path)
    assert result.valid, [item.render() for item in result.diagnostics]
    assert path.read_bytes() == before
    assert result.normalized is not None
    assert result.normalized["typeId"] == "example.signalBloom"
    assert [group["id"] for group in result.normalized["parameterGroups"]] == [
        "example",
        "exampleMotion",
        "exampleTransform",
        "exampleColor",
        "exampleModulation",
    ]
    assert len(result.normalized["parameters"]) == 18
    assert all(
        item["applyMode"] == "suggestion-only"
        for item in result.inventory["mappings"]
    )


def test_future_schema_fails_closed(tmp_path: Path) -> None:
    result = mutate(
        fixture(tmp_path),
        lambda document: document.__setitem__("schemaVersion", 2),
    )
    assert "package.schema-version" in codes(result)


def test_unsupported_package_major_fails_closed(tmp_path: Path) -> None:
    result = mutate(
        fixture(tmp_path),
        lambda document: document.__setitem__("packageVersion", "2.0.0"),
    )
    assert "package.package-version-unsupported" in codes(result)


def test_duplicate_identity_has_stable_path(tmp_path: Path) -> None:
    def duplicate(document: dict[str, object]) -> None:
        parameters = document["parameters"]
        assert isinstance(parameters, list)
        parameters.append(copy.deepcopy(parameters[0]))

    result = mutate(fixture(tmp_path), duplicate)
    duplicate_errors = [
        item
        for item in result.diagnostics
        if item.code == "package.duplicate-id"
    ]
    assert duplicate_errors
    assert duplicate_errors[0].json_path == "$.parameters[18].id"


def test_unsafe_and_missing_references_fail(tmp_path: Path) -> None:
    unsafe = mutate(
        fixture(tmp_path / "unsafe"),
        lambda document: document["source"].__setitem__(  # type: ignore[index, union-attr]
            "registration", "../outside.cpp"
        ),
    )
    assert "package.reference-unsafe" in codes(unsafe)
    missing = mutate(
        fixture(tmp_path / "missing"),
        lambda document: document["tests"].__setitem__(  # type: ignore[index, union-attr]
            "confidenceProfile", "tests/missing.json"
        ),
    )
    assert "package.reference-missing" in codes(missing)


def test_unknown_capability_and_incompatible_dependency_fail(
    tmp_path: Path,
) -> None:
    def drift(document: dict[str, object]) -> None:
        capabilities = document["capabilities"]
        dependencies = document["dependencies"]
        assert isinstance(capabilities, list)
        assert isinstance(dependencies, list)
        capabilities.append("devices.webcam.select")
        dependencies[0]["resolvedVersion"] = "2.0.0"

    result = mutate(fixture(tmp_path), drift)
    assert {
        "package.capability-unknown",
        "package.dependency-version",
    }.issubset(codes(result))


def test_dependency_cycle_and_unresolved_edge_fail(tmp_path: Path) -> None:
    def cycle(document: dict[str, object]) -> None:
        dependencies = document["dependencies"]
        assert isinstance(dependencies, list)
        dependencies[0]["dependsOn"] = ["synaptome.runtime"]
        dependencies[1]["dependsOn"] = ["synaptome.element-sdk"]
        dependencies.append(
            {
                "id": "examples.missing",
                "kind": "package",
                "required": True,
                "versionRange": ">=1.0.0 <2.0.0",
                "resolvedVersion": "1.0.0",
                "provider": "examples.missing",
                "dependsOn": ["examples.notDeclared"],
            }
        )

    result = mutate(fixture(tmp_path), cycle)
    assert {
        "package.dependency-cycle",
        "package.dependency-unresolved",
    }.issubset(codes(result))


def test_optional_absence_is_reported_without_failure(tmp_path: Path) -> None:
    def optional(document: dict[str, object]) -> None:
        dependencies = document["dependencies"]
        assert isinstance(dependencies, list)
        dependencies.append(
            {
                "id": "examples.optional-asset",
                "kind": "asset",
                "required": False,
                "versionRange": ">=1.0.0 <2.0.0",
                "resolvedVersion": "1.0.0",
                "path": "media/optional.bin",
            }
        )

    result = mutate(fixture(tmp_path), optional)
    assert result.valid
    assert result.resolved_dependencies[-1]["status"] == "optional-absent"


def test_optional_section_omission_differs_from_explicit_empty(
    tmp_path: Path,
) -> None:
    path = fixture(tmp_path)
    result = mutate(path, lambda document: document.pop("aliases"))
    assert result.valid
    presence = {
        item["section"]: item["status"]
        for item in result.inventory["sectionPresence"]
    }
    assert presence["aliases"] == "omitted"
    assert presence["deprecations"] == "present-empty"


def test_ambiguous_and_non_forward_migrations_fail(tmp_path: Path) -> None:
    path = fixture(tmp_path)
    migration_dir = path.parent / "migrations"
    migration_dir.mkdir()
    (migration_dir / "a.json").write_text("{}\n", encoding="utf-8")
    (migration_dir / "b.json").write_text("{}\n", encoding="utf-8")

    def migrations(document: dict[str, object]) -> None:
        document["migrations"] = [
            {
                "id": "first",
                "fromVersion": "0.1.0",
                "toVersion": "1.0.0",
                "path": "migrations/a.json",
            },
            {
                "id": "second",
                "fromVersion": "0.1.0",
                "toVersion": "0.1.0",
                "path": "migrations/b.json",
            },
        ]

    result = mutate(path, migrations)
    assert {
        "package.migration-ambiguous",
        "package.migration-order",
    }.issubset(codes(result))


def test_exact_parity_reports_order_and_field_drift() -> None:
    result = package_v1.validate_package(
        REFERENCE_ROOT / "layer.package.json"
    )
    assert result.normalized is not None
    runtime = copy.deepcopy(result.normalized)
    runtime["parameters"][0]["label"] = "Drifted"
    diagnostics = package_v1.compare_descriptors(
        "examples.signal_bloom", result.normalized, runtime
    )
    assert diagnostics[0].json_path == "$.normalized.parameters[0].label"
    assert diagnostics[0].runtime_field == (
        "ElementTypeContract.parameters[0].label"
    )
    assert diagnostics[0].expected == "Visible"
    assert diagnostics[0].observed == "Drifted"
    reordered = copy.deepcopy(result.normalized)
    reordered["parameterGroups"].reverse()
    assert package_v1.compare_descriptors(
        "examples.signal_bloom", result.normalized, reordered
    )


def test_activation_set_rejects_conflicting_stable_identities() -> None:
    result = package_v1.validate_package(
        REFERENCE_ROOT / "layer.package.json"
    )
    diagnostics = package_v1.validate_activation_set([result, result])
    assert len(diagnostics) == 4
    assert {item.code for item in diagnostics} == {
        "package.activation-conflict"
    }

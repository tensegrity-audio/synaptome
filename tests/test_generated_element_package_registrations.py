from __future__ import annotations

import json
import shutil
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import generate_element_package_registrations as generator  # noqa: E402


REFERENCE_PACKAGE = (
    ROOT / "docs/examples/layer_packages/signal_bloom"
)


def _registration_set(
    tmp_path: Path,
    references: list[str],
) -> Path:
    path = tmp_path / "registration-set.json"
    path.write_text(
        json.dumps(
            {"schemaVersion": 1, "packages": references}
        ),
        encoding="utf-8",
    )
    return path


def _copy_package(tmp_path: Path, name: str) -> Path:
    destination = tmp_path / name
    shutil.copytree(REFERENCE_PACKAGE, destination)
    return destination / "layer.package.json"


def test_reference_record_is_validated_and_complete() -> None:
    records = generator.load_records()
    assert len(records) == 1
    record = records[0]
    assert record.package_id == "examples.signal_bloom"
    assert record.type_id == "example.signalBloom"
    assert record.binding_mode == "bind-only"
    assert record.symbol == (
        "synaptomeCreateElementPackage_examples_signal_bloom"
    )
    assert record.descriptor_signature == (
        "39e6e7934d09689bd1a952e2d040f3a36ebdf595a47446778d1745a2fcdb00de"
    )
    assert [path.name for path in record.compile_paths] == [
        "SignalBloomLayer.cpp",
        "register_signal_bloom.cpp",
    ]


def test_checked_in_outputs_are_deterministic_and_current() -> None:
    first = generator.generated_outputs()
    second = generator.generated_outputs()
    assert first == second
    assert generator.check_outputs(first) == []


def test_generated_registration_preflights_before_mutation() -> None:
    source = generator.generated_outputs()[generator.GENERATED_SOURCE]
    preflight = source.index(
        'if (elementTypes.contains("example.signalBloom"))'
    )
    registration = source.index(
        "elementTypes.registerType("
    )
    creator = source.index(
        "synaptomeCreateElementPackage_examples_signal_bloom();",
        registration,
    )
    postcondition = source.index(
        '"generated registrar did not publish expected type: '
        'example.signalBloom"'
    )
    assert preflight < registration < creator < postcondition


def test_generated_parameter_order_matches_normalized_package() -> None:
    record = generator.load_records()[0]
    source = generator.generated_outputs()[generator.GENERATED_SOURCE]
    positions = [
        source.index(f'value.id = "{parameter["id"]}";')
        for parameter in record.normalized["parameters"]
    ]
    assert positions == sorted(positions)


def test_generated_target_owns_package_sources_without_project_edits() -> None:
    target = generator.generated_outputs()[generator.GENERATED_TARGETS]
    assert (
        "'$(SynaptomeEnableGeneratedElementPackages)'=='true'" in target
    )
    assert "SignalBloomLayer.cpp" in target
    assert "register_signal_bloom.cpp" in target
    assert "GeneratedElementPackageRegistrations.cpp" in target
    assert "Element_SignalBloom.vcxproj" not in target


def test_stale_output_is_rejected(tmp_path: Path) -> None:
    output = tmp_path / "generated.cpp"
    output.write_text("stale\n", encoding="utf-8")
    errors = generator.check_outputs({output: "current\n"})
    assert len(errors) == 1
    assert "is stale" in errors[0]
    assert "-stale" in errors[0]
    assert "+current" in errors[0]


def test_duplicate_registration_set_path_is_rejected(
    tmp_path: Path,
) -> None:
    reference = (
        "docs/examples/layer_packages/signal_bloom/layer.package.json"
    )
    registration_set = tmp_path / "registration-set.json"
    registration_set.write_text(
        json.dumps(
            {
                "schemaVersion": 1,
                "packages": [reference, reference],
            }
        ),
        encoding="utf-8",
    )
    with pytest.raises(
        generator.GenerationError,
        match="duplicate paths",
    ):
        generator.load_records(registration_set)


def test_duplicate_package_identity_is_rejected_before_generation(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    first = _copy_package(tmp_path, "first")
    second = _copy_package(tmp_path, "second")
    paths = {"first": first, "second": second}
    monkeypatch.setattr(
        generator,
        "_resolve_repo_reference",
        lambda raw, context: paths[str(raw)],
    )
    registration_set = _registration_set(
        tmp_path, ["first", "second"]
    )
    with pytest.raises(
        generator.GenerationError,
        match="registration set conflicts",
    ):
        generator.load_records(registration_set)


def test_unresolved_required_package_dependency_is_rejected(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    package_path = _copy_package(tmp_path, "package")
    package = json.loads(package_path.read_text(encoding="utf-8"))
    package["dependencies"].append(
        {
            "id": "missing.package",
            "kind": "package",
            "required": True,
            "versionRange": ">=0.1.0 <1.0.0",
            "resolvedVersion": "0.1.0",
            "provider": "missing.package",
        }
    )
    package_path.write_text(json.dumps(package), encoding="utf-8")
    monkeypatch.setattr(
        generator,
        "_resolve_repo_reference",
        lambda raw, context: package_path,
    )
    registration_set = _registration_set(tmp_path, ["package"])
    with pytest.raises(
        generator.GenerationError,
        match="required package dependency 'missing.package'",
    ):
        generator.load_records(registration_set)


def test_creator_symbol_drift_is_rejected(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    package_path = _copy_package(tmp_path, "package")
    creator = package_path.parent / "source/register_signal_bloom.cpp"
    creator.write_text(
        creator.read_text(encoding="utf-8").replace(
            "synaptomeCreateElementPackage_examples_signal_bloom",
            "wrongCreator",
        ),
        encoding="utf-8",
    )
    monkeypatch.setattr(
        generator,
        "_resolve_repo_reference",
        lambda raw, context: package_path,
    )
    registration_set = _registration_set(tmp_path, ["package"])
    with pytest.raises(
        generator.GenerationError,
        match="must define std::unique_ptr<Layer>",
    ):
        generator.load_records(registration_set)


@pytest.mark.parametrize(
    "reference",
    [
        "../outside/layer.package.json",
        r"docs\examples\layer.package.json",
        "/absolute/layer.package.json",
    ],
)
def test_unsafe_registration_set_path_is_rejected(
    tmp_path: Path,
    reference: str,
) -> None:
    registration_set = tmp_path / "registration-set.json"
    registration_set.write_text(
        json.dumps(
            {
                "schemaVersion": 1,
                "packages": [reference],
            }
        ),
        encoding="utf-8",
    )
    with pytest.raises(generator.GenerationError):
        generator.load_records(registration_set)


def test_registration_symbol_is_stable() -> None:
    assert generator.registration_symbol("examples.signal_bloom") == (
        "synaptomeCreateElementPackage_examples_signal_bloom"
    )

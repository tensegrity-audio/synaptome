from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))

import element_confidence as confidence


def load_runner():
    spec = importlib.util.spec_from_file_location(
        "run_element_confidence", TOOLS / "run_element_confidence.py"
    )
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_frozen_profiles_load_and_preflight() -> None:
    grid = confidence.load_profile(
        confidence.PROFILE_ROOT / "grid.json"
    )
    resolved, errors = confidence.preflight_dependencies(grid)
    assert not errors
    assert len(resolved) == len(grid["dependencies"])

    package_path = (
        ROOT
        / "docs"
        / "examples"
        / "layer_packages"
        / "signal_bloom"
        / "layer.package.json"
    )
    package = confidence.load_json_object(package_path, "package")
    _, signal_bloom = confidence.find_profile_for_package(package)
    resolved, errors = confidence.preflight_dependencies(
        signal_bloom, package_path=package_path, package=package
    )
    assert not errors
    assert any(item["kind"] == "test-profile" for item in resolved)


def test_duplicate_dependency_has_stable_diagnostic(tmp_path: Path) -> None:
    dependency = tmp_path / "source.cpp"
    dependency.write_text("// fixture\n", encoding="utf-8")
    profile = {
        "dependencies": [
            {"kind": "source", "id": "same", "path": str(dependency)},
            {"kind": "source", "id": "same", "path": str(dependency)},
        ]
    }
    _, errors = confidence.preflight_dependencies(profile)
    assert errors == [
        "dependency.duplicate-id: 'same' at indexes 0 and 1",
        f"dependency.duplicate-target: source '{dependency.as_posix()}' at indexes 0 and 1",
    ]


def test_static_runner_writes_passing_report(tmp_path: Path) -> None:
    runner = load_runner()
    destination = tmp_path / "grid-report.json"
    exit_code = runner.main(
        ["--profile", "grid", "--tier", "static", "--report", str(destination)]
    )
    report = json.loads(destination.read_text(encoding="utf-8"))
    assert exit_code == 0
    assert report["schemaVersion"] == 1
    assert report["overallResult"] == "pass"
    assert report["elementType"] == "grid"
    assert report["stateSignatures"] == []


def test_ci_runner_fails_required_unimplemented_tiers(tmp_path: Path) -> None:
    runner = load_runner()
    destination = tmp_path / "grid-ci-report.json"
    source = confidence.load_profile(confidence.PROFILE_ROOT / "grid.json")
    source["dependencies"] = []
    source["validators"] = []
    source.pop("contractHarness")
    profile_path = tmp_path / "pending-profile.json"
    profile_path.write_text(json.dumps(source), encoding="utf-8")
    exit_code = runner.main(
        [
            "--profile",
            str(profile_path),
            "--tier",
            "ci",
            "--report",
            str(destination),
        ]
    )
    report = json.loads(destination.read_text(encoding="utf-8"))
    required_skips = [
        check
        for check in report["checks"]
        if check["required"] and check["status"] == "skip"
    ]
    assert exit_code == 1
    assert report["overallResult"] == "fail"
    assert [check["tier"] for check in required_skips] == [1, 2, 3]


def test_reviewed_baseline_enforces_pixel_and_p95(tmp_path: Path) -> None:
    profile = confidence.load_profile(
        confidence.PROFILE_ROOT / "grid.json"
    )
    report = confidence.new_report(profile, "ci")
    report.graphics.update(
        {
            "vendor": "Test Vendor",
            "renderer": "Test Renderer",
            "version": "1.0",
            "pixelSignature": "a" * 64,
        }
    )
    report.timings = {
        "updateMs": {"median": 0.5, "p95": 1.1, "maximum": 1.2},
        "drawMs": {"median": 1.0, "p95": 2.2, "maximum": 2.4},
    }
    baseline = {
        "schemaVersion": 1,
        "entries": [
            {
                "id": "test-renderer",
                "profileId": "grid",
                "rendererClass": {
                    "system": report.platform["system"],
                    "machine": report.platform["machine"],
                    "vendor": "Test Vendor",
                    "renderer": "Test Renderer",
                    "version": "1.0",
                },
                "pixelSignature": "a" * 64,
                "timings": {
                    "updateP95Ms": 1.0,
                    "drawP95Ms": 2.0,
                },
                "review": {
                    "approvedBy": "test",
                    "approvedUtc": "2026-07-28T00:00:00Z",
                    "sourceCommit": "abc123",
                    "reason": "fixture",
                },
            }
        ],
    }
    path = tmp_path / "baseline.json"
    path.write_text(json.dumps(baseline), encoding="utf-8")
    checks = confidence.compare_reviewed_baseline(report, path)
    assert [check.status for check in checks] == ["pass", "pass"]
    report.timings["drawMs"]["p95"] = 2.5
    checks = confidence.compare_reviewed_baseline(report, path)
    assert checks[1].status == "fail"

    report.timings["updateMs"]["p95"] = None
    report.timings["drawMs"]["p95"] = None
    checks = confidence.compare_reviewed_baseline(report, path)
    assert checks[1].status == "skip"
    assert not checks[1].required

"""Shared model, profile loading, and dependency preflight for SEAC-6."""
from __future__ import annotations

import json
import hashlib
import os
import platform
import subprocess
import tempfile
import time
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]
PROFILE_ROOT = REPO_ROOT / "tools" / "element_confidence_profiles"
REPORT_SCHEMA_VERSION = 1


class ConfidenceError(ValueError):
    """A stable, user-facing confidence-suite configuration error."""


@dataclass
class CheckResult:
    check_id: str
    tier: int
    status: str
    duration_ms: float
    diagnostic: str
    required: bool = True

    def to_json(self) -> dict[str, Any]:
        return {
            "id": self.check_id,
            "tier": self.tier,
            "status": self.status,
            "required": self.required,
            "durationMs": round(self.duration_ms, 3),
            "diagnostic": self.diagnostic,
        }


@dataclass
class ConfidenceReport:
    timestamp_utc: str
    commit: str
    configuration: str
    platform: dict[str, str]
    tier: str
    element_type: str
    package_profile_id: str
    binding_mode: str
    deterministic_inputs: dict[str, Any]
    resolved_dependencies: list[dict[str, Any]] = field(default_factory=list)
    declaration_live_comparison: dict[str, Any] = field(
        default_factory=lambda: {"status": "not-run", "diagnostic": ""}
    )
    state_signatures: list[str] = field(default_factory=list)
    graphics: dict[str, Any] = field(
        default_factory=lambda: {
            "renderer": None,
            "vendor": None,
            "version": None,
            "nonblank": None,
            "pixelSignature": None,
            "leakedState": [],
        }
    )
    reload: dict[str, Any] = field(
        default_factory=lambda: {
            "count": 0,
            "warmWorkingSetBytes": None,
            "finalWorkingSetBytes": None,
            "growthSlopeBytesPerReload": None,
        }
    )
    timings: dict[str, Any] = field(
        default_factory=lambda: {
            "updateMs": {"median": None, "p95": None, "maximum": None},
            "drawMs": {"median": None, "p95": None, "maximum": None},
        }
    )
    checks: list[CheckResult] = field(default_factory=list)

    @property
    def overall_result(self) -> str:
        return (
            "fail"
            if any(
                check.required and check.status in {"fail", "skip"}
                for check in self.checks
            )
            else "pass"
        )

    def to_json(self) -> dict[str, Any]:
        return {
            "schemaVersion": REPORT_SCHEMA_VERSION,
            "timestampUtc": self.timestamp_utc,
            "commit": self.commit,
            "configuration": self.configuration,
            "platform": self.platform,
            "tier": self.tier,
            "elementType": self.element_type,
            "packageProfileId": self.package_profile_id,
            "bindingMode": self.binding_mode,
            "resolvedDependencies": self.resolved_dependencies,
            "declarationLiveComparison": self.declaration_live_comparison,
            "deterministicInputs": self.deterministic_inputs,
            "stateSignatures": self.state_signatures,
            "graphics": self.graphics,
            "reload": self.reload,
            "timings": self.timings,
            "checks": [check.to_json() for check in self.checks],
            "overallResult": self.overall_result,
        }


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace(
        "+00:00", "Z"
    )


def git_commit() -> str:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=REPO_ROOT,
        check=False,
        capture_output=True,
        text=True,
    )
    return completed.stdout.strip() if completed.returncode == 0 else "unknown"


def platform_info() -> dict[str, str]:
    return {
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "python": platform.python_version(),
    }


def _require_nonempty_string(owner: dict[str, Any], key: str, context: str) -> str:
    value = owner.get(key)
    if not isinstance(value, str) or not value:
        raise ConfidenceError(f"{context}.{key} must be a non-empty string")
    return value


def load_json_object(path: Path, context: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ConfidenceError(f"{context} does not exist: {repo_path(path)}") from exc
    except json.JSONDecodeError as exc:
        raise ConfidenceError(
            f"{context} JSON is invalid: {repo_path(path)}:{exc.lineno}:{exc.colno}"
        ) from exc
    if not isinstance(value, dict):
        raise ConfidenceError(f"{context} root must be an object")
    return value


def profile_path(value: str) -> Path:
    supplied = Path(value)
    if supplied.is_absolute() or supplied.parent != Path(".") or supplied.suffix:
        return supplied if supplied.is_absolute() else REPO_ROOT / supplied
    return PROFILE_ROOT / f"{value}.json"


def load_profile(path: Path) -> dict[str, Any]:
    profile = load_json_object(path, "profile")
    if profile.get("schemaVersion") != 1:
        raise ConfidenceError("profile.schemaVersion must equal 1")
    for key in ("id", "elementType", "bindingMode"):
        _require_nonempty_string(profile, key, "profile")
    inputs = profile.get("inputs")
    if not isinstance(inputs, dict):
        raise ConfidenceError("profile.inputs must be an object")
    for key in (
        "viewportWidth",
        "viewportHeight",
        "updateFrames",
        "fixedStepSeconds",
        "bpm",
        "transportSpeed",
        "seed",
        "determinismRepetitions",
    ):
        if not isinstance(inputs.get(key), (int, float)) or isinstance(
            inputs.get(key), bool
        ):
            raise ConfidenceError(f"profile.inputs.{key} must be numeric")
    dependencies = profile.get("dependencies")
    if not isinstance(dependencies, list):
        raise ConfidenceError("profile.dependencies must be an array")
    for index, dependency in enumerate(dependencies):
        context = f"profile.dependencies[{index}]"
        if not isinstance(dependency, dict):
            raise ConfidenceError(f"{context} must be an object")
        _require_nonempty_string(dependency, "kind", context)
        _require_nonempty_string(dependency, "id", context)
        _require_nonempty_string(dependency, "path", context)
        if "contains" in dependency and not isinstance(dependency["contains"], str):
            raise ConfidenceError(f"{context}.contains must be a string")
    validators = profile.get("validators", [])
    if not isinstance(validators, list):
        raise ConfidenceError("profile.validators must be an array")
    for index, validator in enumerate(validators):
        context = f"profile.validators[{index}]"
        if not isinstance(validator, dict):
            raise ConfidenceError(f"{context} must be an object")
        _require_nonempty_string(validator, "id", context)
        command = validator.get("command")
        if not isinstance(command, list) or not command or not all(
            isinstance(part, str) and part for part in command
        ):
            raise ConfidenceError(f"{context}.command must be a non-empty string array")
    harness = profile.get("contractHarness")
    if harness is not None:
        if not isinstance(harness, dict):
            raise ConfidenceError("profile.contractHarness must be an object")
        _require_nonempty_string(harness, "project", "profile.contractHarness")
        _require_nonempty_string(
            harness, "executable", "profile.contractHarness"
        )
    return profile


def find_profile_for_package(package: dict[str, Any]) -> tuple[Path, dict[str, Any]]:
    package_id = package.get("packageId")
    matches: list[tuple[Path, dict[str, Any]]] = []
    for path in sorted(PROFILE_ROOT.glob("*.json")):
        candidate = load_profile(path)
        if candidate.get("packageId") == package_id:
            matches.append((path, candidate))
    if not matches:
        raise ConfidenceError(
            f"no confidence profile declares packageId '{package_id}'"
        )
    if len(matches) > 1:
        joined = ", ".join(repo_path(path) for path, _ in matches)
        raise ConfidenceError(
            f"duplicate confidence profiles for packageId '{package_id}': {joined}"
        )
    return matches[0]


def repo_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.resolve().as_posix()


def resolve_path(raw: str, base: Path = REPO_ROOT) -> Path:
    path = Path(raw)
    return path.resolve() if path.is_absolute() else (base / path).resolve()


def package_dependencies(
    package_path: Path, package: dict[str, Any]
) -> list[dict[str, str]]:
    dependencies: list[dict[str, str]] = [
        {"kind": "package", "id": "package-manifest", "path": str(package_path)}
    ]
    source = package.get("source", {})
    if isinstance(source, dict):
        registration = source.get("registration")
        if isinstance(registration, str) and registration:
            dependencies.append(
                {"kind": "registration-source", "id": "registration", "path": registration}
            )
        files = source.get("files", [])
        if isinstance(files, list):
            for index, raw in enumerate(files):
                if isinstance(raw, str) and raw:
                    dependencies.append(
                        {"kind": "source", "id": f"source-{index}", "path": raw}
                    )
    for index, raw in enumerate(package.get("presets", [])):
        if isinstance(raw, dict) and isinstance(raw.get("path"), str):
            dependencies.append(
                {
                    "kind": "asset",
                    "id": f"preset-{raw.get('presetId', index)}",
                    "path": raw["path"],
                }
            )
    tests = package.get("tests", {})
    if isinstance(tests, dict) and isinstance(tests.get("bench"), str):
        dependencies.append(
            {"kind": "test-profile", "id": "package-bench", "path": tests["bench"]}
        )
    return dependencies


def preflight_dependencies(
    profile: dict[str, Any],
    *,
    package_path: Path | None = None,
    package: dict[str, Any] | None = None,
) -> tuple[list[dict[str, Any]], list[str]]:
    raw_dependencies = list(profile["dependencies"])
    if package_path is not None and package is not None:
        raw_dependencies.extend(package_dependencies(package_path, package))

    resolved: list[dict[str, Any]] = []
    errors: list[str] = []
    seen_ids: dict[str, int] = {}
    seen_targets: dict[tuple[str, str], int] = {}
    for index, dependency in enumerate(raw_dependencies):
        dependency_id = str(dependency["id"])
        kind = str(dependency["kind"])
        base = (
            package_path.parent
            if package_path is not None and index >= len(profile["dependencies"])
            else REPO_ROOT
        )
        path = resolve_path(str(dependency["path"]), base)
        record: dict[str, Any] = {
            "id": dependency_id,
            "kind": kind,
            "path": repo_path(path),
            "status": "resolved",
        }
        if dependency_id in seen_ids:
            errors.append(
                f"dependency.duplicate-id: '{dependency_id}' at indexes "
                f"{seen_ids[dependency_id]} and {index}"
            )
            record["status"] = "duplicate"
        else:
            seen_ids[dependency_id] = index
        target_key = (kind, os.path.normcase(str(path)))
        if target_key in seen_targets:
            errors.append(
                f"dependency.duplicate-target: {kind} '{repo_path(path)}' at indexes "
                f"{seen_targets[target_key]} and {index}"
            )
            record["status"] = "duplicate"
        else:
            seen_targets[target_key] = index
        if not path.is_file():
            errors.append(
                f"dependency.missing: {kind} '{dependency_id}' -> {repo_path(path)}"
            )
            record["status"] = "missing"
        contains = dependency.get("contains")
        if isinstance(contains, str) and path.is_file():
            count = path.read_text(encoding="utf-8", errors="replace").count(contains)
            expected_count = dependency.get("expectedCount", 1)
            record["matchCount"] = count
            if count != expected_count:
                errors.append(
                    f"dependency.registry-count: '{dependency_id}' expected "
                    f"{expected_count}, found {count} in {repo_path(path)}"
                )
                record["status"] = "invalid"
        resolved.append(record)
    return resolved, errors


def command_argv(raw: list[str], package_path: Path | None) -> list[str]:
    import sys

    result: list[str] = []
    for part in raw:
        if part == "{python}":
            result.append(sys.executable)
        elif part == "{package}":
            if package_path is None:
                raise ConfidenceError("validator uses {package} without --package")
            result.append(str(package_path))
        else:
            result.append(part)
    return result


def run_validator(
    validator: dict[str, Any], package_path: Path | None
) -> CheckResult:
    started = time.perf_counter()
    argv = command_argv(validator["command"], package_path)
    try:
        completed = subprocess.run(
            argv,
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        output = (completed.stdout + completed.stderr).strip()
        status = "pass" if completed.returncode == 0 else "fail"
        diagnostic = (
            output.splitlines()[-1]
            if output
            else f"command exited {completed.returncode}"
        )
    except FileNotFoundError:
        status = "fail"
        diagnostic = f"executable not found: {argv[0]}"
    return CheckResult(
        check_id=str(validator["id"]),
        tier=0,
        status=status,
        required=bool(validator.get("required", True)),
        duration_ms=(time.perf_counter() - started) * 1000.0,
        diagnostic=diagnostic,
    )


def _last_output_line(completed: subprocess.CompletedProcess[str]) -> str:
    output = (completed.stdout + completed.stderr).strip()
    return output.splitlines()[-1] if output else (
        f"command exited {completed.returncode}"
    )


def run_contract_harness(
    profile: dict[str, Any],
) -> tuple[list[CheckResult], dict[str, Any] | None]:
    from validate_layer_authoring import find_msbuild

    harness = profile.get("contractHarness")
    if not isinstance(harness, dict):
        return (
            [
                CheckResult(
                    check_id="native-contract-and-lifecycle",
                    tier=1,
                    status="skip",
                    required=True,
                    duration_ms=0.0,
                    diagnostic="profile does not configure a Tier 1 contract harness",
                )
            ],
            None,
        )

    project = resolve_path(harness["project"])
    executable = resolve_path(harness["executable"])
    build_started = time.perf_counter()
    try:
        build = subprocess.run(
            [
                find_msbuild(),
                str(project),
                "/t:Build",
                "/p:Configuration=Release",
                "/p:Platform=x64",
                "/m",
                "/v:minimal",
            ],
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        build_check = CheckResult(
            check_id="native-contract-build",
            tier=1,
            status="pass" if build.returncode == 0 else "fail",
            required=True,
            duration_ms=(time.perf_counter() - build_started) * 1000.0,
            diagnostic=_last_output_line(build),
        )
    except FileNotFoundError as exc:
        build_check = CheckResult(
            check_id="native-contract-build",
            tier=1,
            status="fail",
            required=True,
            duration_ms=(time.perf_counter() - build_started) * 1000.0,
            diagnostic=f"executable not found: {exc.filename}",
        )
    if build_check.status != "pass":
        return (
            [
                build_check,
                CheckResult(
                    check_id="native-contract-and-lifecycle",
                    tier=1,
                    status="skip",
                    required=True,
                    duration_ms=0.0,
                    diagnostic="native contract did not run because its build failed",
                ),
            ],
            None,
        )
    if not executable.is_file():
        return (
            [
                build_check,
                CheckResult(
                    check_id="native-contract-and-lifecycle",
                    tier=1,
                    status="fail",
                    required=True,
                    duration_ms=0.0,
                    diagnostic=f"built executable is missing: {repo_path(executable)}",
                ),
            ],
            None,
        )

    run_started = time.perf_counter()
    with tempfile.TemporaryDirectory(prefix="synaptome-element-confidence-") as raw:
        native_report = Path(raw) / "native-evidence.json"
        completed = subprocess.run(
            [str(executable), "--output", str(native_report)],
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        evidence: dict[str, Any] | None = None
        diagnostic = _last_output_line(completed)
        status = "pass" if completed.returncode == 0 else "fail"
        if completed.returncode == 0:
            try:
                evidence = load_json_object(native_report, "native evidence")
                payloads = evidence.get("statePayloads")
                if (
                    not isinstance(payloads, list)
                    or len(payloads) !=
                        profile["inputs"]["determinismRepetitions"]
                    or not all(isinstance(item, str) for item in payloads)
                ):
                    raise ConfidenceError(
                        "native evidence statePayloads do not match repetitions"
                    )
                evidence["stateSignatures"] = [
                    hashlib.sha256(item.encode("utf-8")).hexdigest()
                    for item in payloads
                ]
                if len(set(evidence["stateSignatures"])) != 1:
                    raise ConfidenceError(
                        "native evidence state signatures differ"
                    )
                if evidence.get("bindingMode") != profile["bindingMode"]:
                    raise ConfidenceError(
                        "native evidence binding mode does not match profile"
                    )
                if evidence.get("elementType") != profile["elementType"]:
                    raise ConfidenceError(
                        "native evidence element type does not match profile"
                    )
            except ConfidenceError as exc:
                status = "fail"
                diagnostic = str(exc)
                evidence = None
    run_check = CheckResult(
        check_id="native-contract-and-lifecycle",
        tier=1,
        status=status,
        required=True,
        duration_ms=(time.perf_counter() - run_started) * 1000.0,
        diagnostic=diagnostic,
    )
    return [build_check, run_check], evidence


def run_graphics_harness(
    profile: dict[str, Any],
) -> tuple[list[CheckResult], dict[str, Any] | None]:
    harness = profile.get("contractHarness")
    if not isinstance(harness, dict):
        return (
            [
                CheckResult(
                    check_id="real-offscreen-graphics",
                    tier=2,
                    status="skip",
                    required=True,
                    duration_ms=0.0,
                    diagnostic="profile does not configure a graphics harness",
                )
            ],
            None,
        )
    executable = resolve_path(harness["executable"])
    if not executable.is_file():
        return (
            [
                CheckResult(
                    check_id="real-offscreen-graphics",
                    tier=2,
                    status="fail",
                    required=True,
                    duration_ms=0.0,
                    diagnostic=f"graphics executable is missing: {repo_path(executable)}",
                )
            ],
            None,
        )

    started = time.perf_counter()
    evidence: dict[str, Any] | None = None
    with tempfile.TemporaryDirectory(prefix="synaptome-element-graphics-") as raw:
        native_report = Path(raw) / "graphics-evidence.json"
        completed = subprocess.run(
            [
                str(executable),
                "--graphics",
                "--output",
                str(native_report),
            ],
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        status = "pass" if completed.returncode == 0 else "fail"
        diagnostic = _last_output_line(completed)
        if completed.returncode == 0:
            try:
                evidence = load_json_object(
                    native_report, "graphics evidence"
                )
                if evidence.get("elementType") != profile["elementType"]:
                    raise ConfidenceError(
                        "graphics evidence element type does not match profile"
                    )
                nonblank = evidence.get("nonblank")
                if not isinstance(nonblank, dict):
                    raise ConfidenceError(
                        "graphics evidence nonblank metrics are missing"
                    )
                if nonblank.get("distinctRgbaValues", 0) < 2:
                    raise ConfidenceError(
                        "graphics evidence has fewer than two RGBA values"
                    )
                if nonblank.get("nonblackRatio", 0.0) < 0.001:
                    raise ConfidenceError(
                        "graphics evidence is below the nonblack threshold"
                    )
                leaked_state = evidence.get("leakedState")
                if not isinstance(leaked_state, list) or leaked_state:
                    names = (
                        ", ".join(str(item) for item in leaked_state)
                        if isinstance(leaked_state, list)
                        else "unknown"
                    )
                    raise ConfidenceError(
                        f"graphics state leak: {names}"
                    )
                pixel_path = Path(str(evidence.get("pixelFile", "")))
                if not pixel_path.is_absolute():
                    pixel_path = (REPO_ROOT / pixel_path).resolve()
                expected_size = (
                    int(nonblank["width"]) *
                    int(nonblank["height"]) *
                    4
                )
                pixels = pixel_path.read_bytes()
                if len(pixels) != expected_size:
                    raise ConfidenceError(
                        "graphics RGBA byte count does not match dimensions"
                    )
                evidence["pixelSignature"] = hashlib.sha256(pixels).hexdigest()
                evidence.pop("pixelFile", None)
            except (ConfidenceError, KeyError, OSError, ValueError) as exc:
                status = "fail"
                diagnostic = str(exc)
                evidence = None
    checks = [
        CheckResult(
            check_id="real-offscreen-graphics",
            tier=2,
            status=status,
            required=True,
            duration_ms=(time.perf_counter() - started) * 1000.0,
            diagnostic=diagnostic,
        ),
        CheckResult(
            check_id="pixel-baseline-comparison",
            tier=2,
            status="skip",
            required=False,
            duration_ms=0.0,
            diagnostic=(
                "no reviewed baseline is configured for this "
                "renderer/vendor/driver class"
            ),
        ),
    ]
    return checks, evidence


def run_reload_harness(
    profile: dict[str, Any],
) -> tuple[list[CheckResult], dict[str, Any] | None]:
    harness = profile.get("contractHarness")
    if not isinstance(harness, dict):
        return (
            [
                CheckResult(
                    check_id="reload-memory-and-timing",
                    tier=3,
                    status="skip",
                    required=True,
                    duration_ms=0.0,
                    diagnostic="profile does not configure a reload harness",
                )
            ],
            None,
        )
    executable = resolve_path(harness["executable"])
    started = time.perf_counter()
    evidence: dict[str, Any] | None = None
    with tempfile.TemporaryDirectory(prefix="synaptome-element-reload-") as raw:
        native_report = Path(raw) / "reload-evidence.json"
        completed = subprocess.run(
            [
                str(executable),
                "--reload",
                "--output",
                str(native_report),
            ],
            cwd=REPO_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
        status = "pass" if completed.returncode == 0 else "fail"
        diagnostic = _last_output_line(completed)
        if native_report.is_file():
            try:
                evidence = load_json_object(
                    native_report, "reload evidence"
                )
                reload = evidence.get("reload")
                timings = evidence.get("timings")
                if not isinstance(reload, dict) or not isinstance(
                    timings, dict
                ):
                    raise ConfidenceError(
                        "reload evidence is missing reload/timing metrics"
                    )
                if reload.get("warmupCount") != 20:
                    raise ConfidenceError(
                        "reload evidence warmup count must equal 20"
                    )
                if reload.get("count") != 200:
                    raise ConfidenceError(
                        "reload evidence measured count must equal 200"
                    )
                if not all(
                    reload.get(key) is True
                    for key in (
                        "graphicsTargetsReleased",
                        "registryEntriesInvalidated",
                        "actionEntriesInvalidated",
                    )
                ):
                    raise ConfidenceError(
                        "reload evidence retained a resource or registry entry"
                    )
                if reload.get("graphicsStateLeaks") != []:
                    raise ConfidenceError(
                        "reload evidence contains graphics-state leaks"
                    )
                warm = int(reload["warmWorkingSetBytes"])
                final = int(reload["finalWorkingSetBytes"])
                slope = float(reload["growthSlopeBytesPerReload"])
                if final > warm + 16 * 1024 * 1024:
                    raise ConfidenceError(
                        "reload final working set exceeds the 16 MiB gate"
                    )
                if slope > 64 * 1024:
                    raise ConfidenceError(
                        "reload memory slope exceeds 64 KiB per reload"
                    )
                for lane in ("updateMs", "drawMs"):
                    values = timings.get(lane)
                    if not isinstance(values, dict) or not all(
                        isinstance(values.get(key), (int, float))
                        for key in ("median", "p95", "maximum")
                    ):
                        raise ConfidenceError(
                            f"reload timing metrics are invalid: {lane}"
                        )
            except (ConfidenceError, KeyError, TypeError, ValueError) as exc:
                status = "fail"
                diagnostic = str(exc)
                evidence = None
        elif completed.returncode == 0:
            status = "fail"
            diagnostic = "reload harness did not write evidence"
    checks = [
        CheckResult(
            check_id="reload-memory-and-timing",
            tier=3,
            status=status,
            required=True,
            duration_ms=(time.perf_counter() - started) * 1000.0,
            diagnostic=diagnostic,
        ),
        CheckResult(
            check_id="performance-baseline-comparison",
            tier=3,
            status="skip",
            required=False,
            duration_ms=0.0,
            diagnostic=(
                "this accepted run is evidence only; no reviewed "
                "machine/renderer-class baseline is configured"
            ),
        ),
    ]
    return checks, evidence


def compare_reviewed_baseline(
    report: ConfidenceReport,
    baseline_path: Path,
) -> list[CheckResult]:
    started = time.perf_counter()
    baseline = load_json_object(baseline_path, "baseline")
    if baseline.get("schemaVersion") != 1:
        raise ConfidenceError("baseline.schemaVersion must equal 1")
    entries = baseline.get("entries")
    if not isinstance(entries, list):
        raise ConfidenceError("baseline.entries must be an array")
    matches: list[dict[str, Any]] = []
    actual_class = {
        "system": report.platform["system"],
        "machine": report.platform["machine"],
        "vendor": report.graphics["vendor"],
        "renderer": report.graphics["renderer"],
        "version": report.graphics["version"],
    }
    for index, raw in enumerate(entries):
        context = f"baseline.entries[{index}]"
        if not isinstance(raw, dict):
            raise ConfidenceError(f"{context} must be an object")
        if raw.get("profileId") != report.package_profile_id:
            continue
        renderer_class = raw.get("rendererClass")
        if not isinstance(renderer_class, dict) or not renderer_class:
            raise ConfidenceError(
                f"{context}.rendererClass must be a non-empty object"
            )
        unknown = set(renderer_class) - set(actual_class)
        if unknown:
            raise ConfidenceError(
                f"{context}.rendererClass has unknown fields: "
                + ", ".join(sorted(unknown))
            )
        if all(
            actual_class.get(key) == value
            for key, value in renderer_class.items()
        ):
            review = raw.get("review")
            if not isinstance(review, dict) or not all(
                isinstance(review.get(key), str) and review[key]
                for key in (
                    "approvedBy",
                    "approvedUtc",
                    "sourceCommit",
                    "reason",
                )
            ):
                raise ConfidenceError(
                    f"{context}.review must record approval, UTC, "
                    "source commit, and reason"
                )
            matches.append(raw)
    if len(matches) > 1:
        ids = ", ".join(str(item.get("id", "")) for item in matches)
        raise ConfidenceError(
            f"duplicate reviewed baselines match this renderer class: {ids}"
        )
    if not matches:
        diagnostic = (
            "reviewed baseline has no entry for this "
            "profile/platform/renderer class"
        )
        return [
            CheckResult(
                check_id="pixel-baseline-comparison",
                tier=2,
                status="skip",
                required=False,
                duration_ms=(time.perf_counter() - started) * 1000.0,
                diagnostic=diagnostic,
            ),
            CheckResult(
                check_id="performance-baseline-comparison",
                tier=3,
                status="skip",
                required=False,
                duration_ms=0.0,
                diagnostic=diagnostic,
            ),
        ]

    entry = matches[0]
    pixel_baseline = entry.get("pixelSignature")
    if pixel_baseline is None:
        pixel_check = CheckResult(
            check_id="pixel-baseline-comparison",
            tier=2,
            status="skip",
            required=False,
            duration_ms=(time.perf_counter() - started) * 1000.0,
            diagnostic="matched reviewed baseline omits a pixel signature",
        )
    elif not isinstance(pixel_baseline, str) or len(pixel_baseline) != 64:
        raise ConfidenceError(
            "matched baseline.pixelSignature must be a SHA-256 hex string"
        )
    else:
        pixel_passed = pixel_baseline == report.graphics["pixelSignature"]
        pixel_check = CheckResult(
            check_id="pixel-baseline-comparison",
            tier=2,
            status="pass" if pixel_passed else "fail",
            required=True,
            duration_ms=(time.perf_counter() - started) * 1000.0,
            diagnostic=(
                f"matched reviewed baseline '{entry.get('id', '')}'"
                if pixel_passed
                else "pixel signature differs from the matched reviewed baseline"
            ),
        )

    timing_baseline = entry.get("timings")
    measured_update_p95 = report.timings["updateMs"]["p95"]
    measured_draw_p95 = report.timings["drawMs"]["p95"]
    if measured_update_p95 is None or measured_draw_p95 is None:
        timing_check = CheckResult(
            check_id="performance-baseline-comparison",
            tier=3,
            status="skip",
            required=False,
            duration_ms=0.0,
            diagnostic=(
                "the selected tier did not collect update/draw timing samples"
            ),
        )
        return [pixel_check, timing_check]
    if timing_baseline is None:
        timing_check = CheckResult(
            check_id="performance-baseline-comparison",
            tier=3,
            status="skip",
            required=False,
            duration_ms=0.0,
            diagnostic="matched reviewed baseline omits timing p95 values",
        )
    elif not isinstance(timing_baseline, dict) or not all(
        isinstance(timing_baseline.get(key), (int, float))
        and timing_baseline[key] >= 0
        for key in ("updateP95Ms", "drawP95Ms")
    ):
        raise ConfidenceError(
            "matched baseline.timings must contain non-negative "
            "updateP95Ms and drawP95Ms"
        )
    else:
        update_limit = float(timing_baseline["updateP95Ms"]) * 1.2
        draw_limit = float(timing_baseline["drawP95Ms"]) * 1.2
        update_actual = float(measured_update_p95)
        draw_actual = float(measured_draw_p95)
        timing_passed = (
            update_actual <= update_limit and draw_actual <= draw_limit
        )
        timing_check = CheckResult(
            check_id="performance-baseline-comparison",
            tier=3,
            status="pass" if timing_passed else "fail",
            required=True,
            duration_ms=0.0,
            diagnostic=(
                f"p95 update={update_actual:.4f}ms/{update_limit:.4f}ms "
                f"draw={draw_actual:.4f}ms/{draw_limit:.4f}ms"
            ),
        )
    return [pixel_check, timing_check]


def new_report(profile: dict[str, Any], tier: str) -> ConfidenceReport:
    return ConfidenceReport(
        timestamp_utc=utc_now(),
        commit=git_commit(),
        configuration="Release/x64",
        platform=platform_info(),
        tier=tier,
        element_type=profile["elementType"],
        package_profile_id=profile["id"],
        binding_mode=profile["bindingMode"],
        deterministic_inputs=dict(profile["inputs"]),
    )

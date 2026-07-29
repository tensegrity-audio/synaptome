#!/usr/bin/env python3
"""Pure Element Package v1 loading, normalization, validation, and parity."""
from __future__ import annotations

import hashlib
import json
import math
import os
import re
import struct
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath
from typing import Any, Iterable

REPO_ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = REPO_ROOT / "docs" / "schemas" / "layer_package.schema.json"
PRESET_SCHEMA_PATH = REPO_ROOT / "docs" / "schemas" / "layer_preset.schema.json"
SUPPORTED_SCHEMA_VERSION = 1
KNOWN_CAPABILITIES = frozenset(
    {"graphics.opengl", "transport.read", "parameters.dynamic-options"}
)
ID_RE = re.compile(r"^[a-z][A-Za-z0-9]*(?:[._-][A-Za-z0-9]+)*$")
SEMVER_RE = re.compile(
    r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
    r"(?:-([0-9A-Za-z.-]+))?(?:\+[0-9A-Za-z.-]+)?$"
)
RANGE_TERM_RE = re.compile(r"^(>=|>|<=|<|=)?(\d+\.\d+\.\d+)$")

try:
    import jsonschema  # type: ignore
except Exception:  # pragma: no cover
    jsonschema = None


@dataclass(frozen=True)
class Diagnostic:
    code: str
    package_id: str
    json_path: str
    message: str
    runtime_field: str = ""
    expected: Any = None
    observed: Any = None

    def render(self) -> str:
        detail = (
            f"; runtimeField={self.runtime_field}; "
            f"expected={json.dumps(self.expected, sort_keys=True)}; "
            f"observed={json.dumps(self.observed, sort_keys=True)}"
            if self.runtime_field
            else ""
        )
        return (
            f"{self.code}: package={self.package_id or '<unknown>'}; "
            f"path={self.json_path}; {self.message}{detail}"
        )

    def to_json(self) -> dict[str, Any]:
        result = {
            "code": self.code,
            "packageId": self.package_id,
            "jsonPath": self.json_path,
            "message": self.message,
        }
        if self.runtime_field:
            result.update(
                {
                    "runtimeField": self.runtime_field,
                    "expected": self.expected,
                    "observed": self.observed,
                }
            )
        return result


@dataclass
class ValidationResult:
    path: Path
    document: dict[str, Any] | None
    normalized: dict[str, Any] | None
    diagnostics: list[Diagnostic] = field(default_factory=list)
    resolved_compatibility: list[dict[str, Any]] = field(default_factory=list)
    resolved_dependencies: list[dict[str, Any]] = field(default_factory=list)
    inventory: dict[str, list[dict[str, Any]]] = field(default_factory=dict)
    migration_path: list[str] = field(default_factory=list)

    @property
    def valid(self) -> bool:
        return self.normalized is not None and not self.diagnostics


def _read_object(path: Path) -> tuple[dict[str, Any] | None, str | None]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return None, "file does not exist"
    except json.JSONDecodeError as exc:
        return None, f"invalid JSON at line {exc.lineno}, column {exc.colno}"
    if not isinstance(value, dict):
        return None, "root must be an object"
    return value, None


def _semver(value: Any) -> tuple[int, int, int] | None:
    if not isinstance(value, str):
        return None
    match = SEMVER_RE.fullmatch(value)
    return tuple(int(match.group(index)) for index in range(1, 4)) if match else None


def version_satisfies(version: str, expression: str) -> bool:
    parsed = _semver(version)
    if parsed is None or not isinstance(expression, str) or not expression:
        return False
    for term in expression.split():
        match = RANGE_TERM_RE.fullmatch(term)
        if not match:
            return False
        expected = _semver(match.group(2))
        if expected is None:
            return False
        operator = match.group(1) or "="
        if not {
            "=": parsed == expected,
            ">": parsed > expected,
            ">=": parsed >= expected,
            "<": parsed < expected,
            "<=": parsed <= expected,
        }[operator]:
            return False
    return True


def _schema_diagnostics(
    document: dict[str, Any], package_id: str
) -> list[Diagnostic]:
    if jsonschema is None:
        return [
            Diagnostic(
                "package.schema-validator-unavailable",
                package_id,
                "$",
                "jsonschema is required for Element Package v1 validation",
            )
        ]
    schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    errors = sorted(
        jsonschema.Draft7Validator(schema).iter_errors(document),
        key=lambda item: tuple(str(part) for part in item.absolute_path),
    )
    diagnostics: list[Diagnostic] = []
    for error in errors:
        path = "$" + "".join(
            f"[{part}]" if isinstance(part, int) else f".{part}"
            for part in error.absolute_path
        )
        diagnostics.append(
            Diagnostic("package.schema", package_id, path, error.message)
        )
    return diagnostics


def resolve_package_reference(
    package_path: Path, raw: Any
) -> tuple[Path | None, str | None]:
    if not isinstance(raw, str) or not raw:
        return None, "reference must be a non-empty string"
    if "\\" in raw:
        return None, "reference must use '/' separators"
    pure = PurePosixPath(raw)
    if pure.is_absolute() or re.match(r"^[A-Za-z]:", raw):
        return None, "absolute references are not portable"
    if any(part in {"", ".", ".."} for part in pure.parts):
        return None, "reference must be normalized and may not traverse"
    root = package_path.parent.resolve()
    resolved = (root / Path(*pure.parts)).resolve()
    try:
        resolved.relative_to(root)
    except ValueError:
        return None, "reference escapes the package root"
    if resolved.exists():
        parent = root
        for part in pure.parts:
            matches = {
                entry.name: entry
                for entry in parent.iterdir()
            } if parent.is_dir() else {}
            if part not in matches:
                case_match = next(
                    (name for name in matches if name.casefold() == part.casefold()),
                    None,
                )
                if case_match is not None:
                    return None, f"reference case differs from '{case_match}'"
                break
            parent = matches[part]
    return resolved, None


def _identity_diagnostics(
    items: Any,
    package_id: str,
    path: str,
    field_name: str = "id",
) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    seen: dict[str, int] = {}
    if not isinstance(items, list):
        return diagnostics
    for index, item in enumerate(items):
        if not isinstance(item, dict):
            continue
        identity = item.get(field_name)
        if not isinstance(identity, str):
            continue
        if identity in seen:
            diagnostics.append(
                Diagnostic(
                    "package.duplicate-id",
                    package_id,
                    f"{path}[{index}].{field_name}",
                    f"duplicate identity '{identity}' also appears at "
                    f"{path}[{seen[identity]}].{field_name}",
                )
            )
        else:
            seen[identity] = index
    return diagnostics


def _value_matches_kind(value: Any, kind: str) -> bool:
    return {
        "float": isinstance(value, (int, float))
        and not isinstance(value, bool)
        and math.isfinite(float(value)),
        "bool": isinstance(value, bool),
        "string": isinstance(value, str),
    }.get(kind, False)


def _normalized_parameter(raw: dict[str, Any]) -> dict[str, Any]:
    kind = raw.get("kind")

    def runtime_number(value: Any) -> Any:
        if kind != "float" or not isinstance(value, (int, float)):
            return value
        return struct.unpack("!f", struct.pack("!f", float(value)))[0]

    raw_range = raw.get("range") if "range" in raw else None
    normalized_range = (
        {
            "min": runtime_number(raw_range.get("min")),
            "max": runtime_number(raw_range.get("max")),
            "step": (
                runtime_number(raw_range.get("step"))
                if "step" in raw_range
                else None
            ),
        }
        if isinstance(raw_range, dict)
        else None
    )
    result: dict[str, Any] = {
        "id": raw.get("id"),
        "kind": kind,
        "groupId": raw.get("groupId"),
        "label": raw.get("label"),
        "default": runtime_number(raw.get("default")),
        "range": normalized_range,
        "units": raw.get("units", ""),
        "description": raw.get("description", ""),
        "visible": raw.get("visible", True),
        "options": [
            {
                **option,
                "value": runtime_number(option.get("value")),
            }
            for option in raw.get("options", [])
            if isinstance(option, dict)
        ],
        "optionSource": None,
        "quickAccessOrder": raw.get("quickAccessOrder"),
        "aliases": raw.get("aliases", []),
        "deprecation": None,
    }
    if isinstance(raw.get("optionsSource"), dict):
        source = raw["optionsSource"]
        result["optionSource"] = {
            "id": source.get("id"),
            "valueField": source.get("value"),
            "labelField": source.get("label"),
        }
    if isinstance(raw.get("deprecated"), dict):
        deprecation = raw["deprecated"]
        result["deprecation"] = {
            "replacementId": deprecation.get("replacement"),
            "reason": deprecation.get("reason"),
        }
    return result


def normalize(document: dict[str, Any]) -> dict[str, Any]:
    element = document.get("element", {})
    parameters = document.get("parameters", [])
    return {
        "typeId": element.get("id") if isinstance(element, dict) else None,
        "kind": element.get("kind") if isinstance(element, dict) else None,
        "bindingMode": (
            element.get("bindingMode") if isinstance(element, dict) else None
        ),
        "actions": (
            [
                {
                    "id": item.get("id"),
                    "label": item.get("label"),
                    "groupId": item.get("groupId"),
                }
                for item in element.get("actions", [])
                if isinstance(item, dict)
            ]
            if isinstance(element, dict)
            else []
        ),
        "parameterGroups": [
            {
                "id": item.get("id"),
                "label": item.get("label"),
                "description": item.get("description", ""),
            }
            for item in document.get("parameterGroups", [])
            if isinstance(item, dict)
        ],
        "parameters": [
            _normalized_parameter(item)
            for item in parameters
            if isinstance(item, dict)
        ],
    }


def descriptor_signature(normalized: dict[str, Any]) -> str:
    payload = json.dumps(
        normalized, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def _validate_parameters(
    document: dict[str, Any], package_id: str
) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    groups = document.get("parameterGroups", [])
    group_ids = {
        item.get("id") for item in groups if isinstance(item, dict)
    } if isinstance(groups, list) else set()
    parameters = document.get("parameters", [])
    if not isinstance(parameters, list):
        return diagnostics
    for index, raw in enumerate(parameters):
        if not isinstance(raw, dict):
            continue
        path = f"$.parameters[{index}]"
        kind = raw.get("kind")
        if raw.get("groupId") not in group_ids:
            diagnostics.append(
                Diagnostic(
                    "package.parameter-group-unresolved",
                    package_id,
                    f"{path}.groupId",
                    f"group '{raw.get('groupId')}' is not declared",
                )
            )
        if not _value_matches_kind(raw.get("default"), str(kind)):
            diagnostics.append(
                Diagnostic(
                    "package.parameter-kind",
                    package_id,
                    f"{path}.default",
                    f"default does not match kind '{kind}'",
                )
            )
        range_meta = raw.get("range")
        if kind == "float" and isinstance(range_meta, dict):
            minimum = range_meta.get("min")
            maximum = range_meta.get("max")
            value = raw.get("default")
            if all(
                isinstance(item, (int, float)) and not isinstance(item, bool)
                for item in (minimum, maximum, value)
            ) and not (minimum <= value <= maximum):
                diagnostics.append(
                    Diagnostic(
                        "package.parameter-range",
                        package_id,
                        f"{path}.default",
                        f"default {value} is outside [{minimum}, {maximum}]",
                    )
                )
        options = raw.get("options", [])
        if options and raw.get("optionsSource") is not None:
            diagnostics.append(
                Diagnostic(
                    "package.parameter-options-ambiguous",
                    package_id,
                    path,
                    "non-empty inline options and optionsSource are mutually exclusive",
                )
            )
        for option_index, option in enumerate(options if isinstance(options, list) else []):
            if isinstance(option, dict) and not _value_matches_kind(
                option.get("value"), str(kind)
            ):
                diagnostics.append(
                    Diagnostic(
                        "package.parameter-option-kind",
                        package_id,
                        f"{path}.options[{option_index}].value",
                        f"option value does not match kind '{kind}'",
                    )
                )
    defaults = (
        document.get("asset", {}).get("defaults", {})
        if isinstance(document.get("asset"), dict)
        else {}
    )
    by_id = {
        item.get("id"): item
        for item in parameters
        if isinstance(item, dict)
    }
    if isinstance(defaults, dict):
        for parameter_id, value in defaults.items():
            declaration = by_id.get(parameter_id)
            if declaration is None:
                diagnostics.append(
                    Diagnostic(
                        "package.definition-default-unresolved",
                        package_id,
                        f"$.asset.defaults.{parameter_id}",
                        "definition default does not reference a declared parameter",
                    )
                )
            elif not _value_matches_kind(value, str(declaration.get("kind"))):
                diagnostics.append(
                    Diagnostic(
                        "package.definition-default-kind",
                        package_id,
                        f"$.asset.defaults.{parameter_id}",
                        "definition default changes the declared value kind",
                    )
                )
    return diagnostics


def _validate_references(
    package_path: Path, document: dict[str, Any], package_id: str
) -> tuple[list[Diagnostic], dict[str, list[dict[str, Any]]]]:
    diagnostics: list[Diagnostic] = []
    inventory: dict[str, list[dict[str, Any]]] = {
        "source": [], "assets": [], "presets": [], "mappings": [], "tests": [],
        "migrations": [],
    }
    inventory["sectionPresence"] = [  # type: ignore[assignment]
        {
            "section": section,
            "status": (
                "omitted"
                if section not in document
                else "present-empty"
                if document.get(section) in ([], {})
                else "present"
            ),
        }
        for section in (
            "presets",
            "presetBanks",
            "mappingPresets",
            "media",
            "tests",
            "aliases",
            "deprecations",
            "migrations",
        )
    ]
    references: list[tuple[str, str, str, bool]] = []
    source = document.get("source", {})
    if isinstance(source, dict):
        references.append(("source", "$.source.registration", str(source.get("registration", "")), True))
        references.extend(
            ("source", f"$.source.files[{index}]", str(raw), True)
            for index, raw in enumerate(source.get("files", []))
        )
    references.extend(
        ("presets", f"$.presets[{index}].path", str(item.get("path", "")), True)
        for index, item in enumerate(document.get("presets", []))
        if isinstance(item, dict)
    )
    references.extend(
        ("assets", f"$.media[{index}].path", str(item.get("path", "")), True)
        for index, item in enumerate(document.get("media", []))
        if isinstance(item, dict)
    )
    tests = document.get("tests", {})
    if isinstance(tests, dict):
        references.append(("tests", "$.tests.confidenceProfile", str(tests.get("confidenceProfile", "")), True))
        references.extend(
            ("tests", f"$.tests.fixtures[{index}]", str(raw), True)
            for index, raw in enumerate(tests.get("fixtures", []))
        )
    references.extend(
        ("migrations", f"$.migrations[{index}].path", str(item.get("path", "")), True)
        for index, item in enumerate(document.get("migrations", []))
        if isinstance(item, dict)
    )
    for kind, json_path, raw, required in references:
        resolved, error = resolve_package_reference(package_path, raw)
        record = {"path": raw, "status": "resolved"}
        if error:
            record["status"] = "unsafe"
            diagnostics.append(
                Diagnostic("package.reference-unsafe", package_id, json_path, error)
            )
        elif resolved is None or not resolved.is_file():
            record["status"] = "missing"
            if required:
                diagnostics.append(
                    Diagnostic(
                        "package.reference-missing",
                        package_id,
                        json_path,
                        f"referenced file is missing: {raw}",
                    )
                )
        inventory[kind].append(record)
    inventory["mappings"] = [
        {"id": item.get("id"), "applyMode": item.get("applyMode")}
        for item in document.get("mappingPresets", [])
        if isinstance(item, dict)
    ]
    return diagnostics, inventory


def _validate_dependencies(
    package_path: Path, document: dict[str, Any], package_id: str
) -> tuple[list[Diagnostic], list[dict[str, Any]]]:
    diagnostics: list[Diagnostic] = []
    resolved: list[dict[str, Any]] = []
    dependencies = document.get("dependencies", [])
    if not isinstance(dependencies, list):
        return diagnostics, resolved
    by_id = {
        item.get("id"): item
        for item in dependencies
        if isinstance(item, dict) and isinstance(item.get("id"), str)
    }
    providers: dict[str, str] = {}
    edges: dict[str, list[str]] = {}
    for index, dependency in enumerate(dependencies):
        if not isinstance(dependency, dict):
            continue
        path = f"$.dependencies[{index}]"
        dependency_id = str(dependency.get("id", ""))
        version_range = str(dependency.get("versionRange", ""))
        version = str(dependency.get("resolvedVersion", ""))
        record = {
            "id": dependency_id,
            "kind": dependency.get("kind"),
            "required": dependency.get("required"),
            "versionRange": version_range,
            "resolvedVersion": version,
            "status": "resolved",
        }
        if not version_satisfies(version, version_range):
            record["status"] = "incompatible"
            diagnostics.append(
                Diagnostic(
                    "package.dependency-version",
                    package_id,
                    f"{path}.resolvedVersion",
                    f"resolved version '{version}' does not satisfy '{version_range}'",
                )
            )
        provider = dependency.get("provider")
        if isinstance(provider, str):
            if provider in providers and providers[provider] != dependency_id:
                record["status"] = "duplicate-provider"
                diagnostics.append(
                    Diagnostic(
                        "package.dependency-duplicate-provider",
                        package_id,
                        f"{path}.provider",
                        f"provider '{provider}' also resolves dependency '{providers[provider]}'",
                    )
                )
            providers[provider] = dependency_id
        raw_path = dependency.get("path")
        if isinstance(raw_path, str):
            target, error = resolve_package_reference(package_path, raw_path)
            if error or target is None or not target.is_file():
                record["status"] = "missing" if dependency.get("required") else "optional-absent"
                if dependency.get("required"):
                    diagnostics.append(
                        Diagnostic(
                            "package.dependency-missing",
                            package_id,
                            f"{path}.path",
                            error or f"required dependency is missing: {raw_path}",
                        )
                    )
        edges[dependency_id] = [
            str(item) for item in dependency.get("dependsOn", [])
        ]
        for target in edges[dependency_id]:
            if target not in by_id:
                record["status"] = "unresolved"
                diagnostics.append(
                    Diagnostic(
                        "package.dependency-unresolved",
                        package_id,
                        f"{path}.dependsOn",
                        f"dependency '{target}' is not declared",
                    )
                )
        resolved.append(record)

    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(node: str, trail: list[str]) -> None:
        if node in visiting:
            cycle = trail[trail.index(node):] + [node]
            diagnostics.append(
                Diagnostic(
                    "package.dependency-cycle",
                    package_id,
                    "$.dependencies",
                    "dependency cycle: " + " -> ".join(cycle),
                )
            )
            return
        if node in visited:
            return
        visiting.add(node)
        for target in edges.get(node, []):
            visit(target, trail + [target])
        visiting.remove(node)
        visited.add(node)

    for dependency_id in edges:
        visit(dependency_id, [dependency_id])
    return diagnostics, resolved


def _validate_migrations(
    document: dict[str, Any], package_id: str
) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    migrations = document.get("migrations", [])
    if not isinstance(migrations, list):
        return diagnostics
    outgoing: dict[str, str] = {}
    for index, migration in enumerate(migrations):
        if not isinstance(migration, dict):
            continue
        source = str(migration.get("fromVersion", ""))
        target = str(migration.get("toVersion", ""))
        if source in outgoing:
            diagnostics.append(
                Diagnostic(
                    "package.migration-ambiguous",
                    package_id,
                    f"$.migrations[{index}].fromVersion",
                    f"multiple migration paths leave version '{source}'",
                )
            )
        outgoing[source] = target
        if _semver(source) is not None and _semver(target) is not None and _semver(source) >= _semver(target):
            diagnostics.append(
                Diagnostic(
                    "package.migration-order",
                    package_id,
                    f"$.migrations[{index}]",
                    "migration target must be newer than its source",
                )
            )
    return diagnostics


def _validate_cross_references(
    package_path: Path,
    document: dict[str, Any],
    package_id: str,
) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    parameters = {
        item.get("id"): item
        for item in document.get("parameters", [])
        if isinstance(item, dict)
    }
    quick_orders: dict[int, int] = {}
    for index, parameter in enumerate(document.get("parameters", [])):
        if not isinstance(parameter, dict):
            continue
        order = parameter.get("quickAccessOrder")
        if isinstance(order, int):
            if order in quick_orders:
                diagnostics.append(
                    Diagnostic(
                        "package.quick-access-duplicate",
                        package_id,
                        f"$.parameters[{index}].quickAccessOrder",
                        f"quick-access order {order} also appears at "
                        f"$.parameters[{quick_orders[order]}].quickAccessOrder",
                    )
                )
            quick_orders[order] = index
    preset_ids: set[str] = set()
    asset = document.get("asset", {})
    asset_id = asset.get("id") if isinstance(asset, dict) else None
    for index, reference in enumerate(document.get("presets", [])):
        if not isinstance(reference, dict):
            continue
        preset_id = str(reference.get("presetId", ""))
        preset_ids.add(preset_id)
        target, error = resolve_package_reference(
            package_path, reference.get("path")
        )
        if error or target is None or not target.is_file():
            continue
        preset, read_error = _read_object(target)
        if read_error or preset is None:
            diagnostics.append(
                Diagnostic(
                    "package.preset-read",
                    package_id,
                    f"$.presets[{index}].path",
                    read_error or "preset root must be an object",
                )
            )
            continue
        for field, expected in (
            ("schemaVersion", 1),
            ("packageId", package_id),
            ("assetId", asset_id),
            ("presetId", preset_id),
        ):
            if preset.get(field) != expected:
                diagnostics.append(
                    Diagnostic(
                        "package.preset-identity",
                        package_id,
                        f"$.presets[{index}].path",
                        f"preset {field} must equal {expected!r}",
                    )
                )
        values = preset.get("parameters", {})
        if isinstance(values, dict):
            for parameter_id, value in values.items():
                declaration = parameters.get(parameter_id)
                if declaration is None:
                    diagnostics.append(
                        Diagnostic(
                            "package.preset-parameter-unresolved",
                            package_id,
                            f"$.presets[{index}].path",
                            f"preset parameter '{parameter_id}' is not declared",
                        )
                    )
                elif not _value_matches_kind(
                    value, str(declaration.get("kind"))
                ):
                    diagnostics.append(
                        Diagnostic(
                            "package.preset-parameter-kind",
                            package_id,
                            f"$.presets[{index}].path",
                            f"preset parameter '{parameter_id}' changes kind",
                        )
                    )
    for bank_index, bank in enumerate(document.get("presetBanks", [])):
        if not isinstance(bank, dict):
            continue
        for preset_id in bank.get("presets", []):
            if preset_id not in preset_ids:
                diagnostics.append(
                    Diagnostic(
                        "package.preset-bank-unresolved",
                        package_id,
                        f"$.presetBanks[{bank_index}].presets",
                        f"preset '{preset_id}' is not declared",
                    )
                )
    for preset_index, preset in enumerate(
        document.get("mappingPresets", [])
    ):
        if not isinstance(preset, dict):
            continue
        for mapping_index, mapping in enumerate(preset.get("mappings", [])):
            if not isinstance(mapping, dict):
                continue
            target = mapping.get("target")
            target_kind = "parameter"
            target_id = target
            if isinstance(target, dict):
                target_kind = target.get("kind")
                target_id = target.get("id")
            declared = (
                target_id in parameters
                if target_kind == "parameter"
                else target_id
                in {
                    action.get("id")
                    for action in document.get("element", {}).get("actions", [])
                    if isinstance(action, dict)
                }
            )
            if not declared:
                diagnostics.append(
                    Diagnostic(
                        "package.mapping-target-unresolved",
                        package_id,
                        f"$.mappingPresets[{preset_index}].mappings"
                        f"[{mapping_index}].target",
                        f"{target_kind} target '{target_id}' is not declared",
                    )
                )
    return diagnostics


def validate_package(package_path: Path) -> ValidationResult:
    package_path = package_path.resolve()
    document, read_error = _read_object(package_path)
    if document is None:
        return ValidationResult(
            package_path,
            None,
            None,
            [Diagnostic("package.read", "", "$", read_error or "read failed")],
        )
    package_id = str(document.get("packageId", ""))
    diagnostics: list[Diagnostic] = []
    if document.get("schemaVersion") != SUPPORTED_SCHEMA_VERSION:
        diagnostics.append(
            Diagnostic(
                "package.schema-version",
                package_id,
                "$.schemaVersion",
                "Element Package readers accept exactly schema v1",
            )
        )
    package_version = _semver(document.get("packageVersion"))
    if package_version is not None and not (
        (0, 1, 0) <= package_version < (1, 0, 0)
    ):
        diagnostics.append(
            Diagnostic(
                "package.package-version-unsupported",
                package_id,
                "$.packageVersion",
                "this package reader supports package versions >=0.1.0 <1.0.0",
            )
        )
    diagnostics.extend(_schema_diagnostics(document, package_id))
    for path, items, field_name in (
        ("$.element.actions", document.get("element", {}).get("actions", []) if isinstance(document.get("element"), dict) else [], "id"),
        ("$.parameterGroups", document.get("parameterGroups", []), "id"),
        ("$.parameters", document.get("parameters", []), "id"),
        ("$.presets", document.get("presets", []), "presetId"),
        ("$.presetBanks", document.get("presetBanks", []), "id"),
        ("$.mappingPresets", document.get("mappingPresets", []), "id"),
        ("$.media", document.get("media", []), "id"),
        ("$.dependencies", document.get("dependencies", []), "id"),
        ("$.migrations", document.get("migrations", []), "id"),
    ):
        diagnostics.extend(_identity_diagnostics(items, package_id, path, field_name))
    capabilities = document.get("capabilities", [])
    if isinstance(capabilities, list):
        for index, capability in enumerate(capabilities):
            if capability not in KNOWN_CAPABILITIES:
                diagnostics.append(
                    Diagnostic(
                        "package.capability-unknown",
                        package_id,
                        f"$.capabilities[{index}]",
                        f"unknown required capability '{capability}'",
                    )
                )
    diagnostics.extend(_validate_parameters(document, package_id))
    reference_diagnostics, inventory = _validate_references(
        package_path, document, package_id
    )
    diagnostics.extend(reference_diagnostics)
    dependency_diagnostics, resolved_dependencies = _validate_dependencies(
        package_path, document, package_id
    )
    diagnostics.extend(dependency_diagnostics)
    diagnostics.extend(_validate_migrations(document, package_id))
    diagnostics.extend(
        _validate_cross_references(package_path, document, package_id)
    )

    compatibility_records: list[dict[str, Any]] = []
    compatibility = document.get("compatibility", {})
    if isinstance(compatibility, dict):
        for key in ("sdk", "runtime"):
            value = compatibility.get(key, {})
            if not isinstance(value, dict):
                continue
            expression = str(value.get("versionRange", ""))
            resolved_version = str(value.get("resolvedVersion", ""))
            status = "resolved" if version_satisfies(resolved_version, expression) else "incompatible"
            compatibility_records.append(
                {
                    "kind": key,
                    "versionRange": expression,
                    "resolvedVersion": resolved_version,
                    "status": status,
                }
            )
            if status != "resolved":
                diagnostics.append(
                    Diagnostic(
                        "package.compatibility-version",
                        package_id,
                        f"$.compatibility.{key}.resolvedVersion",
                        f"resolved version '{resolved_version}' does not satisfy '{expression}'",
                    )
                )
    asset = document.get("asset", {})
    element = document.get("element", {})
    if isinstance(asset, dict) and isinstance(element, dict) and asset.get("type") != element.get("id"):
        diagnostics.append(
            Diagnostic(
                "package.definition-type",
                package_id,
                "$.asset.type",
                "definition type must reference element.id",
            )
        )
    return ValidationResult(
        path=package_path,
        document=document,
        normalized=normalize(document) if not diagnostics else None,
        diagnostics=diagnostics,
        resolved_compatibility=compatibility_records,
        resolved_dependencies=resolved_dependencies,
        inventory=inventory,
        migration_path=[],
    )


def compare_descriptors(
    package_id: str,
    package_descriptor: dict[str, Any],
    runtime_descriptor: dict[str, Any],
) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []

    def walk(expected: Any, observed: Any, json_path: str, runtime_path: str) -> None:
        if type(expected) is not type(observed):
            diagnostics.append(
                Diagnostic(
                    "package.runtime-parity",
                    package_id,
                    json_path,
                    "serialized and Runtime field types differ",
                    runtime_path,
                    expected,
                    observed,
                )
            )
            return
        if isinstance(expected, dict):
            expected_keys = list(expected)
            observed_keys = list(observed)
            if set(expected_keys) != set(observed_keys):
                diagnostics.append(
                    Diagnostic(
                        "package.runtime-parity",
                        package_id,
                        json_path,
                        "serialized and Runtime object fields differ",
                        runtime_path,
                        sorted(expected_keys),
                        sorted(observed_keys),
                    )
                )
                return
            for key in expected_keys:
                walk(
                    expected[key],
                    observed[key],
                    f"{json_path}.{key}",
                    f"{runtime_path}.{key}",
                )
        elif isinstance(expected, list):
            if len(expected) != len(observed):
                diagnostics.append(
                    Diagnostic(
                        "package.runtime-parity",
                        package_id,
                        json_path,
                        "ordered array lengths differ",
                        runtime_path,
                        len(expected),
                        len(observed),
                    )
                )
                return
            for index, value in enumerate(expected):
                walk(
                    value,
                    observed[index],
                    f"{json_path}[{index}]",
                    f"{runtime_path}[{index}]",
                )
        elif expected != observed:
            diagnostics.append(
                Diagnostic(
                    "package.runtime-parity",
                    package_id,
                    json_path,
                    "serialized and Runtime values differ",
                    runtime_path,
                    expected,
                    observed,
                )
            )

    walk(package_descriptor, runtime_descriptor, "$.normalized", "ElementTypeContract")
    return diagnostics


def validate_activation_set(
    results: Iterable[ValidationResult],
) -> list[Diagnostic]:
    diagnostics: list[Diagnostic] = []
    seen: dict[tuple[str, str], str] = {}
    for result in results:
        if result.document is None:
            continue
        package_id = str(result.document.get("packageId", ""))
        asset = result.document.get("asset", {})
        element = result.document.get("element", {})
        identities = {
            "package": package_id,
            "definition": asset.get("id") if isinstance(asset, dict) else None,
            "type": element.get("id") if isinstance(element, dict) else None,
            "registryPrefix": asset.get("registryPrefix") if isinstance(asset, dict) else None,
        }
        for kind, identity in identities.items():
            if not isinstance(identity, str):
                continue
            key = (kind, identity)
            if key in seen:
                diagnostics.append(
                    Diagnostic(
                        "package.activation-conflict",
                        package_id,
                        "$",
                        f"{kind} identity '{identity}' conflicts with package '{seen[key]}'",
                    )
                )
            else:
                seen[key] = package_id
    return diagnostics

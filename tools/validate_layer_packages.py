#!/usr/bin/env python3
"""Validate draft Synaptome layer package fixtures."""
from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

import layer_package_discovery
import element_package_v1

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROOTS = layer_package_discovery.fixture_roots()
PACKAGE_SCHEMA = REPO_ROOT / "docs" / "schemas" / "layer_package.schema.json"
PRESET_SCHEMA = REPO_ROOT / "docs" / "schemas" / "layer_preset.schema.json"

try:
    import jsonschema  # type: ignore

    JSONSCHEMA_AVAILABLE = True
except Exception:  # pragma: no cover - depends on environment
    JSONSCHEMA_AVAILABLE = False


@dataclass
class PackageSummary:
    path: Path
    package_id: str
    asset_id: str
    layer_type: str
    parameter_count: int
    preset_count: int
    mapping_preset_count: int


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def iter_package_paths(roots: Iterable[Path]) -> list[Path]:
    return layer_package_discovery.iter_package_paths(roots)


def schema_errors(data: Any, schema_path: Path, path: Path) -> list[str]:
    if not JSONSCHEMA_AVAILABLE:
        return []
    schema = load_json(schema_path)
    validator = jsonschema.Draft7Validator(schema)
    errors: list[str] = []
    for err in validator.iter_errors(data):
        loc = "".join(f"[{part}]" for part in err.path)
        where = f"{rel(path)}{loc}" if loc else rel(path)
        errors.append(f"{where}: {err.message}")
    return errors


def as_dict(value: Any) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


def as_list(value: Any) -> list[Any]:
    return value if isinstance(value, list) else []


def resolve_package_path(package_path: Path, raw: Any) -> Path:
    text = str(raw)
    path = Path(text)
    if path.is_absolute():
        return path
    return (package_path.parent / path).resolve()


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def value_matches_kind(value: Any, kind: str) -> bool:
    if kind == "float":
        return is_number(value)
    if kind == "bool":
        return isinstance(value, bool)
    if kind == "string":
        return isinstance(value, str)
    return False


def validate_value_range(value: Any, param: dict[str, Any], ctx: str) -> list[str]:
    errors: list[str] = []
    if param.get("kind") != "float" or not is_number(value):
        return errors
    range_meta = as_dict(param.get("range"))
    if not range_meta:
        errors.append(f"{ctx}: float parameter '{param.get('id')}' is missing a range")
        return errors
    minimum = range_meta.get("min")
    maximum = range_meta.get("max")
    step = range_meta.get("step")
    if not is_number(minimum) or not is_number(maximum):
        errors.append(f"{ctx}: float parameter '{param.get('id')}' range must include numeric min/max")
        return errors
    if minimum > maximum:
        errors.append(f"{ctx}: float parameter '{param.get('id')}' range min exceeds max")
    if value < minimum or value > maximum:
        errors.append(
            f"{ctx}: value for '{param.get('id')}'={value} is outside range [{minimum}, {maximum}]"
        )
    if step is not None and (not is_number(step) or step <= 0):
        errors.append(f"{ctx}: float parameter '{param.get('id')}' range step must be positive")
    return errors


def collect_parameter_errors(parameters: list[Any], package_path: Path) -> tuple[dict[str, dict[str, Any]], list[str]]:
    errors: list[str] = []
    declared: dict[str, dict[str, Any]] = {}
    for index, raw in enumerate(parameters):
        ctx = f"{rel(package_path)}.parameters[{index}]"
        if not isinstance(raw, dict):
            errors.append(f"{ctx}: parameter must be an object")
            continue
        param_id = raw.get("id")
        kind = raw.get("kind")
        label = raw.get("label")
        if not isinstance(param_id, str) or not param_id:
            errors.append(f"{ctx}: parameter id must be a non-empty string")
            continue
        if param_id in declared:
            errors.append(f"{ctx}: duplicate parameter id '{param_id}'")
        declared[param_id] = raw
        if kind not in {"float", "bool", "string"}:
            errors.append(f"{ctx}: parameter kind must be float, bool, or string")
        elif "default" in raw and not value_matches_kind(raw.get("default"), kind):
            errors.append(f"{ctx}: default for '{param_id}' must match kind {kind}")
        if not isinstance(label, str) or ":" not in label:
            errors.append(f"{ctx}: label should use canonical 'Section: Label' form")
        if kind == "float":
            errors.extend(validate_value_range(raw.get("default"), raw, ctx))
        if raw.get("options") is not None and raw.get("optionsSource") is not None:
            errors.append(f"{ctx}: use either options[] or optionsSource, not both")
        for opt_index, option in enumerate(as_list(raw.get("options"))):
            if not isinstance(option, dict):
                errors.append(f"{ctx}.options[{opt_index}]: option must be an object")
                continue
            if "value" not in option or not value_matches_kind(option.get("value"), str(kind)):
                errors.append(f"{ctx}.options[{opt_index}]: option value must match parameter kind {kind}")
    return declared, errors


def validate_preset(
    package_path: Path,
    package: dict[str, Any],
    preset_ref: dict[str, Any],
    params: dict[str, dict[str, Any]],
) -> tuple[str, list[str]]:
    errors: list[str] = []
    preset_id = str(preset_ref.get("presetId", ""))
    preset_path = resolve_package_path(package_path, preset_ref.get("path", ""))
    if not preset_path.exists():
        return preset_id, [f"{rel(package_path)} preset '{preset_id}' path is missing: {rel(preset_path)}"]
    try:
        preset = load_json(preset_path)
    except json.JSONDecodeError as exc:
        return preset_id, [f"{rel(preset_path)}: JSON parse error: {exc}"]
    errors.extend(schema_errors(preset, PRESET_SCHEMA, preset_path))
    if not isinstance(preset, dict):
        return preset_id, [f"{rel(preset_path)}: preset must be a JSON object"]

    asset_id = as_dict(package.get("asset")).get("id")
    if preset.get("assetId") != asset_id:
        errors.append(f"{rel(preset_path)}: assetId must match package asset id '{asset_id}'")
    if preset.get("packageId") not in {None, package.get("packageId")}:
        errors.append(f"{rel(preset_path)}: packageId must match package id '{package.get('packageId')}'")
    if preset.get("presetId") != preset_id:
        errors.append(f"{rel(preset_path)}: presetId must match package preset reference '{preset_id}'")

    values = as_dict(preset.get("parameters"))
    for suffix, value in values.items():
        param = params.get(suffix)
        if param is None:
            errors.append(f"{rel(preset_path)}: preset parameter '{suffix}' is not declared by the package")
            continue
        kind = str(param.get("kind"))
        if not value_matches_kind(value, kind):
            errors.append(f"{rel(preset_path)}: preset value for '{suffix}' must match kind {kind}")
        errors.extend(validate_value_range(value, param, rel(preset_path)))
    return preset_id, errors


def validate_mapping_presets(
    package_path: Path,
    mapping_presets: list[Any],
    params: dict[str, dict[str, Any]],
    actions: set[str],
) -> list[str]:
    errors: list[str] = []
    seen: set[str] = set()
    for index, raw in enumerate(mapping_presets):
        ctx = f"{rel(package_path)}.mappingPresets[{index}]"
        if not isinstance(raw, dict):
            errors.append(f"{ctx}: mapping preset must be an object")
            continue
        preset_id = raw.get("id")
        if not isinstance(preset_id, str) or not preset_id:
            errors.append(f"{ctx}: mapping preset id must be a non-empty string")
        elif preset_id in seen:
            errors.append(f"{ctx}: duplicate mapping preset id '{preset_id}'")
        seen.add(str(preset_id))
        for map_index, mapping in enumerate(as_list(raw.get("mappings"))):
            map_ctx = f"{ctx}.mappings[{map_index}]"
            if not isinstance(mapping, dict):
                errors.append(f"{map_ctx}: mapping must be an object")
                continue
            target = mapping.get("target")
            target_kind = "parameter"
            target_id = target
            if isinstance(target, dict):
                target_kind = target.get("kind")
                target_id = target.get("id")
            if target_kind == "parameter" and target_id not in params:
                errors.append(f"{map_ctx}: target '{target_id}' is not a declared parameter suffix")
            elif target_kind == "action" and target_id not in actions:
                errors.append(f"{map_ctx}: target '{target_id}' is not a declared action")
            source = as_dict(mapping.get("source"))
            pattern = source.get("pattern")
            if not isinstance(pattern, str) or not pattern.startswith("/"):
                errors.append(f"{map_ctx}: OSC source pattern must start with '/'")
            for key in ("in", "out"):
                pair = source.get(key)
                if not (
                    isinstance(pair, list)
                    and len(pair) == 2
                    and all(is_number(item) for item in pair)
                ):
                    errors.append(f"{map_ctx}: source.{key} must be a numeric [min, max] pair")
            for key in ("smooth", "deadband"):
                value = source.get(key)
                if value is not None and (not is_number(value) or value < 0):
                    errors.append(f"{map_ctx}: source.{key} must be non-negative")
            trigger = source.get("trigger")
            if target_kind == "action":
                if not isinstance(trigger, dict):
                    errors.append(f"{map_ctx}: action target requires source.trigger")
                else:
                    if trigger.get("edge") not in {"rising", "falling", "both"}:
                        errors.append(f"{map_ctx}: trigger.edge must be rising, falling, or both")
                    if not is_number(trigger.get("threshold")):
                        errors.append(f"{map_ctx}: trigger.threshold must be numeric")
            elif trigger is not None:
                errors.append(f"{map_ctx}: parameter target must not declare source.trigger")
    return errors


def validate_package(package_path: Path) -> tuple[PackageSummary | None, list[str]]:
    result = element_package_v1.validate_package(package_path)
    errors = [diagnostic.render() for diagnostic in result.diagnostics]
    package = result.document
    if package is None:
        return None, errors
    asset = as_dict(package.get("asset"))
    element = as_dict(package.get("element"))
    summary = PackageSummary(
        path=package_path,
        package_id=str(package.get("packageId", "")),
        asset_id=str(asset.get("id", "")),
        layer_type=str(element.get("id", "")),
        parameter_count=len(as_list(package.get("parameters"))),
        preset_count=len(as_list(package.get("presets"))),
        mapping_preset_count=len(as_list(package.get("mappingPresets"))),
    )
    return summary, errors


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "paths",
        nargs="*",
        help="Optional layer.package.json files or roots to validate. Defaults to package fixtures.",
    )
    parser.add_argument("--check", action="store_true", help="Validate packages (default)")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    roots = tuple(layer_package_discovery.resolve_path(raw) for raw in args.paths)
    if not roots:
        roots = DEFAULT_ROOTS
    package_paths = iter_package_paths(roots)
    if not package_paths:
        print("No layer packages found", file=sys.stderr)
        return 1

    all_errors: list[str] = []
    summaries: list[PackageSummary] = []
    package_ids: dict[str, Path] = {}
    asset_ids: dict[str, Path] = {}
    layer_types: dict[str, Path] = {}

    for package_path in package_paths:
        summary, errors = validate_package(package_path)
        if summary is not None:
            summaries.append(summary)
            for label, value, seen in (
                ("packageId", summary.package_id, package_ids),
                ("asset.id", summary.asset_id, asset_ids),
                ("asset.type", summary.layer_type, layer_types),
            ):
                if value in seen:
                    errors.append(
                        f"{rel(package_path)}: duplicate {label} '{value}' also used by {rel(seen[value])}"
                    )
                else:
                    seen[value] = package_path
        all_errors.extend(errors)

    if all_errors:
        for error in all_errors:
            print(f"layer package error: {error}", file=sys.stderr)
        return 1

    parameter_count = sum(summary.parameter_count for summary in summaries)
    preset_count = sum(summary.preset_count for summary in summaries)
    mapping_count = sum(summary.mapping_preset_count for summary in summaries)
    print(
        "Layer package validation passed "
        f"({len(summaries)} packages, {parameter_count} parameters, "
        f"{preset_count} presets, {mapping_count} mapping presets)"
    )
    for summary in summaries:
        print(f"- {summary.package_id}: {rel(summary.path)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

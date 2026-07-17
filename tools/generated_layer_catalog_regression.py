#!/usr/bin/env python3
"""Generate/check draft catalog output for file-backed generated layers."""
from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterable

import layer_catalog_regression

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROOTS = (REPO_ROOT / "docs" / "examples" / "generated_layers",)
DEFAULT_EXPECTED = REPO_ROOT / "tools" / "testdata" / "generated_layers" / "expected_generated_layer_catalog.json"
TEMPLATE_SCHEMA = REPO_ROOT / "docs" / "schemas" / "generated_layer_template.schema.json"
SIDECAR_SCHEMA = REPO_ROOT / "docs" / "schemas" / "generated_layer_sidecar.schema.json"
TEMPLATE_NAME = "generated_layer.template.json"

try:
    import jsonschema  # type: ignore

    JSONSCHEMA_AVAILABLE = True
except Exception:  # pragma: no cover - depends on environment
    JSONSCHEMA_AVAILABLE = False


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def dumps(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def as_dict(value: Any) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


def as_list(value: Any) -> list[Any]:
    return value if isinstance(value, list) else []


def resolve_path(base: Path, raw: Any) -> Path:
    path = Path(str(raw))
    return path if path.is_absolute() else (base / path).resolve()


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


def iter_template_paths(roots: Iterable[Path]) -> list[Path]:
    paths: list[Path] = []
    for root in roots:
        resolved = root.resolve()
        if resolved.is_file() and resolved.name == TEMPLATE_NAME:
            paths.append(resolved)
        elif resolved.is_dir():
            paths.extend(path.resolve() for path in resolved.rglob(TEMPLATE_NAME))
    return sorted(set(paths))


def slugify(value: str) -> str:
    text = value.replace("\\", "/").lower()
    text = re.sub(r"[^a-z0-9]+", "_", text)
    text = re.sub(r"_+", "_", text).strip("_")
    return text or "content"


def titleize(value: str) -> str:
    text = re.sub(r"[_\-]+", " ", value).strip()
    return " ".join(part.capitalize() for part in text.split()) or "Generated Content"


def render_pattern(pattern: str, values: dict[str, str]) -> str:
    rendered = pattern
    for key, value in values.items():
        rendered = rendered.replace("{" + key + "}", value)
    return rendered


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


def range_errors(value: Any, param: dict[str, Any], ctx: str) -> list[str]:
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
        errors.append(f"{ctx}: value for '{param.get('id')}'={value} is outside range [{minimum}, {maximum}]")
    if step is not None and (not is_number(step) or step <= 0):
        errors.append(f"{ctx}: float parameter '{param.get('id')}' range step must be positive")
    return errors


def collect_parameter_errors(parameters: list[Any], ctx: str) -> tuple[dict[str, dict[str, Any]], list[str]]:
    declared: dict[str, dict[str, Any]] = {}
    errors: list[str] = []
    for index, raw in enumerate(parameters):
        param_ctx = f"{ctx}.parameters[{index}]"
        if not isinstance(raw, dict):
            errors.append(f"{param_ctx}: parameter must be an object")
            continue
        param_id = raw.get("id")
        kind = raw.get("kind")
        label = raw.get("label")
        if not isinstance(param_id, str) or not param_id:
            errors.append(f"{param_ctx}: parameter id must be a non-empty string")
            continue
        if param_id in declared:
            errors.append(f"{param_ctx}: duplicate parameter id '{param_id}'")
        declared[param_id] = raw
        if kind not in {"float", "bool", "string"}:
            errors.append(f"{param_ctx}: parameter kind must be float, bool, or string")
        elif "default" in raw and not value_matches_kind(raw.get("default"), str(kind)):
            errors.append(f"{param_ctx}: default for '{param_id}' must match kind {kind}")
        if not isinstance(label, str) or ":" not in label:
            errors.append(f"{param_ctx}: label should use canonical 'Section: Label' form")
        if kind == "float":
            errors.extend(range_errors(raw.get("default"), raw, param_ctx))
        if raw.get("options") is not None and raw.get("optionsSource") is not None:
            errors.append(f"{param_ctx}: use either options[] or optionsSource, not both")
        for opt_index, option in enumerate(as_list(raw.get("options"))):
            if not isinstance(option, dict):
                errors.append(f"{param_ctx}.options[{opt_index}]: option must be an object")
                continue
            if "value" not in option or not value_matches_kind(option.get("value"), str(kind)):
                errors.append(f"{param_ctx}.options[{opt_index}]: option value must match parameter kind {kind}")
        options_source = raw.get("optionsSource")
        if options_source is not None:
            if not isinstance(options_source, dict):
                errors.append(f"{param_ctx}.optionsSource: must be an object")
            else:
                for field in ("id", "value", "label"):
                    if not isinstance(options_source.get(field), str) or not options_source.get(field):
                        errors.append(f"{param_ctx}.optionsSource.{field}: must be a non-empty string")
    return declared, errors


def normalize_parameter(raw: dict[str, Any], default: Any) -> dict[str, Any]:
    entry: dict[str, Any] = {
        "id": str(raw.get("id", "")),
        "kind": str(raw.get("kind", "")),
        "label": str(raw.get("label", "")),
        "default": default,
    }
    if "range" in raw:
        range_meta = as_dict(raw.get("range"))
        entry["range"] = {
            "min": range_meta.get("min"),
            "max": range_meta.get("max"),
            "step": range_meta.get("step"),
        }
    if "units" in raw:
        entry["units"] = str(raw.get("units", ""))
    if "options" in raw:
        entry["options"] = as_list(raw.get("options"))
    if "optionsSource" in raw:
        entry["optionsSource"] = as_dict(raw.get("optionsSource"))
    return entry


def content_paths(template_path: Path, template: dict[str, Any]) -> tuple[list[Path], Path]:
    content = as_dict(template.get("content"))
    content_root = resolve_path(template_path.parent, content.get("root", "."))
    glob = str(content.get("glob", ""))
    paths = sorted(path.resolve() for path in content_root.glob(glob) if path.is_file())
    return paths, content_root


def sidecar_path_for(content_path: Path, suffix: str) -> Path:
    return content_path.with_name(content_path.stem + suffix)


def normalize_content(
    template_path: Path,
    template: dict[str, Any],
    content_path: Path,
    content_root: Path,
    params: dict[str, dict[str, Any]],
) -> tuple[dict[str, Any] | None, list[str]]:
    errors: list[str] = []
    content = as_dict(template.get("content"))
    asset = as_dict(template.get("asset"))
    source = as_dict(template.get("source"))
    compatibility = as_dict(template.get("compatibility"))
    sidecar_suffix = str(content.get("sidecarSuffix", ".generated_layer.json"))
    sidecar_path = sidecar_path_for(content_path, sidecar_suffix)
    sidecar: dict[str, Any] = {}

    if sidecar_path.exists():
        try:
            raw_sidecar = load_json(sidecar_path)
        except json.JSONDecodeError as exc:
            return None, [f"{rel(sidecar_path)}: JSON parse error: {exc}"]
        errors.extend(schema_errors(raw_sidecar, SIDECAR_SCHEMA, sidecar_path))
        if not isinstance(raw_sidecar, dict):
            return None, [f"{rel(sidecar_path)}: sidecar must be a JSON object"]
        sidecar = raw_sidecar
        if sidecar.get("templateId") != template.get("templateId"):
            errors.append(f"{rel(sidecar_path)}: templateId must match '{template.get('templateId')}'")
        declared_content = resolve_path(sidecar_path.parent, sidecar.get("contentFile", ""))
        if declared_content != content_path.resolve():
            errors.append(f"{rel(sidecar_path)}: contentFile must point to {content_path.name}")

    try:
        content_rel = content_path.resolve().relative_to(content_root.resolve()).with_suffix("")
    except ValueError:
        content_rel = Path(content_path.stem)
    content_slug = slugify(content_rel.as_posix())
    content_title = titleize(str(sidecar.get("label") or content_path.stem))
    values = {
        "contentSlug": content_slug,
        "contentTitle": content_title,
        "contentFile": content_path.name,
    }
    asset_id = render_pattern(str(asset.get("idPattern", "")), values)
    registry_prefix = render_pattern(str(asset.get("registryPrefixPattern", "")), values)
    label = str(sidecar.get("label") or render_pattern(str(asset.get("labelPattern", "")), values))
    description = str(sidecar.get("description") or render_pattern(str(asset.get("descriptionPattern", "")), values))

    if "{contentSlug}" not in str(asset.get("idPattern", "")):
        errors.append(f"{rel(template_path)}: asset.idPattern must include '{{contentSlug}}'")
    if "{contentSlug}" not in str(asset.get("registryPrefixPattern", "")):
        errors.append(f"{rel(template_path)}: asset.registryPrefixPattern must include '{{contentSlug}}'")

    if content_path.suffix.lower() != ".stl":
        errors.append(f"{rel(content_path)}: first generated-layer template slice only supports .stl files")
    if not content_path.read_text(encoding="utf-8", errors="ignore").strip():
        errors.append(f"{rel(content_path)}: content file is empty")

    sidecar_defaults = as_dict(sidecar.get("defaults"))
    normalized_parameters: list[dict[str, Any]] = []
    for param_id, param in params.items():
        default = sidecar_defaults.get(param_id, param.get("default"))
        if not value_matches_kind(default, str(param.get("kind"))):
            errors.append(f"{rel(sidecar_path)}: default for '{param_id}' must match kind {param.get('kind')}")
        errors.extend(range_errors(default, param, rel(sidecar_path) if sidecar else rel(template_path)))
        normalized_parameters.append(normalize_parameter(param, default))
    for override_id in sidecar_defaults:
        if override_id not in params:
            errors.append(f"{rel(sidecar_path)}: default '{override_id}' is not declared by template parameters")

    tags = [str(item) for item in as_list(asset.get("tags"))]
    tags.extend(str(item) for item in as_list(sidecar.get("tags")))
    entry = {
        "id": asset_id,
        "label": label,
        "category": str(asset.get("category", "")),
        "layerGroup": str(asset.get("layerGroup", "")),
        "model": str(asset.get("model", "")),
        "stateModel": str(asset.get("stateModel", "")),
        "type": str(asset.get("type", "")),
        "kind": "generated-content-layer",
        "registryPrefix": registry_prefix,
        "templateId": str(template.get("templateId", "")),
        "templatePath": rel(template_path),
        "content": {
            "kind": str(content.get("kind", "")),
            "path": rel(content_path),
            "sidecar": rel(sidecar_path) if sidecar else "",
            "physicalScale": sidecar.get("physicalScale"),
        },
        "sourceStrategy": str(source.get("strategy", "template-generated")),
        "implementationType": str(source.get("implementationType", "")),
        "manifestInspectionOnly": bool(compatibility.get("manifestInspectionOnly", False)),
        "runtimeLoadingImplemented": bool(compatibility.get("runtimeLoadingImplemented", False)),
        "description": description,
        "tags": sorted(set(tags)),
        "parameterCount": len(normalized_parameters),
        "defaultKeys": sorted(params.keys()),
        "parameters": sorted(normalized_parameters, key=lambda item: item["id"]),
    }
    return entry, errors


def normalize_template(template_path: Path) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        template = load_json(template_path)
    except json.JSONDecodeError as exc:
        return [], [f"{rel(template_path)}: JSON parse error: {exc}"]
    errors = schema_errors(template, TEMPLATE_SCHEMA, template_path)
    if not isinstance(template, dict):
        return [], [f"{rel(template_path)}: template must be a JSON object"]

    params, param_errors = collect_parameter_errors(as_list(template.get("parameters")), rel(template_path))
    errors.extend(param_errors)
    content_files, content_root = content_paths(template_path, template)
    if not content_files:
        errors.append(f"{rel(template_path)}: template did not discover any content files")

    entries: list[dict[str, Any]] = []
    for content_path in content_files:
        entry, content_errors = normalize_content(template_path, template, content_path, content_root, params)
        errors.extend(content_errors)
        if entry is not None:
            entries.append(entry)
    return entries, errors


def legacy_asset_ids() -> set[str]:
    catalog = layer_catalog_regression.build_catalog()
    entries = catalog.get("entries", [])
    ids = {str(entry.get("id", "")) for entry in entries if isinstance(entry, dict)}
    return ids


def build_catalog(roots: Iterable[Path] | None = None) -> tuple[dict[str, Any], list[str]]:
    selected_roots = tuple(roots or DEFAULT_ROOTS)
    template_paths = iter_template_paths(selected_roots)
    entries: list[dict[str, Any]] = []
    errors: list[str] = []
    for template_path in template_paths:
        template_entries, template_errors = normalize_template(template_path)
        entries.extend(template_entries)
        errors.extend(template_errors)

    entries.sort(key=lambda item: (item["category"], item["layerGroup"], item["label"]))
    seen_ids: dict[str, str] = {}
    duplicate_ids: list[str] = []
    for entry in entries:
        entry_id = str(entry.get("id", ""))
        if entry_id in seen_ids:
            duplicate_ids.append(entry_id)
        else:
            seen_ids[entry_id] = str(entry.get("content", {}).get("path", ""))

    legacy_ids = legacy_asset_ids()
    legacy_conflicts = sorted({str(entry.get("id", "")) for entry in entries} & legacy_ids)
    if duplicate_ids:
        errors.append(f"duplicate generated asset IDs: {', '.join(sorted(set(duplicate_ids)))}")
    if legacy_conflicts:
        errors.append(f"generated asset IDs conflict with legacy layer assets: {', '.join(legacy_conflicts)}")

    parameter_count = sum(int(entry.get("parameterCount", 0)) for entry in entries)
    categories: dict[str, int] = {}
    layer_groups: dict[str, int] = {}
    for entry in entries:
        categories[entry["category"]] = categories.get(entry["category"], 0) + 1
        if entry["layerGroup"]:
            layer_groups[entry["layerGroup"]] = layer_groups.get(entry["layerGroup"], 0) + 1

    catalog = {
        "schemaVersion": 1,
        "status": "draft",
        "sourceStrategy": [
            "Static generated-layer template expansion for docs/examples content files.",
            "Generated entries are compatibility fixtures only and are not loaded by the runtime.",
            "Legacy layer catalog IDs are checked so generated content cannot collide silently.",
        ],
        "sources": [rel(root) for root in selected_roots]
        + [
            "docs/schemas/generated_layer_template.schema.json",
            "docs/schemas/generated_layer_sidecar.schema.json",
            "tools/generated_layer_catalog_regression.py",
        ],
        "counts": {
            "templates": len(template_paths),
            "generatedEntries": len(entries),
            "parameters": parameter_count,
            "categories": len(categories),
            "layerGroups": len(layer_groups),
            "legacyConflicts": len(legacy_conflicts),
        },
        "categories": dict(sorted(categories.items())),
        "layerGroups": dict(sorted(layer_groups.items())),
        "legacyCompatibility": {
            "conflicts": legacy_conflicts,
        },
        "entries": entries,
    }
    return catalog, errors


def check_catalog(expected_path: Path, roots: Iterable[Path]) -> int:
    actual, errors = build_catalog(roots)
    if errors:
        for error in errors:
            print(f"generated layer catalog error: {error}", file=sys.stderr)
        return 1
    actual_text = dumps(actual)
    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    if actual_text != expected_text:
        print(f"Generated layer catalog snapshot is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated layer catalog",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1
    counts = actual["counts"]
    print(
        "Generated layer catalog regression passed "
        f"({counts['templates']} templates, {counts['generatedEntries']} entries, "
        f"{counts['parameters']} parameters, {counts['legacyConflicts']} legacy conflicts)"
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected generated-layer snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--root", type=Path, action="append", help="Generated-layer fixture root")
    parser.add_argument("--expected", type=Path, default=DEFAULT_EXPECTED, help="Expected snapshot path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    roots = tuple((REPO_ROOT / root).resolve() if not root.is_absolute() else root for root in (args.root or ()))
    if not roots:
        roots = DEFAULT_ROOTS
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        actual, errors = build_catalog(roots)
        if errors:
            for error in errors:
                print(f"generated layer catalog error: {error}", file=sys.stderr)
            return 1
        expected_path.parent.mkdir(parents=True, exist_ok=True)
        expected_path.write_text(dumps(actual), encoding="utf-8")
        print(f"Wrote generated layer catalog snapshot: {rel(expected_path)}")
        return 0
    return check_catalog(expected_path, roots)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

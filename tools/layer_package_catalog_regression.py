#!/usr/bin/env python3
"""Generate/check a golden snapshot of layer package discovery output."""
from __future__ import annotations

import argparse
import difflib
import json
import sys
from pathlib import Path
from typing import Any, Iterable

import layer_package_discovery
import validate_layer_packages

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROOTS = layer_package_discovery.fixture_roots()
DEFAULT_ROOT = DEFAULT_ROOTS[0]
DEFAULT_EXPECTED = REPO_ROOT / "tools" / "testdata" / "layer_packages" / "expected_package_catalog.json"


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


def normalize_parameter(raw: dict[str, Any]) -> dict[str, Any]:
    entry: dict[str, Any] = {
        "id": str(raw.get("id", "")),
        "kind": str(raw.get("kind", "")),
        "label": str(raw.get("label", "")),
        "default": raw.get("default"),
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
    if "deprecated" in raw:
        entry["deprecated"] = as_dict(raw.get("deprecated"))
    return entry


def normalize_package(package_path: Path) -> tuple[dict[str, Any] | None, list[str]]:
    summary, errors = validate_layer_packages.validate_package(package_path)
    if errors:
        return None, errors
    package = load_json(package_path)
    if not isinstance(package, dict) or summary is None:
        return None, [f"{rel(package_path)}: package must be a JSON object"]

    asset = as_dict(package.get("asset"))
    source = as_dict(package.get("source"))
    compatibility = as_dict(package.get("compatibility"))
    parameters = [
        normalize_parameter(param)
        for param in as_list(package.get("parameters"))
        if isinstance(param, dict)
    ]
    parameter_ids = [param["id"] for param in parameters]

    presets = []
    for preset in as_list(package.get("presets")):
        if isinstance(preset, dict):
            presets.append(
                {
                    "presetId": str(preset.get("presetId", "")),
                    "label": str(preset.get("label", "")),
                    "path": str(preset.get("path", "")),
                }
            )

    mapping_presets = []
    for mapping_preset in as_list(package.get("mappingPresets")):
        if not isinstance(mapping_preset, dict):
            continue
        mapping_presets.append(
            {
                "id": str(mapping_preset.get("id", "")),
                "label": str(mapping_preset.get("label", "")),
                "targets": [
                    str(mapping.get("target", ""))
                    for mapping in as_list(mapping_preset.get("mappings"))
                    if isinstance(mapping, dict)
                ],
                "patterns": [
                    str(as_dict(mapping.get("source")).get("pattern", ""))
                    for mapping in as_list(mapping_preset.get("mappings"))
                    if isinstance(mapping, dict)
                ],
            }
        )

    entry: dict[str, Any] = {
        "id": str(asset.get("id", "")),
        "label": str(asset.get("label", "")),
        "category": str(asset.get("category", "")),
        "layerGroup": str(asset.get("layerGroup", "")),
        "model": str(asset.get("model", "")),
        "stateModel": str(asset.get("stateModel", "")),
        "type": str(asset.get("type", "")),
        "kind": "package-layer",
        "registryPrefix": str(asset.get("registryPrefix", "")),
        "packageId": str(package.get("packageId", "")),
        "packageVersion": str(package.get("packageVersion", "")),
        "path": rel(package_path),
        "sourceStrategy": str(source.get("strategy", "")),
        "sourceRegistration": str(source.get("registration", "")),
        "sourceRegistrationRequired": bool(compatibility.get("sourceRegistrationRequired", False)),
        "manifestInspectionOnly": bool(compatibility.get("manifestInspectionOnly", False)),
        "parameterCount": len(parameters),
        "defaultKeys": sorted(parameter_ids),
        "parameters": parameters,
        "presets": presets,
        "presetBanks": [
            {
                "id": str(bank.get("id", "")),
                "label": str(bank.get("label", "")),
                "presets": [str(item) for item in as_list(bank.get("presets"))],
            }
            for bank in as_list(package.get("presetBanks"))
            if isinstance(bank, dict)
        ],
        "mappingPresets": mapping_presets,
        "bench": str(as_dict(package.get("tests")).get("bench", "")),
    }
    return entry, []


def build_catalog(roots: Iterable[Path] | None = None) -> tuple[dict[str, Any], list[str]]:
    selected_roots = tuple(roots or DEFAULT_ROOTS)
    package_paths = validate_layer_packages.iter_package_paths(selected_roots)
    entries: list[dict[str, Any]] = []
    errors: list[str] = []
    for package_path in package_paths:
        entry, package_errors = normalize_package(package_path)
        errors.extend(package_errors)
        if entry is not None:
            entries.append(entry)
    entries.sort(key=lambda item: (item["category"], item["layerGroup"], item["label"]))

    categories: dict[str, int] = {}
    layer_groups: dict[str, int] = {}
    package_ids: set[str] = set()
    asset_ids: set[str] = set()
    types: dict[str, int] = {}
    parameter_count = 0
    preset_count = 0
    mapping_preset_count = 0
    for entry in entries:
        categories[entry["category"]] = categories.get(entry["category"], 0) + 1
        if entry["layerGroup"]:
            layer_groups[entry["layerGroup"]] = layer_groups.get(entry["layerGroup"], 0) + 1
        package_ids.add(entry["packageId"])
        asset_ids.add(entry["id"])
        types[entry["type"]] = types.get(entry["type"], 0) + 1
        parameter_count += int(entry["parameterCount"])
        preset_count += len(entry["presets"])
        mapping_preset_count += len(entry["mappingPresets"])

    catalog = {
        "schemaVersion": 1,
        "sourceStrategy": "Static mirror of layer package folder discovery and package-to-Browser catalog normalization.",
        "discovery": layer_package_discovery.root_report(selected_roots),
        "sources": [rel(root) for root in selected_roots]
        + [
            "docs/schemas/layer_package.schema.json",
            "docs/schemas/layer_preset.schema.json",
            "tools/layer_package_discovery.py",
            "tools/validate_layer_packages.py",
        ],
        "counts": {
            "packages": len(entries),
            "categories": len(categories),
            "layerGroups": len(layer_groups),
            "types": len(types),
            "parameters": parameter_count,
            "presets": preset_count,
            "mappingPresets": mapping_preset_count,
            "packageIds": len(package_ids),
            "assetIds": len(asset_ids),
        },
        "categories": dict(sorted(categories.items())),
        "layerGroups": dict(sorted(layer_groups.items())),
        "types": dict(sorted(types.items())),
        "entries": entries,
    }
    return catalog, errors


def check_catalog(expected_path: Path, roots: Iterable[Path]) -> int:
    actual, errors = build_catalog(roots)
    if errors:
        for error in errors:
            print(f"layer package catalog error: {error}", file=sys.stderr)
        return 1
    actual_text = dumps(actual)
    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    if actual_text != expected_text:
        print(f"Layer package catalog snapshot is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated layer package catalog",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1
    counts = actual["counts"]
    print(
        "Layer package catalog regression passed "
        f"({counts['packages']} packages, {counts['parameters']} parameters, "
        f"{counts['presets']} presets)"
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected package catalog snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--root", type=Path, action="append", help="Package discovery root")
    parser.add_argument("--expected", type=Path, default=DEFAULT_EXPECTED, help="Expected snapshot path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    roots = layer_package_discovery.roots_from_args(args.root, DEFAULT_ROOTS)
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        actual, errors = build_catalog(roots)
        if errors:
            for error in errors:
                print(f"layer package catalog error: {error}", file=sys.stderr)
            return 1
        expected_path.parent.mkdir(parents=True, exist_ok=True)
        expected_path.write_text(dumps(actual), encoding="utf-8")
        print(f"Wrote layer package catalog snapshot: {rel(expected_path)}")
        return 0
    return check_catalog(expected_path, roots)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

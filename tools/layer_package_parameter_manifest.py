#!/usr/bin/env python3
"""Generate/check package-derived public parameter manifest output."""
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
DEFAULT_EXPECTED = REPO_ROOT / "tools" / "testdata" / "layer_packages" / "expected_package_parameter_manifest.json"
SLOT_RANGE = [1, 8]


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


def normalize_parameter(package_path: Path, package: dict[str, Any], raw: dict[str, Any]) -> dict[str, Any]:
    asset = as_dict(package.get("asset"))
    groups = {
        str(group.get("id", "")): group
        for group in as_list(package.get("parameterGroups"))
        if isinstance(group, dict)
    }
    group = as_dict(groups.get(str(raw.get("groupId", ""))))
    prefix = str(asset.get("registryPrefix", ""))
    suffix = str(raw.get("id", ""))
    entry: dict[str, Any] = {
        "id": f"{prefix}.{suffix}",
        "kind": str(raw.get("kind", "")),
        "scope": "layer-package",
        "family": str(group.get("label", "")),
        "units": str(raw.get("units", "")),
        "packageId": str(package.get("packageId", "")),
        "assetId": str(asset.get("id", "")),
        "layerType": str(asset.get("type", "")),
        "registryPrefix": prefix,
        "suffix": suffix,
        "label": str(raw.get("label", "")),
        "groupId": str(raw.get("groupId", "")),
        "visible": bool(raw.get("visible", True)),
        "default": raw.get("default"),
        "source": {
            "path": rel(package_path),
            "kind": "layer.package.json"
        }
    }
    if "range" in raw:
        range_meta = as_dict(raw.get("range"))
        entry["range"] = {
            "min": range_meta.get("min"),
            "max": range_meta.get("max"),
            "step": range_meta.get("step"),
        }
    if "options" in raw:
        entry["options"] = as_list(raw.get("options"))
    if "optionsSource" in raw:
        entry["optionsSource"] = as_dict(raw.get("optionsSource"))
    if "deprecated" in raw:
        entry["deprecated"] = as_dict(raw.get("deprecated"))
    return entry


def normalize_console_template(package: dict[str, Any], raw: dict[str, Any]) -> dict[str, Any]:
    asset = as_dict(package.get("asset"))
    suffix = str(raw.get("id", ""))
    entry: dict[str, Any] = {
        "idPattern": f"console.layer{{slot}}.{suffix}",
        "kind": str(raw.get("kind", "")),
        "suffix": suffix,
        "slotRange": SLOT_RANGE,
        "sourcePackages": [str(package.get("packageId", ""))],
        "sourceLayerTypes": [str(asset.get("type", ""))],
    }
    return entry


def build_manifest(roots: Iterable[Path] | None = None) -> tuple[dict[str, Any], list[str]]:
    selected_roots = tuple(roots or DEFAULT_ROOTS)
    package_paths = validate_layer_packages.iter_package_paths(selected_roots)
    parameters: list[dict[str, Any]] = []
    templates_by_suffix_kind: dict[tuple[str, str], dict[str, Any]] = {}
    errors: list[str] = []
    package_ids: set[str] = set()
    asset_ids: set[str] = set()

    for package_path in package_paths:
        summary, package_errors = validate_layer_packages.validate_package(package_path)
        errors.extend(package_errors)
        if package_errors or summary is None:
            continue
        package = load_json(package_path)
        if not isinstance(package, dict):
            errors.append(f"{rel(package_path)}: package must be a JSON object")
            continue
        asset = as_dict(package.get("asset"))
        package_ids.add(str(package.get("packageId", "")))
        asset_ids.add(str(asset.get("id", "")))
        for raw in as_list(package.get("parameters")):
            if not isinstance(raw, dict):
                continue
            parameters.append(normalize_parameter(package_path, package, raw))
            template = normalize_console_template(package, raw)
            key = (template["suffix"], template["kind"])
            if key not in templates_by_suffix_kind:
                templates_by_suffix_kind[key] = template
            else:
                existing = templates_by_suffix_kind[key]
                for field in ("sourcePackages", "sourceLayerTypes"):
                    merged = sorted(set(existing[field]) | set(template[field]))
                    existing[field] = merged

    parameters.sort(key=lambda item: item["id"])
    console_templates = sorted(
        templates_by_suffix_kind.values(),
        key=lambda item: (item["suffix"], item["kind"]),
    )

    manifest = {
        "schemaVersion": 1,
        "status": "draft",
        "generator": "tools/layer_package_parameter_manifest.py",
        "sourceStrategy": [
            "Package parameter declarations are expanded through asset.registryPrefix.",
            "Console slot templates use package parameter suffixes and do not imply runtime registration yet."
        ],
        "discovery": layer_package_discovery.root_report(selected_roots),
        "sources": [rel(root) for root in selected_roots]
        + [
            "docs/schemas/layer_package.schema.json",
            "tools/layer_package_discovery.py",
            "tools/validate_layer_packages.py"
        ],
        "counts": {
            "packages": len(package_ids),
            "assets": len(asset_ids),
            "parameters": len(parameters),
            "consoleSlotTemplates": len(console_templates),
        },
        "parameters": parameters,
        "consoleSlotTemplates": console_templates,
    }
    return manifest, errors


def check_manifest(expected_path: Path, roots: Iterable[Path]) -> int:
    actual, errors = build_manifest(roots)
    if errors:
        for error in errors:
            print(f"layer package parameter manifest error: {error}", file=sys.stderr)
        return 1
    actual_text = dumps(actual)
    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    if actual_text != expected_text:
        print(f"Layer package parameter manifest snapshot is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated layer package parameter manifest",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1
    counts = actual["counts"]
    print(
        "Layer package parameter manifest passed "
        f"({counts['packages']} packages, {counts['parameters']} parameters, "
        f"{counts['consoleSlotTemplates']} console templates)"
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected package parameter manifest snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--root", type=Path, action="append", help="Package discovery root")
    parser.add_argument("--expected", type=Path, default=DEFAULT_EXPECTED, help="Expected snapshot path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    roots = layer_package_discovery.roots_from_args(args.root, DEFAULT_ROOTS)
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        actual, errors = build_manifest(roots)
        if errors:
            for error in errors:
                print(f"layer package parameter manifest error: {error}", file=sys.stderr)
            return 1
        expected_path.parent.mkdir(parents=True, exist_ok=True)
        expected_path.write_text(dumps(actual), encoding="utf-8")
        print(f"Wrote layer package parameter manifest snapshot: {rel(expected_path)}")
        return 0
    return check_manifest(expected_path, roots)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

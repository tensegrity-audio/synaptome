#!/usr/bin/env python3
"""Generate/check a draft read-only Browser inspection payload for layer metadata."""
from __future__ import annotations

import argparse
import difflib
import json
import sys
from pathlib import Path
from typing import Any

import generated_layer_catalog_regression
import layer_package_catalog_regression

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_EXPECTED = (
    REPO_ROOT
    / "tools"
    / "testdata"
    / "layer_browser_inspection"
    / "expected_layer_browser_inspection_payload.json"
)


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def dumps(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def as_dict(value: Any) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


def as_list(value: Any) -> list[Any]:
    return value if isinstance(value, list) else []


def parameter_section(label: str) -> str:
    return label.split(":", 1)[0].strip() if ":" in label else "Other"


def normalize_parameter(raw: dict[str, Any]) -> dict[str, Any]:
    label = str(raw.get("label", ""))
    entry: dict[str, Any] = {
        "id": str(raw.get("id", "")),
        "kind": str(raw.get("kind", "")),
        "label": label,
        "section": parameter_section(label),
        "default": raw.get("default"),
    }
    if "range" in raw:
        entry["range"] = as_dict(raw.get("range"))
    if "units" in raw:
        entry["units"] = str(raw.get("units", ""))
    if "options" in raw:
        entry["options"] = as_list(raw.get("options"))
    if "optionsSource" in raw:
        entry["optionsSource"] = as_dict(raw.get("optionsSource"))
    if "deprecated" in raw:
        entry["deprecated"] = as_dict(raw.get("deprecated"))
    return entry


def controls_summary(parameters: list[dict[str, Any]]) -> dict[str, Any]:
    sections: dict[str, int] = {}
    for param in parameters:
        section = str(param.get("section", "Other"))
        sections[section] = sections.get(section, 0) + 1
    return {
        "count": len(parameters),
        "sections": dict(sorted(sections.items())),
        "parameters": sorted(parameters, key=lambda item: (item["section"], item["label"], item["id"])),
    }


def normalize_package_entry(raw: dict[str, Any]) -> dict[str, Any]:
    parameters = [
        normalize_parameter(param)
        for param in as_list(raw.get("parameters"))
        if isinstance(param, dict)
    ]
    return {
        "assetId": str(raw.get("id", "")),
        "label": str(raw.get("label", "")),
        "category": str(raw.get("category", "")),
        "layerGroup": str(raw.get("layerGroup", "")),
        "kind": "package-layer",
        "layerType": str(raw.get("type", "")),
        "registryPrefix": str(raw.get("registryPrefix", "")),
        "source": {
            "kind": "layer-package",
            "path": str(raw.get("path", "")),
            "strategy": str(raw.get("sourceStrategy", "")),
            "sourceRegistrationRequired": bool(raw.get("sourceRegistrationRequired", False)),
        },
        "inspection": {
            "manifestInspectionOnly": bool(raw.get("manifestInspectionOnly", False)),
            "runtimeLoadable": False,
            "requiresInstantiation": False,
            "mutatesScene": False,
        },
        "controls": controls_summary(parameters),
        "presets": [
            {
                "presetId": str(preset.get("presetId", "")),
                "label": str(preset.get("label", "")),
                "path": str(preset.get("path", "")),
            }
            for preset in as_list(raw.get("presets"))
            if isinstance(preset, dict)
        ],
        "presetBanks": [
            {
                "id": str(bank.get("id", "")),
                "label": str(bank.get("label", "")),
                "presets": [str(item) for item in as_list(bank.get("presets"))],
            }
            for bank in as_list(raw.get("presetBanks"))
            if isinstance(bank, dict)
        ],
        "mappingPresets": [
            {
                "id": str(preset.get("id", "")),
                "label": str(preset.get("label", "")),
                "targets": [str(item) for item in as_list(preset.get("targets"))],
                "patterns": [str(item) for item in as_list(preset.get("patterns"))],
            }
            for preset in as_list(raw.get("mappingPresets"))
            if isinstance(preset, dict)
        ],
    }


def normalize_generated_entry(raw: dict[str, Any]) -> dict[str, Any]:
    parameters = [
        normalize_parameter(param)
        for param in as_list(raw.get("parameters"))
        if isinstance(param, dict)
    ]
    content = as_dict(raw.get("content"))
    return {
        "assetId": str(raw.get("id", "")),
        "label": str(raw.get("label", "")),
        "category": str(raw.get("category", "")),
        "layerGroup": str(raw.get("layerGroup", "")),
        "kind": "generated-content-layer",
        "layerType": str(raw.get("type", "")),
        "registryPrefix": str(raw.get("registryPrefix", "")),
        "source": {
            "kind": "generated-content",
            "templateId": str(raw.get("templateId", "")),
            "templatePath": str(raw.get("templatePath", "")),
            "strategy": str(raw.get("sourceStrategy", "")),
        },
        "inspection": {
            "manifestInspectionOnly": bool(raw.get("manifestInspectionOnly", False)),
            "runtimeLoadable": bool(raw.get("runtimeLoadingImplemented", False)),
            "requiresInstantiation": False,
            "mutatesScene": False,
        },
        "content": {
            "kind": str(content.get("kind", "")),
            "path": str(content.get("path", "")),
            "sidecar": str(content.get("sidecar", "")),
            "physicalScale": content.get("physicalScale"),
        },
        "controls": controls_summary(parameters),
        "presets": [],
        "presetBanks": [],
        "mappingPresets": [],
    }


def build_payload() -> tuple[dict[str, Any], list[str]]:
    package_catalog, package_errors = layer_package_catalog_regression.build_catalog()
    generated_catalog, generated_errors = generated_layer_catalog_regression.build_catalog()
    errors = list(package_errors) + list(generated_errors)

    package_entries = [
        normalize_package_entry(entry)
        for entry in as_list(package_catalog.get("entries"))
        if isinstance(entry, dict)
    ]
    generated_entries = [
        normalize_generated_entry(entry)
        for entry in as_list(generated_catalog.get("entries"))
        if isinstance(entry, dict)
    ]
    entries = sorted(
        package_entries + generated_entries,
        key=lambda item: (item["category"], item["layerGroup"], item["label"], item["assetId"]),
    )

    seen_ids: set[str] = set()
    duplicates: set[str] = set()
    for entry in entries:
        asset_id = entry["assetId"]
        if asset_id in seen_ids:
            duplicates.add(asset_id)
        seen_ids.add(asset_id)
        inspection = as_dict(entry.get("inspection"))
        if inspection.get("requiresInstantiation"):
            errors.append(f"{asset_id}: read-only inspection payload must not require instantiation")
        if inspection.get("mutatesScene"):
            errors.append(f"{asset_id}: read-only inspection payload must not mutate scenes")
        if inspection.get("runtimeLoadable"):
            errors.append(f"{asset_id}: draft inspection payload must not claim runtime loading")
    if duplicates:
        errors.append(f"duplicate inspection asset IDs: {', '.join(sorted(duplicates))}")

    categories: dict[str, int] = {}
    layer_groups: dict[str, int] = {}
    kinds: dict[str, int] = {}
    parameter_count = 0
    preset_count = 0
    mapping_preset_count = 0
    for entry in entries:
        categories[entry["category"]] = categories.get(entry["category"], 0) + 1
        if entry["layerGroup"]:
            layer_groups[entry["layerGroup"]] = layer_groups.get(entry["layerGroup"], 0) + 1
        kinds[entry["kind"]] = kinds.get(entry["kind"], 0) + 1
        parameter_count += int(as_dict(entry.get("controls")).get("count", 0))
        preset_count += len(as_list(entry.get("presets")))
        mapping_preset_count += len(as_list(entry.get("mappingPresets")))

    payload = {
        "schemaVersion": 1,
        "status": "draft-read-only",
        "sourceStrategy": [
            "Read-only inspection payload assembled from package and generated-layer fixture metadata.",
            "This payload is a docs/tools contract only and is not consumed by the Browser yet.",
            "Entries must not require layer instantiation, scene mutation, runtime scanning, or canonical manifest changes.",
        ],
        "sources": [
            "tools/layer_package_catalog_regression.py",
            "tools/generated_layer_catalog_regression.py",
            "docs/schemas/layer_browser_inspection_payload.schema.json",
            "docs/examples/layer_packages",
            "docs/examples/generated_layers",
        ],
        "counts": {
            "entries": len(entries),
            "packageEntries": len(package_entries),
            "generatedEntries": len(generated_entries),
            "parameters": parameter_count,
            "presets": preset_count,
            "mappingPresets": mapping_preset_count,
            "categories": len(categories),
            "layerGroups": len(layer_groups),
        },
        "categories": dict(sorted(categories.items())),
        "layerGroups": dict(sorted(layer_groups.items())),
        "kinds": dict(sorted(kinds.items())),
        "entries": entries,
    }
    return payload, errors


def check_payload(expected_path: Path) -> int:
    actual, errors = build_payload()
    if errors:
        for error in errors:
            print(f"inspection payload error: {error}", file=sys.stderr)
        return 1
    actual_text = dumps(actual)
    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    if actual_text != expected_text:
        print(f"Layer Browser inspection payload is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated layer browser inspection payload",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1
    counts = actual["counts"]
    print(
        "Layer Browser inspection payload passed "
        f"({counts['entries']} entries, {counts['parameters']} parameters, "
        f"{counts['presets']} presets, {counts['mappingPresets']} mapping presets)"
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected inspection payload snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--expected", type=Path, default=DEFAULT_EXPECTED, help="Expected snapshot path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        actual, errors = build_payload()
        if errors:
            for error in errors:
                print(f"inspection payload error: {error}", file=sys.stderr)
            return 1
        expected_path.parent.mkdir(parents=True, exist_ok=True)
        expected_path.write_text(dumps(actual), encoding="utf-8")
        print(f"Wrote Layer Browser inspection payload snapshot: {rel(expected_path)}")
        return 0
    return check_payload(expected_path)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

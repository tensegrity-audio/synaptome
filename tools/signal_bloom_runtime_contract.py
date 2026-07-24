#!/usr/bin/env python3
"""Check that the vetted Signal Bloom runtime adapter matches its package."""
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

import layer_package_runtime_adapter
import validate_layer_packages

REPO_ROOT = Path(__file__).resolve().parents[1]
PACKAGE_PATH = REPO_ROOT / "docs/examples/layer_packages/signal_bloom/layer.package.json"
CATALOG_PATH = REPO_ROOT / "synaptome/bin/data/layers-optional/examples.signal_bloom.json"
ACTIVATION_PATH = REPO_ROOT / "synaptome/bin/data/config/layer-packages.json"


def load(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ValueError(f"{path.relative_to(REPO_ROOT)} must be an object")
    return value


def main() -> int:
    errors: list[str] = []
    summary, package_errors = validate_layer_packages.validate_package(PACKAGE_PATH)
    errors.extend(package_errors)
    package = load(PACKAGE_PATH)
    catalog = load(CATALOG_PATH)
    activation = load(ACTIVATION_PATH)
    expected_catalog, adapter_errors = layer_package_runtime_adapter.build_adapter(PACKAGE_PATH)
    errors.extend(adapter_errors)
    if expected_catalog is None:
        errors.append("optional catalog adapter could not be generated")
    elif catalog != expected_catalog:
        errors.append(
            "optional catalog adapter is stale; run "
            "python tools\\synaptome_layer.py runtime-adapter "
            "docs\\examples\\layer_packages\\signal_bloom\\layer.package.json "
            "--output synaptome\\bin\\data\\layers-optional\\examples.signal_bloom.json --write"
        )

    package_defaults = {
        str(parameter.get("id")): parameter.get("default")
        for parameter in package.get("parameters", [])
        if isinstance(parameter, dict) and "default" in parameter
    }
    expected_presets: dict[str, Any] = {}
    for reference in package.get("presets", []):
        if not isinstance(reference, dict):
            continue
        preset_path = PACKAGE_PATH.parent / str(reference.get("path", ""))
        preset = load(preset_path)
        expected_presets[str(reference.get("presetId", ""))] = preset.get("parameters", {})
    package_mapping_ids = {
        str(item.get("id"))
        for item in package.get("mappingPresets", [])
        if isinstance(item, dict)
    }
    catalog_mappings = catalog.get("mappingPresets", [])
    catalog_mapping_ids = {
        str(item.get("id"))
        for item in catalog_mappings
        if isinstance(item, dict)
    }
    if catalog_mapping_ids != package_mapping_ids:
        errors.append("generated optional catalog mapping preset IDs do not match the package")
    for item in catalog_mappings:
        if item.get("appliedByDefault") is not False or item.get("ownership") != "suggestion-only":
            errors.append(f"mapping preset {item.get('id')} must remain disabled suggestion-only metadata")

    if activation.get("enabled") is not False:
        errors.append("committed package activation must default to disabled")
    for item in activation.get("packages", []):
        if item.get("id") == package.get("packageId") and item.get("enabled") is not False:
            errors.append("committed Signal Bloom package entry must default to disabled")

    if summary is None:
        errors.append("package summary could not be built")
    if errors:
        for error in errors:
            print(f"Signal Bloom runtime contract error: {error}", file=sys.stderr)
        return 1
    print(
        "Signal Bloom runtime contract passed "
        f"({len(package_defaults)} parameters, {len(expected_presets)} presets, "
        f"{len(package_mapping_ids)} mapping presets, activation default-off)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Generate/check the vetted runtime catalog adapter for one layer package."""
from __future__ import annotations

import difflib
import json
from pathlib import Path
from typing import Any

import validate_layer_packages

REPO_ROOT = Path(__file__).resolve().parents[1]


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def load_object(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ValueError(f"{rel(path)} must be a JSON object")
    return value


def dumps(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def as_dict(value: Any) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


def as_list(value: Any) -> list[Any]:
    return value if isinstance(value, list) else []


def build_adapter(package_path: Path) -> tuple[dict[str, Any] | None, list[str]]:
    package_path = package_path.resolve()
    summary, errors = validate_layer_packages.validate_package(package_path)
    if errors or summary is None:
        return None, errors

    try:
        package = load_object(package_path)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        return None, [str(exc)]

    asset = as_dict(package.get("asset"))
    parameters = [
        parameter
        for parameter in as_list(package.get("parameters"))
        if isinstance(parameter, dict)
    ]
    defaults = {
        str(parameter.get("id", "")): parameter.get("default")
        for parameter in parameters
    }

    presets: dict[str, Any] = {}
    preset_metadata: list[dict[str, str]] = []
    for reference in as_list(package.get("presets")):
        if not isinstance(reference, dict):
            continue
        preset_id = str(reference.get("presetId", ""))
        preset_path = validate_layer_packages.resolve_package_path(
            package_path,
            reference.get("path", ""),
        )
        try:
            preset = load_object(preset_path)
        except (OSError, json.JSONDecodeError, ValueError) as exc:
            errors.append(str(exc))
            continue
        presets[preset_id] = as_dict(preset.get("parameters"))
        preset_metadata.append(
            {
                "id": preset_id,
                "label": str(reference.get("label", preset_id)),
            }
        )

    preset_banks = [
        {
            "id": str(item.get("id", "")),
            "label": str(item.get("label", item.get("id", ""))),
            "presets": [str(preset_id) for preset_id in as_list(item.get("presets"))],
        }
        for item in as_list(package.get("presetBanks"))
        if isinstance(item, dict)
    ]

    mapping_presets = [
        {
            "id": str(item.get("id", "")),
            "appliedByDefault": False,
            "ownership": "suggestion-only",
        }
        for item in as_list(package.get("mappingPresets"))
        if isinstance(item, dict)
    ]

    if errors:
        return None, errors

    adapter = {
        "id": str(asset.get("id", "")),
        "label": str(asset.get("label", "")),
        "category": str(asset.get("category", "")),
        "layerGroup": str(asset.get("layerGroup", "")),
        "model": str(asset.get("model", "")),
        "stateModel": str(asset.get("stateModel", "")),
        "type": str(asset.get("type", "")),
        "registryPrefix": str(asset.get("registryPrefix", "")),
        "opacity": 1.0,
        "defaults": defaults,
        "presets": presets,
        "presetMetadata": preset_metadata,
        "presetBanks": preset_banks,
        "mappingPresets": mapping_presets,
    }
    return adapter, []


def check_adapter(package_path: Path, output_path: Path) -> tuple[bool, list[str]]:
    adapter, errors = build_adapter(package_path)
    if adapter is None or errors:
        return False, errors
    try:
        current = output_path.read_text(encoding="utf-8")
    except OSError as exc:
        return False, [f"{rel(output_path)}: {exc}"]
    expected = dumps(adapter)
    if current == expected:
        return True, []
    diff = "\n".join(
        difflib.unified_diff(
            current.splitlines(),
            expected.splitlines(),
            fromfile=rel(output_path),
            tofile=f"generated from {rel(package_path)}",
            lineterm="",
        )
    )
    return False, [f"{rel(output_path)} is stale\n{diff}"]


def write_adapter(package_path: Path, output_path: Path) -> list[str]:
    adapter, errors = build_adapter(package_path)
    if adapter is None or errors:
        return errors
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(dumps(adapter), encoding="utf-8")
    return []

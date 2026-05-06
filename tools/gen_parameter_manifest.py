#!/usr/bin/env python3
"""Generate/check the Synaptome parameter manifest.

The manifest is intentionally static: it reads the app's C++ registration
sites and layer catalog JSON without instantiating openFrameworks.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]
APP_ROOT = REPO_ROOT / "synaptome"
DEFAULT_OUTPUT = REPO_ROOT / "docs" / "contracts" / "parameter_manifest.json"

PARAM_ID_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z0-9_]+)+$")
CONSOLE_PATTERN_RE = re.compile(r"^console\.layer\{slot\}(?:\.[A-Za-z0-9_]+)+$")


def rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def line_for(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def source_ref(path: Path, line: int) -> dict[str, Any]:
    return {"path": rel(path), "line": line}


def family_for(param_id: str) -> str:
    return param_id.split(".", 1)[0]


def infer_units(param_id: str) -> str:
    suffix = param_id.rsplit(".", 1)[-1]
    lowered = suffix.lower()
    if lowered in {"bpm"}:
        return "bpm"
    if lowered.endswith("hz"):
        return "Hz"
    if lowered.endswith("deg"):
        return "deg"
    if lowered in {"fontsize", "size", "block", "cellsize", "pointsize", "thickness"}:
        return "px"
    if lowered.endswith("beats"):
        return "beats"
    if "opacity" in lowered or "alpha" in lowered or lowered in {"coverage", "mix", "gain"}:
        return "normalized"
    return ""


def value_kind(value: Any) -> str:
    if isinstance(value, bool):
        return "bool"
    if isinstance(value, (int, float)):
        return "float"
    if isinstance(value, str):
        return "string"
    if isinstance(value, list):
        return "array"
    if isinstance(value, dict):
        return "object"
    return type(value).__name__


def default_from_layer(defaults: dict[str, Any], suffix: str) -> Any:
    key = suffix.removeprefix(".")
    if key in defaults:
        return defaults[key]

    channel_maps = {
        "color": ("colorR", "colorG", "colorB"),
        "backgroundColor": ("bgColorR", "bgColorG", "bgColorB"),
        "aliveColor": ("aliveR", "aliveG", "aliveB"),
        "deadColor": ("deadR", "deadG", "deadB"),
        "trailColor": ("trailR", "trailG", "trailB"),
        "bgColor": ("bgR", "bgG", "bgB"),
    }
    for source_key, channel_keys in channel_maps.items():
        if key in channel_keys and source_key in defaults and isinstance(defaults[source_key], list):
            channel_index = channel_keys.index(key)
            if channel_index < len(defaults[source_key]):
                return defaults[source_key][channel_index]
    return None


def add_unique(entries: dict[str, dict[str, Any]], entry: dict[str, Any]) -> None:
    param_id = entry["id"]
    existing = entries.get(param_id)
    if existing is None:
        entries[param_id] = entry
        return
    existing_sources = existing.setdefault("sources", [])
    source = entry.get("source")
    if source and source not in existing_sources:
        existing_sources.append(source)


def parse_factory_types(of_app: Path) -> dict[str, str]:
    text = of_app.read_text(encoding="utf-8")
    pattern = re.compile(
        r'registerType\(\s*"([^"]+)"\s*,\s*\[\]\(\)\s*\{\s*return\s+std::make_unique<([^>]+)>',
        re.MULTILINE,
    )
    return {match.group(1): match.group(2) for match in pattern.finditer(text)}


def parse_core_parameters(of_app: Path) -> list[dict[str, Any]]:
    text = of_app.read_text(encoding="utf-8")
    entries: list[dict[str, Any]] = []
    call_kinds = {
        "addFloat": "float",
        "addBool": "bool",
        "addString": "string",
        "registerHudToggle": "bool",
    }
    for call, kind in call_kinds.items():
        pattern = re.compile(rf"\b{call}\s*\(\s*\"([^\"]+)\"", re.MULTILINE)
        for match in pattern.finditer(text):
            param_id = match.group(1)
            entries.append(
                {
                    "id": param_id,
                    "kind": kind,
                    "scope": "core",
                    "family": family_for(param_id),
                    "units": infer_units(param_id),
                    "source": source_ref(of_app, line_for(text, match.start())),
                }
            )

    sensor_pattern = re.compile(r'\{\s*"(sensors\.[^"]+)"\s*,')
    for match in sensor_pattern.finditer(text):
        param_id = match.group(1)
        entries.append(
            {
                "id": param_id,
                "kind": "float",
                "scope": "sensor",
                "family": "sensors",
                "units": infer_units(param_id),
                "source": source_ref(of_app, line_for(text, match.start())),
            }
        )
    return entries


def parse_source_templates(source_path: Path, class_name: str | None = None) -> list[dict[str, Any]]:
    text = source_path.read_text(encoding="utf-8", errors="replace")
    assignments: dict[str, str] = {}
    for match in re.finditer(r"(\w+_?)\s*=\s*prefix\s*\+\s*\"(\.[^\"]+)\"", text):
        assignments[match.group(1)] = match.group(2)

    templates: list[dict[str, Any]] = []
    direct = re.compile(r"registry\.add(Float|Bool|String)\s*\(\s*prefix\s*\+\s*\"(\.[^\"]+)\"")
    for match in direct.finditer(text):
        kind = match.group(1).lower()
        templates.append(
            {
                "suffix": match.group(2),
                "kind": "string" if kind == "string" else kind,
                "source": source_ref(source_path, line_for(text, match.start())),
            }
        )

    helper_direct = re.compile(r"\bregister(Float|Bool|String)\s*\(\s*registry\s*,\s*prefix\s*\+\s*\"(\.[^\"]+)\"")
    for match in helper_direct.finditer(text):
        kind = match.group(1).lower()
        templates.append(
            {
                "suffix": match.group(2),
                "kind": "string" if kind == "string" else kind,
                "source": source_ref(source_path, line_for(text, match.start())),
            }
        )

    via_variable = re.compile(r"registry\.add(Float|Bool|String)\s*\(\s*(\w+_?)\s*,")
    for match in via_variable.finditer(text):
        variable = match.group(2)
        suffix = assignments.get(variable)
        if not suffix:
            continue
        kind = match.group(1).lower()
        templates.append(
            {
                "suffix": suffix,
                "kind": "string" if kind == "string" else kind,
                "source": source_ref(source_path, line_for(text, match.start())),
            }
        )

    explicit = re.compile(r"registry\.add(Float|Bool|String)\s*\(\s*\"([^\"]+)\"")
    for match in explicit.finditer(text):
        kind = match.group(1).lower()
        templates.append(
            {
                "id": match.group(2),
                "kind": "string" if kind == "string" else kind,
                "source": source_ref(source_path, line_for(text, match.start())),
            }
        )

    dedup: dict[tuple[str, str], dict[str, Any]] = {}
    for template in templates:
        key = (template.get("id") or template.get("suffix") or "", template["kind"])
        template["sourceClass"] = class_name or ""
        dedup[key] = template
    return sorted(dedup.values(), key=lambda item: (item.get("id", ""), item.get("suffix", ""), item["kind"]))


def parse_layer_templates(factory_types: dict[str, str]) -> tuple[dict[str, list[dict[str, Any]]], list[dict[str, Any]]]:
    templates_by_type: dict[str, list[dict[str, Any]]] = {}
    explicit_parameters: list[dict[str, Any]] = []
    class_to_type = {class_name: layer_type for layer_type, class_name in factory_types.items()}

    for source_path in sorted((APP_ROOT / "src" / "visuals").rglob("*.cpp")):
        text = source_path.read_text(encoding="utf-8", errors="replace")
        setup_match = re.search(r"void\s+([A-Za-z0-9_]+)::setup\s*\(\s*ParameterRegistry&\s+registry\s*\)", text)
        class_name = setup_match.group(1) if setup_match else None
        templates = parse_source_templates(source_path, class_name)
        if not templates:
            continue
        for template in templates:
            if "id" in template:
                param_id = template["id"]
                explicit_parameters.append(
                    {
                        "id": param_id,
                        "kind": template["kind"],
                        "scope": "effect",
                        "family": family_for(param_id),
                        "units": infer_units(param_id),
                        "source": template["source"],
                    }
                )
        suffix_templates = [template for template in templates if "suffix" in template]
        if class_name and class_name in class_to_type and suffix_templates:
            templates_by_type[class_to_type[class_name]] = suffix_templates
    return templates_by_type, explicit_parameters


def layer_assets() -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for path in sorted((APP_ROOT / "bin" / "data" / "layers").rglob("*.json")):
        if "scenes" in path.parts:
            continue
        data = load_json(path)
        if not isinstance(data, dict):
            continue
        asset_id = data.get("id")
        layer_type = data.get("type")
        if not isinstance(asset_id, str) or not isinstance(layer_type, str):
            continue
        entries.append(
            {
                "id": asset_id,
                "type": layer_type,
                "label": data.get("label", asset_id),
                "category": data.get("category", ""),
                "registryPrefix": data.get("registryPrefix", asset_id),
                "defaults": data.get("defaults", {}) if isinstance(data.get("defaults", {}), dict) else {},
                "source": source_ref(path, 1),
            }
        )
    return entries


def is_catalog_surface_without_layer_params(layer_type: str) -> bool:
    return layer_type.startswith("fx.") or layer_type == "ui.hud.widget" or layer_type == "text"


def build_manifest() -> dict[str, Any]:
    of_app = APP_ROOT / "src" / "ofApp.cpp"
    factory_types = parse_factory_types(of_app)
    layer_templates, explicit_parameters = parse_layer_templates(factory_types)

    parameters: dict[str, dict[str, Any]] = {}
    for entry in parse_core_parameters(of_app):
        add_unique(parameters, entry)
    for entry in explicit_parameters:
        add_unique(parameters, entry)

    layer_template_entries: list[dict[str, Any]] = []
    console_templates: dict[tuple[str, str], dict[str, Any]] = {}
    for layer_type, templates in sorted(layer_templates.items()):
        for template in templates:
            suffix = template["suffix"]
            pattern = f"{{registryPrefix}}{suffix}"
            layer_template_entries.append(
                {
                    "layerType": layer_type,
                    "idPattern": pattern,
                    "kind": template["kind"],
                    "suffix": suffix.removeprefix("."),
                    "source": template["source"],
                    "sourceClass": template.get("sourceClass", ""),
                }
            )
            console_key = (suffix, template["kind"])
            console_templates.setdefault(
                console_key,
                {
                    "idPattern": f"console.layer{{slot}}{suffix}",
                    "kind": template["kind"],
                    "suffix": suffix.removeprefix("."),
                    "slotRange": [1, 8],
                    "sourceLayerTypes": [],
                },
            )
            console_templates[console_key]["sourceLayerTypes"].append(layer_type)

    asset_entries = layer_assets()
    unresolved_assets: list[dict[str, Any]] = []
    catalog_assets_without_layer_parameters: list[dict[str, Any]] = []
    for asset in asset_entries:
        templates = layer_templates.get(asset["type"], [])
        if not templates:
            if is_catalog_surface_without_layer_params(asset["type"]):
                catalog_assets_without_layer_parameters.append(
                    {
                        "assetId": asset["id"],
                        "type": asset["type"],
                        "reason": "catalog surface uses global/effect/HUD parameters instead of Layer::setup parameters",
                        "source": asset["source"],
                    }
                )
                continue
            unresolved_assets.append(
                {
                    "assetId": asset["id"],
                    "type": asset["type"],
                    "reason": "no static parameter templates found for layer type",
                    "source": asset["source"],
                }
            )
            continue
        for template in templates:
            suffix = template["suffix"]
            param_id = f"{asset['registryPrefix']}{suffix}"
            default_value = default_from_layer(asset["defaults"], suffix)
            entry = {
                "id": param_id,
                "kind": template["kind"],
                "scope": "layer_asset",
                "family": family_for(param_id),
                "units": infer_units(param_id),
                "assetId": asset["id"],
                "layerType": asset["type"],
                "template": f"{{registryPrefix}}{suffix}",
                "source": asset["source"],
            }
            if default_value is not None:
                entry["default"] = default_value
                entry["defaultKind"] = value_kind(default_value)
            add_unique(parameters, entry)

        opacity_id = f"{asset['registryPrefix']}.opacity"
        add_unique(
            parameters,
            {
                "id": opacity_id,
                "kind": "float",
                "scope": "layer_asset",
                "family": family_for(opacity_id),
                "units": "normalized",
                "assetId": asset["id"],
                "layerType": asset["type"],
                "template": "{registryPrefix}.opacity",
                "default": asset.get("opacity", 1.0),
                "defaultKind": "float",
                "source": asset["source"],
            },
        )

    console_template_list = sorted(console_templates.values(), key=lambda item: item["idPattern"])
    for template in console_template_list:
        template["sourceLayerTypes"] = sorted(set(template["sourceLayerTypes"]))
    console_template_list.insert(
        0,
        {
            "idPattern": "console.layer{slot}.opacity",
            "kind": "float",
            "suffix": "opacity",
            "slotRange": [1, 8],
            "sourceLayerTypes": ["all console layers"],
        },
    )

    all_parameters = sorted(parameters.values(), key=lambda entry: entry["id"])
    suffix_counts: dict[str, int] = {}
    for entry in all_parameters:
        suffix = entry["id"].rsplit(".", 1)[-1]
        suffix_counts[suffix] = suffix_counts.get(suffix, 0) + 1
    common_suffixes = [
        {"suffix": suffix, "count": count}
        for suffix, count in sorted(suffix_counts.items(), key=lambda pair: (-pair[1], pair[0]))
        if count >= 3
    ]

    return {
        "schemaVersion": 1,
        "status": "generated",
        "generator": "tools/gen_parameter_manifest.py",
        "sourceStrategy": [
            "Static C++ scan of ParameterRegistry addFloat/addBool/addString registrations.",
            "Layer asset expansion from synaptome/bin/data/layers registryPrefix values.",
            "Console slot templates are patterns because live scene prefixes are slot-indexed.",
        ],
        "sources": [
            "synaptome/src/ofApp.cpp",
            "synaptome/src/visuals/**/*.cpp",
            "synaptome/bin/data/layers/**/*.json",
        ],
        "counts": {
            "parameters": len(all_parameters),
            "layerTemplates": len(layer_template_entries),
            "consoleTemplates": len(console_template_list),
            "catalogAssetsWithoutLayerParameters": len(catalog_assets_without_layer_parameters),
            "unresolvedAssets": len(unresolved_assets),
        },
        "parameters": all_parameters,
        "layerTemplates": layer_template_entries,
        "consoleSlotTemplates": console_template_list,
        "commonSuffixes": common_suffixes,
        "catalogAssetsWithoutLayerParameters": catalog_assets_without_layer_parameters,
        "unresolvedAssets": unresolved_assets,
    }


def validate_manifest(manifest: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    seen: set[str] = set()
    for idx, entry in enumerate(manifest.get("parameters", [])):
        param_id = entry.get("id")
        if not isinstance(param_id, str):
            errors.append(f"parameters[{idx}].id must be a string")
            continue
        if param_id in seen:
            errors.append(f"duplicate parameter id: {param_id}")
        seen.add(param_id)
        if not PARAM_ID_RE.match(param_id):
            errors.append(f"invalid parameter id: {param_id}")
        if entry.get("kind") not in {"float", "bool", "string"}:
            errors.append(f"{param_id}: kind must be float, bool, or string")

    for idx, entry in enumerate(manifest.get("consoleSlotTemplates", [])):
        pattern = entry.get("idPattern")
        if not isinstance(pattern, str) or not CONSOLE_PATTERN_RE.match(pattern):
            errors.append(f"consoleSlotTemplates[{idx}].idPattern is invalid: {pattern}")
    return errors


def dumps_manifest(manifest: dict[str, Any]) -> str:
    return json.dumps(manifest, indent=2, sort_keys=False) + "\n"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Generate/check the Synaptome parameter manifest")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT), help="Manifest path")
    parser.add_argument("--write", action="store_true", help="Write the manifest")
    parser.add_argument("--check", action="store_true", help="Fail if the checked-in manifest is stale")
    args = parser.parse_args(argv)

    manifest = build_manifest()
    errors = validate_manifest(manifest)
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1

    output = Path(args.output)
    rendered = dumps_manifest(manifest)
    if args.write:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(rendered, encoding="utf-8")
        print(f"Wrote {output}")
        return 0

    if args.check:
        if not output.exists():
            print(f"error: missing manifest {output}", file=sys.stderr)
            return 1
        existing = output.read_text(encoding="utf-8")
        if existing != rendered:
            print(f"error: {output} is stale; run python tools\\gen_parameter_manifest.py --write", file=sys.stderr)
            return 1
        print(f"Parameter manifest is current: {output}")
        return 0

    print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

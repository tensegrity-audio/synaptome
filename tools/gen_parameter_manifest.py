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

import layer_package_discovery
import layer_package_parameter_manifest

REPO_ROOT = Path(__file__).resolve().parents[1]
APP_ROOT = REPO_ROOT / "synaptome"
DEFAULT_OUTPUT = REPO_ROOT / "docs" / "contracts" / "parameter_manifest.json"
DEFAULT_COMBINED_OUTPUT = REPO_ROOT / "tools" / "testdata" / "layer_packages" / "expected_combined_parameter_manifest.json"
BUILTIN_ELEMENT_CONTRACTS = (
    REPO_ROOT / "docs" / "contracts" / "builtin_element_parameters.json"
)

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


def parse_factory_types(*sources: Path) -> dict[str, str]:
    text = "\n".join(source.read_text(encoding="utf-8") for source in sources)
    inline_pattern = re.compile(
        r'registerType\(\s*(?:ElementDescriptor\s*)?\{\s*"([^"]+)".*?'
        r'std::make_unique<([^>]+)>',
        re.DOTALL,
    )
    factory_types = {
        match.group(1): match.group(2)
        for match in inline_pattern.finditer(text)
    }
    contract_types = {
        match.group(1): match.group(2)
        for match in re.finditer(
            r'ElementTypeContract\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)'
            r'\s*\{.*?contract\.element\s*=\s*\{\s*"([^"]+)".*?'
            r'return\s+contract\s*;',
            text,
            re.DOTALL,
        )
    }
    for match in re.finditer(
        r'registerType\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\)\s*,'
        r'.*?std::make_unique<([^>]+)>',
        text,
        re.DOTALL,
    ):
        layer_type = contract_types.get(match.group(1))
        if layer_type:
            factory_types[layer_type] = match.group(2)
    return factory_types


def parse_core_parameters(source_path: Path) -> list[dict[str, Any]]:
    text = source_path.read_text(encoding="utf-8")
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
                    "source": source_ref(source_path, line_for(text, match.start())),
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
                "source": source_ref(source_path, line_for(text, match.start())),
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


def declared_layer_templates() -> dict[str, list[dict[str, Any]]]:
    snapshot = load_json(BUILTIN_ELEMENT_CONTRACTS)
    # Source packages use the same declaration contract as built-ins. Only the
    # explicit, validated registration set can contribute compiled types here.
    import generate_element_package_registrations

    type_entries = [(entry, BUILTIN_ELEMENT_CONTRACTS) for entry in snapshot.get("types", [])]
    builtin_ids = {entry.get("typeId") for entry, _ in type_entries}
    for record in generate_element_package_registrations.load_records():
        if record.type_id in builtin_ids:
            continue  # Signal Bloom remains in the historical built-in snapshot.
        package = load_json(record.package_path)
        type_entries.append(({
            "typeId": record.type_id,
            "declarations": {"parameters": package["parameters"]},
        }, record.package_path))
    templates: dict[str, list[dict[str, Any]]] = {}
    for type_entry, declaration_source in type_entries:
        if not isinstance(type_entry, dict):
            continue
        type_id = type_entry.get("typeId")
        declarations = type_entry.get("declarations", {})
        parameters = declarations.get("parameters", []) if isinstance(declarations, dict) else []
        if not isinstance(type_id, str) or not isinstance(parameters, list):
            continue
        type_templates: list[dict[str, Any]] = []
        for parameter in parameters:
            if not isinstance(parameter, dict):
                continue
            parameter_id = parameter.get("id")
            kind = parameter.get("kind")
            if not isinstance(parameter_id, str) or kind not in {"float", "bool", "string"}:
                continue
            type_templates.append(
                {
                    "suffix": f".{parameter_id}",
                    "kind": kind,
                    "source": source_ref(declaration_source, 1),
                    "sourceClass": "ElementTypeContract",
                    "groupId": parameter.get("groupId", ""),
                    "label": parameter.get("label", parameter_id),
                    "default": parameter.get("default"),
                    "units": parameter.get("units", ""),
                    "description": parameter.get("description", ""),
                    "range": parameter.get("range"),
                    "quickAccessOrder": parameter.get("quickAccessOrder"),
                    "optionSource": parameter.get("optionSource"),
                }
            )
        templates[type_id] = type_templates
    return templates


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


def build_manifest(
    include_packages: bool = False,
    package_roots: list[Path] | tuple[Path, ...] | None = None,
) -> dict[str, Any]:
    of_app = APP_ROOT / "src" / "ofApp.cpp"
    builtin_host_bindings = (
        APP_ROOT / "src" / "runtime" / "BuiltinElementHostBindings.cpp"
    )
    builtin_elements = APP_ROOT / "src" / "runtime" / "BuiltinElements.cpp"
    layer_templates = declared_layer_templates()
    _, explicit_parameters = parse_layer_templates({})

    parameters: dict[str, dict[str, Any]] = {}
    for core_source in (of_app, builtin_host_bindings):
        for entry in parse_core_parameters(core_source):
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
                catalog_assets_without_layer_parameters.appen
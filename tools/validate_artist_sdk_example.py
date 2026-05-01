#!/usr/bin/env python3
"""Validate the public artist SDK source/catalog/scene example fixture."""
from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterable

REPO_ROOT = Path(__file__).resolve().parents[1]
EXAMPLE_ROOT = REPO_ROOT / "docs" / "examples" / "artist_sdk"
EXPECTED_PATH = REPO_ROOT / "tools" / "testdata" / "artist_sdk" / "expected_artist_sdk_example.json"

HEADER_PATH = EXAMPLE_ROOT / "SignalBloomLayer.h"
SOURCE_PATH = EXAMPLE_ROOT / "SignalBloomLayer.cpp"
REGISTRATION_PATH = EXAMPLE_ROOT / "register_signal_bloom.cpp"
CATALOG_PATH = EXAMPLE_ROOT / "signal_bloom.layer.json"
SCENE_PATH = EXAMPLE_ROOT / "signal_bloom.scene.json"

EXPECTED_ASSET_ID = "examples.signal_bloom"
EXPECTED_TYPE = "example.signalBloom"
EXPECTED_PREFIX = "examples.signal_bloom"

REQUIRED_TEMPLATES = {
    "global_transport",
    "layer_local_basics",
    "layer_local_color",
    "media_playback_pairing",
    "osc_routes",
    "sensor_modifiers",
    "scene_persistence",
}

REQUIRED_PARAMETERS = {
    "visible": "bool",
    "speed": "float",
    "bpmSync": "bool",
    "bpmMultiplier": "float",
    "scale": "float",
    "rotationDeg": "float",
    "alpha": "float",
    "gain": "float",
    "lineOpacity": "float",
    "colorR": "float",
    "colorG": "float",
    "colorB": "float",
    "bgColorR": "float",
    "bgColorG": "float",
    "bgColorB": "float",
    "xInput": "float",
    "yInput": "float",
    "speedInput": "float",
}

MEDIA_SLOT_SUFFIXES = {"visible", "gain", "mirror", "loop", "clip"}
GLOBAL_TARGETS = {"transport.bpm", "globals.speed", "fx.master"}
DISALLOWED_SOURCE_STRINGS = {
    "ofLoadImage",
    "ofLoadURL",
    "ofVideoGrabber",
    "ofVideoPlayer",
    "ofxAssimpModelLoader",
    "std::ifstream",
}


def rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def dumps(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def extract_parameters(source_text: str) -> dict[str, str]:
    pattern = re.compile(r"registry\.add(Float|Bool|String)\(\s*prefix\s*\+\s*\"\.([^\"]+)\"")
    found: dict[str, str] = {}
    for kind, suffix in pattern.findall(source_text):
        found[suffix] = kind.lower()
    return dict(sorted(found.items()))


def iter_targets(value: Any) -> Iterable[str]:
    if isinstance(value, dict):
        for key, child in value.items():
            if key == "target" and isinstance(child, str):
                yield child
            else:
                yield from iter_targets(child)
    elif isinstance(value, list):
        for child in value:
            yield from iter_targets(child)


def slot_parameter_suffixes(scene: dict[str, Any], slot_index: int) -> list[str]:
    slots = scene.get("console", {}).get("slots", [])
    if not isinstance(slots, list):
        return []
    prefix = f"console.layer{slot_index}."
    suffixes: list[str] = []
    for slot in slots:
        if not isinstance(slot, dict) or slot.get("index") != slot_index:
            continue
        params = slot.get("parameters", {})
        if not isinstance(params, dict):
            continue
        for param_id in params:
            if isinstance(param_id, str) and param_id.startswith(prefix):
                suffixes.append(param_id[len(prefix):])
    return sorted(suffixes)


def scene_slot_asset_ids(scene: dict[str, Any]) -> list[str]:
    slots = scene.get("console", {}).get("slots", [])
    if not isinstance(slots, list):
        return []
    return [
        slot.get("assetId", "")
        for slot in slots
        if isinstance(slot, dict) and isinstance(slot.get("assetId"), str)
    ]


def collect_errors(
    header_text: str,
    source_text: str,
    registration_text: str,
    catalog: dict[str, Any],
    scene: dict[str, Any],
    parameters: dict[str, str],
) -> list[str]:
    errors: list[str] = []

    if "class SignalBloomLayer : public Layer" not in header_text:
        errors.append("SignalBloomLayer.h must declare SignalBloomLayer as a Layer subclass")

    for method in ("configure", "setup", "update", "draw"):
        if f"void SignalBloomLayer::{method}" not in source_text:
            errors.append(f"SignalBloomLayer.cpp must implement {method}()")

    for banned in sorted(DISALLOWED_SOURCE_STRINGS):
        if banned in source_text:
            errors.append(f"SignalBloomLayer.cpp uses heavy setup/resource call '{banned}'")

    for suffix, expected_type in REQUIRED_PARAMETERS.items():
        actual_type = parameters.get(suffix)
        if actual_type != expected_type:
            errors.append(f"missing {expected_type} parameter suffix '{suffix}'")

    if catalog.get("id") != EXPECTED_ASSET_ID:
        errors.append(f"catalog id must be {EXPECTED_ASSET_ID}")
    if catalog.get("type") != EXPECTED_TYPE:
        errors.append(f"catalog type must be {EXPECTED_TYPE}")
    if catalog.get("registryPrefix") != EXPECTED_PREFIX:
        errors.append(f"catalog registryPrefix must be {EXPECTED_PREFIX}")

    sdk = catalog.get("sdk", {})
    if not isinstance(sdk, dict):
        errors.append("catalog sdk metadata must be an object")
    else:
        templates = sdk.get("templates", [])
        if not isinstance(templates, list):
            errors.append("catalog sdk.templates must be an array")
        else:
            missing_templates = sorted(REQUIRED_TEMPLATES - {item for item in templates if isinstance(item, str)})
            if missing_templates:
                errors.append(f"catalog sdk.templates missing: {', '.join(missing_templates)}")
        if sdk.get("factoryRegistration") != rel(REGISTRATION_PATH):
            errors.append("catalog sdk.factoryRegistration must point to register_signal_bloom.cpp")
        if sdk.get("noHeavySetupSideEffects") is not True:
            errors.append("catalog sdk.noHeavySetupSideEffects must be true")

    if f'registerType("{EXPECTED_TYPE}"' not in registration_text:
        errors.append("registration snippet must register the catalog type")
    if "std::make_unique<SignalBloomLayer>" not in registration_text:
        errors.append("registration snippet must create SignalBloomLayer")

    slot_assets = scene_slot_asset_ids(scene)
    if EXPECTED_ASSET_ID not in slot_assets:
        errors.append("scene must load Signal Bloom into a Console slot")
    if "media.clip.default" not in slot_assets:
        errors.append("scene must pair the SDK layer with the public media clip layer")

    scene_schema = scene.get("scene", {}).get("schemaVersion")
    if scene_schema != 2:
        errors.append("scene.schemaVersion must be 2")

    signal_suffixes = set(slot_parameter_suffixes(scene, 1))
    missing_scene_suffixes = sorted(set(REQUIRED_PARAMETERS) - signal_suffixes)
    if missing_scene_suffixes:
        errors.append(f"scene slot 1 missing SDK parameter values: {', '.join(missing_scene_suffixes)}")

    media_suffixes = set(slot_parameter_suffixes(scene, 2))
    missing_media_suffixes = sorted(MEDIA_SLOT_SUFFIXES - media_suffixes)
    if missing_media_suffixes:
        errors.append(f"scene slot 2 missing media parameter values: {', '.join(missing_media_suffixes)}")

    targets = set(iter_targets(scene))
    missing_global_targets = sorted(GLOBAL_TARGETS - targets)
    if missing_global_targets:
        errors.append(f"scene banks missing global targets: {', '.join(missing_global_targets)}")

    required_route_targets = {
        "console.layer1.speed",
        "console.layer1.colorR",
        "console.layer1.xInput",
        "console.layer1.yInput",
        "console.layer1.speedInput",
        "console.layer2.gain",
    }
    missing_route_targets = sorted(required_route_targets - targets)
    if missing_route_targets:
        errors.append(f"scene mappings/banks missing route targets: {', '.join(missing_route_targets)}")

    for target in sorted(targets):
        if target.startswith("console.layer1."):
            suffix = target.removeprefix("console.layer1.")
            if suffix not in REQUIRED_PARAMETERS:
                errors.append(f"scene target {target} is not declared by the SDK layer")
        elif target.startswith("console.layer2."):
            suffix = target.removeprefix("console.layer2.")
            if suffix not in MEDIA_SLOT_SUFFIXES:
                errors.append(f"scene target {target} is not a public media fixture suffix")
        elif target not in GLOBAL_TARGETS:
            errors.append(f"scene target {target} is not part of this SDK fixture")

    return errors


def build_contract() -> tuple[dict[str, Any], list[str]]:
    missing_files = [
        path
        for path in (HEADER_PATH, SOURCE_PATH, REGISTRATION_PATH, CATALOG_PATH, SCENE_PATH)
        if not path.exists()
    ]
    if missing_files:
        missing = ", ".join(rel(path) for path in missing_files)
        return {}, [f"missing required fixture files: {missing}"]

    header_text = HEADER_PATH.read_text(encoding="utf-8")
    source_text = SOURCE_PATH.read_text(encoding="utf-8")
    registration_text = REGISTRATION_PATH.read_text(encoding="utf-8")
    catalog = load_json(CATALOG_PATH)
    scene = load_json(SCENE_PATH)
    if not isinstance(catalog, dict):
        return {}, ["catalog fixture must be a JSON object"]
    if not isinstance(scene, dict):
        return {}, ["scene fixture must be a JSON object"]

    parameters = extract_parameters(source_text)
    targets = sorted(set(iter_targets(scene)))
    signal_slot_suffixes = slot_parameter_suffixes(scene, 1)
    media_slot_suffixes = slot_parameter_suffixes(scene, 2)
    sdk = catalog.get("sdk", {})
    templates = sorted(sdk.get("templates", [])) if isinstance(sdk, dict) and isinstance(sdk.get("templates"), list) else []

    summary: dict[str, Any] = {
        "schemaVersion": 1,
        "sourceStrategy": "Static public SDK fixture for a source-registered Layer, Browser catalog metadata, and saved scene usage.",
        "sources": [
            rel(HEADER_PATH),
            rel(SOURCE_PATH),
            rel(REGISTRATION_PATH),
            rel(CATALOG_PATH),
            rel(SCENE_PATH),
        ],
        "asset": {
            "id": catalog.get("id", ""),
            "label": catalog.get("label", ""),
            "category": catalog.get("category", ""),
            "type": catalog.get("type", ""),
            "registryPrefix": catalog.get("registryPrefix", ""),
        },
        "sdk": {
            "level": sdk.get("level", "") if isinstance(sdk, dict) else "",
            "factoryRegistration": sdk.get("factoryRegistration", "") if isinstance(sdk, dict) else "",
            "noHeavySetupSideEffects": sdk.get("noHeavySetupSideEffects", False) if isinstance(sdk, dict) else False,
            "templates": templates,
        },
        "parameters": [
            {"suffix": suffix, "type": kind}
            for suffix, kind in sorted(parameters.items())
        ],
        "scene": {
            "schemaVersion": scene.get("scene", {}).get("schemaVersion"),
            "slotAssetIds": scene_slot_asset_ids(scene),
            "signalBloomSlotParameterSuffixes": signal_slot_suffixes,
            "mediaSlotParameterSuffixes": media_slot_suffixes,
            "targets": targets,
        },
    }

    errors = collect_errors(header_text, source_text, registration_text, catalog, scene, parameters)
    return summary, errors


def check(expected_path: Path) -> int:
    actual, errors = build_contract()
    if errors:
        for error in errors:
            print(f"artist SDK example error: {error}", file=sys.stderr)
        return 1

    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    actual_text = dumps(actual)
    if actual_text != expected_text:
        print(f"Artist SDK example snapshot is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated artist SDK example",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1

    print(
        "Artist SDK example validation passed "
        f"({len(actual['parameters'])} parameters, {len(actual['scene']['targets'])} scene targets)"
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected SDK example snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--expected", type=Path, default=EXPECTED_PATH, help="Expected snapshot path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        actual, errors = build_contract()
        if errors:
            for error in errors:
                print(f"artist SDK example error: {error}", file=sys.stderr)
            return 1
        expected_path.parent.mkdir(parents=True, exist_ok=True)
        expected_path.write_text(dumps(actual), encoding="utf-8")
        print(f"Wrote artist SDK example snapshot: {rel(expected_path)}")
        return 0
    return check(expected_path)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

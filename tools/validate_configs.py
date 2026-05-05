#!/usr/bin/env python3
"""Validate project configuration files.

Extensible schema checks to catch malformed JSON before runtime.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Dict, List

BasePath = Path(__file__).resolve().parents[1]

ValidationFn = Callable[[object, Path], List[str]]

MAC_REGEX = re.compile(r"^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$")
HEX_ID_REGEX = re.compile(r"^0x[0-9A-Fa-f]{2,4}$")


def _load_json(path: Path) -> object:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except json.JSONDecodeError as exc:  # pragma: no cover - CLI path
        raise ValueError(f"JSON parse error in {path}: {exc}") from exc


def _require_key(obj: dict, key: str, context: str) -> List[str]:
    if key not in obj:
        return [f"Missing '{key}' in {context}"]
    return []


def _ensure_range(value: int, lo: int, hi: int, context: str) -> List[str]:
    if not (lo <= value <= hi):
        return [f"{context} must be between {lo} and {hi}"]
    return []


def _validate_browser_metadata(metadata: object, ctx: str) -> List[str]:
    errors: List[str] = []
    if metadata is None:
        return errors
    if not isinstance(metadata, dict):
        errors.append(f"{ctx} must be an object")
        return errors
    if "preferredInput" in metadata:
        preferred = metadata["preferredInput"]
        if not isinstance(preferred, str):
            errors.append(f"{ctx}.preferredInput must be a string if provided")
        elif preferred not in {"key", "midi", "osc", "any"}:
            errors.append(f"{ctx}.preferredInput must be one of ['key', 'midi', 'osc', 'any']")
    for text_field in ("category", "subcategory", "section", "captureHint", "notes"):
        if text_field in metadata and not isinstance(metadata[text_field], str):
            errors.append(f"{ctx}.{text_field} must be a string")
    if "tags" in metadata:
        tags = metadata["tags"]
        if not isinstance(tags, list) or not all(isinstance(tag, str) for tag in tags):
            errors.append(f"{ctx}.tags must be an array of strings")
    if "transform" in metadata:
        transform = metadata["transform"]
        if not isinstance(transform, dict):
            errors.append(f"{ctx}.transform must be an object")
        else:
            if "curve" in transform and not isinstance(transform["curve"], str):
                errors.append(f"{ctx}.transform.curve must be a string")
            for num_field in ("min", "max"):
                if num_field in transform and not isinstance(transform[num_field], (int, float)):
                    errors.append(f"{ctx}.transform.{num_field} must be numeric")
            if "units" in transform and not isinstance(transform["units"], str):
                errors.append(f"{ctx}.transform.units must be a string")
    return errors



def _validate_bank_controls(entries: object, ctx: str) -> List[str]:
    errors: List[str] = []
    if entries is None:
        return errors
    if not isinstance(entries, list):
        errors.append(f"{ctx} must be an array")
        return errors
    seen_ids = set()
    for idx, entry in enumerate(entries):
        entry_ctx = f"{ctx}[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        errors.extend(_require_key(entry, "id", entry_ctx))
        bank_id = entry.get("id")
        if bank_id is not None and not isinstance(bank_id, str):
            errors.append(f"{entry_ctx}.id must be a string")
        if isinstance(bank_id, str):
            if bank_id in seen_ids:
                errors.append(f"{entry_ctx}.id '{bank_id}' is duplicated")
            seen_ids.add(bank_id)
        label = entry.get("label")
        if label is not None and not isinstance(label, str):
            errors.append(f"{entry_ctx}.label must be a string")
        parent = entry.get("parent")
        if parent is not None and not isinstance(parent, str):
            errors.append(f"{entry_ctx}.parent must be a string")
        controls = entry.get("controls", [])
        if controls is None:
            continue
        if not isinstance(controls, list):
            errors.append(f"{entry_ctx}.controls must be an array")
            continue
        control_ids = set()
        for c_idx, ctrl in enumerate(controls):
            ctrl_ctx = f"{entry_ctx}.controls[{c_idx}]"
            if not isinstance(ctrl, dict):
                errors.append(f"{ctrl_ctx} must be an object")
                continue
            errors.extend(_require_key(ctrl, "id", ctrl_ctx))
            ctrl_id = ctrl.get("id")
            if ctrl_id is not None and not isinstance(ctrl_id, str):
                errors.append(f"{ctrl_ctx}.id must be a string")
            if isinstance(ctrl_id, str):
                if ctrl_id in control_ids:
                    errors.append(f"{ctrl_ctx}.id '{ctrl_id}' is duplicated within the bank")
                control_ids.add(ctrl_id)
            for key in ("label", "target", "modifier", "description"):
                value = ctrl.get(key)
                if value is not None and not isinstance(value, str):
                    errors.append(f"{ctrl_ctx}.{key} must be a string")
            if "softTakeover" in ctrl and not isinstance(ctrl["softTakeover"], bool):
                errors.append(f"{ctrl_ctx}.softTakeover must be boolean")
    return errors


def validate_scene(data: object, path: Path) -> List[str]:
    ctx = f"{path.name}"
    if not isinstance(data, dict):
        return [f"{ctx} must be a JSON object"]

    errors: List[str] = []
    banks = data.get("banks")
    if banks is not None:
        if not isinstance(banks, dict):
            errors.append("'banks' must be an object")
        else:
            if "global" in banks:
                errors.extend(_validate_bank_controls(banks.get("global"), f"{ctx}.banks.global"))
            if "scene" in banks:
                errors.extend(_validate_bank_controls(banks.get("scene"), f"{ctx}.banks.scene"))
            if "layers" in banks:
                layers = banks.get("layers")
                if not isinstance(layers, dict):
                    errors.append("banks.layers must be an object mapping layer ids to arrays")
                else:
                    for layer_id, entries in layers.items():
                        if not isinstance(entries, list):
                            errors.append(f"{ctx}.banks.layers[{layer_id}] must be an array")
                            continue
                        errors.extend(_validate_bank_controls(entries, f"{ctx}.banks.layers[{layer_id}]"))
    return errors

def validate_hotkeys(data: object, path: Path) -> List[str]:
    ctx = f"{path.name}"
    if not isinstance(data, dict):
        return [f"{ctx} must be a JSON object"]

    errors: List[str] = []
    errors.extend(_require_key(data, "version", ctx))
    if "version" in data and not isinstance(data["version"], int):
        errors.append("'version' must be an integer")

    bindings = data.get("bindings")
    if not isinstance(bindings, dict):
        errors.append("'bindings' must be an object")
        return errors

    for binding_id, entry in bindings.items():
        entry_ctx = f"bindings.{binding_id}"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        if "key" not in entry or not isinstance(entry.get("key"), int):
            errors.append(f"{entry_ctx}.key must be an integer")
        for field in ("label", "scope", "displayName"):
            if field in entry and not isinstance(entry[field], str):
                errors.append(f"{entry_ctx}.{field} must be a string")
        if "scope" in entry and isinstance(entry["scope"], str):
            normalized = entry["scope"].lower()
            if normalized not in {"", "global", "scene", "local"}:
                errors.append(f"{entry_ctx}.scope '{entry['scope']}' is not recognized")
        if "learnable" in entry and not isinstance(entry["learnable"], bool):
            errors.append(f"{entry_ctx}.learnable must be boolean")
        if "browser" in entry:
            errors.extend(_validate_browser_metadata(entry["browser"], f"{entry_ctx}.browser"))
        if "controlHub" in entry:
            errors.extend(_validate_browser_metadata(entry["controlHub"], f"{entry_ctx}.controlHub"))

    return errors


def validate_midi_map(data: object, path: Path) -> List[str]:
    errors: List[str] = []
    ctx = f"{path.name}"
    if not isinstance(data, dict):
        return [f"{ctx} must be a JSON object"]

    if "device" in data and not isinstance(data["device"], str):
        errors.append("'device' must be a string")
    if "deviceIndex" in data and not isinstance(data["deviceIndex"], int):
        errors.append("'deviceIndex' must be an integer")

    cc_entries = data.get("cc", [])
    if not isinstance(cc_entries, list):
        errors.append("'cc' must be an array")
    for idx, entry in enumerate(cc_entries):
        entry_ctx = f"cc[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        errors.extend(_require_key(entry, "target", entry_ctx))
        if "target" in entry and not isinstance(entry["target"], str):
            errors.append(f"{entry_ctx}.target must be a string")
        if "num" in entry:
            if not isinstance(entry["num"], int):
                errors.append(f"{entry_ctx}.num must be an integer")
            else:
                errors.extend(_ensure_range(entry["num"], 0, 127, f"{entry_ctx}.num"))
        if "out" in entry:
            vec = entry["out"]
            if not (isinstance(vec, list) and len(vec) == 2 and all(isinstance(v, (int, float)) for v in vec)):
                errors.append(f"{entry_ctx}.out must be a [min, max] numeric pair")
        if "snapInt" in entry and not isinstance(entry["snapInt"], bool):
            errors.append(f"{entry_ctx}.snapInt must be boolean")
        if "step" in entry and not isinstance(entry["step"], (int, float)):
            errors.append(f"{entry_ctx}.step must be numeric")
        if "browser" in entry:
            errors.extend(_validate_browser_metadata(entry["browser"], f"{entry_ctx}.browser"))
        if "controlHub" in entry:
            errors.extend(_validate_browser_metadata(entry["controlHub"], f"{entry_ctx}.controlHub"))

    btn_entries = data.get("buttons", [])
    if not isinstance(btn_entries, list):
        errors.append("'buttons' must be an array if present")
    for idx, entry in enumerate(btn_entries):
        entry_ctx = f"buttons[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        errors.extend(_require_key(entry, "target", entry_ctx))
        errors.extend(_require_key(entry, "num", entry_ctx))
        if "target" in entry and not isinstance(entry["target"], str):
            errors.append(f"{entry_ctx}.target must be a string")
        if "num" in entry and not isinstance(entry["num"], int):
            errors.append(f"{entry_ctx}.num must be an integer")
        if "type" in entry and entry["type"] not in {"toggle", "set"}:
            errors.append(f"{entry_ctx}.type must be one of ['toggle', 'set'] if provided")
        if "setValue" in entry and not isinstance(entry["setValue"], (int, float)):
            errors.append(f"{entry_ctx}.setValue must be numeric")
        if "browser" in entry:
            errors.extend(_validate_browser_metadata(entry["browser"], f"{entry_ctx}.browser"))
        if "controlHub" in entry:
            errors.extend(_validate_browser_metadata(entry["controlHub"], f"{entry_ctx}.controlHub"))

    osc_entries = data.get("osc", [])
    if not isinstance(osc_entries, list):
        errors.append("'osc' must be an array if present")
    for idx, entry in enumerate(osc_entries):
        entry_ctx = f"osc[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        errors.extend(_require_key(entry, "pattern", entry_ctx))
        errors.extend(_require_key(entry, "target", entry_ctx))
        if "pattern" in entry and not isinstance(entry["pattern"], str):
            errors.append(f"{entry_ctx}.pattern must be a string")
        if "target" in entry and not isinstance(entry["target"], str):
            errors.append(f"{entry_ctx}.target must be a string")
        for field in ("in", "out"):
            if field in entry:
                vec = entry[field]
                if not (isinstance(vec, list) and len(vec) == 2 and all(isinstance(v, (int, float)) for v in vec)):
                    errors.append(f"{entry_ctx}.{field} must be a [min, max] numeric pair")
        if "smooth" in entry and not isinstance(entry["smooth"], (int, float)):
            errors.append(f"{entry_ctx}.smooth must be numeric")
        if "deadband" in entry and not isinstance(entry["deadband"], (int, float)):
            errors.append(f"{entry_ctx}.deadband must be numeric")
        if "rateLimitHz" in entry and not isinstance(entry["rateLimitHz"], (int, float)):
            errors.append(f"{entry_ctx}.rateLimitHz must be numeric")
        if "browser" in entry:
            errors.extend(_validate_browser_metadata(entry["browser"], f"{entry_ctx}.browser"))
        if "controlHub" in entry:
            errors.extend(_validate_browser_metadata(entry["controlHub"], f"{entry_ctx}.controlHub"))

    return errors


def validate_netmap(data: object, path: Path) -> List[str]:
    ctx = f"{path.name}"
    if not isinstance(data, dict):
        return [f"{ctx} must be a JSON object"]

    errors: List[str] = []
    if "channel" not in data or not isinstance(data["channel"], int):
        errors.append("'channel' must be an integer")
    else:
        errors.extend(_ensure_range(data["channel"], 1, 14, "channel"))

    if "pmk" in data:
        if not (isinstance(data["pmk"], str) and len(data["pmk"]) == 32 and all(c in "0123456789abcdefABCDEF" for c in data["pmk"])):
            errors.append("'pmk' must be a 32-character hex string")

    devices = data.get("devices")
    if not isinstance(devices, list) or not devices:
        errors.append("'devices' must be a non-empty array")
        return errors

    seen_names = set()
    for idx, entry in enumerate(devices):
        entry_ctx = f"devices[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        for key in ("name", "role", "deviceId", "mac"):
            errors.extend(_require_key(entry, key, entry_ctx))
        name = entry.get("name")
        if isinstance(name, str):
            if name in seen_names:
                errors.append(f"Duplicate device name '{name}'")
            seen_names.add(name)
        if "deviceId" in entry and (not isinstance(entry["deviceId"], str) or not HEX_ID_REGEX.match(entry["deviceId"])):
            errors.append(f"{entry_ctx}.deviceId must be a hex string like 0x0101")
        if "mac" in entry and (not isinstance(entry["mac"], str) or not MAC_REGEX.match(entry["mac"])):
            errors.append(f"{entry_ctx}.mac must be in AA:BB:CC:DD:EE:FF format")
        if "enabled" in entry and not isinstance(entry["enabled"], bool):
            errors.append(f"{entry_ctx}.enabled must be boolean")

    return errors


def validate_osc_map(data: object, path: Path) -> List[str]:
    ctx = f"{path.name}"
    if not isinstance(data, dict):
        return [f"{ctx} must be a JSON object"]

    errors: List[str] = []
    defaults = data.get("defaults", {})
    if not isinstance(defaults, dict):
        errors.append("'defaults' must be an object if present")
    else:
        for key in ("rateLimitHz", "smooth", "deadband"):
            if key in defaults and not isinstance(defaults[key], (int, float)):
                errors.append(f"defaults.{key} must be numeric")

    routes = data.get("routes", [])
    if not isinstance(routes, list) or not routes:
        errors.append("'routes' must be a non-empty array")
        return errors

    for idx, entry in enumerate(routes):
        entry_ctx = f"routes[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        for key in ("pattern", "target"):
            errors.extend(_require_key(entry, key, entry_ctx))
            if key in entry and not isinstance(entry[key], str):
                errors.append(f"{entry_ctx}.{key} must be a string")
        for key in ("rateLimitHz", "smooth", "deadband"):
            if key in entry and not isinstance(entry[key], (int, float)):
                errors.append(f"{entry_ctx}.{key} must be numeric")
        if "browser" in entry:
            errors.extend(_validate_browser_metadata(entry["browser"], f"{entry_ctx}.browser"))
        if "controlHub" in entry:
            errors.extend(_validate_browser_metadata(entry["controlHub"], f"{entry_ctx}.controlHub"))

    return errors


def validate_videos(data: object, path: Path) -> List[str]:
    ctx = f"{path.name}"
    if not isinstance(data, dict):
        return [f"{ctx} must be a JSON object"]

    errors: List[str] = []
    clips = data.get("clips", [])
    if not isinstance(clips, list) or not clips:
        errors.append("'clips' must be a non-empty array")
    else:
        seen_ids = set()
        for idx, entry in enumerate(clips):
            entry_ctx = f"clips[{idx}]"
            if not isinstance(entry, dict):
                errors.append(f"{entry_ctx} must be an object")
                continue
            for key in ("id", "path"):
                errors.extend(_require_key(entry, key, entry_ctx))
            clip_id = entry.get("id")
            if isinstance(clip_id, str):
                if clip_id in seen_ids:
                    errors.append(f"Duplicate clip id '{clip_id}'")
                seen_ids.add(clip_id)
            if "prewarm" in entry and not isinstance(entry["prewarm"], bool):
                errors.append(f"{entry_ctx}.prewarm must be boolean")
            if "loop" in entry and not isinstance(entry["loop"], bool):
                errors.append(f"{entry_ctx}.loop must be boolean")

    layers = data.get("layers", [])
    if not isinstance(layers, list):
        errors.append("'layers' must be an array if present")
    else:
        for idx, entry in enumerate(layers):
            entry_ctx = f"layers[{idx}]"
            if not isinstance(entry, dict):
                errors.append(f"{entry_ctx} must be an object")
                continue
            for key in ("id", "defaultClip"):
                errors.extend(_require_key(entry, key, entry_ctx))
                if key in entry and not isinstance(entry[key], str):
                    errors.append(f"{entry_ctx}.{key} must be a string")
            if "opacity" in entry and not isinstance(entry["opacity"], (int, float)):
                errors.append(f"{entry_ctx}.opacity must be numeric")
            if "blendMode" in entry and not isinstance(entry["blendMode"], str):
                errors.append(f"{entry_ctx}.blendMode must be a string")

    return errors


def validate_profiles(data: object, path: Path) -> List[str]:
    ctx = f"{path.name}"
    if not isinstance(data, dict):
        return [f"{ctx} must be a JSON object"]

    errors: List[str] = []
    profiles = data.get("profiles", [])
    if not isinstance(profiles, list) or not profiles:
        errors.append("'profiles' must be a non-empty array")
        return errors

    valid_modes = {"auto", "cyberdeck-direct", "host-only", "sync", "independent"}
    seen_ids = set()
    for idx, entry in enumerate(profiles):
        entry_ctx = f"profiles[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        for key in ("id", "name", "mode", "settings"):
            errors.extend(_require_key(entry, key, entry_ctx))
        profile_id = entry.get("id")
        if isinstance(profile_id, str):
            if profile_id in seen_ids:
                errors.append(f"Duplicate profile id '{profile_id}'")
            seen_ids.add(profile_id)
        if "mode" in entry and entry["mode"] not in valid_modes:
            errors.append(f"{entry_ctx}.mode must be one of {sorted(valid_modes)}")
        settings = entry.get("settings")
        if settings is not None and not isinstance(settings, dict):
            errors.append(f"{entry_ctx}.settings must be an object")
        else:
            for lane in ("matrixportal", "host", "cyberdeck"):
                if lane in settings:
                    if not isinstance(settings[lane], dict):
                        errors.append(f"{entry_ctx}.settings.{lane} must be an object")

    return errors


CONFIG_VALIDATORS: Dict[Path, ValidationFn] = {
    Path("synaptome/bin/data/config/midi-map.json"): validate_midi_map,
    Path("synaptome/bin/data/config/hotkeys.json"): validate_hotkeys,
    Path("synaptome/bin/data/config/netmap.json"): validate_netmap,
    Path("synaptome/bin/data/config/osc-map.json"): validate_osc_map,
    Path("synaptome/bin/data/config/videos.json"): validate_videos,
    Path("synaptome/bin/data/config/profiles.json"): validate_profiles,
    Path("synaptome/bin/data/config/scene-last.json"): validate_scene,
    Path("synaptome/bin/data/layers/scenes/default.json"): validate_scene,
}

BROWSER_CONFIG_TARGETS = [
    Path("synaptome/bin/data/config/midi-map.json"),
    Path("synaptome/bin/data/config/osc-map.json"),
    Path("synaptome/bin/data/config/hotkeys.json"),
]

BROWSER_EXAMPLE_SCHEMAS = {
    Path("docs/examples/parameter_example.json"): Path("docs/schemas/parameter.schema.json"),
    Path("docs/examples/midi_bank_example.json"): Path("docs/schemas/midi_bank.schema.json"),
    Path("docs/examples/osc_map_example.json"): Path("docs/schemas/osc_map.schema.json"),
    Path("docs/examples/hotkeys_example.json"): Path("docs/schemas/hotkeys.schema.json"),
}


@dataclass(frozen=True)
class ContractEntry:
    name: str
    status: str
    sources: tuple[Path, ...]
    validator_command: str
    fixtures: tuple[Path, ...] = ()
    notes: str = ""
    check_command: tuple[str, ...] = ()
    public_app: bool = False


CONTRACT_INDEX_DOC = Path("docs/contracts/README.md")

CONTRACT_ENTRIES: tuple[ContractEntry, ...] = (
    ContractEntry(
        name="App OSC map",
        status="validated",
        sources=(Path("synaptome/bin/data/config/osc-map.json"),),
        validator_command="python tools\\validate_configs.py synaptome/bin/data/config/osc-map.json",
        fixtures=(Path("docs/examples/osc_map_example.json"),),
        public_app=True,
    ),
    ContractEntry(
        name="Parameter ID manifest",
        status="validated",
        sources=(
            Path("synaptome/src/core/ParameterRegistry.h"),
            Path("synaptome/bin/data/layers"),
            Path("docs/contracts/parameter_manifest.json"),
            Path("docs/contracts/parameter_vocabulary.md"),
            Path("docs/schemas/parameter_manifest.schema.json"),
            Path("tools/gen_parameter_manifest.py"),
        ),
        validator_command="python tools\\gen_parameter_manifest.py --check",
        fixtures=(Path("docs/contracts/parameter_manifest.json"), Path("docs/examples/parameter_example.json")),
        notes="Static manifest covers core, sensor, effect, layer asset, and console slot parameter ID patterns; strict target validation is tracked by Parameter target references.",
        check_command=(sys.executable, "tools/gen_parameter_manifest.py", "--check"),
        public_app=True,
    ),
    ContractEntry(
        name="Parameter target references",
        status="validated",
        sources=(
            Path("docs/contracts/parameter_manifest.json"),
            Path("tools/validate_parameter_targets.py"),
            Path("tools/testdata/runtime_state"),
            Path("synaptome/bin/data/device_maps"),
        ),
        validator_command="python tools\\validate_parameter_targets.py --strict --contract-fixtures",
        fixtures=(
            Path("tools/testdata/runtime_state/config"),
            Path("tools/testdata/runtime_state/layers/scenes"),
            Path("tools/testdata/device_maps/synthetic_controller.json"),
        ),
        notes="Strict semantic validator checks committed runtime-state fixtures for scene, MIDI, OSC, audio, device-map, and HUD target references against the generated manifest and layer catalog IDs. Live app-written state is checked separately when needed.",
        check_command=(sys.executable, "tools/validate_parameter_targets.py", "--strict", "--contract-fixtures"),
        public_app=True,
    ),
    ContractEntry(
        name="Layer asset catalog",
        status="validated",
        sources=(
            Path("synaptome/bin/data/layers"),
            Path("synaptome/src/visuals/LayerLibrary.cpp"),
            Path("synaptome/src/ofApp.cpp"),
            Path("tools/layer_catalog_regression.py"),
        ),
        validator_command="python tools\\layer_catalog_regression.py --check",
        fixtures=(Path("tools/testdata/layer_catalog/expected_catalog.json"),),
        notes="Golden static catalog snapshot mirrors LayerLibrary JSON ingestion and checks runtime layer types against factory registrations.",
        check_command=(sys.executable, "tools/layer_catalog_regression.py", "--check"),
        public_app=True,
    ),
    ContractEntry(
        name="Artist SDK example fixture",
        status="validated",
        sources=(
            Path("docs/examples/artist_sdk/SignalBloomLayer.h"),
            Path("docs/examples/artist_sdk/SignalBloomLayer.cpp"),
            Path("docs/examples/artist_sdk/register_signal_bloom.cpp"),
            Path("docs/examples/artist_sdk/signal_bloom.layer.json"),
            Path("docs/examples/artist_sdk/signal_bloom.scene.json"),
            Path("tools/validate_artist_sdk_example.py"),
            Path("tools/testdata/artist_sdk/expected_artist_sdk_example.json"),
        ),
        validator_command="python tools\\validate_artist_sdk_example.py --check",
        fixtures=(
            Path("docs/examples/artist_sdk"),
            Path("tools/testdata/artist_sdk/expected_artist_sdk_example.json"),
        ),
        notes="Static public SDK fixture proves a source-registered Layer, Browser catalog entry, Console scene slot, media pairing, reusable parameter suffixes, and MIDI/OSC/sensor route targets. Source registration remains explicit until the extension/package seam is chosen.",
        check_command=(sys.executable, "tools/validate_artist_sdk_example.py", "--check"),
        public_app=True,
    ),
    ContractEntry(
        name="Scene persistence schema",
        status="validated",
        sources=(
            Path("tools/testdata/runtime_state/layers/scenes"),
            Path("tools/testdata/runtime_state/config/scene-last.json"),
            Path("synaptome/bin/data/layers"),
            Path("synaptome/src/ofApp.cpp"),
            Path("tools/validate_scene_persistence_contract.py"),
        ),
        validator_command="python tools\\validate_scene_persistence_contract.py --check",
        fixtures=(
            Path("tools/testdata/runtime_state/layers/scenes"),
            Path("tools/testdata/runtime_state/config/scene-last.json"),
            Path("tools/testdata/scene_persistence/expected_scene_contract.json"),
        ),
        notes="Static fixture covers committed scene JSON shape and catalog references without depending on live app-written scene state; app-native staged apply/rollback fixture coverage remains a future public gate.",
        check_command=(sys.executable, "tools/validate_scene_persistence_contract.py", "--check"),
        public_app=True,
    ),
    ContractEntry(
        name="Device-map schema",
        status="validated",
        sources=(
            Path("synaptome/bin/data/device_maps"),
            Path("docs/schemas/device_map.schema.json"),
            Path("tools/device_map_regression.py"),
            Path("tools/testdata/device_maps/expected_logical_slots.json"),
            Path("tools/testdata/device_maps/synthetic_controller.json"),
        ),
        validator_command="python tools\\device_map_regression.py --check",
        fixtures=(
            Path("synaptome/bin/data/device_maps/MIDI Mix 0.json"),
            Path("tools/testdata/device_maps/synthetic_controller.json"),
            Path("tools/testdata/device_maps/expected_logical_slots.json"),
        ),
        notes="Strict logical slot regression covers MIDI Mix plus a synthetic controller, role families, sensitivity range, physical bindings, and duplicate-binding detection.",
        check_command=(sys.executable, "tools/device_map_regression.py", "--check"),
        public_app=True,
    ),
    ContractEntry(
        name="MIDI mapping persistence",
        status="validated",
        sources=(Path("synaptome/bin/data/config/midi-map.json"),),
        validator_command="python tools\\validate_configs.py synaptome/bin/data/config/midi-map.json",
        fixtures=(Path("docs/examples/midi_bank_example.json"),),
        public_app=True,
    ),
    ContractEntry(
        name="HUD layout/feed persistence",
        status="validated",
        sources=(
            Path("synaptome/src/ui/HudRegistry.cpp"),
            Path("synaptome/src/ui/HudFeedRegistry.cpp"),
            Path("synaptome/src/ui/ControlMappingHubState.h"),
            Path("synaptome/src/ui/overlays/OverlayWidget.h"),
            Path("synaptome/bin/data/layers/hud"),
            Path("tools/testdata/runtime_state/config/control_hub_prefs.json"),
            Path("tools/testdata/runtime_state/config/console.json"),
            Path("tools/validate_hud_layout_contract.py"),
            Path("tools/testdata/hud_layout/expected_widgets.json"),
        ),
        validator_command="python tools\\validate_hud_layout_contract.py --check",
        fixtures=(
            Path("synaptome/bin/data/layers/hud"),
            Path("tools/testdata/runtime_state/config/control_hub_prefs.json"),
            Path("tools/testdata/runtime_state/config/console.json"),
            Path("tools/testdata/hud_layout/expected_widgets.json"),
        ),
        notes="Static HUD contract covers widget asset identity, declared feed IDs, fixture Browser HUD preferences, and fixture Console overlay layout snapshots; dynamic feed payloads, live app-written state, and timestamps remain runtime-local.",
        check_command=(sys.executable, "tools/validate_hud_layout_contract.py", "--check"),
        public_app=True,
    ),
    ContractEntry(
        name="Console layout/secondary display persistence",
        status="validated",
        sources=(
            Path("synaptome/src/io/ConsoleStore.h"),
            Path("tools/testdata/runtime_state/config/console.json"),
            Path("tools/testdata/runtime_state/config/ui/slot_assignments.json"),
            Path("tools/validate_console_layout_contract.py"),
            Path("tools/testdata/console_layout/expected_console_contract.json"),
        ),
        validator_command="python tools\\validate_console_layout_contract.py --check",
        fixtures=(
            Path("tools/testdata/runtime_state/config/console.json"),
            Path("tools/testdata/runtime_state/config/ui/slot_assignments.json"),
            Path("tools/testdata/console_layout/expected_console_contract.json"),
        ),
        notes="Static Console/display contract covers fixture eight-slot inventory, layer asset references, overlay flags, display preference shape, and HUD placement shape while excluding live monitor coordinates, timestamps, and sensor warm-start snapshots from the golden fixture.",
        check_command=(sys.executable, "tools/validate_console_layout_contract.py", "--check"),
        public_app=True,
    ),
)

# Optional JSON Schema based validators (for docs/examples/*.json)
try:
    import jsonschema  # type: ignore
    JSONSCHEMA_AVAILABLE = True
except Exception:  # pragma: no cover - environment dependent
    JSONSCHEMA_AVAILABLE = False


def _make_schema_validator(schema_path: Path) -> ValidationFn:
    """Return a ValidationFn that validates data against the given JSON Schema file.

    If the `jsonschema` package isn't available, a small local fallback validates
    the subset of Draft 7 keywords used by this repo's schemas.
    """
    try:
        schema = _load_json(schema_path)
    except ValueError:
        def missing_schema(data: object, path: Path) -> List[str]:
            return [f"Failed to load schema {schema_path}"]
        return missing_schema

    if not JSONSCHEMA_AVAILABLE:
        def fallback_validator(data: object, path: Path) -> List[str]:
            return _validate_schema_subset(data, schema, schema, path.name)
        return fallback_validator

    validator = jsonschema.Draft7Validator(schema)

    def _validator(data: object, path: Path) -> List[str]:
        errors: List[str] = []
        for err in validator.iter_errors(data):
            loc = "".join([f"[{p}]" for p in err.path])
            if loc:
                where = f"{path.name}{loc}"
            else:
                where = f"{path.name}"
            errors.append(f"{where}: {err.message}")
        return errors

    return _validator


def _schema_type_matches(value: object, expected: str) -> bool:
    if expected == "object":
        return isinstance(value, dict)
    if expected == "array":
        return isinstance(value, list)
    if expected == "string":
        return isinstance(value, str)
    if expected == "integer":
        return isinstance(value, int) and not isinstance(value, bool)
    if expected == "number":
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    if expected == "boolean":
        return isinstance(value, bool)
    if expected == "null":
        return value is None
    return True


def _type_label(value: object) -> str:
    if isinstance(value, bool):
        return "boolean"
    if isinstance(value, dict):
        return "object"
    if isinstance(value, list):
        return "array"
    if isinstance(value, str):
        return "string"
    if isinstance(value, int):
        return "integer"
    if isinstance(value, float):
        return "number"
    if value is None:
        return "null"
    return type(value).__name__


def _resolve_schema_ref(root_schema: object, ref: str) -> object:
    if not ref.startswith("#/"):
        return {}
    current = root_schema
    for part in ref[2:].split("/"):
        if isinstance(current, dict):
            current = current.get(part, {})
        else:
            return {}
    return current


def _child_path(path: str, key: object) -> str:
    if isinstance(key, int):
        return f"{path}[{key}]"
    return f"{path}.{key}"


def _validate_schema_subset(value: object, schema: object, root_schema: object, path: str) -> List[str]:
    if not isinstance(schema, dict):
        return []

    if "$ref" in schema:
        resolved = _resolve_schema_ref(root_schema, str(schema["$ref"]))
        return _validate_schema_subset(value, resolved, root_schema, path)

    if "oneOf" in schema:
        options = schema.get("oneOf")
        if not isinstance(options, list):
            return []
        matches = 0
        first_errors: List[str] = []
        for option in options:
            option_errors = _validate_schema_subset(value, option, root_schema, path)
            if not option_errors:
                matches += 1
            elif not first_errors:
                first_errors = option_errors
        if matches == 1:
            return []
        if matches == 0:
            return [f"{path}: does not match any allowed schema"] + first_errors[:3]
        return [f"{path}: matches more than one allowed schema"]

    errors: List[str] = []

    expected_type = schema.get("type")
    if isinstance(expected_type, str):
        if not _schema_type_matches(value, expected_type):
            return [f"{path}: expected {expected_type}, got {_type_label(value)}"]
    elif isinstance(expected_type, list):
        if not any(_schema_type_matches(value, str(item)) for item in expected_type):
            expected = ", ".join(str(item) for item in expected_type)
            return [f"{path}: expected one of [{expected}], got {_type_label(value)}"]

    if "enum" in schema:
        enum_values = schema.get("enum")
        if isinstance(enum_values, list) and value not in enum_values:
            errors.append(f"{path}: value {value!r} is not one of {enum_values!r}")

    if isinstance(value, str):
        if "minLength" in schema and len(value) < int(schema["minLength"]):
            errors.append(f"{path}: string is shorter than {schema['minLength']}")
        if "pattern" in schema:
            pattern = str(schema["pattern"])
            if re.match(pattern, value) is None:
                errors.append(f"{path}: string does not match pattern {pattern!r}")

    if isinstance(value, (int, float)) and not isinstance(value, bool):
        if "minimum" in schema and value < schema["minimum"]:
            errors.append(f"{path}: value must be >= {schema['minimum']}")
        if "maximum" in schema and value > schema["maximum"]:
            errors.append(f"{path}: value must be <= {schema['maximum']}")

    if isinstance(value, list):
        if "minItems" in schema and len(value) < int(schema["minItems"]):
            errors.append(f"{path}: array must contain at least {schema['minItems']} items")
        if "maxItems" in schema and len(value) > int(schema["maxItems"]):
            errors.append(f"{path}: array must contain at most {schema['maxItems']} items")
        item_schema = schema.get("items")
        if isinstance(item_schema, dict):
            for idx, item in enumerate(value):
                errors.extend(_validate_schema_subset(item, item_schema, root_schema, _child_path(path, idx)))

    if isinstance(value, dict):
        required = schema.get("required", [])
        if isinstance(required, list):
            for key in required:
                if isinstance(key, str) and key not in value:
                    errors.append(f"{path}: missing required property {key!r}")

        if "minProperties" in schema and len(value) < int(schema["minProperties"]):
            errors.append(f"{path}: object must contain at least {schema['minProperties']} properties")

        properties = schema.get("properties", {})
        if not isinstance(properties, dict):
            properties = {}
        pattern_properties = schema.get("patternProperties", {})
        if not isinstance(pattern_properties, dict):
            pattern_properties = {}

        matched_keys = set()
        for key, child_schema in properties.items():
            if key in value:
                matched_keys.add(key)
                errors.extend(_validate_schema_subset(value[key], child_schema, root_schema, _child_path(path, key)))

        for pattern, child_schema in pattern_properties.items():
            try:
                compiled = re.compile(str(pattern))
            except re.error:
                continue
            for key, child_value in value.items():
                if compiled.search(str(key)):
                    matched_keys.add(key)
                    errors.extend(_validate_schema_subset(child_value, child_schema, root_schema, _child_path(path, key)))

        additional = schema.get("additionalProperties", True)
        for key, child_value in value.items():
            if key in matched_keys:
                continue
            if additional is False:
                errors.append(f"{path}: additional property {key!r} is not allowed")
            elif isinstance(additional, dict):
                errors.extend(_validate_schema_subset(child_value, additional, root_schema, _child_path(path, key)))

    return errors


def _append_browser_targets(targets: Dict[Path, ValidationFn]) -> None:
    """Populate validation targets for the Browser (Control Panel) quick check."""
    for rel_path in BROWSER_CONFIG_TARGETS:
        validator = CONFIG_VALIDATORS.get(rel_path)
        if validator is not None:
            targets[rel_path] = validator
    schemas_root = BasePath
    for rel_path, schema_rel in BROWSER_EXAMPLE_SCHEMAS.items():
        schema_path = (BasePath / schema_rel).resolve()
        targets[rel_path] = _make_schema_validator(schema_path)


def _contract_schema_for_resolved_path(resolved: Path):
    try:
        rel_path = resolved.relative_to(BasePath)
    except ValueError:
        return None

    if rel_path.match("synaptome/bin/data/device_maps/*.json"):
        return BasePath / "docs" / "schemas" / "device_map.schema.json"

    if rel_path.match("synaptome/bin/data/layers/scenes/*.json"):
        return BasePath / "docs" / "schemas" / "scene.schema.json"

    if rel_path.match("synaptome/bin/data/layers/**/*.json"):
        return BasePath / "docs" / "schemas" / "layer_asset.schema.json"

    return None


def _append_contract_schema_targets(targets: Dict[Path, ValidationFn]) -> None:
    data_root = BasePath / "synaptome" / "bin" / "data"

    device_schema = BasePath / "docs" / "schemas" / "device_map.schema.json"
    device_maps_dir = data_root / "device_maps"
    if device_schema.exists() and device_maps_dir.exists():
        for path in device_maps_dir.glob("*.json"):
            rel_path = path.relative_to(BasePath)
            targets.setdefault(rel_path, _make_schema_validator(device_schema))

    layer_schema = BasePath / "docs" / "schemas" / "layer_asset.schema.json"
    layers_dir = data_root / "layers"
    scenes_dir = layers_dir / "scenes"
    if layer_schema.exists() and layers_dir.exists():
        for path in layers_dir.rglob("*.json"):
            if scenes_dir in path.parents:
                continue
            rel_path = path.relative_to(BasePath)
            targets.setdefault(rel_path, _make_schema_validator(layer_schema))

    scene_schema = BasePath / "docs" / "schemas" / "scene.schema.json"
    if scene_schema.exists() and scenes_dir.exists():
        for path in scenes_dir.glob("*.json"):
            rel_path = path.relative_to(BasePath)
            targets.setdefault(rel_path, _make_schema_validator(scene_schema))


def _source_status(paths: tuple[Path, ...]) -> tuple[bool, List[Path]]:
    missing: List[Path] = []
    for rel_path in paths:
        if not (BasePath / rel_path).exists():
            missing.append(rel_path)
    return (len(missing) == 0, missing)


def _run_contract_check(command: tuple[str, ...]) -> tuple[bool, str]:
    if not command:
        return True, ""
    try:
        result = subprocess.run(
            command,
            cwd=BasePath,
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError as exc:
        return False, str(exc)
    output = "\n".join(part.strip() for part in (result.stdout, result.stderr) if part.strip())
    return result.returncode == 0, output


def _print_contract_report(public_app_only: bool = False) -> int:
    index_path = BasePath / CONTRACT_INDEX_DOC
    if not index_path.exists():
        print(f"contract index missing: {CONTRACT_INDEX_DOC}")
        return 1

    entries = CONTRACT_ENTRIES
    title = "Contract coverage report"
    if public_app_only:
        entries = tuple(entry for entry in CONTRACT_ENTRIES if entry.public_app)
        title = "Public app contract coverage report"

    print(f"{title} ({CONTRACT_INDEX_DOC})")
    if public_app_only:
        print("mode: Synaptome app/runtime extraction subset")
    print("")

    counts: Dict[str, int] = {}
    missing_sources = False
    failed_checks = False
    for entry in entries:
        sources_present, missing = _source_status(entry.sources)
        status = entry.status if sources_present else "missing-source"
        check_output = ""
        if sources_present and entry.check_command:
            check_ok, check_output = _run_contract_check(entry.check_command)
            if not check_ok:
                status = "check-failed"
                failed_checks = True
        counts[status] = counts.get(status, 0) + 1
        if missing:
            missing_sources = True

        print(f"- {entry.name}: {status}")
        print(f"  validator: {entry.validator_command}")
        if entry.fixtures:
            fixture_text = ", ".join(str(path) for path in entry.fixtures)
            print(f"  fixtures: {fixture_text}")
        if missing:
            missing_text = ", ".join(str(path) for path in missing)
            print(f"  missing sources: {missing_text}")
        if entry.check_command:
            command_text = " ".join(entry.check_command)
            print(f"  check: {command_text}")
            if check_output:
                for line in check_output.splitlines():
                    print(f"    {line}")
        if entry.notes:
            print(f"  notes: {entry.notes}")

    print("")
    summary = ", ".join(f"{key}={counts[key]}" for key in sorted(counts))
    print(f"Summary: {summary}")
    if missing_sources:
        print("error: one or more contract sources are missing")
        return 1
    if failed_checks:
        print("error: one or more contract checks failed")
        return 1
    return 0


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description="Validate tensegrity configuration files")
    parser.add_argument(
        "paths",
        nargs="*",
        help="Optional subset of config files to validate (relative or absolute paths). Default: all known configs.",
    )
    parser.add_argument(
        "--browser",
        dest="browser",
        action="store_true",
        help="Validate only the Browser (Control Panel) configs and examples.",
    )
    parser.add_argument(
        "--control-hub",
        dest="browser",
        action="store_true",
        help="(Deprecated) alias for --browser; validates the same Browser configs.",
    )
    parser.add_argument(
        "--contracts",
        dest="contracts",
        action="store_true",
        help="Report contract coverage status from docs/contracts/README.md without changing normal config validation.",
    )
    parser.add_argument(
        "--public-app",
        dest="public_app",
        action="store_true",
        help="Report only public Synaptome app/runtime contracts for extraction validation.",
    )
    args = parser.parse_args(argv)

    if args.contracts and args.public_app:
        parser.error("--contracts cannot be combined with --public-app")

    if args.contracts or args.public_app:
        if args.paths or args.browser:
            parser.error("--contracts/--public-app cannot be combined with paths, --browser, or --control-hub")
        return _print_contract_report(public_app_only=args.public_app)

    targets: Dict[Path, ValidationFn] = {}
    if args.paths:
        for raw in args.paths:
            # allow absolute or repo-relative paths
            resolved = (BasePath / raw).resolve() if not Path(raw).is_absolute() else Path(raw)
            matched = None
            for cfg_path, validator in CONFIG_VALIDATORS.items():
                full = (BasePath / cfg_path).resolve()
                if resolved == full:
                    matched = (cfg_path, validator)
                    break
            if matched is None:
                contract_schema = _contract_schema_for_resolved_path(resolved)
                if contract_schema is not None and contract_schema.exists():
                    targets[Path(raw) if Path(raw).is_absolute() else Path(raw)] = _make_schema_validator(contract_schema)
                    continue
                # Try to find a matching schema in docs/schemas by stem.
                # Support example filenames like 'hotkeys_example.json' -> schema 'hotkeys.schema.json'.
                stem = resolved.stem
                if stem.endswith("_example"):
                    schema_stem = stem[: -len("_example")]
                else:
                    schema_stem = stem
                try_schema = (BasePath / "docs" / "schemas" / f"{schema_stem}.schema.json").resolve()
                if try_schema.exists():
                    targets[Path(raw) if Path(raw).is_absolute() else Path(raw)] = _make_schema_validator(try_schema)
                    continue
                print(f"warning: no validator registered for {raw}")
                continue
            targets[matched[0]] = matched[1]
    elif not args.browser:
        # start with the built-in validators, then auto-include docs/examples/* where a schema exists
        targets = dict(CONFIG_VALIDATORS)
        examples_dir = BasePath / "docs" / "examples"
        schemas_dir = BasePath / "docs" / "schemas"
        if examples_dir.exists():
            for p in examples_dir.glob("*.json"):
                # map 'hotkeys_example.json' -> 'hotkeys.schema.json'
                if p.stem.endswith("_example"):
                    schema_stem = p.stem[: -len("_example")]
                else:
                    schema_stem = p.stem
                schema_path = schemas_dir / f"{schema_stem}.schema.json"
                if schema_path.exists():
                    rel_example = Path("docs") / "examples" / p.name
                    targets[rel_example] = _make_schema_validator(schema_path)
        _append_contract_schema_targets(targets)

    if args.browser:
        _append_browser_targets(targets)

    exit_code = 0
    for rel_path, validator in targets.items():
        # rel_path may be absolute (user supplied) or repo-relative (keys in CONFIG_VALIDATORS)
        if rel_path.is_absolute():
            full_path = rel_path.resolve()
        else:
            full_path = (BasePath / rel_path).resolve()
        if not full_path.exists():
            print(f"warning: {rel_path} not found (skipping)")
            continue
        try:
            data = _load_json(full_path)
            errors = validator(data, full_path)
        except ValueError as exc:
            print(exc)
            exit_code = 1
            continue
        if errors:
            exit_code = 1
            print(f"{rel_path} failed validation:")
            for msg in errors:
                print(f"  - {msg}")
        else:
            print(f"{rel_path}: ok")
    return exit_code


if __name__ == "__main__":  # pragma: no cover - CLI entry
    sys.exit(main(sys.argv[1:]))

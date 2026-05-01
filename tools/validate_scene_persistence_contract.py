#!/usr/bin/env python3
"""Validate saved scene persistence fixtures.

This is a static contract gate. It checks committed scene JSON for stable
scene-owned shape and compares a compact golden summary. Runtime staged apply
and rollback semantics remain owned by the scene/display transaction child.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
LIVE_SCENE_DIR = ROOT / "synaptome/bin/data/layers/scenes"
LIVE_SCENE_LAST = ROOT / "synaptome/bin/data/config/scene-last.json"
FIXTURE_SCENE_DIR = ROOT / "tools/testdata/runtime_state/layers/scenes"
FIXTURE_SCENE_LAST = ROOT / "tools/testdata/runtime_state/config/scene-last.json"
LAYER_DIR = ROOT / "synaptome/bin/data/layers"
EXPECTED = ROOT / "tools/testdata/scene_persistence/expected_scene_contract.json"


class ContractError(RuntimeError):
    pass


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ContractError(f"{path}: JSON parse failed: {exc}") from exc


def canonical_json(data: Any) -> str:
    return json.dumps(data, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def layer_asset_ids() -> set[str]:
    ids: set[str] = set()
    for path in sorted(LAYER_DIR.rglob("*.json")):
        if "scenes" in path.relative_to(LAYER_DIR).parts:
            continue
        data = load_json(path)
        if isinstance(data, dict) and isinstance(data.get("id"), str):
            ids.add(data["id"])
    return ids


def validate_scalar(value: Any, context: str) -> None:
    if isinstance(value, (int, float, str, bool)) or value is None:
        return
    if isinstance(value, dict):
        if "base" not in value:
            raise ContractError(f"{context}: modifier object must include base")
        validate_scalar(value["base"], f"{context}.base")
        modifiers = value.get("modifiers", [])
        if not isinstance(modifiers, list):
            raise ContractError(f"{context}.modifiers must be an array")
        return
    raise ContractError(f"{context}: unsupported persisted value type {type(value).__name__}")


def collect_bank_targets(node: Any) -> list[str]:
    targets: list[str] = []

    def walk(value: Any) -> None:
        if isinstance(value, dict):
            target = value.get("target")
            if isinstance(target, str):
                targets.append(target)
            for child in value.values():
                walk(child)
        elif isinstance(value, list):
            for child in value:
                walk(child)

    walk(node)
    return sorted(set(targets))


def summarize_scene(path: Path, asset_ids: set[str]) -> dict[str, Any]:
    data = load_json(path)
    if not isinstance(data, dict):
        raise ContractError(f"{path}: scene root must be an object")

    reparsed = json.loads(canonical_json(data))
    if reparsed != data:
        raise ContractError(f"{path}: scene is not JSON round-trip stable")

    console = data.get("console", {})
    if console is not None and not isinstance(console, dict):
        raise ContractError(f"{path}: console must be an object")
    slots = console.get("slots", []) if isinstance(console, dict) else []
    if not isinstance(slots, list):
        raise ContractError(f"{path}: console.slots must be an array")

    seen_indices: set[int] = set()
    slot_summaries: list[dict[str, Any]] = []
    total_params = 0
    coverage_modes: set[str] = set()

    for idx, slot in enumerate(slots):
        context = f"{path}: console.slots[{idx}]"
        if not isinstance(slot, dict):
            raise ContractError(f"{context} must be an object")
        index = slot.get("index")
        if not isinstance(index, int) or not 1 <= index <= 8:
            raise ContractError(f"{context}.index must be an integer from 1 to 8")
        if index in seen_indices:
            raise ContractError(f"{path}: duplicate console slot index {index}")
        seen_indices.add(index)

        asset_id = slot.get("assetId")
        if not isinstance(asset_id, str) or not asset_id:
            raise ContractError(f"{context}.assetId must be a non-empty string")
        if asset_id not in asset_ids:
            raise ContractError(f"{context}.assetId '{asset_id}' is not in the layer catalog")
        if "active" in slot and not isinstance(slot["active"], bool):
            raise ContractError(f"{context}.active must be boolean")
        if "opacity" in slot and not isinstance(slot["opacity"], (int, float)):
            raise ContractError(f"{context}.opacity must be numeric")
        if isinstance(slot.get("opacity"), (int, float)) and not 0 <= float(slot["opacity"]) <= 1:
            raise ContractError(f"{context}.opacity must be between 0 and 1")
        if "label" in slot and not isinstance(slot["label"], str):
            raise ContractError(f"{context}.label must be a string")

        coverage = slot.get("coverage")
        if coverage is not None:
            if not isinstance(coverage, dict):
                raise ContractError(f"{context}.coverage must be an object")
            mode = coverage.get("mode")
            if not isinstance(mode, str) or not mode:
                raise ContractError(f"{context}.coverage.mode must be a non-empty string")
            coverage_modes.add(mode)
            columns = coverage.get("columns")
            if columns is not None and (not isinstance(columns, int) or columns < 0):
                raise ContractError(f"{context}.coverage.columns must be a non-negative integer")

        params = slot.get("parameters", {})
        if params is not None and not isinstance(params, dict):
            raise ContractError(f"{context}.parameters must be an object")
        param_count = len(params) if isinstance(params, dict) else 0
        total_params += param_count
        if isinstance(params, dict):
            for param_id, value in params.items():
                if not isinstance(param_id, str) or not param_id:
                    raise ContractError(f"{context}.parameters contains a non-string id")
                validate_scalar(value, f"{context}.parameters.{param_id}")

        slot_summaries.append(
            {
                "index": index,
                "assetId": asset_id,
                "active": bool(slot.get("active", True)),
                "parameterCount": param_count,
                "hasCoverage": coverage is not None,
            }
        )

    for root_key in ("globals", "effects"):
        value = data.get(root_key, {})
        if value is not None and not isinstance(value, dict):
            raise ContractError(f"{path}: {root_key} must be an object")
        if isinstance(value, dict):
            for key, child in value.items():
                if root_key == "globals":
                    validate_scalar(child, f"{path}: globals.{key}")
                elif not isinstance(child, dict):
                    raise ContractError(f"{path}: effects.{key} must be an object")

    bank_targets = collect_bank_targets(data.get("banks", {}))
    return {
        "path": relative(path),
        "hash": sha256_text(canonical_json(data)),
        "topLevelKeys": sorted(data.keys()),
        "slotCount": len(slots),
        "activeSlotCount": sum(1 for slot in slot_summaries if slot["active"]),
        "slotAssets": [slot["assetId"] for slot in slot_summaries],
        "slotParameterCount": total_params,
        "coverageModes": sorted(coverage_modes),
        "effectKeys": sorted(data.get("effects", {}).keys()) if isinstance(data.get("effects", {}), dict) else [],
        "globalKeys": sorted(data.get("globals", {}).keys()) if isinstance(data.get("globals", {}), dict) else [],
        "bankTargets": bank_targets,
    }


def build_snapshot(scene_dir: Path, scene_last: Path) -> dict[str, Any]:
    asset_ids = layer_asset_ids()
    if not asset_ids:
        raise ContractError("No layer asset IDs found")
    scene_paths = sorted(scene_dir.glob("*.json")) + [scene_last]
    scenes = [summarize_scene(path, asset_ids) for path in scene_paths]
    return {
        "version": 1,
        "sceneCount": len(scenes),
        "layerAssetCount": len(asset_ids),
        "scenes": scenes,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true", help="refresh the expected scene contract fixture")
    parser.add_argument("--check", action="store_true", help="check against the expected scene contract fixture")
    parser.add_argument(
        "--live",
        action="store_true",
        help="Use live app-written scene files instead of committed runtime-state fixtures",
    )
    parser.add_argument("--scene-dir", type=Path, help="Override scene directory")
    parser.add_argument("--scene-last", type=Path, help="Override scene-last path")
    args = parser.parse_args()

    try:
        scene_dir = args.scene_dir if args.scene_dir else (LIVE_SCENE_DIR if args.live else FIXTURE_SCENE_DIR)
        scene_last = args.scene_last if args.scene_last else (LIVE_SCENE_LAST if args.live else FIXTURE_SCENE_LAST)
        if not scene_dir.is_absolute():
            scene_dir = ROOT / scene_dir
        if not scene_last.is_absolute():
            scene_last = ROOT / scene_last
        snapshot = build_snapshot(scene_dir, scene_last)
        if args.write:
            EXPECTED.parent.mkdir(parents=True, exist_ok=True)
            EXPECTED.write_text(json.dumps(snapshot, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            print(f"Wrote {relative(EXPECTED)}")
            return 0
        expected = load_json(EXPECTED)
        if snapshot != expected:
            raise ContractError(f"Scene persistence fixture drifted; run {Path(__file__).name} --write after intentional changes")
        print(f"Scene persistence contract passed ({snapshot['sceneCount']} scenes)")
        return 0
    except ContractError as exc:
        print(f"Scene persistence contract failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

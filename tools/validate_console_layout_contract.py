#!/usr/bin/env python3
"""Validate the Synaptome Console/display persistence contract.

The Console file contains both durable presentation state and volatile runtime
snapshots. This validator keeps the public contract strict around slot shape,
asset references, overlay flags, display preferences, and HUD placement shape,
while intentionally ignoring monitor coordinates, timestamps, and sensor
snapshots in the golden fixture.
"""
from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]
APP_ROOT = REPO_ROOT / "synaptome"
LIVE_CONSOLE_CONFIG = APP_ROOT / "bin" / "data" / "config" / "console.json"
LIVE_SLOT_ASSIGNMENTS = APP_ROOT / "bin" / "data" / "config" / "ui" / "slot_assignments.json"
FIXTURE_CONSOLE_CONFIG = REPO_ROOT / "tools" / "testdata" / "runtime_state" / "config" / "console.json"
FIXTURE_SLOT_ASSIGNMENTS = REPO_ROOT / "tools" / "testdata" / "runtime_state" / "config" / "ui" / "slot_assignments.json"
CONSOLE_CONFIG = FIXTURE_CONSOLE_CONFIG
SLOT_ASSIGNMENTS = FIXTURE_SLOT_ASSIGNMENTS
LAYER_ASSET_ROOT = APP_ROOT / "bin" / "data" / "layers"
HUD_ASSET_ROOT = LAYER_ASSET_ROOT / "hud"
CONSOLE_STORE = APP_ROOT / "src" / "io" / "ConsoleStore.h"
LAYER_CATALOG_EXPECTED = REPO_ROOT / "tools" / "testdata" / "layer_catalog" / "expected_catalog.json"
DEFAULT_EXPECTED = REPO_ROOT / "tools" / "testdata" / "console_layout" / "expected_console_contract.json"

SLOT_COUNT = 8
ALLOWED_DUAL_DISPLAY_MODES = {"single", "dual"}
ALLOWED_LAYOUT_TARGETS = {"projector", "controller"}
ALLOWED_WIDGET_TARGETS = {"projector", "controller", "both"}
ALLOWED_BANDS = {"hud", "console", "workbench"}
BACKGROUND_RE = re.compile(r"^#[0-9A-Fa-f]{6}$")


def rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def layer_asset_ids() -> set[str]:
    if LAYER_CATALOG_EXPECTED.exists():
        catalog = load_json(LAYER_CATALOG_EXPECTED)
        if isinstance(catalog, dict) and isinstance(catalog.get("entries"), list):
            ids = {entry.get("id") for entry in catalog["entries"] if isinstance(entry, dict)}
            return {str(item) for item in ids if isinstance(item, str) and item}

    ids: set[str] = set()
    scenes_dir = LAYER_ASSET_ROOT / "scenes"
    for path in LAYER_ASSET_ROOT.rglob("*.json"):
        if scenes_dir in path.parents:
            continue
        data = load_json(path)
        if isinstance(data, dict) and isinstance(data.get("id"), str):
            ids.add(data["id"])
    return ids


def hud_widget_ids() -> set[str]:
    ids: set[str] = set()
    for path in HUD_ASSET_ROOT.glob("*.json"):
        data = load_json(path)
        if isinstance(data, dict) and isinstance(data.get("id"), str):
            ids.add(data["id"])
    return ids


def normalize_layers(data: Any, valid_asset_ids: set[str], errors: list[str]) -> list[dict[str, Any]]:
    ctx = rel(CONSOLE_CONFIG)
    if not isinstance(data, list):
        errors.append(f"{ctx}.layers must be an array")
        return []
    if len(data) != SLOT_COUNT:
        errors.append(f"{ctx}.layers must contain exactly {SLOT_COUNT} entries")

    seen_indices: set[int] = set()
    normalized: list[dict[str, Any]] = []
    for idx, entry in enumerate(data):
        entry_ctx = f"{ctx}.layers[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue

        slot_index = entry.get("index")
        if not is_int(slot_index):
            errors.append(f"{entry_ctx}.index must be an integer")
            slot_index = idx + 1
        elif slot_index < 1 or slot_index > SLOT_COUNT:
            errors.append(f"{entry_ctx}.index must be between 1 and {SLOT_COUNT}")
        if is_int(slot_index):
            if slot_index in seen_indices:
                errors.append(f"{entry_ctx}.index {slot_index} is duplicated")
            seen_indices.add(slot_index)

        asset_id = entry.get("assetId", "")
        if not isinstance(asset_id, str):
            errors.append(f"{entry_ctx}.assetId must be a string")
            asset_id = ""
        elif asset_id and asset_id not in valid_asset_ids:
            errors.append(f"{entry_ctx}.assetId {asset_id!r} is not in the layer catalog")

        active = entry.get("active")
        if not isinstance(active, bool):
            errors.append(f"{entry_ctx}.active must be boolean")
            active = False
        if active and not asset_id:
            errors.append(f"{entry_ctx}.assetId must be non-empty when active is true")

        opacity = entry.get("opacity", 1.0)
        if not is_number(opacity):
            errors.append(f"{entry_ctx}.opacity must be numeric")
            opacity = 1.0
        elif opacity < 0.0 or opacity > 1.0:
            errors.append(f"{entry_ctx}.opacity must be between 0.0 and 1.0")

        normalized_entry: dict[str, Any] = {
            "index": int(slot_index) if is_int(slot_index) else idx + 1,
            "assetId": asset_id,
            "active": bool(active),
            "opacity": float(opacity) if is_number(opacity) else 1.0,
        }

        for key in ("label", "displayName"):
            if key in entry:
                if not isinstance(entry[key], str):
                    errors.append(f"{entry_ctx}.{key} must be a string when present")
                else:
                    normalized_entry[key] = entry[key]

        coverage = entry.get("coverage")
        if coverage is not None:
            if not isinstance(coverage, dict):
                errors.append(f"{entry_ctx}.coverage must be an object when present")
            else:
                coverage_out: dict[str, Any] = {}
                mode = coverage.get("mode", "upstream")
                if not isinstance(mode, str) or not mode:
                    errors.append(f"{entry_ctx}.coverage.mode must be a non-empty string")
                    mode = "upstream"
                coverage_out["mode"] = mode

                columns = coverage.get("columns", 0)
                if not is_int(columns):
                    errors.append(f"{entry_ctx}.coverage.columns must be an integer")
                    columns = 0
                elif columns < 0:
                    errors.append(f"{entry_ctx}.coverage.columns must be >= 0")
                coverage_out["columns"] = max(0, int(columns)) if is_int(columns) else 0
                normalized_entry["coverage"] = coverage_out

        normalized.append(normalized_entry)

    expected_indices = set(range(1, SLOT_COUNT + 1))
    if seen_indices != expected_indices:
        missing = sorted(expected_indices - seen_indices)
        extra = sorted(seen_indices - expected_indices)
        if missing:
            errors.append(f"{ctx}.layers missing slot indices {missing}")
        if extra:
            errors.append(f"{ctx}.layers has invalid slot indices {extra}")

    normalized.sort(key=lambda item: item["index"])
    return normalized


def require_bool_object(data: Any, ctx: str, fields: tuple[str, ...], errors: list[str]) -> dict[str, bool]:
    if not isinstance(data, dict):
        errors.append(f"{ctx} must be an object")
        return {}
    out: dict[str, bool] = {}
    for field in fields:
        value = data.get(field)
        if not isinstance(value, bool):
            errors.append(f"{ctx}.{field} must be boolean")
            value = False
        out[field] = bool(value)
    return out


def validate_secondary_display(data: Any, errors: list[str]) -> dict[str, Any]:
    ctx = f"{rel(CONSOLE_CONFIG)}.secondaryDisplay"
    if not isinstance(data, dict):
        errors.append(f"{ctx} must be an object")
        return {}

    bool_fields = ("enabled", "vsync", "followPrimary")
    int_fields = ("x", "y", "width", "height")
    for field in bool_fields:
        if not isinstance(data.get(field), bool):
            errors.append(f"{ctx}.{field} must be boolean")
    for field in int_fields:
        if not is_int(data.get(field)):
            errors.append(f"{ctx}.{field} must be an integer")

    if is_int(data.get("width")) and data["width"] <= 0:
        errors.append(f"{ctx}.width must be > 0")
    if is_int(data.get("height")) and data["height"] <= 0:
        errors.append(f"{ctx}.height must be > 0")

    monitor_id = data.get("monitorId", "")
    if not isinstance(monitor_id, str):
        errors.append(f"{ctx}.monitorId must be a string")

    dpi_scale = data.get("dpiScale", 1.0)
    if not is_number(dpi_scale):
        errors.append(f"{ctx}.dpiScale must be numeric")
    elif dpi_scale <= 0:
        errors.append(f"{ctx}.dpiScale must be > 0")

    background = data.get("background", "#000000")
    if not isinstance(background, str) or not BACKGROUND_RE.match(background):
        errors.append(f"{ctx}.background must be a #RRGGBB string")

    # Shape only: actual monitor coordinates and dimensions are local to the operator machine.
    return {
        "fields": sorted(data.keys()),
        "backgroundFormat": "#RRGGBB",
        "coordinatePolicy": "local-machine-values-validated-but-not-golden",
    }


def normalize_widget_entry(
    value: Any,
    ctx: str,
    allowed_ids: set[str],
    errors: list[str],
) -> dict[str, Any] | None:
    if not isinstance(value, dict):
        errors.append(f"{ctx} must be an object")
        return None

    widget_id = value.get("id")
    if not isinstance(widget_id, str) or not widget_id:
        errors.append(f"{ctx}.id must be a non-empty string")
        return None
    if widget_id not in allowed_ids:
        errors.append(f"{ctx}.id {widget_id!r} is not a HUD widget")

    column = value.get("column")
    if not is_int(column):
        errors.append(f"{ctx}.column must be an integer")
        column = 0
    elif column < 0:
        errors.append(f"{ctx}.column must be >= 0")

    visible = value.get("visible")
    if not isinstance(visible, bool):
        errors.append(f"{ctx}.visible must be boolean")
        visible = True

    collapsed = value.get("collapsed")
    if not isinstance(collapsed, bool):
        errors.append(f"{ctx}.collapsed must be boolean")
        collapsed = False

    band = value.get("band", "hud")
    if not isinstance(band, str) or band not in ALLOWED_BANDS:
        errors.append(f"{ctx}.band must be one of {sorted(ALLOWED_BANDS)}")
        band = "hud"

    target = value.get("target", "projector")
    if not isinstance(target, str) or target not in ALLOWED_WIDGET_TARGETS:
        errors.append(f"{ctx}.target must be one of {sorted(ALLOWED_WIDGET_TARGETS)}")
        target = "projector"

    return {
        "id": widget_id,
        "column": int(column) if is_int(column) else 0,
        "visible": bool(visible),
        "collapsed": bool(collapsed),
        "band": band,
        "target": target,
    }


def normalize_overlay_layouts(data: Any, allowed_ids: set[str], errors: list[str]) -> dict[str, Any]:
    ctx = f"{rel(CONSOLE_CONFIG)}.overlayLayouts"
    if not isinstance(data, dict):
        errors.append(f"{ctx} must be an object")
        return {}

    active_target = data.get("activeTarget", "projector")
    if not isinstance(active_target, str) or active_target not in ALLOWED_LAYOUT_TARGETS:
        errors.append(f"{ctx}.activeTarget must be one of {sorted(ALLOWED_LAYOUT_TARGETS)}")
        active_target = "projector"

    if "lastSyncMs" in data and not is_number(data.get("lastSyncMs")):
        errors.append(f"{ctx}.lastSyncMs must be numeric when present")

    targets: dict[str, list[dict[str, Any]]] = {}
    for target in sorted(ALLOWED_LAYOUT_TARGETS):
        target_node = data.get(target)
        target_ctx = f"{ctx}.{target}"
        if not isinstance(target_node, dict):
            errors.append(f"{target_ctx} must be an object")
            targets[target] = []
            continue
        if "capturedAtMs" in target_node and not is_number(target_node.get("capturedAtMs")):
            errors.append(f"{target_ctx}.capturedAtMs must be numeric when present")
        widgets = target_node.get("widgets")
        if not isinstance(widgets, list):
            errors.append(f"{target_ctx}.widgets must be an array")
            targets[target] = []
            continue
        entries: list[dict[str, Any]] = []
        seen: set[str] = set()
        for idx, item in enumerate(widgets):
            entry = normalize_widget_entry(item, f"{target_ctx}.widgets[{idx}]", allowed_ids, errors)
            if entry is None:
                continue
            if entry["id"] in seen:
                errors.append(f"{target_ctx}.widgets duplicate id {entry['id']!r}")
            seen.add(entry["id"])
            entries.append(entry)
        entries.sort(key=lambda item: item["id"])
        targets[target] = entries

    return {
        "activeTarget": active_target,
        "targets": targets,
        "timestampPolicy": "lastSyncMs-and-capturedAtMs-validated-but-not-golden",
    }


def normalize_slot_assignments(data: Any, valid_asset_ids: set[str], errors: list[str]) -> dict[str, Any]:
    ctx = rel(SLOT_ASSIGNMENTS)
    if not isinstance(data, dict):
        errors.append(f"{ctx}: root must be an object")
        return {}
    assignments = data.get("assignments")
    if not isinstance(assignments, list):
        errors.append(f"{ctx}.assignments must be an array")
        return {"assignments": []}

    normalized: list[dict[str, Any]] = []
    seen_slots: set[int] = set()
    for idx, entry in enumerate(assignments):
        entry_ctx = f"{ctx}.assignments[{idx}]"
        if not isinstance(entry, dict):
            errors.append(f"{entry_ctx} must be an object")
            continue
        out: dict[str, Any] = {}
        slot = entry.get("slot", entry.get("index"))
        if slot is not None:
            if not is_int(slot) or slot < 1 or slot > SLOT_COUNT:
                errors.append(f"{entry_ctx}.slot/index must be an integer between 1 and {SLOT_COUNT}")
            else:
                if slot in seen_slots:
                    errors.append(f"{entry_ctx}.slot/index {slot} is duplicated")
                seen_slots.add(slot)
                out["slot"] = int(slot)
        asset_id = entry.get("assetId")
        if asset_id is not None:
            if not isinstance(asset_id, str):
                errors.append(f"{entry_ctx}.assetId must be a string")
            elif asset_id and asset_id not in valid_asset_ids:
                errors.append(f"{entry_ctx}.assetId {asset_id!r} is not in the layer catalog")
            else:
                out["assetId"] = asset_id
        normalized.append(out)

    normalized.sort(key=lambda item: (item.get("slot", 999), item.get("assetId", "")))
    return {"assignments": normalized}


def normalize_console_config(
    data: Any,
    valid_asset_ids: set[str],
    hud_ids: set[str],
    errors: list[str],
) -> dict[str, Any]:
    ctx = rel(CONSOLE_CONFIG)
    if not isinstance(data, dict):
        errors.append(f"{ctx}: root must be an object")
        return {}

    version = data.get("version")
    if not is_int(version):
        errors.append(f"{ctx}.version must be an integer")
        version = 0

    dual_display = data.get("dualDisplay")
    dual_mode = ""
    if not isinstance(dual_display, dict):
        errors.append(f"{ctx}.dualDisplay must be an object")
    else:
        dual_mode = dual_display.get("mode", "single")
        if not isinstance(dual_mode, str) or dual_mode not in ALLOWED_DUAL_DISPLAY_MODES:
            errors.append(f"{ctx}.dualDisplay.mode must be one of {sorted(ALLOWED_DUAL_DISPLAY_MODES)}")
            dual_mode = "single"

    focus = data.get("controllerFocus")
    if not isinstance(focus, dict):
        errors.append(f"{ctx}.controllerFocus must be an object")
        focus_value = False
    else:
        focus_value = focus.get("consolePreferred")
        if not isinstance(focus_value, bool):
            errors.append(f"{ctx}.controllerFocus.consolePreferred must be boolean")
            focus_value = False

    return {
        "version": int(version) if is_int(version) else 0,
        "layers": normalize_layers(data.get("layers"), valid_asset_ids, errors),
        "overlays": require_bool_object(
            data.get("overlays"),
            f"{ctx}.overlays",
            ("hudVisible", "consoleVisible", "controlHubVisible", "menuVisible"),
            errors,
        ),
        "dualDisplay": {"mode": dual_mode},
        "secondaryDisplay": validate_secondary_display(data.get("secondaryDisplay"), errors),
        "controllerFocus": {"consolePreferred": bool(focus_value)},
        "overlayLayouts": normalize_overlay_layouts(data.get("overlayLayouts"), hud_ids, errors),
    }


def build_contract() -> tuple[dict[str, Any], list[str]]:
    errors: list[str] = []
    valid_asset_ids = layer_asset_ids()
    hud_ids = hud_widget_ids()
    if not valid_asset_ids:
        errors.append(f"{rel(LAYER_ASSET_ROOT)}: no layer catalog asset IDs found")
    if not hud_ids:
        errors.append(f"{rel(HUD_ASSET_ROOT)}: no HUD widget IDs found")

    console = normalize_console_config(load_json(CONSOLE_CONFIG), valid_asset_ids, hud_ids, errors)
    assignments = normalize_slot_assignments(load_json(SLOT_ASSIGNMENTS), valid_asset_ids, errors)

    contract = {
        "schemaVersion": 1,
        "sourceStrategy": (
            "Static Console/display persistence contract. Slot shape, asset references, overlay flags, "
            "display preference types, and HUD placement shape are validated. Local monitor coordinates, "
            "timestamps, and sensor warm-start snapshots are intentionally excluded from the golden fixture."
        ),
        "sources": [
            rel(CONSOLE_CONFIG),
            rel(SLOT_ASSIGNMENTS),
            rel(CONSOLE_STORE),
            rel(LAYER_CATALOG_EXPECTED),
            rel(HUD_ASSET_ROOT),
        ],
        "policies": {
            "slotCount": SLOT_COUNT,
            "dualDisplayModes": sorted(ALLOWED_DUAL_DISPLAY_MODES),
            "layoutTargets": sorted(ALLOWED_LAYOUT_TARGETS),
            "widgetTargets": sorted(ALLOWED_WIDGET_TARGETS),
            "bands": sorted(ALLOWED_BANDS),
            "localOnlyFields": [
                "overlayLayouts.lastSyncMs",
                "overlayLayouts.*.capturedAtMs",
                "secondaryDisplay.monitorId",
                "secondaryDisplay.x",
                "secondaryDisplay.y",
                "secondaryDisplay.width",
                "secondaryDisplay.height",
                "sensors",
            ],
        },
        "counts": {
            "validLayerAssets": len(valid_asset_ids),
            "hudWidgets": len(hud_ids),
            "consoleLayers": len(console.get("layers", [])) if isinstance(console, dict) else 0,
            "slotAssignments": len(assignments.get("assignments", [])) if isinstance(assignments, dict) else 0,
        },
        "console": console,
        "slotAssignments": assignments,
    }
    return contract, errors


def dumps(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def check_contract(expected_path: Path) -> int:
    actual, errors = build_contract()
    if errors:
        print("Console layout contract validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    actual_text = dumps(actual)
    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    if actual_text != expected_text:
        print(f"Console layout snapshot is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated Console layout contract",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1

    counts = actual["counts"]
    print(
        "Console layout contract passed "
        f"({counts['consoleLayers']} slots, {counts['validLayerAssets']} layer assets, "
        f"{counts['hudWidgets']} HUD widgets)"
    )
    return 0


def write_contract(expected_path: Path) -> int:
    actual, errors = build_contract()
    if errors:
        print("Console layout contract validation failed; snapshot not written:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    expected_path.parent.mkdir(parents=True, exist_ok=True)
    expected_path.write_text(dumps(actual), encoding="utf-8")
    print(f"Wrote Console layout snapshot: {rel(expected_path)}")
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected Console layout snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--expected", type=Path, default=DEFAULT_EXPECTED, help="Expected snapshot path")
    parser.add_argument(
        "--live",
        action="store_true",
        help="Use live app-written Console state instead of committed runtime-state fixtures",
    )
    parser.add_argument("--console-config", type=Path, help="Override Console config path")
    parser.add_argument("--slot-assignments", type=Path, help="Override slot assignments path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    global CONSOLE_CONFIG, SLOT_ASSIGNMENTS
    args = parse_args(argv)
    CONSOLE_CONFIG = args.console_config or (LIVE_CONSOLE_CONFIG if args.live else FIXTURE_CONSOLE_CONFIG)
    SLOT_ASSIGNMENTS = args.slot_assignments or (LIVE_SLOT_ASSIGNMENTS if args.live else FIXTURE_SLOT_ASSIGNMENTS)
    if not CONSOLE_CONFIG.is_absolute():
        CONSOLE_CONFIG = REPO_ROOT / CONSOLE_CONFIG
    if not SLOT_ASSIGNMENTS.is_absolute():
        SLOT_ASSIGNMENTS = REPO_ROOT / SLOT_ASSIGNMENTS
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        return write_contract(expected_path)
    return check_contract(expected_path)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

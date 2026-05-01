#!/usr/bin/env python3
"""Validate the Synaptome HUD layout contract.

This is a static guard for the operator-facing HUD surface. It intentionally
validates widget identity, placement, layout targets, bands, and feed names,
while leaving live feed payloads, timestamps, display geometry, and Browser
selection preferences out of the contract.
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
HUD_ASSET_ROOT = APP_ROOT / "bin" / "data" / "layers" / "hud"
LIVE_CONTROL_PREFS = APP_ROOT / "bin" / "data" / "config" / "control_hub_prefs.json"
LIVE_CONSOLE_CONFIG = APP_ROOT / "bin" / "data" / "config" / "console.json"
FIXTURE_CONTROL_PREFS = REPO_ROOT / "tools" / "testdata" / "runtime_state" / "config" / "control_hub_prefs.json"
FIXTURE_CONSOLE_CONFIG = REPO_ROOT / "tools" / "testdata" / "runtime_state" / "config" / "console.json"
CONTROL_PREFS = FIXTURE_CONTROL_PREFS
CONSOLE_CONFIG = FIXTURE_CONSOLE_CONFIG
CONTROL_HUB_STATE = APP_ROOT / "src" / "ui" / "ControlMappingHubState.h"
DEFAULT_EXPECTED = REPO_ROOT / "tools" / "testdata" / "hud_layout" / "expected_widgets.json"

ALLOWED_BANDS = {"hud", "console", "workbench"}
ALLOWED_LAYOUT_TARGETS = {"projector", "controller"}
ALLOWED_WIDGET_TARGETS = {"projector", "controller", "both"}


class ContractBuildError(Exception):
    pass


def rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def parse_hud_column_count() -> int:
    try:
        text = CONTROL_HUB_STATE.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return 4
    match = re.search(r"static\s+constexpr\s+int\s+kHudColumnCount\s*=\s*(\d+)\s*;", text)
    if not match:
        return 4
    return int(match.group(1))


def normalize_hud_asset(path: Path, errors: list[str]) -> dict[str, Any] | None:
    data = load_json(path)
    ctx = rel(path)
    if not isinstance(data, dict):
        errors.append(f"{ctx}: root must be an object")
        return None

    widget_id = data.get("id")
    if not isinstance(widget_id, str) or not widget_id.startswith("hud."):
        errors.append(f"{ctx}: id must be a hud.* string")
        return None

    layer_type = data.get("type")
    if layer_type != "ui.hud.widget":
        errors.append(f"{ctx}: type must be ui.hud.widget")

    registry_prefix = data.get("registryPrefix")
    if registry_prefix != widget_id:
        errors.append(f"{ctx}: registryPrefix must match id {widget_id!r}")

    hud_widget = data.get("hudWidget")
    if not isinstance(hud_widget, dict):
        errors.append(f"{ctx}: hudWidget must be an object")
        return None

    module = hud_widget.get("module")
    if not isinstance(module, str) or not module:
        errors.append(f"{ctx}: hudWidget.module must be a non-empty string")
        module = ""

    toggle_id = hud_widget.get("toggleId", widget_id)
    if toggle_id != widget_id:
        errors.append(f"{ctx}: hudWidget.toggleId must match id {widget_id!r}")

    default_band = hud_widget.get("defaultBand", "hud")
    if default_band not in ALLOWED_BANDS:
        errors.append(f"{ctx}: hudWidget.defaultBand must be one of {sorted(ALLOWED_BANDS)}")
        default_band = "hud"

    default_column = hud_widget.get("defaultColumn", 0)
    if not is_int(default_column):
        errors.append(f"{ctx}: hudWidget.defaultColumn must be an integer")
        default_column = 0

    telemetry = hud_widget.get("telemetry", [])
    telemetry_feeds: list[str] = []
    if telemetry is None:
        telemetry = []
    if not isinstance(telemetry, list):
        errors.append(f"{ctx}: hudWidget.telemetry must be an array when present")
    else:
        for idx, feed in enumerate(telemetry):
            if not isinstance(feed, str) or not feed:
                errors.append(f"{ctx}: hudWidget.telemetry[{idx}] must be a non-empty string")
                continue
            telemetry_feeds.append(feed)

    return {
        "id": widget_id,
        "label": str(data.get("label", widget_id)),
        "module": module,
        "toggleId": str(toggle_id),
        "defaultBand": str(default_band),
        "defaultColumn": int(default_column),
        "telemetryFeeds": telemetry_feeds,
        "path": rel(path),
    }


def load_hud_assets(errors: list[str]) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    seen: set[str] = set()
    for path in sorted(HUD_ASSET_ROOT.glob("*.json")):
        entry = normalize_hud_asset(path, errors)
        if entry is None:
            continue
        if entry["id"] in seen:
            errors.append(f"{rel(path)}: duplicate HUD widget id {entry['id']!r}")
        seen.add(entry["id"])
        entries.append(entry)
    entries.sort(key=lambda item: item["id"])
    return entries


def normalize_widget_entry(
    value: Any,
    ctx: str,
    allowed_ids: set[str],
    column_count: int,
    *,
    require_visible: bool,
    allow_target: bool,
    errors: list[str],
) -> dict[str, Any] | None:
    if not isinstance(value, dict):
        errors.append(f"{ctx}: widget entry must be an object")
        return None

    widget_id = value.get("id")
    if not isinstance(widget_id, str) or not widget_id:
        errors.append(f"{ctx}.id must be a non-empty string")
        return None
    if widget_id not in allowed_ids:
        errors.append(f"{ctx}.id {widget_id!r} is not a registered HUD widget")

    column = value.get("column")
    if not is_int(column):
        errors.append(f"{ctx}.column must be an integer")
        column = 0
    elif column < 0 or column >= column_count:
        errors.append(f"{ctx}.column must be between 0 and {column_count - 1}")

    band = value.get("band", "hud")
    if not isinstance(band, str) or band not in ALLOWED_BANDS:
        errors.append(f"{ctx}.band must be one of {sorted(ALLOWED_BANDS)}")
        band = "hud"

    collapsed = value.get("collapsed", False)
    if not isinstance(collapsed, bool):
        errors.append(f"{ctx}.collapsed must be boolean")
        collapsed = False

    normalized: dict[str, Any] = {
        "id": widget_id,
        "column": int(column) if is_int(column) else 0,
        "band": band,
        "collapsed": bool(collapsed),
    }

    if require_visible:
        visible = value.get("visible")
        if not isinstance(visible, bool):
            errors.append(f"{ctx}.visible must be boolean")
            visible = True
        normalized["visible"] = bool(visible)
    elif "visible" in value:
        visible = value.get("visible")
        if not isinstance(visible, bool):
            errors.append(f"{ctx}.visible must be boolean when present")
        else:
            normalized["visible"] = visible

    if allow_target:
        target = value.get("target", "projector")
        if not isinstance(target, str) or target not in ALLOWED_WIDGET_TARGETS:
            errors.append(f"{ctx}.target must be one of {sorted(ALLOWED_WIDGET_TARGETS)}")
            target = "projector"
        normalized["target"] = target
    elif "target" in value:
        target = value.get("target")
        if not isinstance(target, str) or target not in ALLOWED_WIDGET_TARGETS:
            errors.append(f"{ctx}.target must be one of {sorted(ALLOWED_WIDGET_TARGETS)} when present")
        else:
            normalized["target"] = target

    return normalized


def normalize_widget_array(
    value: Any,
    ctx: str,
    allowed_ids: set[str],
    column_count: int,
    *,
    require_visible: bool,
    allow_target: bool,
    require_all_widgets: bool,
    errors: list[str],
) -> list[dict[str, Any]]:
    if not isinstance(value, list):
        errors.append(f"{ctx} must be an array")
        return []

    entries: list[dict[str, Any]] = []
    seen: set[str] = set()
    for idx, item in enumerate(value):
        entry = normalize_widget_entry(
            item,
            f"{ctx}[{idx}]",
            allowed_ids,
            column_count,
            require_visible=require_visible,
            allow_target=allow_target,
            errors=errors,
        )
        if entry is None:
            continue
        widget_id = entry["id"]
        if widget_id in seen:
            errors.append(f"{ctx}: duplicate widget id {widget_id!r}")
        seen.add(widget_id)
        entries.append(entry)

    if require_all_widgets and seen != allowed_ids:
        missing = sorted(allowed_ids - seen)
        extra = sorted(seen - allowed_ids)
        if missing:
            errors.append(f"{ctx}: missing HUD widgets {missing}")
        if extra:
            errors.append(f"{ctx}: unknown HUD widgets {extra}")

    entries.sort(key=lambda item: item["id"])
    return entries


def normalize_control_prefs(
    allowed_ids: set[str],
    column_count: int,
    errors: list[str],
) -> dict[str, Any]:
    data = load_json(CONTROL_PREFS)
    ctx = rel(CONTROL_PREFS)
    if not isinstance(data, dict):
        errors.append(f"{ctx}: root must be an object")
        return {}

    hud_layout_target = data.get("hudLayoutTarget", "projector")
    if not isinstance(hud_layout_target, str) or hud_layout_target not in ALLOWED_LAYOUT_TARGETS:
        errors.append(f"{ctx}.hudLayoutTarget must be one of {sorted(ALLOWED_LAYOUT_TARGETS)}")
        hud_layout_target = "projector"

    hud_visible = data.get("hudVisible", True)
    if not isinstance(hud_visible, bool):
        errors.append(f"{ctx}.hudVisible must be boolean")
        hud_visible = True

    hud_state_migrated = data.get("hudStateMigrated", False)
    if not isinstance(hud_state_migrated, bool):
        errors.append(f"{ctx}.hudStateMigrated must be boolean")
        hud_state_migrated = False

    return {
        "hudLayoutTarget": hud_layout_target,
        "hudVisible": hud_visible,
        "hudStateMigrated": hud_state_migrated,
        "hudWidgets": normalize_widget_array(
            data.get("hudWidgets"),
            f"{ctx}.hudWidgets",
            allowed_ids,
            column_count,
            require_visible=True,
            allow_target=False,
            require_all_widgets=True,
            errors=errors,
        ),
        "hudControllerWidgets": normalize_widget_array(
            data.get("hudControllerWidgets"),
            f"{ctx}.hudControllerWidgets",
            allowed_ids,
            column_count,
            require_visible=False,
            allow_target=False,
            require_all_widgets=True,
            errors=errors,
        ),
    }


def normalize_console_layouts(
    allowed_ids: set[str],
    column_count: int,
    errors: list[str],
) -> dict[str, Any]:
    data = load_json(CONSOLE_CONFIG)
    ctx = rel(CONSOLE_CONFIG)
    if not isinstance(data, dict):
        errors.append(f"{ctx}: root must be an object")
        return {}

    overlay_layouts = data.get("overlayLayouts")
    if not isinstance(overlay_layouts, dict):
        errors.append(f"{ctx}.overlayLayouts must be an object")
        return {}

    active_target = overlay_layouts.get("activeTarget", "projector")
    if not isinstance(active_target, str) or active_target not in ALLOWED_LAYOUT_TARGETS:
        errors.append(f"{ctx}.overlayLayouts.activeTarget must be one of {sorted(ALLOWED_LAYOUT_TARGETS)}")
        active_target = "projector"

    normalized_targets: dict[str, list[dict[str, Any]]] = {}
    for target in sorted(ALLOWED_LAYOUT_TARGETS):
        target_node = overlay_layouts.get(target)
        target_ctx = f"{ctx}.overlayLayouts.{target}"
        if not isinstance(target_node, dict):
            errors.append(f"{target_ctx} must be an object")
            normalized_targets[target] = []
            continue
        if "capturedAtMs" in target_node and not is_number(target_node.get("capturedAtMs")):
            errors.append(f"{target_ctx}.capturedAtMs must be numeric when present")
        normalized_targets[target] = normalize_widget_array(
            target_node.get("widgets"),
            f"{target_ctx}.widgets",
            allowed_ids,
            column_count,
            require_visible=True,
            allow_target=True,
            require_all_widgets=True,
            errors=errors,
        )

    if "lastSyncMs" in overlay_layouts and not is_number(overlay_layouts.get("lastSyncMs")):
        errors.append(f"{ctx}.overlayLayouts.lastSyncMs must be numeric when present")

    return {
        "activeTarget": active_target,
        "targets": normalized_targets,
    }


def build_contract() -> tuple[dict[str, Any], list[str]]:
    errors: list[str] = []
    column_count = parse_hud_column_count()
    hud_assets = load_hud_assets(errors)
    allowed_ids = {entry["id"] for entry in hud_assets}
    feed_ids = sorted({feed for entry in hud_assets for feed in entry["telemetryFeeds"]})

    if not allowed_ids:
        errors.append(f"{rel(HUD_ASSET_ROOT)}: no HUD widgets found")

    contract = {
        "schemaVersion": 1,
        "sourceStrategy": (
            "Static HUD contract for widget catalog, placement preferences, layout snapshots, "
            "and declared feed IDs. Dynamic feed payloads, timestamps, display geometry, and "
            "Browser selection state are intentionally excluded."
        ),
        "sources": [
            rel(HUD_ASSET_ROOT),
            rel(CONTROL_PREFS),
            rel(CONSOLE_CONFIG),
            rel(CONTROL_HUB_STATE),
            "synaptome/src/ui/HudRegistry.cpp",
            "synaptome/src/ui/HudFeedRegistry.cpp",
            "synaptome/src/ui/overlays/OverlayWidget.h",
        ],
        "allowedBands": sorted(ALLOWED_BANDS),
        "allowedLayoutTargets": sorted(ALLOWED_LAYOUT_TARGETS),
        "allowedWidgetTargets": sorted(ALLOWED_WIDGET_TARGETS),
        "hudColumnCount": column_count,
        "counts": {
            "hudWidgets": len(hud_assets),
            "feedIds": len(feed_ids),
            "controlPrefsWidgets": 0,
            "controllerPrefsWidgets": 0,
            "consoleLayoutTargets": len(ALLOWED_LAYOUT_TARGETS),
            "consoleLayoutWidgets": 0,
        },
        "hudWidgets": hud_assets,
        "feedIds": feed_ids,
        "controlHubPrefs": normalize_control_prefs(allowed_ids, column_count, errors),
        "consoleOverlayLayouts": normalize_console_layouts(allowed_ids, column_count, errors),
    }

    prefs = contract["controlHubPrefs"]
    if isinstance(prefs, dict):
        contract["counts"]["controlPrefsWidgets"] = len(prefs.get("hudWidgets", []))
        contract["counts"]["controllerPrefsWidgets"] = len(prefs.get("hudControllerWidgets", []))

    layouts = contract["consoleOverlayLayouts"]
    if isinstance(layouts, dict):
        targets = layouts.get("targets", {})
        if isinstance(targets, dict):
            contract["counts"]["consoleLayoutWidgets"] = sum(
                len(widgets) for widgets in targets.values() if isinstance(widgets, list)
            )

    return contract, errors


def dumps(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def check_contract(expected_path: Path) -> int:
    actual, errors = build_contract()
    if errors:
        print("HUD layout contract validation failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    actual_text = dumps(actual)
    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    if actual_text != expected_text:
        print(f"HUD layout snapshot is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated HUD layout contract",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1

    counts = actual["counts"]
    print(
        "HUD layout contract passed "
        f"({counts['hudWidgets']} widgets, {counts['feedIds']} feed IDs, "
        f"{counts['consoleLayoutWidgets']} console placements)"
    )
    return 0


def write_contract(expected_path: Path) -> int:
    actual, errors = build_contract()
    if errors:
        print("HUD layout contract validation failed; snapshot not written:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    expected_path.parent.mkdir(parents=True, exist_ok=True)
    expected_path.write_text(dumps(actual), encoding="utf-8")
    print(f"Wrote HUD layout snapshot: {rel(expected_path)}")
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected HUD layout snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--expected", type=Path, default=DEFAULT_EXPECTED, help="Expected snapshot path")
    parser.add_argument(
        "--live",
        action="store_true",
        help="Use live app-written control/console prefs instead of committed runtime-state fixtures",
    )
    parser.add_argument("--control-prefs", type=Path, help="Override Browser preferences path")
    parser.add_argument("--console-config", type=Path, help="Override Console config path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    global CONTROL_PREFS, CONSOLE_CONFIG
    args = parse_args(argv)
    CONTROL_PREFS = args.control_prefs or (LIVE_CONTROL_PREFS if args.live else FIXTURE_CONTROL_PREFS)
    CONSOLE_CONFIG = args.console_config or (LIVE_CONSOLE_CONFIG if args.live else FIXTURE_CONSOLE_CONFIG)
    if not CONTROL_PREFS.is_absolute():
        CONTROL_PREFS = REPO_ROOT / CONTROL_PREFS
    if not CONSOLE_CONFIG.is_absolute():
        CONSOLE_CONFIG = REPO_ROOT / CONSOLE_CONFIG
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        return write_contract(expected_path)
    return check_contract(expected_path)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

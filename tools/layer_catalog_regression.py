#!/usr/bin/env python3
"""Generate/check a golden snapshot of the Synaptome layer catalog.

This mirrors the data-level behavior of LayerLibrary::reload without launching
openFrameworks. It is a public SDK guard: layer asset JSON should resolve into
stable Browser/runtime entries, and runtime layer types should have factory
registrations.
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
DEFAULT_ROOT = APP_ROOT / "bin" / "data" / "layers"
DEFAULT_EXPECTED = REPO_ROOT / "tools" / "testdata" / "layer_catalog" / "expected_catalog.json"


def rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def parse_factory_types() -> dict[str, str]:
    of_app = APP_ROOT / "src" / "ofApp.cpp"
    text = of_app.read_text(encoding="utf-8", errors="replace")
    pattern = re.compile(
        r'registerType\(\s*"([^"]+)"\s*,\s*\[\]\(\)\s*\{\s*return\s+std::make_unique<([^>]+)>',
        re.MULTILINE,
    )
    return {match.group(1): match.group(2) for match in pattern.finditer(text)}


def clamp_opacity(value: Any, default: float = 1.0) -> float:
    raw = value if isinstance(value, (int, float)) and not isinstance(value, bool) else default
    return float(max(0.0, min(1.0, raw)))


def normalize_entry(path: Path, config: dict[str, Any], factory_types: dict[str, str]) -> dict[str, Any]:
    entry_id = config.get("id") or path.stem
    label = config.get("label") or entry_id
    category = config.get("category") or "Unsorted"
    layer_type = config.get("type") or ""
    registry_prefix = config.get("registryPrefix") or entry_id

    coverage = config.get("coverage") if isinstance(config.get("coverage"), dict) else {}
    hud_widget = config.get("hudWidget") if isinstance(config.get("hudWidget"), dict) else {}
    hud_module = hud_widget.get("module", "") if isinstance(hud_widget, dict) else ""

    kind = "runtime-layer"
    if isinstance(layer_type, str) and layer_type.startswith("fx."):
        kind = "post-effect"
    elif layer_type == "ui.hud.widget" or hud_module:
        kind = "hud-widget"

    normalized: dict[str, Any] = {
        "id": str(entry_id),
        "label": str(label),
        "category": str(category),
        "type": str(layer_type),
        "kind": kind,
        "registryPrefix": str(registry_prefix),
        "opacity": clamp_opacity(config.get("opacity", 1.0)),
        "path": rel(path),
        "factoryClass": factory_types.get(str(layer_type), ""),
        "defaultKeys": sorted(config.get("defaults", {}).keys()) if isinstance(config.get("defaults"), dict) else [],
    }

    if coverage:
        normalized["coverage"] = {
            "mode": str(coverage.get("mode", "upstream")),
            "columns": max(0, int(coverage.get("columns", 0))) if isinstance(coverage.get("columns", 0), int) else 0,
        }

    if hud_module:
        telemetry = hud_widget.get("telemetry", [])
        normalized["hud"] = {
            "module": str(hud_module),
            "toggleId": str(hud_widget.get("toggleId", registry_prefix)),
            "defaultBand": str(hud_widget.get("defaultBand", "hud")),
            "defaultColumn": int(hud_widget.get("defaultColumn", 0)) if isinstance(hud_widget.get("defaultColumn", 0), int) else 0,
            "telemetryFeeds": [item for item in telemetry if isinstance(item, str)] if isinstance(telemetry, list) else [],
        }

    return normalized


def iter_layer_files(root: Path) -> list[Path]:
    return [
        path
        for path in sorted(root.rglob("*.json"))
        if f"{path.parent.name}".lower() != "scenes" and "layers/scenes" not in path.as_posix()
    ]


def build_catalog(root: Path = DEFAULT_ROOT) -> dict[str, Any]:
    factory_types = parse_factory_types()
    entries: list[dict[str, Any]] = []
    skipped: list[dict[str, str]] = []

    for path in iter_layer_files(root):
        config = load_json(path)
        if not isinstance(config, dict):
            skipped.append({"path": rel(path), "reason": "root is not an object"})
            continue
        if "id" not in config or "type" not in config:
            skipped.append({"path": rel(path), "reason": "missing id or type"})
            continue
        entries.append(normalize_entry(path, config, factory_types))

    entries.sort(key=lambda item: (item["category"], item["label"]))

    categories: dict[str, int] = {}
    types: dict[str, int] = {}
    kinds: dict[str, int] = {}
    duplicates: list[str] = []
    seen_ids: set[str] = set()
    unresolved_runtime_types: list[str] = []

    for entry in entries:
        categories[entry["category"]] = categories.get(entry["category"], 0) + 1
        types[entry["type"]] = types.get(entry["type"], 0) + 1
        kinds[entry["kind"]] = kinds.get(entry["kind"], 0) + 1
        if entry["id"] in seen_ids:
            duplicates.append(entry["id"])
        seen_ids.add(entry["id"])
        if entry["kind"] == "runtime-layer" and not entry["factoryClass"]:
            unresolved_runtime_types.append(entry["type"])

    return {
        "schemaVersion": 1,
        "sourceStrategy": "Static mirror of LayerLibrary JSON ingestion and LayerFactory registration checks.",
        "sources": [
            rel(root),
            "synaptome/src/visuals/LayerLibrary.cpp",
            "synaptome/src/ofApp.cpp",
        ],
        "counts": {
            "entries": len(entries),
            "skipped": len(skipped),
            "categories": len(categories),
            "types": len(types),
            "factoryTypes": len(factory_types),
            "duplicateIds": len(set(duplicates)),
            "unresolvedRuntimeTypes": len(set(unresolved_runtime_types)),
        },
        "categories": dict(sorted(categories.items())),
        "kinds": dict(sorted(kinds.items())),
        "types": dict(sorted(types.items())),
        "factoryTypes": dict(sorted(factory_types.items())),
        "entries": entries,
        "skipped": skipped,
        "duplicateIds": sorted(set(duplicates)),
        "unresolvedRuntimeTypes": sorted(set(unresolved_runtime_types)),
    }


def dumps(data: dict[str, Any]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def check_catalog(expected_path: Path) -> int:
    actual = build_catalog()
    actual_text = dumps(actual)
    expected_text = expected_path.read_text(encoding="utf-8") if expected_path.exists() else ""
    if actual_text != expected_text:
        print(f"Layer catalog snapshot is stale: {rel(expected_path)}", file=sys.stderr)
        diff = difflib.unified_diff(
            expected_text.splitlines(),
            actual_text.splitlines(),
            fromfile=rel(expected_path),
            tofile="generated layer catalog",
            lineterm="",
        )
        for line in diff:
            print(line, file=sys.stderr)
        return 1

    counts = actual["counts"]
    if counts["duplicateIds"] or counts["unresolvedRuntimeTypes"]:
        print(
            f"Layer catalog snapshot matched, but catalog has {counts['duplicateIds']} duplicate IDs "
            f"and {counts['unresolvedRuntimeTypes']} unresolved runtime types",
            file=sys.stderr,
        )
        return 1

    print(
        "Layer catalog regression passed "
        f"({counts['entries']} entries, {counts['categories']} categories, {counts['factoryTypes']} factory types)"
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite the expected catalog snapshot")
    parser.add_argument("--check", action="store_true", help="Check the expected snapshot (default)")
    parser.add_argument("--expected", type=Path, default=DEFAULT_EXPECTED, help="Expected snapshot path")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    expected_path = args.expected if args.expected.is_absolute() else REPO_ROOT / args.expected
    if args.write:
        expected_path.parent.mkdir(parents=True, exist_ok=True)
        expected_path.write_text(dumps(build_catalog()), encoding="utf-8")
        print(f"Wrote layer catalog snapshot: {rel(expected_path)}")
        return 0
    return check_catalog(expected_path)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

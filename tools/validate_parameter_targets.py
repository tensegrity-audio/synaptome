#!/usr/bin/env python3
"""Validate persisted parameter targets against the generated manifest.

The default mode is advisory: unresolved references are reported, but the
command exits successfully. Use --strict when the legacy target set is clean
enough to make this a hard release gate.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = REPO_ROOT / "docs" / "contracts" / "parameter_manifest.json"
CONTRACT_FIXTURE_ROOT = REPO_ROOT / "tools" / "testdata" / "runtime_state"

TARGETISH_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z0-9_]+)+$")
CONSOLE_TARGET_RE = re.compile(r"^console\.layer([1-8])(\..+)$")

DEFAULT_CONFIG_FILES = (
    Path("synaptome/bin/data/config/audio.json"),
    Path("synaptome/bin/data/config/console.json"),
    Path("synaptome/bin/data/config/midi-map.json"),
    Path("synaptome/bin/data/config/osc-map.json"),
    Path("synaptome/bin/data/config/scene-last.json"),
    Path("synaptome/bin/data/config/control_hub_prefs.json"),
)

CONTRACT_FIXTURE_FILES = (
    Path("config/audio.json"),
    Path("config/console.json"),
    Path("config/control_hub_prefs.json"),
    Path("config/midi-map.json"),
    Path("config/osc-map.json"),
    Path("config/scene-last.json"),
)


@dataclass(frozen=True)
class Finding:
    path: Path
    pointer: str
    target: str
    source: str
    reason: str


class ManifestIndex:
    def __init__(self, manifest: dict[str, Any]) -> None:
        self.ids = {
            item["id"]
            for item in manifest.get("parameters", [])
            if isinstance(item, dict) and isinstance(item.get("id"), str)
        }
        self.console_patterns = {
            item["idPattern"]
            for item in manifest.get("consoleSlotTemplates", [])
            if isinstance(item, dict) and isinstance(item.get("idPattern"), str)
        }
        self.catalog_ids = {
            item["assetId"]
            for item in manifest.get("parameters", [])
            if isinstance(item, dict) and isinstance(item.get("assetId"), str)
        }
        self.catalog_ids.update(
            item["assetId"]
            for item in manifest.get("catalogAssetsWithoutLayerParameters", [])
            if isinstance(item, dict) and isinstance(item.get("assetId"), str)
        )

    def accepts(self, target: str) -> bool:
        if target in self.ids:
            return True
        match = CONSOLE_TARGET_RE.match(target)
        if not match:
            return False
        _, suffix = match.groups()
        return f"console.layer{{slot}}{suffix}" in self.console_patterns

    def accepts_hud_widget(self, widget_id: str) -> bool:
        normalized = "hud.telemetry" if widget_id == "telemetry" else widget_id
        return normalized in self.ids or normalized in self.catalog_ids


def rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def pointer_join(pointer: str, key: str | int) -> str:
    escaped = str(key).replace("~", "~0").replace("/", "~1")
    return f"{pointer}/{escaped}" if pointer else f"/{escaped}"


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def default_files() -> list[Path]:
    paths = [REPO_ROOT / item for item in DEFAULT_CONFIG_FILES]
    paths.extend(sorted((REPO_ROOT / "synaptome/bin/data/device_maps").glob("*.json")))
    paths.extend(sorted((REPO_ROOT / "synaptome/bin/data/layers/scenes").glob("*.json")))
    return [path for path in paths if path.exists()]


def contract_fixture_files() -> list[Path]:
    paths = [CONTRACT_FIXTURE_ROOT / item for item in CONTRACT_FIXTURE_FILES]
    paths.extend(sorted((CONTRACT_FIXTURE_ROOT / "layers/scenes").glob("*.json")))
    paths.extend(sorted((REPO_ROOT / "tools/testdata/device_maps").glob("*.json")))
    return [path for path in paths if path.exists()]


def is_parameterish(target: str) -> bool:
    return bool(TARGETISH_RE.match(target))


def add_finding(
    findings: list[Finding],
    index: ManifestIndex,
    path: Path,
    pointer: str,
    target: str,
    source: str,
) -> None:
    if not is_parameterish(target):
        return
    if index.accepts(target):
        return
    findings.append(
        Finding(
            path=path,
            pointer=pointer,
            target=target,
            source=source,
            reason="not present in parameter manifest or console slot templates",
        )
    )


def add_hud_finding(
    findings: list[Finding],
    index: ManifestIndex,
    path: Path,
    pointer: str,
    widget_id: str,
) -> None:
    if index.accepts_hud_widget(widget_id):
        return
    findings.append(
        Finding(
            path=path,
            pointer=pointer,
            target=widget_id,
            source="HUD widget id",
            reason="not present in parameter manifest or layer catalog assets",
        )
    )


def walk_mapping_targets(
    data: Any,
    index: ManifestIndex,
    path: Path,
    findings: list[Finding],
    pointer: str = "",
) -> None:
    if isinstance(data, dict):
        for key, value in data.items():
            child_pointer = pointer_join(pointer, key)
            if key == "target" and isinstance(value, str):
                add_finding(findings, index, path, child_pointer, value, "mapping target")
            else:
                walk_mapping_targets(value, index, path, findings, child_pointer)
    elif isinstance(data, list):
        for idx, value in enumerate(data):
            walk_mapping_targets(value, index, path, findings, pointer_join(pointer, idx))


def collect_scene_targets(data: Any, index: ManifestIndex, path: Path, findings: list[Finding]) -> None:
    if not isinstance(data, dict):
        return

    globals_obj = data.get("globals")
    if isinstance(globals_obj, dict):
        for key in globals_obj:
            add_finding(findings, index, path, pointer_join("/globals", key), key, "scene global parameter")

    effects_obj = data.get("effects")
    if isinstance(effects_obj, dict):
        for effect_id, params in effects_obj.items():
            if not isinstance(params, dict):
                continue
            for key in params:
                target = f"effects.{effect_id}.{key}"
                pointer = pointer_join(pointer_join("/effects", effect_id), key)
                add_finding(findings, index, path, pointer, target, "scene effect parameter")

    console_obj = data.get("console")
    slots = console_obj.get("slots") if isinstance(console_obj, dict) else None
    if isinstance(slots, list):
        for slot_index, slot in enumerate(slots):
            if not isinstance(slot, dict):
                continue
            params = slot.get("parameters")
            if not isinstance(params, dict):
                continue
            params_pointer = pointer_join(pointer_join(pointer_join("/console", "slots"), slot_index), "parameters")
            for key in params:
                add_finding(
                    findings,
                    index,
                    path,
                    pointer_join(params_pointer, key),
                    key,
                    "scene console slot parameter",
                )

    for section in ("banks", "mappings"):
        if section in data:
            walk_mapping_targets(data[section], index, path, findings, pointer_join("", section))


def collect_hud_targets(data: Any, index: ManifestIndex, path: Path, findings: list[Finding]) -> None:
    def walk(value: Any, pointer: str = "") -> None:
        if isinstance(value, dict):
            widget_id = value.get("id")
            is_widget_shape = any(key in value for key in ("band", "column", "visible", "collapsed"))
            if isinstance(widget_id, str) and is_widget_shape and (widget_id.startswith("hud.") or widget_id == "telemetry"):
                add_hud_finding(findings, index, path, pointer_join(pointer, "id"), widget_id)
            for key, child in value.items():
                walk(child, pointer_join(pointer, key))
        elif isinstance(value, list):
            for idx, child in enumerate(value):
                walk(child, pointer_join(pointer, idx))

    walk(data)


def is_scene_file(path: Path) -> bool:
    normalized = path.as_posix()
    return normalized.endswith("/config/scene-last.json") or "/layers/scenes/" in normalized


def validate_files(paths: Iterable[Path], manifest_path: Path) -> tuple[int, list[Finding]]:
    manifest = load_json(manifest_path)
    index = ManifestIndex(manifest)
    findings: list[Finding] = []
    checked = 0

    for path in paths:
        data = load_json(path)
        checked += 1
        if is_scene_file(path):
            collect_scene_targets(data, index, path, findings)
        else:
            walk_mapping_targets(data, index, path, findings)
            if path.name in {"control_hub_prefs.json", "console.json"}:
                collect_hud_targets(data, index, path, findings)

    deduped = sorted(
        set(findings),
        key=lambda item: (rel(item.path), item.pointer, item.target, item.source),
    )
    return checked, deduped


def print_report(checked: int, findings: list[Finding], strict: bool) -> None:
    mode = "strict" if strict else "advisory"
    if not findings:
        print(f"Parameter target validation passed ({checked} files checked, {mode})")
        return

    print(f"Parameter target validation found {len(findings)} unresolved references ({checked} files checked, {mode})")
    for finding in findings:
        print(
            f"- {rel(finding.path)}{finding.pointer}: {finding.target} "
            f"({finding.source}; {finding.reason})"
        )
    if not strict:
        print("Advisory mode: unresolved references are reported but do not fail the command.")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST, help="Parameter manifest path")
    parser.add_argument("--strict", action="store_true", help="Exit nonzero when unresolved references are found")
    parser.add_argument(
        "--contract-fixtures",
        action="store_true",
        help="Validate committed runtime-state fixtures instead of live app-written files",
    )
    parser.add_argument("files", nargs="*", type=Path, help="Optional JSON files to check")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    manifest_path = args.manifest if args.manifest.is_absolute() else REPO_ROOT / args.manifest
    if args.files:
        paths = [path if path.is_absolute() else REPO_ROOT / path for path in args.files]
    elif args.contract_fixtures:
        paths = contract_fixture_files()
    else:
        paths = default_files()
    checked, findings = validate_files(paths, manifest_path)
    print_report(checked, findings, args.strict)
    return 1 if args.strict and findings else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

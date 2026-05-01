#!/usr/bin/env python3
"""Validate logical device-map slots against a golden fixture."""
from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]

DEFAULT_MAPS = (
    Path("synaptome/bin/data/device_maps/MIDI Mix 0.json"),
    Path("tools/testdata/device_maps/synthetic_controller.json"),
)
DEFAULT_EXPECTED = Path("tools/testdata/device_maps/expected_logical_slots.json")

ALLOWED_ROLES = {"button", "fader", "knob", "macro", "shift"}
ALLOWED_BINDING_TYPES = {"cc", "note"}
MAX_COLUMNS = 8
MIN_SENSITIVITY = 0.1
MAX_SENSITIVITY = 4.0


class DeviceMapError(ValueError):
    pass


def rel(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def natural_column_key(item: tuple[str, Any]) -> tuple[str, int]:
    key, _ = item
    prefix = "".join(ch for ch in key if not ch.isdigit())
    digits = "".join(ch for ch in key if ch.isdigit())
    return prefix, int(digits) if digits else 0


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def validate_binding(binding: Any, pointer: str, errors: list[str]) -> tuple[str, int | None, int] | None:
    require(isinstance(binding, dict), f"{pointer} must be an object", errors)
    if not isinstance(binding, dict):
        return None

    binding_type = binding.get("type")
    number = binding.get("number")
    channel = binding.get("channel")
    require(isinstance(binding_type, str), f"{pointer}.type must be a string", errors)
    require(binding_type in ALLOWED_BINDING_TYPES, f"{pointer}.type must be one of {sorted(ALLOWED_BINDING_TYPES)}", errors)
    require(isinstance(number, int), f"{pointer}.number must be an integer", errors)
    if isinstance(number, int):
        require(0 <= number <= 127, f"{pointer}.number must be 0..127", errors)
    if channel is not None:
        require(isinstance(channel, int), f"{pointer}.channel must be an integer", errors)
        if isinstance(channel, int):
            require(1 <= channel <= 16, f"{pointer}.channel must be 1..16", errors)
    if isinstance(binding_type, str) and isinstance(number, int):
        return binding_type, channel if isinstance(channel, int) else None, number
    return None


def summarize_map(path: Path) -> dict[str, Any]:
    data = load_json(path)
    errors: list[str] = []
    require(isinstance(data, dict), f"{rel(path)} must be an object", errors)
    if not isinstance(data, dict):
        raise DeviceMapError("\n".join(errors))

    device_id = data.get("deviceId")
    name = data.get("name")
    columns = data.get("columns")
    require(isinstance(device_id, str) and bool(device_id), "deviceId must be a non-empty string", errors)
    require(isinstance(name, str) and bool(name), "name must be a non-empty string", errors)
    require(isinstance(columns, dict) and bool(columns), "columns must be a non-empty object", errors)
    if not isinstance(columns, dict):
        raise DeviceMapError("\n".join(errors))

    require(len(columns) <= MAX_COLUMNS, f"columns must contain at most {MAX_COLUMNS} entries", errors)

    role_counts: Counter[str] = Counter()
    binding_type_counts: Counter[str] = Counter()
    physical_bindings: set[tuple[str, int | None, int]] = set()
    duplicate_bindings: list[str] = []
    column_summaries: list[dict[str, Any]] = []
    total_slots = 0

    for column_id, column in sorted(columns.items(), key=natural_column_key):
        pointer = f"/columns/{column_id}"
        require(isinstance(column, dict), f"{pointer} must be an object", errors)
        if not isinstance(column, dict):
            continue
        slots = column.get("slots")
        require(isinstance(slots, list) and bool(slots), f"{pointer}/slots must be a non-empty array", errors)
        if not isinstance(slots, list):
            continue

        seen_slot_ids: set[str] = set()
        slot_summaries: list[str] = []
        for slot_index, slot in enumerate(slots):
            slot_pointer = f"{pointer}/slots/{slot_index}"
            require(isinstance(slot, dict), f"{slot_pointer} must be an object", errors)
            if not isinstance(slot, dict):
                continue
            slot_id = slot.get("id")
            role = slot.get("role")
            sensitivity = slot.get("sensitivity")
            binding = slot.get("binding")
            require(isinstance(slot_id, str) and bool(slot_id), f"{slot_pointer}/id must be a non-empty string", errors)
            if isinstance(slot_id, str):
                require(slot_id not in seen_slot_ids, f"{slot_pointer}/id duplicates another slot in {column_id}", errors)
                seen_slot_ids.add(slot_id)
            require(isinstance(role, str), f"{slot_pointer}/role must be a string", errors)
            require(role in ALLOWED_ROLES, f"{slot_pointer}/role must be one of {sorted(ALLOWED_ROLES)}", errors)
            require(isinstance(sensitivity, (int, float)), f"{slot_pointer}/sensitivity must be numeric", errors)
            if isinstance(sensitivity, (int, float)):
                require(
                    MIN_SENSITIVITY <= float(sensitivity) <= MAX_SENSITIVITY,
                    f"{slot_pointer}/sensitivity must be {MIN_SENSITIVITY}..{MAX_SENSITIVITY}",
                    errors,
                )
            binding_tuple = validate_binding(binding, f"{slot_pointer}/binding", errors)
            if binding_tuple:
                if binding_tuple in physical_bindings:
                    duplicate_bindings.append(f"{binding_tuple[0]}:{binding_tuple[1]}:{binding_tuple[2]}")
                physical_bindings.add(binding_tuple)
                binding_type_counts[binding_tuple[0]] += 1
                channel_text = "-" if binding_tuple[1] is None else str(binding_tuple[1])
                role_counts[str(role)] += 1
                slot_summaries.append(f"{slot_id}:{role}:{binding_tuple[0]}:{channel_text}:{binding_tuple[2]}")
            total_slots += 1

        column_summaries.append(
            {
                "id": column_id,
                "name": column.get("name", column_id),
                "slots": slot_summaries,
            }
        )

    if duplicate_bindings:
        errors.append(f"duplicate physical bindings: {sorted(duplicate_bindings)}")

    if errors:
        raise DeviceMapError("\n".join(errors))

    summary = {
        "path": rel(path),
        "deviceId": device_id,
        "name": name,
        "model": data.get("model", ""),
        "portHints": data.get("portHints", []),
        "columnCount": len(column_summaries),
        "slotCount": total_slots,
        "roleCounts": dict(sorted(role_counts.items())),
        "bindingTypes": dict(sorted(binding_type_counts.items())),
        "columns": column_summaries,
    }
    return summary


def generate_snapshot(paths: tuple[Path, ...]) -> dict[str, Any]:
    summaries = [summarize_map(path) for path in paths]
    return {
        "schemaVersion": 1,
        "maps": summaries,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--expected", default=str(DEFAULT_EXPECTED), help="Expected fixture path.")
    parser.add_argument("--update", action="store_true", help="Rewrite expected fixture.")
    parser.add_argument("--check", action="store_true", help="Check expected fixture. Default when --update is not used.")
    parser.add_argument("maps", nargs="*", help="Device-map files to validate.")
    args = parser.parse_args()

    map_paths = tuple((REPO_ROOT / path).resolve() for path in (args.maps or [str(path) for path in DEFAULT_MAPS]))
    expected_path = (REPO_ROOT / args.expected).resolve()

    try:
        snapshot = generate_snapshot(map_paths)
    except (OSError, json.JSONDecodeError, DeviceMapError) as exc:
        print(f"Device-map regression failed: {exc}", file=sys.stderr)
        return 1

    if args.update:
        expected_path.parent.mkdir(parents=True, exist_ok=True)
        expected_path.write_text(json.dumps(snapshot, indent=2) + "\n", encoding="utf-8")
        print(f"Wrote {rel(expected_path)}")
        return 0

    try:
        expected = load_json(expected_path)
    except OSError as exc:
        print(f"Device-map regression failed: {exc}", file=sys.stderr)
        return 1

    if snapshot != expected:
        print("Device-map regression failed: logical slot snapshot differs from expected fixture", file=sys.stderr)
        print("Run `python tools\\device_map_regression.py --update` after intentional changes.", file=sys.stderr)
        return 1

    print(
        "Device-map regression passed "
        f"({len(snapshot['maps'])} maps, "
        f"{sum(item['columnCount'] for item in snapshot['maps'])} columns, "
        f"{sum(item['slotCount'] for item in snapshot['maps'])} slots)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

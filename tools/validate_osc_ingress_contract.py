#!/usr/bin/env python3
"""Validate the consumer-owned OSC ingress and Synaptome Mesh compatibility fixture."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_FIXTURE = (
    ROOT
    / "tools"
    / "testdata"
    / "osc_ingress"
    / "synaptome_mesh_v0_1_0_consumer_capture.json"
)

TYPE_TAGS = {
    "int32": "i",
    "int64": "h",
    "float32": "f",
    "float64": "d",
    "string": "s",
    "symbol": "S",
    "blob": "b",
    "char": "c",
    "midi": "m",
    "true": "T",
    "false": "F",
    "nil": "N",
    "impulse": "I",
    "timetag": "t",
    "rgba": "r",
}
NUMERIC_TYPES = {"int32", "int64", "float32", "float64"}


def normalize_address(address: str) -> tuple[str, bool, bool]:
    prefix = "/synaptome_mesh"
    mesh_alias = address.startswith(prefix + "/")
    canonical = address[len(prefix) :] if mesh_alias else address
    route_alias = False

    hr_prefix = "/sensor/hr/"
    heart_suffix = "/heart-bpm"
    if canonical.startswith(hr_prefix) and canonical.endswith(heart_suffix):
        identity = canonical[len(hr_prefix) : -len(heart_suffix)]
        if identity and "/" not in identity:
            canonical = canonical[: -len(heart_suffix)] + "/bpm"
            route_alias = True
    return canonical, mesh_alias, route_alias


def payload_key(args: list[dict[str, Any]]) -> str:
    return json.dumps(args, separators=(",", ":"), sort_keys=True)


def validate_fixture(path: Path) -> list[str]:
    errors: list[str] = []
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"{path}: cannot read fixture: {exc}"]

    if document.get("contract") != "synaptome-mesh-osc":
        errors.append("$.contract must be 'synaptome-mesh-osc'")
    if document.get("producerVersion") != "0.1.0":
        errors.append("$.producerVersion must be '0.1.0'")
    if document.get("consumerProfile") != "synaptome-mesh-v1":
        errors.append("$.consumerProfile must be 'synaptome-mesh-v1'")

    window = document.get("dedupeWindowMs")
    if not isinstance(window, int) or isinstance(window, bool) or window < 0:
        errors.append("$.dedupeWindowMs must be a nonnegative integer")
        window = 0

    origin = document.get("origin")
    if not isinstance(origin, dict):
        errors.append("$.origin must be an object")
        origin = {}
    transport = origin.get("transport")
    endpoint = origin.get("endpoint")
    if not isinstance(transport, str) or not transport:
        errors.append("$.origin.transport must be a nonempty string")
    if not isinstance(endpoint, str) or not endpoint:
        errors.append("$.origin.endpoint must be a nonempty string")

    events = document.get("events")
    if not isinstance(events, list) or not events:
        return errors + ["$.events must be a nonempty array"]

    pending: dict[str, tuple[int, bool]] = {}
    for index, event in enumerate(events):
        where = f"$.events[{index}]"
        if not isinstance(event, dict):
            errors.append(f"{where} must be an object")
            continue
        if event.get("sequence") != index + 1:
            errors.append(f"{where}.sequence must equal {index + 1}")
        timestamp = event.get("timestampMs")
        if not isinstance(timestamp, int) or isinstance(timestamp, bool) or timestamp < 0:
            errors.append(f"{where}.timestampMs must be a nonnegative integer")
            continue
        address = event.get("address")
        if not isinstance(address, str) or not address.startswith("/"):
            errors.append(f"{where}.address must be an absolute OSC address")
            continue
        args = event.get("args")
        if not isinstance(args, list):
            errors.append(f"{where}.args must be an array")
            continue

        inferred_tags = ","
        for arg_index, arg in enumerate(args):
            arg_where = f"{where}.args[{arg_index}]"
            if not isinstance(arg, dict):
                errors.append(f"{arg_where} must be an object")
                continue
            arg_type = arg.get("type")
            if arg_type not in TYPE_TAGS:
                errors.append(f"{arg_where}.type is unsupported")
                continue
            inferred_tags += TYPE_TAGS[arg_type]
            if arg_type in NUMERIC_TYPES:
                value = arg.get("value")
                if (
                    not isinstance(value, (int, float))
                    or isinstance(value, bool)
                    or not math.isfinite(float(value))
                ):
                    errors.append(f"{arg_where}.value must be finite numeric data")

        type_tags = event.get("typeTags")
        if type_tags != inferred_tags:
            errors.append(
                f"{where}.typeTags must match typed arguments "
                f"({inferred_tags!r}, got {type_tags!r})"
            )

        canonical, mesh_alias, route_alias = normalize_address(address)
        expected = event.get("expected")
        if not isinstance(expected, dict):
            errors.append(f"{where}.expected must be an object")
            continue
        comparisons = {
            "canonicalAddress": canonical,
            "meshNamespaceAlias": mesh_alias,
            "routeAliasApplied": route_alias,
            "routableScalar": (
                len(args) == 1 and args[0].get("type") in NUMERIC_TYPES
            ),
        }
        for key, actual in comparisons.items():
            if expected.get(key) != actual:
                errors.append(
                    f"{where}.expected.{key} must be {actual!r}, "
                    f"got {expected.get(key)!r}"
                )

        key = "\n".join(
            [
                str(transport),
                str(endpoint),
                canonical,
                str(type_tags),
                payload_key(args),
            ]
        )
        suppress = False
        prior = pending.get(key)
        if prior is not None:
            prior_timestamp, prior_mesh_alias = prior
            if (
                timestamp >= prior_timestamp
                and timestamp - prior_timestamp <= window
                and mesh_alias != prior_mesh_alias
            ):
                suppress = True
                pending.pop(key)
        if not suppress:
            pending[key] = (timestamp, mesh_alias)
        if expected.get("dispatch") != (not suppress):
            errors.append(
                f"{where}.expected.dispatch must be {not suppress!r}, "
                f"got {expected.get('dispatch')!r}"
            )

    if not any(
        event.get("expected", {}).get("routableScalar") is False
        for event in events
        if isinstance(event, dict)
    ):
        errors.append("$.events must include at least one observed non-scalar payload")
    if not any(
        event.get("expected", {}).get("routeAliasApplied") is True
        for event in events
        if isinstance(event, dict)
    ):
        errors.append("$.events must exercise the Mesh heart-bpm route alias")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "fixture",
        nargs="?",
        type=Path,
        default=DEFAULT_FIXTURE,
        help="OSC ingress fixture to validate",
    )
    args = parser.parse_args()
    path = args.fixture.resolve()
    errors = validate_fixture(path)
    if errors:
        for error in errors:
            print(f"ERROR: {error}")
        return 1
    print(f"OSC ingress contract OK: {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

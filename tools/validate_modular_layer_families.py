#!/usr/bin/env python3
"""Validate shared-runtime layer families migrated to the modular workflow."""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISUALS = ROOT / "synaptome/src/visuals"
CATALOG = ROOT / "synaptome/bin/data/layers/generative"


@dataclass(frozen=True)
class Family:
    runtime_type: str
    class_name: str
    source_name: str
    assets: dict[str, str]
    deterministic: bool = True


FAMILIES = (
    Family(
        "circuitTrace",
        "CircuitTraceLayer",
        "CircuitTraceLayer.cpp",
        {
            "generative.circuitSlime": "circuitSlime",
            "generative.circuitMycelium": "circuitMycelium",
            "generative.circuitRiver": "circuitRiver",
            "generative.circuitAntTunnels": "circuitAntTunnels",
            "generative.circuitFlowField": "circuitFlowField",
        },
    ),
    Family(
        "agentField",
        "AgentFieldLayer",
        "AgentFieldLayer.cpp",
        {
            "generative.antTunnels": "antTunnels",
            "generative.slimeMold": "slimeMold",
            "generative.physarum": "physarumParticles",
        },
    ),
    Family(
        "flocking",
        "FlockingLayer",
        "FlockingLayer.cpp",
        {
            "generative.schooling": "schooling",
            "generative.murmuration": "murmuration",
        },
    ),
)


def registered_parameters(source: str) -> dict[str, str]:
    registered = {
        suffix: kind
        for kind, suffix in re.findall(
            r'registry\.add(Float|Bool|String)\(\s*prefix\s*\+\s*"\.([^"]+)"',
            source,
        )
    }
    registered.update({
        suffix: kind
        for kind, suffix in re.findall(r'\badd(Float|Bool)\("([^"]+)"', source)
    })
    for helper, suffix, kind in (
        ("visible", "visible", "Bool"),
        ("speed", "speed", "Float"),
        ("alpha", "alpha", "Float"),
    ):
        if re.search(rf"\bcommon\.{helper}\s*\(", source):
            registered[suffix] = kind
    registered.update({
        suffix: "Float"
        for suffix in re.findall(r'\bcommon\.number\(\s*"([^"]+)"', source)
    })
    registered.update({
        suffix: "Bool"
        for suffix in re.findall(r'\bcommon\.boolean\(\s*"([^"]+)"', source)
    })
    return registered


def catalogs_by_id() -> dict[str, tuple[Path, dict]]:
    result: dict[str, tuple[Path, dict]] = {}
    for path in CATALOG.glob("*.json"):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        asset_id = data.get("id") if isinstance(data, dict) else None
        if isinstance(asset_id, str):
            result[asset_id] = path, data
    return result


def validate(families: tuple[Family, ...] = FAMILIES) -> list[str]:
    errors: list[str] = []
    catalogs = catalogs_by_id()
    for family in families:
        source_path = VISUALS / family.source_name
        header_path = source_path.with_suffix(".h")
        if not source_path.exists() or not header_path.exists():
            errors.append(f"{family.runtime_type}: missing shared runtime source/header")
            continue
        source = source_path.read_text(encoding="utf-8")
        header = header_path.read_text(encoding="utf-8")
        if f"class {family.class_name} : public Layer" not in header:
            errors.append(f"{family.runtime_type}: runtime is not a Layer subclass")
        for method in ("configure", "setup", "update", "draw"):
            if f"void {family.class_name}::{method}" not in source:
                errors.append(f"{family.runtime_type}: missing {method}()")
        if family.deterministic:
            if "std::mt19937" not in header:
                errors.append(f"{family.runtime_type}: missing owned deterministic PRNG")
            if re.search(r"\b(?:ofRandom|ofRandomuf|std::rand|rand)\s*\(", source):
                errors.append(f"{family.runtime_type}: uses process-global randomness")
            if not re.search(r'rng_\s*\.\s*seed\s*\(', source):
                errors.append(f"{family.runtime_type}: reset does not apply persisted seed")
            if "requestedSeed()" not in source:
                errors.append(f"{family.runtime_type}: persisted seed contract is incomplete")

        parameters = registered_parameters(source)
        for required in ("visible", "seed", "reseed", "speed", "alpha"):
            if required not in parameters:
                errors.append(f"{family.runtime_type}: missing shared parameter {required}")

        for asset_id, model in family.assets.items():
            record = catalogs.get(asset_id)
            if record is None:
                errors.append(f"{family.runtime_type}: missing catalog asset {asset_id}")
                continue
            path, data = record
            if data.get("type") != family.runtime_type:
                errors.append(f"{path.name}: type must remain {family.runtime_type}")
            if data.get("model") != model:
                errors.append(f"{path.name}: model must remain {model}")
            if data.get("registryPrefix") != asset_id:
                errors.append(f"{path.name}: registryPrefix must remain {asset_id}")
            defaults = data.get("defaults")
            if not isinstance(defaults, dict):
                errors.append(f"{path.name}: defaults must be an object")
                continue
            missing = sorted(set(parameters) - set(defaults))
            extra = sorted(set(defaults) - set(parameters))
            if missing:
                errors.append(f"{path.name}: missing defaults: {', '.join(missing)}")
            if extra:
                errors.append(f"{path.name}: defaults without runtime parameters: {', '.join(extra)}")
            modes = data.get("modes", [])
            if family.runtime_type != "circuitTrace":
                mode_ids = [item.get("id") for item in modes if isinstance(item, dict)]
                if len(mode_ids) != 3 or len(set(mode_ids)) != 3:
                    errors.append(f"{path.name}: must declare three unique live modes")
    return errors


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--family",
        action="append",
        choices=[family.runtime_type for family in FAMILIES],
        help="Validate only this runtime family; may be repeated",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    selected = (
        tuple(family for family in FAMILIES if family.runtime_type in args.family)
        if args.family
        else FAMILIES
    )
    errors = validate(selected)
    if errors:
        for error in errors:
            print(f"modular family contract error: {error}", file=sys.stderr)
        return 1
    asset_count = sum(len(family.assets) for family in selected)
    print(
        "Modular layer family validation passed "
        f"({len(selected)} shared runtimes, {asset_count} stable assets, deterministic seeds)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

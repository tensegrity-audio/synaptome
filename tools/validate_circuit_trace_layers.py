#!/usr/bin/env python3
"""Validate the deterministic eight-direction Circuit Trace layer family."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "synaptome/src/visuals/CircuitTraceLayer.h"
SOURCE = ROOT / "synaptome/src/visuals/CircuitTraceLayer.cpp"
MOTION = ROOT / "synaptome/src/visuals/EightDirectionMotion.h"
PROJECT = ROOT / "synaptome/Synaptome.vcxproj"
APP = ROOT / "synaptome/src/ofApp.cpp"
CATALOG = ROOT / "synaptome/bin/data/layers"
ASSETS = {
    "generative.circuitSlime": "circuitSlime",
    "generative.circuitMycelium": "circuitMycelium",
    "generative.circuitRiver": "circuitRiver",
    "generative.circuitAntTunnels": "circuitAntTunnels",
    "generative.circuitFlowField": "circuitFlowField",
}
PARAMETERS = {
    "visible": "Bool", "speed": "Float", "bpmSync": "Bool",
    "bpmMultiplier": "Float", "alpha": "Float", "seed": "Float",
    "reseed": "Bool", "autoReseed": "Bool", "autoReseedEveryBeats": "Float",
    "behavior": "Float", "agentCount": "Float", "stepSize": "Float",
    "sensorDistance": "Float", "turnChance": "Float", "branchChance": "Float",
    "deposit": "Float", "decay": "Float", "diffuse": "Float",
    "tracePersistence": "Float", "traceWidth": "Float", "glow": "Float",
    "viaChance": "Float", "backgroundAlpha": "Float", "trailAlpha": "Float",
    "bgR": "Float", "bgG": "Float", "bgB": "Float", "traceR": "Float",
    "traceG": "Float", "traceB": "Float",
}


def relative(path: Path) -> str:
    return path.resolve().relative_to(ROOT).as_posix()


def catalogs() -> tuple[dict[str, tuple[Path, dict[str, Any]]], list[str]]:
    found: dict[str, tuple[Path, dict[str, Any]]] = {}
    errors: list[str] = []
    for path in CATALOG.rglob("*.json"):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        if not isinstance(data, dict) or data.get("id") not in ASSETS:
            continue
        asset_id = data["id"]
        if asset_id in found:
            errors.append(f"duplicate asset id {asset_id}")
        else:
            found[asset_id] = path, data
    return found, errors


def validate() -> list[str]:
    errors: list[str] = []
    for path in (HEADER, SOURCE, MOTION, PROJECT, APP):
        if not path.exists():
            errors.append(f"missing required file: {relative(path)}")
    if errors:
        return errors
    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")
    motion = MOTION.read_text(encoding="utf-8")
    project = PROJECT.read_text(encoding="utf-8")
    app = APP.read_text(encoding="utf-8")

    if "class CircuitTraceLayer : public Layer" not in header:
        errors.append("CircuitTraceLayer must be a modular Layer subclass")
    for name in ("configure", "setup", "update", "draw"):
        if f"void CircuitTraceLayer::{name}" not in source:
            errors.append(f"CircuitTraceLayer must implement {name}()")
    for token in ("kDirectionCount = 8", "kSteps", "quantizeVector"):
        if token not in motion:
            errors.append(f"EightDirectionMotion missing {token}")
    if "eight_direction::step(" not in source:
        errors.append("all trace position changes must use eight_direction::step()")
    if re.search(r"\b(?:ofRandom|ofRandomuf|std::rand|rand)\s*\(", source):
        errors.append("CircuitTraceLayer must not use process-global randomness")
    for token in ("std::mt19937", "debugStateSignature", "debugAgentHeadings",
                  "reseedForTest"):
        if token not in header + source:
            errors.append(f"deterministic lifecycle contract missing {token}")
    if not re.search(r"rng_\s*\.\s*seed\s*\(", source):
        errors.append("reseed must reset the owned PRNG")
    if not re.search(r"paramReseed_\s*=\s*false", source):
        errors.append("reseed action must reset after consumption")
    if "kNearestTextureFilter" not in source or "kLinearTextureFilter" in source:
        errors.append("circuit presentation must use crisp nearest-neighbor texture sampling")
    if "viaDrill" in source or "viaRing" in source:
        errors.append("circuit junctions must not render drilled plus-in-square markers")
    if "std::sqrt" not in source:
        errors.append("trace-width coverage must use circular distance rather than a square kernel")
    if "kMyceliumSeedColumns" not in source or "kMyceliumSeedRows" not in source:
        errors.append("circuit mycelium must seed multiple colonies across the full frame")
    if "stepAntTunnels" not in header + source:
        errors.append("circuit ant tunnels must own a distinct routing behavior")
    if "stepFlowField" not in header + source:
        errors.append("circuit flow field must own a distinct routing behavior")
    if "eight_direction::quantizeVector(" not in source:
        errors.append("circuit flow vectors must quantize through the shared eight-direction seam")
    if source.count("markVia(agent.position)") > 1:
        errors.append("vias must be explicit probability-driven accents, not forced branch/start stamps")

    direct_pattern = re.compile(
        r'registry\.add(Float|Bool|String)\(\s*prefix\s*\+\s*"\.([^"]+)"'
    )
    helper_pattern = re.compile(r'\badd(Float|Bool)\("([^"]+)"')
    registered = {
        suffix: kind
        for kind, suffix in direct_pattern.findall(source)
    }
    registered.update({
        suffix: kind
        for kind, suffix in helper_pattern.findall(source)
    })
    if re.search(r"\bcommon\.visible\s*\(", source):
        registered["visible"] = "Bool"
    if re.search(r"\bcommon\.speed\s*\(", source):
        registered["speed"] = "Float"
    if re.search(r"\bcommon\.alpha\s*\(", source):
        registered["alpha"] = "Float"
    registered.update({
        suffix: "Float"
        for suffix in re.findall(r'\bcommon\.number\(\s*"([^"]+)"', source)
    })
    registered.update({
        suffix: "Bool"
        for suffix in re.findall(r'\bcommon\.boolean\(\s*"([^"]+)"', source)
    })
    for suffix, kind in PARAMETERS.items():
        if registered.get(suffix) != kind:
            errors.append(f"missing {kind.lower()} parameter {suffix}")
    unknown = sorted(set(registered) - set(PARAMETERS))
    if unknown:
        errors.append(f"validator parameter contract needs updating: {', '.join(unknown)}")

    found, catalog_errors = catalogs()
    errors.extend(catalog_errors)
    missing = sorted(set(ASSETS) - set(found))
    if missing:
        errors.append(f"missing circuit catalog assets: {', '.join(missing)}")
    for asset_id, model in ASSETS.items():
        if asset_id not in found:
            continue
        path, data = found[asset_id]
        if data.get("type") != "circuitTrace":
            errors.append(f"{relative(path)} type must be circuitTrace")
        if data.get("model") != model:
            errors.append(f"{relative(path)} model must be {model}")
        if data.get("registryPrefix") != asset_id:
            errors.append(f"{relative(path)} registryPrefix must equal its asset id")
        if data.get("layerGroup") != "Circuit Organics":
            errors.append(f"{relative(path)} layerGroup must be Circuit Organics")
        defaults = data.get("defaults")
        if not isinstance(defaults, dict):
            errors.append(f"{relative(path)} defaults must be an object")
            continue
        absent = sorted(set(PARAMETERS) - set(defaults))
        if absent:
            errors.append(f"{relative(path)} missing persisted defaults: {', '.join(absent)}")
        extra = sorted(set(defaults) - set(PARAMETERS))
        if extra:
            errors.append(f"{relative(path)} has undeclared defaults: {', '.join(extra)}")
        texture_size = data.get("textureSize")
        if (
            not isinstance(texture_size, list)
            or len(texture_size) < 2
            or texture_size[0] != 256
            or texture_size[1] != 144
        ):
            errors.append(f"{relative(path)} textureSize must be 256x144 for the coarse pixel language")
        if defaults.get("viaChance") != 0.0:
            errors.append(f"{relative(path)} must default to clean traces without via symbols")

    for entry in (
        r'ClCompile Include="src\visuals\CircuitTraceLayer.cpp"',
        r'ClInclude Include="src\visuals\CircuitTraceLayer.h"',
        r'ClInclude Include="src\visuals\EightDirectionMotion.h"',
    ):
        if entry not in project:
            errors.append(f"Synaptome.vcxproj missing {entry}")
    if 'registerType("circuitTrace"' not in app:
        errors.append("LayerFactory must register circuitTrace")
    if "std::make_unique<CircuitTraceLayer>" not in app:
        errors.append("circuitTrace registration must construct CircuitTraceLayer")
    return errors


def main() -> int:
    errors = validate()
    if errors:
        for error in errors:
            print(f"circuit trace contract error: {error}", file=sys.stderr)
        return 1
    print(
        f"Circuit Trace validation passed (8 directions, deterministic reseed, "
        f"{len(PARAMETERS)} parameters, {len(ASSETS)} catalog models, "
        "crisp coarse trace renderer)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

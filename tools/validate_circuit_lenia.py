#!/usr/bin/env python3
"""Validate Circuit Lenia's shared simulation and fixed circuit presentation."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "synaptome/src/visuals/LeniaLayer.h"
SOURCE = ROOT / "synaptome/src/visuals/LeniaLayer.cpp"
CATALOG = ROOT / "synaptome/bin/data/layers/generative/circuit_lenia.json"
ORGANIC_CATALOG = ROOT / "synaptome/bin/data/layers/generative/lenia.json"
REGISTRATION = ROOT / "synaptome/src/runtime/BuiltinElements.cpp"
MAPPING_FIXTURE = (
    ROOT / "tools/testdata/circuit_lenia/default_mapping_routes.json"
)

CIRCUIT_LENIA_OSC_DEFAULTS = {
    "/control/circuit-lenia/threshold": (
        "generative.circuitLenia.circuitThreshold", [0.0, 0.55]
    ),
    "/control/circuit-lenia/levels": (
        "generative.circuitLenia.circuitLevels", [2.0, 8.0]
    ),
    "/control/circuit-lenia/trace-width": (
        "generative.circuitLenia.circuitTraceWidth", [1.0, 4.0]
    ),
    "/control/circuit-lenia/growth-center": (
        "generative.circuitLenia.growthCenter", [0.20, 0.55]
    ),
    "/control/circuit-lenia/growth-width": (
        "generative.circuitLenia.growthWidth", [0.02, 0.18]
    ),
    "/control/circuit-lenia/injection-rate": (
        "generative.circuitLenia.injectionRate", [0.0, 0.12]
    ),
    "/control/circuit-lenia/field-scale": (
        "generative.circuitLenia.fieldScale", [0.5, 4.0]
    ),
}


def registered_parameters(source: str) -> set[str]:
    result = set(re.findall(
        r'registry\.addBool\(\s*prefix\s*\+\s*"\.([^"]+)"', source
    ))
    result.update(re.findall(
        r'registerFloat\(\s*(?:\n\s*)?registry,\s*prefix\s*\+\s*"\.([^"]+)"',
        source,
    ))
    return result


def validate() -> list[str]:
    errors: list[str] = []
    for path in (
        HEADER,
        SOURCE,
        CATALOG,
        ORGANIC_CATALOG,
        REGISTRATION,
        MAPPING_FIXTURE,
    ):
        if not path.exists():
            errors.append(f"missing required file: {path.relative_to(ROOT)}")
    if errors:
        return errors

    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")
    registration = REGISTRATION.read_text(encoding="utf-8")
    data = json.loads(CATALOG.read_text(encoding="utf-8"))
    organic = json.loads(ORGANIC_CATALOG.read_text(encoding="utf-8"))
    midi_map = json.loads(MAPPING_FIXTURE.read_text(encoding="utf-8"))

    for token in (
        "Presentation::Circuit",
        "circuitBandAt",
        "circuitContourAt",
        "paramCircuitThreshold_",
        "paramCircuitLevels_",
        "paramCircuitTraceWidth_",
        "debugStateSignature",
    ):
        if token not in header + source:
            errors.append(f"Lenia circuit presentation missing {token}")
    if 'config.value("presentation"' not in source:
        errors.append("Lenia must select presentation from immutable catalog identity")
    if "texture_.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST)" not in source:
        errors.append("Circuit Lenia must preserve hard nearest-neighbor pixels")
    if "std::floor(normalized * levels)" not in source:
        errors.append("Circuit Lenia must quantize the continuous field into hard bands")
    if re.search(r"\b(?:ofRandom|ofRandomuf|std::rand|rand)\s*\(", source):
        errors.append("Lenia must not use process-global randomness")
    if "rng_.seed(activeSeed())" not in source:
        errors.append("Lenia reseed must reset its owned PRNG")
    if "/control/circuit-lenia/" in header + source:
        errors.append("Circuit Lenia OSC addresses must not be hardcoded in its renderer")
    if "MidiRouter" in header + source or "onOscMessage" in header + source:
        errors.append("Circuit Lenia must remain independent of the OSC router")

    if data.get("id") != "generative.circuitLenia":
        errors.append("Circuit Lenia asset ID must remain generative.circuitLenia")
    if data.get("type") != "lenia":
        errors.append("Circuit Lenia must reuse the proven lenia runtime")
    if data.get("registryPrefix") != "generative.circuitLenia":
        errors.append("Circuit Lenia registry prefix must equal its asset ID")
    if data.get("presentation") != "circuit":
        errors.append("Circuit Lenia catalog must select the circuit presentation")
    if organic.get("presentation") == "circuit":
        errors.append("the established Lenia asset must retain organic presentation")
    if data.get("textureSize") != [160, 90]:
        errors.append("Circuit Lenia must use the coarse 160x90 field")
    if (
        not re.search(
            r'ElementDescriptor\s*\{\s*"lenia"\s*,\s*ElementKind::Visual',
            registration,
        )
        or "std::make_unique<LeniaLayer>" not in registration
    ):
        errors.append("the shared Lenia runtime must remain factory registered")

    defaults = data.get("defaults")
    if not isinstance(defaults, dict):
        errors.append("Circuit Lenia defaults must be an object")
        return errors
    parameters = registered_parameters(source)
    missing = sorted(parameters - set(defaults))
    extra = sorted(set(defaults) - parameters)
    if missing:
        errors.append(f"Circuit Lenia missing defaults: {', '.join(missing)}")
    if extra:
        errors.append(f"Circuit Lenia undeclared defaults: {', '.join(extra)}")
    if defaults.get("circuitTraceWidth") != 1:
        errors.append("Circuit Lenia must default to one coarse simulation-pixel trace")
    if defaults.get("circuitLevels") not in (3, 4, 5):
        errors.append("Circuit Lenia must default to a restrained contour count")

    routes = {
        item.get("pattern"): item
        for item in midi_map.get("osc", [])
        if isinstance(item, dict)
    }
    profiles = {
        item.get("pattern"): item
        for item in midi_map.get("oscSources", [])
        if isinstance(item, dict)
    }
    for pattern, (target, output_range) in CIRCUIT_LENIA_OSC_DEFAULTS.items():
        route = routes.get(pattern)
        if route is None:
            errors.append(f"missing editable OSC route: {pattern}")
            continue
        if route.get("target") != target or route.get("bank") != "home":
            errors.append(f"invalid editable OSC route: {pattern}")
        profile = profiles.get(pattern)
        if profile is None:
            errors.append(f"missing editable OSC source profile: {pattern}")
            continue
        if profile.get("blend") != "absolute" or profile.get("relative") is not False:
            errors.append(f"OSC route must directly set its visible parameter: {pattern}")
        if profile.get("in") != [0.0, 1.0] or profile.get("out") != output_range:
            errors.append(f"invalid OSC input/output range: {pattern}")
    return errors


def main() -> int:
    errors = validate()
    if errors:
        for error in errors:
            print(f"circuit lenia contract error: {error}", file=sys.stderr)
        return 1
    print(
        "Circuit Lenia validation passed "
        "(shared deterministic simulation, 37 defaults, hard 160x90 contours, "
        "7 editable OSC defaults)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

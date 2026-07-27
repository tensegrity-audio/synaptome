#!/usr/bin/env python3
"""Validate the first distinct-runtime Cellular Fields migration slice."""
from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VISUALS = ROOT / "synaptome/src/visuals"
CATALOG = ROOT / "synaptome/bin/data/layers/generative"
PROJECT = ROOT / "synaptome/Synaptome.vcxproj"
FACTORY = ROOT / "synaptome/src/runtime/BuiltinElements.cpp"


@dataclass(frozen=True)
class CellularRuntime:
    asset_id: str
    runtime_type: str
    class_name: str
    source_name: str
    catalog_name: str
    builder_suffixes: tuple[str, ...]
    required_parameters: dict[str, str]
    default_aliases: frozenset[str]


RUNTIMES = (
    CellularRuntime(
        "generative.gameOfLife",
        "gameOfLife",
        "GameOfLifeLayer",
        "GameOfLifeLayer.cpp",
        "game_of_life.json",
        ("speed", "alpha", "seed", "reseed"),
        {"speed": "Float", "alpha": "Float", "seed": "Float", "reseed": "Bool"},
        frozenset({"aliveColor", "deadColor", "seedDensity"}),
    ),
    CellularRuntime(
        "generative.excitableMedia",
        "excitableMedia",
        "ExcitableMediaLayer",
        "ExcitableMediaLayer.cpp",
        "excitable_media.json",
        ("visible", "speed", "alpha", "seed", "reseed"),
        {
            "visible": "Bool",
            "speed": "Float",
            "alpha": "Float",
            "seed": "Float",
            "reseed": "Bool",
        },
        frozenset({
            "backgroundColor",
            "excitationColor",
            "refractoryColor",
            "wavefrontColor",
        }),
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
        suffix: "Float"
        for suffix in re.findall(
            r'registerFloat\(\s*registry\s*,\s*prefix\s*\+\s*"\.([^"]+)"',
            source,
        )
    })
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


def validate_runtime(runtime: CellularRuntime, project: str, factory: str) -> list[str]:
    errors: list[str] = []
    source_path = VISUALS / runtime.source_name
    header_path = source_path.with_suffix(".h")
    catalog_path = CATALOG / runtime.catalog_name
    for path in (source_path, header_path, catalog_path):
        if not path.exists():
            errors.append(f"{runtime.runtime_type}: missing {path.relative_to(ROOT).as_posix()}")
    if errors:
        return errors

    source = source_path.read_text(encoding="utf-8")
    header = header_path.read_text(encoding="utf-8")
    try:
        catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return [f"{runtime.catalog_name}: invalid JSON: {exc}"]

    if f"class {runtime.class_name} : public Layer" not in header:
        errors.append(f"{runtime.runtime_type}: must remain a distinct Layer subclass")
    for method in ("configure", "setup", "update", "draw"):
        if f"void {runtime.class_name}::{method}" not in source:
            errors.append(f"{runtime.runtime_type}: missing {method}()")

    if not re.search(r"\bLayerParameterBuilder\s+common\s*\(", source):
        errors.append(f"{runtime.runtime_type}: common builder instance is missing")
    for suffix in runtime.builder_suffixes:
        suffix_pattern = rf'prefix\s*\+\s*"\.{re.escape(suffix)}"'
        descriptor_pattern = r"common\.(?:bool|float)Descriptor\s*\("
        builder_registration = re.compile(
            rf"(?:{suffix_pattern}.{{0,320}}{descriptor_pattern}|"
            rf"{descriptor_pattern}.{{0,320}}{suffix_pattern})",
            re.DOTALL,
        )
        if not builder_registration.search(source):
            errors.append(
                f"{runtime.runtime_type}: {suffix} must use the common builder descriptor"
            )

    parameters = registered_parameters(source)
    for suffix, kind in runtime.required_parameters.items():
        if parameters.get(suffix) != kind:
            errors.append(f"{runtime.runtime_type}: missing {kind.lower()} parameter {suffix}")

    if "std::mt19937" not in header:
        errors.append(f"{runtime.runtime_type}: stochastic lifecycle needs an owned std::mt19937")
    if not re.search(r"rng_\s*\.\s*seed\s*\(", source):
        errors.append(f"{runtime.runtime_type}: reseed must reset the owned PRNG")
    if re.search(r"\b(?:ofRandom|ofRandomuf|std::rand|rand)\s*\(", source):
        errors.append(f"{runtime.runtime_type}: must not use process-global randomness")
    if "std::random_device" in source or "random_device" in source:
        errors.append(f"{runtime.runtime_type}: must not reseed from nondeterministic random_device")

    if not isinstance(catalog, dict):
        return errors + [f"{runtime.catalog_name}: catalog must be an object"]
    if catalog.get("id") != runtime.asset_id:
        errors.append(f"{runtime.catalog_name}: id must remain {runtime.asset_id}")
    if catalog.get("type") != runtime.runtime_type:
        errors.append(f"{runtime.catalog_name}: type must remain {runtime.runtime_type}")
    if catalog.get("registryPrefix") != runtime.asset_id:
        errors.append(f"{runtime.catalog_name}: registryPrefix must remain {runtime.asset_id}")
    if catalog.get("layerGroup") != "Cellular Fields":
        errors.append(f"{runtime.catalog_name}: layerGroup must be Cellular Fields")
    defaults = catalog.get("defaults")
    if not isinstance(defaults, dict):
        errors.append(f"{runtime.catalog_name}: defaults must be an object")
    else:
        # Whole-layer visibility remains slot-owned. A runtime may expose the
        # legacy control, but migration does not require duplicating that
        # ownership in catalog defaults.
        persisted_parameters = set(parameters) - {"visible"}
        missing = sorted(persisted_parameters - set(defaults))
        extra = sorted(set(defaults) - set(parameters) - runtime.default_aliases)
        if missing:
            errors.append(f"{runtime.catalog_name}: missing defaults: {', '.join(missing)}")
        if extra:
            errors.append(
                f"{runtime.catalog_name}: defaults without registered parameters: "
                f"{', '.join(extra)}"
            )

    compile_entry = rf'ClCompile Include="src\visuals\{runtime.source_name}"'
    header_entry = rf'ClInclude Include="src\visuals\{header_path.name}"'
    if compile_entry not in project:
        errors.append(f"{runtime.runtime_type}: project missing {compile_entry}")
    if header_entry not in project:
        errors.append(f"{runtime.runtime_type}: project missing {header_entry}")
    if f'registerType("{runtime.runtime_type}"' not in factory:
        errors.append(f"{runtime.runtime_type}: factory registration is missing")
    if f"std::make_unique<{runtime.class_name}>" not in factory:
        errors.append(f"{runtime.runtime_type}: factory must construct {runtime.class_name}")
    return errors


def validate() -> list[str]:
    project = PROJECT.read_text(encoding="utf-8") if PROJECT.exists() else ""
    factory = FACTORY.read_text(encoding="utf-8") if FACTORY.exists() else ""
    errors: list[str] = []
    if not project:
        errors.append("Synaptome project file is missing")
    if not factory:
        errors.append("Layer factory source is missing")
    for runtime in RUNTIMES:
        errors.extend(validate_runtime(runtime, project, factory))
    if len({runtime.runtime_type for runtime in RUNTIMES}) != len(RUNTIMES):
        errors.append("Cellular Fields runtimes must retain distinct type identities")
    return errors


def main() -> int:
    errors = validate()
    if errors:
        for error in errors:
            print(f"cellular fields contract error: {error}", file=sys.stderr)
        return 1
    print(
        "Cellular Fields validation passed "
        "(2 distinct runtimes, stable identities, complete defaults, deterministic seeds)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

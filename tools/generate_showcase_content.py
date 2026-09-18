#!/usr/bin/env python3
"""Publish the reviewed show packages as ordinary catalog content, without runtime discovery.

The package manifest remains the authoring source. Calm/energized presets are
materialized as catalog looks because the legacy package preset Apply command
requires a machine-local package activation owner. This generator does not
change that owner or pretend that source code can be loaded without a rebuild.
"""
from __future__ import annotations

import argparse
from collections import Counter
from copy import deepcopy
import json
from pathlib import Path
import sys

import element_package_v1
import layer_browser_inspection_payload
import layer_package_catalog_regression

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = ROOT / "packages/showcase"
ASSETS = ROOT / "synaptome/bin/data/layers/generative/showcase"
INSPECTION = ROOT / "synaptome/bin/data/config/layer-package-inspection.json"
NAMES = (
    "chladni_plate", "dendritic_crystal", "differential_growth",
    "elastic_lattice", "gerstner_ocean", "magnetic_dipoles",
    "phase_lattice", "sand_ripples", "strange_attractor",
    "voronoi_foam", "vortex_advection", "wave_tank",
)


def encoded(value: object) -> str:
    return json.dumps(value, indent=2, ensure_ascii=True) + "\n"


def outputs() -> dict[Path, str]:
    result: dict[Path, str] = {}
    inspection, errors = layer_browser_inspection_payload.build_payload()
    if errors:
        raise ValueError("\n".join(errors))
    for name in NAMES:
        path = PACKAGES / name / "layer.package.json"
        checked = element_package_v1.validate_package(path)
        if not checked.valid:
            raise ValueError(str(checked.diagnostics))
        package = json.loads(path.read_text())
        raw, errors = layer_package_catalog_regression.normalize_package(path)
        if errors or raw is None:
            raise ValueError("\n".join(errors))
        package_entry = layer_browser_inspection_payload.normalize_package_entry(raw)
        # Mapping suggestions work with normal assigned catalog definitions.
        # Persisted package-preset selection additionally requires activation
        # preferences, which this content-only release deliberately does not own.
        package_entry["presets"] = []
        package_entry["presetBanks"] = []
        defaults = {parameter["id"]: parameter["default"] for parameter in package["parameters"]}
        defaults.update(package["asset"].get("defaults", {}))
        for look in ("default", "calm", "energized"):
            reference = next(item for item in package["presets"] if item["presetId"] == look)
            preset = json.loads((path.parent / reference["path"]).read_text())
            asset = deepcopy(package["asset"])
            asset["schemaVersion"] = 1
            asset["packageVersion"] = package["packageVersion"]
            asset["opacity"] = 1.0
            asset["defaults"] = {**defaults, **preset["parameters"]}
            if look != "default":
                asset["id"] += "." + look
                asset["registryPrefix"] += "." + look
                asset["label"] += " / " + look.title()
            asset["description"] = package["asset"].get("description", package["description"])
            filename = name + ("" if look == "default" else "_" + look) + ".json"
            result[ASSETS / filename] = encoded(asset)
            entry = deepcopy(package_entry)
            entry["assetId"] = asset["id"]
            entry["registryPrefix"] = asset["registryPrefix"]
            entry["label"] = asset["label"]
            for control in entry["controls"]["parameters"]:
                control["default"] = asset["defaults"][control["id"]]
            inspection["entries"].append(entry)
    entries = sorted(inspection["entries"], key=lambda entry: (
        entry["category"], entry["layerGroup"], entry["label"], entry["assetId"]))
    if len({entry["assetId"] for entry in entries}) != len(entries):
        raise ValueError("duplicate show inspection identity")
    inspection["entries"] = entries
    inspection["sources"].append("packages/showcase")
    inspection["sources"].append("tools/generate_showcase_content.py")
    inspection["sourceStrategy"].append(
        "Reviewed compiled show packages expose default, calm and energized catalog looks; mapping presets remain suggestions.")
    inspection["categories"] = dict(sorted(Counter(e["category"] for e in entries).items()))
    inspection["layerGroups"] = dict(sorted(Counter(e["layerGroup"] for e in entries if e["layerGroup"]).items()))
    inspection["kinds"] = dict(sorted(Counter(e["kind"] for e in entries).items()))
    inspection["counts"] = {
        "entries": len(entries),
        "packageEntries": sum(e["kind"] == "package-layer" for e in entries),
        "generatedEntries": sum(e["kind"] == "generated-content-layer" for e in entries),
        "parameters": sum(e["controls"]["count"] for e in entries),
        "presets": sum(len(e["presets"]) for e in entries),
        "mappingPresets": sum(len(e["mappingPresets"]) for e in entries),
        "categories": len(inspection["categories"]),
        "layerGroups": len(inspection["layerGroups"]),
    }
    result[INSPECTION] = encoded(inspection)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        expected = outputs()
        stale = []
        for path, content in expected.items():
            if args.check:
                if not path.is_file() or path.read_text() != content:
                    stale.append(path.relative_to(ROOT).as_posix())
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content)
        if stale:
            raise ValueError("Stale show catalog/inspection content: " + ", ".join(stale))
    except (OSError, ValueError, StopIteration) as error:
        print("Show content generation failed: " + str(error), file=sys.stderr)
        return 1
    print(f"Show content {'check' if args.check else 'generation'} passed: 12 models, 36 catalog looks, mapping inspection.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

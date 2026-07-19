#!/usr/bin/env python3
"""Artist-facing checks for Synaptome layer packages."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import validate_layer_packages


def check_package(path: Path) -> tuple[dict[str, Any], list[str]]:
    resolved = path.resolve()
    summary, errors = validate_layer_packages.validate_package(resolved)
    report: dict[str, Any] = {
        "schemaVersion": 1,
        "command": "synaptome-layer check",
        "path": validate_layer_packages.rel(resolved),
        "valid": not errors,
        "inspectionSafe": False,
        "activationReady": False,
    }
    if summary is not None:
        report.update(
            {
                "packageId": summary.package_id,
                "assetId": summary.asset_id,
                "layerType": summary.layer_type,
                "counts": {
                    "parameters": summary.parameter_count,
                    "presets": summary.preset_count,
                    "mappingPresets": summary.mapping_preset_count,
                },
            }
        )
        try:
            package = validate_layer_packages.load_json(resolved)
        except (OSError, json.JSONDecodeError):
            package = {}
        compatibility = package.get("compatibility", {}) if isinstance(package, dict) else {}
        source = package.get("source", {}) if isinstance(package, dict) else {}
        report["inspectionSafe"] = bool(
            not errors
            and isinstance(compatibility, dict)
            and compatibility.get("manifestInspectionOnly") is True
            and compatibility.get("noHeavySetupSideEffects") is True
        )
        report["activationReady"] = bool(
            not errors
            and isinstance(source, dict)
            and source.get("strategy") == "source-registration"
            and compatibility.get("sourceRegistrationRequired") is True
        )
    report["errors"] = errors
    return report, errors


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(prog="synaptome-layer")
    subcommands = parser.add_subparsers(dest="command", required=True)
    check = subcommands.add_parser("check", help="validate one layer.package.json")
    check.add_argument("path", type=Path)
    check.add_argument("--json", action="store_true", help="emit a machine-readable report")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if args.command != "check":
        return 2
    report, errors = check_package(args.path)
    if args.json:
        print(json.dumps(report, indent=2))
    elif errors:
        print(f"FAIL {report['path']}", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
    else:
        print(f"PASS {report['packageId']} ({report['path']})")
        print(
            f"  {report['counts']['parameters']} parameters, "
            f"{report['counts']['presets']} presets, "
            f"{report['counts']['mappingPresets']} mapping presets"
        )
        print(f"  inspection-safe: {'yes' if report['inspectionSafe'] else 'no'}")
        print(f"  opt-in activation-ready: {'yes' if report['activationReady'] else 'no'}")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

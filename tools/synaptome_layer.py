#!/usr/bin/env python3
"""Artist-facing checks for Synaptome layer packages."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import layer_package_runtime_adapter
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
        source = package.get("source", {}) if isinstance(package, dict) else {}
        report["inspectionSafe"] = not errors
        report["activationReady"] = bool(
            not errors
            and isinstance(source, dict)
            and source.get("strategy") == "source-registration"
        )
    report["errors"] = errors
    return report, errors


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(prog="synaptome-layer")
    subcommands = parser.add_subparsers(dest="command", required=True)
    check = subcommands.add_parser("check", help="validate one layer.package.json")
    check.add_argument("path", type=Path)
    check.add_argument("--json", action="store_true", help="emit a machine-readable report")
    adapter = subcommands.add_parser(
        "runtime-adapter",
        help="generate or check a vetted runtime catalog adapter",
    )
    adapter.add_argument("path", type=Path, help="layer.package.json")
    adapter.add_argument("--output", type=Path, required=True)
    action = adapter.add_mutually_exclusive_group()
    action.add_argument("--check", action="store_true", help="check the generated adapter (default)")
    action.add_argument("--write", action="store_true", help="write the generated adapter")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if args.command == "runtime-adapter":
        package_path = args.path.resolve()
        output_path = args.output.resolve()
        if args.write:
            errors = layer_package_runtime_adapter.write_adapter(package_path, output_path)
            if errors:
                for error in errors:
                    print(f"runtime adapter error: {error}", file=sys.stderr)
                return 1
            print(
                "Wrote runtime adapter "
                f"{layer_package_runtime_adapter.rel(output_path)} "
                f"from {layer_package_runtime_adapter.rel(package_path)}"
            )
            return 0
        valid, errors = layer_package_runtime_adapter.check_adapter(package_path, output_path)
        if not valid:
            for error in errors:
                print(f"runtime adapter error: {error}", file=sys.stderr)
            return 1
        print(
            "Runtime adapter is current "
            f"({layer_package_runtime_adapter.rel(output_path)})"
        )
        return 0
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

#!/usr/bin/env python3
"""Run fail-fast validation stages for a layer-family authoring profile."""
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, TextIO

ROOT = Path(__file__).resolve().parents[1]
PROFILE_ROOT = ROOT / "tools" / "layer_authoring_profiles"
STAGE_KEYS = {
    "fast": "fast",
    "native": "native",
    "incremental-app": "incrementalApp",
}


@dataclass(frozen=True)
class Command:
    stage: str
    label: str
    argv: tuple[str, ...]


def profile_path(value: str) -> Path:
    direct = Path(value)
    if direct.suffix == ".json" or direct.is_absolute() or direct.parent != Path("."):
        return direct if direct.is_absolute() else ROOT / direct
    return PROFILE_ROOT / f"{value}.json"


def load_profile(path: Path) -> dict[str, Any]:
    try:
        profile = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ValueError(f"profile does not exist: {path}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"profile JSON is invalid: {path}: {exc}") from exc
    if not isinstance(profile, dict):
        raise ValueError("profile root must be an object")
    if not isinstance(profile.get("name"), str) or not profile["name"]:
        raise ValueError("profile.name must be a non-empty string")
    stages = profile.get("stages")
    if not isinstance(stages, dict):
        raise ValueError("profile.stages must be an object")
    return profile


def find_msbuild() -> str:
    on_path = shutil.which("MSBuild.exe") or shutil.which("msbuild")
    if on_path:
        return on_path
    if sys.platform == "win32":
        vswhere = Path(
            r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
        )
        if vswhere.exists():
            result = subprocess.run(
                [
                    str(vswhere),
                    "-latest",
                    "-products",
                    "*",
                    "-requires",
                    "Microsoft.Component.MSBuild",
                    "-find",
                    r"MSBuild\**\Bin\MSBuild.exe",
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            for line in result.stdout.splitlines():
                candidate = Path(line.strip())
                if candidate.is_file():
                    return str(candidate)
    return "MSBuild.exe"


def commands_for(profile: dict[str, Any], selected_stages: list[str]) -> list[Command]:
    result: list[Command] = []
    stages = profile["stages"]
    for stage in selected_stages:
        key = STAGE_KEYS[stage]
        raw_commands = stages.get(key, [])
        if not isinstance(raw_commands, list):
            raise ValueError(f"profile.stages.{key} must be an array")
        for index, raw in enumerate(raw_commands):
            if not isinstance(raw, dict):
                raise ValueError(f"profile.stages.{key}[{index}] must be an object")
            label = raw.get("label")
            argv = raw.get("command")
            if not isinstance(label, str) or not label:
                raise ValueError(f"profile.stages.{key}[{index}].label must be non-empty")
            if not isinstance(argv, list) or not argv or not all(
                isinstance(item, str) and item for item in argv
            ):
                raise ValueError(
                    f"profile.stages.{key}[{index}].command must be a non-empty string array"
                )
            expanded = tuple(
                sys.executable if item == "{python}" else
                find_msbuild() if item == "{msbuild}" else item
                for item in argv
            )
            result.append(Command(stage, label, expanded))
    return result


def run_commands(
    commands: list[Command],
    *,
    cwd: Path = ROOT,
    keep_going: bool = False,
    dry_run: bool = False,
    output: TextIO = sys.stdout,
) -> int:
    total_start = time.perf_counter()
    failures = 0
    for index, command in enumerate(commands, start=1):
        rendered = subprocess.list2cmdline(command.argv)
        print(
            f"[layer-authoring] {index}/{len(commands)} "
            f"{command.stage.upper()} {command.label}",
            file=output,
        )
        print(f"[layer-authoring]   {rendered}", file=output)
        if dry_run:
            continue
        started = time.perf_counter()
        try:
            completed = subprocess.run(command.argv, cwd=cwd, check=False)
            return_code = completed.returncode
        except FileNotFoundError:
            return_code = 127
            print(
                f"[layer-authoring] FAIL executable not found: {command.argv[0]}",
                file=output,
            )
        elapsed = time.perf_counter() - started
        if return_code != 0:
            failures += 1
            print(
                f"[layer-authoring] FAIL {command.label} "
                f"({elapsed:.2f}s, exit {return_code})",
                file=output,
            )
            if not keep_going:
                print("[layer-authoring] stopped at first failure", file=output)
                return return_code
        else:
            print(f"[layer-authoring] PASS {command.label} ({elapsed:.2f}s)", file=output)
    elapsed = time.perf_counter() - total_start
    if failures:
        print(f"[layer-authoring] {failures} stage command(s) failed ({elapsed:.2f}s)", file=output)
        return 1
    print(f"[layer-authoring] PASS all selected stages ({elapsed:.2f}s)", file=output)
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "profile",
        nargs="?",
        default="circuit-trace",
        help="Profile name or repository-relative JSON path (default: circuit-trace)",
    )
    parser.add_argument(
        "--native",
        action="store_true",
        help="After fast checks, build the headless native target and run its selected tests",
    )
    parser.add_argument(
        "--incremental-app",
        action="store_true",
        help="After fast checks, request an incremental Synaptome app build",
    )
    parser.add_argument("--keep-going", action="store_true", help="Run later commands after a failure")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without running them")
    parser.add_argument("--list", action="store_true", help="List available profiles and exit")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if args.list:
        for path in sorted(PROFILE_ROOT.glob("*.json")):
            print(path.stem)
        return 0
    try:
        profile = load_profile(profile_path(args.profile))
        selected = ["fast"]
        if args.native:
            selected.append("native")
        if args.incremental_app:
            selected.append("incremental-app")
        commands = commands_for(profile, selected)
    except ValueError as exc:
        print(f"layer authoring profile error: {exc}", file=sys.stderr)
        return 2
    if not commands:
        print("layer authoring profile error: selected stages contain no commands", file=sys.stderr)
        return 2
    print(f"[layer-authoring] profile: {profile['name']}")
    return run_commands(
        commands,
        keep_going=args.keep_going,
        dry_run=args.dry_run,
    )


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

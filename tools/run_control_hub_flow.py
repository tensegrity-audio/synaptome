#!/usr/bin/env python3
"""Run the Browser flow harness and surface artifact summaries."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the Control Panel (Browser) flow harness")
    parser.add_argument(
        "--runner",
        choices=("native", "platformio"),
        default="native",
        help="Harness runner to execute (default: native)",
    )
    parser.add_argument(
        "--env",
        default="browser_flow",
        help="PlatformIO environment to execute when --runner=platformio (default: browser_flow)",
    )
    parser.add_argument(
        "--platformio",
        default="platformio",
        help="PlatformIO executable when --runner=platformio (default: platformio)",
    )
    parser.add_argument(
        "--native-exe",
        default="synaptome/tests/BrowserFlowTest/x64/Release/BrowserFlowTest.exe",
        help="Native BrowserFlowTest executable when --runner=native",
    )
    parser.add_argument(
        "--artifact",
        default="tests/artifacts/browser_flow.json",
        help="Artifact path emitted by the harness",
    )
    parser.add_argument(
        "--no-summary",
        action="store_true",
        help="Skip printing the artifact summary",
    )
    parser.add_argument(
        "--dual-screen-phase2",
        action="store_true",
        help="Validate the dual-screen Phase 2 harness artifact after tests complete",
    )
    parser.add_argument(
        "--midi-artifact",
        default="tests/artifacts/midi_mapping_flow.json",
        help="MIDI mapping flow artifact emitted by the harness",
    )
    parser.add_argument(
        "--osc-artifact",
        default="tests/artifacts/osc_ingest_flow.json",
        help="OSC ingest flow artifact emitted by the harness",
    )
    parser.add_argument(
        "--webcam-artifact",
        default="tests/artifacts/webcam_replay_flow.json",
        help="Webcam replay flow artifact emitted by the harness",
    )
    return parser.parse_args()


def run_platformio(pio: str, env: str) -> None:
    cmd = [pio, "test", "-e", env]
    env_vars = dict(os.environ)
    print("[browser_flow]", " ".join(cmd), flush=True)
    result = subprocess.run(cmd, env=env_vars)
    if result.returncode != 0:
        raise SystemExit(result.returncode)


def run_native(executable: Path) -> None:
    if not executable.exists():
        raise SystemExit(
            "Native Browser Flow executable missing: "
            f"{executable}\n"
            "Build it first, for example:\n"
            "  msbuild synaptome\\tests\\BrowserFlowTest\\BrowserFlowTest.vcxproj "
            "/p:Configuration=Release /p:Platform=x64"
        )
    cmd = [str(executable)]
    print("[browser_flow]", " ".join(cmd), flush=True)
    result = subprocess.run(cmd, env=dict(os.environ))
    if result.returncode != 0:
        raise SystemExit(result.returncode)


def load_artifact(path: Path) -> dict:
    if not path.exists():
        raise SystemExit(f"Harness succeeded but artifact missing: {path}")
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Failed to parse artifact {path}: {exc}") from exc


def summarize_artifact(path: Path) -> None:
    payload = load_artifact(path)
    events = payload.get("events", [])
    parameters = payload.get("parameters", {})
    print(f"[browser_flow] events={len(events)} parameters={len(parameters)}")
    for pid, meta in parameters.items():
        history = meta.get("history", [])
        last_value = history[-1] if history else "?"
        print(f"  - {pid}: last={last_value} samples={len(history)}")


def summarize_dual_screen_phase2(path: Path) -> None:
    payload = load_artifact(path)
    block = payload.get("dual_screen_phase2") or payload
    iterations = int(block.get("iterations", 0))
    if iterations < 200:
        raise SystemExit(
            f"Dual-screen Phase 2 harness expected at least 200 iterations, got {iterations}"
        )
    widget_id = block.get("widgetId", "?")
    last_target = block.get("lastTarget", "?")
    projector = block.get("projector", {})
    controller = block.get("controller", {})
    route_events = block.get("routeEvents", {})
    proj_route = int(route_events.get("projector", 0))
    ctrl_route = int(route_events.get("controller", 0))
    required_fields = []
    for label, node in (("projector", projector), ("controller", controller)):
        if "column" not in node:
            required_fields.append(f"{label}.column")
        if "target" not in node:
            required_fields.append(f"{label}.target")
    if required_fields:
        raise SystemExit(
            "Dual-screen Phase 2 artifact missing fields: " + ", ".join(required_fields)
        )
    if last_target not in {"projector", "controller"}:
        raise SystemExit(f"Dual-screen Phase 2 artifact has invalid lastTarget: {last_target}")
    if proj_route == 0 or ctrl_route == 0:
        raise SystemExit(
            "Dual-screen Phase 2 artifact missing overlay route telemetry coverage"
        )
    print(
        "[dual_screen_phase2]"
        f" widget={widget_id} iterations={iterations} lastTarget={last_target}"
    )
    print(
        f"  - projector: column={projector['column']} captures={projector.get('captures', 0)}"
    )
    print(
        f"  - controller: column={controller['column']} captures={controller.get('captures', 0)}"
    )
    print(
        f"  - routeEvents: projector={proj_route} controller={ctrl_route}"
    )

def summarize_midi_mapping_flow(path: Path) -> None:
    payload = load_artifact(path)
    mapping = payload.get("mapping", {})
    parameter = mapping.get("parameter", "?")
    cc = mapping.get("cc", "?")
    value = mapping.get("finalValue", "?")
    devices = payload.get("devices", [])
    print(
        "[midi_mapping_flow]"
        f" devices={len(devices)} parameter={parameter} cc={cc} finalValue={value}"
    )


def summarize_osc_ingest_flow(path: Path) -> None:
    payload = load_artifact(path)
    samples = payload.get("samples", [])
    latest = samples[-1] if samples else {}
    param_id = latest.get("parameterId", "?")
    value = latest.get("value", "?")
    coverage = payload.get("coverage", {})
    required = ["matrixBioamp", "matrixMic", "deck", "hostMic"]
    missing = [name for name in required if not coverage.get(name)]
    covered = [name for name, enabled in coverage.items() if enabled]
    coverage_summary = ",".join(covered) if covered else "none"
    if missing:
        raise SystemExit(
            "[osc_ingest_flow] coverage missing: " + ", ".join(missing)
        )
    print(
        "[osc_ingest_flow]"
        f" samples={len(samples)} lastParameter={param_id} lastValue={value} coverage={coverage_summary}"
    )


def summarize_webcam_flow(path: Path) -> None:
    payload = load_artifact(path)
    devices = payload.get("devices", [])
    setups = payload.get("setups", [])
    frames = payload.get("frames", [])
    close_count = payload.get("closeCount", 0)
    failed = sum(1 for entry in setups if not entry.get("success"))
    print(
        "[webcam_flow]"
        f" devices={len(devices)} setups={len(setups)} failures={failed} closes={close_count} frames={len(frames)}"
    )


def main() -> None:
    args = parse_args()
    if args.runner == "platformio":
        run_platformio(args.platformio, args.env)
    else:
        run_native(Path(args.native_exe))
    if not args.no_summary:
        summarize_artifact(Path(args.artifact))
        summarize_midi_mapping_flow(Path(args.midi_artifact))
        summarize_osc_ingest_flow(Path(args.osc_artifact))
        summarize_webcam_flow(Path(args.webcam_artifact))
    if args.dual_screen_phase2:
        summarize_dual_screen_phase2(Path("tests/artifacts/dual_screen_phase2.json"))


if __name__ == "__main__":
    main()

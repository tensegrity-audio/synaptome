#!/usr/bin/env python3
"""
Utility to print the webcam device inventory captured by VideoGrabberLayer.

The runtime now writes `synaptome/bin/data/logs/webcam_devices.json`
whenever devices are enumerated. Run this script to see the indices, labels,
and active/armed state so external webcams can be selected confidently.
"""

import argparse
import json
import sys
from pathlib import Path


def format_device(entry: dict) -> str:
    status = []
    if entry.get("available"):
        status.append("available")
    else:
        status.append("offline")
    if entry.get("active"):
        status.append("active")
    if entry.get("armed"):
        status.append("armed")
    status_str = ", ".join(status)
    return f"[{entry.get('index', '?')}] id={entry.get('id', '?')} '{entry.get('label', '')}' ({status_str})"


def main() -> int:
    default_path = Path("synaptome/bin/data/logs/webcam_devices.json")
    parser = argparse.ArgumentParser(description="Print recorded webcam device inventory.")
    parser.add_argument(
        "--path",
        type=Path,
        default=default_path,
        help=f"Path to webcam_devices.json (default: {default_path})",
    )
    args = parser.parse_args()

    if not args.path.exists():
        print(f"No webcam inventory file found at {args.path}. Launch the app and refresh devices first.", file=sys.stderr)
        return 1

    try:
        data = json.loads(args.path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        print(f"Failed to parse JSON from {args.path}: {exc}", file=sys.stderr)
        return 1

    devices = data.get("devices", [])
    if not devices:
        print("No webcam devices recorded in the inventory file.")
        return 0

    timestamp = data.get("timestampMs")
    if timestamp is not None:
        print(f"Captured at {timestamp} ms")
    print(f"Devices found ({len(devices)}):")
    for entry in devices:
        print(" ", format_device(entry))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

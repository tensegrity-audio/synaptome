#!/usr/bin/env python3
"""Validate preferences-v1 examples and focused rejection fixtures."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "docs/schemas/preferences.schema.json"
VALID = (
    ROOT / "docs/examples/preferences_example.json",
    ROOT / "tools/testdata/preferences/canonical_v1.json",
)
INVALID = ROOT / "tools/testdata/preferences/invalid_cases.json"
LEGACY = ROOT / "tools/testdata/preferences/legacy_control_hub.json"


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))

STABLE_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,126}$")
TOP_KEYS = {"schemaVersion", "browser", "hud", "hotkeys", "packages", "mappings"}


def validate(document) -> list[str]:
    errors: list[str] = []
    if not isinstance(document, dict):
        return ["document must be an object"]
    if document.get("schemaVersion") != 1:
        errors.append("schemaVersion must equal 1")
    if set(document) - TOP_KEYS:
        errors.append("unknown top-level keys")
    browser = document.get("browser")
    if browser is not None:
        if not isinstance(browser, dict):
            errors.append("browser must be an object")
        else:
            ratio = browser.get("treeWidthRatio")
            if ratio is not None and (
                isinstance(ratio, bool)
                or not isinstance(ratio, (int, float))
                or not 0.1 <= ratio <= 0.5
            ):
                errors.append("invalid treeWidthRatio")
            visible = browser.get("visibleColumns", {})
            if isinstance(visible, dict) and visible.get("name") is False:
                errors.append("name column cannot be hidden")
    mappings = document.get("mappings")
    if mappings is not None:
        if not isinstance(mappings, dict):
            errors.append("mappings must be an object")
        else:
            active = mappings.get("activeBank")
            if active is not None and (
                not isinstance(active, str)
                or (active and STABLE_ID.fullmatch(active) is None)
            ):
                errors.append("invalid activeBank")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.parse_args()
    schema = load(SCHEMA)
    failures: list[str] = []
    if schema.get("properties", {}).get("schemaVersion", {}).get("const") != 1:
        failures.append("schema does not freeze schemaVersion 1")
    for path in VALID:
        errors = validate(load(path))
        if errors:
            failures.append(f"{path}: {errors[0]}")
    for case in load(INVALID):
        if not validate(case["document"]):
            failures.append(f"invalid case accepted: {case['id']}")
    legacy = load(LEGACY)
    if "schemaVersion" in legacy or legacy.get("selectedCategory") != "Scenes":
        failures.append("legacy Control Hub compatibility fixture is malformed")
    if failures:
        for failure in failures:
            print(f"[preferences-contract] FAIL {failure}")
        return 1
    print(
        "[preferences-contract] PASS canonical example/fixture, "
        f"{len(load(INVALID))} rejection cases, and legacy reader fixture"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

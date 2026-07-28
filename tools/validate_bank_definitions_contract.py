#!/usr/bin/env python3
"""Validate the operator bank-definitions-v1 fixtures."""
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "docs/schemas/bank_definitions.schema.json"
VALID = (
    ROOT / "docs/examples/bank_definitions_example.json",
    ROOT / "tools/testdata/bank_definitions/canonical_v1.json",
    ROOT / "tools/testdata/bank_definitions/canonical_empty_v1.json",
)
INVALID = ROOT / "tools/testdata/bank_definitions/invalid_cases.json"
STABLE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,126}$")


def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def errors(document) -> list[str]:
    found: list[str] = []
    if not isinstance(document, dict):
        return ["document must be an object"]
    if set(document) - {"schemaVersion", "globalBanks"}:
        found.append("unknown top-level key")
    if document.get("schemaVersion") != 1:
        found.append("schemaVersion must equal 1")
    banks = document.get("globalBanks")
    if not isinstance(banks, list):
        return found + ["globalBanks must be an array"]
    ids: set[str] = set()
    parents: dict[str, str] = {}
    for bank in banks:
        if not isinstance(bank, dict) or STABLE.fullmatch(str(bank.get("id", ""))) is None:
            found.append("invalid bank id")
            continue
        bank_id = bank["id"]
        if bank_id in ids:
            found.append("duplicate bank id")
        ids.add(bank_id)
        if "parent" in bank:
            parents[bank_id] = bank["parent"]
        for control in bank.get("controls", []):
            if not isinstance(control, dict) or (
                "target" not in control and "modifier" not in control
            ):
                found.append("orphan control")
    for bank_id, parent in parents.items():
        if parent not in ids:
            found.append("missing parent")
        seen: set[str] = set()
        current = bank_id
        while current in parents:
            if current in seen:
                found.append("inheritance cycle")
                break
            seen.add(current)
            current = parents[current]
    return found


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.parse_args()
    schema = load(SCHEMA)
    failures: list[str] = []
    if schema.get("properties", {}).get("schemaVersion", {}).get("const") != 1:
        failures.append("schema does not freeze v1")
    for path in VALID:
        issue = errors(load(path))
        if issue:
            failures.append(f"{path}: {issue[0]}")
    cases = load(INVALID)
    for case in cases:
        if not errors(case["document"]):
            failures.append(f"invalid case accepted: {case['id']}")
    if failures:
        for failure in failures:
            print(f"[bank-definitions-contract] FAIL {failure}")
        return 1
    print(
        "[bank-definitions-contract] PASS canonical + empty + "
        f"{len(cases)} rejection cases"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

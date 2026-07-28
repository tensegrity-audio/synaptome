#!/usr/bin/env python3
"""Validate the canonical mapping-bank route snapshot and legacy boundary."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

try:
    import jsonschema  # type: ignore
except ImportError:  # pragma: no cover - environment/setup failure
    jsonschema = None


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "docs/schemas/mapping_bank_route_snapshot.schema.json"
CANONICAL = ROOT / "tools/testdata/mapping_bank/canonical_v1.json"
LEGACY = ROOT / "tools/testdata/mapping_bank/legacy_unversioned.json"
PUBLIC_INTERCHANGE = ROOT / "docs/examples/midi_bank_example.json"
DOCUMENT_SOURCE = ROOT / "synaptome/src/io/MappingBankDocument.cpp"
ROUTER_SOURCE = ROOT / "synaptome/src/io/MidiRouter.cpp"


class ContractError(RuntimeError):
    pass


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ContractError(f"{path}: invalid JSON: {exc}") from exc


def validate() -> None:
    schema = load_json(SCHEMA)
    canonical = load_json(CANONICAL)
    legacy = load_json(LEGACY)
    interchange = load_json(PUBLIC_INTERCHANGE)
    if jsonschema is not None:
        validator = jsonschema.Draft7Validator(schema)
        errors = sorted(validator.iter_errors(canonical), key=lambda error: list(error.path))
        if errors:
            rendered = "; ".join(
                f"{'/'.join(str(part) for part in error.path) or '$'}: {error.message}"
                for error in errors
            )
            raise ContractError(f"{CANONICAL}: {rendered}")

    if set(canonical) != {
        "schemaVersion",
        "cc",
        "buttons",
        "oscSources",
        "osc",
    }:
        raise ContractError("canonical v1 fixture does not contain exactly the route snapshot fields")
    if canonical["schemaVersion"] != 1:
        raise ContractError("canonical mapping-bank fixture must be v1")
    if schema.get("properties", {}).get("schemaVersion", {}).get("const") != 1:
        raise ContractError("mapping-bank schema must pin schemaVersion to 1")
    for section in ("cc", "buttons", "oscSources", "osc"):
        entries = canonical.get(section)
        if not isinstance(entries, list):
            raise ContractError(f"canonical mapping-bank {section} must be an array")
        if any(not isinstance(entry, dict) for entry in entries):
            raise ContractError(f"canonical mapping-bank {section} entries must be objects")
    for index, entry in enumerate(canonical["cc"]):
        if not isinstance(entry.get("num"), int) or not 0 <= entry["num"] <= 127:
            raise ContractError(f"canonical cc[{index}].num must be 0..127")
        if not isinstance(entry.get("target"), str) or not entry["target"]:
            raise ContractError(f"canonical cc[{index}].target must be a non-empty string")
        out_range = entry.get("out")
        if not (
            isinstance(out_range, list)
            and len(out_range) == 2
            and all(isinstance(value, (int, float)) for value in out_range)
        ):
            raise ContractError(f"canonical cc[{index}].out must be a numeric pair")
    for section in ("oscSources", "osc"):
        for index, entry in enumerate(canonical[section]):
            if not isinstance(entry.get("pattern"), str) or not entry["pattern"].startswith("/"):
                raise ContractError(
                    f"canonical {section}[{index}].pattern must begin with '/'"
                )
    if "schemaVersion" in legacy:
        raise ContractError("legacy compatibility fixture must remain unversioned")
    if not isinstance(legacy.get("cc"), list):
        raise ContractError("legacy fixture must exercise the actual MidiRouter cc vocabulary")
    if set(("version", "bank", "mappings")) - set(interchange):
        raise ContractError("public MIDI interchange fixture changed shape unexpectedly")
    if any(key in interchange for key in ("schemaVersion", "cc", "buttons", "oscSources", "osc")):
        raise ContractError("public MIDI interchange fixture was conflated with runtime mapping-bank v1")

    document_source = DOCUMENT_SOURCE.read_text(encoding="utf-8")
    router_source = ROUTER_SOURCE.read_text(encoding="utf-8", errors="replace")
    for snippet in (
        'contains("schemaVersion")',
        "MappingBankDocumentKind::LegacyUnversioned",
        "MappingBankDocumentKind::CurrentV1",
        "MappingBankDocumentError::UnsupportedFutureVersion",
    ):
        if snippet not in document_source:
            raise ContractError(f"mapping document reader missing source contract: {snippet}")
    for snippet in (
        'doc["schemaVersion"]',
        'doc["cc"] = ofJson::array()',
        'doc["buttons"] = ofJson::array()',
        'doc["oscSources"] = ofJson::array()',
        'doc["osc"] = ofJson::array()',
        "refusing backup downgrade",
    ):
        if snippet not in router_source:
            raise ContractError(f"mapping writer/reader missing source contract: {snippet}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate committed mapping-bank contract files.",
    )
    parser.parse_args()
    try:
        validate()
    except ContractError as exc:
        print(f"Mapping-bank contract error: {exc}", file=sys.stderr)
        return 1
    print("Mapping-bank v1 contract passed (canonical + legacy + interchange separation)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

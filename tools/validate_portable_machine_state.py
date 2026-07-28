#!/usr/bin/env python3
"""Reject machine-local device selectors and paths in portable JSON artifacts."""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
CLASSIFICATION = (
    ROOT / "tools/testdata/portable_machine_state/classification.json"
)
CASES = ROOT / "tools/testdata/portable_machine_state/cases.json"

WINDOWS_ABSOLUTE_RE = re.compile(r"^[A-Za-z]:[\\/]")
class ContractError(RuntimeError):
    pass


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"{path}: failed to load JSON: {exc}") from exc


def is_absolute_local_path(value: str, json_path: str) -> bool:
    leaf = json_path.rsplit(".", 1)[-1].lower()
    lowered_path = json_path.lower()
    osc_address = value.startswith("/") and (
        "address" in leaf
        or (
            "mapping" in lowered_path
            and (".pattern" in lowered_path or ".patterns[" in lowered_path)
        )
    )
    return bool(
        WINDOWS_ABSOLUTE_RE.match(value)
        or value.startswith("\\")
        or value.lower().startswith("file://")
        or (value.startswith("/") and not osc_address)
    )


def iter_nodes(value: Any, path: str = "$") -> Iterable[tuple[str, Any]]:
    yield path, value
    if isinstance(value, dict):
        for key, child in value.items():
            yield from iter_nodes(child, f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from iter_nodes(child, f"{path}[{index}]")


def find_selector_paths(value: Any, path: str = "$") -> list[str]:
    paths: list[str] = []
    if isinstance(value, dict):
        if value.get("type") == "media.webcam":
            for node_path, node in iter_nodes(value, path):
                if not isinstance(node, dict):
                    continue
                for selector in ("deviceName", "deviceIndex"):
                    if selector in node:
                        paths.append(f"{node_path}.{selector}")
        for key, child in value.items():
            paths.extend(find_selector_paths(child, f"{path}.{key}"))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            paths.extend(find_selector_paths(child, f"{path}[{index}]"))
    return sorted(set(paths))


def validate_portable_document(document: Any) -> list[str]:
    errors = [
        f"{path} contains an absolute local filesystem path"
        for path, node in iter_nodes(document)
        if isinstance(node, str) and is_absolute_local_path(node, path)
    ]
    errors.extend(
        f"{path} selects physical webcam hardware in a portable layer asset"
        for path in find_selector_paths(document)
    )
    return errors


def expand_portable_files(catalog: dict[str, Any]) -> list[Path]:
    files: set[Path] = set()
    for pattern in catalog.get("portableGlobs", []):
        if not isinstance(pattern, str):
            raise ContractError("portableGlobs entries must be strings")
        files.update(path for path in ROOT.glob(pattern) if path.is_file())
    return sorted(files)


def validate_catalog(catalog: Any) -> dict[str, Any]:
    if not isinstance(catalog, dict) or catalog.get("schemaVersion") != 1:
        raise ContractError("classification catalog must use schemaVersion 1")
    portable = catalog.get("portableGlobs")
    local = catalog.get("localGlobs")
    legacy = catalog.get("legacyCompatibilityFiles")
    if not isinstance(portable, list) or not portable:
        raise ContractError("classification catalog needs portableGlobs")
    if not isinstance(local, list) or not local:
        raise ContractError("classification catalog needs localGlobs")
    if not isinstance(legacy, list):
        raise ContractError(
            "classification catalog needs legacyCompatibilityFiles"
        )
    rules = catalog.get("rules")
    if not isinstance(rules, dict):
        raise ContractError("classification catalog needs rules")
    if rules.get("portableWebcamSelectors") != [
        "deviceName",
        "deviceIndex",
    ]:
        raise ContractError(
            "portableWebcamSelectors must lock deviceName and deviceIndex"
        )
    if rules.get("absoluteLocalPaths") is not True:
        raise ContractError("absoluteLocalPaths rule must be enabled")

    portable_files = set(expand_portable_files(catalog))
    if not portable_files:
        raise ContractError("portableGlobs did not match any files")
    for field in ("localGlobs", "legacyCompatibilityFiles"):
        for pattern in catalog[field]:
            if not isinstance(pattern, str):
                raise ContractError(f"{field} entries must be strings")
            matches = {path for path in ROOT.glob(pattern) if path.is_file()}
            if not matches:
                raise ContractError(
                    f"{field} pattern did not match a file: {pattern}"
                )
            overlap = matches & portable_files
            if overlap:
                rendered = ", ".join(
                    path.relative_to(ROOT).as_posix() for path in sorted(overlap)
                )
                raise ContractError(
                    f"{field} overlaps portable classification: {rendered}"
                )
    return catalog


def validate_cases(source: Any) -> int:
    if not isinstance(source, dict) or source.get("schemaVersion") != 1:
        raise ContractError("case fixture must use schemaVersion 1")
    cases = source.get("cases")
    if not isinstance(cases, list) or not cases:
        raise ContractError("case fixture must contain cases")
    seen: set[str] = set()
    for case in cases:
        if not isinstance(case, dict):
            raise ContractError("each case must be an object")
        case_id = case.get("id")
        if not isinstance(case_id, str) or not case_id or case_id in seen:
            raise ContractError("case IDs must be unique non-empty strings")
        seen.add(case_id)
        classification = case.get("classification")
        expected = case.get("expected")
        if classification not in {
            "portable",
            "local",
            "legacy-compatibility",
        }:
            raise ContractError(f"{case_id}: invalid classification")
        if expected not in {"accept", "reject"}:
            raise ContractError(f"{case_id}: invalid expected result")
        errors = (
            validate_portable_document(case.get("document"))
            if classification == "portable"
            else []
        )
        actual = "reject" if errors else "accept"
        if actual != expected:
            raise ContractError(
                f"{case_id}: expected {expected}, got {actual}: {errors}"
            )
    return len(cases)


def run() -> tuple[int, int]:
    catalog = validate_catalog(load_json(CLASSIFICATION))
    case_count = validate_cases(load_json(CASES))
    failures: list[str] = []
    portable_files = expand_portable_files(catalog)
    for path in portable_files:
        document = load_json(path)
        for error in validate_portable_document(document):
            failures.append(
                f"{path.relative_to(ROOT).as_posix()}: {error}"
            )
    if failures:
        raise ContractError(
            "portable machine-state violations:\n- " + "\n- ".join(failures)
        )
    return len(portable_files), case_count


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Validate that portable Scene/layer/package JSON excludes "
            "machine-local webcam selectors and filesystem paths."
        )
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate the committed classification, cases, and artifacts.",
    )
    parser.parse_args()
    try:
        file_count, case_count = run()
    except ContractError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    print(
        "Portable machine-state promotion gate passed "
        f"({file_count} portable JSON files + {case_count} classification cases)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

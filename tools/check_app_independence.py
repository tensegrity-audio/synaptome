#!/usr/bin/env python3
"""Audit app source/build surfaces for firmware implementation dependencies."""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

REPO_ROOT = Path(__file__).resolve().parents[1]
APP_ROOT = REPO_ROOT / "synaptome"

TEXT_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".filters",
    ".h",
    ".hh",
    ".hpp",
    ".inl",
    ".ipp",
    ".m",
    ".mm",
    ".props",
    ".sln",
    ".vcxproj",
}
SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".inl",
    ".ipp",
    ".m",
    ".mm",
}
TEXT_FILENAMES = {
    "addons.make",
    "config.make",
    "Makefile",
}
EXCLUDED_PARTS = {
    ".git",
    ".vs",
    "bin",
    "debug",
    "release",
    "obj",
    "x64",
}

BANNED_PATTERNS = (
    ("firmware directory tensegrity_gateway", re.compile(r"tensegrity_gateway", re.IGNORECASE)),
    ("firmware directory tensegrity_matrixportal", re.compile(r"tensegrity_matrixportal", re.IGNORECASE)),
    ("firmware directory tensegrity_cyberdeck", re.compile(r"tensegrity_cyberdeck", re.IGNORECASE)),
    ("firmware directory tensegrity_cardputer", re.compile(r"tensegrity_cardputer", re.IGNORECASE)),
    ("firmware directory tensegrity_memento", re.compile(r"tensegrity_memento", re.IGNORECASE)),
    ("legacy firmware payload header", re.compile(r"payload\.h", re.IGNORECASE)),
)

COMMON_INCLUDE_PATTERN = re.compile(
    r"(?:Tensegrity[\\/]+tensegrity[\\/]+common|(?:\.\.[\\/]+){3,}common)(?:[\\/;\s<\"]|$)",
    re.IGNORECASE,
)
BUILD_ROOT_ALIAS_PATTERNS = (
    (
        "hardcoded openFrameworks app junction path",
        re.compile(
            r"Documents[\\/]+openFrameworks[\\/]+apps[\\/]+myApps[\\/]+synaptome",
            re.IGNORECASE,
        ),
    ),
    (
        "hardcoded repo app root path",
        re.compile(r"Documents[\\/]+Tensegrity[\\/]+tensegrity[\\/]+synaptome", re.IGNORECASE),
    ),
    (
        "hardcoded repo thirdparty path",
        re.compile(r"Tensegrity[\\/]+tensegrity[\\/]+thirdparty", re.IGNORECASE),
    ),
    (
        "relative path from openFrameworks app to repo root",
        re.compile(r"(?:\.\.[\\/]+){4}Tensegrity[\\/]+tensegrity", re.IGNORECASE),
    ),
)
INCLUDE_DIRECTIVE_PATTERN = re.compile(r"^\s*#\s*include\s+([<\"])([^>\"]+)[>\"]")


@dataclass(frozen=True)
class Finding:
    path: Path
    line_number: int
    reason: str
    line: str


def _is_text_candidate(path: Path) -> bool:
    return path.suffix.lower() in TEXT_SUFFIXES or path.name in TEXT_FILENAMES


def _iter_candidates(root: Path) -> Iterable[Path]:
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        rel_parts = path.relative_to(root).parts
        if any(part.lower() in EXCLUDED_PARTS for part in rel_parts):
            continue
        if _is_text_candidate(path):
            yield path


def _is_source_candidate(path: Path) -> bool:
    return path.suffix.lower() in SOURCE_SUFFIXES


def _root_common_headers() -> set[str]:
    common_dir = REPO_ROOT / "common"
    if not common_dir.exists():
        return set()
    return {path.name.lower() for path in common_dir.iterdir() if path.is_file() and path.suffix.lower() in {".h", ".hpp"}}


def _resolves_under(path: Path, parent: Path) -> bool:
    try:
        path.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def _scan_file(path: Path, root_common_headers: set[str]) -> tuple[list[Finding], list[Finding], list[Finding], list[Finding]]:
    errors: list[Finding] = []
    warnings: list[Finding] = []
    root_common_includes: list[Finding] = []
    build_root_aliases: list[Finding] = []
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except OSError as exc:
        errors.append(Finding(path, 0, "could not read file", str(exc)))
        return errors, warnings, root_common_includes, build_root_aliases

    for line_number, line in enumerate(text.splitlines(), start=1):
        for reason, pattern in BANNED_PATTERNS:
            if pattern.search(line):
                errors.append(Finding(path, line_number, reason, line.strip()))
        for reason, pattern in BUILD_ROOT_ALIAS_PATTERNS:
            if pattern.search(line):
                build_root_aliases.append(Finding(path, line_number, reason, line.strip()))
        if COMMON_INCLUDE_PATTERN.search(line):
            warnings.append(Finding(path, line_number, "shared repo-root common include", line.strip()))

        if _is_source_candidate(path):
            include_match = INCLUDE_DIRECTIVE_PATTERN.match(line)
            if include_match:
                include_text = include_match.group(2).strip().replace("\\", "/")
                include_name = Path(include_text).name.lower()
                if include_name in root_common_headers:
                    local_path = (path.parent / include_text).resolve()
                    if not local_path.exists() or not _resolves_under(local_path, APP_ROOT):
                        root_common_includes.append(
                            Finding(path, line_number, "source include may resolve to repo-root common", line.strip())
                        )
    return errors, warnings, root_common_includes, build_root_aliases


def _rel(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


def _print_findings(label: str, findings: Sequence[Finding]) -> None:
    if not findings:
        return
    print(label)
    for finding in findings:
        location = _rel(finding.path)
        if finding.line_number:
            location = f"{location}:{finding.line_number}"
        line = finding.line
        if len(line) > 160:
            line = f"{line[:157]}..."
        print(f"- {location}: {finding.reason}: {line}")


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        default=str(APP_ROOT),
        help="App root to scan. Defaults to synaptome.",
    )
    args = parser.parse_args(argv)

    root = Path(args.root)
    if not root.is_absolute():
        root = (REPO_ROOT / root).resolve()
    if not root.exists():
        print(f"error: app root does not exist: {root}", file=sys.stderr)
        return 1

    errors: list[Finding] = []
    warnings: list[Finding] = []
    root_common_includes: list[Finding] = []
    build_root_aliases: list[Finding] = []
    root_common_headers = _root_common_headers()
    scanned = 0
    for path in _iter_candidates(root):
        scanned += 1
        file_errors, file_warnings, file_root_common, file_build_roots = _scan_file(path, root_common_headers)
        errors.extend(file_errors)
        warnings.extend(file_warnings)
        root_common_includes.extend(file_root_common)
        build_root_aliases.extend(file_build_roots)

    print(f"App independence audit scanned {scanned} files under {_rel(root)}")
    _print_findings("Firmware implementation references:", errors)
    _print_findings("Source includes requiring repo-root common:", root_common_includes)
    _print_findings("Hardcoded or mixed build-root aliases:", build_root_aliases)
    _print_findings("Shared-contract include warnings:", warnings)

    if errors or root_common_includes or build_root_aliases:
        print(
            "error: app source/build surfaces reference firmware implementation details, repo-root common headers, "
            "or hardcoded mixed build roots"
        )
        return 1

    print("No firmware implementation references found.")
    print("No app source includes require repo-root common headers.")
    print("No hardcoded mixed build-root aliases found.")
    if warnings:
        print("Warnings do not fail the audit, but G4 remains partial until build proof resolves them.")
    return 0


if __name__ == "__main__":  # pragma: no cover - CLI entry
    sys.exit(main(sys.argv[1:]))

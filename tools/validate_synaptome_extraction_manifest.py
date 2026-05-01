#!/usr/bin/env python3
"""Validate the draft public Synaptome extraction manifest."""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from fnmatch import fnmatchcase
from pathlib import Path
from typing import Any, Iterable, Sequence

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = REPO_ROOT / "docs" / "contracts" / "synaptome_public_extraction_manifest.json"


@dataclass(frozen=True)
class PatternRule:
    pattern: str
    reason: str
    required: bool = False


@dataclass(frozen=True)
class Classification:
    included: list[str]
    excluded: list[str]
    unclassified: list[str]
    review: dict[str, list[str]]
    included_never_public: dict[str, list[str]]
    unmatched_required: list[PatternRule]


def _load_manifest(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)
    if manifest.get("schema_version") != 1:
        raise ValueError(f"unsupported manifest schema_version: {manifest.get('schema_version')!r}")
    return manifest


def _rules(items: Iterable[dict[str, Any]]) -> list[PatternRule]:
    rules: list[PatternRule] = []
    for item in items:
        pattern = str(item.get("pattern", "")).replace("\\", "/").strip()
        reason = str(item.get("reason", "")).strip()
        if not pattern:
            raise ValueError("manifest rule is missing a pattern")
        if not reason:
            raise ValueError(f"manifest rule is missing a reason: {pattern}")
        rules.append(PatternRule(pattern=pattern, reason=reason, required=bool(item.get("required", False))))
    return rules


def _git_files() -> list[str]:
    completed = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=REPO_ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )
    return sorted(path for path in completed.stdout.decode("utf-8").split("\0") if path)


def _matches(path: str, pattern: str) -> bool:
    pattern = pattern.replace("\\", "/")
    if pattern == "*":
        return True
    if pattern.endswith("/**"):
        prefix = pattern[:-3]
        return path == prefix or path.startswith(f"{prefix}/")
    return fnmatchcase(path, pattern)


def _matching_rules(path: str, rules: Sequence[PatternRule]) -> list[PatternRule]:
    return [rule for rule in rules if _matches(path, rule.pattern)]


def _first_match(path: str, rules: Sequence[PatternRule]) -> PatternRule | None:
    for rule in rules:
        if _matches(path, rule.pattern):
            return rule
    return None


def _classify(manifest: dict[str, Any], files: Sequence[str]) -> Classification:
    include_rules = _rules(manifest.get("include", []))
    exclude_rules = _rules(manifest.get("exclude", []))
    review_rules = _rules(manifest.get("review", []))
    never_public_rules = [
        PatternRule(pattern=str(pattern).replace("\\", "/"), reason="never_public", required=False)
        for pattern in manifest.get("never_public", [])
    ]

    included: list[str] = []
    excluded: list[str] = []
    unclassified: list[str] = []
    review: dict[str, list[str]] = {rule.pattern: [] for rule in review_rules}
    included_never_public: dict[str, list[str]] = {rule.pattern: [] for rule in never_public_rules}

    for path in files:
        include_match = _first_match(path, include_rules)
        if include_match is not None:
            included.append(path)
            for rule in review_rules:
                if _matches(path, rule.pattern):
                    review[rule.pattern].append(path)
            for rule in never_public_rules:
                if _matches(path, rule.pattern):
                    included_never_public[rule.pattern].append(path)
            continue

        if _first_match(path, exclude_rules) is not None:
            excluded.append(path)
        else:
            unclassified.append(path)

    unmatched_required = [
        rule for rule in include_rules if rule.required and not any(_matches(path, rule.pattern) for path in files)
    ]

    return Classification(
        included=included,
        excluded=excluded,
        unclassified=unclassified,
        review={pattern: paths for pattern, paths in review.items() if paths},
        included_never_public={pattern: paths for pattern, paths in included_never_public.items() if paths},
        unmatched_required=unmatched_required,
    )


def _print_list(title: str, paths: Sequence[str], limit: int) -> None:
    if not paths:
        return
    print(title)
    for path in paths[:limit]:
        print(f"  - {path}")
    remaining = len(paths) - limit
    if remaining > 0:
        print(f"  ... {remaining} more")


def _print_grouped(title: str, grouped: dict[str, list[str]], limit: int) -> None:
    if not grouped:
        return
    print(title)
    for pattern, paths in grouped.items():
        print(f"  {pattern}: {len(paths)}")
        for path in paths[:limit]:
            print(f"    - {path}")
        remaining = len(paths) - limit
        if remaining > 0:
            print(f"    ... {remaining} more")


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", default=str(DEFAULT_MANIFEST), help="Manifest JSON path")
    parser.add_argument("--check", action="store_true", help="Fail on manifest errors")
    parser.add_argument("--strict-review", action="store_true", help="Treat review matches as failures")
    parser.add_argument("--list-included", action="store_true", help="Print included files")
    parser.add_argument("--list-excluded", action="store_true", help="Print excluded files")
    parser.add_argument("--limit", type=int, default=25, help="Maximum files to print per list/group")
    args = parser.parse_args(argv)

    manifest_path = Path(args.manifest)
    if not manifest_path.is_absolute():
        manifest_path = (REPO_ROOT / manifest_path).resolve()

    try:
        manifest = _load_manifest(manifest_path)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"error: could not load extraction manifest: {exc}", file=sys.stderr)
        return 1

    try:
        files = _git_files()
    except subprocess.CalledProcessError as exc:
        print(f"error: git ls-files failed: {exc}", file=sys.stderr)
        return 1

    result = _classify(manifest, files)

    print("Synaptome public extraction manifest")
    print(f"manifest: {manifest_path.relative_to(REPO_ROOT).as_posix()}")
    print(f"status: {manifest.get('status', 'unknown')}")
    print(f"tracked files: {len(files)}")
    print(f"included candidates: {len(result.included)}")
    print(f"excluded/staging files: {len(result.excluded)}")
    print(f"review-gated included files: {sum(len(paths) for paths in result.review.values())}")
    print(f"unclassified files: {len(result.unclassified)}")

    if result.unmatched_required:
        print("Required include patterns with no tracked matches:")
        for rule in result.unmatched_required:
            print(f"  - {rule.pattern}: {rule.reason}")

    _print_grouped("Review-gated included files:", result.review, args.limit)
    _print_grouped("ERROR: never-public patterns included by allowlist:", result.included_never_public, args.limit)
    _print_list("Unclassified files:", result.unclassified, args.limit)

    if args.list_included:
        _print_list("Included files:", result.included, args.limit)
    if args.list_excluded:
        _print_list("Excluded/staging files:", result.excluded, args.limit)

    failed = bool(result.unmatched_required or result.included_never_public or result.unclassified)
    if args.strict_review and result.review:
        failed = True
    if args.check and failed:
        print("error: extraction manifest is not ready for dry-run copy")
        return 1

    if result.review:
        print("note: review-gated files are allowed in draft mode; resolve before public publication.")
    print("Extraction manifest check completed.")
    return 0


if __name__ == "__main__":  # pragma: no cover - CLI entry
    sys.exit(main(sys.argv[1:]))

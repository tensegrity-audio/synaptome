#!/usr/bin/env python3
"""Shared layer package discovery policy for Synaptome tooling."""
from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

REPO_ROOT = Path(__file__).resolve().parents[1]
APP_ROOT = REPO_ROOT / "synaptome"
PACKAGE_MANIFEST_NAME = "layer.package.json"
LEGACY_LAYER_ASSET_ROOT = APP_ROOT / "bin" / "data" / "layers"
FIXTURE_PACKAGE_ROOT = REPO_ROOT / "docs" / "examples" / "layer_packages"
RUNTIME_PACKAGE_ROOT = APP_ROOT / "bin" / "data" / "layer_packages"


@dataclass(frozen=True)
class PackageRoot:
    path: Path
    role: str
    purpose: str


DISCOVERY_ROOTS: tuple[PackageRoot, ...] = (
    PackageRoot(
        FIXTURE_PACKAGE_ROOT,
        "fixture",
        "Tracked example packages used by docs, schemas, and regression tests.",
    ),
    PackageRoot(
        RUNTIME_PACKAGE_ROOT,
        "runtime",
        "Future install/discovery root for package folders dropped into the app data tree.",
    ),
)


def rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def resolve_path(raw: str | Path) -> Path:
    path = Path(raw)
    return path if path.is_absolute() else (REPO_ROOT / path).resolve()


def fixture_roots() -> tuple[Path, ...]:
    return (FIXTURE_PACKAGE_ROOT,)


def runtime_roots() -> tuple[Path, ...]:
    return (RUNTIME_PACKAGE_ROOT,)


def policy_roots() -> tuple[Path, ...]:
    return tuple(root.path for root in DISCOVERY_ROOTS)


def roots_from_args(raw_roots: Iterable[str | Path] | None, default_roots: Iterable[Path]) -> tuple[Path, ...]:
    roots = tuple(resolve_path(raw) for raw in (raw_roots or ()))
    return roots or tuple(default_roots)


def iter_package_paths(roots: Iterable[Path]) -> list[Path]:
    paths: list[Path] = []
    for root in roots:
        resolved = root.resolve()
        if resolved.is_file() and resolved.name == PACKAGE_MANIFEST_NAME:
            paths.append(resolved)
        elif resolved.is_dir():
            paths.extend(path.resolve() for path in resolved.rglob(PACKAGE_MANIFEST_NAME))
    return sorted(set(paths))


def root_report(roots: Iterable[Path] = policy_roots()) -> dict[str, object]:
    requested = tuple(root.resolve() for root in roots)
    known_by_path = {root.path.resolve(): root for root in DISCOVERY_ROOTS}
    entries: list[dict[str, object]] = []
    for root in requested:
        policy = known_by_path.get(root)
        package_paths = iter_package_paths((root,))
        entries.append(
            {
                "path": rel(root),
                "role": policy.role if policy else "custom",
                "purpose": policy.purpose if policy else "Caller-supplied package discovery root.",
                "exists": root.exists(),
                "packageCount": len(package_paths),
                "packages": [rel(path) for path in package_paths],
            }
        )
    return {
        "schemaVersion": 1,
        "manifestName": PACKAGE_MANIFEST_NAME,
        "legacyLayerAssetRoot": rel(LEGACY_LAYER_ASSET_ROOT),
        "coexistenceRule": (
            "Layer packages are discovered from package roots. Legacy *.json layer assets remain "
            "the runtime catalog source until package loading is explicitly enabled; duplicate "
            "asset IDs between the two surfaces are treated as conflicts by combined checks."
        ),
        "roots": entries,
    }


def dumps(data: dict[str, object]) -> str:
    return json.dumps(data, indent=2, sort_keys=False) + "\n"


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("roots", nargs="*", help="Optional custom roots or layer.package.json files")
    parser.add_argument("--fixtures-only", action="store_true", help="Report only docs/examples package fixtures")
    parser.add_argument("--runtime-only", action="store_true", help="Report only the app runtime package root")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    if args.fixtures_only and args.runtime_only:
        print("error: choose at most one of --fixtures-only or --runtime-only", file=sys.stderr)
        return 1
    if args.fixtures_only:
        roots = fixture_roots()
    elif args.runtime_only:
        roots = runtime_roots()
    else:
        roots = roots_from_args(args.roots, policy_roots())
    print(dumps(root_report(roots)), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

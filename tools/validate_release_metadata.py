#!/usr/bin/env python3
"""Validate public release naming and version metadata."""
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
SEMVER_RE = re.compile(r"^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$")


def fail(message: str) -> int:
    print(f"error: {message}", file=sys.stderr)
    return 1


def main() -> int:
    version_path = REPO_ROOT / "VERSION"
    if not version_path.exists():
        return fail("missing root VERSION file")

    version = version_path.read_text(encoding="utf-8").strip()
    if not SEMVER_RE.fullmatch(version):
        return fail(f"VERSION must be MAJOR.MINOR.PATCH without leading v: {version!r}")

    release_policy = (REPO_ROOT / "docs" / "release_policy.md").read_text(encoding="utf-8")
    required_snippets = [
        "**Synaptome** is the product",
        "`synaptome` is the repository",
        f"`v{version}`",
        f"Synaptome v{version}",
    ]
    missing = [snippet for snippet in required_snippets if snippet not in release_policy]
    if missing:
        return fail("release policy missing expected version/naming snippets: " + ", ".join(missing))

    readme = (REPO_ROOT / "README.md").read_text(encoding="utf-8")
    if f"Synaptome v{version}" not in readme:
        return fail(f"README must mention Synaptome v{version}")

    print(f"Release metadata valid: Synaptome v{version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

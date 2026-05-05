#!/usr/bin/env python3
"""Validate Synaptome's built-in OSC route glob patterns.

OscParameterRouter accepts glob-style route patterns. This check protects the
mesh-style sensor routes from drifting back to regex-looking `.*` segments.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OFAPP_PATH = ROOT / "synaptome" / "src" / "ofApp.cpp"


EXPECTED_MATCHES = {
    "/sensor/hr/*/bpm": ("/sensor/hr/0x0301/bpm",),
    "/sensor/matrix/*/mic-level": ("/sensor/matrix/0x0101/mic-level",),
    "/sensor/matrix/*/mic-peak": ("/sensor/matrix/0x0101/mic-peak",),
    "/sensor/deck/*/deck-scene": ("/sensor/deck/0x0201/deck-scene",),
    "/sensor/deck/*/scene": ("/sensor/deck/0x0201/scene",),
}

REGEX_LOOKING_NEGATIVES = (
    ("/sensor/matrix/.*/mic-level", "/sensor/matrix/0x0101/mic-level"),
    ("/sensor/deck/.*/scene", "/sensor/deck/0x0201/deck-scene"),
)


def router_glob_to_regex(pattern: str) -> re.Pattern[str]:
    """Mirror OscParameterRouter::makeRegex for route fixture checks."""
    expr = "^"
    for char in pattern:
        if char == "*":
            expr += ".*"
        elif char == "?":
            expr += "."
        elif char.isalnum() or char in {"/", "_", "-"}:
            expr += char
        else:
            expr += "\\" + char
    expr += "$"
    return re.compile(expr)


def setup_osc_routes_body(source: str) -> str:
    marker = "void ofApp::setupOscRoutes()"
    start = source.find(marker)
    if start < 0:
        raise ValueError("ofApp::setupOscRoutes() not found")
    brace = source.find("{", start)
    if brace < 0:
        raise ValueError("ofApp::setupOscRoutes() body not found")

    depth = 0
    for idx in range(brace, len(source)):
        char = source[idx]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:idx]
    raise ValueError("ofApp::setupOscRoutes() body is unterminated")


def extract_route_literals(source: str) -> list[str]:
    body = setup_osc_routes_body(source)
    patterns: list[str] = []
    patterns.extend(re.findall(r'cfg\.pattern\s*=\s*"([^"]+)"', body))
    patterns.extend(re.findall(r'oscRouter\.addBoolRoute\(\s*"([^"]+)"', body))
    return patterns


def collect_errors(patterns: list[str]) -> list[str]:
    errors: list[str] = []
    unique_patterns = set(patterns)

    for pattern in patterns:
        if ".*" in pattern:
            errors.append(f"{pattern}: use glob `*`, not regex-looking `.*`")

    for pattern, addresses in EXPECTED_MATCHES.items():
        if pattern not in unique_patterns:
            errors.append(f"missing built-in OSC route pattern: {pattern}")
            continue
        matcher = router_glob_to_regex(pattern)
        for address in addresses:
            if matcher.fullmatch(address) is None:
                errors.append(f"{pattern} did not match expected mesh address {address}")

    for pattern, address in REGEX_LOOKING_NEGATIVES:
        matcher = router_glob_to_regex(pattern)
        if matcher.fullmatch(address) is not None:
            errors.append(f"negative fixture unexpectedly matched: {pattern} -> {address}")

    return errors


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source",
        type=Path,
        default=OFAPP_PATH,
        help="Path to ofApp.cpp. Defaults to the Synaptome source tree.",
    )
    args = parser.parse_args(argv)

    source_path = args.source.resolve()
    try:
        source = source_path.read_text(encoding="utf-8")
        patterns = extract_route_literals(source)
    except OSError as exc:
        print(f"error: failed to read {source_path}: {exc}")
        return 1
    except ValueError as exc:
        print(f"error: {exc}")
        return 1

    errors = collect_errors(patterns)
    if errors:
        print("OSC route pattern validation failed:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"Built-in OSC route globs validated ({len(patterns)} patterns)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

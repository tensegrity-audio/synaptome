#!/usr/bin/env python3
"""Validate the Signal Control -> Synaptome receive contract fixture."""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

from validate_configs import validate_osc_input


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_FIXTURE = ROOT / "tools" / "testdata" / "signal_control" / "expected_receive_contract.json"
DEFAULT_CONFIG = ROOT / "synaptome" / "bin" / "data" / "config" / "osc-input.json"
EXPECTED_METRICS = ("mic-level", "mic-peak", "mic-bass", "mic-mids", "mic-highs")
SOURCE_RE = re.compile(r"^[a-z0-9_-]+$")
HOST_ROUTE_RE = re.compile(r"^/sensor/host/([^/]+)/([^/]+)$")


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def number_pair(value: Any) -> bool:
    return (
        isinstance(value, list)
        and len(value) == 2
        and all(isinstance(item, (int, float)) and not isinstance(item, bool) for item in value)
    )


def validate_fixture(data: Any) -> list[str]:
    errors: list[str] = []
    require(isinstance(data, dict), "fixture must be an object", errors)
    if not isinstance(data, dict):
        return errors

    require(data.get("contract") == "synaptome-signal-control-receive", "contract name is unexpected", errors)
    require(data.get("version") == 1, "version must be 1", errors)

    transport = data.get("transport")
    require(isinstance(transport, dict), "transport must be an object", errors)
    if isinstance(transport, dict):
        require(transport.get("kind") == "udp", "transport.kind must be udp", errors)
        require(transport.get("defaultHost") == "127.0.0.1", "transport.defaultHost must be 127.0.0.1", errors)
        require(transport.get("defaultPort") == 9000, "transport.defaultPort must be 9000", errors)

    input_modes = data.get("inputModes")
    require(isinstance(input_modes, dict), "inputModes must be an object", errors)
    if isinstance(input_modes, dict):
        require(input_modes.get("default") == "directSerial", "inputModes.default must be directSerial", errors)
        require(input_modes.get("supported") == ["directSerial", "routerUdp"], "inputModes.supported is unexpected", errors)

    source_rules = data.get("sourceRules")
    require(isinstance(source_rules, dict), "sourceRules must be an object", errors)
    default_source = None
    if isinstance(source_rules, dict):
        default_source = source_rules.get("defaultSource")
        require(isinstance(default_source, str) and SOURCE_RE.match(default_source) is not None, "sourceRules.defaultSource must be a valid source id", errors)
        require(source_rules.get("pattern") == SOURCE_RE.pattern, "sourceRules.pattern is unexpected", errors)

    freshness = data.get("freshnessMs")
    require(isinstance(freshness, dict), "freshnessMs must be an object", errors)
    if isinstance(freshness, dict):
        require(freshness.get("telemetryFreshMax") == 2000, "telemetryFreshMax must be 2000", errors)
        require(freshness.get("waveformFreshMax") == 1000, "waveformFreshMax must be 1000", errors)
        require(freshness.get("staleAfter") == 5000, "staleAfter must be 5000", errors)

    telemetry = data.get("telemetry")
    require(isinstance(telemetry, list), "telemetry must be an array", errors)
    seen_metrics: list[str] = []
    if isinstance(telemetry, list):
        for idx, entry in enumerate(telemetry):
            ctx = f"telemetry[{idx}]"
            require(isinstance(entry, dict), f"{ctx} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            address = entry.get("address")
            metric = entry.get("metric")
            match = HOST_ROUTE_RE.match(address) if isinstance(address, str) else None
            require(match is not None, f"{ctx}.address must be /sensor/host/<source>/<metric>", errors)
            if match is not None:
                source_id, route_metric = match.groups()
                require(SOURCE_RE.match(source_id) is not None, f"{ctx}.address source id is invalid", errors)
                require(source_id == default_source, f"{ctx}.address should use the default source fixture", errors)
                require(route_metric == metric, f"{ctx}.metric must match the address tail", errors)
            require(metric in EXPECTED_METRICS, f"{ctx}.metric is not one of {EXPECTED_METRICS}", errors)
            if isinstance(metric, str):
                seen_metrics.append(metric)
            require(entry.get("type") == "float", f"{ctx}.type must be float", errors)
            require(entry.get("range") == [0.0, 1.0], f"{ctx}.range must be [0.0, 1.0]", errors)
            require(entry.get("usesScalarHistory") is True, f"{ctx}.usesScalarHistory must be true", errors)
    require(seen_metrics == list(EXPECTED_METRICS), f"telemetry metrics must be ordered as {EXPECTED_METRICS}", errors)

    waveform = data.get("waveform")
    require(isinstance(waveform, dict), "waveform must be an object", errors)
    if isinstance(waveform, dict):
        address = waveform.get("address")
        match = HOST_ROUTE_RE.match(address) if isinstance(address, str) else None
        require(match is not None, "waveform.address must be /sensor/host/<source>/waveform", errors)
        if match is not None:
            source_id, metric = match.groups()
            require(SOURCE_RE.match(source_id) is not None, "waveform.address source id is invalid", errors)
            require(source_id == default_source, "waveform.address should use the default source fixture", errors)
            require(metric == "waveform", "waveform address tail must be waveform", errors)
        require(waveform.get("type") == "float-list", "waveform.type must be float-list", errors)
        require(waveform.get("sampleRange") == [-1.0, 1.0], "waveform.sampleRange must be [-1.0, 1.0]", errors)
        require(waveform.get("allowedSampleCounts") == [64, 128, 256], "waveform.allowedSampleCounts is unexpected", errors)
        require(waveform.get("defaultSampleCount") == 128, "waveform.defaultSampleCount must be 128", errors)
        require(waveform.get("defaultRateHz") == 30, "waveform.defaultRateHz must be 30", errors)
        require(waveform.get("usesScalarHistory") is False, "waveform.usesScalarHistory must be false", errors)
        require(waveform.get("publishesAudioAnalysisBusSnapshot") is True, "waveform must publish AudioAnalysisBus snapshots", errors)

    return errors


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", type=Path, default=DEFAULT_FIXTURE, help="Signal Control receive fixture")
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG, help="Synaptome osc-input.json to validate")
    parser.add_argument("--check", action="store_true", help="Compatibility flag for contract reports")
    args = parser.parse_args(argv)

    fixture_path = args.fixture if args.fixture.is_absolute() else ROOT / args.fixture
    config_path = args.config if args.config.is_absolute() else ROOT / args.config

    errors: list[str] = []
    try:
        fixture = load_json(fixture_path)
        errors.extend(validate_fixture(fixture))
    except OSError as exc:
        errors.append(f"failed to read fixture {fixture_path}: {exc}")
    except json.JSONDecodeError as exc:
        errors.append(f"fixture JSON parse error in {fixture_path}: {exc}")

    try:
        config = load_json(config_path)
        errors.extend(f"osc-input config: {error}" for error in validate_osc_input(config, config_path))
    except OSError as exc:
        errors.append(f"failed to read config {config_path}: {exc}")
    except json.JSONDecodeError as exc:
        errors.append(f"config JSON parse error in {config_path}: {exc}")

    if errors:
        print("Signal Control receive contract validation failed:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print("Signal Control receive contract validated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

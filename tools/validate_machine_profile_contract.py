#!/usr/bin/env python3
"""Validate the pure machine-profile v1 document contract."""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

try:
    import jsonschema  # type: ignore
except ImportError:  # pragma: no cover - environment/setup dependent
    jsonschema = None


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "docs/schemas/machine_profile.schema.json"
EXAMPLE = ROOT / "docs/examples/machine_profile_example.json"
CANONICAL = ROOT / "tools/testdata/machine_profile/canonical_v1.json"
EMPTY = ROOT / "tools/testdata/machine_profile/canonical_empty_v1.json"
INVALID = ROOT / "tools/testdata/machine_profile/invalid_cases.json"
HEADER = ROOT / "synaptome/src/io/MachineProfileDocument.h"
SOURCE = ROOT / "synaptome/src/io/MachineProfileDocument.cpp"
APP_SOURCE = ROOT / "synaptome/src/ofApp.cpp"

STABLE_ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,126}$")
ASSIGNMENT_KEY_RE = re.compile(
    r"^[A-Za-z0-9][A-Za-z0-9._-]*(::[A-Za-z0-9][A-Za-z0-9._-]*)?$"
)
MAX_ASSIGNMENT_KEY_LENGTH = 255
MAX_BINDING_ID_LENGTH = 255


class ContractError(RuntimeError):
    pass


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"{path}: failed to load JSON: {exc}") from exc


def validate_endpoint(endpoint: Any, path: str) -> list[str]:
    errors: list[str] = []
    if not isinstance(endpoint, dict):
        return [f"{path} must be an object"]
    allowed = {"id", "enabled", "transport", "udp", "serial"}
    unknown = set(endpoint) - allowed
    if unknown:
        errors.append(f"{path} contains unknown fields: {sorted(unknown)}")
    endpoint_id = endpoint.get("id")
    if not isinstance(endpoint_id, str) or STABLE_ID_RE.fullmatch(endpoint_id) is None:
        errors.append(f"{path}.id must be a stable ID")
    if not isinstance(endpoint.get("enabled"), bool):
        errors.append(f"{path}.enabled must be a boolean")

    transport = endpoint.get("transport")
    if transport == "udp":
        if "serial" in endpoint:
            errors.append(f"{path} UDP endpoint must not contain serial")
        udp = endpoint.get("udp")
        if not isinstance(udp, dict):
            errors.append(f"{path}.udp must be an object")
        else:
            unknown_udp = set(udp) - {"host", "port"}
            if unknown_udp:
                errors.append(f"{path}.udp contains unknown fields: {sorted(unknown_udp)}")
            if not isinstance(udp.get("host"), str) or not udp.get("host"):
                errors.append(f"{path}.udp.host must be a non-empty string")
            port = udp.get("port")
            if (
                not isinstance(port, int)
                or isinstance(port, bool)
                or port < 1
                or port > 65535
            ):
                errors.append(f"{path}.udp.port must be an integer from 1 through 65535")
    elif transport == "serial":
        if "udp" in endpoint:
            errors.append(f"{path} serial endpoint must not contain udp")
        serial = endpoint.get("serial")
        if not isinstance(serial, dict):
            errors.append(f"{path}.serial must be an object")
        else:
            unknown_serial = set(serial) - {
                "autoPort",
                "baud",
                "port",
                "portNameContains",
            }
            if unknown_serial:
                errors.append(
                    f"{path}.serial contains unknown fields: {sorted(unknown_serial)}"
                )
            auto_port = serial.get("autoPort")
            if not isinstance(auto_port, bool):
                errors.append(f"{path}.serial.autoPort must be a boolean")
            baud = serial.get("baud")
            if (
                not isinstance(baud, int)
                or isinstance(baud, bool)
                or baud < 1200
                or baud > 4_000_000
            ):
                errors.append(
                    f"{path}.serial.baud must be an integer from 1200 through 4000000"
                )
            for key in ("port", "portNameContains"):
                if key in serial and (
                    not isinstance(serial[key], str) or not serial[key]
                ):
                    errors.append(f"{path}.serial.{key} must be a non-empty string")
            if auto_port is False and not (
                isinstance(serial.get("port"), str)
                and serial["port"]
                or isinstance(serial.get("portNameContains"), str)
                and serial["portNameContains"]
            ):
                errors.append(
                    f"{path}.serial requires port or portNameContains when autoPort is false"
                )
    else:
        errors.append(f"{path}.transport must be udp or serial")
    return errors


def validate_control_slots(control_slots: Any, path: str) -> list[str]:
    errors: list[str] = []
    if not isinstance(control_slots, dict):
        return [f"{path} must be an object"]
    unknown = set(control_slots) - {"assignments"}
    if unknown:
        errors.append(f"{path} contains unknown fields: {sorted(unknown)}")
    assignments = control_slots.get("assignments")
    if not isinstance(assignments, list):
        errors.append(f"{path}.assignments must be an array")
        return errors

    seen_assignment_keys: set[str] = set()
    allowed = {"assignmentKey", "deviceProfileId", "slotId", "analog"}
    for index, assignment in enumerate(assignments):
        assignment_path = f"{path}.assignments[{index}]"
        if not isinstance(assignment, dict):
            errors.append(f"{assignment_path} must be an object")
            continue
        unknown_assignment = set(assignment) - allowed
        if unknown_assignment:
            errors.append(
                f"{assignment_path} contains unknown fields: "
                f"{sorted(unknown_assignment)}"
            )
        missing = allowed - set(assignment)
        if missing:
            errors.append(
                f"{assignment_path} is missing required fields: {sorted(missing)}"
            )

        assignment_key = assignment.get("assignmentKey")
        if (
            not isinstance(assignment_key, str)
            or len(assignment_key) > MAX_ASSIGNMENT_KEY_LENGTH
            or ASSIGNMENT_KEY_RE.fullmatch(assignment_key) is None
        ):
            errors.append(
                f"{assignment_path}.assignmentKey must be a stable parameter ID "
                "or canonical assetId::parameterSuffix key"
            )
        elif assignment_key in seen_assignment_keys:
            errors.append(f"duplicate control-slot assignmentKey: {assignment_key}")
        else:
            seen_assignment_keys.add(assignment_key)

        for key in ("deviceProfileId", "slotId"):
            value = assignment.get(key)
            if (
                not isinstance(value, str)
                or not value.strip()
                or len(value) > MAX_BINDING_ID_LENGTH
            ):
                errors.append(
                    f"{assignment_path}.{key} must be a nonblank string no "
                    f"longer than {MAX_BINDING_ID_LENGTH} characters"
                )
        if not isinstance(assignment.get("analog"), bool):
            errors.append(f"{assignment_path}.analog must be a boolean")
    return errors


def validate_midi(midi: Any, path: str) -> list[str]:
    errors: list[str] = []
    if not isinstance(midi, dict):
        return [f"{path} must be an object"]
    unknown = set(midi) - {"inputs"}
    if unknown:
        errors.append(f"{path} contains unknown fields: {sorted(unknown)}")
    inputs = midi.get("inputs")
    if not isinstance(inputs, list):
        errors.append(f"{path}.inputs must be an array")
        return errors
    if len(inputs) > 1:
        errors.append(f"{path}.inputs must contain at most one input in v1")
    allowed = {"deviceProfileId", "portName"}
    for index, input_binding in enumerate(inputs):
        input_path = f"{path}.inputs[{index}]"
        if not isinstance(input_binding, dict):
            errors.append(f"{input_path} must be an object")
            continue
        unknown_input = set(input_binding) - allowed
        if unknown_input:
            errors.append(
                f"{input_path} contains unknown fields: {sorted(unknown_input)}"
            )
        missing = allowed - set(input_binding)
        if missing:
            errors.append(
                f"{input_path} is missing required fields: {sorted(missing)}"
            )
        for key in ("deviceProfileId", "portName"):
            value = input_binding.get(key)
            if (
                not isinstance(value, str)
                or not value.strip()
                or len(value) > MAX_BINDING_ID_LENGTH
            ):
                errors.append(
                    f"{input_path}.{key} must be a nonblank string no longer "
                    f"than {MAX_BINDING_ID_LENGTH} characters"
                )
    return errors


def validate_document(document: Any) -> tuple[list[str], bool]:
    errors: list[str] = []
    if not isinstance(document, dict):
        return ["root must be an object"], False

    version = document.get("schemaVersion")
    if not isinstance(version, int) or isinstance(version, bool):
        return ["schemaVersion must be an integer"], False
    if version > 1:
        return [f"unsupported future schemaVersion {version}"], True
    if version != 1:
        return ["schemaVersion must be exactly 1"], False

    unknown = set(document) - {
        "schemaVersion",
        "profileId",
        "osc",
        "midi",
        "controlSlots",
    }
    if unknown:
        errors.append(f"root contains unknown fields: {sorted(unknown)}")
    profile_id = document.get("profileId")
    if not isinstance(profile_id, str) or STABLE_ID_RE.fullmatch(profile_id) is None:
        errors.append("profileId must be a stable ID")

    osc = document.get("osc")
    if not isinstance(osc, dict):
        errors.append("osc must be an object")
        return errors, False
    unknown_osc = set(osc) - {"inputs", "activeInputId", "outputs"}
    if unknown_osc:
        errors.append(f"osc contains unknown fields: {sorted(unknown_osc)}")
    inputs = osc.get("inputs")
    if not isinstance(inputs, list):
        errors.append("osc.inputs must be an array")
        return errors, False
    outputs = osc.get("outputs", [])
    if not isinstance(outputs, list):
        errors.append("osc.outputs must be an array")
        return errors, False

    all_ids: set[str] = set()
    enabled_input_ids: set[str] = set()
    for section, entries in (("inputs", inputs), ("outputs", outputs)):
        for index, endpoint in enumerate(entries):
            path = f"osc.{section}[{index}]"
            errors.extend(validate_endpoint(endpoint, path))
            if not isinstance(endpoint, dict):
                continue
            endpoint_id = endpoint.get("id")
            if not isinstance(endpoint_id, str):
                continue
            if endpoint_id in all_ids:
                errors.append(f"duplicate OSC endpoint id: {endpoint_id}")
            all_ids.add(endpoint_id)
            if section == "inputs" and endpoint.get("enabled") is True:
                enabled_input_ids.add(endpoint_id)

    active_input_id = osc.get("activeInputId")
    if active_input_id is None:
        if enabled_input_ids:
            errors.append("activeInputId is required when an input is enabled")
    elif (
        not isinstance(active_input_id, str)
        or STABLE_ID_RE.fullmatch(active_input_id) is None
    ):
        errors.append("activeInputId must be a stable ID")
    elif active_input_id not in enabled_input_ids:
        errors.append("activeInputId must reference an enabled input")

    if "midi" in document:
        errors.extend(validate_midi(document["midi"], "midi"))

    if "controlSlots" in document:
        errors.extend(
            validate_control_slots(document["controlSlots"], "controlSlots")
        )

    return errors, False


def validate() -> int:
    schema = load_json(SCHEMA)
    example = load_json(EXAMPLE)
    canonical = load_json(CANONICAL)
    empty = load_json(EMPTY)
    invalid = load_json(INVALID)

    if schema.get("properties", {}).get("schemaVersion", {}).get("const") != 1:
        raise ContractError("machine-profile schema must pin schemaVersion to 1")
    if "controlSlots" in schema.get("required", []):
        raise ContractError("controlSlots must remain an optional root section")
    if "midi" in schema.get("required", []):
        raise ContractError("midi must remain an optional root section")
    midi_schema = schema.get("properties", {}).get("midi", {})
    if midi_schema.get("required") != ["inputs"]:
        raise ContractError("midi must require exactly its inputs array")
    midi_inputs_schema = midi_schema.get("properties", {}).get("inputs", {})
    if midi_inputs_schema.get("maxItems") != 1:
        raise ContractError("machine-profile v1 must accept at most one MIDI input")
    midi_input_schema = schema.get("definitions", {}).get("midiInput", {})
    if (
        midi_input_schema.get("required")
        != ["deviceProfileId", "portName"]
        or midi_input_schema.get("additionalProperties") is not False
        or set(midi_input_schema.get("properties", {}))
        != {"deviceProfileId", "portName"}
    ):
        raise ContractError(
            "MIDI input rows must contain exactly deviceProfileId and portName"
        )
    midi_description = " ".join(
        (
            str(midi_schema.get("description", "")),
            str(midi_input_schema.get("description", "")),
        )
    ).lower()
    for token in (
        "omission delegates to legacy",
        "present empty inputs array disables midi",
        "exact unique",
        "hints",
        "substrings",
        "indices",
        "port-zero fallback",
        "non-unique port remains unresolved",
        "must not guess",
    ):
        if token not in midi_description:
            raise ContractError(
                f"MIDI schema description missing resolution-policy token: {token}"
            )
    control_slot_schema = (
        schema.get("properties", {})
        .get("controlSlots", {})
    )
    if control_slot_schema.get("required") != ["assignments"]:
        raise ContractError(
            "controlSlots must require exactly its assignments array"
        )
    for path, document in (
        (EXAMPLE, example),
        (CANONICAL, canonical),
        (EMPTY, empty),
    ):
        errors, future = validate_document(document)
        if errors or future:
            raise ContractError(f"{path}: {'; '.join(errors)}")
        if jsonschema is not None:
            schema_errors = sorted(
                jsonschema.Draft7Validator(schema).iter_errors(document),
                key=lambda error: list(error.path),
            )
            if schema_errors:
                rendered = "; ".join(error.message for error in schema_errors)
                raise ContractError(f"{path}: schema validation failed: {rendered}")

    osc_only = {
        key: value
        for key, value in canonical.items()
        if key not in {"midi", "controlSlots"}
    }
    errors, future = validate_document(osc_only)
    if errors or future:
        raise ContractError(
            "optional MIDI/controlSlots omission must preserve the OSC-only "
            "v1 shape: "
            + "; ".join(errors)
        )
    if jsonschema is not None:
        schema_errors = list(
            jsonschema.Draft7Validator(schema).iter_errors(osc_only)
        )
        if schema_errors:
            raise ContractError(
                "schema rejected optional MIDI/controlSlots omission"
            )

    if empty != {
        "schemaVersion": 1,
        "profileId": "fixture-empty",
        "osc": {"inputs": []},
        "midi": {"inputs": []},
        "controlSlots": {"assignments": []},
    }:
        raise ContractError("canonical empty fixture changed explicit-empty semantics")
    assignments = (
        canonical.get("controlSlots", {})
        .get("assignments")
    )
    if not isinstance(assignments, list) or not assignments:
        raise ContractError(
            "canonical fixture must exercise logical control-slot assignments"
        )
    for index, assignment in enumerate(assignments):
        if not isinstance(assignment, dict) or set(assignment) != {
            "assignmentKey",
            "deviceProfileId",
            "slotId",
            "analog",
        }:
            raise ContractError(
                f"canonical assignment {index} does not use exact canonical fields"
            )
    if not any(
        assignment.get("deviceProfileId") == "MIDI Mix 0"
        for assignment in assignments
    ):
        raise ContractError(
            "canonical fixture must preserve the shipped spaced device ID"
        )
    midi_inputs = canonical.get("midi", {}).get("inputs")
    if midi_inputs != [
        {
            "deviceProfileId": "MIDI Mix 0",
            "portName": "MIDI Mix 0",
        }
    ]:
        raise ContractError(
            "canonical fixture must exercise the exact single MIDI input shape"
        )

    midi_omitted = {
        key: value
        for key, value in canonical.items()
        if key != "midi"
    }
    errors, future = validate_document(midi_omitted)
    if errors or future:
        raise ContractError(
            "midi omission must preserve the valid profile shape: "
            + "; ".join(errors)
        )
    if jsonschema is not None:
        schema_errors = list(
            jsonschema.Draft7Validator(schema).iter_errors(midi_omitted)
        )
        if schema_errors:
            raise ContractError("schema rejected optional midi omission")

    cases = invalid.get("cases") if isinstance(invalid, dict) else None
    if not isinstance(cases, list) or not cases:
        raise ContractError("invalid_cases.json must contain a non-empty cases array")
    seen_case_ids: set[str] = set()
    for index, case in enumerate(cases):
        if not isinstance(case, dict):
            raise ContractError(f"invalid case {index} must be an object")
        case_id = case.get("id")
        if not isinstance(case_id, str) or not case_id or case_id in seen_case_ids:
            raise ContractError(f"invalid case {index} has missing/duplicate id")
        seen_case_ids.add(case_id)
        errors, future = validate_document(case.get("document"))
        if not errors:
            raise ContractError(f"invalid case was accepted: {case_id}")
        expected = case.get("error")
        if expected == "future" and not future:
            raise ContractError(f"future case was not distinguishable: {case_id}")
        if expected != "future" and future:
            raise ContractError(f"invalid case was misclassified as future: {case_id}")

    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")
    app_source = APP_SOURCE.read_text(encoding="utf-8", errors="replace")
    for snippet in (
        "kCurrentMachineProfileSchemaVersion = 1",
        "kMaxMachineProfileAssignmentKeyLength = 255",
        "kMaxMachineProfileBindingIdLength = 255",
        "UnsupportedFutureVersion",
        "validateMachineProfileDocument",
    ):
        if snippet not in header:
            raise ContractError(f"machine-profile header missing contract token: {snippet}")
    for snippet in (
        'contains("schemaVersion")',
        'source["schemaVersion"]',
        'enabledInputIds.find(activeInputId)',
        'source.contains("midi")',
        '"$.midi.inputs must be an array"',
        '"$.midi.inputs must contain at most one input in v1"',
        'source.contains("controlSlots")',
        '"duplicate control-slot assignmentKey "',
        'result.document = source',
    ):
        if snippet not in source:
            raise ContractError(f"machine-profile reader missing contract token: {snippet}")
    for snippet in (
        'ofToDataPath("config/machine-profile.json", true)',
        "loadMachineProfileSettings();",
        "loadOscInputSettingsFromMachineProfile()",
        "refusing backup or legacy downgrade",
        "Using legacy OSC input compatibility config",
        "writeJsonRecoverably(",
        "previous binding restored",
    ):
        if snippet not in app_source:
            raise ContractError(
                f"machine-profile runtime wiring missing contract token: {snippet}"
            )

    forbidden_example_tokens = (
        "deviceIndex",
        "monitorId",
        "mappings",
        "layers",
        "hotkeys",
        "telemetry",
        "parameterId",
        "deviceId",
        "deviceName",
        "slotLabel",
    )
    example_text = EXAMPLE.read_text(encoding="utf-8")
    for token in forbidden_example_tokens:
        if f'"{token}"' in example_text:
            raise ContractError(
                f"public machine-profile example absorbed non-OSC state: {token}"
            )

    return len(cases)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Run read-only validation (default).",
    )
    parser.parse_args(argv)
    try:
        case_count = validate()
    except ContractError as exc:
        print(f"Machine-profile contract error: {exc}", file=sys.stderr)
        return 1
    print(
        "Machine-profile v1 contract passed "
        f"(canonical + empty + {case_count} rejection cases)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

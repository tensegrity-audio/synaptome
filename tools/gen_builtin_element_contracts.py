#!/usr/bin/env python3
"""Validate and generate compiled/catalog/doc views of built-in element parameters."""
from __future__ import annotations

import argparse
import json
import math
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs/contracts/builtin_element_parameters.json"
COMPILED = (
    ROOT
    / "synaptome/src/runtime"
    / "BuiltinElementParameterContracts.generated.inc"
)
CATALOG = ROOT / "docs/contracts/element_parameter_catalog.json"
REFERENCE = ROOT / "docs/element_parameter_reference.md"

TYPE_ID = re.compile(r"^[a-z][A-Za-z0-9]*(?:\.[a-z][A-Za-z0-9]*)*$")
LOCAL_ID = re.compile(r"^[a-z][A-Za-z0-9]*$")
GROUP_ID = LOCAL_ID
KINDS = {"float", "bool", "string"}


class ContractError(RuntimeError):
    pass


def load_source() -> dict[str, Any]:
    try:
        data = json.loads(SOURCE.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"cannot read {SOURCE.relative_to(ROOT)}: {exc}") from exc
    if not isinstance(data, dict) or data.get("schemaVersion") != 1:
        raise ContractError("built-in element contract snapshot must use schemaVersion 1")
    types = data.get("types")
    if not isinstance(types, list) or not types:
        raise ContractError("built-in element contract snapshot requires types[]")
    return data


def value_matches(kind: str, value: Any) -> bool:
    if kind == "float":
        return (
            isinstance(value, (int, float))
            and not isinstance(value, bool)
            and math.isfinite(float(value))
        )
    if kind == "bool":
        return isinstance(value, bool)
    return isinstance(value, str)


def validate(data: dict[str, Any]) -> None:
    seen_types: set[str] = set()
    for type_index, item in enumerate(data["types"]):
        context = f"types[{type_index}]"
        if not isinstance(item, dict):
            raise ContractError(f"{context} must be an object")
        type_id = item.get("typeId")
        if not isinstance(type_id, str) or not TYPE_ID.fullmatch(type_id):
            raise ContractError(f"{context}.typeId is invalid: {type_id!r}")
        if type_id in seen_types:
            raise ContractError(f"duplicate element typeId: {type_id}")
        seen_types.add(type_id)

        declarations = item.get("declarations")
        if not isinstance(declarations, dict):
            raise ContractError(f"{context}.declarations must be an object")
        groups = declarations.get("groups")
        parameters = declarations.get("parameters")
        if not isinstance(groups, list) or not isinstance(parameters, list):
            raise ContractError(f"{context}.declarations requires groups[] and parameters[]")

        group_ids: set[str] = set()
        for group_index, group in enumerate(groups):
            group_context = f"{context}.groups[{group_index}]"
            if not isinstance(group, dict):
                raise ContractError(f"{group_context} must be an object")
            group_id = group.get("id")
            if not isinstance(group_id, str) or not GROUP_ID.fullmatch(group_id):
                raise ContractError(f"{group_context}.id is invalid: {group_id!r}")
            if group_id in group_ids:
                raise ContractError(f"{context} has duplicate group {group_id}")
            if not isinstance(group.get("label"), str) or not group["label"]:
                raise ContractError(f"{group_context}.label must be nonempty")
            group_ids.add(group_id)

        parameter_ids: set[str] = set()
        quick_orders: set[int] = set()
        for parameter_index, parameter in enumerate(parameters):
            parameter_context = f"{context}.parameters[{parameter_index}]"
            if not isinstance(parameter, dict):
                raise ContractError(f"{parameter_context} must be an object")
            parameter_id = parameter.get("id")
            kind = parameter.get("kind")
            if not isinstance(parameter_id, str) or not LOCAL_ID.fullmatch(parameter_id):
                raise ContractError(f"{parameter_context}.id is invalid: {parameter_id!r}")
            if parameter_id in {"active", "opacity"}:
                raise ContractError(f"{parameter_context}.id is layer-container reserved")
            if parameter_id in parameter_ids:
                raise ContractError(f"{context} has duplicate parameter {parameter_id}")
            parameter_ids.add(parameter_id)
            if kind not in KINDS:
                raise ContractError(f"{parameter_context}.kind is invalid: {kind!r}")
            if parameter.get("groupId") not in group_ids:
                raise ContractError(f"{parameter_context} references an unknown group")
            if not isinstance(parameter.get("label"), str) or not parameter["label"]:
                raise ContractError(f"{parameter_context}.label must be nonempty")
            if not value_matches(kind, parameter.get("default")):
                raise ContractError(f"{parameter_context}.default does not match {kind}")

            range_node = parameter.get("range")
            if range_node is not None:
                if kind != "float" or not isinstance(range_node, dict):
                    raise ContractError(f"{parameter_context}.range is invalid")
                minimum = range_node.get("min")
                maximum = range_node.get("max")
                if not value_matches("float", minimum) or not value_matches("float", maximum):
                    raise ContractError(f"{parameter_context}.range bounds must be finite")
                if float(minimum) > float(maximum):
                    raise ContractError(f"{parameter_context}.range min exceeds max")
                default = float(parameter["default"])
                if default < float(minimum) or default > float(maximum):
                    raise ContractError(f"{parameter_context}.default is outside range")
                step = range_node.get("step")
                if step is not None and (
                    not value_matches("float", step) or float(step) <= 0.0
                ):
                    raise ContractError(f"{parameter_context}.range step is invalid")

            order = parameter.get("quickAccessOrder")
            if order is not None:
                if not isinstance(order, int) or isinstance(order, bool) or order < 0:
                    raise ContractError(f"{parameter_context}.quickAccessOrder is invalid")
                if order in quick_orders:
                    raise ContractError(f"{context} has duplicate quick-access order {order}")
                quick_orders.add(order)

            options = parameter.get("options", [])
            option_source = parameter.get("optionSource")
            if options and option_source:
                raise ContractError(f"{parameter_context} has options and optionSource")
            if not isinstance(options, list):
                raise ContractError(f"{parameter_context}.options must be an array")
            for option_index, option in enumerate(options):
                if (
                    not isinstance(option, dict)
                    or not value_matches(kind, option.get("value"))
                    or not isinstance(option.get("label"), str)
                    or not option["label"]
                ):
                    raise ContractError(
                        f"{parameter_context}.options[{option_index}] is invalid"
                    )
            if option_source is not None:
                if not isinstance(option_source, dict):
                    raise ContractError(f"{parameter_context}.optionSource is invalid")
                for field in ("id", "valueField", "labelField"):
                    if not isinstance(option_source.get(field), str) or not option_source[field]:
                        raise ContractError(
                            f"{parameter_context}.optionSource.{field} is required"
                        )

    if len(seen_types) != 23:
        raise ContractError(f"expected 23 shipping element types, found {len(seen_types)}")


def compiled_text(data: dict[str, Any]) -> str:
    payload = json.dumps(data, indent=2, ensure_ascii=True) + "\n"
    if ")SYNAPTOME_JSON\"" in payload:
        raise ContractError("snapshot collides with generated raw-string delimiter")
    chunks = [
        payload[offset : offset + 12000]
        for offset in range(0, len(payload), 12000)
    ]
    chunk_literals = "\n".join(
        '    R"SYNAPTOME_JSON(' + chunk + ')SYNAPTOME_JSON",'
        for chunk in chunks
    )
    return (
        "// Generated by tools/gen_builtin_element_contracts.py. Do not edit.\n"
        "static constexpr const char* "
        "kBuiltinElementParameterContractJsonChunks[] = {\n"
        + chunk_literals
        + "\n};\n"
    )


def catalog_data(data: dict[str, Any]) -> dict[str, Any]:
    types: list[dict[str, Any]] = []
    total_groups = 0
    total_parameters = 0
    for item in data["types"]:
        declarations = item["declarations"]
        groups = declarations["groups"]
        parameters = declarations["parameters"]
        total_groups += len(groups)
        total_parameters += len(parameters)
        types.append(
            {
                "typeId": item["typeId"],
                "groupCount": len(groups),
                "parameterCount": len(parameters),
                "groups": groups,
                "parameters": [
                    {
                        key: parameter[key]
                        for key in (
                            "id",
                            "kind",
                            "groupId",
                            "label",
                            "units",
                            "description",
                            "quickAccessOrder",
                            "optionSource",
                        )
                        if key in parameter
                    }
                    for parameter in parameters
                ],
            }
        )
    return {
        "schemaVersion": 1,
        "status": "generated",
        "generator": "tools/gen_builtin_element_contracts.py",
        "source": SOURCE.relative_to(ROOT).as_posix(),
        "counts": {
            "types": len(types),
            "groups": total_groups,
            "parameters": total_parameters,
        },
        "types": types,
    }


def reference_text(data: dict[str, Any]) -> str:
    total = sum(
        len(item["declarations"]["parameters"]) for item in data["types"]
    )
    lines = [
        "# Built-in Element Parameter Reference",
        "",
        "Generated by `tools/gen_builtin_element_contracts.py` from the reviewed",
        "built-in declaration snapshot. Do not edit this file by hand.",
        "",
        f"Shipping types: **{len(data['types'])}**. Parameters: **{total}**.",
        "",
        "| Element type | Groups | Parameters |",
        "| --- | ---: | ---: |",
    ]
    for item in data["types"]:
        declarations = item["declarations"]
        lines.append(
            f"| `{item['typeId']}` | {len(declarations['groups'])} | "
            f"{len(declarations['parameters'])} |"
        )
    for item in data["types"]:
        declarations = item["declarations"]
        group_labels = {
            group["id"]: group["label"] for group in declarations["groups"]
        }
        lines.extend(
            [
                "",
                f"## `{item['typeId']}`",
                "",
                "| Parameter | Kind | Group | Label | Units |",
                "| --- | --- | --- | --- | --- |",
            ]
        )
        if not declarations["parameters"]:
            lines.append("| _None_ |  |  |  |  |")
            continue
        for parameter in declarations["parameters"]:
            units = str(parameter.get("units", "")).replace("|", "\\|")
            label = str(parameter["label"]).replace("|", "\\|")
            lines.append(
                f"| `{parameter['id']}` | {parameter['kind']} | "
                f"{group_labels[parameter['groupId']]} | {label} | {units} |"
            )
    return "\n".join(lines) + "\n"


def write_or_check(path: Path, expected: str, check: bool) -> bool:
    if check:
        try:
            actual = path.read_text(encoding="utf-8")
        except OSError:
            actual = ""
        if actual != expected:
            print(f"[builtin-element-contracts] stale: {path.relative_to(ROOT)}")
            return False
        return True
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(expected, encoding="utf-8", newline="\n")
    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        data = load_source()
        validate(data)
        catalog = json.dumps(
            catalog_data(data), indent=2, ensure_ascii=False
        ) + "\n"
        outputs = (
            (COMPILED, compiled_text(data)),
            (CATALOG, catalog),
            (REFERENCE, reference_text(data)),
        )
        valid = all(
            write_or_check(path, expected, args.check)
            for path, expected in outputs
        )
    except ContractError as exc:
        print(f"[builtin-element-contracts] FAIL {exc}")
        return 1
    if not valid:
        return 1
    mode = "check" if args.check else "write"
    print(
        f"[builtin-element-contracts] PASS {mode}: "
        f"{len(data['types'])} types, "
        f"{sum(len(item['declarations']['parameters']) for item in data['types'])} parameters"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

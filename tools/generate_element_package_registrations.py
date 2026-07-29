#!/usr/bin/env python3
"""Generate the controlled source-package registration and build records."""
from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any

import element_package_v1

ROOT = Path(__file__).resolve().parents[1]
REGISTRATION_SET = (
    ROOT / "docs/contracts/element_package_registration_set_v1.json"
)
GENERATED_HEADER = (
    ROOT
    / "synaptome/src/runtime"
    / "GeneratedElementPackageRegistrations.h"
)
GENERATED_SOURCE = (
    ROOT
    / "synaptome/src/runtime"
    / "GeneratedElementPackageRegistrations.cpp"
)
GENERATED_TARGETS = (
    ROOT / "synaptome/build/GeneratedElementPackages.targets"
)


class GenerationError(RuntimeError):
    pass


@dataclass(frozen=True)
class RegistrationRecord:
    package_path: Path
    package_id: str
    package_version: str
    implementation_version: str
    type_id: str
    kind: str
    binding_mode: str
    definition_id: str
    registry_prefix: str
    registration_path: Path
    registration_reference: str
    compile_paths: tuple[Path, ...]
    descriptor_signature: str
    symbol: str
    normalized: dict[str, Any]


def _rel(path: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return path.resolve().as_posix()


def _read_object(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise GenerationError(f"cannot read {_rel(path)}: {exc}") from exc
    if not isinstance(value, dict):
        raise GenerationError(f"{_rel(path)} root must be an object")
    return value


def _resolve_repo_reference(raw: Any, context: str) -> Path:
    if not isinstance(raw, str) or not raw:
        raise GenerationError(f"{context} must be a non-empty string")
    if "\\" in raw:
        raise GenerationError(f"{context} must use '/' separators")
    pure = PurePosixPath(raw)
    if pure.is_absolute() or any(part in {"", ".", ".."} for part in pure.parts):
        raise GenerationError(
            f"{context} must be a normalized repository-relative path"
        )
    resolved = (ROOT / Path(*pure.parts)).resolve()
    try:
        resolved.relative_to(ROOT)
    except ValueError as exc:
        raise GenerationError(f"{context} escapes the repository") from exc
    if not resolved.is_file():
        raise GenerationError(f"{context} does not name a file: {raw}")
    return resolved


def registration_symbol(package_id: str) -> str:
    suffix = re.sub(r"[^A-Za-z0-9]", "_", package_id)
    return f"synaptomeCreateElementPackage_{suffix}"


def load_records(
    registration_set: Path = REGISTRATION_SET,
) -> list[RegistrationRecord]:
    source = _read_object(registration_set)
    if set(source) != {"schemaVersion", "packages"}:
        raise GenerationError(
            f"{_rel(registration_set)} must contain only "
            "schemaVersion and packages"
        )
    if source.get("schemaVersion") != 1:
        raise GenerationError("registration set readers accept exactly v1")
    package_references = source.get("packages")
    if not isinstance(package_references, list) or not package_references:
        raise GenerationError("registration set requires a non-empty packages[]")
    if len(package_references) != len(set(map(str, package_references))):
        raise GenerationError("registration set contains duplicate paths")

    results: list[element_package_v1.ValidationResult] = []
    paths: list[Path] = []
    for index, raw in enumerate(package_references):
        path = _resolve_repo_reference(
            raw, f"$.packages[{index}]"
        )
        result = element_package_v1.validate_package(path)
        if not result.valid:
            details = "\n".join(
                diagnostic.render() for diagnostic in result.diagnostics
            )
            raise GenerationError(
                f"package preflight failed for {_rel(path)}\n{details}"
            )
        results.append(result)
        paths.append(path)

    conflicts = element_package_v1.validate_activation_set(results)
    if conflicts:
        raise GenerationError(
            "registration set conflicts\n"
            + "\n".join(item.render() for item in conflicts)
        )

    records: list[RegistrationRecord] = []
    symbols: dict[str, str] = {}
    package_ids = {
        str(result.document.get("packageId"))
        for result in results
        if result.document is not None
    }
    for path, result in zip(paths, results):
        assert result.document is not None
        assert result.normalized is not None
        document = result.document
        source_record = document["source"]
        asset = document["asset"]
        element = document["element"]
        if source_record.get("strategy") != "source-registration":
            raise GenerationError(
                f"{document['packageId']}: controlled registration only "
                "accepts source-registration packages"
            )
        package_id = str(document["packageId"])
        symbol = registration_symbol(package_id)
        if symbol in symbols:
            raise GenerationError(
                f"registration symbol collision: {package_id} and "
                f"{symbols[symbol]} both map to {symbol}"
            )
        symbols[symbol] = package_id

        registration_reference = str(source_record["registration"])
        registration_path, error = (
            element_package_v1.resolve_package_reference(
                path, registration_reference
            )
        )
        if error or registration_path is None:
            raise GenerationError(
                f"{package_id}: invalid source registration: {error}"
            )
        registration_text = registration_path.read_text(encoding="utf-8")
        declaration = re.compile(
            rf"\bstd::unique_ptr\s*<\s*Layer\s*>\s*"
            rf"{re.escape(symbol)}\s*\(\s*\)"
        )
        if declaration.search(registration_text) is None:
            raise GenerationError(
                f"{package_id}: {registration_reference} must define "
                f"std::unique_ptr<Layer> {symbol}()"
            )

        compile_paths: list[Path] = []
        for raw_source in source_record.get("files", []):
            source_path, source_error = (
                element_package_v1.resolve_package_reference(path, raw_source)
            )
            if source_error or source_path is None:
                raise GenerationError(
                    f"{package_id}: invalid source file {raw_source!r}: "
                    f"{source_error}"
                )
            if source_path.suffix.lower() in {".c", ".cc", ".cpp", ".cxx"}:
                compile_paths.append(source_path)
        compile_paths.append(registration_path)
        if len(compile_paths) != len(set(compile_paths)):
            raise GenerationError(
                f"{package_id}: source registration is duplicated in files[]"
            )

        for dependency in document.get("dependencies", []):
            if not isinstance(dependency, dict):
                continue
            if dependency.get("kind") != "package" or not dependency.get(
                "required"
            ):
                continue
            target = dependency.get("provider") or dependency.get("id")
            if target not in package_ids:
                raise GenerationError(
                    f"{package_id}: required package dependency "
                    f"{target!r} is not in the controlled registration set"
                )

        records.append(
            RegistrationRecord(
                package_path=path,
                package_id=package_id,
                package_version=str(document["packageVersion"]),
                implementation_version=str(
                    document["implementationVersion"]
                ),
                type_id=str(element["id"]),
                kind=str(element["kind"]),
                binding_mode=str(element["bindingMode"]),
                definition_id=str(asset["id"]),
                registry_prefix=str(asset["registryPrefix"]),
                registration_path=registration_path,
                registration_reference=registration_reference,
                compile_paths=tuple(compile_paths),
                descriptor_signature=(
                    element_package_v1.descriptor_signature(
                        result.normalized
                    )
                ),
                symbol=symbol,
                normalized=result.normalized,
            )
        )
    return sorted(records, key=lambda item: item.package_id)


def render_header() -> str:
    return """#pragma once

#include <cstddef>

class LayerFactory;

namespace synaptome::runtime {

struct GeneratedElementPackageRegistration {
    const char* packageId;
    const char* packageVersion;
    const char* implementationVersion;
    const char* typeId;
    const char* kind;
    const char* bindingMode;
    const char* definitionId;
    const char* registryPrefix;
    const char* sourceRegistration;
    const char* descriptorSignature;
};

const GeneratedElementPackageRegistration*
generatedElementPackageRegistrations(std::size_t& count) noexcept;

void registerGeneratedElementPackages(LayerFactory& elementTypes);

} // namespace synaptome::runtime
"""


def _cpp_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=True)


def _cpp_float(value: Any) -> str:
    rendered = format(float(value), ".9g")
    if "." not in rendered and "e" not in rendered.lower():
        rendered += ".0"
    return f"{rendered}f"


def _cpp_value(value: Any, kind: str) -> str:
    if kind == "float":
        return _cpp_float(value)
    if kind == "bool":
        return "true" if value else "false"
    return f"std::string({_cpp_string(str(value))})"


def _contract_function(
    record: RegistrationRecord,
    index: int,
) -> str:
    normalized = record.normalized
    kind = normalized["kind"]
    if kind == "visual":
        cpp_kind = "ElementKind::Visual"
    elif kind == "effect":
        cpp_kind = "ElementKind::Effect"
    else:
        raise GenerationError(
            f"{record.package_id}: Runtime ElementKind cannot represent "
            f"package kind {kind!r}"
        )
    actions = ",\n".join(
        "        ActionDescriptor{"
        f"{_cpp_string(str(action['id']))}, "
        f"{_cpp_string(str(action['label']))}, "
        f"{_cpp_string(str(action['groupId']))}, \"\""
        "}"
        for action in normalized["actions"]
    )
    groups = ",\n".join(
        "        ParameterGroupDeclaration{"
        f"{_cpp_string(str(group['id']))}, "
        f"{_cpp_string(str(group['label']))}, "
        f"{_cpp_string(str(group['description']))}"
        "}"
        for group in normalized["parameterGroups"]
    )
    parameter_blocks: list[str] = []
    kind_names = {
        "float": "ParameterKind::Float",
        "bool": "ParameterKind::Bool",
        "string": "ParameterKind::String",
    }
    for parameter in normalized["parameters"]:
        parameter_kind = str(parameter["kind"])
        lines = [
            "        ParameterDeclaration value;",
            f"        value.id = {_cpp_string(str(parameter['id']))};",
            f"        value.kind = {kind_names[parameter_kind]};",
            f"        value.groupId = "
            f"{_cpp_string(str(parameter['groupId']))};",
            f"        value.label = "
            f"{_cpp_string(str(parameter['label']))};",
            "        value.defaultValue = "
            f"{_cpp_value(parameter['default'], parameter_kind)};",
        ]
        parameter_range = parameter["range"]
        if isinstance(parameter_range, dict):
            step = (
                _cpp_float(parameter_range["step"])
                if parameter_range["step"] is not None
                else "std::nullopt"
            )
            lines.append(
                "        value.range = ParameterRange{"
                f"{_cpp_float(parameter_range['min'])}, "
                f"{_cpp_float(parameter_range['max'])}, {step}"
                "};"
            )
        lines.extend(
            [
                f"        value.units = "
                f"{_cpp_string(str(parameter['units']))};",
                f"        value.description = "
                f"{_cpp_string(str(parameter['description']))};",
            ]
        )
        options = parameter["options"]
        if options:
            option_rows = ",\n".join(
                "            ParameterOption{"
                f"{_cpp_value(option['value'], parameter_kind)}, "
                f"{_cpp_string(str(option['label']))}, "
                f"{_cpp_string(str(option.get('description', '')))}"
                "}"
                for option in options
            )
            lines.extend(
                [
                    "        value.options = {",
                    option_rows + ",",
                    "        };",
                ]
            )
        option_source = parameter["optionSource"]
        if isinstance(option_source, dict):
            lines.append(
                "        value.optionSource = ParameterOptionSource{"
                f"{_cpp_string(str(option_source['id']))}, "
                f"{_cpp_string(str(option_source['valueField']))}, "
                f"{_cpp_string(str(option_source['labelField']))}"
                "};"
            )
        if parameter["quickAccessOrder"] is not None:
            lines.append(
                "        value.quickAccessOrder = "
                f"{int(parameter['quickAccessOrder'])};"
            )
        if parameter["aliases"]:
            aliases = ", ".join(
                _cpp_string(str(alias)) for alias in parameter["aliases"]
            )
            lines.append(f"        value.aliases = {{{aliases}}};")
        deprecation = parameter["deprecation"]
        if isinstance(deprecation, dict):
            lines.append(
                "        value.deprecation = ParameterDeprecation{"
                f"{_cpp_string(str(deprecation['replacementId']))}, "
                f"{_cpp_string(str(deprecation['reason']))}"
                "};"
            )
        lines.append(
            "        value.visible = "
            + ("true;" if parameter["visible"] else "false;")
        )
        lines.append(
            "        contract.parameters.parameters.push_back("
            "std::move(value));"
        )
        parameter_blocks.append(
            "    {\n" + "\n".join(lines) + "\n    }"
        )
    return (
        f"ElementTypeContract generatedContract{index}() {{\n"
        "    ElementTypeContract contract;\n"
        "    contract.element = ElementDescriptor{\n"
        f"        {_cpp_string(record.type_id)},\n"
        f"        {cpp_kind},\n"
        "        {\n"
        f"{actions}\n"
        "        },\n"
        "    };\n"
        "    contract.parameters.groups = {\n"
        f"{groups}\n"
        "    };\n"
        + "\n".join(parameter_blocks)
        + "\n    return contract;\n"
        "}\n"
    )


def render_source(records: list[RegistrationRecord]) -> str:
    declarations = "\n".join(
        f"std::unique_ptr<Layer> {record.symbol}();"
        for record in records
    )
    rows = "\n".join(
        "    {\n"
        f"        {_cpp_string(record.package_id)},\n"
        f"        {_cpp_string(record.package_version)},\n"
        f"        {_cpp_string(record.implementation_version)},\n"
        f"        {_cpp_string(record.type_id)},\n"
        f"        {_cpp_string(record.kind)},\n"
        f"        {_cpp_string(record.binding_mode)},\n"
        f"        {_cpp_string(record.definition_id)},\n"
        f"        {_cpp_string(record.registry_prefix)},\n"
        f"        {_cpp_string(record.registration_reference)},\n"
        f"        {_cpp_string(record.descriptor_signature)},\n"
        "    },"
        for record in records
    )
    preflight = "\n".join(
        "    if (elementTypes.contains("
        f"{_cpp_string(record.type_id)})) {{\n"
        "        throw std::logic_error(\n"
        f"            \"generated package type already registered: "
        f"{record.type_id}\");\n"
        "    }"
        for record in records
    )
    contracts = "\n".join(
        _contract_function(record, index)
        for index, record in enumerate(records)
    )
    calls = "\n".join(
        "    elementTypes.registerType(\n"
        f"        generatedContract{index}(),\n"
        f"        [] {{ return {record.symbol}(); }},\n"
        "        LayerFactory::ParameterBindingMode::"
        + (
            "Explicit"
            if record.binding_mode == "bind-only"
            else "LegacySetupAdapter"
        )
        + ");\n"
        "    if (!elementTypes.contains("
        f"{_cpp_string(record.type_id)})) {{\n"
        "        throw std::logic_error(\n"
        f"            \"generated registrar did not publish expected type: "
        f"{record.type_id}\");\n"
        "    }"
        for index, record in enumerate(records)
    )
    return f"""// Generated by tools/generate_element_package_registrations.py.
// Do not edit by hand.
#include "GeneratedElementPackageRegistrations.h"

#include "../visuals/LayerFactory.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

{declarations}

namespace synaptome::runtime {{
namespace {{

using synaptome::element::ActionDescriptor;
using synaptome::element::ElementDescriptor;
using synaptome::element::ElementKind;
using synaptome::element::ElementTypeContract;
using synaptome::element::ParameterDeclaration;
using synaptome::element::ParameterDeprecation;
using synaptome::element::ParameterGroupDeclaration;
using synaptome::element::ParameterKind;
using synaptome::element::ParameterOption;
using synaptome::element::ParameterOptionSource;
using synaptome::element::ParameterRange;

constexpr GeneratedElementPackageRegistration kRegistrations[] = {{
{rows}
}};

{contracts}

}} // namespace

const GeneratedElementPackageRegistration*
generatedElementPackageRegistrations(std::size_t& count) noexcept {{
    count = sizeof(kRegistrations) / sizeof(kRegistrations[0]);
    return kRegistrations;
}}

void registerGeneratedElementPackages(LayerFactory& elementTypes) {{
{preflight}
{calls}
}}

}} // namespace synaptome::runtime
"""


def _xml_escape(value: str) -> str:
    return (
        value.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
        .replace("'", "&apos;")
    )


def render_targets(records: list[RegistrationRecord]) -> str:
    items: list[str] = [
        '    <ClCompile Include="$(SynaptomeRepoRoot)\\synaptome\\src\\runtime'
        '\\GeneratedElementPackageRegistrations.cpp">',
        "      <ObjectFileName>$(IntDir)GeneratedElementPackages\\aggregate.obj"
        "</ObjectFileName>",
        "    </ClCompile>",
    ]
    for record in records:
        package_dir = record.package_path.parent
        object_dir = registration_symbol(record.package_id)
        for source_path in record.compile_paths:
            relative = _rel(source_path).replace("/", "\\")
            include_dir = _rel(package_dir / "source").replace("/", "\\")
            object_name = source_path.stem
            items.extend(
                [
                    f'    <ClCompile Include="$(SynaptomeRepoRoot)\\'
                    f'{_xml_escape(relative)}">',
                    "      <AdditionalIncludeDirectories>"
                    f"$(SynaptomeRepoRoot)\\{_xml_escape(include_dir)};"
                    "%(AdditionalIncludeDirectories)"
                    "</AdditionalIncludeDirectories>",
                    "      <ObjectFileName>$(IntDir)"
                    f"GeneratedElementPackages\\{object_dir}\\"
                    f"{_xml_escape(object_name)}.obj</ObjectFileName>",
                    "    </ClCompile>",
                ]
            )
    return (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">\n'
        "  <!-- Generated by tools/generate_element_package_registrations.py. -->\n"
        "  <ItemGroup Condition=\"'$(SynaptomeEnableGeneratedElementPackages)'"
        "=='true'\">\n"
        + "\n".join(items)
        + "\n  </ItemGroup>\n</Project>\n"
    )


def generated_outputs(
    registration_set: Path = REGISTRATION_SET,
) -> dict[Path, str]:
    records = load_records(registration_set)
    return {
        GENERATED_HEADER: render_header(),
        GENERATED_SOURCE: render_source(records),
        GENERATED_TARGETS: render_targets(records),
    }


def check_outputs(outputs: dict[Path, str]) -> list[str]:
    errors: list[str] = []
    for path, expected in outputs.items():
        try:
            observed = path.read_text(encoding="utf-8")
        except OSError as exc:
            errors.append(f"cannot read {_rel(path)}: {exc}")
            continue
        if observed == expected:
            continue
        diff = "\n".join(
            difflib.unified_diff(
                observed.splitlines(),
                expected.splitlines(),
                fromfile=_rel(path),
                tofile=f"generated {_rel(path)}",
                lineterm="",
            )
        )
        errors.append(f"{_rel(path)} is stale\n{diff}")
    return errors


def write_outputs(outputs: dict[Path, str]) -> None:
    for path, content in outputs.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--registration-set",
        type=Path,
        default=REGISTRATION_SET,
    )
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        outputs = generated_outputs(args.registration_set.resolve())
        if args.write:
            write_outputs(outputs)
            print(
                "generated controlled element-package registration: "
                + ", ".join(_rel(path) for path in outputs)
            )
            return 0
        errors = check_outputs(outputs)
    except GenerationError as exc:
        print(f"element package registration error: {exc}", file=sys.stderr)
        return 1
    if errors:
        for error in errors:
            print(
                f"element package registration error: {error}",
                file=sys.stderr,
            )
        return 1
    print("element package registration: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

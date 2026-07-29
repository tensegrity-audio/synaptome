#!/usr/bin/env python3
"""Validate the linked RuntimeCore lifecycle and composition control plane."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def function_body(source: str, signature: str) -> str:
    """Return one brace-balanced C++ function body."""
    start = source.find(signature)
    if start < 0:
        raise ValueError(f"missing function signature: {signature}")
    brace = source.find("{", start)
    if brace < 0:
        raise ValueError(f"missing function body: {signature}")

    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:index]
    raise ValueError(f"unterminated function body: {signature}")


def strip_cpp_comments(source: str) -> str:
    """Remove C++ comments while preserving strings, chars, and line layout."""
    result: list[str] = []
    index = 0
    quote = ""
    escaped = False
    while index < len(source):
        char = source[index]
        next_char = source[index + 1] if index + 1 < len(source) else ""
        if quote:
            result.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = ""
            index += 1
            continue
        if char in ('"', "'"):
            quote = char
            result.append(char)
            index += 1
            continue
        if char == "/" and next_char == "/":
            result.extend((" ", " "))
            index += 2
            while index < len(source) and source[index] not in "\r\n":
                result.append(" ")
                index += 1
            continue
        if char == "/" and next_char == "*":
            result.extend((" ", " "))
            index += 2
            while index < len(source):
                if (
                    source[index] == "*"
                    and index + 1 < len(source)
                    and source[index + 1] == "/"
                ):
                    result.extend((" ", " "))
                    index += 2
                    break
                result.append(
                    source[index] if source[index] in "\r\n" else " "
                )
                index += 1
            continue
        result.append(char)
        index += 1
    return "".join(result)


def call_expressions(source: str, callee_pattern: str) -> list[str]:
    """Return balanced call expressions matching a regex callee pattern."""
    calls: list[str] = []
    pattern = re.compile(callee_pattern + r"\s*\(")
    for match in pattern.finditer(source):
        open_paren = source.find("(", match.start(), match.end())
        depth = 0
        quote = ""
        escaped = False
        for index in range(open_paren, len(source)):
            char = source[index]
            if quote:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == quote:
                    quote = ""
                continue
            if char in ('"', "'"):
                quote = char
                continue
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    calls.append(source[match.start():index + 1])
                    break
    return calls


def first_string_literal(source: str) -> str | None:
    match = re.search(r'"((?:\\.|[^"\\])*)"', source)
    return match.group(1) if match else None


def main() -> int:
    errors: list[str] = []
    runtime_header = (ROOT / "synaptome/src/runtime/Runtime.h").read_text(encoding="utf-8")
    runtime_source = (ROOT / "synaptome/src/runtime/Runtime.cpp").read_text(encoding="utf-8")
    app = (ROOT / "synaptome/src/ofApp.cpp").read_text(encoding="utf-8")
    app_header = (ROOT / "synaptome/src/ofApp.h").read_text(encoding="utf-8")
    composition_header = (
        ROOT / "synaptome/src/runtime/CompositionLayer.h"
    ).read_text(encoding="utf-8")
    composition_types = (
        ROOT / "synaptome/src/runtime/CompositionTypes.h"
    ).read_text(encoding="utf-8")
    parameter_registry = (
        ROOT / "synaptome/src/core/ParameterRegistry.h"
    ).read_text(encoding="utf-8")
    layer_header = (
        ROOT / "synaptome/src/visuals/Layer.h"
    ).read_text(encoding="utf-8")
    action_header = (
        ROOT / "synaptome/sdk/include/synaptome/element/Action.h"
    ).read_text(encoding="utf-8")
    descriptor_header = (
        ROOT / "synaptome/sdk/include/synaptome/element/ElementDescriptor.h"
    ).read_text(encoding="utf-8")
    parameter_header = (
        ROOT / "synaptome/sdk/include/synaptome/element/Parameter.h"
    ).read_text(encoding="utf-8")
    parameter_binding_header = (
        ROOT / "synaptome/sdk/include/synaptome/element/ParameterBinding.h"
    ).read_text(encoding="utf-8")
    telemetry_header = (
        ROOT / "synaptome/sdk/include/synaptome/element/Telemetry.h"
    ).read_text(encoding="utf-8")
    action_table = (
        ROOT / "synaptome/src/runtime/ElementActionTable.h"
    ).read_text(encoding="utf-8")
    parameter_table = (
        ROOT / "synaptome/src/runtime/ElementParameterTable.h"
    ).read_text(encoding="utf-8")
    telemetry_buffer = (
        ROOT / "synaptome/src/runtime/ElementTelemetryBuffer.h"
    ).read_text(encoding="utf-8")
    geodesic_header = (
        ROOT / "synaptome/src/visuals/GeodesicLayer.h"
    ).read_text(encoding="utf-8")
    geodesic_source = (
        ROOT / "synaptome/src/visuals/GeodesicLayer.cpp"
    ).read_text(encoding="utf-8")
    game_of_life_header = (
        ROOT / "synaptome/src/visuals/GameOfLifeLayer.h"
    ).read_text(encoding="utf-8")
    game_of_life_source = (
        ROOT / "synaptome/src/visuals/GameOfLifeLayer.cpp"
    ).read_text(encoding="utf-8")
    video_grabber_header = (
        ROOT / "synaptome/src/visuals/VideoGrabberLayer.h"
    ).read_text(encoding="utf-8")
    video_grabber_source = (
        ROOT / "synaptome/src/visuals/VideoGrabberLayer.cpp"
    ).read_text(encoding="utf-8")
    video_clip_header = (
        ROOT / "synaptome/src/visuals/VideoClipLayer.h"
    ).read_text(encoding="utf-8")
    video_clip_source = (
        ROOT / "synaptome/src/visuals/VideoClipLayer.cpp"
    ).read_text(encoding="utf-8")
    midi_router = (
        ROOT / "synaptome/src/io/MidiRouter.h"
    ).read_text(encoding="utf-8") + (
        ROOT / "synaptome/src/io/MidiRouter.cpp"
    ).read_text(encoding="utf-8")
    osc_parameter_router = (
        ROOT / "synaptome/src/io/OscParameterRouter.h"
    ).read_text(encoding="utf-8") + (
        ROOT / "synaptome/src/io/OscParameterRouter.cpp"
    ).read_text(encoding="utf-8")
    control_hub_header = (
        ROOT / "synaptome/src/ui/ControlMappingHubState.h"
    ).read_text(encoding="utf-8")
    layer_factory_header = (
        ROOT / "synaptome/src/visuals/LayerFactory.h"
    ).read_text(encoding="utf-8")
    layer_factory_source = (
        ROOT / "synaptome/src/visuals/LayerFactory.cpp"
    ).read_text(encoding="utf-8")
    builtin_elements = (
        ROOT / "synaptome/src/runtime/BuiltinElements.cpp"
    ).read_text(encoding="utf-8")
    builtin_parameter_contracts = (
        ROOT
        / "synaptome/src/runtime/BuiltinElementParameterContracts.cpp"
    ).read_text(encoding="utf-8")
    builtin_parameter_contracts_header = (
        ROOT
        / "synaptome/src/runtime/BuiltinElementParameterContracts.h"
    ).read_text(encoding="utf-8")
    post_effect_header = (
        ROOT / "synaptome/src/visuals/effects/PostEffectChain.h"
    ).read_text(encoding="utf-8")
    post_effect_source = (
        ROOT / "synaptome/src/visuals/effects/PostEffectChain.cpp"
    ).read_text(encoding="utf-8")
    host_effects_header = (
        ROOT / "synaptome/src/host/HostCompositionEffects.h"
    ).read_text(encoding="utf-8")
    host_renderer_header = (
        ROOT / "synaptome/src/host/HostCompositionRenderer.h"
    ).read_text(encoding="utf-8")
    host_renderer_source = (
        ROOT / "synaptome/src/host/HostCompositionRenderer.cpp"
    ).read_text(encoding="utf-8")
    sdk_include_root = ROOT / "synaptome/sdk/include"
    sdk_public_headers = sorted(
        path
        for path in sdk_include_root.rglob("*")
        if path.is_file() and path.suffix.lower() in {".h", ".hpp"}
    )
    sdk_public_surfaces = sdk_public_headers + [
        ROOT / "synaptome/src/visuals/Layer.h",
        ROOT / "synaptome/src/visuals/LayerParameterBuilder.h",
    ]
    project = (ROOT / "synaptome/Synaptome.vcxproj").read_text(encoding="utf-8")
    project_filters = (
        ROOT / "synaptome/Synaptome.vcxproj.filters"
    ).read_text(encoding="utf-8")
    solution = (ROOT / "synaptome/Synaptome.sln").read_text(encoding="utf-8")
    core_project = (
        ROOT / "synaptome/runtime/SynaptomeRuntimeCore.vcxproj"
    ).read_text(encoding="utf-8")
    test_project = (
        ROOT / "synaptome/tests/RuntimeCoreTest/RuntimeCoreTest.vcxproj"
    ).read_text(encoding="utf-8")
    renderer_test_project = (
        ROOT
        / "synaptome/tests/HostCompositionRendererTest"
        / "HostCompositionRendererTest.vcxproj"
    ).read_text(encoding="utf-8")
    renderer_test_filters = (
        ROOT
        / "synaptome/tests/HostCompositionRendererTest"
        / "HostCompositionRendererTest.vcxproj.filters"
    ).read_text(encoding="utf-8")
    runtime_test = (
        ROOT / "tests/runtime_core_native_main.cpp"
    ).read_text(encoding="utf-8")
    renderer_test = (
        ROOT / "tests/host_composition_renderer_native_main.cpp"
    ).read_text(encoding="utf-8")
    package_bench = (
        ROOT / "tests/layer_package_bench_main.cpp"
    ).read_text(encoding="utf-8")
    declared_surface_checks = (
        ROOT / "tests/element_confidence/DeclaredSurfaceChecks.h"
    ).read_text(encoding="utf-8")

    for token in ("ofApp", "../ui/", "../io/", "PostEffectChain"):
        if token in runtime_header or token in runtime_source:
            errors.append(f"runtime lifecycle seam depends on forbidden host surface: {token}")
    for token in ('"../visuals/', '"../ofJson.h"'):
        if token in runtime_header:
            errors.append(f"runtime header bypasses the public SDK include seam: {token}")
    if "<synaptome/element/compat/Layer.h>" not in runtime_header:
        errors.append("runtime header must consume Layer through the public SDK include seam")
    for token in (
        "prepareElement",
        "releaseElement",
        "prefixIsAvailable",
        "removeById",
        "activePrefixes_",
        "stagedParameters_",
        "prepareCompositionElementReplacement",
        "replacingIds",
        "onParameterRegistryCommitted",
        "isReservedCompositionParameter",
        "instanceId",
        "ElementErrorCode",
        "compositionSnapshot",
        "compositionLayerSnapshot",
        "invokeCompositionAction",
        "compositionElementTelemetry",
        "adoptPreparedElement",
        "releasePreparedElement",
        "updateCompositionElements",
        "drawCompositionElement",
        "shutdownComposition",
        "resolveEffectCoverage",
        "assignCompositionEntry",
        "setCompositionLayerActive",
        "setCompositionLayerLabel",
        "setCompositionLayerCoverage",
        "clearCompositionLayer",
    ):
        if token not in runtime_source and token not in runtime_header:
            errors.append(f"runtime lifecycle seam is missing {token}")
    for token in (
        "enum class CompositionKind",
        "struct CompositionAssignment",
        "struct CompositionLayerSnapshot",
        "struct CompositionSnapshot",
        "enum class CompositionMutationError",
        "struct CompositionMutationResult",
        "enum class CompositionActionError",
        "struct CompositionActionResult",
        "enum class CompositionTelemetryError",
        "struct CompositionTelemetryResult",
    ):
        if token not in composition_types:
            errors.append(f"runtime composition control plane is missing {token}")
    for pattern, description in (
        (
            r"(?m)^\s*CompositionLayer\*\s+compositionLayer\s*\(",
            "mutable composition-layer accessor",
        ),
        (
            r"(?m)^\s*bool\s+adoptPreparedElement\s*\(",
            "legacy bool composition adoption",
        ),
        (
            r"(?m)^\s*void\s+releaseCompositionElement\s*\(",
            "element-only composition release",
        ),
    ):
        if re.search(pattern, runtime_header):
            errors.append(f"Runtime still exposes transitional {description}")
    for token, description in (
        ("using CompositionLayers", "public live composition-array alias"),
        ("compositionLayersForHost", "live composition aggregate accessor"),
    ):
        if token in runtime_header:
            errors.append(f"Runtime still exposes transitional {description}")
    if re.search(
        r"(?m)^\s*const\s+CompositionLayer\s*\*\s+compositionLayer\s*\(",
        runtime_header,
    ):
        errors.append("Runtime still exposes a public live CompositionLayer accessor")
    if "coverageParamValue" in composition_header:
        errors.append(
            "composition record still duplicates typed coverage as a host float"
        )
    snapshot_start = composition_types.find("struct CompositionLayerSnapshot")
    snapshot_end = composition_types.find(
        "enum class CompositionMutationError",
        snapshot_start,
    )
    if snapshot_start < 0 or snapshot_end < 0:
        errors.append("could not inspect Runtime composition snapshot DTOs")
        snapshot_surface = ""
    else:
        snapshot_surface = composition_types[snapshot_start:snapshot_end]
    for token in (
        "zeroBasedIndex",
        "occupied",
        "hasElement",
        "kind",
        "definitionId",
        "label",
        "typeId",
        "registryPrefix",
        "active",
        "opacity",
        "coverage",
        "std::vector<element::ActionDescriptor> actions",
        "std::array<CompositionLayerSnapshot",
    ):
        if token not in snapshot_surface:
            errors.append(f"composition snapshot DTOs are missing {token}")
    for token in (
        "*",
        "&",
        "ofFbo",
        "ParameterRegistry",
        "LayerFactory",
        "std::function",
        "Creator",
        "ofApp",
        "unique_ptr",
        "shared_ptr",
        "Telemetry",
    ):
        if token in snapshot_surface:
            errors.append(f"composition snapshot DTOs expose forbidden ownership: {token}")

    descriptor_start = descriptor_header.find("struct ActionDescriptor")
    descriptor_end = descriptor_header.find(
        "enum class ElementKind",
        descriptor_start,
    )
    if descriptor_start < 0 or descriptor_end < 0:
        errors.append("could not inspect the public static action descriptor")
        action_descriptor_surface = ""
    else:
        action_descriptor_surface = descriptor_header[
            descriptor_start:descriptor_end
        ]
    for token in (
        "std::string id;",
        "std::string label;",
        "std::string groupId;",
        "std::string description;",
    ):
        if token not in action_descriptor_surface:
            errors.append(f"static action descriptor is missing {token}")
    if "std::string group;" in action_descriptor_surface:
        errors.append(
            "static action descriptor must use stable groupId instead of group"
        )
    for token in (
        "*",
        "&",
        "std::function",
        "ActionHandler",
        "Layer",
        "Runtime",
        "ParameterRegistry",
        "ofApp",
    ):
        if token in action_descriptor_surface:
            errors.append(
                f"static action descriptor exposes forbidden ownership: {token}"
            )
    for token in (
        "using ActionHandler = std::function<ActionExecutionResult()>",
        "class ActionRegistrar",
        "virtual void bind(",
    ):
        if token not in action_header:
            errors.append(f"public action binding contract is missing {token}")
    for token in (
        "enum class ElementKind",
        "Visual",
        "Effect",
        "struct ElementDescriptor",
        "std::string typeId;",
        "ElementKind kind",
        "std::vector<ActionDescriptor> actions;",
    ):
        if token not in descriptor_header:
            errors.append(f"public element descriptor is missing {token}")
    for forbidden in (
        "std::function",
        "ActionHandler",
        "Creator",
        "Layer",
        "Runtime",
        "ParameterRegistry",
        "ofFbo",
        "ofJson",
    ):
        if forbidden in descriptor_header:
            errors.append(
                "public element descriptor exposes forbidden ownership: "
                + forbidden
            )
    for token in (
        "enum class ParameterKind",
        "using ParameterValue = std::variant<float, bool, std::string>",
        "struct ParameterRange",
        "std::optional<float> step;",
        "struct ParameterOption",
        "struct ParameterOptionSource",
        "std::string valueField;",
        "std::string labelField;",
        "struct ParameterGroupDeclaration",
        "struct ParameterDeprecation",
        "struct ParameterDeclaration",
        "std::string groupId;",
        "ParameterValue defaultValue",
        "std::optional<ParameterRange> range;",
        "std::vector<ParameterOption> options;",
        "std::optional<ParameterOptionSource> optionSource;",
        "std::optional<int> quickAccessOrder;",
        "std::vector<std::string> aliases;",
        "std::optional<ParameterDeprecation> deprecation;",
        "struct ParameterDeclarationSet",
        "std::vector<ParameterGroupDeclaration> groups;",
        "std::vector<ParameterDeclaration> parameters;",
        "struct ElementTypeContract",
        "ElementDescriptor element;",
        "ParameterDeclarationSet parameters;",
    ):
        if token not in parameter_header:
            errors.append(f"public parameter declaration DTO is missing {token}")
    for forbidden in (
        "*",
        "&",
        "std::function",
        "ParameterBinder",
        "ParameterRegistry",
        "ActionHandler",
        "Creator",
        "Layer",
        "Runtime",
        "ofFbo",
        "ofJson",
        "unique_ptr",
        "shared_ptr",
    ):
        if forbidden in parameter_header:
            errors.append(
                "public parameter declaration DTO exposes forbidden "
                "binding/ownership: " + forbidden
            )
    for token in (
        "class ParameterBinder",
        "virtual void bind(std::string parameterId, float& storage) = 0;",
        "virtual void bind(std::string parameterId, bool& storage) = 0;",
        "virtual void bind(std::string parameterId, std::string& storage) = 0;",
        "class ParameterBindable",
        "virtual void bindParameters(ParameterBinder& binder) = 0;",
    ):
        if token not in parameter_binding_header:
            errors.append(f"public parameter binding seam is missing {token}")
    for token in (
        "class ElementParameterTable final : public element::ParameterBinder",
        "void bindLegacyRegistry(",
        "contractError() const",
        "applyDeclarationDefaults() const",
        "void populate(",
        "element registered an undeclared parameter binding",
        "element registered a duplicate parameter binding",
        "element registered a parameter binding with the wrong",
        "element did not bind declared parameter",
        "element bound one storage address to multiple",
    ):
        if token not in parameter_table:
            errors.append(f"runtime parameter binding table is missing {token}")
    for token in (
        "enum class ParameterDeclarationState",
        "LegacySetupDiscovery",
        "Declared",
        "enum class ParameterBindingMode",
        "Explicit",
        "LegacySetupAdapter",
        "struct ElementTypeContractRecord",
        "ParameterDeclarationState state",
        "ParameterBindingMode bindingMode",
        "synaptome::element::ElementTypeContract contract;",
        "synaptome::element::ElementTypeContract contract,",
        "const ElementTypeContractRecord* typeContract(",
        "std::vector<ElementTypeContractRecord> typeContracts() const;",
    ):
        if token not in layer_factory_header:
            errors.append(
                "scoped element registry is missing parameter declaration "
                "migration/inspection API: " + token
            )
    for token in (
        "ParameterDeclarationState::LegacySetupDiscovery",
        "ParameterDeclarationState::Declared",
        "parameterContractError(record.contract.parameters)",
        "registerTypeRecord(std::move(record), std::move(creator))",
        "entry.typeContract.contract.element",
        "result.push_back(entry.typeContract)",
    ):
        if token not in layer_factory_source:
            errors.append(
                "scoped element registry is missing atomic parameter "
                "contract behavior: " + token
            )
    for token in (
        "ParameterBindingMode::Explicit",
        "parameterTable.bindLegacyRegistry(",
        "parameterTable.populate(",
    ):
        if token not in runtime_source:
            errors.append(
                "Runtime is missing declared parameter binding mode behavior: "
                + token
            )
    for token in (
        "builtinElementParameterDeclarations(typeId)",
        "ParameterBindingMode::",
        "LegacySetupAdapter",
    ):
        if token not in builtin_elements:
            errors.append(
                "built-in registration is missing authoritative generated "
                "parameter declarations: " + token
            )
    for token in (
        "builtinElementParameterDeclarations(",
        "builtinElementParameterTypeIds()",
    ):
        if (
            token not in builtin_parameter_contracts
            and token not in builtin_parameter_contracts_header
        ):
            errors.append(
                "compiled built-in parameter contract loader is missing " +
                token
            )
    if "registerActions(" not in layer_header:
        errors.append("compatibility Layer is missing optional action registration")
    for token in (
        "contractError() const",
        "entry.descriptor.id",
        "entry.handler",
        "duplicate action binding",
        "undeclared action binding",
        "did not bind declared action",
        "descriptors() const",
        "find(",
    ):
        if token not in action_table:
            errors.append(f"runtime live action table is missing {token}")

    telemetry_entry_start = telemetry_header.find("struct TelemetryEntry")
    telemetry_entry_end = telemetry_header.find(
        "class TelemetrySink",
        telemetry_entry_start,
    )
    if telemetry_entry_start < 0 or telemetry_entry_end < 0:
        errors.append("could not inspect the public telemetry entry DTO")
        telemetry_entry_surface = ""
    else:
        telemetry_entry_surface = telemetry_header[
            telemetry_entry_start:telemetry_entry_end
        ]
    for token in (
        "using TelemetryValue =",
        "std::variant<bool, std::int64_t, double, std::string>",
        "std::string id;",
        "std::string label;",
        "std::string groupId;",
        "std::string description;",
        "TelemetryValue value;",
        "class TelemetrySink",
        "virtual void add(TelemetryEntry entry) = 0;",
    ):
        if token not in telemetry_header:
            errors.append(f"public element telemetry contract is missing {token}")
    for token in (
        "*",
        "&",
        "std::function",
        "Layer",
        "Runtime",
        "ParameterRegistry",
        "ofJson",
        "HudFeedRegistry",
    ):
        if token in telemetry_entry_surface:
            errors.append(
                f"public telemetry entry exposes forbidden ownership: {token}"
            )
    if not re.search(
        r"virtual\s+void\s+collectTelemetry\s*\(\s*"
        r"synaptome::element::TelemetrySink&\s+sink\s*\)\s*const",
        layer_header,
    ):
        errors.append(
            "compatibility Layer must expose optional const telemetry collection"
        )
    for token in (
        "class ElementTelemetryBuffer final : public element::TelemetrySink",
        "void add(element::TelemetryEntry entry) override",
        "contractError() const",
        "isValidId(",
        "entry.id",
        "entry.label.empty()",
        "isValidGroupId(entry.groupId)",
        "invalid telemetry group ID",
        "duplicate telemetry ID",
        "takeEntries() noexcept",
    ):
        if token not in telemetry_buffer:
            errors.append(f"runtime telemetry buffer is missing {token}")
    telemetry_group_start = telemetry_buffer.find(
        "static bool isValidGroupId("
    )
    telemetry_group_end = telemetry_buffer.find(
        "std::vector<element::TelemetryEntry> entries_",
        telemetry_group_start,
    )
    if telemetry_group_start < 0 or telemetry_group_end < 0:
        errors.append("could not inspect telemetry groupId validation")
        telemetry_group_surface = ""
    else:
        telemetry_group_surface = telemetry_buffer[
            telemetry_group_start:telemetry_group_end
        ]
    for forbidden in ("character == '.'", "character == '_'", "character == '-'"):
        if forbidden in telemetry_group_surface:
            errors.append(
                "telemetry groupId must remain a single alphanumeric "
                f"lowerCamel segment: {forbidden}"
            )
    for token in (
        "enum class CompositionTelemetryError",
        "None",
        "IndexOutOfRange",
        "SlotEmpty",
        "KindMismatch",
        "ContractViolation",
        "CollectionFailure",
        "struct CompositionTelemetryResult",
        "std::vector<element::TelemetryEntry> entries",
        "const element::TelemetryEntry* find(",
        "const T* valueAs(",
        "std::get_if<T>",
    ):
        if token not in composition_types:
            errors.append(f"Runtime telemetry result DTO is missing {token}")
    for token in (
        "CompositionTelemetryResult Runtime::compositionElementTelemetry(",
        "CompositionTelemetryError::IndexOutOfRange",
        "CompositionTelemetryError::SlotEmpty",
        "CompositionTelemetryError::KindMismatch",
        "CompositionTelemetryError::ContractViolation",
        "CompositionTelemetryError::CollectionFailure",
        "layer.element_->collectTelemetry(buffer)",
        "buffer.contractError()",
        "buffer.takeEntries()",
        "catch (const std::exception& error)",
        "catch (...)",
    ):
        if token not in runtime_source:
            errors.append(f"Runtime on-demand telemetry query is missing {token}")
    for token in ("VideoGrabberLayer", "VideoClipLayer", "GeodesicLayer"):
        if token in runtime_header or token in runtime_source:
            errors.append(
                "generic Runtime telemetry query depends on a concrete "
                f"element: {token}"
            )
    for header, label in (
        (video_grabber_header, "webcam"),
        (video_clip_header, "video clip"),
    ):
        if not re.search(r"\bcollectTelemetry\s*\(", strip_cpp_comments(header)):
            errors.append(f"{label} element is missing telemetry collection")
    try:
        webcam_telemetry_body = function_body(
            strip_cpp_comments(video_grabber_source),
            "void VideoGrabberLayer::collectTelemetry",
        )
        clip_telemetry_body = function_body(
            strip_cpp_comments(video_clip_source),
            "void VideoClipLayer::collectTelemetry",
        )
    except ValueError as exc:
        errors.append(str(exc))
        webcam_telemetry_body = ""
        clip_telemetry_body = ""
    concrete_telemetry_contracts = (
        (
            webcam_telemetry_body,
            "media.sourceLabel",
            '"media"',
            "currentDeviceLabel()",
            "webcam source label",
        ),
        (
            webcam_telemetry_body,
            "media.captureInitialized",
            '"media"',
            "isCaptureInitialized()",
            "webcam capture readiness",
        ),
        (
            clip_telemetry_body,
            "media.sourceLabel",
            '"media"',
            "currentClipLabel()",
            "video clip source label",
        ),
    )
    for body, telemetry_id, group_literal, value_expression, label in (
        concrete_telemetry_contracts
    ):
        if body.count(f'"{telemetry_id}"') != 1:
            errors.append(
                f"{label} must publish exact telemetry ID {telemetry_id} once"
            )
        if group_literal not in body:
            errors.append(f"{label} must retain stable media groupId")
        if value_expression not in body:
            errors.append(
                f"{label} must collect the cached typed value via "
                f"{value_expression}"
            )
    for token in (
        "prefix + \".subdivisions\"",
        "&paramSubdivisions_",
        "subdivisionsMeta.range.min = 1.0f",
        "subdivisionsMeta.range.max = 4.0f",
        "subdivisionsMeta.range.step = 1.0f",
    ):
        if token not in geodesic_source:
            errors.append(
                "Geodesic durable subdivision parameter contract is missing "
                f"{token}"
            )
    if "float paramSubdivisions_ = 2.0f" not in geodesic_header:
        errors.append(
            "Geodesic subdivisions must retain a durable registered parameter"
        )
    for token in (
        "GeodesicLayer",
        "GameOfLifeLayer",
    ):
        if token in runtime_header or token in runtime_source:
            errors.append(
                f"generic Runtime action dispatch depends on a concrete element: {token}"
            )
    if "dynamic_cast<element::ParameterBindable*>" not in re.sub(
        r"\s+", "", runtime_source
    ):
        errors.append(
            "declared Runtime parameter setup must query the public "
            "ParameterBindable capability"
        )
    for header, label in (
        (geodesic_header, "Geodesic"),
        (game_of_life_header, "Game of Life"),
    ):
        if not re.search(r"\bregisterActions\s*\(", strip_cpp_comments(header)):
            errors.append(
                f"{label} live action implementation is missing binding"
            )
    try:
        geodesic_actions_body = function_body(
            strip_cpp_comments(geodesic_source),
            "void GeodesicLayer::registerActions",
        )
        game_of_life_actions_body = function_body(
            strip_cpp_comments(game_of_life_source),
            "void GameOfLifeLayer::registerActions",
        )
    except ValueError as exc:
        errors.append(str(exc))
        geodesic_action_calls: list[str] = []
        game_of_life_action_calls: list[str] = []
    else:
        geodesic_action_calls = call_expressions(
            geodesic_actions_body,
            r"\bregistrar\s*\.\s*bind",
        )
        game_of_life_action_calls = call_expressions(
            game_of_life_actions_body,
            r"\bregistrar\s*\.\s*bind",
        )

    action_success_pattern = re.compile(
        r"\bActionExecutionResult\s*::\s*succeeded\s*\(\s*\)"
    )
    concrete_action_contracts = (
        (
            geodesic_action_calls,
            "subdivision.increment",
            "geometry",
            "incrementSubdivision",
            ("decrementSubdivision",),
            "Geodesic",
        ),
        (
            geodesic_action_calls,
            "subdivision.decrement",
            "geometry",
            "decrementSubdivision",
            ("incrementSubdivision",),
            "Geodesic",
        ),
        (
            game_of_life_action_calls,
            "simulation.randomize",
            "simulation",
            "randomize",
            (),
            "Game of Life",
        ),
    )
    for calls, action_id, _group_id, handler_name, forbidden_handlers, label in (
        concrete_action_contracts
    ):
        matching_calls = [
            call
            for call in calls
            if first_string_literal(call) == action_id
        ]
        if len(matching_calls) != 1:
            errors.append(
                f"{label} must bind action {action_id} exactly once"
            )
            continue
        action_call = matching_calls[0]
        handler_pattern = re.compile(
            rf"(?:\bthis\s*->\s*)?\b{re.escape(handler_name)}\s*"
            r"\(\s*\)"
        )
        if not handler_pattern.search(action_call):
            errors.append(
                f"{label} action {action_id} is not paired with "
                f"{handler_name}()"
            )
        for forbidden_handler in forbidden_handlers:
            if re.search(
                rf"(?:\bthis\s*->\s*)?\b"
                rf"{re.escape(forbidden_handler)}\s*\(\s*\)",
                action_call,
            ):
                errors.append(
                    f"{label} action {action_id} is also wired to "
                    f"{forbidden_handler}()"
                )
        if not action_success_pattern.search(action_call):
            errors.append(
                f"{label} action {action_id} must report successful execution"
            )

    for call in game_of_life_action_calls:
        registered_id = first_string_literal(call)
        if registered_id and "reseed" in registered_id.split("."):
            errors.append(
                "Game of Life must not expose a .reseed live action route: "
                f"{registered_id}"
            )
    for token in (
        '"subdivision.increment"',
        '"Increase Subdivision"',
        '"subdivision.decrement"',
        '"Decrease Subdivision"',
        '"geometry"',
        '"simulation.randomize"',
        '"Randomize Simulation"',
        '"simulation"',
    ):
        if token not in builtin_elements:
            errors.append(
                "controlled static action descriptors are missing " + token
            )
    for token in (
        'result.stage = "actions"',
        "registerActions(result.stagedActions_)",
        "stagedActions_.contractError()",
        "state.actions = layer.actions_.descriptors()",
        "CompositionActionError::IndexOutOfRange",
        "CompositionActionError::SlotEmpty",
        "CompositionActionError::KindMismatch",
        "CompositionActionError::ActionNotFound",
        "CompositionActionError::Rejected",
        "CompositionActionError::ExecutionFailure",
        "layer.actions_.find(actionId)",
        "ActionExecutionStatus::Succeeded",
        "ActionExecutionStatus::Rejected",
        "ActionExecutionStatus::Failed",
        "catch (const std::exception& error)",
        "catch (...)",
    ):
        if token not in runtime_source:
            errors.append(f"Runtime live action control plane is missing {token}")
    if composition_header.find("std::unique_ptr<Layer> element_") > composition_header.find(
        "ElementActionTable actions_"
    ):
        errors.append(
            "live action handlers must be declared after the element so they "
            "are destroyed first"
        )
    if runtime_header.find("std::unique_ptr<Layer> element_") > runtime_header.find(
        "ElementActionTable stagedActions_"
    ):
        errors.append(
            "prepared action handlers must be declared after the candidate "
            "element so they are destroyed first"
        )
    for token in (
        "layer->actions_.swap(prepared.stagedActions_)",
        "retiredActions.swap(layer->actions_)",
        "retiredElement.swap(layer->element_)",
    ):
        if token not in runtime_source:
            errors.append(f"Runtime action/element lifetime ordering is missing {token}")
    for token in (
        "ElementResult prepareCompositionElementReplacement(",
        "CompositionSnapshot compositionSnapshot() const",
        "std::optional<CompositionLayerSnapshot> compositionLayerSnapshot(",
        "CompositionActionResult invokeCompositionAction(",
        "CompositionTelemetryResult compositionElementTelemetry(",
    ):
        if token not in runtime_header:
            errors.append(f"Runtime immutable query/host seam is missing {token}")
    if "legacyCompositionElementForHost" in runtime_header + runtime_source + app:
        errors.append(
            "the zero-caller mutable composition-element seam must remain retired"
        )
    for surface_name, surface in (
        ("Runtime header", runtime_header),
        ("Runtime source", runtime_source),
        ("host", app_header + app),
        ("RuntimeCore test", runtime_test),
    ):
        if "compositionElementForHost" in surface:
            errors.append(
                "the read-only concrete composition-element seam must remain "
                f"retired from {surface_name}"
            )
    if re.search(
        r"\bprepareElementReplacement\s*\(",
        runtime_header + runtime_source + app + runtime_test,
    ):
        errors.append(
            "pointer-addressed element replacement API must remain removed"
        )
    if not re.search(
        r"ElementResult\s+prepareCompositionElementReplacement\s*\(\s*"
        r"std::size_t\s+zeroBasedIndex\s*,\s*"
        r"const\s+ElementRequest&\s+request\s*,\s*"
        r"const\s+ProgressCallback&\s+progress\s*=\s*\{\}\s*\)",
        runtime_header,
    ):
        errors.append(
            "Runtime replacement preparation must be addressed by zero-based "
            "composition-layer index"
        )
    if "struct ConsoleSlot" in app_header:
        errors.append("ofApp still defines the composition-layer storage record")
    if "CompositionLayerSnapshot" not in app_header:
        errors.append("ofApp metadata helpers must consume composition snapshots")
    for token in (
        "Runtime::CompositionLayers",
        "compositionLayersForHost",
    ):
        if token in app_header or token in app:
            errors.append(f"ofApp still consumes the live composition surface: {token}")
    if re.search(r"runtime::CompositionLayer(?!Snapshot)", app_header):
        errors.append("ofApp still names the live Runtime composition record")
    if (
        "LayerFactory elementTypes_" not in app_header
        or "Runtime runtime_{elementTypes_, paramRegistry}" not in app_header
    ):
        errors.append("ofApp must own and inject one scoped element type registry")
    if "std::unique_ptr<Layer> element_" not in composition_header:
        errors.append("runtime composition layer must own its element privately")
    for token in (
        "CompositionRenderTargets",
        "compositionRenderTargetsForHost",
        "layerFbo",
        "upstreamFbo",
        "effectFbo",
    ):
        if (
            token in runtime_header
            or token in runtime_source
            or token in composition_header
            or token in runtime_test
        ):
            errors.append(
                "RuntimeCore must not own or expose host composition render "
                f"targets: {token}"
            )
    if "ofFbo" in runtime_header + runtime_source + composition_header:
        errors.append(
            "RuntimeCore must not depend on the host composition FBO type"
        )
    if "struct CompositionCoverageWindow" not in composition_types:
        errors.append("runtime composition types are missing the effect coverage window")
    for token in ("CoverageWindow", "resolveCoverageWindow"):
        if token in post_effect_header or token in post_effect_source:
            errors.append(
                f"PostEffectChain still owns legacy composition policy: {token}"
            )
    if "postEffects.resolveCoverageWindow" in app:
        errors.append(
            "host still resolves effect coverage through PostEffectChain"
        )
    for token in (
        "class HostCompositionEffects",
        "isConsoleRouted",
        "defaultCoverageForType",
        "applySlot",
        "applyGlobal",
    ):
        if token not in host_effects_header:
            errors.append(
                f"host composition effect interface is missing {token}"
            )
    for pattern, description in (
        (
            r"virtual\s+bool\s+isConsoleRouted\s*\(\s*"
            r"std::string_view\b[^)]*\)\s*const\s+noexcept\s*=\s*0",
            "noexcept console-route query",
        ),
        (
            r"virtual\s+float\s+defaultCoverageForType\s*\(\s*"
            r"std::string_view\b[^)]*\)\s*const\s+noexcept\s*=\s*0",
            "noexcept default-coverage query",
        ),
        (
            r"virtual\s+bool\s+applySlot\s*\(\s*std::string_view\b[^,]*,\s*"
            r"const\s+ofFbo\s*&\s*\w+\s*,\s*ofFbo\s*&\s*\w+\s*\)\s*=\s*0",
            "slot-effect application",
        ),
        (
            r"virtual\s+void\s+applyGlobal\s*\(\s*ofFbo\s*&\s*\w+\s*\)\s*=\s*0",
            "global-effect application",
        ),
    ):
        if not re.search(pattern, host_effects_header, re.DOTALL):
            errors.append(
                "host composition effect interface is missing canonical "
                + description
            )
    if (
        "routeStateForType" in host_effects_header
        or "routeStateForType" in host_renderer_header + host_renderer_source
    ):
        errors.append(
            "host composition rendering must consume a boolean route query, "
            "not a route enum/integer"
        )
    for token in (
        "HostCompositionEffects",
        "isConsoleRouted",
        "defaultCoverageForType",
        "applySlot",
        "applyGlobal",
    ):
        if token not in post_effect_header + post_effect_source:
            errors.append(
                f"PostEffectChain is missing host effect-pipeline contract {token}"
            )
    for token in (
        "enum class RenderStatus",
        "Rendered",
        "InvalidViewport",
        "CompositeAllocationFailed",
        "class HostCompositionRenderer",
        "HostCompositionRenderer(",
        "RenderStatus render(",
        "drawLatest(",
        "drawPreview(",
        "hasFrame(",
        "releaseGraphicsResources(",
        "runtime::Runtime& runtime_",
        "HostCompositionEffects& effects_",
    ):
        if token not in host_renderer_header + host_renderer_source:
            errors.append(f"host composition renderer is missing {token}")
    renderer_class_surface = host_renderer_header[
        host_renderer_header.find("class HostCompositionRenderer"):
    ]
    renderer_public_surface = renderer_class_surface.split("private:", 1)[0]
    if "ofFbo" in renderer_public_surface:
        errors.append(
            "HostCompositionRenderer must not expose a composition FBO"
        )
    renderer_private_surface = (
        renderer_class_surface.split("private:", 1)[1]
        if "private:" in renderer_class_surface
        else ""
    )
    private_fbo_members = re.findall(
        r"^\s*ofFbo\s+[A-Za-z_][A-Za-z0-9_]*\s*;",
        renderer_private_surface,
        re.MULTILINE,
    )
    if len(private_fbo_members) < 4 or "std::array<" not in renderer_private_surface:
        errors.append(
            "HostCompositionRenderer must privately own the per-slot and "
            "composite render targets"
        )
    for token in (
        "compositionSnapshot",
        "resolveEffectCoverage",
        "resizeCompositionElements",
        "drawCompositionElement",
        "isConsoleRouted",
        "defaultCoverageForType",
        "applySlot",
        "applyGlobal",
    ):
        if token not in host_renderer_source:
            errors.append(
                f"HostCompositionRenderer is missing delegated render behavior: {token}"
            )
    for token in (
        "PostEffectChain",
        "HostCompositionRenderer",
        "HostCompositionEffects",
    ):
        if token in runtime_header + runtime_source + composition_header:
            errors.append(
                f"RuntimeCore depends on forbidden host renderer/effect surface: {token}"
            )
    if "PostEffectChain" in host_renderer_header + host_renderer_source:
        errors.append(
            "HostCompositionRenderer must depend on HostCompositionEffects, "
            "not concrete PostEffectChain"
        )
    for header_path in sdk_public_surfaces:
        header_text = header_path.read_text(encoding="utf-8")
        for token in (
            "CompositionKind",
            "CompositionAssignment",
            "CompositionLayerSnapshot",
            "CompositionSnapshot",
            "CompositionMutationError",
            "CompositionMutationResult",
            "CompositionRenderTargets",
            "CompositionCoverageWindow",
            "resolveEffectCoverage",
            "PostEffectChain",
            "HostCompositionRenderer",
            "HostCompositionEffects",
        ):
            if token in header_text:
                relative_path = header_path.relative_to(ROOT)
                errors.append(
                    f"public Element SDK header leaks runtime composition "
                    f"ownership ({token}): {relative_path}"
                )
    for token in (
        "runtime_.prepareElement",
        "runtime_.prepareCompositionElementReplacement",
        "runtime_.adoptPreparedElement",
        "runtime_.assignCompositionEntry",
        "runtime_.setCompositionLayerActive",
        "runtime_.setCompositionLayerLabel",
        "runtime_.setCompositionLayerCoverage",
        "runtime_.clearCompositionLayer",
        "runtime_.compositionSnapshot",
        "runtime_.compositionLayerSnapshot",
        "runtime_.invokeCompositionAction",
        "runtime_.compositionElementTelemetry",
        "runtime_.updateCompositionElements",
        "runtime_.shutdownComposition",
    ):
        if token not in app:
            errors.append(f"ofApp must delegate generic composition behavior: {token}")
    for token in (
        "compositionRenderer_.render(",
        "compositionRenderer_.drawLatest(",
        "compositionRenderer_.drawPreview(",
        "compositionRenderer_.hasFrame(",
        "compositionRenderer_.releaseGraphicsResources(",
    ):
        if token not in app:
            errors.append(f"ofApp must delegate composition rendering: {token}")
    for token in (
        '#include "host/HostCompositionRenderer.h"',
        "synaptome::host::HostCompositionRenderer compositionRenderer_",
    ):
        if token not in app_header:
            errors.append(
                f"ofApp is missing its host composition renderer dependency: {token}"
            )
    for token in (
        "compositionRenderTargetsForHost",
        "CompositionRenderTargets",
        "compositeFbo",
        "ensureConsoleLayerViewports",
        "ensureSlotFbo",
        "applyEffectSlot",
    ):
        if token in app_header + app:
            errors.append(
                f"ofApp still owns retired composition render behavior: {token}"
            )
    for token in (
        'telemetry.valueAs<std::string>(',
        '"media.sourceLabel"',
        "telemetry.valueAs<bool>(",
        '"media.captureInitialized"',
    ):
        if token not in app:
            errors.append(
                "host media status must consume exact typed Runtime telemetry: "
                f"{token}"
            )
    for token in (
        "currentDeviceLabel()",
        "isCaptureInitialized()",
        "currentClipLabel()",
        "dynamic_cast<const VideoGrabberLayer*>",
        "dynamic_cast<const VideoClipLayer*>",
    ):
        if token in app:
            errors.append(
                "host media status still inspects a concrete element: "
                f"{token}"
            )
    for token in (
        "GridLayer* gridLayer",
        "GeodesicLayer* geodesicLayer",
        "PerlinNoiseLayer* perlinLayer",
        "GameOfLifeLayer* gameOfLifeLayer",
        "refreshLayerReferences",
    ):
        if token in app_header or token in app:
            errors.append(
                "ofApp must not retain derived element-pointer caches or their "
                f"refresh path: {token}"
            )
    for token in (
        "void registerPerlinMidi(PerlinNoiseLayer*",
        "void registerGameOfLifeMidi(GameOfLifeLayer*",
    ):
        if token in app_header or token in app:
            errors.append(
                "element-specific MIDI registration still accepts a concrete "
                f"element pointer: {token}"
            )
    if "runtime_.compositionLayer(" in app or "runtime_.releaseCompositionElement" in app:
        errors.append("ofApp still calls a transitional mutable Runtime composition API")
    if re.search(r"(?:\.|->)(?:layerFbo|upstreamFbo|effectFbo)\b", app):
        errors.append("ofApp directly consumes a composition render target")
    if "ofFbo" in app_header + app:
        errors.append("ofApp must not directly own or manipulate an FBO")
    if "std::unique_ptr<Layer> element;" in runtime_header:
        errors.append("prepared element ownership must not be publicly transferable")
    if "replacingIds" not in parameter_registry or "void swap(" not in parameter_registry:
        errors.append("parameter registry is missing the strong replacement primitive")
    if (
        "onParameterRegistryCommitted" not in layer_header
        or "noexcept" not in layer_header[
            layer_header.find("onParameterRegistryCommitted"):
            layer_header.find("onParameterRegistryCommitted") + 160
        ]
    ):
        errors.append("element contract is missing the no-fail live-registry commit hook")
    clear_start = app.find("bool ofApp::clearConsoleSlot")
    clear_end = app.find("void ofApp::persistConsoleAssignments", clear_start)
    clear_body = app[clear_start:clear_end]
    if "runtime_.clearCompositionLayer" not in clear_body:
        errors.append("host clear flow must delegate assignment/parameter removal to Runtime")
    if "paramRegistry.removeById" in clear_body:
        errors.append("host clear flow still removes Runtime-owned parameters directly")
    clear_commit = clear_body.find("runtime_.clearCompositionLayer")
    clear_invalidation = clear_body.find(
        "invalidateParameterRegistryStorage",
        clear_commit,
    )
    clear_midi_unbind = clear_body.find(
        "midi.unbindByPrefix",
        clear_commit,
    )
    if min(clear_commit, clear_invalidation, clear_midi_unbind) < 0:
        errors.append(
            "host clear flow must commit, invalidate registry consumers, "
            "and retire mappings"
        )
    elif not clear_commit < clear_invalidation < clear_midi_unbind:
        errors.append(
            "host clear flow must not retire mappings before Runtime commits "
            "and registry consumers are invalidated"
        )
    if "return false;" not in clear_body or "return true;" not in clear_body:
        errors.append("host clear flow must propagate Runtime mutation failure")

    add_start = app.find("bool ofApp::addAssetToConsoleLayer")
    add_end = app.find("void ofApp::openAssetBrowserForConsole", add_start)
    add_body = app[add_start:add_end]
    for forbidden in ("LayerFactory::instance().create", "l->configure(", "l->setup("):
        if forbidden in add_body:
            errors.append(f"host add flow still performs generic lifecycle operation: {forbidden}")
    replacement_prepare = add_body.find(
        "runtime_.prepareCompositionElementReplacement"
    )
    replacement_adopt = add_body.find("runtime_.adoptPreparedElement")
    destructive_clear = add_body.find("clearConsoleSlot(idx)")
    registry_invalidation = add_body.find(
        "invalidateParameterRegistryStorage",
        replacement_adopt,
    )
    post_adoption_snapshot = add_body.find(
        "runtime_.compositionLayerSnapshot",
        replacement_adopt,
    )
    perlin_registry_bind = add_body.find(
        "registerPerlinMidi(*installed)",
        post_adoption_snapshot,
    )
    game_of_life_registry_bind = add_body.find(
        "registerGameOfLifeMidi(*installed)",
        post_adoption_snapshot,
    )
    game_of_life_randomize = add_body.find(
        "randomizeGameOfLifeAtSlot",
        post_adoption_snapshot,
    )
    if min(
        replacement_prepare,
        replacement_adopt,
        destructive_clear,
        registry_invalidation,
        post_adoption_snapshot,
        perlin_registry_bind,
        game_of_life_registry_bind,
        game_of_life_randomize,
    ) < 0:
        errors.append("host visual replacement transaction is incomplete")
    elif not (
        replacement_prepare <
        replacement_adopt <
        registry_invalidation <=
        post_adoption_snapshot <
        perlin_registry_bind <
        game_of_life_registry_bind <
        game_of_life_randomize <
        destructive_clear
    ):
        errors.append(
            "host must adopt, invalidate registry consumers, resolve the "
            "installed snapshot, bind registry-backed controls, run the "
            "narrow GoL action, and keep destructive non-element clear after "
            "the atomic Runtime path"
        )
    if "runtime_.legacyCompositionElementForHost" in add_body:
        errors.append(
            "host add/replacement flow must invoke named action adapters "
            "rather than requesting a mutable live element"
        )

    app_without_comments = strip_cpp_comments(app)
    try:
        geodesic_query_body = function_body(
            app,
            "std::optional<int> ofApp::geodesicSubdivisionAtSlot",
        )
        geodesic_adjust_body = function_body(
            app_without_comments,
            "bool ofApp::adjustGeodesicSubdivisionAtSlot",
        )
        game_of_life_randomize_body = function_body(
            app_without_comments,
            "bool ofApp::randomizeGameOfLifeAtSlot",
        )
        first_type_body = function_body(
            app,
            "ofApp::firstConsoleElementOfType",
        )
        float_value_body = function_body(
            app,
            "std::optional<float> ofApp::consoleFloatValue",
        )
        bool_value_body = function_body(
            app,
            "std::optional<bool> ofApp::consoleBoolValue",
        )
        float_write_body = function_body(
            app,
            "bool ofApp::writeConsoleFloatLive",
        )
        bool_write_body = function_body(
            app,
            "bool ofApp::writeConsoleBoolLive",
        )
        perlin_midi_body = function_body(
            app,
            "void ofApp::registerPerlinMidi",
        )
        game_of_life_midi_body = function_body(
            app,
            "void ofApp::registerGameOfLifeMidi",
        )
        osc_routes_body = function_body(
            app,
            "void ofApp::setupOscRoutes",
        )
    except ValueError as exc:
        errors.append(str(exc))
    else:
        for token in (
            "runtime_.compositionLayerSnapshot",
            'slot->typeId != "geodesic"',
            'consoleFloatValue(*slot, "subdivisions")',
            "std::round(*subdivisions)",
        ):
            if token not in geodesic_query_body:
                errors.append(
                    "durable Geodesic subdivision parameter query is missing "
                    f"semantic guard: {token}"
                )
        for forbidden in (
            "compositionElementForHost",
            "dynamic_cast<",
            "geodesic->subdivisions()",
        ):
            if forbidden in geodesic_query_body:
                errors.append(
                    "Geodesic subdivision query still inspects a concrete "
                    f"element instead of durable parameter state: {forbidden}"
                )

        for token in (
            "runtime_.compositionLayerSnapshot",
            'slot->typeId != "geodesic"',
            "delta == 0",
        ):
            if token not in geodesic_adjust_body:
                errors.append(
                    "Geodesic runtime action adapter is missing semantic "
                    f"guard: {token}"
                )
        polarity_expression = (
            r"(?:delta\s*>\s*0|0\s*<\s*delta)\s*\?\s*"
            r'"subdivision\.increment"\s*:\s*'
            r'"subdivision\.decrement"'
        )
        geodesic_invocations = call_expressions(
            geodesic_adjust_body,
            r"\bruntime_\s*\.\s*invokeCompositionAction",
        )
        invokes_polarity_directly = any(
            re.search(polarity_expression, call, re.DOTALL)
            for call in geodesic_invocations
        )
        mapped_action = re.search(
            rf"(?:const\s+)?(?:std::string_view|auto)\s+"
            rf"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*"
            rf"{polarity_expression}",
            geodesic_adjust_body,
            re.DOTALL,
        )
        invokes_mapped_action = bool(
            mapped_action
            and any(
                re.search(
                    rf"\b{re.escape(mapped_action.group(1))}\b",
                    call,
                )
                for call in geodesic_invocations
            )
        )
        if not geodesic_invocations:
            errors.append(
                "Geodesic runtime action adapter must invoke a Runtime action"
            )
        elif not invokes_polarity_directly and not invokes_mapped_action:
            errors.append(
                "Geodesic runtime action adapter must map positive delta to "
                "subdivision.increment and non-positive delta to "
                "subdivision.decrement in the invoked action"
            )
        for token in (
            "runtime_.compositionLayerSnapshot",
            'slot->typeId != "gameOfLife"',
        ):
            if token not in game_of_life_randomize_body:
                errors.append(
                    "Game of Life runtime action adapter is missing semantic "
                    f"guard: {token}"
                )
        game_of_life_invocations = call_expressions(
            game_of_life_randomize_body,
            r"\bruntime_\s*\.\s*invokeCompositionAction",
        )
        if not any(
            re.search(r'"simulation\.randomize"', call)
            for call in game_of_life_invocations
        ):
            errors.append(
                "Game of Life runtime action adapter must invoke "
                "simulation.randomize"
            )

        if "runtime_.legacyCompositionElementForHost" in app:
            errors.append(
                "host action paths must not request mutable live elements"
            )
        for token in (
            "dynamic_cast<GeodesicLayer*>",
            "dynamic_cast<GameOfLifeLayer*>",
            "dynamic_cast<GridLayer*>",
            "dynamic_cast<PerlinNoiseLayer*>",
            "dynamic_cast<const PerlinNoiseLayer*>",
            "dynamic_cast<const GameOfLifeLayer*>",
            "dynamic_cast<const GeodesicLayer*>",
            "dynamic_cast<const VideoGrabberLayer*>",
            "dynamic_cast<const VideoClipLayer*>",
        ):
            if token in app:
                errors.append(
                    "ordinary parameter views still inspect a concrete "
                    f"element: {token}"
                )

        for body, context in (
            (first_type_body, "composition type lookup"),
            (float_value_body, "float parameter view"),
            (bool_value_body, "bool parameter view"),
            (float_write_body, "float live action"),
            (bool_write_body, "bool live action"),
        ):
            if "legacyCompositionElementForHost" in body:
                errors.append(
                    f"{context} must not use mutable element access"
                )
        for token in (
            "runtime_.compositionSnapshot",
            "slot.hasElement",
            "slot.typeId == typeId",
        ):
            if token not in first_type_body:
                errors.append(
                    "first-element lookup is missing snapshot ordering/type "
                    f"semantics: {token}"
                )
        for body, kind in (
            (float_value_body, "float"),
            (bool_value_body, "bool"),
        ):
            for token in ("param->value", "param->baseValue"):
                if token not in body:
                    errors.append(
                        f"{kind} parameter view must prefer live storage and "
                        f"retain a base fallback: {token}"
                    )
        if "*param->value = value" not in float_write_body:
            errors.append("float console action must write live registry storage")
        if "*param->value = value" not in bool_write_body:
            errors.append("bool console action must write live registry storage")

        for body, type_id in (
            (perlin_midi_body, "perlin"),
            (game_of_life_midi_body, "gameOfLife"),
        ):
            for token in (
                f'slot.typeId != "{type_id}"',
                "consoleFloatParam(slot, suffix)",
                "param->meta.range.min",
                "param->meta.range.max",
                "param->value",
            ):
                if token not in body:
                    errors.append(
                        f"{type_id} MIDI adapter is missing registry-backed "
                        f"binding semantics: {token}"
                    )
            for forbidden in (
                "legacyCompositionElementForHost",
                "dynamic_cast<",
                "ParamPtr()",
            ):
                if forbidden in body:
                    errors.append(
                        f"{type_id} MIDI adapter still uses a concrete "
                        f"element seam: {forbidden}"
                    )
        for token in (
            'bindFloat("octaves", true, 1.0f)',
            'bindFloat("palette", true, 1.0f)',
        ):
            if token not in perlin_midi_body:
                errors.append(
                    "Perlin MIDI adapter changed established snap/step "
                    f"semantics: {token}"
                )
        for token in (
            'bindFloat("fadeFrames", true, 1.0f)',
            'bindFloat("preset", true, 1.0f)',
            'bindBool("reseed", MidiRouter::BoolMode::Assign)',
        ):
            if token not in game_of_life_midi_body:
                errors.append(
                    "Game of Life MIDI adapter changed established binding "
                    f"semantics: {token}"
                )
        for token in (
            'firstConsoleElementOfType("geodesic")',
            'consoleFloatParam(*geodesic, "hover")',
            'consoleFloatParam(*geodesic, "spin")',
            'firstConsoleElementOfType("grid")',
            'consoleBoolParam(*grid, "visible")',
        ):
            if token not in osc_routes_body:
                errors.append(
                    "core OSC routes are missing snapshot/registry-backed "
                    f"binding: {token}"
                )

    persisted_roots = (
        ROOT / "docs/contracts",
        ROOT / "docs/examples",
        ROOT / "synaptome/bin/data",
        ROOT / "tools/testdata",
    )
    persisted_action_ids = (
        "subdivision.increment",
        "subdivision.decrement",
        "simulation.randomize",
    )
    persisted_telemetry_ids = (
        "media.sourceLabel",
        "media.captureInitialized",
    )
    for persisted_root in persisted_roots:
        for json_path in persisted_root.rglob("*.json"):
            text = json_path.read_text(encoding="utf-8", errors="replace")
            for action_id in persisted_action_ids:
                if action_id in text:
                    errors.append(
                        "live-only action leaked into persisted JSON: "
                        f"{json_path.relative_to(ROOT)} ({action_id})"
                    )
            for telemetry_id in persisted_telemetry_ids:
                if telemetry_id in text:
                    errors.append(
                        "live-only telemetry leaked into persisted JSON: "
                        f"{json_path.relative_to(ROOT)} ({telemetry_id})"
                    )
    deferred_mapping_surface = midi_router + osc_parameter_router
    deferred_schema_paths = (
        ROOT / "docs/schemas/layer_browser_inspection_payload.schema.json",
        ROOT / "docs/schemas/scene.schema.json",
        ROOT / "docs/schemas/midi_bank.schema.json",
        ROOT / "docs/schemas/osc_map.schema.json",
        ROOT / "docs/schemas/hotkeys.schema.json",
        ROOT / "docs/schemas/device_map.schema.json",
        ROOT / "docs/schemas/parameter_manifest.schema.json",
    )
    deferred_schema_surface = "\n".join(
        path.read_text(encoding="utf-8")
        for path in deferred_schema_paths
    )
    # SEAC-9 intentionally permits package-declared action targets to flow
    # through the mapping bank and Browser inspection payload. Telemetry is
    # still live-only and must not enter these persisted surfaces.
    for token in (
        "compositionElementTelemetry",
        "TelemetryEntry",
        "media.sourceLabel",
        "media.captureInitialized",
    ):
        if token in deferred_mapping_surface:
            errors.append(
                "MIDI/OSC mapping router must not persist live telemetry: "
                f"{token}"
            )
    for token in (
        '"telemetry"',
        '"telemetryId"',
        "media.sourceLabel",
        "media.captureInitialized",
    ):
        if token in deferred_schema_surface:
            errors.append(
                "persisted package/scene/mapping schemas must not declare "
                f"live telemetry surfaces: {token}"
            )

    direct_metadata_write = re.compile(
        r"(?:\bslot|consoleSlots\s*(?:\[[^\]]+\]|\.at\([^)]*\)))"
        r"(?:\.|->)(?:assetId|label|type|paramPrefix|kind|active|opacity|"
        r"coverage)(?:\.[A-Za-z_][A-Za-z0-9_]*)?\s*"
        r"(?:=(?!=)|\.(?:swap|clear|assign)\s*\()"
    )
    if direct_metadata_write.search(app):
        errors.append("host still writes Runtime-owned composition metadata directly")
    if "midi.unbindTargetsByPrefix(prefix)" not in add_body:
        errors.append("replacement must preserve address-based MIDI/OSC maps")
    if "if (!clearConsoleSlot(idx))" not in add_body:
        errors.append(
            "non-element assignment must stop when the prior slot cannot clear"
        )
    if "bool ofApp::clearAllConsoleSlots()" not in app:
        errors.append("bulk console clear must expose transactional failure")
    scene_layout_start = app.find("bool ofApp::loadConsoleLayoutFromScene")
    scene_layout_end = app.find(
        "void ofApp::writeConsoleLayoutToScene",
        scene_layout_start,
    )
    scene_layout_body = app[scene_layout_start:scene_layout_end]
    if "if (!clearAllConsoleSlots())" not in scene_layout_body:
        errors.append("scene publication must stop after bulk-clear failure")
    if "if (!clearConsoleSlot(layerIndex - 1))" not in app:
        errors.append("Control & Mapping unload must report clear failure")
    if "paramRegistry.findFloat(paramId)" not in app:
        errors.append("slot opacity registration must support same-address reuse")
    if "invalidateParameterRegistryStorage() noexcept" not in control_hub_header:
        errors.append(
            "parameter consumers need synchronous no-throw invalidation after "
            "registry storage replacement"
        )
    if "setOfflineElementCreator" not in control_hub_header:
        errors.append(
            "Control & Mapping offline inspection must receive a narrow "
            "scoped element creator"
        )
    if "bool contains(" not in layer_factory_header:
        errors.append("scoped element type registry is missing non-constructing lookup")
    if "runtime_.hasElementType(entry->type)" not in app:
        errors.append("scene validation must query Runtime without constructing an element")
    setup_start = app.find("void ofApp::setup()")
    setup_end = app.find("void ofApp::update()", setup_start)
    if setup_start < 0 or setup_end < 0:
        errors.append("could not inspect host setup registration boundary")
    elif ".registerType(" in app[setup_start:setup_end]:
        errors.append("host setup still mutates the element type registry")
    for surface_name, surface in (
        ("factory header", layer_factory_header),
        ("factory source", layer_factory_source),
        ("runtime", runtime_header + runtime_source),
        ("host", app_header + app),
        ("Control & Mapping", control_hub_header),
        ("RuntimeCore test", runtime_test),
        ("package bench", package_bench),
    ):
        if "LayerFactory::instance" in surface or "static LayerFactory& instance" in surface:
            errors.append(
                f"{surface_name} still depends on the process-global element factory"
            )

    for token in (
        "ConfigurationType>StaticLibrary",
        "Synaptome.ElementSdk.props",
        "openframeworksRelease.props",
        r"src\runtime\Runtime.cpp",
        r"src\runtime\ElementParameterTable.h",
        r"src\visuals\LayerFactory.cpp",
        r"\synaptome\element\ParameterBinding.h",
    ):
        if token.lower() not in core_project.lower():
            errors.append(f"runtime-core library is missing {token}")
    if r"runtime\SynaptomeRuntimeCore.vcxproj".lower() not in project.lower():
        errors.append("host must link the SynaptomeRuntimeCore project")
    for token in (
        r'ClCompile Include="src\host\HostCompositionRenderer.cpp"',
        r'ClInclude Include="src\host\HostCompositionRenderer.h"',
        r'ClInclude Include="src\host\HostCompositionEffects.h"',
    ):
        if token.lower() not in project.lower():
            errors.append(f"host project is missing composition renderer wiring: {token}")
    for token in (
        r'src\host\HostCompositionRenderer.cpp',
        r'src\host\HostCompositionRenderer.h',
        r'src\host\HostCompositionEffects.h',
    ):
        if token.lower() not in project_filters.lower():
            errors.append(
                f"host project filters are missing composition renderer wiring: {token}"
            )
    for token in (
        "HostCompositionRenderer",
        "HostCompositionEffects",
        r"src\host",
    ):
        if token.lower() in core_project.lower():
            errors.append(
                f"RuntimeCore project includes forbidden host render surface: {token}"
            )
    for token in (
        r'ClCompile Include="src\runtime\Runtime.cpp"',
        r'ClCompile Include="src\visuals\LayerFactory.cpp"',
    ):
        if token.lower() in project.lower():
            errors.append(f"host still compiles runtime-core source directly: {token}")
    for token in (
        r"runtime_core_native_main.cpp",
        r"runtime\SynaptomeRuntimeCore.vcxproj",
        "openframeworksRelease.props",
    ):
        if token.lower() not in test_project.lower():
            errors.append(f"runtime-core test target is missing {token}")
    for token in (
        r"src\runtime\Runtime.cpp",
        r"src\visuals\LayerFactory.cpp",
        r"$(SynaptomeTestRoot)\stubs",
    ):
        if token.lower() in test_project.lower():
            errors.append(f"runtime-core test bypasses the shipping link/header seam: {token}")
    renderer_test_lower = renderer_test_project.lower()
    required_renderer_test_items = (
        "configurationtype>application",
        r"$(synaptometestroot)\host_composition_renderer_native_main.cpp",
        r"$(synaptomeapproot)\src\host\hostcompositionrenderer.cpp",
        r"$(synaptomeapproot)\src\runtime\runtime.cpp",
        r"$(synaptomeapproot)\src\visuals\layerfactory.cpp",
        r"$(synaptometestroot)\stubs\offbo.h",
        r"$(synaptometestroot)\stubs\ofglstub.h",
        r"$(synaptometestroot)\stubs\ofrectangle.h",
        r"$(synaptomeapproot)\src\host\hostcompositioneffects.h",
        r"$(synaptomeapproot)\src\host\hostcompositionrenderer.h",
    )
    for token in required_renderer_test_items:
        if token not in renderer_test_lower:
            errors.append(
                "host composition renderer test target is missing shipping/stub "
                f"wiring: {token}"
            )
    include_directories_match = re.search(
        r"<AdditionalIncludeDirectories>(.*?)</AdditionalIncludeDirectories>",
        renderer_test_project,
        re.DOTALL,
    )
    if include_directories_match is None:
        errors.append(
            "host composition renderer test target is missing include directories"
        )
    else:
        include_directories = include_directories_match.group(1).lower()
        stub_root = r"$(synaptometestroot)\stubs"
        app_source_root = r"$(synaptomeapproot)\src"
        if (
            stub_root not in include_directories
            or app_source_root not in include_directories
            or include_directories.index(stub_root)
            > include_directories.index(app_source_root)
        ):
            errors.append(
                "host composition renderer test must resolve stubs before "
                "shipping application source roots"
            )
    for token in (
        "posteffectchain",
        "ofapp",
        r"openframeworkslib.vcxproj",
        r"synaptomeruntimecore.vcxproj",
    ):
        if token in renderer_test_lower:
            errors.append(
                "host composition renderer test must compile the narrow "
                f"shipping-source seam without {token}"
            )
    required_renderer_test_filter_items = (
        r"host_composition_renderer_native_main.cpp",
        r"hostcompositionrenderer.cpp",
        r"runtime.cpp",
        r"layerfactory.cpp",
        r"offbo.h",
        r"ofglstub.h",
        r"ofrectangle.h",
        r"hostcompositioneffects.h",
        r"hostcompositionrenderer.h",
    )
    renderer_test_filters_lower = renderer_test_filters.lower()
    for token in required_renderer_test_filter_items:
        if token not in renderer_test_filters_lower:
            errors.append(
                "host composition renderer test filters are missing "
                f"project item: {token}"
            )
    renderer_test_guid_match = re.search(
        r"<ProjectGuid>\s*(\{[0-9A-Fa-f-]+\})\s*</ProjectGuid>",
        renderer_test_project,
    )
    if renderer_test_guid_match is None:
        errors.append(
            "host composition renderer test project is missing its project GUID"
        )
    else:
        renderer_test_guid = renderer_test_guid_match.group(1).upper()
        if (
            '"HostCompositionRendererTest", '
            '"tests\\HostCompositionRendererTest\\'
            'HostCompositionRendererTest.vcxproj", '
            f'"{renderer_test_guid}"'
        ) not in solution:
            errors.append(
                "solution is missing the host composition renderer test project"
            )
        for configuration in ("Debug", "Release"):
            for suffix in ("ActiveCfg", "Build.0"):
                token = (
                    f"{renderer_test_guid}.{configuration}|x64.{suffix} = "
                    f"{configuration}|x64"
                )
                if token not in solution:
                    errors.append(
                        "solution is missing host composition renderer test "
                        f"configuration wiring: {token}"
                    )
    for token in (
        "FboTrace",
        "RenderStatus::Rendered",
        "RenderStatus::InvalidViewport",
        "RenderStatus::CompositeAllocationFailed",
        "isConsoleRouted(",
        "applySlot(",
        "applyGlobal(",
        "drawLatest(",
        "drawPreview(",
        "releaseGraphicsResources(",
        "ofstub::failAllocationOnAttempt(",
    ):
        if token not in renderer_test:
            errors.append(
                "host composition renderer test is missing lifecycle/effect "
                f"coverage: {token}"
            )
    for token in (
        "PostEffectChain",
        "ofApp",
        '#include "../synaptome/src/host/HostCompositionRenderer.cpp"',
        '#include "../synaptome/src/runtime/Runtime.cpp"',
        '#include "../synaptome/src/visuals/LayerFactory.cpp"',
    ):
        if token in renderer_test:
            errors.append(
                "host composition renderer test bypasses its narrow project "
                f"boundary: {token}"
            )
    if "SynaptomeRuntimeCore" not in solution:
        errors.append("runtime-core library must be a first-class solution target")
    if "RunEffectCoverageWindowScenario" not in runtime_test:
        errors.append("RuntimeCore test is missing effect coverage-window coverage")
    for token in (
        "RunCompositionSnapshotScenario",
        "RunCompositionSlotReplacementScenario",
        "prepareCompositionElementReplacement",
        "slot replacement bounds or empty-slot validation constructed an element",
        "effect slot replacement constructed an element",
        "overlay slot replacement constructed an element",
        "slot replacement preparation changed the live element",
        "slot adoption did not replace registry storage while retaining the retired element",
        "stale slot replacement changed the winning live element",
        "prepared replacement generation is stale",
        "Runtime expiry destroyed or invalidated its prepared replacement",
        "expired Runtime replacement did not release its candidate safely",
        "compositionSnapshot",
        "compositionLayerSnapshot",
        "caller.mutated",
        "mutating a composition snapshot changed Runtime state",
        "composition snapshot constructed an extra element",
        "CompositionMutationError::",
        "assignCompositionEntry",
        "setCompositionLayerActive",
        "setCompositionLayerLabel",
        "setCompositionLayerCoverage",
        "clearCompositionLayer",
        "tests.stable-opacity-mapping",
        "RunElementDescriptorRegistryScenario",
        "invalid or duplicate descriptors mutated the registry",
        "construction-free parameter contract lookup lost declared or legacy state",
        "mutating copied parameter contracts changed stable factory storage",
        "invalid parameter declaration was not rejected atomically",
        "RunDeclaredParameterBindingScenario",
        "declared parameter metadata/default/base/live authority drifted",
        "declared parameter bindings did not reach live element storage",
        "declared binding failure leaked state or lost diagnostics",
        "declared binding failure did not release its prefix",
        "explicit declared-empty binding did not prepare",
        "RunCompositionActionScenario",
        "InvalidActionMode::UnknownBinding",
        "InvalidActionMode::DuplicateBinding",
        "InvalidActionMode::EmptyHandler",
        "InvalidActionMode::MissingBinding",
        "invalid static action declarations were accepted",
        "effect descriptor reached the visual element creator",
        "prepared actions became discoverable before adoption",
        "mutating a copied action descriptor changed Runtime state",
        "action execution status translation or exception containment drifted",
        "same local action ID did not remain scoped to its composition slot",
        "prepared replacement published candidate action declarations",
        "action replacement did not atomically publish and retain tables",
        "retired action handlers did not precede matching element destruction",
        "clear retained action discovery, invocation, or handler lifetime",
        "shutdown retained a live action table or destroyed it out of order",
        "RunCompositionTelemetryScenario",
        "element with no telemetry did not return an empty success",
        "ordinary composition snapshot collected volatile telemetry",
        "adopted inactive element telemetry or typed value projection drifted",
        "telemetry copy isolation or on-demand freshness drifted",
        "same telemetry IDs leaked across composition slots",
        "invalid telemetry was accepted or left Runtime unusable",
        "prepared telemetry candidate became visible before adoption",
        "telemetry replacement did not publish the adopted instance",
        "clear retained live telemetry",
        "shutdown retained live telemetry",
        "RunLegacyParameterAdapterScenario",
        "legacy setup metadata overrode declarations or configured values",
        "legacy adapter failure leaked state or lost diagnostics",
    ):
        if token not in runtime_test:
            errors.append(f"RuntimeCore test is missing control-plane coverage: {token}")

    for token in (
        "verifyDeclaredPackageSurface(",
        "legacy discovery and declared-empty parameter states collapsed",
        "mutating copied Signal Bloom contracts changed factory state",
    ):
        if token not in package_bench:
            errors.append(
                "package bench is missing reusable static parameter "
                "declaration coverage: " + token
            )
    for token in (
        "factory.typeContract(typeId)",
        "LayerFactory::ParameterDeclarationState::Declared",
        "expectedGroups.size()",
        "expectedParameterGroups.size()",
        "simplePackageLabel(",
        "option-source selectors drifted",
        "static option metadata drifted",
        "deprecation metadata drifted",
    ):
        if token not in declared_surface_checks:
            errors.append(
                "reusable package surface checks are missing static parameter "
                "coverage: " + token
            )

    try:
        telemetry_test_body = function_body(
            runtime_test,
            "void RunCompositionTelemetryScenario",
        )
    except ValueError as exc:
        errors.append(str(exc))
    else:
        for token in (
            "runtime.compositionElementTelemetry(",
            "runtime.compositionSnapshot()",
            "request.enabled = false",
            "!inactiveTelemetrySnapshot->active",
        ):
            if token not in telemetry_test_body:
                errors.append(
                    "RuntimeCore telemetry scenario is missing on-demand/"
                    f"inactive semantics: {token}"
                )

    if errors:
        print("[runtime-core-boundary] FAIL")
        for error in errors:
            print(f"  - {error}")
        return 1
    print(
        "[runtime-core-boundary] PASS linked RuntimeCore lifecycle and "
        "immutable composition query/control/replacement plus static "
        "action/parameter declarations, explicit legacy/declared inspection, "
        "bind-only live action, and on-demand typed telemetry planes, with "
        "host-only composition rendering and its stub-backed renderer contract"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

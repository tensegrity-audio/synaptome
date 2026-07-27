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
    telemetry_header = (
        ROOT / "synaptome/sdk/include/synaptome/element/Telemetry.h"
    ).read_text(encoding="utf-8")
    action_table = (
        ROOT / "synaptome/src/runtime/ElementActionTable.h"
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
    post_effect_header = (
        ROOT / "synaptome/src/visuals/effects/PostEffectChain.h"
    ).read_text(encoding="utf-8")
    post_effect_source = (
        ROOT / "synaptome/src/visuals/effects/PostEffectChain.cpp"
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
    solution = (ROOT / "synaptome/Synaptome.sln").read_text(encoding="utf-8")
    core_project = (
        ROOT / "synaptome/runtime/SynaptomeRuntimeCore.vcxproj"
    ).read_text(encoding="utf-8")
    test_project = (
        ROOT / "synaptome/tests/RuntimeCoreTest/RuntimeCoreTest.vcxproj"
    ).read_text(encoding="utf-8")
    runtime_test = (
        ROOT / "tests/runtime_core_native_main.cpp"
    ).read_text(encoding="utf-8")
    package_bench = (
        ROOT / "tests/layer_package_bench_main.cpp"
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
        "compositionRenderTargetsForHost",
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

    descriptor_start = action_header.find("struct ActionDescriptor")
    descriptor_end = action_header.find(
        "enum class ActionExecutionStatus",
        descriptor_start,
    )
    if descriptor_start < 0 or descriptor_end < 0:
        errors.append("could not inspect the public live action descriptor")
        action_descriptor_surface = ""
    else:
        action_descriptor_surface = action_header[
            descriptor_start:descriptor_end
        ]
    for token in (
        "std::string id;",
        "std::string label;",
        "std::string groupId;",
        "std::string description;",
    ):
        if token not in action_descriptor_surface:
            errors.append(f"live action descriptor is missing {token}")
    if "std::string group;" in action_descriptor_surface:
        errors.append(
            "live action descriptor must use stable groupId instead of group"
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
                f"live action descriptor exposes forbidden ownership: {token}"
            )
    for token in (
        "using ActionHandler = std::function<ActionExecutionResult()>",
        "class ActionRegistrar",
        "virtual void add(",
    ):
        if token not in action_header:
            errors.append(f"public live action contract is missing {token}")
    if "registerActions(" not in layer_header:
        errors.append("compatibility Layer is missing optional action registration")
    for token in (
        "contractError() const",
        "isValidId(",
        "entry.descriptor.id",
        "entry.descriptor.label.empty()",
        "isValidGroupId(entry.descriptor.groupId)",
        "invalid action group ID",
        "entry.handler",
        "duplicate action ID",
        "descriptors() const",
        "find(",
    ):
        if token not in action_table:
            errors.append(f"runtime live action table is missing {token}")
    if "character != '_'" in action_table:
        errors.append(
            "live action IDs must use dotted lowerCamel segments; underscores "
            "must remain invalid"
        )
    group_validator_start = action_table.find(
        "static bool isValidGroupId("
    )
    group_validator_end = action_table.find(
        "std::vector<Entry> entries_",
        group_validator_start,
    )
    if group_validator_start < 0 or group_validator_end < 0:
        errors.append("could not inspect live action groupId validation")
        group_validator_surface = ""
    else:
        group_validator_surface = action_table[
            group_validator_start:group_validator_end
        ]
    for token in (
        "id.empty()",
        "id.front() < 'a'",
        "id.front() > 'z'",
        "character >= 'a'",
        "character <= 'z'",
        "character >= 'A'",
        "character <= 'Z'",
        "character >= '0'",
        "character <= '9'",
    ):
        if token not in group_validator_surface:
            errors.append(
                f"live action groupId lowerCamel validation is missing {token}"
            )
    for forbidden in ("character == '.'", "character == '_'", "character == '-'"):
        if forbidden in group_validator_surface:
            errors.append(
                "live action groupId must remain a single alphanumeric "
                f"lowerCamel segment: {forbidden}"
            )

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
        "dynamic_cast<",
    ):
        if token in runtime_header or token in runtime_source:
            errors.append(
                f"generic Runtime action dispatch depends on a concrete element: {token}"
            )
    for header, label in (
        (geodesic_header, "Geodesic"),
        (game_of_life_header, "Game of Life"),
    ):
        if not re.search(r"\bregisterActions\s*\(", strip_cpp_comments(header)):
            errors.append(
                f"{label} live action implementation is missing registration"
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
            r"\bregistrar\s*\.\s*add",
        )
        game_of_life_action_calls = call_expressions(
            game_of_life_actions_body,
            r"\bregistrar\s*\.\s*add",
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
    for calls, action_id, group_id, handler_name, forbidden_handlers, label in (
        concrete_action_contracts
    ):
        matching_calls = [
            call
            for call in calls
            if first_string_literal(call) == action_id
        ]
        if len(matching_calls) != 1:
            errors.append(
                f"{label} must register action {action_id} exactly once"
            )
            continue
        action_call = matching_calls[0]
        descriptor_literals = re.findall(
            r'"((?:\\.|[^"\\])*)"',
            action_call,
        )
        if len(descriptor_literals) < 4:
            errors.append(
                f"{label} action {action_id} is missing complete descriptor metadata"
            )
        else:
            if not descriptor_literals[1]:
                errors.append(
                    f"{label} action {action_id} must retain a nonempty label"
                )
            if descriptor_literals[2] != group_id:
                errors.append(
                    f"{label} action {action_id} groupId must remain {group_id}"
                )
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
        "CompositionRenderTargets compositionRenderTargetsForHost(",
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
        "runtime_.compositionRenderTargetsForHost",
        "runtime_.resizeCompositionElements",
        "runtime_.updateCompositionElements",
        "runtime_.drawCompositionElement",
        "runtime_.resolveEffectCoverage",
        "runtime_.shutdownComposition",
    ):
        if token not in app:
            errors.append(f"ofApp must delegate generic composition behavior: {token}")
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
    if re.search(
        r"(?:\.|->)(?:layerFbo|upstreamFbo|effectFbo)\b",
        app,
    ):
        errors.append(
            "ofApp bypasses the named CompositionRenderTargets host seam"
        )
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
        ROOT / "docs/schemas/layer_package.schema.json",
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
    for token in (
        "invokeCompositionAction",
        "ActionDescriptor",
        "ActionHandler",
        ".actions.",
        "compositionElementTelemetry",
        "TelemetryEntry",
        "media.sourceLabel",
        "media.captureInitialized",
    ):
        if token in deferred_mapping_surface:
            errors.append(
                "MIDI/OSC mapping router must not persist or invoke live "
                f"actions/telemetry in this checkpoint: {token}"
            )
    for token in (
        '"actions"',
        '"action"',
        '"actionId"',
        '"targetKind"',
        ".actions.",
        '"telemetry"',
        '"telemetryId"',
        "media.sourceLabel",
        "media.captureInitialized",
    ):
        if token in deferred_schema_surface:
            errors.append(
                "persisted package/scene/mapping schemas must not declare "
                f"live action/telemetry surfaces in this checkpoint: {token}"
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
        r"src\visuals\LayerFactory.cpp",
    ):
        if token.lower() not in core_project.lower():
            errors.append(f"runtime-core library is missing {token}")
    if r"runtime\SynaptomeRuntimeCore.vcxproj".lower() not in project.lower():
        errors.append("host must link the SynaptomeRuntimeCore project")
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
        "compositionRenderTargetsForHost",
        "tests.stable-opacity-mapping",
        "RunCompositionActionScenario",
        "InvalidActionMode::InvalidUnderscore",
        "InvalidActionMode::EmptyLabel",
        "InvalidActionMode::InvalidGroupId",
        'groupId == "version"',
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
    ):
        if token not in runtime_test:
            errors.append(f"RuntimeCore test is missing control-plane coverage: {token}")

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
        "immutable composition query/control/replacement plus live action and "
        "on-demand typed telemetry planes"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

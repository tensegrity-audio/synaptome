#!/usr/bin/env python3
"""Validate the first physical Element SDK and Signal Bloom build boundary."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "synaptome"
SDK = APP / "sdk" / "include" / "synaptome" / "element" / "compat"
EXAMPLE = ROOT / "docs" / "examples" / "artist_sdk"
PACKAGE_SIGNAL = (
    ROOT
    / "docs"
    / "examples"
    / "layer_packages"
    / "signal_bloom"
    / "source"
)


def read(path: Path, errors: list[str]) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError as exc:
        errors.append(f"cannot read {path.relative_to(ROOT)}: {exc}")
        return ""


def direct_includes(text: str) -> list[str]:
    return re.findall(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', text, re.MULTILINE)


def main() -> int:
    errors: list[str] = []
    action_header = read(
        APP / "sdk" / "include" / "synaptome" / "element" / "Action.h",
        errors,
    )
    descriptor_header = read(
        APP
        / "sdk"
        / "include"
        / "synaptome"
        / "element"
        / "ElementDescriptor.h",
        errors,
    )
    parameter_header = read(
        APP / "sdk" / "include" / "synaptome" / "element" / "Parameter.h",
        errors,
    )
    parameter_binding_header = read(
        APP
        / "sdk"
        / "include"
        / "synaptome"
        / "element"
        / "ParameterBinding.h",
        errors,
    )
    action_contract = descriptor_header + "\n" + action_header
    telemetry_header = read(
        APP / "sdk" / "include" / "synaptome" / "element" / "Telemetry.h",
        errors,
    )
    layer_forwarder = read(SDK / "Layer.h", errors)
    builder_forwarder = read(SDK / "LayerParameterBuilder.h", errors)
    example_header = read(EXAMPLE / "SignalBloomLayer.h", errors)
    example_source = read(EXAMPLE / "SignalBloomLayer.cpp", errors)
    runtime_header = read(PACKAGE_SIGNAL / "SignalBloomLayer.h", errors)
    runtime_source = read(PACKAGE_SIGNAL / "SignalBloomLayer.cpp", errors)
    public_sdk_headers = sorted(
        path
        for path in (APP / "sdk" / "include").rglob("*")
        if path.is_file() and path.suffix.lower() in {".h", ".hpp"}
    )
    public_sdk_surfaces = public_sdk_headers + [
        APP / "src" / "visuals" / "Layer.h",
        APP / "src" / "visuals" / "LayerParameterBuilder.h",
    ]
    contract_project = read(
        APP / "tests" / "ElementSdkCompileContract" / "ElementSdkCompileContract.vcxproj",
        errors,
    )
    element_project = read(
        APP / "build" / "GeneratedElementPackages.targets",
        errors,
    )
    app_project = read(APP / "Synaptome.vcxproj", errors)
    app_source = read(APP / "src" / "ofApp.cpp", errors)
    app_header = read(APP / "src" / "ofApp.h", errors)
    runtime_project = read(
        APP / "runtime" / "SynaptomeRuntimeCore.vcxproj",
        errors,
    )
    host_effects_header = read(
        APP / "src" / "host" / "HostCompositionEffects.h",
        errors,
    )
    host_renderer_header = read(
        APP / "src" / "host" / "HostCompositionRenderer.h",
        errors,
    )
    host_renderer_source = read(
        APP / "src" / "host" / "HostCompositionRenderer.cpp",
        errors,
    )
    builtin_host_bindings_header = read(
        APP / "src" / "runtime" / "BuiltinElementHostBindings.h",
        errors,
    )
    builtin_host_bindings_source = read(
        APP / "src" / "runtime" / "BuiltinElementHostBindings.cpp",
        errors,
    )
    builtin_source = read(APP / "src" / "runtime" / "BuiltinElements.cpp", errors)
    signal_registration_source = read(
        PACKAGE_SIGNAL / "register_signal_bloom.cpp", errors
    )
    generated_registration_header = read(
        APP
        / "src"
        / "runtime"
        / "GeneratedElementPackageRegistrations.h",
        errors,
    )
    generated_registration_source = read(
        APP
        / "src"
        / "runtime"
        / "GeneratedElementPackageRegistrations.cpp",
        errors,
    )
    bench_project = read(
        APP / "tests" / "LayerPackageBench" / "LayerPackageBench.vcxproj",
        errors,
    )
    browser_flow_project = read(
        APP / "tests" / "BrowserFlowTest" / "BrowserFlowTest.vcxproj",
        errors,
    )
    bench_source = read(ROOT / "tests" / "layer_package_bench_main.cpp", errors)

    expected_forwarders = {
        "Layer.h": "../../../../../src/visuals/Layer.h",
        "LayerParameterBuilder.h": "../../../../../src/visuals/LayerParameterBuilder.h",
    }
    for name, expected in expected_forwarders.items():
        text = layer_forwarder if name == "Layer.h" else builder_forwarder
        includes = direct_includes(text)
        if includes != [expected]:
            errors.append(f"{name} must be the one documented compatibility forwarder")

    for token in (
        "struct ActionDescriptor",
        "std::string id",
        "std::string label",
        "std::string groupId;",
        "std::string description",
        "enum class ActionExecutionStatus",
        "Succeeded",
        "Rejected",
        "Failed",
        "struct ActionExecutionResult",
        "using ActionHandler = std::function<ActionExecutionResult()>",
        "class ActionRegistrar",
        "virtual void bind(",
    ):
        if token not in action_contract:
            errors.append(f"public action contract is missing {token}")
    if "std::string group;" in action_contract:
        errors.append(
            "public action descriptor must expose stable groupId, not ambiguous group"
        )
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
    for token in (
        "*",
        "&",
        "std::function",
        "ActionHandler",
        "Creator",
        "Layer",
        "Runtime",
        "ParameterRegistry",
        "ofJson",
        "ofFbo",
    ):
        if token in descriptor_header:
            errors.append(
                "public element descriptor exposes forbidden ownership: "
                + token
            )
    for token in (
        "enum class ParameterKind",
        "Float",
        "Bool",
        "String",
        "using ParameterValue = std::variant<float, bool, std::string>",
        "struct ParameterRange",
        "std::optional<float> step;",
        "struct ParameterOption",
        "ParameterValue value;",
        "struct ParameterOptionSource",
        "std::string valueField;",
        "std::string labelField;",
        "struct ParameterGroupDeclaration",
        "struct ParameterDeprecation",
        "std::string replacementId;",
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
            errors.append(f"public parameter declaration contract is missing {token}")
    for token in (
        "*",
        "&",
        "std::function",
        "ParameterBinder",
        "ParameterRegistry",
        "ActionHandler",
        "Creator",
        "Layer",
        "Runtime",
        "ofJson",
        "ofFbo",
        "unique_ptr",
        "shared_ptr",
    ):
        if token in parameter_header:
            errors.append(
                "public parameter declaration contract exposes forbidden "
                "binding/ownership: " + token
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
            errors.append(f"public parameter binding contract is missing {token}")
    for token in (
        "ParameterRegistry",
        "LayerFactory",
        "Runtime",
        "ofJson",
        "ofFbo",
        "std::function",
        "unique_ptr",
        "shared_ptr",
    ):
        if token in parameter_binding_header:
            errors.append(
                "public parameter binding contract imports forbidden "
                "host/runtime ownership: " + token
            )
    for token in (
        "using TelemetryValue =",
        "std::variant<bool, std::int64_t, double, std::string>",
        "struct TelemetryEntry",
        "std::string id;",
        "std::string label;",
        "std::string groupId;",
        "std::string description;",
        "TelemetryValue value;",
        "class TelemetrySink",
        "virtual void add(TelemetryEntry entry) = 0;",
    ):
        if token not in telemetry_header:
            errors.append(f"public telemetry contract is missing {token}")
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
                "public telemetry entry exposes forbidden ownership: "
                f"{token}"
            )
    for token in (
        "Composition",
        "Runtime",
        "LayerFactory",
        "ParameterRegistry",
        "ofApp",
        "ofMain",
        "MidiRouter",
        "OscParameterRouter",
    ):
        if (
            token in action_header
            or token in descriptor_header
            or token in telemetry_header
        ):
            errors.append(
                "public descriptor/action/telemetry contract imports forbidden "
                f"host/runtime ownership: {token}"
            )
    compatibility_layer = read(APP / "src" / "visuals" / "Layer.h", errors)
    if "registerActions(" not in compatibility_layer:
        errors.append("compatibility Layer must expose optional live action binding")
    if not re.search(
        r"virtual\s+void\s+collectTelemetry\s*\(\s*"
        r"synaptome::element::TelemetrySink&\s+sink\s*\)\s*const",
        compatibility_layer,
    ):
        errors.append(
            "compatibility Layer must expose optional const telemetry collection"
        )
    if "<synaptome/element/Telemetry.h>" not in compatibility_layer:
        errors.append(
            "compatibility Layer must consume the public telemetry contract"
        )

    for header in public_sdk_surfaces:
        text = read(header, errors)
        for token in (
            "CompositionKind",
            "CompositionAssignment",
            "CompositionLayerSnapshot",
            "CompositionSnapshot",
            "CompositionMutationError",
            "CompositionMutationResult",
            "CompositionRenderTargets",
            "CompositionCoverageWindow",
            "PostEffectChain",
            "HostCompositionRenderer",
            "HostCompositionEffects",
        ):
            if token in text:
                errors.append(
                    "public Element SDK leaks Runtime composition/effect "
                    f"surface {token}: {header.relative_to(ROOT)}"
                )

    for name, text in {
        "public SignalBloomLayer.h": example_header,
        "public SignalBloomLayer.cpp": example_source,
    }.items():
        for include in direct_includes(text):
            normalized = include.replace("\\", "/")
            if ".." in normalized.split("/"):
                errors.append(f"{name} escapes its public include roots: {include}")
            if any(
                token in normalized.lower()
                for token in (
                    "ofapp",
                    "layerfactory",
                    "layerlibrary",
                    "hostcomposition",
                    "/host/",
                    "/ui/",
                    "/io/",
                )
            ):
                errors.append(f"{name} imports a host/runtime dependency: {include}")

    if "<synaptome/element/compat/Layer.h>" not in example_header:
        errors.append("Signal Bloom header must use the public compatibility Layer include")
    if "<synaptome/element/ParameterBinding.h>" not in example_header:
        errors.append("Signal Bloom header must use the public parameter binding contract")
    if "<synaptome/element/compat/LayerParameterBuilder.h>" in example_source:
        errors.append("declared Signal Bloom must not retain legacy metadata registration")
    if "__has_include" in example_header:
        errors.append("Signal Bloom header must not use include-order-dependent fallback logic")
    if example_header != runtime_header:
        errors.append(
            "public and package Signal Bloom headers must remain byte-identical"
        )
    public_methods = set(
        re.findall(r"void\s+SignalBloomLayer::([A-Za-z0-9_]+)\s*\(", example_source)
    )
    package_methods = set(
        re.findall(r"void\s+SignalBloomLayer::([A-Za-z0-9_]+)\s*\(", runtime_source)
    )
    if public_methods != package_methods:
        errors.append(
            "public and package Signal Bloom implementations expose "
            "different method sets"
        )

    contract_lower = contract_project.lower()
    required_contract_tokens = (
        "synaptome.elementsdk.props",
        r"docs\examples\artist_sdk\signalbloomlayer.cpp",
        r"$(synaptometestroot)\element_sdk_compile_contract.cpp",
        r"$(synaptometestroot)\stubs",
    )
    for token in required_contract_tokens:
        if token not in contract_lower:
            errors.append(f"compile-contract project missing {token}")
    compile_contract_source = read(
        ROOT / "tests" / "element_sdk_compile_contract.cpp",
        errors,
    ).lower()
    if "<synaptome/element/action.h>" not in compile_contract_source:
        errors.append("compile-contract must include the public Action header directly")
    if (
        "<synaptome/element/elementdescriptor.h>"
        not in compile_contract_source
    ):
        errors.append(
            "compile-contract must include the public ElementDescriptor header directly"
        )
    if "<synaptome/element/parameter.h>" not in compile_contract_source:
        errors.append(
            "compile-contract must include the public Parameter header directly"
        )
    if "<synaptome/element/parameterbinding.h>" not in compile_contract_source:
        errors.append(
            "compile-contract must include the public ParameterBinding header directly"
        )
    if r"\synaptome\element\parameter.h" not in contract_lower:
        errors.append(
            "compile-contract project must list the public Parameter header"
        )
    if r"\synaptome\element\parameterbinding.h" not in contract_lower:
        errors.append(
            "compile-contract project must list the public ParameterBinding header"
        )
    forbidden_contract_roots = (
        r"$(synaptomeapproot)\src;",
        r"$(synaptomeapproot)\src\core",
   
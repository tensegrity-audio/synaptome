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
        r"$(synaptomeapproot)\src\visuals",
        r"$(synaptomeapproot)\src\ui",
        r"$(synaptomeapproot)\src\io",
    )
    for token in forbidden_contract_roots:
        if token in contract_lower:
            errors.append(f"compile-contract project exposes private include root {token}")

    element_lower = element_project.lower()
    for token in (
        "synaptomeenablegeneratedelementpackages",
        r"docs\examples\layer_packages\signal_bloom\source\signalbloomlayer.cpp",
        r"docs\examples\layer_packages\signal_bloom\source\register_signal_bloom.cpp",
        "generatedelementpackageregistrations.cpp",
    ):
        if token not in element_lower:
            errors.append(f"generated package build target missing {token}")

    app_lower = app_project.lower()
    for token in (
        r'clcompile include="src\visuals\signalbloomlayer.cpp"',
        r"elements\signal_bloom\element_signalbloom.vcxproj",
        r'clcompile include="src\runtime\signalbloomregistration.cpp"',
    ):
        if token in app_lower:
            errors.append(
                "host project retains obsolete Signal Bloom wiring: "
                + token
            )
    if "synaptomeenablegeneratedelementpackages>true" not in app_lower:
        errors.append(
            "host must opt into the generated package build target"
        )
    if r'clcompile include="src\runtime\builtinelements.cpp"' not in app_lower:
        errors.append("host project must compile the controlled registration unit")
    if (
        r'clcompile include="src\runtime\builtinelementhostbindings.cpp"'
        not in app_lower
    ):
        errors.append("host project must compile the built-in host binding unit")
    if (
        r'clinclude include="src\runtime\builtinelementhostbindings.h"'
        not in app_lower
    ):
        errors.append("host project must include the built-in host binding header")
    if "SignalBloomLayer" in app_source:
        errors.append("ofApp.cpp must not know the Signal Bloom concrete class")
    if ".registerType(" in app_source:
        errors.append("ofApp.cpp must delegate all element type bindings")
    if "registerBuiltinElements(elementTypes_)" not in app_source:
        errors.append("host must call the controlled built-in registration entrypoint")
    if (
        "registerBuiltinElementHostParameters(paramRegistry)"
        not in app_source
    ):
        errors.append("host must register built-in host parameters through the binding unit")
    if "updateBuiltinElementHostParameters()" not in app_source:
        errors.append("host must synchronize built-in host parameters through the binding unit")
    for token in ("TextLayerState", "TextLayer.h"):
        if token in app_source or token in app_header:
            errors.append(
                "ofApp must not own the text element compatibility binding: "
                + token
            )
    for token in (
        "TextLayer",
        "TextLayerState",
        "::instance",
        "syncFontSelection",
        "std::function",
        "Composition",
        "ofFbo",
        "Layer*",
        "void*",
    ):
        if token in builtin_host_bindings_header:
            errors.append(
                "built-in host binding header exposes concrete or executable "
                "ownership: "
                + token
            )
    for token in (
        "void registerBuiltinElementHostParameters(ParameterRegistry& registry);",
        "void updateBuiltinElementHostParameters();",
    ):
        if token not in builtin_host_bindings_header:
            errors.append(
                "built-in host binding header is missing its pointer-free "
                "entrypoint: "
                + token
            )
    for token in (
        '#include "../visuals/TextLayerState.h"',
        "TextLayerState::instance()",
        "refreshAvailableFonts()",
        "syncFontSelection()",
    ):
        if token not in builtin_host_bindings_source:
            errors.append(
                "built-in host binding source must privately own text state "
                "synchronization: "
                + token
            )
    expected_text_parameter_ids = {
        "overlay.text.content",
        "overlay.text.topLeft",
        "overlay.text.topRight",
        "overlay.text.bottomLeft",
        "overlay.text.bottomRight",
        "overlay.text.font",
        "overlay.text.fontIndex",
        "overlay.text.size",
        "overlay.text.corner.size",
        "overlay.text.color.r",
        "overlay.text.color.g",
        "overlay.text.color.b",
    }
    bound_text_parameter_id_list = re.findall(
        r'add(?:Float|String)\s*\(\s*"(overlay\.text\.[^"]+)"',
        builtin_host_bindings_source,
    )
    if (
        len(bound_text_parameter_id_list) != len(expected_text_parameter_ids)
        or set(bound_text_parameter_id_list) != expected_text_parameter_ids
    ):
        errors.append(
            "built-in host binding source must own exactly the 12 text "
            "parameter IDs"
        )
    if "registerGeneratedElementPackages(elementTypes)" not in builtin_source:
        errors.append(
            "built-in registration unit must compose generated packages"
        )
    for token in (
        "GeneratedElementPackageRegistration",
        "generatedElementPackageRegistrations",
        "registerGeneratedElementPackages",
        '"examples.signal_bloom"',
        '"example.signalBloom"',
        '"39e6e7934d09689bd1a952e2d040f3a36ebdf595a47446778d1745a2fcdb00de"',
    ):
        if token not in (
            generated_registration_header
            + "\n"
            + generated_registration_source
        ):
            errors.append(
                "generated registration surface is missing " + token
            )
    for token in (
        "ElementTypeContract generatedContract0()",
        '"example.signalBloom"',
        "contract.parameters.groups =",
        "contract.parameters.parameters.push_back",
        "ParameterOptionSource{",
        '"transport.bpmMultipliers"',
        "ParameterDeprecation{",
        '"opacity"',
        "elementTypes.registerType(",
        "generatedContract0()",
        "synaptomeCreateElementPackage_examples_signal_bloom",
    ):
        if token not in generated_registration_source:
            errors.append(
                "generated Signal Bloom registration is missing " + token
            )
    for token in (
        "synaptomeCreateElementPackage_examples_signal_bloom",
        "std::make_unique<SignalBloomLayer>",
    ):
        if token not in signal_registration_source:
            errors.append("Signal Bloom creator is missing " + token)
    bench_project_lower = bench_project.lower()
    if r"\src\runtime\builtinelements.cpp" in bench_project_lower:
        errors.append("package bench must not compile the full host built-in registrar")
    if "synaptomeenablegeneratedelementpackages>true" not in bench_project_lower:
        errors.append(
            "package bench must opt into generated package registration"
        )
    if "registerGeneratedElementPackages(factory)" not in bench_source:
        errors.append(
            "package bench must call generated package registration"
        )

    registered_types = set(
        re.findall(
            r'registerType\(\s*(?:ElementDescriptor\s*)?\{\s*"([^"]+)"',
            builtin_source + "\n" + generated_registration_source,
        )
    )
    registered_types.update(
        re.findall(
            r'register(?:Explicit)?Builtin\(\s*ElementDescriptor\s*\{\s*"([^"]+)"',
            builtin_source,
        )
    )
    if (
        "elementTypes.registerType(" in generated_registration_source
        and "generatedContract0()" in generated_registration_source
    ):
        registered_types.add("example.signalBloom")
    canonical_types: set[str] = set()
    for catalog_path in (APP / "bin" / "data" / "layers").rglob("*.json"):
        try:
            catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        layer_type = catalog.get("type") if isinstance(catalog, dict) else None
        if (
            isinstance(layer_type, str)
            and layer_type
            and not layer_type.startswith("fx.")
            and layer_type != "ui.hud.widget"
        ):
            canonical_types.add(layer_type)
    allowed_types = set(canonical_types)
    for package_path in (ROOT / "docs" / "examples" / "layer_packages").rglob(
        "layer.package.json"
    ):
        try:
            package = json.loads(package_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        asset = package.get("asset") if isinstance(package, dict) else None
        layer_type = asset.get("type") if isinstance(asset, dict) else None
        if isinstance(layer_type, str) and layer_type:
            allowed_types.add(layer_type)
    unexpected_registrations = sorted(registered_types - allowed_types)
    if unexpected_registrations:
        errors.append(
            "registered types must be canonical catalog or opt-in package types: "
            + ", ".join(unexpected_registrations)
        )
    missing_canonical_registrations = sorted(canonical_types - registered_types)
    if missing_canonical_registrations:
        errors.append(
            "canonical runtime types must remain registered: "
            + ", ".join(missing_canonical_registrations)
        )

    concrete_classes = set(
        re.findall(
            r"std::make_unique<([^>]+)>",
            builtin_source + "\n" + signal_registration_source,
        )
    )
    leaked_classes = sorted(
        class_name
        for class_name in concrete_classes
        if re.search(
            rf"\b{re.escape(class_name)}\b",
            app_source + "\n" + app_header,
        )
    )
    if leaked_classes:
        errors.append(
            "ofApp must not include or reference registration-only concrete elements: "
            + ", ".join(leaked_classes)
        )
    runtime_project_lower = runtime_project.lower()
    app_project_lower = app_project.lower()
    for token in (
        r'clcompile include="src\host\hostcompositionrenderer.cpp"',
        r'clinclude include="src\host\hostcompositionrenderer.h"',
        r'clinclude include="src\host\hostcompositioneffects.h"',
    ):
        if token not in app_project_lower:
            errors.append(
                "host application project is missing composition-renderer "
                "wiring " + token
            )
    for token in (
        "class HostCompositionEffects",
        "isConsoleRouted",
        "defaultCoverageForType",
        "applySlot",
        "applyGlobal",
    ):
        if token not in host_effects_header:
            errors.append(f"host composition effect interface is missing {token}")
    for token in (
        "enum class RenderStatus",
        "class HostCompositionRenderer",
        "HostCompositionRenderer(",
        "RenderStatus render(",
        "drawLatest(",
        "drawPreview(",
        "hasFrame(",
        "releaseGraphicsResources(",
    ):
        if token not in host_renderer_header + host_renderer_source:
            errors.append(f"host composition renderer is missing {token}")
    for target_name, target_text in (
        ("RuntimeCore", runtime_project),
        (
            "Element SDK compile contract",
            contract_project + "\n" + example_header + "\n" + example_source,
        ),
        (
        "generated Signal Bloom package target",
            element_project + "\n" + runtime_header + "\n" + runtime_source,
        ),
        ("layer package bench", bench_project + "\n" + bench_source),
        ("browser flow", browser_flow_project),
    ):
        lowered_target = target_text.lower()
        for token in (
            "hostcompositionrenderer",
            "hostcompositioneffects",
            r"\src\host",
        ):
            if token in lowered_target:
                errors.append(
                    f"{target_name} must exclude host-only composition "
                    f"rendering: {token}"
                )
    for registration_unit in (
        "builtinelementhostbindings.cpp",
        "builtinelements.cpp",
        "generatedelementpackageregistrations.cpp",
    ):
        if registration_unit in runtime_project_lower:
            errors.append(
                "RuntimeCore must exclude host registration unit "
                + registration_unit
            )
    binding_unit = "builtinelementhostbindings"
    if (
        binding_unit in runtime_project_lower
        or binding_unit in element_lower
        or binding_unit in bench_project_lower
    ):
        errors.append(
            "RuntimeCore and element/package targets must exclude the "
            "host-only built-in binding unit"
        )
    browser_flow_project_lower = browser_flow_project.lower()
    for token in (
        r"\src\runtime\builtinelementhostbindings.cpp",
        r"\src\visuals\textlayerstate.cpp",
    ):
        if token not in browser_flow_project_lower:
            errors.append(
                "BrowserFlow must compile the zero-text host binding seam: " + token
            )
    if r"\src\visuals\textlayer.cpp" in browser_flow_project_lower:
        errors.append(
            "BrowserFlow zero-text binding contract must not compile TextLayer.cpp"
        )
    if '#include "../docs/examples/artist_sdk/SignalBloomLayer.cpp"' in bench_source:
        errors.append("bench must link the compile-contract library instead of including .cpp")
    if '#include "../synaptome/src/visuals/LayerFactory.cpp"' in bench_source:
        errors.append("bench must compile LayerFactory as a project item instead of including .cpp")

    if errors:
        print("[element-sdk-boundary] FAIL")
        for error in errors:
            print(f"  - {error}")
        return 1
    print(
        "[element-sdk-boundary] PASS public includes, generated package sources, "
        "pointer-free static element/action/parameter declarations, explicit "
        "legacy/declared registration, bind-only live actions, typed telemetry, "
        "controlled registration, compile-contract roots, and no "
        "Runtime/host-renderer composition leak"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

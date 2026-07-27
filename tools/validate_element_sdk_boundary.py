#!/usr/bin/env python3
"""Validate the first physical Element SDK and Signal Bloom build boundary."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "synaptome"
SDK = APP / "sdk" / "include" / "synaptome" / "element" / "compat"
EXAMPLE = ROOT / "docs" / "examples" / "artist_sdk"
RUNTIME_SIGNAL = APP / "src" / "visuals"


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
    layer_forwarder = read(SDK / "Layer.h", errors)
    builder_forwarder = read(SDK / "LayerParameterBuilder.h", errors)
    example_header = read(EXAMPLE / "SignalBloomLayer.h", errors)
    example_source = read(EXAMPLE / "SignalBloomLayer.cpp", errors)
    runtime_header = read(RUNTIME_SIGNAL / "SignalBloomLayer.h", errors)
    runtime_source = read(RUNTIME_SIGNAL / "SignalBloomLayer.cpp", errors)
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
        APP / "elements" / "signal_bloom" / "Element_SignalBloom.vcxproj",
        errors,
    )
    app_project = read(APP / "Synaptome.vcxproj", errors)
    app_source = read(APP / "src" / "ofApp.cpp", errors)
    builtin_source = read(APP / "src" / "runtime" / "BuiltinElements.cpp", errors)
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

    for header in public_sdk_surfaces:
        text = read(header, errors)
        for token in (
            "CompositionKind",
            "CompositionAssignment",
            "CompositionMutationError",
            "CompositionMutationResult",
            "CompositionRenderTargets",
            "CompositionCoverageWindow",
            "PostEffectChain",
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
                for token in ("ofapp", "layerfactory", "layerlibrary", "/ui/", "/io/")
            ):
                errors.append(f"{name} imports a host/runtime dependency: {include}")

    if "<synaptome/element/compat/Layer.h>" not in example_header:
        errors.append("Signal Bloom header must use the public compatibility Layer include")
    if "<synaptome/element/compat/LayerParameterBuilder.h>" not in example_source:
        errors.append("Signal Bloom source must use the public compatibility builder include")
    if "__has_include" in example_header:
        errors.append("Signal Bloom header must not use include-order-dependent fallback logic")
    if example_header != runtime_header or example_source != runtime_source:
        errors.append("public and shipping Signal Bloom sources must remain byte-identical")

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
        "configurationtype>staticlibrary",
        "synaptome.elementsdk.props",
        "openframeworksrelease.props",
        r"src\visuals\signalbloomlayer.cpp",
    ):
        if token not in element_lower:
            errors.append(f"shipping Signal Bloom project missing {token}")

    app_lower = app_project.lower()
    if r'clcompile include="src\visuals\signalbloomlayer.cpp"' in app_lower:
        errors.append("host must not compile Signal Bloom directly")
    if r"elements\signal_bloom\element_signalbloom.vcxproj" not in app_lower:
        errors.append("host must link the shipping Signal Bloom element project")
    if r'clcompile include="src\runtime\builtinelements.cpp"' not in app_lower:
        errors.append("host project must compile the controlled registration unit")
    if "SignalBloomLayer" in app_source:
        errors.append("ofApp.cpp must not know the Signal Bloom concrete class")
    if "registerBuiltinElements(elementTypes)" not in app_source:
        errors.append("host must call the controlled built-in registration entrypoint")
    if 'registerType("example.signalBloom"' not in builtin_source:
        errors.append("built-in registration unit must own the Signal Bloom type binding")
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
        "[element-sdk-boundary] PASS public includes, shipping static library, "
        "controlled registration, compile-contract roots, and no Runtime "
        "composition leak"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

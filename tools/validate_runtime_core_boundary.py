#!/usr/bin/env python3
"""Validate the first runtime-owned element lifecycle seam."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    errors: list[str] = []
    runtime_header = (ROOT / "synaptome/src/runtime/Runtime.h").read_text(encoding="utf-8")
    runtime_source = (ROOT / "synaptome/src/runtime/Runtime.cpp").read_text(encoding="utf-8")
    app = (ROOT / "synaptome/src/ofApp.cpp").read_text(encoding="utf-8")
    app_header = (ROOT / "synaptome/src/ofApp.h").read_text(encoding="utf-8")
    composition_header = (
        ROOT / "synaptome/src/runtime/CompositionLayer.h"
    ).read_text(encoding="utf-8")
    parameter_registry = (
        ROOT / "synaptome/src/core/ParameterRegistry.h"
    ).read_text(encoding="utf-8")
    layer_header = (
        ROOT / "synaptome/src/visuals/Layer.h"
    ).read_text(encoding="utf-8")
    control_hub_header = (
        ROOT / "synaptome/src/ui/ControlMappingHubState.h"
    ).read_text(encoding="utf-8")
    project = (ROOT / "synaptome/Synaptome.vcxproj").read_text(encoding="utf-8")
    solution = (ROOT / "synaptome/Synaptome.sln").read_text(encoding="utf-8")
    core_project = (
        ROOT / "synaptome/runtime/SynaptomeRuntimeCore.vcxproj"
    ).read_text(encoding="utf-8")
    test_project = (
        ROOT / "synaptome/tests/RuntimeCoreTest/RuntimeCoreTest.vcxproj"
    ).read_text(encoding="utf-8")

    for token in ("ofApp", "../ui/", "../io/"):
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
        "prepareElementReplacement",
        "replacingIds",
        "onParameterRegistryCommitted",
        "isReservedCompositionParameter",
        "instanceId",
        "ElementErrorCode",
        "CompositionLayers",
        "adoptPreparedElement",
        "releasePreparedElement",
        "updateCompositionElements",
        "drawCompositionElement",
        "shutdownComposition",
    ):
        if token not in runtime_source and token not in runtime_header:
            errors.append(f"runtime lifecycle seam is missing {token}")
    if "struct ConsoleSlot" in app_header:
        errors.append("ofApp still defines the composition-layer storage record")
    if "Runtime::CompositionLayers& consoleSlots" not in app_header:
        errors.append("ofApp compatibility view must reference runtime-owned composition layers")
    if "std::unique_ptr<Layer> element_" not in composition_header:
        errors.append("runtime composition layer must own its element privately")
    for token in (
        "runtime_.prepareElement",
        "runtime_.prepareElementReplacement",
        "runtime_.adoptPreparedElement",
        "runtime_.releaseCompositionElement",
        "runtime_.resizeCompositionElements",
        "runtime_.updateCompositionElements",
        "runtime_.drawCompositionElement",
        "runtime_.shutdownComposition",
    ):
        if token not in app:
            errors.append(f"ofApp must delegate generic composition behavior: {token}")
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
    clear_start = app.find("void ofApp::clearConsoleSlot")
    clear_end = app.find("ConsoleStore::State ofApp::captureConsoleState", clear_start)
    clear_body = app[clear_start:clear_end]
    if 'paramRegistry.removeById(oldPrefix + ".opacity")' not in clear_body:
        errors.append("host clear flow must remove its exact slot-opacity registration")

    add_start = app.find("bool ofApp::addAssetToConsoleLayer")
    add_end = app.find("void ofApp::openAssetBrowserForConsole", add_start)
    add_body = app[add_start:add_end]
    for forbidden in ("LayerFactory::instance().create", "l->configure(", "l->setup("):
        if forbidden in add_body:
            errors.append(f"host add flow still performs generic lifecycle operation: {forbidden}")
    replacement_prepare = add_body.find("runtime_.prepareElementReplacement")
    replacement_adopt = add_body.find("runtime_.adoptPreparedElement")
    destructive_clear = add_body.find("clearConsoleSlot(idx)")
    registry_invalidation = add_body.find(
        "invalidateParameterRegistryStorage",
        replacement_adopt,
    )
    retired_reference_refresh = add_body.find(
        "refreshLayerReferences()",
        replacement_adopt,
    )
    visual_publish = add_body.find(
        "slot.assetId.swap(nextAssetId)",
        replacement_prepare,
    )
    if min(
        replacement_prepare,
        replacement_adopt,
        destructive_clear,
        registry_invalidation,
        retired_reference_refresh,
        visual_publish,
    ) < 0:
        errors.append("host visual replacement transaction is incomplete")
    elif not (
        replacement_prepare <
        replacement_adopt <
        registry_invalidation <=
        retired_reference_refresh <
        visual_publish <
        destructive_clear
    ):
        errors.append(
            "host must adopt, invalidate registry consumers and retired "
            "references, then publish visual metadata without clearing the "
            "new runtime element"
        )
    if "midi.unbindTargetsByPrefix(prefix)" not in add_body:
        errors.append("replacement must preserve address-based MIDI/OSC maps")
    if "paramRegistry.findFloat(paramId)" not in app:
        errors.append("slot opacity registration must support same-address reuse")
    if "invalidateParameterRegistryStorage() noexcept" not in control_hub_header:
        errors.append(
            "parameter consumers need synchronous no-throw invalidation after "
            "registry storage replacement"
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

    if errors:
        print("[runtime-core-boundary] FAIL")
        for error in errors:
            print(f"  - {error}")
        return 1
    print("[runtime-core-boundary] PASS linked runtime-core library and lifecycle target")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

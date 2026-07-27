#!/usr/bin/env python3
"""Validate the linked RuntimeCore lifecycle and composition control plane."""
from __future__ import annotations

import re
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
    composition_types = (
        ROOT / "synaptome/src/runtime/CompositionTypes.h"
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
        "resolveEffectCoverage",
        "assignCompositionEntry",
        "setCompositionLayerActive",
        "setCompositionLayerLabel",
        "setCompositionLayerCoverage",
        "clearCompositionLayer",
        "compositionRenderTargetsForHost",
        "legacyCompositionElementForHost",
    ):
        if token not in runtime_source and token not in runtime_header:
            errors.append(f"runtime lifecycle seam is missing {token}")
    for token in (
        "enum class CompositionKind",
        "struct CompositionAssignment",
        "enum class CompositionMutationError",
        "struct CompositionMutationResult",
    ):
        if token not in composition_types:
            errors.append(f"runtime composition control plane is missing {token}")
    for pattern, description in (
        (
            r"(?m)^\s*CompositionLayers&\s+compositionLayersForHost\s*\(",
            "mutable composition aggregate view",
        ),
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
    if "coverageParamValue" in composition_header:
        errors.append(
            "composition record still duplicates typed coverage as a host float"
        )
    if "const CompositionLayers& compositionLayersForHost() const" not in runtime_header:
        errors.append("Runtime must expose only the transitional const live composition view")
    if "struct ConsoleSlot" in app_header:
        errors.append("ofApp still defines the composition-layer storage record")
    if (
        "const synaptome::runtime::Runtime::CompositionLayers& consoleSlots"
        not in app_header
    ):
        errors.append(
            "ofApp compatibility view must be const-only over runtime-owned "
            "composition layers"
        )
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
        "runtime_.prepareElementReplacement",
        "runtime_.adoptPreparedElement",
        "runtime_.assignCompositionEntry",
        "runtime_.setCompositionLayerActive",
        "runtime_.setCompositionLayerLabel",
        "runtime_.setCompositionLayerCoverage",
        "runtime_.clearCompositionLayer",
        "runtime_.compositionRenderTargetsForHost",
        "runtime_.resizeCompositionElements",
        "runtime_.updateCompositionElements",
        "runtime_.drawCompositionElement",
        "runtime_.resolveEffectCoverage",
        "runtime_.shutdownComposition",
    ):
        if token not in app:
            errors.append(f"ofApp must delegate generic composition behavior: {token}")
    if "runtime_.legacyCompositionElementForHost" not in app:
        errors.append(
            "remaining element-specific host adapters must use the named "
            "legacy composition-element seam"
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
    if min(
        replacement_prepare,
        replacement_adopt,
        destructive_clear,
        registry_invalidation,
        retired_reference_refresh,
    ) < 0:
        errors.append("host visual replacement transaction is incomplete")
    elif not (
        replacement_prepare <
        replacement_adopt <
        registry_invalidation <=
        retired_reference_refresh <
        destructive_clear
    ):
        errors.append(
            "host must adopt, invalidate registry consumers and retired "
            "references, and keep destructive non-element clear after the "
            "atomic Runtime adoption path"
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
        "CompositionMutationError::",
        "assignCompositionEntry",
        "setCompositionLayerActive",
        "setCompositionLayerLabel",
        "setCompositionLayerCoverage",
        "clearCompositionLayer",
        "compositionRenderTargetsForHost",
        "tests.stable-opacity-mapping",
    ):
        if token not in runtime_test:
            errors.append(f"RuntimeCore test is missing control-plane coverage: {token}")

    if errors:
        print("[runtime-core-boundary] FAIL")
        for error in errors:
            print(f"  - {error}")
        return 1
    print(
        "[runtime-core-boundary] PASS linked RuntimeCore lifecycle and "
        "composition control plane"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

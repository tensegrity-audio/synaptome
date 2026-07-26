#!/usr/bin/env python3
"""Validate the source-level scene/display transaction boundary.

This is intentionally lightweight. It does not prove runtime behavior, but it
does catch the highest-risk regression classes: publish-time side effects
drifting back into scene apply, legacy scenes clearing live mappings, and scene
writes replacing the last-known-good file before a verified recovery copy
exists.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OF_APP = ROOT / "synaptome/src/ofApp.cpp"
MIDI_ROUTER = ROOT / "synaptome/src/io/MidiRouter.cpp"


class ContractError(RuntimeError):
    pass


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise ContractError(f"missing function signature: {signature}")
    brace = source.find("{", start)
    if brace < 0:
        raise ContractError(f"missing function body for: {signature}")

    depth = 0
    for index in range(brace, len(source)):
        char = source[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:index]
    raise ContractError(f"unterminated function body for: {signature}")


def require_all(body: str, snippets: tuple[str, ...], context: str) -> None:
    missing = [snippet for snippet in snippets if snippet not in body]
    if missing:
        raise ContractError(f"{context} missing required snippet(s): {', '.join(missing)}")


def forbid_all(body: str, snippets: tuple[str, ...], context: str) -> None:
    present = [snippet for snippet in snippets if snippet in body]
    if present:
        raise ContractError(f"{context} contains forbidden snippet(s): {', '.join(present)}")


def require_order(body: str, snippets: tuple[str, ...], context: str) -> None:
    positions = []
    for snippet in snippets:
        position = body.find(snippet)
        if position < 0:
            raise ContractError(f"{context} missing ordered snippet: {snippet}")
        positions.append(position)
    if positions != sorted(positions):
        raise ContractError(f"{context} snippets are out of order: {' -> '.join(snippets)}")


def validate() -> None:
    source = OF_APP.read_text(encoding="utf-8", errors="replace")
    midi_source = MIDI_ROUTER.read_text(encoding="utf-8", errors="replace")
    recovery_body = function_body(
        source,
        "bool writeJsonRecoverably(const std::string& path,",
    )
    build_body = function_body(
        source,
        "bool ofApp::buildSceneApplyPlan(const std::string& canonicalPath,",
    )
    apply_body = function_body(source, "bool ofApp::applyScenePlan(SceneApplyPlan& plan)")
    publish_body = function_body(source, "bool ofApp::publishScenePlan(const SceneApplyPlan& plan,")
    rollback_body = function_body(source, "bool ofApp::rollbackSceneLoad(const SceneLoadRollbackSnapshot& snapshot,")
    load_body = function_body(source, "bool ofApp::loadScene(const std::string& path)")
    persist_body = function_body(source, "void ofApp::persistConsoleAssignments()")
    parse_mapping_body = function_body(
        midi_source,
        "ParsedMappingState parseMappingSnapshot(const ofJson& snapshot,",
    )
    mapping_load_body = function_body(
        midi_source,
        "bool MidiRouter::load(const std::string& jsonPath)",
    )
    mapping_import_body = function_body(
        midi_source,
        "bool MidiRouter::importMappingSnapshot(const ofJson& snapshot, bool replaceExisting)",
    )

    require_all(
        recovery_body,
        (
            'const std::string tmpPath = path + ".tmp"',
            'const std::string backupPath = path + ".bak"',
            "ofSavePrettyJson(tmpPath, snapshot)",
            "const ofJson verified = ofLoadJson(tmpPath)",
            "if (verified != snapshot)",
            "ofFile::moveFromTo(path, backupPath, false, true)",
            "ofFile::moveFromTo(tmpPath, path, false, true)",
            "ofFile::moveFromTo(backupPath, path, false, true)",
        ),
        "writeJsonRecoverably",
    )
    require_order(
        recovery_body,
        (
            "ofSavePrettyJson(tmpPath, snapshot)",
            "const ofJson verified = ofLoadJson(tmpPath)",
            "ofFile::moveFromTo(path, backupPath, false, true)",
            "ofFile::moveFromTo(tmpPath, path, false, true)",
            "ofFile::moveFromTo(backupPath, path, false, true)",
        ),
        "writeJsonRecoverably",
    )

    require_all(
        build_body,
        (
            "plan.routerMappingsDefined = true",
            "plan.slotAssignmentsDefined = true",
        ),
        "buildSceneApplyPlan",
    )

    require_all(
        parse_mapping_body,
        (
            'requireObject(snapshot, "$")',
            'requireArraySection(snapshot, "cc")',
            'requireArraySection(snapshot, "buttons")',
            'requireArraySection(snapshot, "oscSources")',
            'requireArraySection(snapshot, "osc")',
            "canonicalizeOscState(initial.oscMaps, initial.oscSourceProfiles)",
        ),
        "parseMappingSnapshot",
    )
    require_all(
        mapping_load_body,
        (
            "preserving current mappings",
            "ofJson candidate = ofLoadJson(candidatePath)",
            "if (!importMappingSnapshot(candidate, true))",
            "close()",
            "mappingPath = jsonPath",
        ),
        "MidiRouter::load",
    )
    require_order(
        mapping_load_body,
        (
            "mappingPath = jsonPath",
            "ofJson candidate = ofLoadJson(candidatePath)",
            "if (!importMappingSnapshot(candidate, true))",
            "close()",
        ),
        "MidiRouter::load",
    )
    require_all(
        mapping_import_body,
        (
            "ParsedMappingState pending",
            "pending = parseMappingSnapshot(snapshot, std::move(pending))",
            "catch (const std::exception& e)",
            "preserving current mappings",
            "ccMaps.swap(pending.ccMaps)",
            "btnMaps.swap(pending.btnMaps)",
            "oscMaps.swap(pending.oscMaps)",
            "oscSourceProfiles.swap(pending.oscSourceProfiles)",
        ),
        "MidiRouter::importMappingSnapshot",
    )
    require_order(
        mapping_import_body,
        (
            "pending = parseMappingSnapshot(snapshot, std::move(pending))",
            "ccMaps.swap(pending.ccMaps)",
            "btnMaps.swap(pending.btnMaps)",
            "oscMaps.swap(pending.oscMaps)",
            "oscSourceProfiles.swap(pending.oscSourceProfiles)",
        ),
        "MidiRouter::importMappingSnapshot",
    )
    forbid_all(
        build_body,
        (
            "plan.routerMappingsDefined = scene.contains",
            "plan.slotAssignmentsDefined = scene.contains",
        ),
        "buildSceneApplyPlan",
    )

    forbid_all(
        apply_body,
        (
            "writeJsonSnapshotAtomically",
            "persistConsoleAssignments",
            "activeScenePath_ =",
            "activeNamedScenePath_ =",
            "midi.importMappingSnapshot",
            "finishSceneLoad(",
            "saveScene(",
            "controlMappingHub->setSlotAssignmentsPath",
        ),
        "applyScenePlan",
    )
    require_all(
        apply_body,
        (
            "consolePersistenceSuspended_ = true",
            "loadConsoleLayoutFromScene",
            "paramRegistry.evaluateAllModifiers",
            "plan.consoleApplied = consoleApplied",
        ),
        "applyScenePlan",
    )

    require_all(
        publish_body,
        (
            "if (plan.routerMappingsDefined &&",
            "midi.importMappingSnapshot(plan.routerSnapshot, true)",
            "if (plan.slotAssignmentsDefined)",
            "writeJsonSnapshotAtomically(slotAssignmentsPath, plan.slotAssignmentsSnapshot)",
            "controlMappingHub->setSlotAssignmentsPath(slotAssignmentsPath)",
            "activeScenePath_ = plan.canonicalPath",
            "activeNamedScenePath_ = plan.activeNamedScenePath",
            'sceneMappingSource_ = plan.routerMappingsDefined ? "scene" : "global (preserved)"',
            "persistConsoleAssignments()",
        ),
        "publishScenePlan",
    )
    require_order(
        publish_body,
        (
            "midi.importMappingSnapshot(plan.routerSnapshot, true)",
            "writeJsonSnapshotAtomically(slotAssignmentsPath, plan.slotAssignmentsSnapshot)",
            "controlMappingHub->setSlotAssignmentsPath(slotAssignmentsPath)",
            "activeScenePath_ = plan.canonicalPath",
            "persistConsoleAssignments()",
        ),
        "publishScenePlan",
    )

    require_all(
        rollback_body,
        (
            "buildSceneApplyPlan(rollbackPath, snapshot.scene, rollbackPlan, error)",
            "applyScenePlan(rollbackPlan)",
            "activeScenePath_ = snapshot.activeScenePath",
            "activeNamedScenePath_ = snapshot.activeNamedScenePath",
            "sceneMappingSource_ = snapshot.mappingSource",
            "midi.importMappingSnapshot(snapshot.routerSnapshot, true)",
            "writeJsonSnapshotAtomically(slotAssignmentsPath, snapshot.slotAssignmentsSnapshot)",
            "syncActiveFxWithConsoleSlots()",
            "refreshLayerReferences()",
        ),
        "rollbackSceneLoad",
    )

    require_all(
        load_body,
        (
            "beginSceneLoadPhase(SceneLoadPhase::Requested",
            "beginSceneLoadPhase(SceneLoadPhase::Parsing",
            "beginSceneLoadPhase(SceneLoadPhase::Validating",
            "beginSceneLoadPhase(SceneLoadPhase::Building",
            "beginSceneLoadPhase(SceneLoadPhase::Applying",
            "beginSceneLoadPhase(SceneLoadPhase::Publishing",
            "captureSceneRollbackSnapshot(canonicalPath)",
            "consolePersistenceSuspended_ = true",
            "applyScenePlan(plan)",
            "publishScenePlan(plan, rollback, error)",
            "rollbackSceneLoad(rollback, canonicalPath, error, false)",
            "rollbackSceneLoad(rollback, canonicalPath, error, true)",
            "finishSceneLoad(true",
        ),
        "loadScene",
    )
    require_order(
        load_body,
        (
            "parseSceneLoadPlan(canonicalPath, plan, error)",
            "buildSceneApplyPlan(canonicalPath, plan.scene, plan, error)",
            "captureSceneRollbackSnapshot(canonicalPath)",
            "applyScenePlan(plan)",
            "publishScenePlan(plan, rollback, error)",
            "finishSceneLoad(true",
        ),
        "loadScene",
    )

    require_all(
        persist_body,
        ("if (consolePersistenceSuspended_) return;",),
        "persistConsoleAssignments",
    )


def main() -> int:
    argparse.ArgumentParser().parse_args()
    try:
        validate()
    except ContractError as exc:
        print(f"Scene/display transaction contract failed: {exc}", file=sys.stderr)
        return 1
    print("Scene/display transaction source contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

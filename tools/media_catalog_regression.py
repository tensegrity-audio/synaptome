#!/usr/bin/env python3
"""Validate Synaptome's manifest-only media catalog and safety fixtures."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path, PurePosixPath
from typing import Any

try:
    import jsonschema  # type: ignore

    JSONSCHEMA_AVAILABLE = True
except ImportError:
    JSONSCHEMA_AVAILABLE = False


BASE_PATH = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = BASE_PATH / "synaptome/bin/data/config/videos.json"
RUNTIME_LAYERS_ROOT = BASE_PATH / "synaptome/bin/data/layers"
SCHEMA_PATH = BASE_PATH / "docs/schemas/media_catalog.schema.json"
PUBLIC_EXAMPLE = BASE_PATH / "docs/examples/media_catalog_example.json"
INVALID_CASES = BASE_PATH / "tools/testdata/media_catalog/invalid_catalog_cases.json"
RUNTIME_CATALOG_SOURCE = BASE_PATH / "synaptome/src/media/VideoCatalog.cpp"
CLIP_ID_RE = re.compile(r"^[a-z0-9][a-z0-9_-]*$")
LAYER_ID_RE = re.compile(r"^[a-z0-9][a-z0-9._-]*$")


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def _unexpected_keys(raw: dict[str, Any], allowed: set[str], context: str) -> list[str]:
    return [f"{context}: unexpected field '{key}'" for key in sorted(set(raw) - allowed)]


def structural_errors(data: Any) -> list[str]:
    if not isinstance(data, dict):
        return ["catalog must be an object"]

    errors: list[str] = []
    root_fields = {"schemaVersion", "discoveryMode", "clips", "layers"}
    errors.extend(_unexpected_keys(data, root_fields, "catalog"))
    for field in root_fields:
        if field not in data:
            errors.append(f"catalog: missing required field '{field}'")
    if data.get("schemaVersion") != 1:
        errors.append("schemaVersion must be 1")
    if data.get("discoveryMode") != "manifest-only":
        errors.append("discoveryMode must be 'manifest-only'")

    clips = data.get("clips")
    if not isinstance(clips, list):
        errors.append("clips must be an array")
        clips = []
    clip_fields = {
        "id", "label", "path", "revision", "sha256", "prewarm", "loop", "replacement", "provenance"
    }
    required_clip_fields = clip_fields - {"replacement"}
    provenance_fields = {"sourceType", "creator", "source", "license", "redistribution", "notes", "generation"}
    required_provenance_fields = provenance_fields - {"generation"}
    for index, raw in enumerate(clips):
        context = f"clips[{index}]"
        if not isinstance(raw, dict):
            errors.append(f"{context} must be an object")
            continue
        errors.extend(_unexpected_keys(raw, clip_fields, context))
        for field in required_clip_fields:
            if field not in raw:
                errors.append(f"{context}: missing required field '{field}'")
        if not isinstance(raw.get("id"), str) or not CLIP_ID_RE.fullmatch(str(raw.get("id", ""))):
            errors.append(f"{context}.id must be a stable lowercase slug")
        for field in ("label", "path"):
            if not isinstance(raw.get(field), str) or not raw.get(field):
                errors.append(f"{context}.{field} must be a non-empty string")
        if not isinstance(raw.get("revision"), int) or raw.get("revision", 0) < 1:
            errors.append(f"{context}.revision must be a positive integer")
        sha256 = raw.get("sha256")
        if not isinstance(sha256, str) or not re.fullmatch(r"[a-f0-9]{64}", sha256):
            errors.append(f"{context}.sha256 must be 64 lowercase hexadecimal characters")
        for field in ("prewarm", "loop"):
            if not isinstance(raw.get(field), bool):
                errors.append(f"{context}.{field} must be boolean")

        provenance = raw.get("provenance")
        if not isinstance(provenance, dict):
            errors.append(f"{context}.provenance must be an object")
        else:
            errors.extend(_unexpected_keys(provenance, provenance_fields, f"{context}.provenance"))
            for field in required_provenance_fields:
                if field not in provenance:
                    errors.append(f"{context}.provenance: missing required field '{field}'")
            if provenance.get("sourceType") not in {"generated", "captured", "authored", "third-party"}:
                errors.append(f"{context}.provenance.sourceType is invalid")
            if provenance.get("redistribution") not in {"allowed", "restricted", "prohibited"}:
                errors.append(f"{context}.provenance.redistribution is invalid")
            for field in ("creator", "source", "license", "notes"):
                if not isinstance(provenance.get(field), str):
                    errors.append(f"{context}.provenance.{field} must be a string")
            generation = provenance.get("generation")
            if generation is not None:
                generation_fields = {"tool", "model", "prompt", "settings"}
                if not isinstance(generation, dict):
                    errors.append(f"{context}.provenance.generation must be an object")
                else:
                    errors.extend(_unexpected_keys(generation, generation_fields, f"{context}.provenance.generation"))
                    for field in generation_fields:
                        if field not in generation:
                            errors.append(f"{context}.provenance.generation: missing required field '{field}'")
                    for field in ("tool", "model", "prompt"):
                        if not isinstance(generation.get(field), str) or not generation.get(field):
                            errors.append(f"{context}.provenance.generation.{field} must be a non-empty string")
                    if not isinstance(generation.get("settings"), dict):
                        errors.append(f"{context}.provenance.generation.settings must be an object")

        replacement = raw.get("replacement")
        if replacement is not None:
            replacement_fields = {"previousSha256", "reason"}
            if not isinstance(replacement, dict):
                errors.append(f"{context}.replacement must be an object")
            else:
                errors.extend(_unexpected_keys(replacement, replacement_fields, f"{context}.replacement"))
                previous = replacement.get("previousSha256")
                if not isinstance(previous, str) or not re.fullmatch(r"[a-f0-9]{64}", previous):
                    errors.append(f"{context}.replacement.previousSha256 is invalid")
                if not isinstance(replacement.get("reason"), str) or not replacement.get("reason"):
                    errors.append(f"{context}.replacement.reason must be a non-empty string")

    layers = data.get("layers")
    if not isinstance(layers, list):
        errors.append("layers must be an array")
        layers = []
    layer_fields = {"id", "defaultClip", "opacity", "blendMode"}
    for index, raw in enumerate(layers):
        context = f"layers[{index}]"
        if not isinstance(raw, dict):
            errors.append(f"{context} must be an object")
            continue
        errors.extend(_unexpected_keys(raw, layer_fields, context))
        for field in ("id", "defaultClip"):
            if not isinstance(raw.get(field), str) or not raw.get(field):
                errors.append(f"{context}.{field} must be a non-empty string")
        if isinstance(raw.get("id"), str) and not LAYER_ID_RE.fullmatch(raw["id"]):
            errors.append(f"{context}.id must be a stable lowercase layer ID")
        if "opacity" in raw and (not isinstance(raw["opacity"], (int, float)) or not 0 <= raw["opacity"] <= 1):
            errors.append(f"{context}.opacity must be between 0 and 1")
        if "blendMode" in raw and (not isinstance(raw["blendMode"], str) or not raw["blendMode"]):
            errors.append(f"{context}.blendMode must be a non-empty string")

    return errors


def schema_errors(data: Any, schema: dict[str, Any]) -> list[str]:
    if not JSONSCHEMA_AVAILABLE:
        return structural_errors(data)
    validator = jsonschema.Draft7Validator(schema)
    errors: list[str] = []
    for error in sorted(validator.iter_errors(data), key=lambda item: list(item.absolute_path)):
        location = ".".join(str(part) for part in error.absolute_path) or "catalog"
        errors.append(f"{location}: {error.message}")
    return errors


def semantic_errors(
    data: Any,
    catalog_path: Path,
    *,
    require_files: bool,
    committed_catalog: bool,
) -> list[str]:
    if not isinstance(data, dict):
        return ["catalog must be an object"]

    errors: list[str] = []
    if data.get("discoveryMode") != "manifest-only":
        errors.append("discoveryMode must remain 'manifest-only'")

    clip_ids: set[str] = set()
    clip_paths: set[str] = set()
    for index, raw in enumerate(data.get("clips", [])):
        if not isinstance(raw, dict):
            continue
        context = f"clips[{index}]"
        clip_id = raw.get("id")
        if isinstance(clip_id, str):
            if clip_id in clip_ids:
                errors.append(f"Duplicate clip id '{clip_id}'")
            clip_ids.add(clip_id)
            if not CLIP_ID_RE.fullmatch(clip_id):
                errors.append(f"{context}.id must be a stable lowercase slug")

        raw_path = raw.get("path")
        if isinstance(raw_path, str):
            if "\\" in raw_path:
                errors.append(f"{context}.path must use forward slashes")
                continue
            media_path = PurePosixPath(raw_path)
            if media_path.is_absolute() or not raw_path.startswith("../media/"):
                errors.append(f"{context}.path must be relative under ../media/")
            if raw_path in clip_paths:
                errors.append(f"Duplicate clip path '{raw_path}'")
            clip_paths.add(raw_path)
            if isinstance(clip_id, str) and media_path.stem != clip_id:
                errors.append(f"{context}.path filename stem must equal clip id '{clip_id}'")
            if committed_catalog and raw_path.startswith("../media/local/"):
                errors.append(f"{context}.path points at operator-local media and cannot be committed")

            file_path = (catalog_path.parent / Path(*media_path.parts)).resolve()
            if require_files and not file_path.is_file():
                errors.append(f"{context}.path does not exist: {raw_path}")
            elif require_files:
                expected_hash = raw.get("sha256")
                actual_hash = hashlib.sha256(file_path.read_bytes()).hexdigest()
                if expected_hash != actual_hash:
                    errors.append(f"{context}.sha256 does not match {raw_path}")

        revision = raw.get("revision")
        replacement = raw.get("replacement")
        if isinstance(revision, int) and revision > 1 and not isinstance(replacement, dict):
            errors.append(f"{context}: revision {revision} requires replacement history")
        if revision == 1 and replacement is not None:
            errors.append(f"{context}: revision 1 must not declare replacement history")

        provenance = raw.get("provenance")
        if isinstance(provenance, dict):
            if provenance.get("sourceType") == "generated" and not isinstance(provenance.get("generation"), dict):
                errors.append(f"{context}: generated media requires provenance.generation")
            if committed_catalog and provenance.get("redistribution") != "allowed":
                errors.append(f"{context}: committed media must allow redistribution")

    layer_ids: set[str] = set()
    for index, raw in enumerate(data.get("layers", [])):
        if not isinstance(raw, dict):
            continue
        context = f"layers[{index}]"
        layer_id = raw.get("id")
        if isinstance(layer_id, str):
            if layer_id in layer_ids:
                errors.append(f"Duplicate media layer id '{layer_id}'")
            layer_ids.add(layer_id)
        default_clip = raw.get("defaultClip")
        if isinstance(default_clip, str) and default_clip not in clip_ids:
            errors.append(f"{context}.defaultClip '{default_clip}' does not resolve")

    return errors


def validate_document(
    path: Path,
    schema: dict[str, Any],
    *,
    require_files: bool,
    committed_catalog: bool,
) -> list[str]:
    data = load_json(path)
    return schema_errors(data, schema) + semantic_errors(
        data,
        path,
        require_files=require_files,
        committed_catalog=committed_catalog,
    )


def runtime_layer_default_errors(catalog_path: Path) -> list[str]:
    catalog = load_json(catalog_path)
    runtime_layers: dict[str, tuple[Path, dict[str, Any]]] = {}
    for path in RUNTIME_LAYERS_ROOT.rglob("*.json"):
        if "scenes" in path.parts:
            continue
        value = load_json(path)
        if isinstance(value, dict) and isinstance(value.get("id"), str):
            runtime_layers[str(value["id"])] = (path, value)

    errors: list[str] = []
    for raw in catalog.get("layers", []):
        if not isinstance(raw, dict):
            continue
        layer_id = raw.get("id")
        if not isinstance(layer_id, str):
            continue
        resolved = runtime_layers.get(layer_id)
        if resolved is None:
            errors.append(f"media layer default id '{layer_id}' does not resolve in the runtime layer catalog")
            continue
        path, layer = resolved
        defaults = layer.get("defaults", {})
        runtime_clip = defaults.get("clipId") if isinstance(defaults, dict) else None
        if runtime_clip != raw.get("defaultClip"):
            errors.append(
                f"{path.relative_to(BASE_PATH).as_posix()} defaults.clipId '{runtime_clip}' "
                f"does not match videos.json defaultClip '{raw.get('defaultClip')}'"
            )
    return errors


def runtime_path_resolution_errors() -> list[str]:
    source = RUNTIME_CATALOG_SOURCE.read_text(encoding="utf-8")
    required_fragments = (
        "std::filesystem::path(jsonPath).parent_path()",
        "(catalogDirectory / rawPath).lexically_normal().string()",
    )
    return [
        "VideoCatalog runtime must resolve manifest paths relative to videos.json"
        for fragment in required_fragments
        if fragment not in source
    ]


def validate_negative_fixtures(schema: dict[str, Any]) -> list[str]:
    fixture = load_json(INVALID_CASES)
    errors: list[str] = []
    cases = fixture.get("cases", []) if isinstance(fixture, dict) else []
    for index, case in enumerate(cases):
        if not isinstance(case, dict):
            errors.append(f"invalid fixture case {index} must be an object")
            continue
        name = str(case.get("name", f"case-{index}"))
        expected = str(case.get("expectedError", ""))
        catalog = case.get("catalog")
        require_files = bool(case.get("requireFiles", False))
        actual = schema_errors(catalog, schema) + semantic_errors(
            catalog,
            INVALID_CASES,
            require_files=require_files,
            committed_catalog=True,
        )
        if not expected or not any(expected in message for message in actual):
            errors.append(f"negative fixture '{name}' did not produce expected error: {expected}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--check", action="store_true", help="Validate the canonical catalog and fixtures")
    args = parser.parse_args()

    catalog_path = args.catalog if args.catalog.is_absolute() else (BASE_PATH / args.catalog)
    schema = load_json(SCHEMA_PATH)
    checks = (
        (
            "canonical catalog",
            validate_document(catalog_path, schema, require_files=True, committed_catalog=True)
            + runtime_layer_default_errors(catalog_path)
            + runtime_path_resolution_errors(),
        ),
        ("public empty example", validate_document(PUBLIC_EXAMPLE, schema, require_files=False, committed_catalog=True)),
        ("negative fixtures", validate_negative_fixtures(schema)),
    )

    failed = False
    for name, errors in checks:
        if errors:
            failed = True
            print(f"{name} failed:")
            for error in errors:
                print(f"  - {error}")
        else:
            print(f"{name}: passed")

    if failed:
        return 1

    catalog = load_json(catalog_path)
    print(
        "Media catalog contract passed "
        f"({len(catalog.get('clips', []))} clips, {len(catalog.get('layers', []))} layer defaults, manifest-only)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

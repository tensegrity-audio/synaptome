#!/usr/bin/env python3
"""Controlled, construction-free package and generated STL discovery.

The module is intentionally usable without the Synaptome application.  A
refresh reads only explicitly configured roots, validates all candidates, and
returns one immutable JSON-compatible snapshot.  Activation is a separate
transaction which accepts only a candidate proven available by that snapshot.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath
from typing import Any, Callable, Iterable, Mapping

import element_package_v1
import generated_layer_catalog_regression
import layer_catalog_regression
import layer_package_catalog_regression

REPO_ROOT = Path(__file__).resolve().parents[1]
SUPPORTED_SCHEMA_VERSION = 1
CONFIG_SCHEMA = REPO_ROOT / "docs/schemas/controlled_package_discovery.schema.json"
REGISTRATION_SET = (
    REPO_ROOT / "docs/contracts/element_package_registration_set_v1.json"
)
DATA_TYPE_SET = (
    REPO_ROOT / "docs/contracts/data_content_type_registration_set_v1.json"
)
PACKAGE_MANIFEST = "layer.package.json"
TEMPLATE_MANIFEST = "generated_layer.template.json"
KNOWN_ROOTS = {
    "fixture-packages": REPO_ROOT / "docs/examples/layer_packages",
    "fixture-generated-stl": REPO_ROOT / "docs/examples/generated_layers",
    "runtime-packages": REPO_ROOT / "synaptome/bin/data/layer_packages",
}
STATUS_VALUES = frozenset(
    {
        "available",
        "inspection-only",
        "invalid",
        "incompatible",
        "duplicate",
        "unavailable",
        "pending-replacement",
    }
)
ROOT_KINDS = frozenset({"packages", "generated-stl"})
_SEMVER = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")


def _canonical(value: Any) -> bytes:
    return json.dumps(
        value, sort_keys=True, separators=(",", ":"), ensure_ascii=False
    ).encode("utf-8")


def _sha256(chunks: Iterable[bytes]) -> str:
    digest = hashlib.sha256()
    for chunk in chunks:
        digest.update(len(chunk).to_bytes(8, "big"))
        digest.update(chunk)
    return digest.hexdigest()


def _relative(path: Path, root: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def _contained(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except (OSError, ValueError):
        return False


def _version(value: str) -> tuple[int, int, int] | None:
    match = _SEMVER.fullmatch(value)
    return tuple(int(match.group(i)) for i in range(1, 4)) if match else None


def _diagnostic(code: str, message: str, **detail: Any) -> dict[str, Any]:
    result = {"code": code, "message": message}
    result.update({key: value for key, value in detail.items() if value not in (None, "")})
    return result


@dataclass(frozen=True)
class DiscoveryRoot:
    id: str
    kind: str
    path: Path
    enabled: bool = True


@dataclass(frozen=True)
class DiscoveryConfig:
    enabled: bool = False
    roots: tuple[DiscoveryRoot, ...] = ()
    schema_version: int = SUPPORTED_SCHEMA_VERSION


@dataclass(frozen=True)
class Candidate:
    candidate_id: str
    kind: str
    status: str
    package_id: str = ""
    package_version: str = ""
    implementation_version: str = ""
    type_id: str = ""
    definition_id: str = ""
    registry_prefix: str = ""
    template_id: str = ""
    content_id: str = ""
    content_version: str = ""
    signature: str = ""
    relative_path: str = ""
    local_path: str = ""
    root_ids: tuple[str, ...] = ()
    diagnostics: tuple[Mapping[str, Any], ...] = ()
    registered_type_available: bool = False
    metadata: Mapping[str, Any] = field(default_factory=dict)
    catalog: Mapping[str, Any] = field(default_factory=dict)
    previous_status: str = ""
    previous_signature: str = ""

    @property
    def activatable(self) -> bool:
        return self.status == "available"

    def to_json(self) -> dict[str, Any]:
        return {
            "candidateId": self.candidate_id,
            "kind": self.kind,
            "status": self.status,
            "activatable": self.activatable,
            "identity": {
                "packageId": self.package_id,
                "packageVersion": self.package_version,
                "implementationVersion": self.implementation_version,
                "typeId": self.type_id,
                "definitionId": self.definition_id,
                "registryPrefix": self.registry_prefix,
                "templateId": self.template_id,
                "contentId": self.content_id,
                "contentVersion": self.content_version,
            },
            "signature": self.signature,
            "registeredTypeAvailable": self.registered_type_available,
            "provenance": {
                "rootIds": list(self.root_ids),
                "relativePath": self.relative_path,
                "localPath": self.local_path,
                "localOnly": True,
            },
            "diagnostics": [dict(item) for item in self.diagnostics],
            "previous": {
                "status": self.previous_status,
                "signature": self.previous_signature,
            },
            "metadata": dict(self.metadata),
            "catalog": dict(self.catalog),
        }


@dataclass(frozen=True)
class DiscoverySnapshot:
    generation: int
    enabled: bool
    stale: bool
    candidates: tuple[Candidate, ...]
    diagnostics: tuple[Mapping[str, Any], ...] = ()
    schema_version: int = SUPPORTED_SCHEMA_VERSION

    def to_json(self) -> dict[str, Any]:
        counts = {status: 0 for status in sorted(STATUS_VALUES)}
        for candidate in self.candidates:
            counts[candidate.status] += 1
        return {
            "schemaVersion": self.schema_version,
            "generation": self.generation,
            "enabled": self.enabled,
            "stale": self.stale,
            "constructionFree": True,
            "activationRequired": True,
            "counts": {
                "candidates": len(self.candidates),
                "activatable": sum(item.activatable for item in self.candidates),
                "statuses": counts,
            },
            "diagnostics": [dict(item) for item in self.diagnostics],
            "candidates": [item.to_json() for item in self.candidates],
        }


class DiscoveryConfigError(ValueError):
    pass


def parse_config(document: Any, *, base: Path) -> DiscoveryConfig:
    if not isinstance(document, dict):
        raise DiscoveryConfigError("discovery config root must be an object")
    allowed = {"schemaVersion", "enabled", "roots"}
    unknown = sorted(set(document) - allowed)
    if unknown:
        raise DiscoveryConfigError("unknown discovery config fields: " + ", ".join(unknown))
    if document.get("schemaVersion") != SUPPORTED_SCHEMA_VERSION:
        raise DiscoveryConfigError("discovery config requires exact schemaVersion 1")
    if not isinstance(document.get("enabled"), bool):
        raise DiscoveryConfigError("discovery config enabled must be boolean")
    raw_roots = document.get("roots")
    if not isinstance(raw_roots, list):
        raise DiscoveryConfigError("discovery config roots must be an array")
    roots: list[DiscoveryRoot] = []
    seen: set[str] = set()
    for index, raw in enumerate(raw_roots):
        if not isinstance(raw, dict):
            raise DiscoveryConfigError(f"roots[{index}] must be an object")
        unknown_root = sorted(set(raw) - {"id", "kind", "path", "enabled"})
        if unknown_root:
            raise DiscoveryConfigError(
                f"roots[{index}] has unknown fields: {', '.join(unknown_root)}"
            )
        root_id = raw.get("id")
        kind = raw.get("kind")
        raw_path = raw.get("path")
        enabled = raw.get("enabled", True)
        if not isinstance(root_id, str) or not root_id:
            raise DiscoveryConfigError(f"roots[{index}].id must be a non-empty string")
        if root_id in seen:
            raise DiscoveryConfigError(f"duplicate discovery root id '{root_id}'")
        if kind not in ROOT_KINDS:
            raise DiscoveryConfigError(
                f"roots[{index}].kind must be packages or generated-stl"
            )
        if not isinstance(raw_path, str) or not raw_path:
            raise DiscoveryConfigError(f"roots[{index}].path must be a non-empty string")
        if not isinstance(enabled, bool):
            raise DiscoveryConfigError(f"roots[{index}].enabled must be boolean")
        path = Path(raw_path)
        resolved = path.resolve() if path.is_absolute() else (base / path).resolve()
        roots.append(DiscoveryRoot(root_id, kind, resolved, enabled))
        seen.add(root_id)
    return DiscoveryConfig(bool(document["enabled"]), tuple(roots))


def load_config(path: Path | None) -> DiscoveryConfig:
    if path is None or not path.exists():
        return DiscoveryConfig()
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise DiscoveryConfigError(
            f"invalid discovery config JSON at line {exc.lineno}, column {exc.colno}"
        ) from exc
    return parse_config(document, base=path.parent)


def controlled_registrations(
    registration_set: Path = REGISTRATION_SET,
    data_type_set: Path = DATA_TYPE_SET,
) -> tuple[dict[str, dict[str, Any]], dict[str, dict[str, Any]]]:
    packages: dict[str, dict[str, Any]] = {}
    types: dict[str, dict[str, Any]] = {}
    if registration_set.exists():
        raw = json.loads(registration_set.read_text(encoding="utf-8"))
        for package_ref in raw.get("packages", []):
            path = Path(package_ref)
            path = path if path.is_absolute() else REPO_ROOT / path
            result = element_package_v1.validate_package(path)
            if not result.valid or result.document is None or result.normalized is None:
                continue
            document = result.document
            record = {
                "packageId": document["packageId"],
                "packageVersion": document["packageVersion"],
                "implementationVersion": document["implementationVersion"],
                "typeId": document["element"]["id"],
                "definitionId": document["asset"]["id"],
                "registryPrefix": document["asset"]["registryPrefix"],
                "descriptorSignature": element_package_v1.descriptor_signature(
                    result.normalized
                ),
            }
            packages[record["packageId"]] = record
            types[record["typeId"]] = record
    if data_type_set.exists():
        raw = json.loads(data_type_set.read_text(encoding="utf-8"))
        for record in raw.get("types", []):
            if isinstance(record, dict) and isinstance(record.get("typeId"), str):
                types[record["typeId"]] = dict(record)
    return packages, types


def _iter_manifests(root: DiscoveryRoot, name: str) -> list[Path]:
    if not root.path.is_dir():
        return []
    found: list[Path] = []
    for directory, dirnames, filenames in os.walk(root.path, followlinks=False):
        directory_path = Path(directory)
        kept: list[str] = []
        for dirname in sorted(dirnames, key=str.casefold):
            child = directory_path / dirname
            if child.is_symlink():
                continue
            kept.append(dirname)
        dirnames[:] = kept
        for filename in sorted(filenames, key=str.casefold):
            if filename == name:
                path = directory_path / filename
                if _contained(path, root.path):
                    found.append(path)
    return sorted(found, key=lambda item: _relative(item, root.path).casefold())


def _coalesced_paths(
    roots: Iterable[DiscoveryRoot], name: str
) -> list[tuple[Path, tuple[DiscoveryRoot, ...]]]:
    by_physical: dict[Path, tuple[Path, list[DiscoveryRoot]]] = {}
    for root in roots:
        for path in _iter_manifests(root, name):
            physical = path.resolve()
            if physical not in by_physical:
                by_physical[physical] = (path, [])
            by_physical[physical][1].append(root)
    return sorted(
        ((path, tuple(provenance)) for path, provenance in by_physical.values()),
        key=lambda item: (
            min(_relative(item[0], root.path).casefold() for root in item[1]),
            str(item[0]).casefold(),
        ),
    )


def _package_signature(result: element_package_v1.ValidationResult) -> str:
    assert result.document is not None
    chunks: list[bytes] = [_canonical(result.document)]
    paths: set[Path] = {result.path}
    for values in result.inventory.values():
        for item in values:
            raw = item.get("path") if isinstance(item, dict) else None
            if isinstance(raw, str) and raw:
                candidate = Path(raw)
                paths.add(
                    candidate if candidate.is_absolute() else result.path.parent / candidate
                )
    for path in sorted(paths, key=lambda item: item.as_posix()):
        if path.is_file():
            chunks.extend((path.name.encode("utf-8"), path.read_bytes()))
    return _sha256(chunks)


def _package_candidate(
    path: Path,
    roots: tuple[DiscoveryRoot, ...],
    registered_packages: Mapping[str, Mapping[str, Any]],
) -> Candidate:
    result = element_package_v1.validate_package(path)
    document = result.document or {}
    package_id = str(document.get("packageId", ""))
    package_version = str(document.get("packageVersion", ""))
    element = document.get("element", {}) if isinstance(document.get("element"), dict) else {}
    asset = document.get("asset", {}) if isinstance(document.get("asset"), dict) else {}
    type_id = str(element.get("id", ""))
    definition_id = str(asset.get("id", ""))
    prefix = str(asset.get("registryPrefix", ""))
    candidate_id = f"package:{package_id or _relative(path, roots[0].path)}"
    diagnostics = tuple(item.to_json() for item in result.diagnostics)
    signature = _package_signature(result) if result.valid else ""
    registration = registered_packages.get(package_id)
    parity = bool(
        registration
        and registration.get("packageVersion") == package_version
        and registration.get("implementationVersion")
        == document.get("implementationVersion")
        and registration.get("typeId") == type_id
        and registration.get("definitionId") == definition_id
        and registration.get("registryPrefix") == prefix
        and result.normalized is not None
        and registration.get("descriptorSignature")
        == element_package_v1.descriptor_signature(result.normalized)
    )
    if not result.valid:
        incompatible_codes = {
            "package.schema-version",
            "package.package-version-unsupported",
            "package.compatibility-version",
            "package.capability-unknown",
            "package.dependency-version",
        }
        status = (
            "incompatible"
            if any(
                item.get("code")
                in {"package.schema-version", "package.package-version-unsupported"}
                for item in diagnostics
            )
            or (
                diagnostics
                and all(item.get("code") in incompatible_codes for item in diagnostics)
            )
            else "invalid"
        )
    elif parity:
        status = "available"
    elif registration:
        status = "incompatible"
        diagnostics += (
            _diagnostic(
                "discovery.registration-parity",
                "package declaration does not exactly match its controlled registration",
                packageId=package_id,
            ),
        )
    else:
        status = "inspection-only"
        diagnostics += (
            _diagnostic(
                "discovery.type-not-registered",
                "source package type is not compiled into this host",
                typeId=type_id,
            ),
        )
    metadata = {
        "label": asset.get("label", package_id),
        "category": asset.get("category", ""),
        "layerGroup": asset.get("layerGroup", ""),
        "compatibility": result.resolved_compatibility,
        "dependencies": result.resolved_dependencies,
        "capabilities": document.get("capabilities", []),
        "parameters": document.get("parameters", []),
        "actions": element.get("actions", []),
        "presets": document.get("presets", []),
        "presetBanks": document.get("presetBanks", []),
        "mappingPresets": document.get("mappingPresets", []),
    }
    catalog, catalog_errors = layer_package_catalog_regression.normalize_package(path)
    if catalog_errors or catalog is None:
        catalog = {}
    else:
        catalog = dict(catalog)
        catalog["defaults"] = {
            str(item.get("id")): item.get("default")
            for item in catalog.get("parameters", [])
            if isinstance(item, dict)
        }
        catalog["discovery"] = {
            "candidateId": candidate_id,
            "signature": signature,
            "localOnly": True,
        }
    return Candidate(
        candidate_id=candidate_id,
        kind="package",
        status=status,
        package_id=package_id,
        package_version=package_version,
        implementation_version=str(document.get("implementationVersion", "")),
        type_id=type_id,
        definition_id=definition_id,
        registry_prefix=prefix,
        signature=signature,
        relative_path=min(_relative(path, root.path) for root in roots),
        local_path=str(path.resolve()),
        root_ids=tuple(sorted(root.id for root in roots)),
        diagnostics=diagnostics,
        registered_type_available=parity,
        metadata=metadata,
        catalog=catalog,
    )


def _content_signature(
    template: Mapping[str, Any],
    sidecar: Mapping[str, Any],
    content_path: Path,
    content_id: str,
) -> str:
    return _sha256(
        (
            _canonical(
                {
                    "templateId": template.get("templateId"),
                    "schemaVersion": template.get("schemaVersion"),
                    "contentId": content_id,
                }
            ),
            _canonical(template),
            _canonical(sidecar),
            content_path.read_bytes(),
        )
    )


def _data_candidates(
    template_path: Path,
    roots: tuple[DiscoveryRoot, ...],
    registered_types: Mapping[str, Mapping[str, Any]],
) -> list[Candidate]:
    entries, errors = generated_layer_catalog_regression.normalize_template(template_path)
    try:
        template = json.loads(template_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        template = {}
    template_id = str(template.get("templateId", ""))
    implementation_type = str(
        template.get("source", {}).get("implementationType", "")
        if isinstance(template.get("source"), dict)
        else ""
    )
    registration = registered_types.get(implementation_type)
    template_allowed = bool(
        registration
        and template_id in registration.get("templateIds", [template_id])
        and "stl" in registration.get("contentKinds", ["stl"])
    )
    if not entries:
        return [
            Candidate(
                candidate_id=f"template:{template_id or _relative(template_path, roots[0].path)}",
                kind="generated-stl",
                status="invalid",
                template_id=template_id,
                type_id=implementation_type,
                relative_path=min(_relative(template_path, root.path) for root in roots),
                local_path=str(template_path.resolve()),
                root_ids=tuple(sorted(root.id for root in roots)),
                diagnostics=tuple(
                    _diagnostic("discovery.template-invalid", message)
                    for message in (errors or ["template produced no content candidates"])
                ),
            )
        ]
    candidates: list[Candidate] = []
    for entry in entries:
        content = entry.get("content", {})
        raw_content_path = Path(str(content.get("path", "")))
        content_path = (
            raw_content_path
            if raw_content_path.is_absolute()
            else REPO_ROOT / raw_content_path
        )
        raw_sidecar = Path(str(content.get("sidecar", "")))
        sidecar_path = (
            raw_sidecar if raw_sidecar.is_absolute() else REPO_ROOT / raw_sidecar
        )
        try:
            sidecar = (
                json.loads(sidecar_path.read_text(encoding="utf-8"))
                if str(content.get("sidecar", ""))
                else {}
            )
        except (OSError, json.JSONDecodeError):
            sidecar = {}
        root = min(
            (root for root in roots if _contained(content_path, root.path)),
            key=lambda item: len(str(item.path)),
            default=roots[0],
        )
        relative_content = _relative(content_path, root.path)
        explicit_id = sidecar.get("contentId") if isinstance(sidecar, dict) else None
        content_id = (
            str(explicit_id)
            if isinstance(explicit_id, str) and explicit_id
            else f"{template_id}:{PurePosixPath(relative_content).with_suffix('').as_posix()}"
        )
        signature = (
            _content_signature(template, sidecar, content_path, content_id)
            if content_path.is_file()
            else ""
        )
        diagnostics = [
            _diagnostic("discovery.template-invalid", message) for message in errors
        ]
        if not content_path.is_file():
            status = "unavailable"
            diagnostics.append(
                _diagnostic(
                    "discovery.content-missing",
                    "declared generated content is unavailable",
                    contentId=content_id,
                )
            )
        elif errors:
            status = "invalid"
        elif not registration:
            status = "unavailable"
            diagnostics.append(
                _diagnostic(
                    "discovery.type-not-registered",
                    "generated content requires an unavailable precompiled type",
                    typeId=implementation_type,
                )
            )
        elif not template_allowed:
            status = "incompatible"
            diagnostics.append(
                _diagnostic(
                    "discovery.template-incompatible",
                    "registered type does not authorize this template/content family",
                    templateId=template_id,
                )
            )
        else:
            status = "available"
        definition_id = str(entry.get("id", ""))
        prefix = str(entry.get("registryPrefix", ""))
        defaults = {
            str(item.get("id")): item.get("default")
            for item in entry.get("parameters", [])
            if isinstance(item, dict)
        }
        catalog = {
            "id": definition_id,
            "label": entry.get("label", definition_id),
            "category": entry.get("category", "Generated"),
            "layerGroup": entry.get("layerGroup", ""),
            "type": implementation_type,
            "registryPrefix": prefix,
            "assetPath": str(content_path.resolve()),
            "defaults": defaults,
            "discovery": {
                "candidateId": f"content:{content_id}",
                "signature": signature,
                "localOnly": True,
            },
        }
        candidates.append(
            Candidate(
                candidate_id=f"content:{content_id}",
                kind="generated-stl",
                status=status,
                type_id=implementation_type,
                definition_id=definition_id,
                registry_prefix=prefix,
                template_id=template_id,
                content_id=content_id,
                content_version=str(sidecar.get("contentVersion", "1.0.0")),
                signature=signature,
                relative_path=relative_content,
                local_path=str(content_path.resolve()),
                root_ids=tuple(sorted(root.id for root in roots)),
                diagnostics=tuple(diagnostics),
                registered_type_available=status == "available",
                metadata={
                    "label": entry.get("label", definition_id),
                    "category": entry.get("category", ""),
                    "layerGroup": entry.get("layerGroup", ""),
                    "parameters": entry.get("parameters", []),
                    "content": content,
                },
                catalog=catalog,
            )
        )
    return candidates


def _legacy_identities() -> dict[str, set[str]]:
    catalog = layer_catalog_regression.build_catalog()
    result = {"definitionId": set(), "registryPrefix": set()}
    for entry in catalog.get("entries", []):
        if isinstance(entry, dict):
            result["definitionId"].add(str(entry.get("id", "")))
            result["registryPrefix"].add(
                str(entry.get("registryPrefix", entry.get("id", "")))
            )
    return result


def _replace(candidate: Candidate, **changes: Any) -> Candidate:
    values = {
        field_name: getattr(candidate, field_name)
        for field_name in Candidate.__dataclass_fields__
    }
    values.update(changes)
    return Candidate(**values)


def _collision_analysis(
    candidates: list[Candidate],
    registered_packages: Mapping[str, Mapping[str, Any]],
    registered_types: Mapping[str, Mapping[str, Any]],
) -> list[Candidate]:
    claims: dict[tuple[str, str], list[int]] = {}
    fields = (
        ("packageId", "package_id"),
        ("typeId", "type_id"),
        ("definitionId", "definition_id"),
        ("registryPrefix", "registry_prefix"),
        ("templateId", "template_id"),
        ("contentId", "content_id"),
    )
    for index, candidate in enumerate(candidates):
        for public_name, attr in fields:
            value = getattr(candidate, attr)
            if value:
                claims.setdefault((public_name, value), []).append(index)
    duplicate_indices: dict[int, list[dict[str, Any]]] = {}
    for (kind, value), indices in claims.items():
        if len(set(indices)) > 1:
            claimants = sorted(candidates[index].candidate_id for index in indices)
            for index in indices:
                duplicate_indices.setdefault(index, []).append(
                    _diagnostic(
                        "discovery.duplicate",
                        f"{kind} '{value}' has multiple live claimants",
                        identityKind=kind,
                        identity=value,
                        claimants=claimants,
                    )
                )
    legacy = _legacy_identities()
    for index, candidate in enumerate(candidates):
        for public_name, attr in (
            ("definitionId", "definition_id"),
            ("registryPrefix", "registry_prefix"),
        ):
            value = getattr(candidate, attr)
            if value and value in legacy[public_name]:
                duplicate_indices.setdefault(index, []).append(
                    _diagnostic(
                        "discovery.legacy-collision",
                        f"{public_name} '{value}' collides with the canonical catalog",
                        identityKind=public_name,
                        identity=value,
                    )
                )
        # A matching controlled package/type is the implementation backing,
        # not a second claimant. A mismatching reuse is already incompatible.
        if candidate.kind == "generated-stl" and candidate.type_id in registered_types:
            record = registered_types[candidate.type_id]
            allowed_templates = record.get("templateIds", [])
            if allowed_templates and candidate.template_id not in allowed_templates:
                duplicate_indices.setdefault(index, []).append(
                    _diagnostic(
                        "discovery.controlled-type-collision",
                        "content candidate is not authorized by the controlled type record",
                        typeId=candidate.type_id,
                    )
                )
        if candidate.kind == "package" and candidate.package_id in registered_packages:
            pass
    result: list[Candidate] = []
    for index, candidate in enumerate(candidates):
        if index in duplicate_indices:
            candidate = _replace(
                candidate,
                status="duplicate",
                diagnostics=candidate.diagnostics
                + tuple(duplicate_indices[index]),
                registered_type_available=False,
            )
        result.append(candidate)
    return result


def _prior_candidates(prior: DiscoverySnapshot | None) -> dict[str, Candidate]:
    return {item.candidate_id: item for item in prior.candidates} if prior else {}


def refresh(
    config: DiscoveryConfig,
    *,
    prior: DiscoverySnapshot | None = None,
    active_versions: Mapping[str, tuple[str, str]] | None = None,
    registrations: tuple[
        Mapping[str, Mapping[str, Any]], Mapping[str, Mapping[str, Any]]
    ]
    | None = None,
) -> DiscoverySnapshot:
    generation = (prior.generation + 1) if prior else 1
    if not config.enabled:
        return DiscoverySnapshot(generation, False, False, ())
    enabled_roots = tuple(root for root in config.roots if root.enabled)
    root_diagnostics: list[dict[str, Any]] = []
    for root in enabled_roots:
        if not root.path.exists():
            root_diagnostics.append(
                _diagnostic(
                    "discovery.root-missing",
                    "configured discovery root does not exist",
                    rootId=root.id,
                    localPath=str(root.path),
                )
            )
        elif not root.path.is_dir():
            root_diagnostics.append(
                _diagnostic(
                    "discovery.root-not-directory",
                    "configured discovery root is not a directory",
                    rootId=root.id,
                    localPath=str(root.path),
                )
            )
    registered_packages, registered_types = (
        registrations or controlled_registrations()
    )
    candidates: list[Candidate] = []
    package_roots = tuple(
        root for root in enabled_roots if root.kind == "packages" and root.path.is_dir()
    )
    content_roots = tuple(
        root
        for root in enabled_roots
        if root.kind == "generated-stl" and root.path.is_dir()
    )
    for path, roots in _coalesced_paths(package_roots, PACKAGE_MANIFEST):
        candidates.append(_package_candidate(path, roots, registered_packages))
    for path, roots in _coalesced_paths(content_roots, TEMPLATE_MANIFEST):
        candidates.extend(_data_candidates(path, roots, registered_types))
    candidates = _collision_analysis(candidates, registered_packages, registered_types)

    previous = _prior_candidates(prior)
    active_versions = active_versions or {}
    refreshed: list[Candidate] = []
    for candidate in candidates:
        old = previous.get(candidate.candidate_id)
        if old:
            candidate = _replace(
                candidate,
                previous_status=old.status,
                previous_signature=old.signature,
            )
            same_version = (
                candidate.package_version or candidate.content_version
            ) == (old.package_version or old.content_version)
            if (
                same_version
                and old.signature
                and candidate.signature
                and old.signature != candidate.signature
            ):
                candidate = _replace(
                    candidate,
                    status="duplicate",
                    registered_type_available=False,
                    diagnostics=candidate.diagnostics
                    + (
                        _diagnostic(
                            "discovery.same-version-drift",
                            "the same stable identity/version now has different bytes",
                            previousSignature=old.signature,
                            currentSignature=candidate.signature,
                        ),
                    ),
                )
        active = active_versions.get(candidate.candidate_id)
        if active and candidate.status == "available":
            active_version, active_signature = active
            new_version = candidate.package_version or candidate.content_version
            parsed_active = _version(active_version)
            parsed_new = _version(new_version)
            if (
                parsed_active
                and parsed_new
                and parsed_new > parsed_active
                and candidate.signature != active_signature
            ):
                candidate = _replace(
                    candidate,
                    status="pending-replacement",
                    diagnostics=candidate.diagnostics
                    + (
                        _diagnostic(
                            "discovery.replacement-review",
                            "a compatible higher version requires explicit acceptance",
                            activeVersion=active_version,
                            candidateVersion=new_version,
                        ),
                    ),
                )
        refreshed.append(candidate)
    current_ids = {item.candidate_id for item in refreshed}
    for candidate_id, old in previous.items():
        if candidate_id not in current_ids:
            refreshed.append(
                _replace(
                    old,
                    status="unavailable",
                    registered_type_available=False,
                    previous_status=old.status,
                    previous_signature=old.signature,
                    diagnostics=old.diagnostics
                    + (
                        _diagnostic(
                            "discovery.removed",
                            "candidate is absent from the current refresh",
                        ),
                    ),
                )
            )
    refreshed.sort(key=lambda item: (item.candidate_id, item.signature))
    return DiscoverySnapshot(
        generation,
        True,
        False,
        tuple(refreshed),
        tuple(root_diagnostics),
    )


def refresh_atomically(
    config: DiscoveryConfig,
    *,
    prior: DiscoverySnapshot | None = None,
    scanner: Callable[..., DiscoverySnapshot] = refresh,
    **kwargs: Any,
) -> DiscoverySnapshot:
    try:
        return scanner(config, prior=prior, **kwargs)
    except Exception as exc:
        if prior is None:
            return DiscoverySnapshot(
                1,
                config.enabled,
                True,
                (),
                (
                    _diagnostic(
                        "discovery.refresh-failed",
                        "refresh failed before a snapshot could be published",
                        detail=str(exc),
                    ),
                ),
            )
        return DiscoverySnapshot(
            prior.generation,
            prior.enabled,
            True,
            prior.candidates,
            prior.diagnostics
            + (
                _diagnostic(
                    "discovery.refresh-failed",
                    "refresh failed; the prior snapshot remains authoritative",
                    detail=str(exc),
                ),
            ),
        )


@dataclass(frozen=True)
class ActivationResult:
    ok: bool
    rollback_succeeded: bool
    error: str
    activation: Mapping[str, Any]
    catalog: tuple[Mapping[str, Any], ...]


def activate_candidate(
    snapshot: DiscoverySnapshot,
    candidate_id: str,
    activation: Mapping[str, Any],
    catalog: Iterable[Mapping[str, Any]],
    *,
    persist_activation: Callable[[Mapping[str, Any]], bool],
    publish_catalog: Callable[[tuple[Mapping[str, Any], ...]], bool],
    prepare_runtime: Callable[[Mapping[str, Any]], Any],
    adopt_runtime: Callable[[Any], bool],
    accept_replacement: bool = False,
) -> ActivationResult:
    previous_activation = dict(activation)
    previous_catalog = tuple(dict(item) for item in catalog)
    candidate = next(
        (item for item in snapshot.candidates if item.candidate_id == candidate_id),
        None,
    )
    if candidate is None:
        return ActivationResult(
            False, True, "candidate is not present in the inspected snapshot",
            previous_activation, previous_catalog
        )
    status_allowed = candidate.status == "available" or (
        candidate.status == "pending-replacement" and accept_replacement
    )
    if snapshot.stale or not snapshot.enabled or not status_allowed:
        return ActivationResult(
            False, True, "candidate is not available for activation",
            previous_activation, previous_catalog
        )
    if not candidate.registered_type_available:
        return ActivationResult(
            False, True, "candidate type is not in the controlled registration set",
            previous_activation, previous_catalog
        )
    candidate_activation = dict(previous_activation)
    records = list(candidate_activation.get("candidates", []))
    records = [
        item
        for item in records
        if not isinstance(item, dict) or item.get("candidateId") != candidate_id
    ]
    records.append(
        {
            "candidateId": candidate_id,
            "signature": candidate.signature,
            "typeId": candidate.type_id,
            "definitionId": candidate.definition_id,
            "enabled": True,
        }
    )
    candidate_activation.update({"schemaVersion": 1, "candidates": records})
    candidate_catalog = tuple(
        item
        for item in previous_catalog
        if not (
            candidate.status == "pending-replacement"
            and item.get("id") == candidate.definition_id
        )
    ) + (dict(candidate.catalog),)
    adopted = False
    try:
        if not persist_activation(candidate_activation):
            raise RuntimeError("activation preference publication failed")
        if not publish_catalog(candidate_catalog):
            raise RuntimeError("catalog publication failed")
        prepared = prepare_runtime(dict(candidate.catalog))
        if prepared is None:
            raise RuntimeError("Runtime preparation failed")
        if not adopt_runtime(prepared):
            raise RuntimeError("Runtime adoption failed")
        adopted = True
    except Exception as exc:
        rollback_ok = bool(publish_catalog(previous_catalog))
        rollback_ok = bool(persist_activation(previous_activation)) and rollback_ok
        return ActivationResult(
            False, rollback_ok, str(exc), previous_activation, previous_catalog
        )
    if not adopted:  # pragma: no cover - defensive
        return ActivationResult(
            False, True, "Runtime adoption did not complete",
            previous_activation, previous_catalog
        )
    return ActivationResult(
        True, True, "", candidate_activation, candidate_catalog
    )


def _snapshot_from_json(document: Mapping[str, Any]) -> DiscoverySnapshot:
    candidates: list[Candidate] = []
    for raw in document.get("candidates", []):
        identity = raw.get("identity", {})
        provenance = raw.get("provenance", {})
        previous = raw.get("previous", {})
        candidates.append(
            Candidate(
                candidate_id=raw["candidateId"],
                kind=raw["kind"],
                status=raw["status"],
                package_id=identity.get("packageId", ""),
                package_version=identity.get("packageVersion", ""),
                implementation_version=identity.get("implementationVersion", ""),
                type_id=identity.get("typeId", ""),
                definition_id=identity.get("definitionId", ""),
                registry_prefix=identity.get("registryPrefix", ""),
                template_id=identity.get("templateId", ""),
                content_id=identity.get("contentId", ""),
                content_version=identity.get("contentVersion", ""),
                signature=raw.get("signature", ""),
                relative_path=provenance.get("relativePath", ""),
                local_path=provenance.get("localPath", ""),
                root_ids=tuple(provenance.get("rootIds", [])),
                diagnostics=tuple(raw.get("diagnostics", [])),
                registered_type_available=raw.get("registeredTypeAvailable", False),
                metadata=raw.get("metadata", {}),
                catalog=raw.get("catalog", {}),
                previous_status=previous.get("status", ""),
                previous_signature=previous.get("signature", ""),
            )
        )
    return DiscoverySnapshot(
        int(document.get("generation", 0)),
        bool(document.get("enabled", False)),
        bool(document.get("stale", False)),
        tuple(candidates),
        tuple(document.get("diagnostics", [])),
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path)
    parser.add_argument("--prior", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    try:
        config = load_config(args.config)
    except DiscoveryConfigError as exc:
        print(f"controlled discovery config error: {exc}", file=sys.stderr)
        return 1
    prior = None
    if args.prior and args.prior.exists():
        prior = _snapshot_from_json(json.loads(args.prior.read_text(encoding="utf-8")))
    snapshot = refresh_atomically(config, prior=prior)
    text = json.dumps(snapshot.to_json(), indent=2, sort_keys=False) + "\n"
    if args.check:
        print(
            f"Controlled discovery passed ({len(snapshot.candidates)} candidates, "
            f"{sum(item.activatable for item in snapshot.candidates)} activatable)"
        )
        return 0 if not snapshot.stale else 1
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0 if not snapshot.stale else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

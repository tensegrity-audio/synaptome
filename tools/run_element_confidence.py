#!/usr/bin/env python3
"""Run the isolated SEAC-6 element confidence suite."""
from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

import element_package_v1
import generate_element_package_registrations
from element_confidence import (
    ConfidenceError,
    CheckResult,
    PROFILE_ROOT,
    REPO_ROOT,
    compare_reviewed_baseline,
    find_profile_for_package,
    load_json_object,
    load_profile,
    new_report,
    preflight_dependencies,
    profile_path,
    repo_path,
    run_contract_harness,
    run_graphics_harness,
    run_reload_harness,
    run_validator,
)

ARTIFACT_ROOT = REPO_ROOT / "artifacts" / "element-confidence"
TIER_NUMBERS = {
    "static": (0,),
    "contract": (0, 1),
    "graphics": (0, 1, 2),
    "reload": (0, 1, 2, 3),
    "ci": (0, 1, 2, 3),
}
PENDING_TIER_CHECKS = {
    1: "native-contract-and-lifecycle",
    2: "real-offscreen-graphics",
    3: "reload-and-performance",
}


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument(
        "--profile", help="Internal confidence profile name or JSON path"
    )
    selection.add_argument(
        "--package", type=Path, help="Element package manifest path"
    )
    parser.add_argument(
        "--tier", choices=tuple(TIER_NUMBERS), default="ci", help="Highest test tier"
    )
    parser.add_argument(
        "--report", type=Path, help="Override the generated JSON report path"
    )
    parser.add_argument(
        "--baseline",
        type=Path,
        help="Reviewed renderer-class pixel/performance baseline JSON",
    )
    parser.add_argument(
        "--list-profiles", action="store_true", help=argparse.SUPPRESS
    )
    return parser.parse_args(argv)


def report_path(profile_id: str, override: Path | None) -> Path:
    if override is not None:
        return override if override.is_absolute() else REPO_ROOT / override
    safe_id = re.sub(r"[^a-zA-Z0-9_.-]+", "-", profile_id)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    return ARTIFACT_ROOT / f"{safe_id}-{stamp}.json"


def write_report(path: Path, report: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = report.to_json()  # type: ignore[attr-defined]
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    package_path: Path | None = None
    package: dict[str, object] | None = None
    package_result: element_package_v1.ValidationResult | None = None
    try:
        if args.package is not None:
            package_path = (
                args.package
                if args.package.is_absolute()
                else (REPO_ROOT / args.package).resolve()
            )
            package = load_json_object(package_path, "package")
            package_result = element_package_v1.validate_package(package_path)
            selected_path, profile = find_profile_for_package(package)
            element = package.get("element")
            if not isinstance(element, dict):
                raise ConfidenceError("package.element must be an object")
            if element.get("id") != profile["elementType"]:
                raise ConfidenceError(
                    "package element.id does not match the selected confidence profile"
                )
        else:
            selected_path = profile_path(args.profile)
            profile = load_profile(selected_path)
    except ConfidenceError as exc:
        print(f"[element-confidence] configuration error: {exc}", file=sys.stderr)
        return 2

    report = new_report(profile, args.tier)
    print(
        f"[element-confidence] {profile['id']} ({profile['elementType']}) "
        f"tier={args.tier}"
    )
    print(f"[element-confidence] profile: {repo_path(selected_path)}")

    package_errors: list[str] = []
    if package_result is not None:
        package_errors = [
            diagnostic.render() for diagnostic in package_result.diagnostics
        ]
        document = package_result.document or {}
        asset = document.get("asset", {})
        element = document.get("element", {})
        report.package = {
            "schemaVersion": document.get("schemaVersion"),
            "packageVersion": document.get("packageVersion"),
            "implementationVersion": document.get("implementationVersion"),
            "identity": {
                "packageId": document.get("packageId"),
                "typeId": element.get("id") if isinstance(element, dict) else None,
                "definitionId": asset.get("id") if isinstance(asset, dict) else None,
                "registryPrefix": (
                    asset.get("registryPrefix")
                    if isinstance(asset, dict)
                    else None
                ),
            },
            "resolvedCompatibility": package_result.resolved_compatibility,
            "declaredDependencies": package_result.resolved_dependencies,
            "serializedDescriptorSignature": (
                element_package_v1.descriptor_signature(
                    package_result.normalized
                )
                if package_result.normalized is not None
                else None
            ),
            "runtimeDescriptorSignature": None,
            "parity": {"status": "not-run", "diagnostics": []},
            "migrationPath": package_result.migration_path,
            "inventory": package_result.inventory,
            "registration": {
                "status": "not-run",
                "registrationSet": (
                    "docs/contracts/"
                    "element_package_registration_set_v1.json"
                ),
                "generatedSource": (
                    "synaptome/src/runtime/"
                    "GeneratedElementPackageRegistrations.cpp"
                ),
                "generatedBuildTarget": (
                    "synaptome/build/"
                    "GeneratedElementPackages.targets"
                ),
                "record": None,
            },
        }
        try:
            registration_records = (
                generate_element_package_registrations.load_records()
            )
            registration_record = next(
                (
                    item
                    for item in registration_records
                    if item.package_id == document.get("packageId")
                ),
                None,
            )
            if registration_record is not None:
                report.package["registration"]["record"] = {
                    "packageId": registration_record.package_id,
                    "packageVersion": registration_record.package_version,
                    "implementationVersion": (
                        registration_record.implementation_version
                    ),
                    "typeId": registration_record.type_id,
                    "kind": registration_record.kind,
                    "bindingMode": registration_record.binding_mode,
                    "definitionId": registration_record.definition_id,
                    "registryPrefix": registration_record.registry_prefix,
                    "sourceRegistration": (
                        registration_record.registration_reference
                    ),
                    "descriptorSignature": (
                        registration_record.descriptor_signature
                    ),
                    "symbol": registration_record.symbol,
                }
        except generate_element_package_registrations.GenerationError:
            pass
        report.checks.append(
            CheckResult(
                check_id="element-package-v1-preflight",
                tier=0,
                status="fail" if package_errors else "pass",
                required=True,
                duration_ms=0.0,
                diagnostic=(
                    "; ".join(package_errors)
                    if package_errors
                    else "Element Package v1 preflight passed"
                ),
            )
        )

    started = __import__("time").perf_counter()
    dependencies, errors = preflight_dependencies(
        profile, package_path=package_path, package=package
    )
    errors = package_errors + errors
    report.resolved_dependencies = dependencies
    report.checks.append(
        CheckResult(
            check_id="dependency-preflight",
            tier=0,
            status="fail" if errors else "pass",
            required=True,
            duration_ms=(__import__("time").perf_counter() - started) * 1000.0,
            diagnostic="; ".join(errors)
            if errors
            else f"resolved {len(dependencies)} unique dependencies",
        )
    )

    if not errors:
        for validator in profile.get("validators", []):
            check = run_validator(validator, package_path)
            report.checks.append(check)
            print(
                f"[element-confidence] {check.status.upper()} {check.check_id}: "
                f"{check.diagnostic}"
            )
            if check.required and check.status != "pass":
                errors.append(
                    f"{check.check_id}: {check.diagnostic}"
                )
            if (
                report.package is not None
                and check.check_id == "generated-package-registration"
            ):
                report.package["registration"]["status"] = check.status

    selected_tiers = TIER_NUMBERS[args.tier]
    contract_ok = False
    if 1 in selected_tiers and not errors:
        contract_checks, evidence = run_contract_harness(
            profile, package_result
        )
        report.checks.extend(contract_checks)
        contract_ok = all(
            check.status == "pass"
            for check in contract_checks
            if check.required
        )
        for check in contract_checks:
            print(
                f"[element-confidence] {check.status.upper()} {check.check_id}: "
                f"{check.diagnostic}"
            )
        if report.package is not None:
            parity = next(
                (
                    check
                    for check in contract_checks
                    if check.check_id == "package-runtime-descriptor-parity"
                ),
                None,
            )
            if parity is not None:
                report.package["parity"] = {
                    "status": parity.status,
                    "diagnostics": (
                        [] if parity.status == "pass" else [parity.diagnostic]
                    ),
                }
            runtime_inventory = (
                package_result.inventory.get("runtimeDescriptor", [])
                if package_result is not None
                else []
            )
            if runtime_inventory:
                report.package["runtimeDescriptorSignature"] = (
                    runtime_inventory[0].get("signature")
                )
        if evidence is not None:
            report.declaration_live_comparison = evidence[
                "declarationLiveComparison"
            ]
            report.state_signatures = evidence["stateSignatures"]

    graphics_ok = False
    if 2 in selected_tiers and not errors:
        if contract_ok:
            graphics_checks, graphics_evidence = run_graphics_harness(
                profile
            )
            report.checks.extend(graphics_checks)
            graphics_ok = all(
                check.status == "pass"
                for check in graphics_checks
                if check.required
            )
            for check in graphics_checks:
                print(
                    f"[element-confidence] {check.status.upper()} {check.check_id}: "
                    f"{check.diagnostic}"
                )
            if graphics_evidence is not None:
                report.graphics = {
                    "renderer": graphics_evidence["renderer"],
                    "vendor": graphics_evidence["vendor"],
                    "version": graphics_evidence["version"],
                    "nonblank": graphics_evidence["nonblank"],
                    "pixelSignature": graphics_evidence[
                        "pixelSignature"
                    ],
                    "leakedState": graphics_evidence["leakedState"],
                }
        else:
            report.checks.append(
                CheckResult(
                    check_id=PENDING_TIER_CHECKS[2],
                    tier=2,
                    status="skip",
                    required=True,
                    duration_ms=0.0,
                    diagnostic="Tier 1 contract did not pass",
                )
            )

    if 3 in selected_tiers and not errors:
        if graphics_ok:
            reload_checks, reload_evidence = run_reload_harness(profile)
            report.checks.extend(reload_checks)
            for check in reload_checks:
                print(
                    f"[element-confidence] {check.status.upper()} {check.check_id}: "
                    f"{check.diagnostic}"
                )
            if reload_evidence is not None:
                native_reload = reload_evidence["reload"]
                report.reload = {
                    "count": native_reload["count"],
                    "warmWorkingSetBytes": native_reload[
                        "warmWorkingSetBytes"
                    ],
                    "finalWorkingSetBytes": native_reload[
                        "finalWorkingSetBytes"
                    ],
                    "growthSlopeBytesPerReload": native_reload[
                        "growthSlopeBytesPerReload"
                    ],
                }
                report.timings = reload_evidence["timings"]
        else:
            report.checks.append(
                CheckResult(
                    check_id=PENDING_TIER_CHECKS[3],
                    tier=3,
                    status="skip",
                    required=True,
                    duration_ms=0.0,
                    diagnostic="Tier 2 graphics did not pass",
                )
            )

    if errors:
        for tier_number in selected_tiers:
            if tier_number == 0:
                continue
            report.checks.append(
                CheckResult(
                    check_id=PENDING_TIER_CHECKS[tier_number],
                    tier=tier_number,
                    status="skip",
                    required=True,
                    duration_ms=0.0,
                    diagnostic="dependency preflight failed",
                )
            )

    if args.baseline is not None and 2 in selected_tiers:
        baseline_path = (
            args.baseline
            if args.baseline.is_absolute()
            else (REPO_ROOT / args.baseline).resolve()
        )
        report.checks = [
            check
            for check in report.checks
            if check.check_id
            not in {
                "pixel-baseline-comparison",
                "performance-baseline-comparison",
            }
        ]
        try:
            baseline_checks = compare_reviewed_baseline(
                report, baseline_path
            )
            report.checks.extend(baseline_checks)
            for check in baseline_checks:
                print(
                    f"[element-confidence] {check.status.upper()} {check.check_id}: "
                    f"{check.diagnostic}"
                )
        except ConfidenceError as exc:
            report.checks.append(
                CheckResult(
                    check_id="reviewed-baseline-configuration",
                    tier=2,
                    status="fail",
                    required=True,
                    duration_ms=0.0,
                    diagnostic=str(exc),
                )
            )

    destination = report_path(profile["id"], args.report)
    if report.package is not None and package_result is not None:
        report.package["inventory"] = package_result.inventory
    write_report(destination, report)
    passed = sum(check.status == "pass" for check in report.checks)
    failed = sum(check.status == "fail" for check in report.checks)
    skipped = sum(check.status == "skip" for check in report.checks)
    print(
        f"[element-confidence] {report.overall_result.upper()} "
        f"pass={passed} fail={failed} skip={skipped}"
    )
    print(f"[element-confidence] report: {repo_path(destination)}")
    return 0 if report.overall_result == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

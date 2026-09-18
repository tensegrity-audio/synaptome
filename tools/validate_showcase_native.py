#!/usr/bin/env python3
"""Compile actual package adapters with native contracts and CPU draw recording.

This portable gate does not create an OpenGL context, measure renderer speed,
or replace Windows Release and real-host acceptance. Builds are incremental;
compiler-generated dependency files include every package and SDK header.
"""
from __future__ import annotations
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def build(compiler: str, sources: list[Path], binary: Path, flags: list[str], jobs: int) -> int:
    cache = binary.parent / "objects"
    cache.mkdir(parents=True, exist_ok=True)

    def compile_one(source: Path) -> tuple[int, Path]:
        key = hashlib.sha256(json.dumps([compiler, str(source), flags]).encode()).hexdigest()[:20]
        obj, dep = cache / f"{key}.o", cache / f"{key}.d"
        if obj.exists() and dep.exists():
            dependencies = shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1])
            if all(Path(p).exists() and Path(p).stat().st_mtime_ns <= obj.stat().st_mtime_ns for p in dependencies):
                return 0, obj
        command = [compiler, *flags, "-MMD", "-MF", str(dep), "-c", str(source), "-o", str(obj)]
        result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.stdout:
            print(result.stdout, end="", flush=True)
        return result.returncode, obj

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(compile_one, sources))
    if any(code for code, _ in results):
        return 1
    link_flags = [flag for flag in flags if flag.startswith("-fsanitize=")]
    return subprocess.run([compiler, *link_flags, *[str(obj) for _, obj in results], "-o", str(binary)], cwd=ROOT).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path(tempfile.gettempdir()) / "synaptome-showcase-validation")
    parser.add_argument("--package", action="append", help="Showcase folder name, repeatable; default all")
    parser.add_argument("--legacy-bench", action="store_true", help="Also compile and run existing LayerPackageBench with original repository stubs")
    parser.add_argument("--sanitize", action="store_true", help="Enable AddressSanitizer and UndefinedBehaviorSanitizer")
    parser.add_argument("--jobs", type=int, default=4, help="Parallel compiler processes (default 4)")
    args = parser.parse_args()
    compiler = shutil.which("g++") or shutil.which("clang++")
    if not compiler:
        parser.error("g++ or clang++ is required for this portable native gate")
    registration = json.loads((ROOT / "docs/contracts/element_package_registration_set_v1.json").read_text())
    manifests = [ROOT / name for name in registration["packages"]]
    showcase = [p for p in manifests if "packages/showcase/" in p.as_posix()]
    if args.package:
        showcase = [p for p in showcase if p.parent.name in args.package]
    if not showcase:
        parser.error("No registered showcase package matched; generate package registrations first")
    sources = [ROOT / "synaptome/src/visuals/LayerFactory.cpp", ROOT / "synaptome/src/runtime/GeneratedElementPackageRegistrations.cpp"]
    for manifest in manifests:
        source = json.loads(manifest.read_text())["source"]
        sources += [manifest.parent / source["registration"]]
        sources += [manifest.parent / name for name in source["files"] if Path(name).suffix == ".cpp"]
    sources = list(dict.fromkeys(sources))
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    flags = ["-std=c++17", "-O1" if args.sanitize else "-O2", "-Wall", "-Wextra", "-Wpedantic"]
    if args.sanitize:
        flags += ["-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    model_binary = args.output / "showcase_models"
    print("Compiling and running 12 pure algorithm contracts", flush=True)
    code = build(compiler, [ROOT / "tests/showcase_models_main.cpp"], model_binary, flags, args.jobs)
    if code:
        return code
    code = subprocess.run([str(model_binary)], cwd=ROOT).returncode
    if code:
        return code
    includes = [ROOT / "tests/stubs", ROOT / "tests", ROOT / "synaptome/sdk/include", ROOT / "synaptome/src", ROOT / "thirdparty"]
    include_flags = [item for include in includes for item in ("-I", str(include))]
    binary = args.output / "showcase_native"
    print(f"Compiling {len(sources) + 1} native translation units", flush=True)
    code = build(compiler, [ROOT / "tests/showcase_native_main.cpp", *sources], binary, flags + ["-I", str(ROOT / "tests/showcase_support")] + include_flags, args.jobs)
    if code:
        return code
    code = subprocess.run([str(binary), str(args.output / "geometry"), *map(str, showcase)], cwd=ROOT).returncode
    if code or not args.legacy_bench:
        return code
    legacy = args.output / "layer_package_bench"
    print("Compiling existing LayerPackageBench against original repository stubs", flush=True)
    code = build(compiler, [ROOT / "tests/layer_package_bench_main.cpp", *sources], legacy, flags + include_flags, args.jobs)
    if code:
        return code
    return subprocess.run([str(legacy)], cwd=ROOT).returncode


if __name__ == "__main__":
    raise SystemExit(main())

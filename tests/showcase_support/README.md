# Portable showcase checks

Run from the repository root:

```sh
python tools/validate_showcase_native.py --legacy-bench
```

The runner compiles the 12 header-only algorithms with an ordinary C++17
compiler, then compiles the actual package adapters, creator leaves, generated
registration, LayerFactory and ElementParameterTable. It never substitutes a
fake parameter registry or rewrites an adapter. Include precedence supplies
this test-only graphics recorder; the existing `--legacy-bench` path separately
uses the original repository stubs and runs the Signal Bloom regression.

Coverage includes:

- Seeded deterministic replay and different seeds producing different state.
- Zero, negative and non-finite model time steps, plus bounded long stalls.
- Public declaration/binding parity using the real ElementParameterTable.
- Declaration defaults, explicit configuration and public registry edits.
- Visible first-frame geometry, deterministic draws and control range endpoints.
- Transport pause, local pause, explicit reseed, one-shot trigger clearing and
  prevention of live seed drift from the persisted registry base.
- Finite CPU vertices/colors, valid mesh indices and normalized alpha.
- Slot opacity, zero-opacity invisibility and alternate viewport dimensions.
- Preservation of caller style, matrix, viewport, depth and cull settings.
- 3,600 model frames and 2,000 additional adapter frames per element.
- Conservative sand transport, checked independently by total material mass.

The runner exports SVGs from actual adapter draw calls under the output
folder's `geometry` directory. These are geometry review aids. Triangle colors
are averaged in SVG export; OpenGL interpolates the vertex colors. These files
are not screenshots of Synaptome.

`--sanitize` enables AddressSanitizer and UndefinedBehaviorSanitizer. In
containers running under ptrace, LeakSanitizer cannot operate. If that precise
environment limitation occurs, rerun with `ASAN_OPTIONS=detect_leaks=0` and
record leak checking as unverified. Do not claim a memory-leak gate passed.

This suite does not validate native Windows linkage, OpenGL pixel output,
actual graphics driver state, live device routing, frame timing, GPU memory,
or real-host Scene persistence. Those remain the existing Windows Release,
confidence-profile and performance-machine acceptance gates. Recorded CPU
geometry and synthetic registry writes are not substitutes for those checks.

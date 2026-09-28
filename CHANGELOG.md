# Kinetra Changelog

## [0.0.2rc1] - 2026-09-28

- Added SPEC §15: memory model and garbage collection semantics.
- Added consolidated bytecode divergence list (SPEC § 12).
- Added KINETRA_GC_THRESHOLD environment knob for benchmarking.
- Added full comparison to tools/bench.ps1 (backends + GC overhead).
- Added v0.0.2 measurement section and acceptance criteria to docs/BENCH.md.
- Added make uninstall target.
- Added `make SANITIZE=...` sanitizer builds and a CI asan job.
- Documented Valigrid zero-leak validation procedure.
- Added bytecode function support: declarations, calls, recursion,
`OP_CALL/OP_RETURN`, and a bytecode call-frame stack.
- Added top-level function pre-scan for mutual recursion.
- Added frame-aware name resolution and const enforcement in the
bytecode VM.
- Added bytecode GC roots: chunk constants, bytecode globals, operand stack.
- Added collection safe point at OP_LOOP back-edges.
- Added bytecode string support: literals, concatenation, equality,
OP_INDEX_STR, OP_SLICE_STR, OP_LEN, string printing.
- Added `split(s, sep)` returning a GC-allocated array of strings.
- Added `join(arr, sep)` with string-element validation.
- Added `find(s, sub)` returning the first index or -1.
- Added `to_number(s)` with strict whole-string parsing.
- Added string indexing: `s[i]` returns a 1-char string.
- Added string slicing: `s[a:b]`, `s[:b]`, `[s[a:]`, `s[:]`.
- Added bounds-checked slice semantics with copied, GC-allocated results.
- Added `TOKEN_COLON` and `NODE_SLICE`.
- Added tests 24 (stdlib) and 25 (parse failure path).
- Added VM root scanner: globals, call frames, and return slot are marked.
- Added automatic collection at safe points (top-level statements,
while iterations, sim iterations).
- Added `gc_set_enabled()`, `gc_try_collect()`, `gc_mark_value()`,
`gc_set_root_scanner()`, and stats getters.
- Added `--gc` flag reporting bytes allocated and collection count.
- Added `tests/21_gc_stress.knt` (100k heap allocations per run).

### Changed

- FEATURE FREEZE for the 0.0.2 line (docs/SPEC.md §14).
- Bytecode function declarations are hoisted (documented divergence).
- Bytecode operand stack moved to file scope for root scanning.
- docs/SPEC.md §12 parity table updated.
- `make_string()` refactored onto length-aware `make_string_len()`.
- GC is suspended inside `parallel for` regions (no concurrent collection).

### Known Limitations

- Bytecode blocks are scope-flat (function frames only).
- Bytecode VM still lacks arrays, mat4/vec3/particle, and user functions.
- New built-ins remain tree-walk only (codegen error under `--bc`).
- Slicing arrays is not supported yet (runtime error).
- Bytecode VM rejects indexing/slicing (codegen error).
- Collection is deferred to loop/statement boundaries; garbage produced
inside a single deeply-nested expression is reclaimed at the next safe
point, not immediately.
- Bytecode VM roots are not wired yet (scheduled for v0.0.2a05).

### Release gating

- v0.0.2 ships when: suite green on all CI jobs (incl. asan), bench
criteria met, Valgrind zero-loss confirmed, packaging round-trip ok.

## [0.0.1] - 2026-09-24

### Released

- First stable public releasee of Kinetra.
- Complete language: types, control flow, functions, scoping, const,
parallel for, sim blocks.
- Two execution backends: tree-walk VM (default) and prototype bytecode
VM (`--bc`).
- Full diagnostic system with source snippets and caret positioning.
- 20-case regression test suite with 100% pass rate.
- OpenMP parallel backend for `parallel for` loops.
- REPL with error recovery.
- Developer tools: `--tokens`, `--ast`, `--bench`, `--folds`, `--repl`.
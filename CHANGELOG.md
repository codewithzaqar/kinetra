# Kinetra Changelog

## [0.0.2a04] - 2026-09-27

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

- `make_string()` refactored onto length-aware `make_string_len()`.
- GC is suspended inside `parallel for` regions (no concurrent collection).

### Known Limitations

- New built-ins remain tree-walk only (codegen error under `--bc`).
- Slicing arrays is not supported yet (runtime error).
- Bytecode VM rejects indexing/slicing (codegen error).
- Collection is deferred to loop/statement boundaries; garbage produced
inside a single deeply-nested expression is reclaimed at the next safe
point, not immediately.
- Bytecode VM roots are not wired yet (scheduled for v0.0.2a05).

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
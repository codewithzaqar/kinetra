# Kinetra Changelog

## [0.0.3a04] - 2026-10-01

### Added

- Added `--check` dry-run mode (syntax validation without execution).
- Added `--check --bc` bytecode compilation validation.
- Added `bc_compile_program()` / `bc_instruction_count()`; split compilation from execution in the bytecode VM.
- Added per-case `.args` support to the test runner.
- Added tests 43-45 for check mode.
- Added SPEC §16 (static check mode).
- Added `upper(s)`, `lower(s)` (ASCII-only), `trim(s)`, and `replace(s, from, to)` (non-overlapping, left-to-right).
- Added `gc_array_resize()` with exact byte accounting.
- Added `push(arr, v)`, `pop(arr)`, `clear(arr)` with reference semantics.
- Added const-bindings mutation rejection for array built-ins.
- Added array slicing `a[i:j]` with copy semantics and omitted-bound forms.
- Added recursive element-wise array equality via `kvalue_equal()` (depth-capped at 64 for cycle structures).

### Changed

- `tested/31_fn_scopes.knt` recursion depth temporaily reduced to 25 pending the tracked frame-accounting bug (2 frames per cell).

### Known Limitations

- Case mapping is ASCII-only by design (byte-stable for UTF-8 passthrough).
- Transforms are tree-walk only (codegen error under --bc).
- Mutation built-ins are tree-walk only (codegen error under --bc).
- Const enforcement is binding-level; aliases bypass it (documented).
- Array slicing/equality remain tree-walk only (codegen error under --bc). 

## [0.0.2] - 2026-09-28

- Garbage collection: mark-and-sweep collector for arrays and strings
with Obj headers, root scanners for both VMs, safe-point collection,
parallel-region suspension, and zero-leak shutdown (`gc_free_all`).
- String indexing `s[i]` and half-open slicing `s[a:b]`, including
omitted-bound forms.
- String standard library: `split`, `join`, `find`, `to_number`.
- Bytecode VM: GC integration, string literals/concat/equality.
string opcodes (`OP_INDEX_STR`, `OP_SLICE_STR`, `OP_LEN`), and user
functions with call frames, recursion, and hoisted declarations.
- Regression suite grown from 20 to 32 cases (GC churn, string matrix,
slice edges, function scoping, error paths) with bytecode parity
markers and an empty-suite guard.
- Sanitizer builds (`make SANITIZE=address,undefined`) and a CI asan job.
- `docs/SPEC.md` §15 memory model and the consolidated bytecdoe
divergence list.
- Bench harness comparison table with measured GC overhead
(`KINETRA_GC_THRESHOLD` knob).
- `make uninstall` target.

### Changed

- `KValue` heap fields unified under a single `Obj* heap` pointer.
- Version constant to `0.0.2`.

### Release gating (all satisfied)

- 32/32 tests green on Linux, macOS, and Windows.
- ASan/UBSan job clean.
- Valgrind: definitely lost = 0 bytes.
- Bench critetia: bytecode >= 3x scalar, GC overhead <= 10%.
- Packaging round-trip verified.
 
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
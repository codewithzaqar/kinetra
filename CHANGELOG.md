# Kinetra Changelog

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
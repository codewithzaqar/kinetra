# Kinetra Changelog

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
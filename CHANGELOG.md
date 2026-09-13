# Kinetra Changelog

## [0.0.1a03] - 2026-09-13

### Added

- Added `print` keyword support.
- Added `TOKEN_PRINT`.
- Added `NODE_PRINT` AST node.
- Added support for single-statement programs using `print expression;`.
- Added `//` line-comment support to the lexer.
- Added `CHANGELOG.md` for version history tracking.
- Added repository ignore rules through `.gitignore`.
- Initial Kinetra bootstrap compiler/runtime structure.
- Lexer, parser, VM, and HPC math module.
- Makefile-based build system.
- Example `test.knt` file.

### Changed

- Parser now parses a statement instead of only a raw expression.
- VM now executes `print` statements directly.
- Improved repository structure and release hygiene.
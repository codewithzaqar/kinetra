# Kinetra Changelog

## [0.0.1a04] - 2026-09-14

### Added

- Added `let` variable declaratios.
- Added variable assignment using `=`.
- Added variable lookup in expressions.
- Added support for multiple statements.
- Added `print` keyword support.
- Added `TOKEN_PRINT`.
- Added `TOKEN_LET`.
- Added `TOKEN_ASSIGN`.
- Added `NODE_ASSIGN`.
- Added `NODE_PRINT` AST node.
- Added a simple VM symbol table for numeric variables.
- Added support for single-statement programs using `print expression;`.
- Added `//` line-comment support to the lexer.
- Added `CHANGELOG.md` for version history tracking.
- Added repository ignore rules through `.gitignore`.
- Initial Kinetra bootstrap compiler/runtime structure.
- Lexer, parser, VM, and HPC math module.
- Makefile-based build system.
- Example `test.knt` file.

### Changed

- Program AST now stores a list of statements.
- Parser now parses a statement instead of only a raw expression.
- VM now executes `print` statements directly.
- Improved repository structure and release hygiene.
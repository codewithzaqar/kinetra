# Kinetra Changelog

## [0.0.1a05] - 2026-09-15

### Added

- Added `sim` blocks.
- Added brace block syntax using `{` and `}`.
- Added `step` statement for declaring simulation iteration count.
- Added built-in `step_index` variable inside simulation loops.
- Added `let` variable declaratios.
- Added variable assignment using `=`.
- Added variable lookup in expressions.
- Added support for multiple statements.
- Added `print` keyword support.
- Added `TOKEN_PRINT`.
- Added `TOKEN_LET`.
- Added `TOKEN_ASSIGN`.
- Added `TOKEN_SIM`.
- Added `TOKEN_STEP`.
- Added `TOKEN_LBRACE`.
- Added `TOKEN_RBRACE`.
- Added `NODE_ASSIGN`.
- Added `NODE_PRINT` AST node.
- Added `NODE_STEP`.
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

- `NODE_SIMULATION_BLOCK` is now used by the parser and VM.
- Program AST now stores a list of statements.
- Parser now parses a statement instead of only a raw expression.
- VM now executes `print` statements directly.
- Improved repository structure and release hygiene.
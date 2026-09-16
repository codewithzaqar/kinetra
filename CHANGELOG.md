# Kinetra Changelog

## [0.0.1a06] - 2026-09-16

### Added

- Added a basic value model supporting numbers and vector values.
- Added `vec3(x, y, z)` constructor syntax.
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
- Added `TOKEN_VEC3`.
- Added `TOKEN_COMMA`.
- Added `NODE_ASSIGN`.
- Added `NODE_PRINT` AST node.
- Added `NODE_STEP`.
- Added `NODE_VEC3`.
- Added vector addition.
- Added vector subtraction.
- Added scalar-vector multiplication.
- Added component-wise vector multiplication.
- Added scalar-vector division.
- Added automatic `dt` variable inside `sim` blocks.
- Added `vec3` printing support.
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

- VM variables now store `KValue` instead of plain `double`.
- VM runtime errors are stricker for invalid operand types.
- `NODE_SIMULATION_BLOCK` is now used by the parser and VM.
- Program AST now stores a list of statements.
- Parser now parses a statement instead of only a raw expression.
- VM now executes `print` statements directly.
- Improved repository structure and release hygiene.
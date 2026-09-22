# Kinetra Changelog

## [0.0.1a19] - 2026-09-22

### Added

- Added `--tokens` flag: full token stream dump with positions.
- Added `--ast` flag: indented AST dump with node types and positions.
- Added `--repl` flag: interactive REPL with persistent state.
- Added multi-line REPL input via bracket balance detection.
- Added automatic expression echoing in the REPL.
- Added diagnostics recovery mode (`setjmp`/`longjmp`) so REPL
sessions survive lex/parse/runtime errors.
- Added `token_type_name()`, `ast_node_name()`, `ast_dump()`.
- Added `vm_eval_and_print()` and `vm_reset_flow()`.
- Added prototype bytecode backend: `include/bytecode.h`, `src/bytecode.c`.
- Added 22 opcodes including short-circuit jump variants.
- Added `--bc` flag to execute via the bytecode VM.
- Added compile-time jump pathcing for if/else, while, break, continue.
- Added `(codegen)` diagnostics stage with exit code 5.
- Added `--bench` support for both VMs for head-to-head timing.
- Added compile-time constant folding for arithmetic, comparison,
logical, and unary `!` operations on literals.
- Added `--folds` flag reporting the parser fold count.
- Added `--bench` flag: 100 silent interations with total/avg ms timing.
- Added `vm_reset()` to clear VM state between benchmark iterations.
- Added `vm_set_quiet()` to suppress print output during benchmarks.
- Added `parser_fold_count()` statistics accessor.
- Added unified diagnostics module: `include/diagnostics.h`, `src/diagnostics.c`.
- Added column tracking to tokens (`Token.column`).
- Added source snippet rendering with caret on lex and parse errors.
- Added snippet reading (line only) for runtime errors.
- Added process exit codes: 0 ok, 1 lex, 2 parse, 3 runtime, 4 I/O, 64 usage.
- Added `pow(x, y)` with NaN result checking.
- Added `exp(x)`.
- Added `log(x)` with non-positive domain checking.
- Added `floor(x)`, `ceil(x)`, `round(x)`.
- Added `clamp(x, lo, hi)` with `lo <= hi` validation.
- Added `lerp(a, b, t)` for numbers and vec3 values.
- Added `reflect(v, n)` vector reflection.
- Added `particle` value type (positon, velocity, force, mass).
- Added `particle(pos, vel)` and `particle(pos, vel, mass)` constructors.
- Added member access operator `.` for vec3 (`.x .y .z`) and particles
 (`.position .velocity .force .mass`).
- Added `apply_force(p, f)` built-in.
- Added `clear_force(p)` built-in.
- Added `integrate(p, dt)` built-in using semi-implicit Euler integration.
- Added array value type with reference semantics.
- Added array literals: `[...]` and empty `[]`.
- Added nested array support.
- Added index read syntax: `a[i]`, including chained indexing.
- Added index write syntax: `a[i] = value;`.
- Added `len()` built-in.
- Added bounds-checked, whole-numer index validation.
- Added recursive value printing for arrays.
- Added `mat4` value type (row-major 4x4).
- Added `mat4()` identity constructor.
- Added `mat4(m()..m15)` explicit constructor.
- Added `mat4 * mat4` matrix multiplication.
- Added `mat4 * vec3` point-transformation sugar.
- Added `translate(x, y, z)` built-in.
- Added `scale(x, y, z)` built-in.
- Added `rotate(axis, angle)` built-in using Rodrigues rotation.
- Added `transform(mat4, vec3)` built-in with homogeneous divide.
- Added `fn name(params) {...}` function declarations.
- Added `return expression;` and bare `return;`.
- Added user-defined function calls with argument lists.
- Added a runtime call stack with local frames.
- Added local scoping and shadowing rules.
- Added recursion support.
- Added `while condition {...}` loops.
- Added `break;` statement.
- Added `continue;` statement.
- Added boolean literals `true` and `false`.
- Added boolean value type to the VM.
- Added comparison operators: `<`, `>`, `<=`, `>=`, `==`, `!=`.
- Added logical operators: `&&`, `||`, `!`.
- Added short-circuit evaluation for `&&` and `||`.
- Added `if / else if / else` control flow with brace blocks.
- Added numeric built-ins: `sqrt()`, `abs()`, `min()`, `max()`.
- Added trig built-ins: `sin()`, `cos()`, `tan()`.
- Added built-in vector math functions: `dot()`, `cross()`, `length()`, `normalize()`.
- Added a basic value model supporting numbers and vector values.
- Added custom simulation time-step syntax: `sim <count> dt <value> {...}`.
- Added `==` / `!=` support for `vec3` and boolean values.
- Added `vec3(x, y, z)` constructor syntax.
- Added `vec3_cross()` to the HPC math library.
- Added `vec3_length()` to the HPC math library.
- Added `vec3_length()` to the HPC math library.
- Added `vec3_normalize()` to the HPC math library.
- Added parse-time arity checking for built-in functions.
- Added runtime type checking for built-in function arguments.
- Added `dt` as a readable built-in variable inside expressions.
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
- Added `TOKEN_BUILTIN`.
- Added `TOKEN_DT`.
- Added `TOKEN_IF`.
- Added `TOKEN_ELSE`.
- Added `TOKEN_TRUE`.
- Added `TOKEN_FALSE`.
- Added `TOKEN_LT`.
- Added `TOKEN_GT`.
- Added `TOKEN_LE`.
- Added `TOKEN_GE`.
- Added `TOKEN_EQ`.
- Added `TOKEN_NE`.
- Added `TOKEN_AND`.
- Added `TOKEN_OR`.
- Added `TOKEN_NOT`.
- Added `TOKEN_WHILE`.
- Added `TOKEN_BREAK`.
- Added `TOKEN_CONTINUE`.
- Added `TOKEN_FN`.
- Added `TOKEN_RETURN`.
- Added `TOKEN_MAT4`.
- Added `TOKEN_LBRACKET`.
- Added `TOKEN_RBRACKET`.
- Added `TOKEN_PARTICLE`.
- Added `TOKEN_DOT`.
- Added `NODE_ASSIGN`.
- Added `NODE_PRINT` AST node.
- Added `NODE_STEP`.
- Added `NODE_VEC3`.
- Added `NODE_CALL`.
- Added `NODE_IF`.
- Added `NODE_BLOCK`.
- Added `NODE_UNARY_OP`.
- Added `NODE_BOOLEAN_LITERAL`.
- Added `NODE_WHILE`.
- Added `NODE_BREAK`.
- Added `NODE_CONTINUE`.
- Added `NODE_FUNCTION`.
- Added `NODE_RETURN`.
- Added `NODE_MAT4`.
- Added `NODE_ARRAY_LITERAL`.
- Added `NODE_INDEX`.
- Added `NODE_INDEX_ASSIGN`.
- Added `NODE_PARTICLE`.
- Added `NODE_MEMBER`.
- Added HPC library functions: `mat4_identity`, `mat4_translate`. `mat4_scale`, `mat4_rotate`, `mat4_mul`, `mat4_mul`, `mat4_transform_point`.
- Added VM flow-signal mechanism for loop control transfer.
- Added `break` / `continue` support inside `sim` blocks.
- Added runtime error for `break` / `continue` outside of any loop.
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

- CLI now supports `--tokens`, `--ast`, `--repl` alongside existing flags.
- `Makefile` now compiles `src/bytecode.c`.
- Division by zero is never folded; the runtime error is preserved.
- CLI argument parsing now supports flags in any position.
- Lex errors are now fatal and reported through the diagnostics module.
- Parser errors report through `diag_error()` with line:column.
- Runtime errors report through `diag_error()` with line.
- `Makefile` now compiles `src/diagnostics.c`.
- Numeric standard library is now complete for the v0.0.1 scope.
- `integrate` is no longer a reserved sim keyword; it is now a built-in function.
- `KValue` is now self-referential tagged struct with array storage.
- `print_value()` now delegates to a recursive inner printer.
- `mat4` is now a reserved word with its own token type.
- Variable lookup is now scope-chain based: innermost frame first, then globals.
- `let` defines in the current scope; assignment targets the nearest existing scope.
- `while` and `sim` loops now propagate `return` signals.
- `fn` and `return` are now reserved words.
- `{}` blocks now propagate flow signals to the enclosing loop.
- `sim` body execution is now flow-signal aware.
- `while`, `break`, `continue` are now reserved words.
- Expression parser now uses a full precedence chain:
	`||` -> `&&` -> comparison -> additive -> term -> unary -> primary.
- `if`, `else`, `true`, `false` are now reserved words.
- `sim` now stores an optional custom `dt` expression in `node->right`.
- Automatic `dt = 1 / steps` only applies when no custom `dt` is given.
- `dt` is now a reserved word and cannot be used as a variable name.
- Built-in calls now dispatch through the HPC math library.
- `free_ast()` now frees call arguments lists.
- `dot`, `cross`, `length`, and `normalize` are now reserved words.
- VM variables now store `KValue` instead of plain `double`.
- VM runtime errors are stricker for invalid operand types.
- `NODE_SIMULATION_BLOCK` is now used by the parser and VM.
- Program AST now stores a list of statements.
- Parser now parses a statement instead of only a raw expression.
- VM now executes `print` statements directly.
- Improved repository structure and release hygiene.

### Roadmap Adjustment

- HPC groundwork moved to `v0.0.1a20`; hardening to `v0.0.1a21`.

### Known Limitations

- A REPL submission that errors leaks its AST/tokens (no cleanup path
through longjmp); acceptable for the alpha, planned for hardening.
- REPL echo re-evaluates the last expression (side-effecting calls run
twice when echoed).
- Bytecode subset covers the scalar core only: numbers, booleans,
variables, arithmetic, comparisons, logical ops, print, if/else,
while/break/continue.
- Functions, vec3/mat4/particle/array, built-ins, and sim blocks
reamin tree-walk only and raise codegen errors under `--bc`.
- Folding covers number/boolean literals only (no vec3/mat4 literals yet).
- Bench iterations re-allocate arrays each run (no GC); memory grows
with iteration count on array-heavy programs.
- Runtime errors do not yet carry columns (tokens have them; call-site
migration is planned).
- Caret alignment assumes spaces; tabs may misalign the caret. 
- Member assignment (`p.position = ...`) is not supported yet.
- Arrays are shared by reference; there is no copy operator yet.
- No garbage collection; array memory lives until process exit.
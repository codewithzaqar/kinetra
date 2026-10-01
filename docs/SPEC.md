# Kinetra Language Specification (v0.0.1)

## 1. Overview

Kinetra is a statically-lexed, dynamically-typed scripting language for
simulations, mathematics, and high-performance computing. Programs are
executed bt a tree-walk VM (default) or a prototype bytecode VM (`--bc`).

## 2. Lexical structure

- Comments: `//` to end of line.
- Identifiers: `[A-Za-z_][A-Za-z0-9_]*`.
- Numbers: decimal floating-point literals (`1`, `2.5`, `.5`).
- Reserved words: `let const print if else true false while break continue
fn return sim step dt parallel for in vec3 mat4 particle`.
- Built-in function names are reserved: see ??8.

## 3. Types

| Type | Literal / constructor | Notes |
|---|---|---|
| number | `1.0` | EEE-754 double |
| boolean | `true` / `false` | |
| vec3 | `vec3(x, y, z)` | |
| mat4 | `mat4()` / `mat4(16 numbers)` | row-major |
| particle | `particle(pos, vel[, mass])` | |
| array | `[a, b, c]` | reference semantics, heterogeneous |
| string | `"text"` | immutable, reference semantics, 255-char literal limit |

## 4. Operator precedence (low -> high)


||
&&
== != < > <= >=
+ -
* /
! (unary)
Postfic: `.member`, `[i]` indexing, `[a:b]` slicing, calls `f(...)`.
Array slicing: `a[i:j]` yields a new array containing a shallow copy of elements `[i, j]`. Omitted bounds default to `0` and `len`. Bounds must be whole numbers in `0..len`; `start > end` is a runtime error.

## 5. Scoping rules

- Three scope kinds: global, function frame, block frame.
- `{}` blocks push a frame; declarations inside die at `}`.
- `while` bodies and `if` branches scope per execution; `sim` bodies
scope per iteration; `parallel for` lanes scope per iteration.
- Lookup walks innermost -> outermost, then globals.
- Assignment targets the nearest **existing** binding; if none exists it
defines in the current scope.
- `const` bindings reject assignment, element mutation, and `let`
redeclaration in the same scope.

## 6. Statements

`let`, `const`, assignment, index assignment, `print`, expression,
`if/else`, `while`, `break`, `continue`, `fn`, `return`.
`sim [count] [dt value] {}`, `step n;`, `parallel for v in n {}`.

## 7. Functions

`fn name(params) {...}`. Registered when the declaration executes.
Calls push a frame; parameters are local. Recursion supported.
`return` yields a value (bare `return` yields `0`).

## 8. Built-ins

Vector: `dot cross length normalize reflect`
Numeric: `sqrt abs min max pow exp log floor ceil round clamp lerp`
Trig: `sin cos tan`
Matrix: `translate rotate scale transform`
Array: `len` accepts arrays and strings. Strings support `+` (concatentation)
and `==`/`!=` (byte-wise comparison). `print` emits string contents
raw, without quotes.
Particle: `integrate apply_force clear_force`
String access: `s[i]` yields a 1-char string; `s[a:b]` yields the
half-open substring `[a, b]`. Bounds must be whole numbers in
`0.len`; omitted bounds default to `0` and `len`. Results are copies.
String: `split(s, sep) -> array`, `join(arr, sep) -> string`,
`find(s, sub) -> number` (-1 when absent), `to_number(s) -> number`
(whole-string parse; otherwise runtime error).
Array equality: `==` compares arrays recutsively, element-wise (numbers, booleans, strings, vec3/mat4/particle by value; nested arrays recursed). Comparison depth is capped at 64; deeper nesting compares by identity, which terminates cyclic structures deterministically.
Array mutatuin (reference semantics): `push(arr, v) -> number` (new length),
`pop(arr) -> value` (runtime error when empty), `clear(arr) -> 0`.
Mutation through a const-bound name is a runtime error; const-ness is binding-level,
so aliases (`let b = a;`) still mutate the shared array --- same rule as index assignment.
Transform: `upper(s)`, `lower(s)` (ASCII-only mapping; other bytes unchanged), `trim(s)` (strips space/tab/LF/CR from both ends), `replace(s, from, to_` (left-to-right, non-overlapping matches; empty `from` is a runtime error). All return new GC strings.

## 9. Diagnostics and exit codes

Stages: `lex` (1), `parse` (2), `runtime` (3), `io` (4), `codegen` (5),
`usgae` (64). Success is 0. Errors render a source snippet with caret
when column information is available.

## 10. Parallelism

`parallel for v in n {...}` executes iterations on OpenMP threads when
built with `make OPENMP=1`. Lanes see globals (read) plus private locals.
Write only distinct array indices. No `break/continue/return` in lanes.

## 11. Limitations (alpha)

Strings are an immutable stub: no slicing, indexing, or formatting.
covers the scalar core only, arrays share refernces.

## 12. Bytecode VM parity audit (prototype, v0.0.1b1)

| Construct | `--bc` support |
|---|---|
| number / boolean literals, variables, `let` | ??? |
| `+ - * /`, comparisons, `&& \|\| !` | ??? |
| `print`, `if/else`, `while`, `break`, `continue` | ??? |
| `const` | ??? enforced (since v0.0.1b2) |
| string literals, `+`, `==`/`!=`, `[i]`, `[a:b]`, `len` | ??? (since v0.0.2)|
| user functions (`fn`), recursion, calls | ??? (since v0.0.2a06; hoisted)|
| vec3 / mat4 / particle / array / string | ??? codegen error |
| member access, indexing, index assignment | ??? codegen error |
| all built-in functions | ??? codegen error |
| user functions (`fn`) | ??? codegen error |
| `sim`, `step`, `parallel for` | ??? codegen error |

Bytecode functions compile to jumped-over body regions with a
function table; `OP_CALL`/`OP_RETURN` manage a frame stack. Top-level
function are pre-scanned, so mutual recursion compiles. Blocks remain
scope-flat in the bytecode VM (function frames only).
Bytecode string opcodes: `OP_INDEX_STR`, `OP_SLICE_STR`, `OP_LEN`.
Bytecode roots (constants, globals, operand stack) are marked before
every sweep; collection runs at `OP_LOOP` back-edges.
Runtime error columns: the tree-walk VM reports columns for constant
violations and division by zero; full column migration is scheduled
for v0.0.1rc1. Bytecode VM runtime errors remain line-only.

## 13. Feature freeze (v0.0.1)

Feature-complete. All runtime errors report exact source columns in
both VMs. Language frozen.

## 14. Beta freeze (v0.0.2)

The 0.0.2 line is feature-frozen: no new syntax, types, or built-ins
until v0.0.2 ships. Remaining beta work is limited to memory
validation (ASan/UBSan/Valgrind), bytecode parity markers,
documentation, and bug fixes.

## 15. Memory model and garbage collection (v0.0.2)

Heap types: arrays and strings (including slice and concatenation
results). Every heap object carries an `Obj` header and lives in a
global registry list.

Collection: mark-and-sweep.

- Roots (tree-walk VM): global variable table, all call frames,
  return slot.
- Roots (bytecode VM): chunk constants, bytecode globals, call
  frames, operand stack.
- Safe points: top-level statement loop, `while` iterations, `sim`
  iterations, and `OP_LOOP` back-edges. Collection never runs
  mid-expression, so unrooted C temporaries cannot be swept.
- `parallel for` suspends collection for the duration of the region.
- Threshold: collect when allocated bytes exceed `next_gc`; after
  each sweep `next_gc = 2 * live bytes`. Override with the
  `KINETRA_GC_THRESHOLD` environment variable (bytes) for
  benchmarking only.
- Shutdown: `gc_free_all()` releases every object; a clean run
  reports zero definitely-lost bytes under Valgrind.

Not collected: C-side temporaries inside built-ins (freed manually),
AST and token memory (process-lifetime).

### Bytecode divergences (documented, intentional)

1. Function declarations are hoisted: callable before their source
   line executes.
2. Blocks are scope-flat; only function calls create scopes.
3. Codegen errors (unsupported): arrays, vec3, mat4, particle,
   `sim`, `parallel for`, and all built-ins except `len` on strings.
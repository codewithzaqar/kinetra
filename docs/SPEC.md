# Kinetra Language Specification --- Draft (v0.0.1a21)

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
- Built-in function names are reserved: see §8.

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

```
||
&&
== != < > <= >=
+ -
* /
! (unary)
. member acces, [i] indexing (postfix)
```

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
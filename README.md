# Kinetra v0.0.1

**Kinetra** is a statically-lexed, dynamically-typed scripting language for
mathematics, physics simulation, and high-performance computing. It pairs a
familiar C-like syntax with first-class vector / matrix / particle types,
deterministic simulation loops, and an opt-in OpenMP parallel backend.

```kinetra
let g = vec(0.0, -9.8, 0.0);
let ball = particle(vec3(0.0, 50.0, 0.0), vec3(5.0, 0.0, 0.0), 1.0);

sim 60 dt 0.016 {
	ball = apply_force(ball, g * ball.mass);
	ball = integrate(ball, dt);
}

print ball.position;
```

---

## Features

- **Seven built-in types:** number, boolean, `vec3`, `mat4`, `particle`, array, string
- **Lexical scoping**, `const` bindings, user-defined functions with recursion
- **Deterministic simulation loops:** `sim [steps] [dt value] {...}`
- **Data parallelism:** `parallel for i in n {...}` (OpenMP, opt-in build)
- **26-function standard library** (vector, matrix, math, trig, array, particle, string)
- **Two execution backends:** tree-walk VM (full language) and prototype
bytecode VM (`--bc`, scalar core, ~4x faster on supported code)
- **Precise diagnostics:** source snippets with caret positioning and stage-specific codes
- **Batteries included:** REPL, token/AST dumps, benchmark harness, 20-case regression suite, CI

--- 

## Build

Requirements: GCC or Clang (C11) and GNU make. OpenMP is optional.

	```
	make            			   # serial build
	make OPENMP = 1 			   # build with the OpenMP parallel backend
	make test       			   # run the 20-case regression suite
	make install PREFIX=/usr/local # install the binary
	```

Windows (MinGW): the same commands work from PowerShell or cmd.
Note: mingw.org MinGW ships without OpenMP --- use MinGW-w64 for `OPENMP=1`.

## Run

```
./kinetra examples/bouncing_ball.knt
```

### CLI reference

| Flag | Effect |
|---|---|
|`--bc`|Execute using the prototype bytecode VM|
|`--bench`|Run 100 silent iterations and report timing|
|`--tokens`|Dump the token stream and exit|
|`--ast`|Dump the syntax tree and exit|
|`--repl`|Start an interactive REPL|
|`--folds`|Report the parser's constant-fold count|
|`--raw`|Suppress banner/stage output (for testing)|
|`--version`|Print the version and exit|
|`--help`, `-h`| Show usgae|

### Exit codes

|Code|Meaning|
|---|---|
|0|Success|
|1|Lex error|
|2|Parse error|
|3|Runtime error|
|4|I/O error|
|5|Codegen error (construct unsupported by `--bc`)|
|64|CLI usage error|

---

## Language tour

### Values and types

|Type|Literal / constructor|Notes|
|---|---|---|
|number|`1.0`,`42`|IEEE-754 double|
|boolean|`true`/`false`||
|vec3|`vec3(x,y,z)`|member access `x y z`|
|mat4|`mat4()`/`mat4(16 values)`|row-major|
|particle|`particle(pos, vel[, mass])`|members `.position .velocity .force .mass` |
|array|`[a, b, c]`|reference semantics, heterogeneous|
|string|`"text"`|immutable, `\" \\ \n \t` escapes|

### Variables and constants

```kinetra
let x = 1.0;        // mutable binding
const limit = 10.0; // immutable; assinment is a runtime error
```

Blocks introduce scopes: declarations inside `{}` die at the closing brace.
Assignment targets the nearest existing binding in the scope chain.

### Control flow

```kinetra
if x < 0.0 {
	print 0.0 - x;
} else if x == 0.0 {
	print 0.0;
} else {
	print x;
}

while x > 1.0 {
	x = x / 2.0;
	if x < 1.5 { break; }
}
```

### Functions

```kinetra
fn hypotenuse(a, b) {
	return sqrt(a * a + b * b);
}

print hypotenuse(3.0, 4.0);  // 5
```

Functions are registered when their declaration executes; recursion is
supported; bare `return;` yields `0`.

### Simulation loops

```kinetra
sim 100 dt 0.016 {
	p = apply_force(p, gravity * p.mass);
	p = integrate(p, dt);
}
```

`step_index` and `dt` are provided automatically inside `sim`.
Custom time step: `sim 100 dt 0.016 {...}`; step count may also be given
inside the body with `step 100;`

### Parallel loops

```kinetra
parallel for i in len(out) {
	out[i] = heavy(i);  // write only distinct array indices.
}
```

Each iteration runs in a private scope; with `make OPENMP=1` iterations are
distributed across OpenMP threads. See `docs/SPEC.md` §10 for the full
semantics and limitations.

### Standard library

|Group|Functions|
|---|---|
|Vector|`dot` `cross` `length` `normalize` `reflect`|
|Numeric|`sqrt` `abs` `min` `max` `pow` `exp` `log` `floor` `ceil` `round` `clamp` `lerp`|
|Trig|`sin` `cos` `tan`|
|Matrix|`translate` `rotate` `scale` `transform`|
|Array / string|`len`|
|Particle|`integrate` `apply_force` `clear_force`|

---

## Examples

- `examples/bouncing_ball.knt` --- 1D physics with energy loss on bounce
- `examples/matrix_pipeline.knt` --- scale -> rotate -> translate transform chain

---

## Tooling

Interactive REPL with persistent state and error recovery:

```bash
$ ./kinetra --repl
> let x = 5;
> x * 2
[Kinetra] 10
```

Inspection and measurement:

```bash
./kinetra --tokens program.knt     # token stream with positions
./kinetra --ast program.knt        # syntax tree
./kinetra --bench program.knt      # tree-walk timing
./kinetra --bc --bench program.knt # bytecode timing
pwsh tools/bench.ps1           	   # side-by-side VM comparison
```

Diagnostics render the offending source line with a caret:

```bash
[Kinetra Error] (parse) Expected ')'
	-> bad.knt:1:15
		1|let x = (1 + 2;
		 |         ^
```

---

## Testing and CI

```bash
make test
```

Runs the 20-case regression matrix in `tests/` (each case: `.knt` source,
`.expected` output, optional `.code` exit code, optional `.bctest` bytecode
parity marker). GitHub Actions builds and tests every push on Linux and
macOS; Windows (MinGW) is verified manually before each tag.

---

## Project layout

```
include/  kinetra.h, value.h, diagnostics.h, bytecode.h, hpc_math.h
src/  main.c, lexer.c, parser.c, vm.c, bytecode.c, diagnostics.c, hpc_math.c
tests/  regession matrix + run_tests.ps1 + generate_tests.ps1
examples/ showcase programs
tools/  bench.ps1
docs/  SPEC.md (language specification), BENCH.md (benchmark guide)
```

---

## Documentation

- `docs/SPEC.md` --- language specification (grammar, scoping, semantics,
bytecode parity audit, feature-freeze policy)
- `docs/BENCH.md` --- how to benchmark both VMs and the OpenMP backend
- `CHANGELOG.md` --- per-release history from `v0.0.1a01` to `v0.0.1`

---

## Roadmap

- **v0.1.0** --- string slicing/formatting, garbage collection
- **v0.2.0** --- bytecode VM feature parity with tree-walk VM
- **v1.0.0** --- module system, compile-time evaluation, JIT prototype

---

## Code of Conduct

This project adheres to the [Contribute Covenant Code of Conduct](CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable
behavior to [INSERT CONTACT METHOD].

## License

Kinetra is open-source software licensed under the [MIT License](LICENSE).
You are free to use, modify, and distribute it, including in proprietary and
commercial applications, provided the copyright notice is retained.
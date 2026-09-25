# Contributing to Kinetra

Thank you for your interest in contributing to Kinetra! Whether you are fixing a bug, improving documentation, or proposing a new feature, your help is highly appreciated.

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to Contribute

- **Report Bugs:** If you find a crash, an incorrect output, or a diagnostic error with a misplaced caret, please open an Issue. Include the Kinetra source code that triggered the bug and the exact error output.
- **Improve Documentation:** Typos, unclear explanations in `docs/SPEC.md`, or missing examples are always welcome.
- **Fix Bugs:** Look through the GitHub issues for bugs tagged `good first issue` or `help wanted`.
- **Propose Features:** Kinetra v0.0.1 is a stable release. If you have an idea for a major new language feature (e.g., a new type, module system, or garbage collection), please open an Issue to discuss the design *before* writing code.

## Development Setup

### Prerequisites
- A C11 compatible compiler (GCC or Clang)
- GNU Make
- PowerShell (Core `pwsh` or Windows PowerShell) for running the test suite

### Building
Clone the repository and build the compiler/VM:

```bash
make            # Standard serial build
make OPENMP=1   # Build with OpenMP parallel backend
```

### Running the REPL
The fastest way to test language features interactively:
```bash
./kinetra --repl
```

## Project Architecture

If you are looking to modify the language, here is where everything lives:

|File|Responsibility|
|---|---|
|`include/kinetra.h`|Token types, AST node types, and core prototypes|
|`include/value.h`|The `KValue` tagged union (the runtime value model)|
|`src/lexer.c`|Converts source text into a stream of Tokens|
|`src/parser.c`|Converts Tokens into an Abstract Syntax Tree (AST)|
|`src/vm.c`|The primary tree-walk Virtual Machine|
|`src/bytecode.c`|The prototype stack-based bytecode compiler and VM|
|`src/diagnostics.c`|Error reporting, source snippets, and caret positioning|
|`src/hpc_math.c`|The underlying C implementations of vector/matrix math|

## How to Add a New Built-in Function

Adding a standard library function (like `sqrt` or `len`) requires touching four files:

1. **Lexer (`src/lexer.c`):** Add the function name to the `TOKEN_BUILTIN` keyword list.
2. **Parser (`src/parser.c`):** Add the function to the arity-checking block (1, 2, or 3 arguments).
3. **Tree-Walk VM (`src/vm.c`):** Add the evaluation logic inside the `NODE_CALL` switch statement.
4. **Tests (`tests/`):** Add a new `.knt` test case and its `.expected` output.

*Note: The prototype bytecode VM (`src/bytecode.c`) currently only supports the scalar core. If your built-in uses arrays, matrices, or strings, it will correctly throw a codegen error (Exit Code 5) under `--bc`.*

## Testing and Coding Standards

### The Golden Rule: Zero Warnings
Kinetra is built with `-Wall -Wextra`. Your code **must** compile with zero warnings. If the compiler warns about an unused variable or implicit conversion, fix it before submitting.

### The Test Suite
Before opening a Pull Request, you must ensure the regression suite passes:

```bash
make test
```

This runs `tests/run_tests.ps1`, executing 20+ test cases against both the tree-walk VM and the bytecode VM (where applicable).

**Adding a new test:**
1. Create `tests/XX_name.knt` with your Kinetra code.
2. Create `tests/XX_name.expected` with the exact expected stdout.
3. *(Optional)* Create `tests/XX_name.code` cintaining the expected exit code (e.g., `3` for a runtime error).
4. *(Optional)* Create an empty `tests/XX_name.bctest` file if the test should also be verified against the bytecode VM.

## Pull Request Process

1. **Fork** the repository and create a new branch (`git checkout -b feature/my-new-feature`).
2. **Commit** your changes. Write clear commit messages (e.g., `feat: add string slicing`, `fix: correct caret alignment on Windows`).
3. **Test** your changes locally (`make clean && make && make test`).
4. **Push** to your fork and open a Pull Request against the `main` branch.
5. **Describe** your changes in the PR description. If it fixes an issue, link it (e.g., "Fixes #42").

Thank you for helping make Kinetra better!
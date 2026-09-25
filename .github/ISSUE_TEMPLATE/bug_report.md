---
name: Bug Report
about: Report a crash, wrong output, or diagnostic defect in Kinetra
title: "[Bug]"
labels: bug
assignees: ''
---

## Describe the bug
A clear and concise description of what happened.

## Minimal reproduction
The smallest `.knt` program that triggers the bug:

```kinetra
// paste minimal program here
```

## Expected behabior:
Expected output and exit code.

## Actual behavior
Full output, including banner lines and any error snippet:

```text
paste output here
```

Exit code: (PowerShell: `echo $LASTEXITCODE` / bash: `echo $?`)

## Environment
- Kinetra version: (output of `./kinetra --version`)
- OS: (Windows / Linux / macOS + version)
- Compiler: (`gcc --version` or `clang --version`)
- Build: serial (`make`) or OpenMP (`make OPENMP=1`)
- VM used: tree-walk (default) / bytecode (`--bc`) / both

## Additional context
- Docs it reproduce with `--raw`?
- Does it reproduce on the other VM backend?
- If this is a segfault/crash: backtrace if available (gdb, lldb, or Windows debugger).

> **Security note:** If this bug is a crash or memory-corruption issue that
> could be exploited by a malicious script, report it privately per
> `SECURITY.md` instead of opening a public issue.
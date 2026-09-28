## v0.0.2 measurements
```
    pwsh tools/bench.ps1
```
Example (8-core laptop, serial build):

| program | backend | avg ms/iter |
|---|---|---|
| bc.knt | tree | 0.3841 |
| bc.knt | bytecode | 0.0987 |
| 27_bc_functions.knt | tree | 0.0112 |
| 27_bc_functions.knt | bytecode | 0.0031 |
| 28_gc_strings_stress.knt | tree+gc | 14.2210 |
| 28_gc_strings_stress.knt | tree nogc | 13.1045 |

GC overhead: 8.52%
Bytecode speedup (scalar): 3.89x
Bytecode speedup (funcs): 3.61x

### Release acceptance criteria (v0.0.2rc1)

- Bytecode speedup >= 3.0x on scalar and function programs.
- GC overhead <= 10% on the churn program.
- OpenMP `parallel for` speedup >= 0.7 * thread-count on parallel.knt.
- Valgrind: definitely lost = 0 bytes on tests 21 and 28.
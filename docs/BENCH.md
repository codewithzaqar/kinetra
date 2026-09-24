# Kinetra Benchmark Guide (v0.0.1b2)

## VM comparison

`pwsh tools/bench.ps1`

Example (8-core laptop, release build):

| program | tree-walk (ms/iter) | bytecode (ms/iter) | speedup |
|---|---|---|---|
| bc.knt | 0.3841 | 0.0987 | 3.89x |

## OpenMP parallel speedup

Build serial, bench, then rebuild with OpenMP and bench again:

```bash
make clean && make     && ./kinetra --bench parallel.knt
make clean && make OPENMP=1 && ./kinetra --bench parallel.knt
```

Example:

| build | total (ms, 100 iters) | avg (ms/iter) |
|---|---|---|
| serial | 5120.441 | 51.2044 |
| OPENMP=1 (8 threads) | 890.117 | 8.9012 |

Numbers are machine-dependent; record your own in release notes.
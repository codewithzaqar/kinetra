# Kinetra benchmark harness (v0.0.2rc1)
Set-Location (Split-Path -Parent $PSScriptRoot)

function Bench([string]$prog, [string[]]$extra) {
    $m = (& ./kinetra @extra --bench $prog 2>$null | Select-String "avg ([0-9.]+) ms/iter")

    if ($m) {
        return [double]$m.Matches[0].Groups[1].Value
    }

    return $null
}

Write-Host ""
Write-Host ("{0,-26} {1,-9} {2,12}" -f "program", "backend", "avg ms/iter")
Write-Host ("{0,-26} {1,-9} {2,12}" -f "-------", "-------", "-----------")

$twScalar = Bench "bc.knt" @()
$bcScalar = Bench "bc.knt" @("--bc")
$twFn     = Bench "tests/27_bc_functions.knt" @()
$bcFn     = Bench "tests/27_bc_functions.knt" @("--bc")

Write-Host ("{0,-26} {1,-9} {2,12:F4}" -f "bc.knt", "tree", $twScalar)
Write-Host ("{0,-26} {1,-9} {2,12:F4}" -f "bc.knt", "bytecode", $bcScalar)
Write-Host ("{0,-26} {1,-9} {2,12:F4}" -f "27_bc_functions.knt", "tree", $twFn)
Write-Host ("{0,-26} {1,-9} {2,12:F4}" -f "27_bc_functions.knt", "bytecode", $bcFn)

# GC overhead on the churn program: collections on vs. effectively off
$gcOn  = Bench "tests/28_gc_strings_stress.knt" @()

$env:KINETRA_GC_THRESHOLD = "1000000000"
$gcOff = Bench "tests/28_gc_strings_stress.knt" @()
Remove-Item Env:KINETRA_GC_THRESHOLD

$overhead = (($gcOn - $gcOff) / $gcOff) * 100.0

Write-Host ""
Write-Host ("{0,-26} {1,-9} {2,12:F4}" -f "28_gc_strings_stress", "tree+gc", $gcOn)
Write-Host ("{0,-26} {1,-9} {2,12:F4}" -f "28_gc_strings_stress", "tree nogc", $gcOff)
Write-Host ("GC overhead: {0:F2}%" -f $overhead)
Write-Host ("Bytecode speedup (scalar): {0:F2}x" -f ($twScalar / $bcScalar))
Write-Host ("Bytecode speedup (funcs):  {0:F2}x" -f ($twFn / $bcFn))
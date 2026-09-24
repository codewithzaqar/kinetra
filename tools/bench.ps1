# Kinetra VM comparison bench (v0.0.1b2)
Set-Location (Split-Path -Parent $PSScriptRoot)

$prog = "bc.knt"

$tw = (& ./kinetra --bench $prog  | Select-String "avg ([0-9.]+) ms/iter")
$bv = (& ./kinetra --bc --bench $prog | Select-String "avg ([0-9.]+) ms/iter")

$twMs = [double]$tw.Matches[0].Groups[1].Value
$bvMs = [double]$bv.Matches[0].Groups[1].Value

Write-Host ""
Write-Host ("program  tree-walk(ms)  bytecide(ms)  speedup")
Write-Host ("{0,-9} {1,13:F4} {2,14:F4} {3,10:F2}x" -f $prog, $twMs, $bvMs, ($twMs / $bvMs))
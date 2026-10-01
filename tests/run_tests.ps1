# Kinetra regression test runner (v0.0.3a04)
# Usage: pwsh -NoProfile -ExecutionPolicy Bypass -File tests/run_tests.ps1
#
# Per-case files (all optional except .knt):
#   <base>.knt        program under test
#   <base>.expected   exact stdout (normalized: CR stripped, trailing NL trimmed)
#   <base>.code       expected exit code (default 0)
#   <base>.bctest     marker: also run under --bc and compare same expectations
#   <base>.args       extra CLI flags for this case (whitespace-separated)

Set-Location (Split-Path -Parent $PSScriptRoot)

function Normalize([string]$text) {
    $t = $text -replace "`r", ""
    return $t.TrimEnd("`n")
}

function Get-ExtraArgs($case) {
    $argsPath = Join-Path $case.DirectoryName ($case.BaseName + ".args")

    if (Test-Path $argsPath -PathType Leaf) {
        $raw = (Get-Content $argsPath -Raw).Trim()

        if ($raw) {
            return @($raw -split '\s+')
        }
    }

    return @()
}

$cases = Get-ChildItem "tests/*.knt" | Sort-Object Name

if (-not $cases -or @($cases).Count -eq 0) {
    Write-Host "FATAL: no test cases found in tests/"
    exit 1
}

$pass = 0
$fail = 0

foreach ($case in $cases) {
    $base = $case.BaseName
    $dir  = $case.DirectoryName

    $expectedPath = Join-Path $dir ($base + ".expected")
    $codePath     = Join-Path $dir ($base + ".code")
    $bcPath       = Join-Path $dir ($base + ".bctest")

    # Force array so @extra splats correctly even for one flag or none
    $extra = @(Get-ExtraArgs $case)

    $wantCode = 0

    if (Test-Path $codePath -PathType Leaf) {
        $wantCode = [int]((Get-Content $codePath -Raw).Trim())
    }

    $wantOut = ""

    if (Test-Path $expectedPath -PathType Leaf) {
        $wantOut = Normalize (Get-Content $expectedPath -Raw)
    }

    # ---- tree-walk pass (with per-case extra args) ----
    $gotOut  = Normalize ((& ./kinetra --raw @extra $case.FullName) | Out-String)
    $gotCode = $LASTEXITCODE

    if ($gotCode -ne $wantCode) {
        Write-Host "  FAIL(exit) tests/$base.knt : expected exit $wantCode, got $gotCode"
        Write-Host "--- expected ---"
        Write-Host $wantOut
        Write-Host ""
        Write-Host "--- actual ---"
        Write-Host $gotOut
        $fail++
        continue
    }

    if ($gotOut -ne $wantOut) {
        Write-Host "  FAIL(diff) tests/$base.knt"
        Write-Host "--- expected ---"
        Write-Host $wantOut
        Write-Host ""
        Write-Host "--- actual ---"
        Write-Host $gotOut
        $fail++
        continue
    }

    # ---- optional bytecode parity pass ----
    if (Test-Path $bcPath -PathType Leaf) {
        $bcOut  = Normalize ((& ./kinetra --raw --bc @extra $case.FullName) | Out-String)
        $bcCode = $LASTEXITCODE

        if ($bcCode -ne $wantCode) {
            Write-Host "  FAIL(bc-exit) tests/$base.knt : expected exit $wantCode, got $bcCode"
            Write-Host "--- expected ---"
            Write-Host $wantOut
            Write-Host ""
            Write-Host "--- actual (bc) ---"
            Write-Host $bcOut
            $fail++
            continue
        }

        if ($bcOut -ne $wantOut) {
            Write-Host "  FAIL(bc-diff) tests/$base.knt"
            Write-Host "--- expected ---"
            Write-Host $wantOut
            Write-Host ""
            Write-Host "--- actual (bc) ---"
            Write-Host $bcOut
            $fail++
            continue
        }

        Write-Host "  PASS tests/$base.knt (+bc)"
    } else {
        Write-Host "  PASS tests/$base.knt"
    }

    $pass++
}

Write-Host ""
Write-Host "$pass passed, $fail failed"

if ($fail -gt 0) {
    exit 1
}

exit 0
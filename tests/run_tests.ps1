# Kinetra test runner (v0.0.1b2)
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Normalize([string]$text) {
	$text = $text -replace "`r", ""
	return $text.TrimEnd("`n")
}

function Run-Case([string]$rel, [string[]]$extraArgs, [string]$expected, [int]$wantCode) {
    $tmp = Join-Path $env:TEMP ("kinetra_out_" + [guid]::NewGuid().ToString("n") + ".txt")

    cmd /c "`"./kinetra`" $($extraArgs -join ' ') --raw $rel > `"$tmp`" 2>&1"
    $gotCode = $LASTEXITCODE

    $out = ""
    if (Test-Path $tmp) {
        $out = Get-Content $tmp -Raw
        Remove-Item $tmp -ErrorAction SilentlyContinue
    }

    if ($gotCode -ne $wantCode) {
        Write-Host "  FAIL(exit) $rel : expected exit $wantCode, got $gotCode"
        return $false
    }

    $actual = Normalize $out

    if ($actual -ne $expected) {
        Write-Host "  FAIL(diff) $rel $($extraArgs -join ' ')"
        Write-Host "--- expected ---"
        Write-Host $expected
        Write-Host "--- actual ---"
        Write-Host $actual
        return $false
    }

    return $true
}

$pass = 0
$fail = 0
$cases = Get-ChildItem "tests/*.knt" | Sort-Object Name

foreach ($case in $cases) {
	$base = $case.FullName -replace '\.knt$', ''
	$rel = "tests/" + $case.Name

	$wantCode = 0
	if (Test-Path "$base.code") {
		$wantCode = [int]((Get-Content "$base.code" -Raw).Trim())
	}

	$expected = ""
	if (Test-Path "$base.expected") {
		$expected = Normalize (Get-Content "$base.expected" -Raw)
	}

	# Pass 1: tree-walk VM
	if (Run-Case $rel @() $expected $wantCode) {
		$pass++
	} else {
		$fail++
		continue
	}

	# Pass 2: bytecode VM parity (only for marked cases)
	if (Test-Path "$base.bctest") {
		if (Run-Case $rel @("--bc") $expected $wantCode) {
			Write-Host "PASS $rel (+bc)"
		} else {
			$fail++
			continue
		}
	} else {
		Write-Host "PASS $rel"
	}
}

Write-Host ""
Write-Host "$pass passed, $fail failed"

if ($fail -ne 0) { exit 1 }
exit 0
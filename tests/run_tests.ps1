# Kinetra test runner (v0.0.1b1)
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Normalize([string]$text) {
	$text = $text -replace "`r", ""
	return $text.TrimEnd("`n")
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

	$out = (& ./kinetra --raw $rel 2>&1 | Out-String)
	$gotCode = $LASTEXITCODE

	if ($gotCode -ne $wantCode) {
		Write-Host "FAIL(exit) $rel : expected exit $wantCode, got $gotCode"
		$fail++
		continue
	}

	$expected = ""
	if (Test-Path "$base.expected") {
		$expected = Normalize (Get-Content "$base.expected" -Raw)
	}

	$actual = Normalize $out

	if ($actual -eq $expected) {
		Write-Host "PASS $rel"
		$pass++
	} else {
		Write-Host "FAIL(diff) $rel"
		Write-Host "--- expected ---"
		Write-Host $expected
		Write-Host "--- actual ---"
		Write-Host $actual
		$fail++
	}
}

Write-Host ""
Write-Host "$pass passed, $fail failed"

if ($fail -ne 0) { exit 1 }
exit 0
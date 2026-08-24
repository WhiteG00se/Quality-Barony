[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $GameDir
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Launcher = Join-Path $ProjectRoot 'distributable\QualityBaronyLauncher.exe'
$Runtime = Join-Path $ProjectRoot 'distributable\QualityBarony.dll'
$GameExe = Join-Path $GameDir 'barony.exe'
$ExpectedHash = '8566DA37BC39EA5A1ED08A8AD57608AF4F019FB415869258FB3C1D310B4419E4'

& (Join-Path $PSScriptRoot 'build.ps1')

if (-not (Test-Path -LiteralPath $GameExe -PathType Leaf)) {
    throw "barony.exe was not found: $GameExe"
}
$BeforeHash = (Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash
if ($BeforeHash -ne $ExpectedHash) {
    throw "Installed game is not the supported v5.0.2 executable: $BeforeHash"
}

Write-Host 'Checking supported executable verification...'
& $Launcher '--game-dir' $GameDir '--verify-only'
if ($LASTEXITCODE -ne 0) { throw 'The launcher rejected the supported executable.' }

Write-Host 'Checking altered executable rejection...'
$AlteredDirectory = Join-Path $ProjectRoot '.local\altered-game'
$AlteredExe = Join-Path $AlteredDirectory 'barony.exe'
New-Item -ItemType Directory -Force -Path $AlteredDirectory | Out-Null
Copy-Item -LiteralPath $GameExe -Destination $AlteredExe -Force
$Stream = [System.IO.File]::Open($AlteredExe,
    [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite)
try {
    $FirstByte = $Stream.ReadByte()
    $Stream.Position = 0
    $Stream.WriteByte($FirstByte -bxor 1)
} finally {
    $Stream.Dispose()
}
& $Launcher '--game-dir' $AlteredDirectory '--verify-only'
if ($LASTEXITCODE -ne 5) {
    throw "Altered executable rejection returned $LASTEXITCODE instead of 5."
}

Write-Host 'Checking missing runtime rejection...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' `
    (Join-Path $ProjectRoot '.local\missing-QualityBarony.dll') '--test-injection'
if ($LASTEXITCODE -ne 6) {
    throw "Missing runtime rejection returned $LASTEXITCODE instead of 6."
}

Write-Host 'Checking DLL injection and combined runtime signatures in a suspended process...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' $Runtime '--test-injection'
if ($LASTEXITCODE -ne 0) { throw 'The suspended-process injection test failed.' }

Write-Host 'Checking unsupported in-memory runtime signature rejection...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' $Runtime `
    '--test-signature-rejection'
if ($LASTEXITCODE -ne 0) { throw 'The runtime accepted an unsupported signature.' }

Write-Host 'Checking unsupported in-memory friendly-fire signature rejection...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' $Runtime `
    '--test-friendly-fire-signature-rejection'
if ($LASTEXITCODE -ne 0) {
    throw 'The runtime accepted an unsupported friendly-fire signature.'
}

Write-Host 'Checking unsupported in-memory EXP-credit signature rejection...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' $Runtime `
    '--test-exp-credit-signature-rejection'
if ($LASTEXITCODE -ne 0) {
    throw 'The runtime accepted an unsupported EXP-credit signature.'
}

Write-Host 'Checking unsupported in-memory minimap signature rejection...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' $Runtime `
    '--test-minimap-signature-rejection'
if ($LASTEXITCODE -ne 0) { throw 'The runtime accepted an unsupported minimap signature.' }

Write-Host 'Checking unsupported in-memory exit-reveal signature rejection...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' $Runtime `
    '--test-reveal-signature-rejection'
if ($LASTEXITCODE -ne 0) { throw 'The runtime accepted an unsupported exit-reveal signature.' }

Write-Host 'Checking unsupported in-memory party-item marker signature rejection...'
& $Launcher '--game-dir' $GameDir '--runtime-dll' $Runtime `
    '--test-item-marker-signature-rejection'
if ($LASTEXITCODE -ne 0) { throw 'The runtime accepted an unsupported party-item marker signature.' }

$AfterHash = (Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash
if ($AfterHash -ne $BeforeHash) {
    throw 'The installed barony.exe changed during runtime testing.'
}
Write-Host "Runtime tests passed; installed barony.exe remained unchanged: $AfterHash"

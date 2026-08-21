[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$LocalRoot = Join-Path $ProjectRoot '.local'
$ToolchainName = 'llvm-mingw-20260616-ucrt-x86_64'
$ToolchainRoot = Join-Path $LocalRoot $ToolchainName
$ToolchainArchive = Join-Path $LocalRoot 'llvm-mingw-20260616-ucrt-x86_64.zip'
$Compiler = Join-Path $ToolchainRoot 'bin\x86_64-w64-mingw32-clang++.exe'
$ToolchainUri = 'https://github.com/mstorsjo/llvm-mingw/releases/download/20260616/llvm-mingw-20260616-ucrt-x86_64.zip'
$ToolchainHash = 'B9B68A4D276E16FA25802AABA458E4638F64B3884C290AACCDC2D87083B6CA35'
$SourceRoot = Join-Path $ProjectRoot 'source'
$Distribution = Join-Path $ProjectRoot 'distributable'
$TestExecutable = Join-Path $LocalRoot 'QualityBaronyXpTests.exe'

New-Item -ItemType Directory -Force -Path $LocalRoot | Out-Null
if (-not (Test-Path -LiteralPath $Compiler -PathType Leaf)) {
    if (-not (Test-Path -LiteralPath $ToolchainArchive -PathType Leaf)) {
        Write-Host 'Downloading the pinned LLVM-MinGW toolchain...'
        Invoke-WebRequest -UseBasicParsing -Uri $ToolchainUri -OutFile $ToolchainArchive
    }
    $ActualHash = (Get-FileHash -LiteralPath $ToolchainArchive -Algorithm SHA256).Hash
    if ($ActualHash -ne $ToolchainHash) {
        throw "LLVM-MinGW archive hash mismatch. Expected $ToolchainHash; received $ActualHash"
    }
    Expand-Archive -LiteralPath $ToolchainArchive -DestinationPath $LocalRoot -Force
}
if (-not (Test-Path -LiteralPath $Compiler -PathType Leaf)) {
    throw "Compiler was not found after extraction: $Compiler"
}

$Common = @('-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-static')

Write-Host 'Building and running Quality EXP unit tests...'
& $Compiler @Common `
    (Join-Path $SourceRoot 'tests\xp_tests.cpp') `
    '-o' $TestExecutable
if ($LASTEXITCODE -ne 0) { throw 'EXP unit-test compilation failed.' }
& $TestExecutable
if ($LASTEXITCODE -ne 0) { throw 'EXP unit tests failed.' }

Write-Host 'Building QualityBarony.dll directly into distributable...'
& $Compiler @Common '-DNDEBUG' '-shared' `
    (Join-Path $SourceRoot 'quality\dll_main.cpp') `
    (Join-Path $SourceRoot 'quality\quality_runtime.cpp') `
    '-o' (Join-Path $Distribution 'QualityBarony.dll')
if ($LASTEXITCODE -ne 0) { throw 'Quality runtime DLL build failed.' }

Write-Host 'Building QualityBaronyLauncher.exe directly into distributable...'
& $Compiler @Common '-DNDEBUG' '-municode' `
    (Join-Path $SourceRoot 'quality\launcher.cpp') `
    '-lbcrypt' '-ladvapi32' `
    '-o' (Join-Path $Distribution 'QualityBaronyLauncher.exe')
if ($LASTEXITCODE -ne 0) { throw 'Quality launcher build failed.' }

Write-Host "Quality Barony runtime ready in $Distribution"

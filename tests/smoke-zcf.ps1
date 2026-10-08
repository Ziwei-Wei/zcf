#requires -Version 7.2

[CmdletBinding()]
param(
    [string]$ZcfExe = '',
    [string]$ClangFormatCompatExe = '',
    [string]$WorkDir = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $ZcfExe) {
    $ZcfExe = Join-Path $repoRoot 'build\release\bin\zcf.exe'
}
if (-not $ClangFormatCompatExe) {
    $ClangFormatCompatExe =
        Join-Path $repoRoot 'build\release\bin\clang-format.exe'
}
if (-not $WorkDir) {
    $WorkDir = Join-Path $repoRoot 'build\zcf-smoke'
}

$ZcfExe = [System.IO.Path]::GetFullPath($ZcfExe)
$ClangFormatCompatExe =
    [System.IO.Path]::GetFullPath($ClangFormatCompatExe)
$WorkDir = [System.IO.Path]::GetFullPath($WorkDir)

foreach ($path in @($ZcfExe, $ClangFormatCompatExe)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required formatter executable was not found: $path"
    }
}

Remove-Item -LiteralPath $WorkDir -Recurse -Force `
    -ErrorAction SilentlyContinue
[void](New-Item -ItemType Directory -Force -Path $WorkDir)

$zcfVersion = & $ZcfExe --version
if ($LASTEXITCODE -ne 0 -or
    ($zcfVersion -join "`n") -notmatch '^zcf \(clang-format version ') {
    throw "zcf reported an unexpected version: $zcfVersion"
}
Write-Host '[PASS] zcf version'

$clangVersion = & $ClangFormatCompatExe --version
if ($LASTEXITCODE -ne 0 -or
    ($clangVersion -join "`n") -notmatch '^clang-format version ') {
    throw "clang-format alias reported an unexpected version: $clangVersion"
}
Write-Host '[PASS] clang-format alias version'

$zcfHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $ZcfExe).Hash
$clangHash =
    (Get-FileHash -Algorithm SHA256 -LiteralPath $ClangFormatCompatExe).Hash
if ($zcfHash -cne $clangHash) {
    throw 'zcf and clang-format alias binaries are not byte-identical.'
}
Write-Host '[PASS] clang-format alias hash'

$sourcePath = Join-Path $WorkDir 'sample.cpp'
[System.IO.File]::WriteAllText(
    $sourcePath,
    "int main(){return 0;}`n",
    [System.Text.UTF8Encoding]::new($false)
)
$zcfOutput = & $ZcfExe -style=ZCF $sourcePath
if ($LASTEXITCODE -ne 0) {
    throw "zcf formatting failed with exit code $LASTEXITCODE."
}
$clangOutput = & $ClangFormatCompatExe -style=ZCF $sourcePath
if ($LASTEXITCODE -ne 0) {
    throw "clang-format alias failed with exit code $LASTEXITCODE."
}
if (($zcfOutput -join "`n") -cne ($clangOutput -join "`n")) {
    throw 'zcf and clang-format alias formatting output differ.'
}
if (($zcfOutput -join "`n") -notmatch "int`nmain\(\)") {
    throw 'zcf did not apply the ZCF preset.'
}
Write-Host '[PASS] ZCF alias parity'

$dump = & $ZcfExe -style=ZCF -dump-config
if ($LASTEXITCODE -ne 0 -or
    ($dump -join "`n") -notmatch 'BodyDrivenLambdaExpansion:\s+true') {
    throw 'zcf did not expose the ZCF extension configuration.'
}
Write-Host '[PASS] ZCF configuration'

Write-Host 'zcf smoke tests passed'

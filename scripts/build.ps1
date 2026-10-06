[CmdletBinding()]
param(
    [string]$LLVMProjectSourceDir = '',
    [string]$BuildDir = '',
    [string]$Configuration = 'Release',
    [string]$TargetsToBuild = 'host',
    [string]$Generator = 'Ninja',
    [int]$Jobs = 0,
    [switch]$StaticMsvcRuntime,
    [switch]$EnableAssertions
)

$ErrorActionPreference = 'Stop'

$ScriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$RepoRoot = Resolve-Path (Join-Path $ScriptRoot '..')

if (-not $LLVMProjectSourceDir) {
    $LLVMProjectSourceDir = Join-Path $RepoRoot.Path 'third_party\llvm-project'
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot.Path 'build\release'
}

$LLVMProjectSourceDir = [System.IO.Path]::GetFullPath($LLVMProjectSourceDir)
$BuildDir = [System.IO.Path]::GetFullPath($BuildDir)

if (-not (Test-Path (Join-Path $LLVMProjectSourceDir 'llvm\CMakeLists.txt'))) {
    throw "LLVMProjectSourceDir must point at a full llvm-project checkout: $LLVMProjectSourceDir"
}

$configureArgs = @(
    '-S', $RepoRoot.Path,
    '-B', $BuildDir,
    '-G', $Generator,
    "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DLLVM_PROJECT_SOURCE_DIR=$LLVMProjectSourceDir",
    "-DLLVM_TARGETS_TO_BUILD=$TargetsToBuild",
    "-DCUSTOM_STATIC_MSVC_RUNTIME=$($StaticMsvcRuntime.IsPresent)",
    "-DCUSTOM_ENABLE_ASSERTIONS=$($EnableAssertions.IsPresent)"
)

if ($Jobs -gt 0) {
    $configureArgs += "-DCUSTOM_BUILD_JOBS=$Jobs"
}

& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& cmake --build $BuildDir --target zcf
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$exeSuffix = if ($IsWindows -or $env:OS -eq 'Windows_NT') { '.exe' } else { '' }
$zcfPath = Join-Path $BuildDir "bin\zcf$exeSuffix"
$clangCompatibilityPath = Join-Path $BuildDir "bin\clang-format$exeSuffix"
Write-Host "Built $zcfPath"
Write-Host "Built $clangCompatibilityPath"

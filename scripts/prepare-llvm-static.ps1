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
    $BuildDir = Join-Path $RepoRoot.Path 'build\llvm-static'
}

$LLVMProjectSourceDir = [System.IO.Path]::GetFullPath($LLVMProjectSourceDir)
$BuildDir = [System.IO.Path]::GetFullPath($BuildDir)

if (-not (Test-Path (Join-Path $LLVMProjectSourceDir 'llvm\CMakeLists.txt'))) {
    throw "LLVMProjectSourceDir must point at a full llvm-project checkout: $LLVMProjectSourceDir"
}

$configureArgs = @(
    '-S', (Join-Path $LLVMProjectSourceDir 'llvm'),
    '-B', $BuildDir,
    '-G', $Generator,
    "-DCMAKE_BUILD_TYPE=$Configuration",
    '-DBUILD_SHARED_LIBS=OFF',
    '-DLLVM_BUILD_LLVM_DYLIB=OFF',
    '-DLLVM_LINK_LLVM_DYLIB=OFF',
    "-DLLVM_TARGETS_TO_BUILD=$TargetsToBuild",
    "-DLLVM_ENABLE_ASSERTIONS=$($EnableAssertions.IsPresent)",
    '-DLLVM_ENABLE_PROJECTS=',
    '-DLLVM_INCLUDE_TESTS=OFF',
    '-DLLVM_INCLUDE_EXAMPLES=OFF',
    '-DLLVM_INCLUDE_BENCHMARKS=OFF',
    '-DLLVM_INCLUDE_TOOLS=ON',
    '-DLLVM_BUILD_TOOLS=ON',
    '-DLLVM_BUILD_UTILS=OFF',
    '-DLLVM_ENABLE_LIBXML2=OFF',
    '-DLLVM_ENABLE_TERMINFO=OFF',
    '-DLLVM_ENABLE_ZLIB=OFF',
    '-DLLVM_ENABLE_ZSTD=OFF'
)

if ($StaticMsvcRuntime.IsPresent) {
    if ($Configuration -eq 'Debug') {
        $configureArgs += '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug'
    } else {
        $configureArgs += '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded'
    }
}

& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$buildArgs = @(
    '--build', $BuildDir,
    '--target',
    'llvm-tblgen',
    'omp_gen',
    'LLVMSupport',
    'LLVMTargetParser',
    'LLVMFrontendOpenMP'
)

if ($Jobs -gt 0) {
    $buildArgs += @('--parallel', $Jobs)
}

& cmake @buildArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$exeSuffix = if ($IsWindows -or $env:OS -eq 'Windows_NT') { '.exe' } else { '' }
$llvmDir = Join-Path $BuildDir 'lib\cmake\llvm'
$llvmTableGen = Join-Path $BuildDir "bin\llvm-tblgen$exeSuffix"

Write-Host ''
Write-Host 'Static LLVM prerequisite build is ready.'
Write-Host "CUSTOM_LLVM_DIR=$llvmDir"
Write-Host "CUSTOM_LLVM_TABLEGEN=$llvmTableGen"

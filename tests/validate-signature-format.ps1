#requires -Version 7.2

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$FormatterExe,
    [string]$StyleFile = '',
    [string]$UpstreamStyleFile = '',
    [string]$CaseDir = '',
    [string]$CanonicalFixture = '',
    [string]$WorkDir = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $StyleFile) {
    $StyleFile = Join-Path $repoRoot 'tests\cpp-style-examples\zcf-signatures.clang-format'
}
if (-not $UpstreamStyleFile) {
    $UpstreamStyleFile = Join-Path $repoRoot 'tests\cpp-style-examples\zcf-target.clang-format'
}
if (-not $CaseDir) {
    $CaseDir = Join-Path $repoRoot 'tests\zcf-style-contracts\cases'
}
if (-not $CanonicalFixture) {
    $CanonicalFixture = Join-Path $repoRoot 'tests\cpp-style-examples\zcf_supported_style_details.cpp'
}
if (-not $WorkDir) {
    $WorkDir = Join-Path $repoRoot 'build\signature-format-tests'
}

$FormatterExe = [System.IO.Path]::GetFullPath($FormatterExe)
$StyleFile = [System.IO.Path]::GetFullPath($StyleFile)
$UpstreamStyleFile = [System.IO.Path]::GetFullPath($UpstreamStyleFile)
$CaseDir = [System.IO.Path]::GetFullPath($CaseDir)
$CanonicalFixture = [System.IO.Path]::GetFullPath($CanonicalFixture)
$WorkDir = [System.IO.Path]::GetFullPath($WorkDir)
foreach ($requiredFile in @(
    $FormatterExe,
    $StyleFile,
    $UpstreamStyleFile,
    $CanonicalFixture
)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required file was not found: $requiredFile"
    }
}
if (-not (Test-Path -LiteralPath $CaseDir -PathType Container)) {
    throw "Signature case directory was not found: $CaseDir"
}

$caseIds = @(
    'closing-parenthesis-argument-indent',
    'force-multiline-function-signatures',
    'context-sensitive-braced-initializers',
    'blank-line-before-return'
)
$optionCases = [ordered]@{
    ArgumentIndentedClosingParentheses = 'closing-parenthesis-argument-indent'
    ForceMultilineFunctionSignatures = 'force-multiline-function-signatures'
    ContextSensitiveBracedInitializers = 'context-sensitive-braced-initializers'
    BlankLineBeforeReturn = 'blank-line-before-return'
}

if (Test-Path -LiteralPath $WorkDir) {
    Remove-Item -LiteralPath $WorkDir -Recurse -Force
}
[void](New-Item -ItemType Directory -Path $WorkDir)

$signatureStyleLines = @([System.IO.File]::ReadAllLines($StyleFile))
$extensionIndex = [Array]::IndexOf($signatureStyleLines, 'ZCFExtensions:')
if ($extensionIndex -lt 0) {
    throw 'Signature style does not contain ZCFExtensions.'
}
$upstreamPrefix = ($signatureStyleLines[0..($extensionIndex - 1)] -join "`n").TrimEnd()
$upstreamStyle = [System.IO.File]::ReadAllText($UpstreamStyleFile).
    Replace("`r`n", "`n").
    Replace(
        '# Native clang-format 20.1.8 baseline for the reviewed ZCF C++ policy.',
        "# Signature, initializer, and return extensions of the closest clang-format`n# 20.1.8 approximation."
    ).
    Replace(
        '# Extension-specific differences are covered by focused contract fixtures.',
        '# The upstream option prefix is checked against zcf-target.clang-format.'
    ).
    Replace("`n...`n", "`n").
    TrimEnd()
if ($upstreamPrefix -cne $upstreamStyle) {
    throw 'Signature style upstream options drifted from zcf-target.clang-format.'
}

$disabledDump = (& $FormatterExe -style=LLVM -dump-config 2>&1) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw "Disabled extension dump-config failed with exit code $LASTEXITCODE."
}
if ($disabledDump.Contains('ZCFExtensions:')) {
    throw 'Disabled dump-config unexpectedly emitted ZCFExtensions.'
}

$enabledDump = (
    & $FormatterExe "-style=file:$StyleFile" -dump-config `
        (Join-Path $CaseDir 'blank-line-before-return.input.cpp') 2>&1
) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw "Enabled extension dump-config failed with exit code $LASTEXITCODE."
}
foreach ($option in @(
    'ArgumentIndentedClosingParentheses',
    'ForceMultilineFunctionSignatures',
    'ContextSensitiveBracedInitializers',
    'BlankLineBeforeReturn'
)) {
    if (-not $enabledDump.Contains("${option}: true")) {
        throw "Enabled dump-config did not preserve $option."
    }
}

function Get-NormalizedText {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [System.IO.File]::ReadAllText($Path).
        Replace("`r`n", "`n").
        TrimEnd([char[]]"`n")
}

foreach ($caseId in $caseIds) {
    $inputPath = Join-Path $CaseDir "$caseId.input.cpp"
    $targetPath = Join-Path $CaseDir "$caseId.zcf.cpp"
    $actualPath = Join-Path $WorkDir "$caseId.actual.cpp"
    foreach ($requiredFile in @($inputPath, $targetPath)) {
        if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
            throw "$caseId is missing required file: $requiredFile"
        }
    }

    Copy-Item -LiteralPath $inputPath -Destination $actualPath
    & $FormatterExe "-style=file:$StyleFile" -i $actualPath
    if ($LASTEXITCODE -ne 0) {
        throw "$caseId formatting failed with exit code $LASTEXITCODE."
    }

    $firstPass = Get-NormalizedText -Path $actualPath
    $target = Get-NormalizedText -Path $targetPath
    if ($firstPass -cne $target) {
        throw "$caseId output differs from its checked-in approved target."
    }

    & $FormatterExe "-style=file:$StyleFile" -i $actualPath
    if ($LASTEXITCODE -ne 0) {
        throw "$caseId second formatting pass failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $actualPath) -cne $firstPass) {
        throw "$caseId is not idempotent across two formatter passes."
    }

    Write-Host "PASS $caseId"
}

foreach ($entry in $optionCases.GetEnumerator()) {
    $option = [string]$entry.Key
    $caseId = [string]$entry.Value
    $optionStyle = Join-Path $WorkDir "$option.disabled.clang-format"
    $styleText = [System.IO.File]::ReadAllText($StyleFile)
    $enabledSetting = "  ${option}: true"
    if (-not $styleText.Contains($enabledSetting)) {
        throw "Signature style is missing enabled option $option."
    }
    [System.IO.File]::WriteAllText(
        $optionStyle,
        $styleText.Replace($enabledSetting, "  ${option}: false"),
        [System.Text.UTF8Encoding]::new($false)
    )

    $actualPath = Join-Path $WorkDir "$option.disabled.cpp"
    $inputPath = Join-Path $CaseDir "$caseId.input.cpp"
    $targetPath = Join-Path $CaseDir "$caseId.zcf.cpp"
    Copy-Item -LiteralPath $inputPath -Destination $actualPath
    & $FormatterExe "-style=file:$optionStyle" -i $actualPath
    if ($LASTEXITCODE -ne 0) {
        throw "$option disabled-case formatting failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $actualPath) -ceq
        (Get-NormalizedText -Path $targetPath)) {
        throw "$option has no independently observable effect in $caseId."
    }
    Write-Host "PASS $option option isolation"
}

$multiSectionStyle = Join-Path $WorkDir 'multi-section.clang-format'
$multiSectionInput = Join-Path $WorkDir 'multi-section.cpp'
[System.IO.File]::WriteAllText(
    $multiSectionStyle,
    @'
---
Language: Cpp
BasedOnStyle: LLVM
ZCFExtensions:
  BlankLineBeforeReturn: true
---
Language: CSharp
BasedOnStyle: Microsoft
ZCFExtensions:
  BlankLineBeforeReturn: false
'@,
    [System.Text.UTF8Encoding]::new($false)
)
[System.IO.File]::WriteAllText(
    $multiSectionInput,
    "int run() { work(); return 1; }`n",
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$multiSectionStyle" -i $multiSectionInput
if ($LASTEXITCODE -ne 0) {
    throw "Multi-section extension formatting failed with exit code $LASTEXITCODE."
}
if (-not (Get-NormalizedText -Path $multiSectionInput).Contains(
    "work();`n`n  return 1;"
)) {
    throw 'C++ formatting did not select its language-specific extension style.'
}
Write-Host 'PASS language-specific extension style'

$finalizedInput = Join-Path $WorkDir 'finalized.cpp'
[System.IO.File]::WriteAllText(
    $finalizedInput,
    @'
int formatted()
{
    work();
    return 1;
}
// clang-format off
int untouched()
{
    work();
    return 2;
}
// clang-format on
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $finalizedInput
if ($LASTEXITCODE -ne 0) {
    throw "Finalized-region formatting failed with exit code $LASTEXITCODE."
}
$finalizedText = Get-NormalizedText -Path $finalizedInput
if (-not $finalizedText.Contains("work();`n`n    return 1;") -or
    -not $finalizedText.Contains("work();`n    return 2;")) {
    throw 'Enabled extension formatting changed a clang-format-off region.'
}
Write-Host 'PASS finalized region preservation'

$canonicalActual = Join-Path $WorkDir 'canonical.actual.cpp'
Copy-Item -LiteralPath $CanonicalFixture -Destination $canonicalActual
& $FormatterExe "-style=file:$StyleFile" -i $canonicalActual
if ($LASTEXITCODE -ne 0) {
    throw "Canonical signature formatting failed with exit code $LASTEXITCODE."
}
$canonicalFirstHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $canonicalActual).Hash
& $FormatterExe "-style=file:$StyleFile" -i $canonicalActual
if ($LASTEXITCODE -ne 0) {
    throw "Canonical signature second pass failed with exit code $LASTEXITCODE."
}
$canonicalSecondHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $canonicalActual).Hash
if ($canonicalFirstHash -cne $canonicalSecondHash) {
    throw 'Canonical signature output is not byte-identical across two passes.'
}
Write-Host 'PASS canonical signature idempotence'

Write-Host "Validated $($caseIds.Count) signature formatting contracts."

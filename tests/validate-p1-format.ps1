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
    $StyleFile = Join-Path $repoRoot 'tests\cpp-style-examples\wformat-p1.clang-format'
}
if (-not $UpstreamStyleFile) {
    $UpstreamStyleFile = Join-Path $repoRoot 'tests\cpp-style-examples\wformat-target.clang-format'
}
if (-not $CaseDir) {
    $CaseDir = Join-Path $repoRoot 'tests\wformat-style-gaps\cases'
}
if (-not $CanonicalFixture) {
    $CanonicalFixture = Join-Path $repoRoot 'tests\cpp-style-examples\wformat_supported_style_details.cpp'
}
if (-not $WorkDir) {
    $WorkDir = Join-Path $repoRoot 'build\p1-format-tests'
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
    throw "P1 case directory was not found: $CaseDir"
}

$caseIds = @(
    'break-constructor-destructor-specifiers',
    'single-argument-call-compaction',
    'nested-aggregate-brace-expansion',
    'requires-expression-brace-layout',
    'parameter-pack-ellipsis-spacing',
    'annotation-function-pointer-spacing',
    'integer-literal-case-normalization',
    'blank-lines-around-control-statements',
    'cross-directive-macro-alignment'
)
$optionCases = [ordered]@{
    BreakConstructorDestructorSpecifiers = 'break-constructor-destructor-specifiers'
    CompactSingleArgumentCalls = 'single-argument-call-compaction'
    ExpandNestedAggregateBraces = 'nested-aggregate-brace-expansion'
    BreakRequiresExpressionBraces = 'requires-expression-brace-layout'
    SpaceParameterPackEllipses = 'parameter-pack-ellipsis-spacing'
    SpaceAnnotationsAndFunctionPointers = 'annotation-function-pointer-spacing'
    NormalizeIntegerLiteralCase = 'integer-literal-case-normalization'
    BlankLinesAroundControlStatements = 'blank-lines-around-control-statements'
    AlignMacrosAcrossDirectives = 'cross-directive-macro-alignment'
}

if (Test-Path -LiteralPath $WorkDir) {
    Remove-Item -LiteralPath $WorkDir -Recurse -Force
}
[void](New-Item -ItemType Directory -Path $WorkDir)

$p1StyleLines = @([System.IO.File]::ReadAllLines($StyleFile))
$extensionIndex = [Array]::IndexOf($p1StyleLines, 'WFormatExtensions:')
if ($extensionIndex -lt 0) {
    throw 'P1 style does not contain WFormatExtensions.'
}
$upstreamPrefix = ($p1StyleLines[0..($extensionIndex - 1)] -join "`n").TrimEnd()
$upstreamStyle = [System.IO.File]::ReadAllText($UpstreamStyleFile).
    Replace("`r`n", "`n").
    Replace(
        '# Closest pure clang-format 20.1.8 approximation of wformat 0.1.6.',
        '# P0 and P1 extensions of the closest clang-format 20.1.8 approximation.'
    ).
    Replace(
        '# Residual and partially supported behaviors are cataloged by the style-gap audit.',
        '# The upstream option prefix is checked against wformat-target.clang-format.'
    ).
    Replace(
        "AlignConsecutiveAssignments:`n  Enabled: true",
        "AlignConsecutiveAssignments:`n  Enabled: false"
    ).
    Replace("`n...`n", "`n").
    TrimEnd()
if ($upstreamPrefix -cne $upstreamStyle) {
    throw 'P1 style upstream options drifted from wformat-target.clang-format.'
}

$disabledDump = (& $FormatterExe -style=LLVM -dump-config 2>&1) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw "Disabled extension dump-config failed with exit code $LASTEXITCODE."
}
if ($disabledDump.Contains('WFormatExtensions:')) {
    throw 'Disabled dump-config unexpectedly emitted WFormatExtensions.'
}

$enabledDump = (
    & $FormatterExe "-style=file:$StyleFile" -dump-config `
        (Join-Path $CaseDir 'blank-line-before-return.input.cpp') 2>&1
) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw "Enabled extension dump-config failed with exit code $LASTEXITCODE."
}
foreach ($option in @(
    'BreakConstructorDestructorSpecifiers',
    'CompactSingleArgumentCalls',
    'ExpandNestedAggregateBraces',
    'BreakRequiresExpressionBraces',
    'SpaceParameterPackEllipses',
    'SpaceAnnotationsAndFunctionPointers',
    'NormalizeIntegerLiteralCase',
    'BlankLinesAroundControlStatements',
    'AlignMacrosAcrossDirectives'
)) {
    if (-not $enabledDump.Contains("${option}: true")) {
        throw "Enabled dump-config did not preserve $option."
    }
}
if ($enabledDump -notmatch
    '(?s)AlignConsecutiveAssignments:.*?Enabled:\s+false') {
    throw 'P1 style did not disable consecutive assignment alignment.'
}

function Get-NormalizedText {
    param([Parameter(Mandatory = $true)][string]$Path)

    return [System.IO.File]::ReadAllText($Path).
        Replace("`r`n", "`n").
        TrimEnd([char[]]"`n")
}

foreach ($caseId in $caseIds) {
    $inputPath = Join-Path $CaseDir "$caseId.input.cpp"
    $targetSuffix = if ($caseId -ceq 'nested-aggregate-brace-expansion') {
        'reviewed.cpp'
    } else {
        'wformat.cpp'
    }
    $targetPath = Join-Path $CaseDir "$caseId.$targetSuffix"
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
        throw "$caseId output differs from its checked-in WFormat target."
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
        throw "P1 style is missing enabled option $option."
    }
    [System.IO.File]::WriteAllText(
        $optionStyle,
        $styleText.Replace($enabledSetting, "  ${option}: false"),
        [System.Text.UTF8Encoding]::new($false)
    )

    $actualPath = Join-Path $WorkDir "$option.disabled.cpp"
    $inputPath = Join-Path $CaseDir "$caseId.input.cpp"
    $targetSuffix = if ($caseId -ceq 'nested-aggregate-brace-expansion') {
        'reviewed.cpp'
    } else {
        'wformat.cpp'
    }
    $targetPath = Join-Path $CaseDir "$caseId.$targetSuffix"
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
WFormatExtensions:
  NormalizeIntegerLiteralCase: true
---
Language: CSharp
BasedOnStyle: Microsoft
WFormatExtensions:
  NormalizeIntegerLiteralCase: false
'@,
    [System.Text.UTF8Encoding]::new($false)
)
[System.IO.File]::WriteAllText(
    $multiSectionInput,
    "unsigned value = 0Xdeadul;`n",
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$multiSectionStyle" -i $multiSectionInput
if ($LASTEXITCODE -ne 0) {
    throw "Multi-section extension formatting failed with exit code $LASTEXITCODE."
}
if (-not [System.IO.File]::ReadAllText($multiSectionInput).Contains('0xDEADUL')) {
    throw 'C++ formatting did not select its language-specific extension style.'
}
Write-Host 'PASS language-specific extension style'

$finalizedInput = Join-Path $WorkDir 'finalized.cpp'
[System.IO.File]::WriteAllText(
    $finalizedInput,
    @'
unsigned formatted = 0Xdeadul;
// clang-format off
unsigned untouched = 0Xdeadul;
// clang-format on
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $finalizedInput
if ($LASTEXITCODE -ne 0) {
    throw "Finalized-region formatting failed with exit code $LASTEXITCODE."
}
$finalizedText = [System.IO.File]::ReadAllText($finalizedInput)
if (-not $finalizedText.Contains('formatted = 0xDEADUL') -or
    -not $finalizedText.Contains('untouched = 0Xdeadul')) {
    throw 'Enabled extension formatting changed a clang-format-off region.'
}
Write-Host 'PASS finalized region preservation'

$aggregateInteractionInput = Join-Path $WorkDir 'aggregate-integer.cpp'
[System.IO.File]::WriteAllText(
    $aggregateInteractionInput,
    "unsigned values[1][1][1] = {{{0Xdeadul}}};`n",
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $aggregateInteractionInput
if ($LASTEXITCODE -ne 0) {
    throw "Aggregate/integer interaction failed with exit code $LASTEXITCODE."
}
$aggregateInteractionText =
    [System.IO.File]::ReadAllText($aggregateInteractionInput)
if (-not $aggregateInteractionText.Contains('0xDEADUL') -or
    -not $aggregateInteractionText.Contains("`n{")) {
    throw 'Aggregate expansion dropped integer-literal normalization.'
}
Write-Host 'PASS aggregate/integer interaction'

$aggregateTriviaInput = Join-Path $WorkDir 'aggregate-trivia.cpp'
[System.IO.File]::WriteAllText(
    $aggregateTriviaInput,
    "unsigned values[1][1][1] = /* keep */ {{{0Xdeadul}}};`n",
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $aggregateTriviaInput
if ($LASTEXITCODE -ne 0) {
    throw "Aggregate trivia preservation failed with exit code $LASTEXITCODE."
}
$aggregateTriviaText = [System.IO.File]::ReadAllText($aggregateTriviaInput)
if (-not $aggregateTriviaText.Contains('/* keep */') -or
    -not $aggregateTriviaText.Contains('0xDEADUL')) {
    throw 'Aggregate processing deleted trivia or skipped safe token rewriting.'
}
Write-Host 'PASS aggregate trivia preservation'

$inheritanceInput = Join-Path $WorkDir 'ordinary-inheritance.cpp'
[System.IO.File]::WriteAllText(
    $inheritanceInput,
    "struct Derived : Base {};`n",
    [System.Text.UTF8Encoding]::new($false)
)
$inheritanceStyle =
    '{BasedOnStyle: LLVM, WFormatExtensions: {SpaceParameterPackEllipses: true}}'
& $FormatterExe "-style=$inheritanceStyle" -i $inheritanceInput
if ($LASTEXITCODE -ne 0) {
    throw "Ordinary inheritance isolation failed with exit code $LASTEXITCODE."
}
if ((Get-NormalizedText -Path $inheritanceInput) -cne
    'struct Derived : Base {};') {
    throw 'Parameter-pack spacing changed ordinary inheritance layout.'
}
Write-Host 'PASS ordinary inheritance isolation'

$canonicalActual = Join-Path $WorkDir 'canonical.actual.cpp'
Copy-Item -LiteralPath $CanonicalFixture -Destination $canonicalActual
& $FormatterExe "-style=file:$StyleFile" -i $canonicalActual
if ($LASTEXITCODE -ne 0) {
    throw "Canonical P1 formatting failed with exit code $LASTEXITCODE."
}
$canonicalFirstHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $canonicalActual).Hash
& $FormatterExe "-style=file:$StyleFile" -i $canonicalActual
if ($LASTEXITCODE -ne 0) {
    throw "Canonical P1 second pass failed with exit code $LASTEXITCODE."
}
$canonicalSecondHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $canonicalActual).Hash
if ($canonicalFirstHash -cne $canonicalSecondHash) {
    throw 'Canonical P1 output is not byte-identical across two passes.'
}
Write-Host 'PASS canonical P1 idempotence'

Write-Host "Validated $($caseIds.Count) P1 formatting contracts."

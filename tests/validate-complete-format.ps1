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
    $StyleFile = Join-Path $repoRoot 'tests\cpp-style-examples\zcf-complete.clang-format'
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
    $WorkDir = Join-Path $repoRoot 'build\complete-format-tests'
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
    throw "Complete-policy case directory was not found: $CaseDir"
}

$caseIds = @(
    'qualified-function-name-continuation-indent',
    'ref-qualifier-requires-layout',
    'compound-requirement-arrow-spacing',
    'space-after-explicit',
    'blank-lines-around-labels',
    'attributed-label-indentation',
    'preprocessor-closing-comment-alignment-boundary'
)
$optionCases = [ordered]@{
    DeindentQualifiedFunctionNames = 'qualified-function-name-continuation-indent'
    BreakRefQualifierRequires = 'ref-qualifier-requires-layout'
    SpaceAfterParenthesizedSpecifiers = 'space-after-explicit'
    BlankLinesAroundLabels = 'blank-lines-around-labels'
    IndentAttributedLabels = 'attributed-label-indentation'
    SeparateClosingDirectiveComments = 'preprocessor-closing-comment-alignment-boundary'
}

if (Test-Path -LiteralPath $WorkDir) {
    Remove-Item -LiteralPath $WorkDir -Recurse -Force
}
[void](New-Item -ItemType Directory -Path $WorkDir)

$completeStyleLines = @([System.IO.File]::ReadAllLines($StyleFile))
$extensionIndex = [Array]::IndexOf($completeStyleLines, 'ZCFExtensions:')
if ($extensionIndex -lt 0) {
    throw 'Complete style does not contain ZCFExtensions.'
}
$upstreamPrefix = ($completeStyleLines[0..($extensionIndex - 1)] -join "`n").TrimEnd()
$upstreamStyle = [System.IO.File]::ReadAllText($UpstreamStyleFile).
    Replace("`r`n", "`n").
    Replace(
        '# Native clang-format 20.1.8 baseline for the reviewed ZCF C++ policy.',
        "# Complete reviewed extension policy for the closest clang-format 20.1.8`n# approximation."
    ).
    Replace(
        '# Extension-specific differences are covered by focused contract fixtures.',
        '# The upstream option prefix is checked against zcf-target.clang-format.'
    ).
    Replace(
        "AlignConsecutiveAssignments:`n  Enabled: true",
        "AlignConsecutiveAssignments:`n  Enabled: false"
    ).
    Replace("`n...`n", "`n").
    TrimEnd()
if ($upstreamPrefix -cne $upstreamStyle) {
    throw 'Complete style upstream options drifted from zcf-target.clang-format.'
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
    'DeindentQualifiedFunctionNames',
    'BreakRefQualifierRequires',
    'SpaceAfterParenthesizedSpecifiers',
    'BlankLinesAroundLabels',
    'IndentAttributedLabels',
    'SeparateClosingDirectiveComments',
    'BodyDrivenLambdaExpansion',
    'VerticalTernaryExpressions',
    'SeparateSwitchCaseBlocks',
    'ScopeStyleNestedTemplates'
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
    $targetSuffix = if ($caseId -ceq 'compound-requirement-arrow-spacing') {
        'reviewed.cpp'
    } else {
        'zcf.cpp'
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

$regressionContracts = [ordered]@{
    'attributed-label-namespace-indentation' = "file:$StyleFile"
    'qualified-name-nested-continuation' = "file:$StyleFile"
    'adjacent-string-literal-arguments' = "file:$StyleFile"
    'return-blank-line-contexts' =
        '{BasedOnStyle: LLVM, ZCFExtensions: {BlankLineBeforeReturn: true}}'
    'control-statement-trailing-comments' =
        '{BasedOnStyle: LLVM, ZCFExtensions: {BlankLinesAroundControlStatements: true}}'
    'nested-aggregate-standalone-layout' =
        '{BasedOnStyle: LLVM, ZCFExtensions: {ExpandNestedAggregateBraces: true}}'
    'nested-aggregate-tab-indentation' =
        '{BasedOnStyle: LLVM, IndentWidth: 4, TabWidth: 4, UseTab: ForIndentation, ZCFExtensions: {ExpandNestedAggregateBraces: true}}'
    'lambda-chain-is-not-aggregate' = "file:$StyleFile"
}
foreach ($contract in $regressionContracts.GetEnumerator()) {
    $caseId = [string]$contract.Key
    $style = [string]$contract.Value
    $actualPath = Join-Path $WorkDir "$caseId.actual.cpp"
    Copy-Item -LiteralPath (Join-Path $CaseDir "$caseId.input.cpp") `
        -Destination $actualPath
    & $FormatterExe "-style=$style" -i $actualPath
    if ($LASTEXITCODE -ne 0) {
        throw "$caseId formatting failed with exit code $LASTEXITCODE."
    }
    $firstPass = Get-NormalizedText -Path $actualPath
    if ($firstPass -cne (Get-NormalizedText -Path (Join-Path $CaseDir "$caseId.zcf.cpp"))) {
        throw "$caseId output differs from its checked-in regression target."
    }
    & $FormatterExe "-style=$style" -i $actualPath
    if ($LASTEXITCODE -ne 0 -or (Get-NormalizedText -Path $actualPath) -cne $firstPass) {
        throw "$caseId is not idempotent across two formatter passes."
    }
    Write-Host "PASS $caseId regression"
}

$nativeArrowInput =
    Join-Path $CaseDir 'compound-requirement-arrow-spacing.input.cpp'
$nativeArrowActual = Join-Path $WorkDir 'native-arrow-spacing.cpp'
Copy-Item -LiteralPath $nativeArrowInput -Destination $nativeArrowActual
& $FormatterExe -style=LLVM -i $nativeArrowActual
if ($LASTEXITCODE -ne 0) {
    throw "Native arrow formatting failed with exit code $LASTEXITCODE."
}
$nativeArrowText = Get-NormalizedText -Path $nativeArrowActual
foreach ($fragment in @(
    'Box(const char *) -> Box<const char *>;',
    '{ value } -> std::same_as<T>;',
    'auto transform() -> int;'
)) {
    if (-not $nativeArrowText.Contains($fragment)) {
        throw "LLVM style did not retain native arrow spacing in '$fragment'."
    }
}
Write-Host 'PASS native LLVM arrow spacing'

$presetStyles = [ordered]@{
    'BasedOnStyle' = '{BasedOnStyle: ZCF}'
    'NamedStyle' = 'ZCF'
    'StyleFile' = "file:$(Join-Path $repoRoot 'tests\cpp-style-examples\zcf.clang-format')"
}
$progressiveInput =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-progressive-calls.input.cpp'
$progressiveExpected =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-progressive-calls.expected.cpp'
$binaryInput =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-binary-operations.input.cpp'
$binaryExpected =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-binary-operations.expected.cpp'
$arithmeticInput =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-progressive-arithmetic.input.cpp'
$arithmeticExpected =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-progressive-arithmetic.expected.cpp'
$compactOperandsInput =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-compact-two-operands.input.cpp'
$compactOperandsExpected =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-compact-two-operands.expected.cpp'
$matrixInput =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-matrix-alignment.input.cpp'
$matrixExpected =
    Join-Path $repoRoot 'tests\cpp-style-examples\zcf-preset-matrix-alignment.expected.cpp'
$presetRuleContracts = @(
    @{
        Option = 'BodyDrivenLambdaExpansion'
        Case = 'body-driven-lambda-expansion'
        Input = Join-Path $CaseDir 'body-driven-lambda-expansion.input.cpp'
        Expected = Join-Path $CaseDir 'body-driven-lambda-expansion.zcf.cpp'
    },
    @{
        Option = 'VerticalTernaryExpressions'
        Case = 'vertical-ternary-expressions'
        Input = Join-Path $CaseDir 'vertical-ternary-expressions.input.cpp'
        Expected = Join-Path $CaseDir 'vertical-ternary-expressions.zcf.cpp'
    },
    @{
        Option = 'SeparateSwitchCaseBlocks'
        Case = 'switch-case-block-separation'
        Input = Join-Path $CaseDir 'switch-case-block-separation.input.cpp'
        Expected = Join-Path $CaseDir 'switch-case-block-separation.zcf.cpp'
    },
    @{
        Option = 'ScopeStyleNestedTemplates'
        Case = 'scope-style-nested-templates'
        Input = Join-Path $CaseDir 'scope-style-nested-templates.input.cpp'
        Expected = Join-Path $CaseDir 'scope-style-nested-templates.zcf.cpp'
    }
)
foreach ($preset in $presetStyles.GetEnumerator()) {
    $actualPath = Join-Path $WorkDir "$($preset.Key).progressive-calls.cpp"
    Copy-Item -LiteralPath $progressiveInput -Destination $actualPath
    & $FormatterExe "-style=$($preset.Value)" -i $actualPath
    if ($LASTEXITCODE -ne 0) {
        throw "$($preset.Key) progressive formatting failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $actualPath) -cne
        (Get-NormalizedText -Path $progressiveExpected)) {
        throw "$($preset.Key) differs from the progressive-call target."
    }
    $firstPass = Get-NormalizedText -Path $actualPath
    & $FormatterExe "-style=$($preset.Value)" -i $actualPath
    if ($LASTEXITCODE -ne 0 -or
        (Get-NormalizedText -Path $actualPath) -cne $firstPass) {
        throw "$($preset.Key) progressive formatting is not idempotent."
    }
    Write-Host "PASS $($preset.Key) progressive-call preset"

    $binaryActual = Join-Path $WorkDir "$($preset.Key).binary-operations.cpp"
    Copy-Item -LiteralPath $binaryInput -Destination $binaryActual
    & $FormatterExe "-style=$($preset.Value)" -i $binaryActual
    if ($LASTEXITCODE -ne 0) {
        throw "$($preset.Key) binary formatting failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $binaryActual) -cne
        (Get-NormalizedText -Path $binaryExpected)) {
        throw "$($preset.Key) differs from the binary-operation target."
    }
    Write-Host "PASS $($preset.Key) binary-operation preset"

    $arithmeticActual =
        Join-Path $WorkDir "$($preset.Key).progressive-arithmetic.cpp"
    Copy-Item -LiteralPath $arithmeticInput -Destination $arithmeticActual
    & $FormatterExe "-style=$($preset.Value)" -i $arithmeticActual
    if ($LASTEXITCODE -ne 0) {
        throw "$($preset.Key) arithmetic formatting failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $arithmeticActual) -cne
        (Get-NormalizedText -Path $arithmeticExpected)) {
        throw "$($preset.Key) differs from the progressive-arithmetic target."
    }
    $arithmeticFirstPass = Get-NormalizedText -Path $arithmeticActual
    & $FormatterExe "-style=$($preset.Value)" -i $arithmeticActual
    if ($LASTEXITCODE -ne 0 -or
        (Get-NormalizedText -Path $arithmeticActual) -cne
        $arithmeticFirstPass) {
        throw "$($preset.Key) progressive arithmetic is not idempotent."
    }
    Write-Host "PASS $($preset.Key) progressive-arithmetic preset"

    $compactOperandsActual =
        Join-Path $WorkDir "$($preset.Key).compact-two-operands.cpp"
    Copy-Item -LiteralPath $compactOperandsInput `
        -Destination $compactOperandsActual
    & $FormatterExe "-style=$($preset.Value)" -i $compactOperandsActual
    if ($LASTEXITCODE -ne 0) {
        throw "$($preset.Key) operand compaction failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $compactOperandsActual) -cne
        (Get-NormalizedText -Path $compactOperandsExpected)) {
        throw "$($preset.Key) differs from the two-operand compaction target."
    }
    Write-Host "PASS $($preset.Key) compact-two-operands preset"

    $matrixActual = Join-Path $WorkDir "$($preset.Key).matrix-alignment.cpp"
    Copy-Item -LiteralPath $matrixInput -Destination $matrixActual
    & $FormatterExe "-style=$($preset.Value)" -i $matrixActual
    if ($LASTEXITCODE -ne 0) {
        throw "$($preset.Key) matrix formatting failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $matrixActual) -cne
        (Get-NormalizedText -Path $matrixExpected)) {
        throw "$($preset.Key) differs from the right-aligned matrix target."
    }
    Write-Host "PASS $($preset.Key) right-aligned matrix preset"

    foreach ($contract in $presetRuleContracts) {
        $caseId = [string]$contract.Case
        $actualPath = Join-Path $WorkDir "$($preset.Key).$caseId.cpp"
        Copy-Item -LiteralPath ([string]$contract.Input) -Destination $actualPath
        & $FormatterExe "-style=$($preset.Value)" -i $actualPath
        if ($LASTEXITCODE -ne 0) {
            throw "$($preset.Key) $caseId formatting failed with exit code $LASTEXITCODE."
        }
        if ((Get-NormalizedText -Path $actualPath) -cne
            (Get-NormalizedText -Path ([string]$contract.Expected))) {
            throw "$($preset.Key) differs from the $caseId target."
        }
        $firstPass = Get-NormalizedText -Path $actualPath
        & $FormatterExe "-style=$($preset.Value)" -i $actualPath
        if ($LASTEXITCODE -ne 0 -or
            (Get-NormalizedText -Path $actualPath) -cne $firstPass) {
            throw "$($preset.Key) $caseId formatting is not idempotent."
        }
        Write-Host "PASS $($preset.Key) $caseId preset"
    }
}

$presetCanonical = Join-Path $WorkDir 'preset-canonical.cpp'
Copy-Item -LiteralPath $CanonicalFixture -Destination $presetCanonical
& $FormatterExe -style=ZCF -i $presetCanonical
if ($LASTEXITCODE -ne 0) {
    throw "ZCF preset canonical formatting failed with exit code $LASTEXITCODE."
}
$presetCanonicalFirstHash =
    (Get-FileHash -Algorithm SHA256 -LiteralPath $presetCanonical).Hash
& $FormatterExe -style=ZCF -i $presetCanonical
if ($LASTEXITCODE -ne 0) {
    throw "ZCF preset canonical second pass failed with exit code $LASTEXITCODE."
}
$presetCanonicalSecondHash =
    (Get-FileHash -Algorithm SHA256 -LiteralPath $presetCanonical).Hash
if ($presetCanonicalFirstHash -cne $presetCanonicalSecondHash) {
    throw 'ZCF preset canonical output is not byte-identical across two passes.'
}
Write-Host 'PASS ZCF preset canonical idempotence'

$presetDump = (
    & $FormatterExe '-style={BasedOnStyle: ZCF}' -dump-config `
        (Join-Path $CaseDir 'blank-line-before-return.input.cpp') 2>&1
) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw "ZCF preset dump-config failed with exit code $LASTEXITCODE."
}
$enabledPresetOptions = @(
    'ArgumentIndentedClosingParentheses',
    'ForceMultilineFunctionSignatures',
    'ContextSensitiveBracedInitializers',
    'BlankLineBeforeReturn',
    'BreakConstructorDestructorSpecifiers',
    'CompactSingleArgumentCalls',
    'ExpandNestedAggregateBraces',
    'BreakRequiresExpressionBraces',
    'SpaceParameterPackEllipses',
    'SpaceAnnotationsAndFunctionPointers',
    'NormalizeIntegerLiteralCase',
    'BlankLinesAroundControlStatements',
    'AlignMacrosAcrossDirectives',
    'DeindentQualifiedFunctionNames',
    'BreakRefQualifierRequires',
    'SpaceAfterParenthesizedSpecifiers',
    'BlankLinesAroundLabels',
    'IndentAttributedLabels',
    'SeparateClosingDirectiveComments',
    'ProgressiveCallExpansion',
    'ProgressiveArithmeticExpansion',
    'CompactTwoOperandExpressions',
    'BodyDrivenLambdaExpansion',
    'VerticalTernaryExpressions',
    'SeparateSwitchCaseBlocks',
    'ScopeStyleNestedTemplates'
)
foreach ($option in $enabledPresetOptions) {
    if (-not $presetDump.Contains("${option}: true")) {
        throw "ZCF preset did not enable $option."
    }
}
foreach ($setting in @(
    'ColumnLimit:     140',
    'AlignArrayOfStructures: Right',
    'AlignOperands:   DontAlign',
    'BreakBeforeBinaryOperators: All',
    'BreakBinaryOperations: RespectPrecedence',
    'PenaltyBreakAssignment: 1000'
)) {
    if (-not $presetDump.Contains($setting)) {
        throw "ZCF preset is missing '$setting'."
    }
}
if ($presetDump -notmatch
    '(?s)AlignConsecutiveAssignments:.*?Enabled:\s+false') {
    throw 'ZCF preset did not disable consecutive assignment alignment.'
}
Write-Host 'PASS ZCF preset policy settings'

$presetDumpStyle = Join-Path $WorkDir 'preset-dump.clang-format'
$presetDumpActual = Join-Path $WorkDir 'preset-dump-progressive-calls.cpp'
[System.IO.File]::WriteAllText(
    $presetDumpStyle,
    $presetDump,
    [System.Text.UTF8Encoding]::new($false)
)
Copy-Item -LiteralPath $progressiveInput -Destination $presetDumpActual
& $FormatterExe "-style=file:$presetDumpStyle" -i $presetDumpActual
if ($LASTEXITCODE -ne 0) {
    throw "ZCF dump-config round trip failed with exit code $LASTEXITCODE."
}
if ((Get-NormalizedText -Path $presetDumpActual) -cne
    (Get-NormalizedText -Path $progressiveExpected)) {
    throw 'ZCF dump-config lost progressive-call behavior.'
}
Write-Host 'PASS ZCF dump-config round trip'

$presetOptionContracts = @(
    @{
        Option = 'ProgressiveArithmeticExpansion'
        Input = $arithmeticInput
        Expected = $arithmeticExpected
    },
    @{
        Option = 'CompactTwoOperandExpressions'
        Input = $compactOperandsInput
        Expected = $compactOperandsExpected
    }
) + $presetRuleContracts
foreach ($contract in $presetOptionContracts) {
    $option = [string]$contract.Option
    $disabledStyle = Join-Path $WorkDir "$option.disabled-preset.clang-format"
    $disabledActual = Join-Path $WorkDir "$option.disabled-preset.cpp"
    [System.IO.File]::WriteAllText(
        $disabledStyle,
        $presetDump.Replace("${option}: true", "${option}: false"),
        [System.Text.UTF8Encoding]::new($false)
    )
    Copy-Item -LiteralPath ([string]$contract.Input) -Destination $disabledActual
    & $FormatterExe "-style=file:$disabledStyle" -i $disabledActual
    if ($LASTEXITCODE -ne 0) {
        throw "$option disabled formatting failed with exit code $LASTEXITCODE."
    }
    if ((Get-NormalizedText -Path $disabledActual) -ceq
        (Get-NormalizedText -Path ([string]$contract.Expected))) {
        throw "$option has no independently observable effect."
    }
    Write-Host "PASS $option reviewed option isolation"
}

$layeredCompactionInput = Join-Path $WorkDir 'layered-compaction.cpp'
[System.IO.File]::WriteAllText(
    $layeredCompactionInput,
    @'
const auto layered = sampleIndex
    * stride
    + channelOffset;
'@,
    [System.Text.UTF8Encoding]::new($false)
)
$layeredCompactionStyle = @'
{BasedOnStyle: LLVM, ColumnLimit: 60, BreakBeforeBinaryOperators: All,
 AlignOperands: DontAlign,
 ZCFExtensions: {ProgressiveArithmeticExpansion: true,
                    CompactTwoOperandExpressions: true}}
'@.Replace("`r", '').Replace("`n", ' ')
& $FormatterExe "-style=$layeredCompactionStyle" -i $layeredCompactionInput
if ($LASTEXITCODE -ne 0) {
    throw "Layered compaction safety failed with exit code $LASTEXITCODE."
}
$layeredCompactionText = Get-NormalizedText -Path $layeredCompactionInput
if (-not $layeredCompactionText.Contains("layered =`n    sampleIndex") -or
    -not $layeredCompactionText.Contains("`n    * stride") -or
    -not $layeredCompactionText.Contains("`n    + channelOffset")) {
    throw 'Layered arithmetic did not preserve its internal layer after =.'
}
Write-Host 'PASS layered assignment break after ='

$longTwoOperandInput = Join-Path $WorkDir 'long-two-operands.cpp'
[System.IO.File]::WriteAllText(
    $longTwoOperandInput,
    @'
const auto delta = measuredValueWithAVeryLongName - targetValueWithAVeryLongName;
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe `
    '-style={BasedOnStyle: LLVM, ColumnLimit: 40, IndentWidth: 4, ContinuationIndentWidth: 4, ZCFExtensions: {ProgressiveArithmeticExpansion: true, CompactTwoOperandExpressions: true}}' `
    -i $longTwoOperandInput
if ($LASTEXITCODE -ne 0) {
    throw "Long two-operand formatting failed with exit code $LASTEXITCODE."
}
$longTwoOperandText = Get-NormalizedText -Path $longTwoOperandInput
if (-not $longTwoOperandText.Contains(
    "measuredValueWithAVeryLongName`n    - targetValueWithAVeryLongName"
)) {
    throw 'A two-operand expression that exceeded ColumnLimit stayed compact.'
}
Write-Host 'PASS long two-operand expansion'

$assignmentBreakInput = Join-Path $WorkDir 'expanded-assignment.cpp'
[System.IO.File]::WriteAllText(
    $assignmentBreakInput,
    @'
const int total = baseValue + shippingFee + serviceCharge + taxAmount;
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe `
    '-style={BasedOnStyle: ZCF, ColumnLimit: 60}' `
    -i $assignmentBreakInput
if ($LASTEXITCODE -ne 0) {
    throw "Expanded assignment formatting failed with exit code $LASTEXITCODE."
}
$assignmentBreakText = Get-NormalizedText -Path $assignmentBreakInput
$assignmentBreakExpected = @'
const int total =
    baseValue
    + shippingFee
    + serviceCharge
    + taxAmount;
'@.Replace("`r`n", "`n").TrimEnd([char[]]"`n")
if ($assignmentBreakText -cne $assignmentBreakExpected) {
    throw 'Expanded arithmetic assignment did not break immediately after =.'
}
Write-Host 'PASS expanded assignment break after ='

$commentArithmeticInput = Join-Path $WorkDir 'comment-arithmetic.cpp'
$commentArithmeticText = @'
const auto total = firstValue + /* keep */ secondValue + thirdValue;
'@
[System.IO.File]::WriteAllText(
    $commentArithmeticInput,
    $commentArithmeticText,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe `
    '-style={BasedOnStyle: LLVM, ColumnLimit: 40, ZCFExtensions: {ProgressiveArithmeticExpansion: true}}' `
    -i $commentArithmeticInput
if ($LASTEXITCODE -ne 0) {
    throw "Comment-bearing arithmetic safety failed with exit code $LASTEXITCODE."
}
if (-not (Get-NormalizedText -Path $commentArithmeticInput).Contains(
    '/* keep */'
)) {
    throw 'Progressive arithmetic expansion deleted an inline comment.'
}
Write-Host 'PASS comment-bearing arithmetic safety'

$overrideInput = Join-Path $WorkDir 'preset-override.cpp'
[System.IO.File]::WriteAllText(
    $overrideInput,
    "int run() { work(); return 1; }`n",
    [System.Text.UTF8Encoding]::new($false)
)
$overrideStyle =
    '{BasedOnStyle: ZCF, ZCFExtensions: {BlankLineBeforeReturn: false}}'
& $FormatterExe "-style=$overrideStyle" -i $overrideInput
if ($LASTEXITCODE -ne 0) {
    throw "ZCF option override failed with exit code $LASTEXITCODE."
}
if ((Get-NormalizedText -Path $overrideInput).Contains(
    "work();`n`n    return 1;"
)) {
    throw 'ZCF option override did not disable return separation.'
}
Write-Host 'PASS ZCF preset option override'

$presetResetStyle = Join-Path $WorkDir 'preset-language-reset.clang-format'
[System.IO.File]::WriteAllText(
    $presetResetStyle,
    @'
---
BasedOnStyle: ZCF
---
Language: Cpp
BasedOnStyle: LLVM
'@,
    [System.Text.UTF8Encoding]::new($false)
)
$presetResetDump = (
    & $FormatterExe "-style=file:$presetResetStyle" -dump-config `
        (Join-Path $CaseDir 'blank-line-before-return.input.cpp') 2>&1
) -join "`n"
if ($LASTEXITCODE -ne 0) {
    throw "ZCF language reset failed with exit code $LASTEXITCODE."
}
if ($presetResetDump.Contains('ZCFExtensions:')) {
    throw 'BasedOnStyle: LLVM did not reset inherited ZCF extensions.'
}
Write-Host 'PASS ZCF preset language reset'

$nonCppSource = @'
class Sample
{
    int Run()
    {
        Work();
        return 1;
    }
}
'@
$nonCppBaseline = Join-Path $WorkDir 'non-cpp-baseline.cs'
$nonCppExtension = Join-Path $WorkDir 'non-cpp-extension.cs'
[System.IO.File]::WriteAllText(
    $nonCppBaseline,
    $nonCppSource,
    [System.Text.UTF8Encoding]::new($false)
)
[System.IO.File]::WriteAllText(
    $nonCppExtension,
    $nonCppSource,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe -style=Microsoft -i $nonCppBaseline
if ($LASTEXITCODE -ne 0) {
    throw "Non-C++ baseline formatting failed with exit code $LASTEXITCODE."
}
$nonCppExtensionStyle =
    '{BasedOnStyle: Microsoft, ZCFExtensions: {BlankLineBeforeReturn: true}}'
& $FormatterExe "-style=$nonCppExtensionStyle" -i $nonCppExtension
if ($LASTEXITCODE -ne 0) {
    throw "Non-C++ extension formatting failed with exit code $LASTEXITCODE."
}
if ((Get-NormalizedText -Path $nonCppExtension) -cne
    (Get-NormalizedText -Path $nonCppBaseline)) {
    throw 'ZCF extensions changed non-C++ formatting.'
}
Write-Host 'PASS non-C++ extension isolation'

$nonCppZCF = Join-Path $WorkDir 'non-cpp-zcf.cs'
$nonCppLLVM = Join-Path $WorkDir 'non-cpp-llvm.cs'
[System.IO.File]::WriteAllText(
    $nonCppZCF,
    $nonCppSource,
    [System.Text.UTF8Encoding]::new($false)
)
[System.IO.File]::WriteAllText(
    $nonCppLLVM,
    $nonCppSource,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe -style=ZCF -i $nonCppZCF
if ($LASTEXITCODE -ne 0) {
    throw "Non-C++ ZCF fallback failed with exit code $LASTEXITCODE."
}
& $FormatterExe -style=LLVM -i $nonCppLLVM
if ($LASTEXITCODE -ne 0) {
    throw "Non-C++ LLVM formatting failed with exit code $LASTEXITCODE."
}
if ((Get-NormalizedText -Path $nonCppZCF) -cne
    (Get-NormalizedText -Path $nonCppLLVM)) {
    throw 'Non-C++ ZCF did not fall back to LLVM with extensions disabled.'
}
Write-Host 'PASS non-C++ ZCF fallback'

foreach ($entry in $optionCases.GetEnumerator()) {
    $option = [string]$entry.Key
    $caseId = [string]$entry.Value
    $optionStyle = Join-Path $WorkDir "$option.disabled.clang-format"
    $styleText = [System.IO.File]::ReadAllText($StyleFile)
    if ($option -ceq 'BlankLinesAroundLabels') {
        $styleText = $styleText.
            Replace('  BlankLineBeforeReturn: true', '  BlankLineBeforeReturn: false').
            Replace(
                '  BlankLinesAroundControlStatements: true',
                '  BlankLinesAroundControlStatements: false'
            )
        $enabledStyle = Join-Path $WorkDir "$option.enabled.clang-format"
        $enabledActual = Join-Path $WorkDir "$option.enabled.cpp"
        [System.IO.File]::WriteAllText(
            $enabledStyle,
            $styleText,
            [System.Text.UTF8Encoding]::new($false)
        )
        Copy-Item -LiteralPath (Join-Path $CaseDir "$caseId.input.cpp") `
            -Destination $enabledActual
        & $FormatterExe "-style=file:$enabledStyle" -i $enabledActual
        if ($LASTEXITCODE -ne 0) {
            throw "$option isolated formatting failed with exit code $LASTEXITCODE."
        }
        if ((Get-NormalizedText -Path $enabledActual) -cne
            (Get-NormalizedText -Path (Join-Path $CaseDir "$caseId.zcf.cpp"))) {
            throw "$option does not independently reproduce $caseId."
        }
    }
    $enabledSetting = "  ${option}: true"
    if (-not $styleText.Contains($enabledSetting)) {
        throw "Complete style is missing enabled option $option."
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
  SpaceAfterParenthesizedSpecifiers: true
---
Language: CSharp
BasedOnStyle: Microsoft
ZCFExtensions:
  SpaceAfterParenthesizedSpecifiers: false
'@,
    [System.Text.UTF8Encoding]::new($false)
)
[System.IO.File]::WriteAllText(
    $multiSectionInput,
    "struct Flag { explicit(false) Flag(); };`n",
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$multiSectionStyle" -i $multiSectionInput
if ($LASTEXITCODE -ne 0) {
    throw "Multi-section extension formatting failed with exit code $LASTEXITCODE."
}
if (-not [System.IO.File]::ReadAllText($multiSectionInput).Contains('explicit (false)')) {
    throw 'C++ formatting did not select its language-specific extension style.'
}
Write-Host 'PASS language-specific extension style'

$finalizedInput = Join-Path $WorkDir 'finalized.cpp'
[System.IO.File]::WriteAllText(
    $finalizedInput,
    @'
struct Flag
{
    explicit(false) Flag();
// clang-format off
    explicit(false) Flag(int);
// clang-format on
};
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $finalizedInput
if ($LASTEXITCODE -ne 0) {
    throw "Finalized-region formatting failed with exit code $LASTEXITCODE."
}
$finalizedText = [System.IO.File]::ReadAllText($finalizedInput)
if (-not $finalizedText.Contains('explicit (false) Flag()') -or
    -not $finalizedText.Contains('explicit(false) Flag(int)')) {
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
    '{BasedOnStyle: LLVM, ZCFExtensions: {SpaceParameterPackEllipses: true}}'
& $FormatterExe "-style=$inheritanceStyle" -i $inheritanceInput
if ($LASTEXITCODE -ne 0) {
    throw "Ordinary inheritance isolation failed with exit code $LASTEXITCODE."
}
if ((Get-NormalizedText -Path $inheritanceInput) -cne
    'struct Derived : Base {};') {
    throw 'Parameter-pack spacing changed ordinary inheritance layout.'
}
Write-Host 'PASS ordinary inheritance isolation'

$deleteExpressionInput = Join-Path $WorkDir 'delete-expression.cpp'
[System.IO.File]::WriteAllText(
    $deleteExpressionInput,
    @'
void cleanup(int* value)
{
    delete value;
    use(value);
}
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $deleteExpressionInput
if ($LASTEXITCODE -ne 0) {
    throw "Delete-expression isolation failed with exit code $LASTEXITCODE."
}
if ((Get-NormalizedText -Path $deleteExpressionInput).Contains(
    "delete value;`n`n"
)) {
    throw 'Declaration-specifier spacing changed an ordinary delete expression.'
}
Write-Host 'PASS delete-expression isolation'

$qualifiedCallInput = Join-Path $WorkDir 'qualified-call.cpp'
[System.IO.File]::WriteAllText(
    $qualifiedCallInput,
    @'
void invoke()
{
    foo:: // qualifier
    bar(
        value
        );
}
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $qualifiedCallInput
if ($LASTEXITCODE -ne 0) {
    throw "Qualified-call isolation failed with exit code $LASTEXITCODE."
}
$qualifiedCallText = [System.IO.File]::ReadAllText($qualifiedCallInput)
if (-not $qualifiedCallText.Contains("    foo::") -or
    -not $qualifiedCallText.Contains("    bar(")) {
    throw 'Qualified function-name policy deindented a qualified call.'
}
Write-Host 'PASS qualified-call isolation'

$topLevelQualifiedCallInput = Join-Path $WorkDir 'top-level-qualified-call.cpp'
[System.IO.File]::WriteAllText(
    $topLevelQualifiedCallInput,
    @'
auto value =
    foo:: // qualifier
    bar(
        argument
        );
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $topLevelQualifiedCallInput
if ($LASTEXITCODE -ne 0) {
    throw "Top-level qualified-call isolation failed with exit code $LASTEXITCODE."
}
$topLevelQualifiedCallText =
    [System.IO.File]::ReadAllText($topLevelQualifiedCallInput)
if (-not $topLevelQualifiedCallText.Contains("    foo::") -or
    -not $topLevelQualifiedCallText.Contains("    bar(")) {
    throw 'Qualified function-name policy deindented a top-level call.'
}
Write-Host 'PASS top-level qualified-call isolation'

$attributedLabelContextInput = Join-Path $WorkDir 'attributed-label-context.cpp'
[System.IO.File]::WriteAllText(
    $attributedLabelContextInput,
    @'
int run()
{
start:
[[maybe_unused]] done:
    return 1;
}
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $attributedLabelContextInput
if ($LASTEXITCODE -ne 0) {
    throw "Attributed-label context failed with exit code $LASTEXITCODE."
}
if (-not [System.IO.File]::ReadAllText($attributedLabelContextInput).Contains(
    "    [[maybe_unused]] done:"
)) {
    throw 'Attributed-label indentation inherited an ordinary label indent.'
}
Write-Host 'PASS attributed-label context indentation'

$noexceptRequiresInput = Join-Path $WorkDir 'ref-noexcept-requires.cpp'
[System.IO.File]::WriteAllText(
    $noexceptRequiresInput,
    @'
template <typename T>
struct Value
{
    auto get() & noexcept requires(sizeof(T) > 0)
    {
        return T{};
    }
};
'@,
    [System.Text.UTF8Encoding]::new($false)
)
& $FormatterExe "-style=file:$StyleFile" -i $noexceptRequiresInput
if ($LASTEXITCODE -ne 0) {
    throw "Ref/noexcept/requires formatting failed with exit code $LASTEXITCODE."
}
$noexceptRequiresText = [System.IO.File]::ReadAllText($noexceptRequiresInput)
if (-not $noexceptRequiresText.Contains(
    "& noexcept requires(sizeof(T) > 0)"
)) {
    throw 'Ref-qualifier policy split noexcept from its trailing requires clause.'
}
Write-Host 'PASS ref/noexcept/requires grouping'

$canonicalActual = Join-Path $WorkDir 'canonical.actual.cpp'
Copy-Item -LiteralPath $CanonicalFixture -Destination $canonicalActual
& $FormatterExe "-style=file:$StyleFile" -i $canonicalActual
if ($LASTEXITCODE -ne 0) {
    throw "Canonical complete-policy formatting failed with exit code $LASTEXITCODE."
}
$canonicalFirstHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $canonicalActual).Hash
& $FormatterExe "-style=file:$StyleFile" -i $canonicalActual
if ($LASTEXITCODE -ne 0) {
    throw "Canonical complete-policy second pass failed with exit code $LASTEXITCODE."
}
$canonicalSecondHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $canonicalActual).Hash
if ($canonicalFirstHash -cne $canonicalSecondHash) {
    throw 'Canonical complete-policy output is not byte-identical across two passes.'
}
Write-Host 'PASS canonical complete-policy idempotence'

Write-Host "Validated 6 specialized extension contracts, 4 reviewed preset contracts, $($regressionContracts.Count) regression contracts, and 1 reviewed native contract."

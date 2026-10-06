#requires -Version 7.2

[CmdletBinding()]
param(
    [string]$Manifest = '',
    [string]$WorkDir = '',
    [string]$ZcfExe = '',
    [string]$ReviewedStyleFile = '',
    [ValidateRange(1, 3600)][int]$TimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $Manifest) {
    $Manifest = Join-Path $repoRoot 'tests\wformat-style-gaps\manifest.json'
}
if (-not $WorkDir) {
    $WorkDir = Join-Path $repoRoot 'build\wformat-style-gap-feature-cases'
}
if (-not $ZcfExe) {
    $ZcfExe =
        Join-Path $repoRoot 'build\release\bin\zcf.exe'
}
if (-not $ReviewedStyleFile) {
    $ReviewedStyleFile =
        Join-Path $repoRoot 'tests\cpp-style-examples\wformat-p1.clang-format'
}
$Manifest = [System.IO.Path]::GetFullPath($Manifest)
$WorkDir = [System.IO.Path]::GetFullPath($WorkDir)
$ZcfExe = [System.IO.Path]::GetFullPath($ZcfExe)
$ReviewedStyleFile = [System.IO.Path]::GetFullPath($ReviewedStyleFile)
if (-not (Test-Path -LiteralPath $Manifest -PathType Leaf)) {
    throw "WFormat capability manifest was not found: $Manifest"
}

$manifestData = Get-Content -Raw -LiteralPath $Manifest | ConvertFrom-Json
$features = @($manifestData.unsupportedDesired)
if ($features.Count -ne 19) {
    throw "Expected 19 approved WFormat features, found $($features.Count)."
}

$caseRoot = Join-Path (Split-Path -Parent $Manifest) 'cases'
$auditScript = Join-Path $PSScriptRoot 'audit-wformat-style-gaps.ps1'
$pwshExe = (Get-Command pwsh -ErrorAction Stop).Source
$failures = [System.Collections.Generic.List[string]]::new()
$seenCases = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::Ordinal
)

foreach ($feature in $features) {
    $featureId = [string]$feature.id
    $caseId = [string]$feature.case
    $validationSource = if (
        $feature.PSObject.Properties.Name -contains 'validationSource'
    ) {
        [string]$feature.validationSource
    } else {
        'wformat'
    }
    if (-not $caseId) {
        $failures.Add("$featureId has no minimized case.")
        continue
    }
    if ($caseId -cne $featureId) {
        $failures.Add("$featureId maps to '$caseId' instead of a same-ID case.")
        continue
    }
    if (-not $seenCases.Add($caseId)) {
        $failures.Add("$featureId reuses duplicate case '$caseId'.")
        continue
    }

    $inputPath = Join-Path $caseRoot "$caseId.input.cpp"
    $targetSuffix = if ($validationSource -ceq 'reviewed') {
        'reviewed.cpp'
    } else {
        'wformat.cpp'
    }
    $targetPath = Join-Path $caseRoot "$caseId.$targetSuffix"
    if (-not (Test-Path -LiteralPath $inputPath -PathType Leaf)) {
        $failures.Add("$featureId is missing input: $inputPath")
        continue
    }
    if (-not (Test-Path -LiteralPath $targetPath -PathType Leaf)) {
        $failures.Add("$featureId is missing target: $targetPath")
        continue
    }

    $caseWorkDir = Join-Path $WorkDir $caseId
    if ($validationSource -ceq 'reviewed') {
        $missingReviewedFiles = @(
            @($ZcfExe, $ReviewedStyleFile) |
                Where-Object {
                    -not (Test-Path -LiteralPath $_ -PathType Leaf)
                }
        )
        if ($missingReviewedFiles.Count) {
            $failures.Add(
                "$featureId is missing reviewed validator input: " +
                ($missingReviewedFiles -join ', ')
            )
            continue
        }
        Remove-Item -LiteralPath $caseWorkDir -Recurse -Force `
            -ErrorAction SilentlyContinue
        [void](New-Item -ItemType Directory -Force -Path $caseWorkDir)
        $firstPath = Join-Path $caseWorkDir '01-reviewed.cpp'
        $secondPath = Join-Path $caseWorkDir '02-reviewed-second-pass.cpp'
        Copy-Item -LiteralPath $inputPath -Destination $firstPath
        & $ZcfExe "-style=file:$ReviewedStyleFile" -i $firstPath
        if ($LASTEXITCODE -ne 0) {
            $failures.Add(
                "$featureId reviewed formatting failed with exit code $LASTEXITCODE."
            )
            continue
        }
        Copy-Item -LiteralPath $firstPath -Destination $secondPath
        & $ZcfExe "-style=file:$ReviewedStyleFile" -i $secondPath
        if ($LASTEXITCODE -ne 0) {
            $failures.Add(
                "$featureId reviewed second pass failed with exit code $LASTEXITCODE."
            )
            continue
        }
        $targetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $targetPath).Hash
        $firstHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $firstPath).Hash
        $secondHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $secondPath).Hash
        if ($firstHash -cne $targetHash) {
            $failures.Add("$featureId differs from its reviewed target.")
            continue
        }
        if ($secondHash -cne $firstHash) {
            $failures.Add("$featureId reviewed target is not idempotent.")
            continue
        }
        Write-Host "PASS $featureId (reviewed)"
        continue
    }
    if ($validationSource -cne 'wformat') {
        $failures.Add(
            "$featureId has unknown validationSource '$validationSource'."
        )
        continue
    }
    $auditOutput = & $pwshExe -NoProfile -File $auditScript `
        -Fixture $inputPath `
        -WorkDir $caseWorkDir `
        -TimeoutSeconds $TimeoutSeconds 2>&1
    if ($LASTEXITCODE -ne 0) {
        $details = ($auditOutput | Select-Object -Last 5) -join [Environment]::NewLine
        $failures.Add("$featureId audit failed:$([Environment]::NewLine)$details")
        continue
    }

    $metadataPath = Join-Path $caseWorkDir 'metadata.json'
    $metadata = Get-Content -Raw -LiteralPath $metadataPath | ConvertFrom-Json
    if (-not $metadata.WFormatIsIdempotent) {
        $failures.Add("$featureId is not stable across two WFormat passes.")
        continue
    }

    $generatedPath = Join-Path $caseWorkDir '01-final.cpp'
    $generatedHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $generatedPath).Hash
    $targetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $targetPath).Hash
    if ($generatedHash -cne $targetHash) {
        $failures.Add(
            "$featureId target differs from its generated WFormat output."
        )
        continue
    }

    $closestPath = Join-Path $caseWorkDir '04-closest-clang.cpp'
    $generatedText = [System.IO.File]::ReadAllText($generatedPath).TrimEnd(
        [char[]]"`r`n"
    )
    $closestText = [System.IO.File]::ReadAllText($closestPath).TrimEnd(
        [char[]]"`r`n"
    )
    if ($generatedText -ceq $closestText) {
        $failures.Add(
            "$featureId differs from closest clang-format only at end of file."
        )
        continue
    }

    Write-Host "PASS $featureId"
}

if ($failures.Count) {
    throw (
        "WFormat feature-case validation failed ($($failures.Count)):" +
        [Environment]::NewLine +
        ($failures -join ([Environment]::NewLine + [Environment]::NewLine))
    )
}

Write-Host "Validated $($features.Count) approved feature cases."

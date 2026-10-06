#requires -Version 7.2

[CmdletBinding()]
param(
    [string]$Fixture = '',
    [string]$WFormatPackageDir = '',
    [string]$WFormatClangExe = '',
    [string]$WFormatClangConfig = '',
    [string]$UncrustifyExe = '',
    [string]$UncrustifyConfig = '',
    [string]$PythonExe = '',
    [string]$UpstreamClangFormatExe = '',
    [string]$ClosestClangConfig = '',
    [string]$WorkDir = '',
    [ValidateRange(1, 3600)][int]$TimeoutSeconds = 120
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$script:Utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$script:StageRecords = [System.Collections.Generic.List[object]]::new()
$script:TimeoutMilliseconds = $TimeoutSeconds * 1000

function Write-Utf8File {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Content
    )

    $parent = Split-Path -Parent $Path
    if ($parent) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }
    [System.IO.File]::WriteAllText($Path, $Content, $script:Utf8NoBom)
}

function Get-Sha256 {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
}

function Resolve-RequiredFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
    return [System.IO.Path]::GetFullPath($Path)
}

function Resolve-RequiredDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "$Description was not found: $Path"
    }
    return [System.IO.Path]::GetFullPath($Path)
}

function Stop-AuditProcess {
    param([Parameter(Mandatory = $true)][System.Diagnostics.Process]$Process)

    try {
        $Process.Kill($true)
    } catch [System.Management.Automation.MethodException] {
        $Process.Kill()
    }
    $Process.WaitForExit()
}

function Invoke-CapturedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [AllowNull()][string]$InputText = $null,
        [int[]]$AllowedExitCodes = @(0)
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.StandardOutputEncoding = $script:Utf8NoBom
    $startInfo.StandardErrorEncoding = $script:Utf8NoBom
    $startInfo.RedirectStandardInput = $null -ne $InputText
    if ($startInfo.RedirectStandardInput) {
        $startInfo.StandardInputEncoding = $script:Utf8NoBom
    }
    foreach ($argument in $Arguments) {
        [void]$startInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    if (-not $process.Start()) {
        throw "Failed to start process: $FilePath"
    }

    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if ($startInfo.RedirectStandardInput) {
        $process.StandardInput.Write($InputText)
        $process.StandardInput.Close()
    }

    $timedOut = -not $process.WaitForExit($script:TimeoutMilliseconds)
    if ($timedOut) {
        Stop-AuditProcess -Process $process
    }

    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    $stopwatch.Stop()
    $exitCode = if ($timedOut) { -1 } else { $process.ExitCode }

    return [pscustomobject]@{
        FilePath = $FilePath
        Arguments = @($Arguments)
        ExitCode = $exitCode
        TimedOut = $timedOut
        DurationMilliseconds = $stopwatch.ElapsedMilliseconds
        Stdout = $stdout
        Stderr = $stderr
        Succeeded = -not $timedOut -and $AllowedExitCodes -contains $exitCode
    }
}

function Invoke-AuditStage {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$OutputPath,
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$InputText
    )

    $result = Invoke-CapturedProcess -FilePath $FilePath -Arguments $Arguments -InputText $InputText
    Write-Utf8File -Path $OutputPath -Content $result.Stdout
    Write-Utf8File -Path "$OutputPath.stderr.txt" -Content $result.Stderr

    $record = [pscustomobject]@{
        Name = $Name
        FilePath = $FilePath
        Arguments = @($Arguments)
        ExitCode = $result.ExitCode
        TimedOut = $result.TimedOut
        DurationMilliseconds = $result.DurationMilliseconds
        OutputPath = $OutputPath
        OutputSha256 = Get-Sha256 -Path $OutputPath
        StderrPath = "$OutputPath.stderr.txt"
    }
    $script:StageRecords.Add($record)

    if (-not $result.Succeeded) {
        $reason = if ($result.TimedOut) {
            "timed out after $TimeoutSeconds seconds"
        } else {
            "exited with code $($result.ExitCode)"
        }
        throw "Audit stage '$Name' $reason. See $OutputPath.stderr.txt"
    }

    return $result.Stdout
}

function Invoke-VersionProbe {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $result = Invoke-CapturedProcess -FilePath $FilePath -Arguments $Arguments
    if (-not $result.Succeeded) {
        throw "Version probe failed for $FilePath with code $($result.ExitCode): $($result.Stderr)"
    }
    return ($result.Stdout + $result.Stderr).Trim()
}

function Invoke-WFormatStages {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$InputText,
        [Parameter(Mandatory = $true)][string]$Prefix
    )

    $clangOutputPath = Join-Path $WorkDir "$Prefix-clang.cpp"
    $uncrustifyOutputPath = Join-Path $WorkDir "$Prefix-uncrustify.cpp"
    $finalOutputPath = Join-Path $WorkDir "$Prefix-final.cpp"
    $assumeFilename = Join-Path (Split-Path -Parent $WFormatClangConfig) 'dummy.cpp'

    $clangOutput = Invoke-AuditStage `
        -Name "$Prefix-clang" `
        -OutputPath $clangOutputPath `
        -FilePath $WFormatClangExe `
        -Arguments @(
            "-style=file:$WFormatClangConfig",
            "-assume-filename=$assumeFilename"
        ) `
        -InputText $InputText

    $uncrustifyOutput = Invoke-AuditStage `
        -Name "$Prefix-uncrustify" `
        -OutputPath $uncrustifyOutputPath `
        -FilePath $UncrustifyExe `
        -Arguments @('-q', '-l', 'CPP', '-c', $UncrustifyConfig) `
        -InputText $clangOutput

    $normalizerScript = @'
import sys
from wformat.normalizer import fix_with_tree_sitter, normalize_integer_literal_in_memory

text = fix_with_tree_sitter(sys.stdin.read())
sys.stdout.write(normalize_integer_literal_in_memory(text))
'@
    $finalOutput = Invoke-AuditStage `
        -Name "$Prefix-final" `
        -OutputPath $finalOutputPath `
        -FilePath $PythonExe `
        -Arguments @('-c', $normalizerScript) `
        -InputText $uncrustifyOutput

    return [pscustomobject]@{
        ClangPath = $clangOutputPath
        ClangText = $clangOutput
        UncrustifyPath = $uncrustifyOutputPath
        UncrustifyText = $uncrustifyOutput
        FinalPath = $finalOutputPath
        FinalText = $finalOutput
    }
}

function Get-SectionForLine {
    param(
        [Parameter(Mandatory = $true)][object[]]$Sections,
        [Parameter(Mandatory = $true)][int]$LineNumber
    )

    $section = $Sections[0]
    foreach ($candidate in $Sections) {
        if ($candidate.Line -gt $LineNumber) {
            break
        }
        $section = $candidate
    }
    return $section.Name
}

function Get-FixtureSections {
    param([Parameter(Mandatory = $true)][string]$Path)

    $lines = [System.IO.File]::ReadAllLines($Path)
    $sections = [System.Collections.Generic.List[object]]::new()
    $sections.Add([pscustomobject]@{ Line = 1; Name = 'file-header' })

    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = $lines[$index]
        if ($line -match '^// (TIER \d+.+)$') {
            $sections.Add([pscustomobject]@{
                    Line = $index + 1
                    Name = $Matches[1].Trim()
                })
        } elseif ($line -match '^// --- (.+) ---$') {
            $sections.Add([pscustomobject]@{
                    Line = $index + 1
                    Name = $Matches[1].Trim()
                })
        } elseif (
            $index -gt 0 -and
            $lines[$index - 1] -match '^// ={20,}$' -and
            $line -match '^// Format style: (.+)$'
        ) {
            $sections.Add([pscustomobject]@{
                    Line = $index + 1
                    Name = "Format style: $($Matches[1].Trim())"
                })
        }
    }

    return @($sections)
}

function New-DiffArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$BeforePath,
        [Parameter(Mandatory = $true)][string]$AfterPath,
        [object[]]$Sections = @()
    )

    $gitExe = (Get-Command git -ErrorAction Stop).Source
    $diffResult = Invoke-CapturedProcess `
        -FilePath $gitExe `
        -Arguments @('--no-pager', 'diff', '--no-index', '--unified=3', '--', $BeforePath, $AfterPath) `
        -AllowedExitCodes @(0, 1)
    if (-not $diffResult.Succeeded) {
        throw "Unable to create diff '$Name': $($diffResult.Stderr)"
    }

    $numstatResult = Invoke-CapturedProcess `
        -FilePath $gitExe `
        -Arguments @('--no-pager', 'diff', '--no-index', '--numstat', '--', $BeforePath, $AfterPath) `
        -AllowedExitCodes @(0, 1)
    if (-not $numstatResult.Succeeded) {
        throw "Unable to create numstat '$Name': $($numstatResult.Stderr)"
    }

    $diffPath = Join-Path $WorkDir "$Name.diff"
    Write-Utf8File -Path $diffPath -Content $diffResult.Stdout
    Write-Utf8File -Path "$diffPath.stderr.txt" -Content $diffResult.Stderr

    $added = 0
    $deleted = 0
    if ($numstatResult.Stdout -match '^(\d+)\s+(\d+)\s+') {
        $added = [int]$Matches[1]
        $deleted = [int]$Matches[2]
    }

    $hunks = [System.Collections.Generic.List[object]]::new()
    foreach ($line in $diffResult.Stdout -split "`n") {
        if ($line -match '^@@ -(\d+)(?:,\d+)? \+(\d+)(?:,\d+)? @@(.*)$') {
            $oldLine = [int]$Matches[1]
            $hunks.Add([pscustomobject]@{
                    Header = $line.TrimEnd("`r")
                    OldLine = $oldLine
                    NewLine = [int]$Matches[2]
                    Section = if ($Sections.Count) {
                        Get-SectionForLine -Sections $Sections -LineNumber $oldLine
                    } else {
                        ''
                    }
                })
        }
    }

    $hunksPath = Join-Path $WorkDir "$Name.hunks.json"
    Write-Utf8File -Path $hunksPath -Content (
        ConvertTo-Json -InputObject @($hunks) -Depth 5
    )

    return [pscustomobject]@{
        Name = $Name
        BeforePath = $BeforePath
        AfterPath = $AfterPath
        DiffPath = $diffPath
        HunksPath = $hunksPath
        HunkCount = $hunks.Count
        AddedLines = $added
        DeletedLines = $deleted
        Equal = $diffResult.ExitCode -eq 0
    }
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

if (-not $PythonExe) {
    $PythonExe = (Get-Command py -ErrorAction Stop).Source
}
if (-not $Fixture) {
    $Fixture = Join-Path $repoRoot 'tests\cpp-style-examples\wformat_supported_style_details.cpp'
}
if (-not $UpstreamClangFormatExe) {
    $UpstreamClangFormatExe = Join-Path $repoRoot 'build\release\clang-build\bin\clang-format.exe'
}
if (-not $ClosestClangConfig) {
    $ClosestClangConfig = Join-Path $repoRoot 'tests\cpp-style-examples\wformat-target.clang-format'
}
if (-not $WorkDir) {
    $WorkDir = Join-Path $repoRoot 'build\wformat-style-gap-audit'
}

$PythonExe = Resolve-RequiredFile -Path $PythonExe -Description 'Python executable'
$Fixture = Resolve-RequiredFile -Path $Fixture -Description 'C++ fixture'
$UpstreamClangFormatExe = Resolve-RequiredFile `
    -Path $UpstreamClangFormatExe `
    -Description 'upstream clang-format executable'
$ClosestClangConfig = Resolve-RequiredFile `
    -Path $ClosestClangConfig `
    -Description 'closest clang-format configuration'

if (-not $WFormatPackageDir) {
    $packageProbe = Invoke-CapturedProcess `
        -FilePath $PythonExe `
        -Arguments @(
            '-c',
            'from pathlib import Path; import wformat; print(Path(wformat.__file__).resolve().parent)'
        )
    if (-not $packageProbe.Succeeded) {
        throw "Unable to locate the installed wformat package: $($packageProbe.Stderr)"
    }
    $WFormatPackageDir = $packageProbe.Stdout.Trim()
}
$WFormatPackageDir = Resolve-RequiredDirectory `
    -Path $WFormatPackageDir `
    -Description 'wformat package directory'

if (-not $WFormatClangExe) {
    $WFormatClangExe = Join-Path $WFormatPackageDir 'bin\clang-format.exe'
}
if (-not $WFormatClangConfig) {
    $WFormatClangConfig = Join-Path $WFormatPackageDir 'data\.clang-format'
}
if (-not $UncrustifyExe) {
    $UncrustifyExe = Join-Path $WFormatPackageDir 'bin\uncrustify.exe'
}
if (-not $UncrustifyConfig) {
    $UncrustifyConfig = Join-Path $WFormatPackageDir 'data\uncrustify.cfg'
}

$WFormatClangExe = Resolve-RequiredFile `
    -Path $WFormatClangExe `
    -Description 'wformat clang-format executable'
$WFormatClangConfig = Resolve-RequiredFile `
    -Path $WFormatClangConfig `
    -Description 'wformat clang-format configuration'
$UncrustifyExe = Resolve-RequiredFile `
    -Path $UncrustifyExe `
    -Description 'uncrustify executable'
$UncrustifyConfig = Resolve-RequiredFile `
    -Path $UncrustifyConfig `
    -Description 'uncrustify configuration'
$WorkDir = [System.IO.Path]::GetFullPath($WorkDir)

Remove-Item -LiteralPath $WorkDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

$inputPath = Join-Path $WorkDir '00-input.cpp'
$inputText = [System.IO.File]::ReadAllText($Fixture)
Write-Utf8File -Path $inputPath -Content $inputText

$firstPass = Invoke-WFormatStages -InputText $inputText -Prefix '01'

$closestOutputPath = Join-Path $WorkDir '04-closest-clang.cpp'
$closestAssumeFilename = Join-Path (Split-Path -Parent $ClosestClangConfig) 'dummy.cpp'
$closestOutput = Invoke-AuditStage `
    -Name '04-closest-clang' `
    -OutputPath $closestOutputPath `
    -FilePath $UpstreamClangFormatExe `
    -Arguments @(
        "-style=file:$ClosestClangConfig",
        "-assume-filename=$closestAssumeFilename"
    ) `
    -InputText $inputText

$secondPass = Invoke-WFormatStages -InputText $firstPass.FinalText -Prefix '05-second-pass'
$sections = Get-FixtureSections -Path $firstPass.FinalPath

$diffs = @(
    New-DiffArtifact `
        -Name 'input-vs-wformat-clang' `
        -BeforePath $inputPath `
        -AfterPath $firstPass.ClangPath `
        -Sections $sections
    New-DiffArtifact `
        -Name 'wformat-clang-vs-uncrustify' `
        -BeforePath $firstPass.ClangPath `
        -AfterPath $firstPass.UncrustifyPath `
        -Sections $sections
    New-DiffArtifact `
        -Name 'uncrustify-vs-final' `
        -BeforePath $firstPass.UncrustifyPath `
        -AfterPath $firstPass.FinalPath `
        -Sections $sections
    New-DiffArtifact `
        -Name 'input-vs-final' `
        -BeforePath $inputPath `
        -AfterPath $firstPass.FinalPath `
        -Sections $sections
    New-DiffArtifact `
        -Name 'wformat-final-vs-closest-clang' `
        -BeforePath $firstPass.FinalPath `
        -AfterPath $closestOutputPath `
        -Sections $sections
    New-DiffArtifact `
        -Name 'first-final-vs-second-final' `
        -BeforePath $firstPass.FinalPath `
        -AfterPath $secondPass.FinalPath `
        -Sections $sections
)

$wformatVersionResult = Invoke-CapturedProcess `
    -FilePath $PythonExe `
    -Arguments @('-c', 'import importlib.metadata; print(importlib.metadata.version("wformat"))')
if (-not $wformatVersionResult.Succeeded) {
    throw "Unable to determine the wformat version: $($wformatVersionResult.Stderr)"
}

$metadata = [ordered]@{
    SchemaVersion = 1
    GeneratedAtUtc = [DateTime]::UtcNow.ToString('o')
    RepoRoot = $repoRoot
    TimeoutSeconds = $TimeoutSeconds
    OracleVersions = [ordered]@{
        WFormat = $wformatVersionResult.Stdout.Trim()
        WFormatClang = Invoke-VersionProbe -FilePath $WFormatClangExe -Arguments @('--version')
        Uncrustify = Invoke-VersionProbe -FilePath $UncrustifyExe -Arguments @('--version')
        UpstreamClang = Invoke-VersionProbe -FilePath $UpstreamClangFormatExe -Arguments @('--version')
    }
    Inputs = @(
        [ordered]@{ Name = 'Fixture'; Path = $Fixture; Sha256 = Get-Sha256 -Path $Fixture }
        [ordered]@{ Name = 'WFormatClangExe'; Path = $WFormatClangExe; Sha256 = Get-Sha256 -Path $WFormatClangExe }
        [ordered]@{ Name = 'WFormatClangConfig'; Path = $WFormatClangConfig; Sha256 = Get-Sha256 -Path $WFormatClangConfig }
        [ordered]@{ Name = 'UncrustifyExe'; Path = $UncrustifyExe; Sha256 = Get-Sha256 -Path $UncrustifyExe }
        [ordered]@{ Name = 'UncrustifyConfig'; Path = $UncrustifyConfig; Sha256 = Get-Sha256 -Path $UncrustifyConfig }
        [ordered]@{ Name = 'UpstreamClangFormatExe'; Path = $UpstreamClangFormatExe; Sha256 = Get-Sha256 -Path $UpstreamClangFormatExe }
        [ordered]@{ Name = 'ClosestClangConfig'; Path = $ClosestClangConfig; Sha256 = Get-Sha256 -Path $ClosestClangConfig }
    )
    Stages = @($script:StageRecords)
    Diffs = @($diffs)
    CanonicalInputMatchesWFormat = $inputText -ceq $firstPass.FinalText
    WFormatIsIdempotent = $firstPass.FinalText -ceq $secondPass.FinalText
    ClosestClangMatchesWFormat = $closestOutput -ceq $firstPass.FinalText
}

$metadataPath = Join-Path $WorkDir 'metadata.json'
Write-Utf8File -Path $metadataPath -Content (ConvertTo-Json -InputObject $metadata -Depth 12)

$summaryLines = [System.Collections.Generic.List[string]]::new()
$summaryLines.Add("Fixture: $Fixture")
$summaryLines.Add("Work directory: $WorkDir")
$summaryLines.Add("Canonical input matches wformat: $($metadata.CanonicalInputMatchesWFormat)")
$summaryLines.Add("Wformat is idempotent: $($metadata.WFormatIsIdempotent)")
$summaryLines.Add("Closest clang-format matches wformat: $($metadata.ClosestClangMatchesWFormat)")
$summaryLines.Add('')
$summaryLines.Add('Diff summary:')
foreach ($diff in $diffs) {
    $summaryLines.Add(
        "  $($diff.Name): $($diff.HunkCount) hunks, +$($diff.AddedLines)/-$($diff.DeletedLines)"
    )
}
$summaryPath = Join-Path $WorkDir 'summary.txt'
Write-Utf8File -Path $summaryPath -Content (($summaryLines -join "`n") + "`n")

if (-not $metadata.WFormatIsIdempotent) {
    throw "Wformat output is not idempotent. See $WorkDir\first-final-vs-second-final.diff"
}

Write-Host ($summaryLines -join [Environment]::NewLine)
Write-Host "Metadata: $metadataPath"

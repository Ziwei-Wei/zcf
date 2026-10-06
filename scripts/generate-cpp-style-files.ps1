[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$FormatterExe,
    [Parameter(Mandatory = $true)][string]$InputDir,
    [Parameter(Mandatory = $true)][string]$OutputDir,
    [Parameter(Mandatory = $true)][string]$StyleFile
)

$ErrorActionPreference = 'Stop'

$FormatterExe = [System.IO.Path]::GetFullPath($FormatterExe)
$InputDir = [System.IO.Path]::GetFullPath($InputDir)
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
$StyleFile = [System.IO.Path]::GetFullPath($StyleFile)

if (-not (Test-Path $FormatterExe)) {
    throw "Formatter executable was not found: $FormatterExe"
}
if (-not (Test-Path $InputDir)) {
    throw "C++ example input directory was not found: $InputDir"
}
if (-not (Test-Path $StyleFile)) {
    throw "C++ target style file was not found: $StyleFile"
}

$inputs = Get-ChildItem -Path $InputDir -Filter '*.cpp' -File |
    Where-Object { $_.Name -notmatch '\.(correct|formatted)\.cpp$' } |
    Sort-Object Name

if (-not $inputs) {
    throw "No C++ example inputs were found in $InputDir"
}

Remove-Item -Recurse -Force $OutputDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
foreach ($input in $inputs) {
    $output = Join-Path $OutputDir "$($input.BaseName).formatted.cpp"
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $FormatterExe
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.StandardOutputEncoding = $utf8NoBom
    $psi.StandardErrorEncoding = $utf8NoBom
    [void]$psi.ArgumentList.Add('--language=cpp')
    [void]$psi.ArgumentList.Add("-style=file:$StyleFile")
    [void]$psi.ArgumentList.Add($input.FullName)

    $process = [System.Diagnostics.Process]::Start($psi)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    if ($process.ExitCode -ne 0) {
        throw "Formatting failed for $($input.Name) with exit code $($process.ExitCode): $stderr"
    }

    [System.IO.File]::WriteAllText($output, $stdout, $utf8NoBom)
    Write-Host "Generated $output"
}

Write-Host "Generated $($inputs.Count) C++ formatted style file(s) in $OutputDir"

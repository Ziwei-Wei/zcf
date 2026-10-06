[CmdletBinding()]
param(
    [string]$CustomExe = '',
    [string]$UpstreamExe = '',
    [string]$WorkDir = '',
    [switch]$FetchDocs
)

$ErrorActionPreference = 'Stop'

$ScriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$RepoRoot = Resolve-Path (Join-Path $ScriptRoot '..')
$DocsUrl = 'https://clang.llvm.org/docs/ClangFormatStyleOptions.html'

if (-not $CustomExe) {
    $CustomExe = Join-Path $RepoRoot.Path 'build\release\bin\zcf.exe'
}

if (-not $UpstreamExe) {
    $UpstreamExe = Join-Path $RepoRoot.Path 'build\release\clang-build\bin\clang-format.exe'
}

if (-not $WorkDir) {
    $WorkDir = Join-Path $RepoRoot.Path 'build\format-comparison-tests'
}

$CustomExe = [System.IO.Path]::GetFullPath($CustomExe)
$UpstreamExe = [System.IO.Path]::GetFullPath($UpstreamExe)
$WorkDir = [System.IO.Path]::GetFullPath($WorkDir)

if (-not (Test-Path $CustomExe)) {
    throw "Custom clang-format executable was not found: $CustomExe"
}

if (-not (Test-Path $UpstreamExe)) {
    throw "Third-party clang-format executable was not found: $UpstreamExe"
}

Remove-Item -Recurse -Force $WorkDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

$script:Utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$script:InvocationId = 0
$script:Passed = 0
$script:Failed = 0
$script:Skipped = 0
$script:FailureRoot = Join-Path $WorkDir 'failures'

function Write-TextFile {
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

function ConvertTo-SafeName {
    param([Parameter(Mandatory = $true)][string]$Name)
    $safe = $Name -replace '[^A-Za-z0-9_.-]+', '_'
    if ($safe.Length -gt 120) {
        $safe = $safe.Substring(0, 120)
    }
    return $safe.Trim('_')
}

function Invoke-Formatter {
    param(
        [Parameter(Mandatory = $true)][string]$Exe,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $script:InvocationId++
    $safeName = ConvertTo-SafeName "$($script:InvocationId)-$Name"
    $stdoutPath = Join-Path $WorkDir "$safeName.stdout"
    $stderrPath = Join-Path $WorkDir "$safeName.stderr"

    & $Exe @Arguments > $stdoutPath 2> $stderrPath
    $exitCode = $LASTEXITCODE

    return [pscustomobject]@{
        ExitCode = $exitCode
        Stdout = [System.IO.File]::ReadAllText($stdoutPath)
        Stderr = [System.IO.File]::ReadAllText($stderrPath)
        StdoutPath = $stdoutPath
        StderrPath = $stderrPath
    }
}

function Save-Failure {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)]$CustomResult,
        [Parameter(Mandatory = $true)]$UpstreamResult
    )

    $safeName = ConvertTo-SafeName $Name
    $failureDir = Join-Path $script:FailureRoot $safeName
    New-Item -ItemType Directory -Force -Path $failureDir | Out-Null

    Write-TextFile (Join-Path $failureDir 'args.txt') (($Arguments -join [Environment]::NewLine) + [Environment]::NewLine)
    Write-TextFile (Join-Path $failureDir 'custom.exit') ([string]$CustomResult.ExitCode)
    Write-TextFile (Join-Path $failureDir 'upstream.exit') ([string]$UpstreamResult.ExitCode)
    Write-TextFile (Join-Path $failureDir 'custom.stdout') $CustomResult.Stdout
    Write-TextFile (Join-Path $failureDir 'upstream.stdout') $UpstreamResult.Stdout
    Write-TextFile (Join-Path $failureDir 'custom.stderr') $CustomResult.Stderr
    Write-TextFile (Join-Path $failureDir 'upstream.stderr') $UpstreamResult.Stderr
}

function Compare-FormatterInvocation {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    $custom = Invoke-Formatter -Exe $CustomExe -Arguments $Arguments -Name "custom-$Name"
    $upstream = Invoke-Formatter -Exe $UpstreamExe -Arguments $Arguments -Name "upstream-$Name"

    $same = $custom.ExitCode -eq $upstream.ExitCode
    $same = $same -and ($custom.Stdout -ceq $upstream.Stdout)
    $same = $same -and ($custom.Stderr -ceq $upstream.Stderr)

    if ($same) {
        $script:Passed++
        Write-Host "[PASS] $Name"
    } else {
        $script:Failed++
        Save-Failure -Name $Name -Arguments $Arguments -CustomResult $custom -UpstreamResult $upstream
        Write-Host "[FAIL] $Name"
    }
}

function Get-UpstreamDumpConfig {
    param([Parameter(Mandatory = $true)][string]$Style)
    $result = Invoke-Formatter -Exe $UpstreamExe -Arguments @("-style=$Style", '-dump-config') -Name "dump-$Style"
    if ($result.ExitCode -ne 0) {
        throw "Unable to dump upstream config for style '$Style': $($result.Stderr)"
    }
    return $result.Stdout
}

function New-Fixture {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$FileName,
        [Parameter(Mandatory = $true)][string]$Text
    )
    return [pscustomobject]@{
        Name = $Name
        FileName = $FileName
        Text = $Text.TrimStart("`r", "`n")
    }
}

function Write-Fixture {
    param(
        [Parameter(Mandatory = $true)]$Fixture,
        [Parameter(Mandatory = $true)][string]$Directory
    )

    $path = Join-Path $Directory $Fixture.FileName
    Write-TextFile $path $Fixture.Text
    return $path
}

function Run-StyleFixtureCase {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]$Fixture,
        [Parameter(Mandatory = $true)][string]$Style
    )

    $caseDir = Join-Path $WorkDir (ConvertTo-SafeName $Name)
    $inputPath = Write-Fixture -Fixture $Fixture -Directory $caseDir
    Compare-FormatterInvocation -Name $Name -Arguments @("-style=$Style", $inputPath)
}

function Run-FileStyleFixtureCase {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]$Fixture,
        [Parameter(Mandatory = $true)][string]$ConfigText
    )

    $caseDir = Join-Path $WorkDir (ConvertTo-SafeName $Name)
    $configPath = Join-Path $caseDir '.clang-format'
    Write-TextFile $configPath $ConfigText
    $inputPath = Write-Fixture -Fixture $Fixture -Directory $caseDir
    Compare-FormatterInvocation -Name $Name -Arguments @("-style=file:$configPath", $inputPath)
}

function Run-InPlaceCase {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]$Fixture,
        [Parameter(Mandatory = $true)][string]$Style
    )

    $caseDir = Join-Path $WorkDir (ConvertTo-SafeName $Name)
    $customDir = Join-Path $caseDir 'custom'
    $upstreamDir = Join-Path $caseDir 'upstream'
    $customPath = Write-Fixture -Fixture $Fixture -Directory $customDir
    $upstreamPath = Write-Fixture -Fixture $Fixture -Directory $upstreamDir

    $custom = Invoke-Formatter -Exe $CustomExe -Arguments @('-i', "-style=$Style", $customPath) -Name "custom-$Name"
    $upstream = Invoke-Formatter -Exe $UpstreamExe -Arguments @('-i', "-style=$Style", $upstreamPath) -Name "upstream-$Name"

    $customFile = [System.IO.File]::ReadAllText($customPath)
    $upstreamFile = [System.IO.File]::ReadAllText($upstreamPath)
    $same = $custom.ExitCode -eq $upstream.ExitCode
    $same = $same -and ($custom.Stdout -ceq $upstream.Stdout)
    $same = $same -and ($custom.Stderr -ceq $upstream.Stderr)
    $same = $same -and ($customFile -ceq $upstreamFile)

    if ($same) {
        $script:Passed++
        Write-Host "[PASS] $Name"
    } else {
        $script:Failed++
        Save-Failure -Name $Name -Arguments @('-i', "-style=$Style", '<fixture>') -CustomResult $custom -UpstreamResult $upstream
        $failureDir = Join-Path $script:FailureRoot (ConvertTo-SafeName $Name)
        Write-TextFile (Join-Path $failureDir 'custom.formatted') $customFile
        Write-TextFile (Join-Path $failureDir 'upstream.formatted') $upstreamFile
        Write-Host "[FAIL] $Name"
    }
}

$Fixtures = @(
    New-Fixture -Name 'cpp-core' -FileName 'sample.cpp' -Text @'
#include "zeta.h"
#include <vector>
#include "alpha.h"
#define SHORT_NAME 42
#define LONGER_NAME 0x007f
#define EVEN_LONGER_NAME (2)
namespace alpha { namespace beta {
template <typename T, typename U> class Example : public Base<T> {
public:
Example(T *ptr,const U &value):ptr_(ptr),value_(value){}
auto compute(int a,int b)->int{ if(a>b){return a+b;}else{return a-b;}}
private:
T *ptr_;
U value_;
};
}}
int a=1;
int somelongname=2;
double c=3;
enum E { A=1, LongName=2 };
struct Bits { int aaaa:1; int b:12; int ccc:8; };
'@
    New-Fixture -Name 'cpp-modern' -FileName 'modern.cc' -Text @'
template <typename T> concept C = requires(T t) { t + 1; };
auto lambda=[](int x){return x<0?-x:x;};
int f(){std::vector<int> values={1,2,3,4,5,6};return values[0];}
const volatile int * const *ptr;
static inline const volatile long value = 1234567890;
'@
    New-Fixture -Name 'objc' -FileName 'sample.m' -Text @'
@interface Foo:NSObject@property(nonatomic,strong)NSString*name;@end
@implementation Foo
- (void)run{if(self.name){NSLog(@"%@",self.name);}}
@end
'@
    New-Fixture -Name 'javascript' -FileName 'sample.js' -Text @'
import {zeta,beta,alpha} from "pkg"; const obj={foo:1,bar:[1,2,3],baz:function(x){return x?{a:1}:{b:2};}};
'@
    New-Fixture -Name 'json' -FileName 'sample.json' -Text @'
{"z":1,"alpha":[{"name":"first","enabled":true},{"name":"second","enabled":false}],"nested":{"value":123}}
'@
    New-Fixture -Name 'java' -FileName 'Sample.java' -Text @'
class Sample{ @Deprecated private final int value=1; public int get(){if(value>0){return value;}return 0;} }
'@
    New-Fixture -Name 'csharp' -FileName 'Sample.cs' -Text @'
namespace Demo{class Sample{public int Value{get;set;} public void Run(){if(Value>0){Value++;}}}}
'@
    New-Fixture -Name 'proto' -FileName 'sample.proto' -Text @'
syntax="proto3"; package demo; message Foo{string name=1; repeated int32 values=2; oneof choice{int32 id=3; string key=4;}}
'@
    New-Fixture -Name 'textproto' -FileName 'sample.textproto' -Text @'
foo { bar: 1 baz: "two" nested { enabled: true value: 3 } }
'@
    New-Fixture -Name 'tablegen' -FileName 'sample.td' -Text @'
def Demo : Instruction { let OutOperandList = (outs GPR:$dst); let InOperandList = (ins GPR:$src); let AsmString = "demo $dst, $src"; }
'@
    New-Fixture -Name 'verilog' -FileName 'sample.sv' -Text @'
module top(input logic clk,output logic y);always_ff@(posedge clk)begin y<=~y;end endmodule
'@
)

$FixtureByName = @{}
foreach ($fixture in $Fixtures) {
    $FixtureByName[$fixture.Name] = $fixture
}

$BaseStyles = @('LLVM', 'Google', 'Chromium', 'Mozilla', 'WebKit', 'Microsoft', 'GNU')

$FocusedStyleCases = @(
    @{ Name = 'column-limit-40'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, ColumnLimit: 40}' },
    @{ Name = 'indent-width-access-modifiers'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, IndentWidth: 4, ContinuationIndentWidth: 8, AccessModifierOffset: -4}' },
    @{ Name = 'allman-braces'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, BreakBeforeBraces: Allman}' },
    @{ Name = 'custom-brace-wrapping'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, BreakBeforeBraces: Custom, BraceWrapping: {AfterClass: true, AfterFunction: true, AfterControlStatement: Always, BeforeElse: true, IndentBraces: true}}' },
    @{ Name = 'pointer-reference-left'; Fixture = 'cpp-modern'; Style = '{BasedOnStyle: LLVM, DerivePointerAlignment: false, PointerAlignment: Left, ReferenceAlignment: Left}' },
    @{ Name = 'align-assignments'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, AlignConsecutiveAssignments: Consecutive}' },
    @{ Name = 'align-declarations'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, AlignConsecutiveDeclarations: Consecutive}' },
    @{ Name = 'align-macros'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, AlignConsecutiveMacros: Consecutive}' },
    @{ Name = 'align-bitfields'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, AlignConsecutiveBitFields: Consecutive}' },
    @{ Name = 'trailing-comments'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, AlignTrailingComments: {Kind: Always, OverEmptyLines: 2}}' },
    @{ Name = 'include-sorting-regroup'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, SortIncludes: CaseInsensitive, IncludeBlocks: Regroup, IncludeCategories: [{Regex: "^<.*", Priority: 2}, {Regex: ".*", Priority: 1}]}' },
    @{ Name = 'short-constructs'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, AllowShortFunctionsOnASingleLine: Empty, AllowShortIfStatementsOnASingleLine: AllIfsAndElse, AllowShortLoopsOnASingleLine: true}' },
    @{ Name = 'bin-pack-off'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, BinPackArguments: false, BinPackParameters: OnePerLine}' },
    @{ Name = 'braced-list-spacing'; Fixture = 'cpp-modern'; Style = '{BasedOnStyle: LLVM, Cpp11BracedListStyle: false, SpaceBeforeCpp11BracedList: true}' },
    @{ Name = 'qualifier-custom'; Fixture = 'cpp-modern'; Style = '{BasedOnStyle: LLVM, QualifierAlignment: Custom, QualifierOrder: [inline, static, type, const, volatile]}' },
    @{ Name = 'integer-literal-separators'; Fixture = 'cpp-modern'; Style = '{BasedOnStyle: LLVM, IntegerLiteralSeparator: {Binary: 4, BinaryMinDigits: 8, Decimal: 3, DecimalMinDigits: 5, Hex: 2, HexMinDigits: 6}}' },
    @{ Name = 'namespace-options'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, NamespaceIndentation: All, CompactNamespaces: true, FixNamespaceComments: true}' },
    @{ Name = 'comment-and-string-options'; Fixture = 'cpp-core'; Style = '{BasedOnStyle: LLVM, ReflowComments: Never, BreakStringLiterals: false}' },
    @{ Name = 'objc-options'; Fixture = 'objc'; Style = '{BasedOnStyle: LLVM, ObjCBlockIndentWidth: 4, ObjCSpaceAfterProperty: true, ObjCSpaceBeforeProtocolList: false}' },
    @{ Name = 'javascript-options'; Fixture = 'javascript'; Style = '{BasedOnStyle: LLVM, JavaScriptQuotes: Single, JavaScriptWrapImports: true, ColumnLimit: 60}' },
    @{ Name = 'json-options'; Fixture = 'json'; Style = '{BasedOnStyle: LLVM, SpaceBeforeJsonColon: true, ColumnLimit: 40}' },
    @{ Name = 'java-options'; Fixture = 'java'; Style = '{BasedOnStyle: Google, BreakAfterJavaFieldAnnotations: true, ColumnLimit: 60}' },
    @{ Name = 'csharp-options'; Fixture = 'csharp'; Style = '{BasedOnStyle: Microsoft, ColumnLimit: 60, NamespaceIndentation: All}' },
    @{ Name = 'proto-disabled'; Fixture = 'proto'; Style = '{BasedOnStyle: LLVM, DisableFormat: true}' },
    @{ Name = 'tablegen-alignment'; Fixture = 'tablegen'; Style = '{BasedOnStyle: LLVM, AlignConsecutiveTableGenDefinitionColons: Consecutive, ColumnLimit: 60}' },
    @{ Name = 'verilog-column-limit'; Fixture = 'verilog'; Style = '{BasedOnStyle: LLVM, ColumnLimit: 40, IndentWidth: 4}' }
)

Write-Host "Custom formatter:   $CustomExe"
Write-Host "Upstream formatter: $UpstreamExe"
Write-Host "Docs reference:     $DocsUrl"
Write-Host "Work directory:     $WorkDir"

if ($FetchDocs) {
    try {
        $docsPath = Join-Path $WorkDir 'ClangFormatStyleOptions.html'
        $response = Invoke-WebRequest -UseBasicParsing -Uri $DocsUrl
        Write-TextFile $docsPath $response.Content
        Write-Host "Fetched style option docs to $docsPath"
    } catch {
        $script:Skipped++
        Write-Warning "Unable to fetch $DocsUrl. Continuing with local option surface: $_"
    }
}

$llvmDump = Get-UpstreamDumpConfig -Style 'LLVM'
$topLevelOptions = @(
    $llvmDump -split "`r?`n" |
        Where-Object { $_ -match '^[A-Za-z][A-Za-z0-9]*:' } |
        ForEach-Object { ($_ -split ':', 2)[0] } |
        Sort-Object -Unique
)
Write-TextFile (Join-Path $WorkDir 'dump-config-top-level-options.txt') (($topLevelOptions -join [Environment]::NewLine) + [Environment]::NewLine)
Write-Host "Complete option surface source: upstream -dump-config exposed $($topLevelOptions.Count) top-level options for this LLVM checkout."

foreach ($style in $BaseStyles) {
    Compare-FormatterInvocation -Name "dump-config/$style" -Arguments @("-style=$style", '-dump-config')
}

foreach ($style in $BaseStyles) {
    foreach ($fixture in $Fixtures) {
        Run-StyleFixtureCase -Name "preset/$style/$($fixture.Name)" -Fixture $fixture -Style $style
    }
}

foreach ($style in $BaseStyles) {
    $config = Get-UpstreamDumpConfig -Style $style
    foreach ($fixtureName in @('cpp-core', 'cpp-modern', 'javascript', 'json', 'proto', 'tablegen')) {
        Run-FileStyleFixtureCase -Name "full-config-roundtrip/$style/$fixtureName" -Fixture $FixtureByName[$fixtureName] -ConfigText $config
    }
}

foreach ($case in $FocusedStyleCases) {
    Run-StyleFixtureCase -Name "focused/$($case.Name)" -Fixture $FixtureByName[$case.Fixture] -Style $case.Style
}

$cppFixture = $FixtureByName['cpp-core']
$rangeDir = Join-Path $WorkDir 'cli-range'
$rangePath = Write-Fixture -Fixture $cppFixture -Directory $rangeDir
Compare-FormatterInvocation -Name 'cli/output-replacements-xml' -Arguments @('-style=LLVM', '--output-replacements-xml', $rangePath)
Compare-FormatterInvocation -Name 'cli/lines-range' -Arguments @('-style=LLVM', '-lines=1:8', $rangePath)
Compare-FormatterInvocation -Name 'cli/offset-length' -Arguments @('-style=LLVM', '-offset=0', '-length=80', $rangePath)
Compare-FormatterInvocation -Name 'cli/dry-run' -Arguments @('-style=LLVM', '--dry-run', $rangePath)
Compare-FormatterInvocation -Name 'cli/sort-includes-override' -Arguments @('-style=LLVM', '-sort-includes', $rangePath)
Compare-FormatterInvocation -Name 'cli/qualifier-alignment-override' -Arguments @('-style=LLVM', '-qualifier-alignment=type const volatile', (Write-Fixture -Fixture $FixtureByName['cpp-modern'] -Directory (Join-Path $WorkDir 'cli-qualifier')))
Run-InPlaceCase -Name 'cli/in-place' -Fixture $cppFixture -Style 'LLVM'

Write-Host ''
Write-Host "Comparison summary: passed=$script:Passed failed=$script:Failed skipped=$script:Skipped"
if ($script:Failed -ne 0) {
    Write-Host "Failure artifacts: $script:FailureRoot"
    exit 1
}

Write-Host 'All custom-vs-upstream clang-format comparison tests passed.'

#include "format_comparison_support.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace format_compare {
namespace {

int InvocationId = 0;

std::string QuoteCommandArgument(std::string_view Argument) {
  std::string Quoted = "\"";
  for (const char C : Argument) {
    if (C == '"')
      Quoted += "\\\"";
    else
      Quoted += C;
  }
  Quoted += '"';
  return Quoted;
}

bool ContainsFixture(std::initializer_list<std::string_view> FixtureNames,
                     const std::string &FixtureName) {
  for (std::string_view Name : FixtureNames) {
    if (Name == FixtureName)
      return true;
  }
  return false;
}

} // namespace

Options TestOptions;

std::string ReadFile(const std::filesystem::path &Path) {
  std::ifstream Input(Path, std::ios::binary);
  std::ostringstream Buffer;
  Buffer << Input.rdbuf();
  return Buffer.str();
}

void WriteFile(const std::filesystem::path &Path, std::string_view Content) {
  std::filesystem::create_directories(Path.parent_path());
  std::ofstream Output(Path, std::ios::binary | std::ios::trunc);
  Output << Content;
}

std::string SafeName(std::string Name) {
  for (char &C : Name) {
    const bool IsSafe = std::isalnum(static_cast<unsigned char>(C)) ||
                        C == '_' || C == '-' || C == '.';
    if (!IsSafe)
      C = '_';
  }
  while (!Name.empty() && Name.front() == '_')
    Name.erase(Name.begin());
  while (!Name.empty() && Name.back() == '_')
    Name.pop_back();
  if (Name.size() > 120)
    Name.resize(120);
  return Name.empty() ? "case" : Name;
}

std::string GTestName(std::string Name) {
  for (char &C : Name) {
    if (!std::isalnum(static_cast<unsigned char>(C)))
      C = '_';
  }
  while (!Name.empty() && Name.front() == '_')
    Name.erase(Name.begin());
  if (Name.empty())
    Name = "Case";
  if (std::isdigit(static_cast<unsigned char>(Name.front())))
    Name.insert(Name.begin(), 'C');
  return Name;
}

ProcessResult RunFormatter(const std::filesystem::path &Exe,
                           const std::vector<std::string> &Arguments,
                           const std::string &Name) {
  ++InvocationId;
  const std::string BaseName =
      SafeName(std::to_string(InvocationId) + "-" + Name);
  const std::filesystem::path StdoutPath =
      TestOptions.WorkDir / (BaseName + ".stdout");
  const std::filesystem::path StderrPath =
      TestOptions.WorkDir / (BaseName + ".stderr");

  std::string Command;
#ifdef _WIN32
  Command = "call ";
#endif
  Command += QuoteCommandArgument(Exe.string());
  for (const std::string &Argument : Arguments) {
    Command += ' ';
    Command += QuoteCommandArgument(Argument);
  }
  Command += " > ";
  Command += QuoteCommandArgument(StdoutPath.string());
  Command += " 2> ";
  Command += QuoteCommandArgument(StderrPath.string());

  const int ExitCode = std::system(Command.c_str());
  return {ExitCode, ReadFile(StdoutPath), ReadFile(StderrPath)};
}

void SaveFailure(const std::string &Name, const std::vector<std::string> &Args,
                 const ProcessResult &Custom, const ProcessResult &Upstream) {
  const std::filesystem::path FailureDir =
      TestOptions.WorkDir / "gtest-failures" / SafeName(Name);
  std::ostringstream ArgsText;
  for (const std::string &Arg : Args)
    ArgsText << Arg << '\n';

  WriteFile(FailureDir / "args.txt", ArgsText.str());
  WriteFile(FailureDir / "custom.exit", std::to_string(Custom.ExitCode));
  WriteFile(FailureDir / "upstream.exit", std::to_string(Upstream.ExitCode));
  WriteFile(FailureDir / "custom.stdout", Custom.Stdout);
  WriteFile(FailureDir / "upstream.stdout", Upstream.Stdout);
  WriteFile(FailureDir / "custom.stderr", Custom.Stderr);
  WriteFile(FailureDir / "upstream.stderr", Upstream.Stderr);
}

void CompareInvocation(const std::string &Name,
                       const std::vector<std::string> &Arguments) {
  SCOPED_TRACE(Name);
  const ProcessResult Custom =
      RunFormatter(TestOptions.CustomClangFormat, Arguments, "custom-" + Name);
  const ProcessResult Upstream = RunFormatter(TestOptions.UpstreamClangFormat,
                                              Arguments, "upstream-" + Name);

  if (Custom.ExitCode != Upstream.ExitCode ||
      Custom.Stdout != Upstream.Stdout || Custom.Stderr != Upstream.Stderr) {
    SaveFailure(Name, Arguments, Custom, Upstream);
  }

  EXPECT_EQ(Custom.ExitCode, Upstream.ExitCode);
  EXPECT_EQ(Custom.Stdout, Upstream.Stdout);
  EXPECT_EQ(Custom.Stderr, Upstream.Stderr);
}

std::string UpstreamDumpConfig(const std::string &Style) {
  const ProcessResult Result =
      RunFormatter(TestOptions.UpstreamClangFormat,
                   {"-style=" + Style, "-dump-config"}, "dump-" + Style);
  EXPECT_EQ(Result.ExitCode, 0) << Result.Stderr;
  return Result.Stdout;
}

std::filesystem::path WriteFixture(const Fixture &F,
                                   const std::filesystem::path &Directory) {
  const std::filesystem::path Path = Directory / F.FileName;
  WriteFile(Path, F.Text);
  return Path;
}

const std::vector<Fixture> &Fixtures() {
  // These are intentionally small, doc-shaped inputs rather than production
  // samples. The comments point future agents back to the examples on
  // ClangFormatStyleOptions.html, which is generated from the pinned
  // third_party\llvm-project\clang\docs\ClangFormatStyleOptions.rst file.
  static const std::vector<Fixture> Value = {
      {"c", "sample.c",
       "C fixture for C-family lexer/parser coverage and C-style "
       "declarations, macros, structs, and control flow.",
       R"(#include <stdio.h>
#define SHORT_NAME 42
#define LONGER_NAME 0x007f
struct Point { int x; int y; };
int add(int a,int b){if(a>b){return a+b;}return a-b;}
)"},
      {"cpp-core", "sample.cpp",
       "C++ fixture combining documentation examples for include grouping, "
       "macros, assignments, declarations, bit-fields, namespaces, and "
       "brace wrapping.",
       R"(#include "zeta.h"
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
)"},
      {"cpp-modern", "modern.cc",
       "C++20 fixture for documented pointer/reference alignment, qualifier "
       "ordering, braced-list spacing, and integer literal separator cases.",
       R"(template <typename T> concept C = requires(T t) { t + 1; };
auto lambda=[](int x){return x<0?-x:x;};
int f(){std::vector<int> values={1,2,3,4,5,6};return values[0];}
const volatile int * const *ptr;
static inline const volatile long value = 1234567890;
)"},
      {"objc", "sample.m",
       "Objective-C fixture for ObjC block/property/protocol-list spacing "
       "examples from the style options documentation.",
       R"(@interface Foo:NSObject@property(nonatomic,strong)NSString*name;@end
@implementation Foo
- (void)run{if(self.name){NSLog(@"%@",self.name);}}
@end
)"},
      {"javascript", "sample.js",
       "JavaScript fixture for documented quote normalization and wrapped "
       "import/export behavior.",
       R"(import {zeta,beta,alpha} from "pkg"; const obj={foo:1,bar:[1,2,3],baz:function(x){return x?{a:1}:{b:2};}};
)"},
      {"json", "sample.json",
       "JSON fixture for SpaceBeforeJsonColon and column-limit examples.",
       R"({"z":1,"alpha":[{"name":"first","enabled":true},{"name":"second","enabled":false}],"nested":{"value":123}}
)"},
      {"java", "Sample.java",
       "Java fixture for BreakAfterJavaFieldAnnotations and column-limit "
       "examples.",
       R"(class Sample{ @Deprecated private final int value=1; public int get(){if(value>0){return value;}return 0;} }
)"},
      {"csharp", "Sample.cs",
       "C# fixture for language-specific column-limit and namespace "
       "indentation examples.",
       R"(namespace Demo{class Sample{public int Value{get;set;} public void Run(){if(Value>0){Value++;}}}}
)"},
      {"cmake", "CMakeLists.txt",
       "CMake fixture for cmake-format backend dispatch and common "
       "ColumnLimit/IndentWidth/UseTab style mapping.",
       R"(cmake_minimum_required(VERSION 3.20)
project(Demo)
add_executable(demo main.cpp include/demo.h src/demo.cpp)
)"},
      {"proto", "sample.proto",
       "Proto fixture for the multi-language DisableFormat example.",
       R"(syntax="proto3"; package demo; message Foo{string name=1; repeated int32 values=2; oneof choice{int32 id=3; string key=4;}}
)"},
      {"textproto", "sample.textproto",
       "TextProto fixture for language detection and predefined-style "
       "coverage.",
       R"(foo { bar: 1 baz: "two" nested { enabled: true value: 3 } }
)"},
      {"tablegen", "sample.td",
       "TableGen fixture for AlignConsecutiveTableGenDefinitionColons and "
       "TableGen column-limit coverage.",
       R"(def Demo : Instruction { let OutOperandList = (outs GPR:$dst); let InOperandList = (ins GPR:$src); let AsmString = "demo $dst, $src"; }
)"},
      {"verilog", "sample.sv",
       "Verilog fixture for language detection and column-limit behavior.",
       R"(module top(input logic clk,output logic y);always_ff@(posedge clk)begin y<=~y;end endmodule
)"}};
  return Value;
}

const Fixture &GetFixture(std::string_view Name) {
  const auto &All = Fixtures();
  const auto It = std::find_if(
      All.begin(), All.end(), [&](const Fixture &F) { return F.Name == Name; });
  EXPECT_NE(It, All.end()) << "Unknown fixture: " << Name;
  return *It;
}

const std::array<std::string, 7> &BaseStyles() {
  static const std::array<std::string, 7> Styles = {
      "LLVM", "Google", "Chromium", "Mozilla", "WebKit", "Microsoft", "GNU"};
  return Styles;
}

const std::vector<FocusedStyleCase> &FocusedStyleCases() {
  // Each focused case names the ClangFormatStyleOptions entries it covers and
  // includes a short documentation example. Keep these strings close to the
  // pinned LLVM docs so future agents understand why the fixture exercises the
  // option without reopening the whole generated style-options page.
  static const std::vector<FocusedStyleCase> Cases = {
      {"column-limit-40", "cpp-core", "{BasedOnStyle: LLVM, ColumnLimit: 40}",
       "ColumnLimit",
       "Docs: ColumnLimit is the maximum output width; 0 means no column "
       "limit."},
      {"indent-width-access-modifiers", "cpp-core",
       "{BasedOnStyle: LLVM, IndentWidth: 4, ContinuationIndentWidth: 8, "
       "AccessModifierOffset: -4}",
       "IndentWidth, ContinuationIndentWidth, AccessModifierOffset",
       "Docs: AccessModifierOffset changes the extra indent/outdent for "
       "access labels such as public:."},
      {"allman-braces", "cpp-core",
       "{BasedOnStyle: LLVM, BreakBeforeBraces: Allman}", "BreakBeforeBraces",
       "Docs: BreakBeforeBraces switches between attached braces and styles "
       "such as Allman, where class/function/control braces move to the next "
       "line."},
      {"custom-brace-wrapping", "cpp-core",
       "{BasedOnStyle: LLVM, BreakBeforeBraces: Custom, BraceWrapping: "
       "{AfterClass: true, AfterFunction: true, AfterControlStatement: "
       "Always, BeforeElse: true, IndentBraces: true}}",
       "BreakBeforeBraces, BraceWrapping",
       "Docs: Custom brace wrapping is enabled with BreakBeforeBraces: Custom "
       "and then per-case flags such as AfterClass: true."},
      {"pointer-reference-left", "cpp-modern",
       "{BasedOnStyle: LLVM, DerivePointerAlignment: false, PointerAlignment: "
       "Left, ReferenceAlignment: Left}",
       "DerivePointerAlignment, PointerAlignment, ReferenceAlignment",
       "Docs: PointerAlignment Left formats int* a; ReferenceAlignment Left "
       "formats int& a; disabling derivation keeps the explicit style."},
      {"align-assignments", "cpp-core",
       "{BasedOnStyle: LLVM, AlignConsecutiveAssignments: Consecutive}",
       "AlignConsecutiveAssignments",
       "Docs: Consecutive aligns assignments like int a = 1; int "
       "somelongname = 2; double c = 3; into one '=' column."},
      {"align-declarations", "cpp-core",
       "{BasedOnStyle: LLVM, AlignConsecutiveDeclarations: Consecutive}",
       "AlignConsecutiveDeclarations",
       "Docs: Consecutive aligns declaration names like int aaaa, float b, "
       "and std::string ccc."},
      {"align-macros", "cpp-core",
       "{BasedOnStyle: LLVM, AlignConsecutiveMacros: Consecutive}",
       "AlignConsecutiveMacros",
       "Docs: Consecutive aligns macro bodies like #define SHORT_NAME 42 and "
       "#define EVEN_LONGER_NAME (2)."},
      {"align-bitfields", "cpp-core",
       "{BasedOnStyle: LLVM, AlignConsecutiveBitFields: Consecutive}",
       "AlignConsecutiveBitFields",
       "Docs: Consecutive aligns bit-field colons like int aaaa : 1; int b : "
       "12; int ccc : 8."},
      {"trailing-comments", "cpp-core",
       "{BasedOnStyle: LLVM, AlignTrailingComments: {Kind: Always, "
       "OverEmptyLines: 2}}",
       "AlignTrailingComments",
       "Docs: Kind: Always aligns trailing comments; OverEmptyLines controls "
       "how many blank lines can still participate."},
      {"include-sorting-regroup", "cpp-core",
       "{BasedOnStyle: LLVM, SortIncludes: CaseInsensitive, IncludeBlocks: "
       "Regroup, IncludeCategories: [{Regex: '^<.*', Priority: 2}, {Regex: "
       "'.*', Priority: 1}]}",
       "SortIncludes, IncludeBlocks, IncludeCategories",
       "Docs: IncludeBlocks: Regroup merges include blocks, sorts them, then "
       "splits groups using IncludeCategories priorities."},
      {"short-constructs", "cpp-core",
       "{BasedOnStyle: LLVM, AllowShortFunctionsOnASingleLine: Empty, "
       "AllowShortIfStatementsOnASingleLine: AllIfsAndElse, "
       "AllowShortLoopsOnASingleLine: true}",
       "AllowShortFunctionsOnASingleLine, "
       "AllowShortIfStatementsOnASingleLine, AllowShortLoopsOnASingleLine",
       "Docs: short constructs cover examples like int f() { return 0; }, "
       "if (a) return;, and while (true) continue;."},
      {"bin-pack-off", "cpp-core",
       "{BasedOnStyle: LLVM, BinPackArguments: false, BinPackParameters: "
       "OnePerLine}",
       "BinPackArguments, BinPackParameters",
       "Docs: BinPackArguments: false puts overflowing call arguments one per "
       "line; BinPackParameters: OnePerLine does the same for declarations."},
      {"braced-list-spacing", "cpp-modern",
       "{BasedOnStyle: LLVM, Cpp11BracedListStyle: false, "
       "SpaceBeforeCpp11BracedList: true}",
       "Cpp11BracedListStyle, SpaceBeforeCpp11BracedList",
       "Docs: Cpp11BracedListStyle false keeps spaces inside examples like "
       "vector<int> x{ 1, 2, 3, 4 }."},
      {"qualifier-custom", "cpp-modern",
       "{BasedOnStyle: LLVM, QualifierAlignment: Custom, QualifierOrder: "
       "[inline, static, type, const, volatile]}",
       "QualifierAlignment, QualifierOrder",
       "Docs: QualifierAlignment rewrites qualifier order, e.g. const int a "
       "versus int const a; Custom follows QualifierOrder."},
      {"integer-literal-separators", "cpp-modern",
       "{BasedOnStyle: LLVM, IntegerLiteralSeparator: {Binary: 4, "
       "BinaryMinDigits: 8, Decimal: 3, DecimalMinDigits: 5, Hex: 2, "
       "HexMinDigits: 6}}",
       "IntegerLiteralSeparator",
       "Docs: positive values insert separators from the right, 0 leaves "
       "them, and negative values remove them; e.g. Binary: 4 can produce "
       "0b1001'1110'1101."},
      {"namespace-options", "cpp-core",
       "{BasedOnStyle: LLVM, NamespaceIndentation: All, CompactNamespaces: "
       "true, FixNamespaceComments: true}",
       "NamespaceIndentation, CompactNamespaces, FixNamespaceComments",
       "Docs: NamespaceIndentation: All indents every namespace level; "
       "CompactNamespaces can combine namespace Foo { namespace Bar {; "
       "FixNamespaceComments adds/fixes closing comments."},
      {"comment-and-string-options", "cpp-core",
       "{BasedOnStyle: LLVM, ReflowComments: Never, BreakStringLiterals: "
       "false}",
       "ReflowComments, BreakStringLiterals",
       "Docs: ReflowComments: Never leaves long comments untouched; "
       "BreakStringLiterals: false preserves a long string literal instead of "
       "splitting it."},
      {"objc-options", "objc",
       "{BasedOnStyle: LLVM, ObjCBlockIndentWidth: 4, ObjCSpaceAfterProperty: "
       "true, ObjCSpaceBeforeProtocolList: false}",
       "ObjCBlockIndentWidth, ObjCSpaceAfterProperty, "
       "ObjCSpaceBeforeProtocolList",
       "Docs: ObjCBlockIndentWidth: 4 indents completion blocks by four "
       "columns; the property/protocol flags control Objective-C spacing."},
      {"javascript-options", "javascript",
       "{BasedOnStyle: LLVM, JavaScriptQuotes: Single, JavaScriptWrapImports: "
       "true, ColumnLimit: 60}",
       "JavaScriptQuotes, JavaScriptWrapImports, ColumnLimit",
       "Docs: JavaScriptQuotes: Single rewrites string1 = \"foo\" to "
       "string1 = 'foo'; JavaScriptWrapImports wraps long imports."},
      {"json-options", "json",
       "{BasedOnStyle: LLVM, SpaceBeforeJsonColon: true, ColumnLimit: 40}",
       "SpaceBeforeJsonColon, ColumnLimit",
       "Docs: SpaceBeforeJsonColon true formats JSON as \"key\" : \"value\" "
       "instead of \"key\": \"value\"."},
      {"java-options", "java",
       "{BasedOnStyle: Google, BreakAfterJavaFieldAnnotations: true, "
       "ColumnLimit: 60}",
       "BreakAfterJavaFieldAnnotations, ColumnLimit",
       "Docs: BreakAfterJavaFieldAnnotations true breaks @Partial, @Mock, and "
       "the field declaration onto separate Java lines."},
      {"csharp-options", "csharp",
       "{BasedOnStyle: Microsoft, ColumnLimit: 60, NamespaceIndentation: All}",
       "ColumnLimit, NamespaceIndentation",
       "Docs: C# uses the same ColumnLimit and NamespaceIndentation rules, "
       "with the Microsoft base style."},
      {"proto-disabled", "proto", "{BasedOnStyle: LLVM, DisableFormat: true}",
       "DisableFormat",
       "Docs: the multi-language config example uses Language: Proto with "
       "DisableFormat: true to leave .proto files unchanged."},
      {"tablegen-alignment", "tablegen",
       "{BasedOnStyle: LLVM, AlignConsecutiveTableGenDefinitionColons: "
       "Consecutive, ColumnLimit: 60}",
       "AlignConsecutiveTableGenDefinitionColons, ColumnLimit",
       "Docs: Consecutive aligns TableGen inheritance colons like def Def : "
       "Parent {}, def DefDef : Parent {}."},
      {"verilog-column-limit", "verilog",
       "{BasedOnStyle: LLVM, ColumnLimit: 40, IndentWidth: 4}",
       "ColumnLimit, IndentWidth",
       "Docs: Verilog participates in the same column-limit and indentation "
       "style surface when detected from a .sv filename."}};
  return Cases;
}

const std::vector<CliInvocationCase> &CppCliInvocationCases() {
  static const std::vector<CliInvocationCase> Cases = {
      {"cpp/cli/output-replacements-xml",
       "cpp-core",
       "CLI docs/API compatibility: --output-replacements-xml should report "
       "replacement ranges instead of formatted text.",
       {"-style=LLVM", "--output-replacements-xml", "{file}"}},
      {"cpp/cli/lines-range",
       "cpp-core",
       "CLI range formatting: -lines=1:8 should limit formatting to the same "
       "source line range as upstream.",
       {"-style=LLVM", "-lines=1:8", "{file}"}},
      {"cpp/cli/offset-length",
       "cpp-core",
       "CLI range formatting: -offset and -length select the same byte range "
       "as upstream.",
       {"-style=LLVM", "-offset=0", "-length=80", "{file}"}},
      {"cpp/cli/dry-run",
       "cpp-core",
       "CLI dry run: --dry-run should return diagnostics/status without "
       "writing changes, matching upstream behavior.",
       {"-style=LLVM", "--dry-run", "{file}"}},
      {"cpp/cli/sort-includes-override",
       "cpp-core",
       "CLI override: -sort-includes should override include sorting for the "
       "same include examples used by the docs.",
       {"-style=LLVM", "-sort-includes", "{file}"}},
      {"cpp/cli/qualifier-alignment-override",
       "cpp-modern",
       "CLI override: -qualifier-alignment should exercise the documented "
       "QualifierAlignment/QualifierOrder surface from the command line.",
       {"-style=LLVM", "-qualifier-alignment=type const volatile", "{file}"}}};
  return Cases;
}

std::vector<StyleFixtureCase>
BuildStyleFixtureCases(std::initializer_list<std::string_view> FixtureNames) {
  std::vector<StyleFixtureCase> Cases;
  for (const std::string &Style : BaseStyles()) {
    for (std::string_view FixtureName : FixtureNames) {
      Cases.push_back({"preset/" + Style + "/" + std::string(FixtureName),
                       Style, std::string(FixtureName)});
    }
  }
  return Cases;
}

std::vector<StyleFixtureCase> BuildFullConfigRoundTripCases(
    std::initializer_list<std::string_view> FixtureNames) {
  std::vector<StyleFixtureCase> Cases;
  for (const std::string &Style : BaseStyles()) {
    for (std::string_view FixtureName : FixtureNames) {
      Cases.push_back(
          {"full-config-roundtrip/" + Style + "/" + std::string(FixtureName),
           Style, std::string(FixtureName)});
    }
  }
  return Cases;
}

std::vector<FocusedStyleCase> FocusedStyleCasesForFixtures(
    std::initializer_list<std::string_view> FixtureNames) {
  std::vector<FocusedStyleCase> Cases;
  for (const FocusedStyleCase &Case : FocusedStyleCases()) {
    if (ContainsFixture(FixtureNames, Case.FixtureName))
      Cases.push_back(Case);
  }
  return Cases;
}

std::filesystem::path CaseDirectory(const std::string &Name) {
  return TestOptions.WorkDir / SafeName(Name);
}

void RunStyleFixtureCase(const std::string &Name, const Fixture &F,
                         const std::string &Style) {
  SCOPED_TRACE("Fixture purpose: " + F.Purpose);
  const std::filesystem::path Input = WriteFixture(F, CaseDirectory(Name));
  CompareInvocation(Name, {"-style=" + Style, Input.string()});
}

void RunConfigFixtureCase(const std::string &Name, const Fixture &F,
                          std::string_view ConfigText) {
  SCOPED_TRACE("Fixture purpose: " + F.Purpose);
  const std::filesystem::path Dir = CaseDirectory(Name);
  const std::filesystem::path Config = Dir / ".clang-format";
  WriteFile(Config, ConfigText);
  const std::filesystem::path Input = WriteFixture(F, Dir);
  CompareInvocation(Name, {"-style=file:" + Config.string(), Input.string()});
}

void RunCliInvocationCase(const CliInvocationCase &Case) {
  SCOPED_TRACE(Case.Purpose);
  const std::filesystem::path Input =
      WriteFixture(GetFixture(Case.FixtureName), CaseDirectory(Case.Name));
  std::vector<std::string> Arguments = Case.Arguments;
  for (std::string &Argument : Arguments) {
    if (Argument == "{file}")
      Argument = Input.string();
  }
  CompareInvocation(Case.Name, Arguments);
}

void RunInPlaceFormattingCase(const std::string &Name, const Fixture &F,
                              const std::string &Style) {
  SCOPED_TRACE("Fixture purpose: " + F.Purpose);
  const std::filesystem::path CaseDir = CaseDirectory(Name);
  const std::filesystem::path CustomPath = WriteFixture(F, CaseDir / "custom");
  const std::filesystem::path UpstreamPath =
      WriteFixture(F, CaseDir / "upstream");

  const ProcessResult Custom = RunFormatter(
      TestOptions.CustomClangFormat,
      {"-i", "-style=" + Style, CustomPath.string()}, "custom-" + Name);
  const ProcessResult Upstream = RunFormatter(
      TestOptions.UpstreamClangFormat,
      {"-i", "-style=" + Style, UpstreamPath.string()}, "upstream-" + Name);

  const std::string CustomFile = ReadFile(CustomPath);
  const std::string UpstreamFile = ReadFile(UpstreamPath);
  if (Custom.ExitCode != Upstream.ExitCode ||
      Custom.Stdout != Upstream.Stdout || Custom.Stderr != Upstream.Stderr ||
      CustomFile != UpstreamFile) {
    SaveFailure(Name, {"-i", "-style=" + Style, "<fixture>"}, Custom, Upstream);
  }

  EXPECT_EQ(Custom.ExitCode, Upstream.ExitCode);
  EXPECT_EQ(Custom.Stdout, Upstream.Stdout);
  EXPECT_EQ(Custom.Stderr, Upstream.Stderr);
  EXPECT_EQ(CustomFile, UpstreamFile);
}

} // namespace format_compare

int main(int argc, char **argv) {
  std::vector<char *> FilteredArgs;
  FilteredArgs.push_back(argv[0]);

  for (int I = 1; I < argc; ++I) {
    const std::string Arg = argv[I];
    auto ConsumeValue = [&](std::filesystem::path &Target,
                            std::string_view Prefix) {
      if (Arg.rfind(std::string(Prefix) + "=", 0) == 0) {
        Target = Arg.substr(Prefix.size() + 1);
        return true;
      }
      if (Arg == std::string(Prefix) && I + 1 < argc) {
        Target = argv[++I];
        return true;
      }
      return false;
    };

    if (ConsumeValue(format_compare::TestOptions.CustomClangFormat, "--zcf") ||
        ConsumeValue(format_compare::TestOptions.CustomFormat,
                     "--custom-format-test-driver") ||
        ConsumeValue(format_compare::TestOptions.UpstreamClangFormat,
                     "--upstream-clang-format") ||
        ConsumeValue(format_compare::TestOptions.WorkDir, "--work-dir")) {
      continue;
    }
    FilteredArgs.push_back(argv[I]);
  }

  if (format_compare::TestOptions.WorkDir.empty())
    format_compare::TestOptions.WorkDir =
        std::filesystem::temp_directory_path() / "zcf-gtest";
  std::filesystem::remove_all(format_compare::TestOptions.WorkDir);
  std::filesystem::create_directories(format_compare::TestOptions.WorkDir);

  if (format_compare::TestOptions.CustomFormat.empty()) {
    std::cerr << "--custom-format-test-driver is required\n";
    return 1;
  }
  if (format_compare::TestOptions.CustomClangFormat.empty()) {
    std::cerr << "--zcf is required\n";
    return 1;
  }
  if (format_compare::TestOptions.UpstreamClangFormat.empty()) {
    std::cerr << "--upstream-clang-format is required\n";
    return 1;
  }

  int FilteredArgc = static_cast<int>(FilteredArgs.size());
  ::testing::InitGoogleTest(&FilteredArgc, FilteredArgs.data());

  return RUN_ALL_TESTS();
}

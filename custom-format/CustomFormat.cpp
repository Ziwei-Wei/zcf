#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef ZCF_LLVM_VERSION
#define ZCF_LLVM_VERSION "unknown"
#endif

namespace fs = std::filesystem;

namespace {

enum class Language {
  Auto,
  C,
  Cpp,
  CSharp,
  CMake,
  Swift,
  Python,
  Rust,
  Unknown
};

enum class Backend {
  ClangFormat,
  CMakeFormat,
  SwiftFormat,
  FuturePython,
  FutureRust,
  Unknown
};

struct ForwardArg {
  std::string Value;
  bool IsFile = false;
};

struct Invocation {
  Language ExplicitLanguage = Language::Auto;
  std::string ClangFormatExe;
  std::string CMakeFormatExe;
  std::string SwiftFormatExe;
  bool ListLanguages = false;
  bool PrintBackends = false;
  bool PrintDispatch = false;
  bool Version = false;
  bool Help = false;
  std::optional<std::string> AssumeFilename;
  std::optional<std::string> Style;
  bool HasSwiftConfiguration = false;
  bool HasCMakeConfiguration = false;
  std::vector<ForwardArg> ForwardArgs;
  std::vector<std::string> Files;
};

struct MappedStyle {
  int LineLength = 100;
  int IndentWidth = 2;
  int TabWidth = 8;
  bool UseTabs = false;
  bool HasValues = false;
};

std::string Lower(std::string Value) {
  std::transform(
      Value.begin(), Value.end(), Value.begin(),
      [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
  return Value;
}

bool StartsWith(std::string_view Value, std::string_view Prefix) {
  return Value.size() >= Prefix.size() &&
         Value.substr(0, Prefix.size()) == Prefix;
}

bool EndsWith(std::string_view Value, std::string_view Suffix) {
  return Value.size() >= Suffix.size() &&
         Value.substr(Value.size() - Suffix.size()) == Suffix;
}

std::string ExecutableSuffix() {
#ifdef _WIN32
  return ".exe";
#else
  return "";
#endif
}

char PathListSeparator() {
#ifdef _WIN32
  return ';';
#else
  return ':';
#endif
}

std::string QuoteCommandArgument(std::string_view Arg) {
  std::string Quoted = "\"";
  for (char C : Arg) {
    if (C == '"')
      Quoted += "\\\"";
    else
      Quoted += C;
  }
  Quoted += '"';
  return Quoted;
}

int RunCommand(const std::string &Exe, const std::vector<std::string> &Args) {
  std::string Command;
#ifdef _WIN32
  Command = "call ";
#endif
  Command += QuoteCommandArgument(Exe);
  for (const std::string &Arg : Args) {
    Command += ' ';
    Command += QuoteCommandArgument(Arg);
  }
  return std::system(Command.c_str());
}

std::optional<fs::path> FindOnPath(const std::string &Name) {
  const char *PathEnv = std::getenv("PATH");
  if (!PathEnv)
    return std::nullopt;

  std::vector<std::string> Suffixes = {""};
#ifdef _WIN32
  if (!EndsWith(Name, ".exe")) {
    Suffixes.push_back(".exe");
    Suffixes.push_back(".cmd");
    Suffixes.push_back(".bat");
  }
#endif

  std::stringstream Stream(PathEnv);
  std::string Entry;
  while (std::getline(Stream, Entry, PathListSeparator())) {
    if (Entry.empty())
      Entry = ".";
    for (const std::string &Suffix : Suffixes) {
      fs::path Candidate = fs::path(Entry) / (Name + Suffix);
      std::error_code EC;
      if (fs::is_regular_file(Candidate, EC))
        return Candidate;
    }
  }
  return std::nullopt;
}

Language ParseLanguage(std::string Value) {
  Value = Lower(Value);
  if (Value == "auto")
    return Language::Auto;
  if (Value == "c")
    return Language::C;
  if (Value == "c++" || Value == "cpp" || Value == "cxx")
    return Language::Cpp;
  if (Value == "c#" || Value == "csharp" || Value == "cs")
    return Language::CSharp;
  if (Value == "cmake" || Value == "cmakelists")
    return Language::CMake;
  if (Value == "swift")
    return Language::Swift;
  if (Value == "python" || Value == "py")
    return Language::Python;
  if (Value == "rust" || Value == "rs")
    return Language::Rust;
  return Language::Unknown;
}

std::string LanguageName(Language L) {
  switch (L) {
  case Language::Auto:
    return "auto";
  case Language::C:
    return "c";
  case Language::Cpp:
    return "cpp";
  case Language::CSharp:
    return "csharp";
  case Language::CMake:
    return "cmake";
  case Language::Swift:
    return "swift";
  case Language::Python:
    return "python";
  case Language::Rust:
    return "rust";
  case Language::Unknown:
    return "unknown";
  }
  return "unknown";
}

Backend BackendForLanguage(Language L) {
  switch (L) {
  case Language::C:
  case Language::Cpp:
  case Language::CSharp:
  case Language::Auto:
    return Backend::ClangFormat;
  case Language::CMake:
    return Backend::CMakeFormat;
  case Language::Swift:
    return Backend::SwiftFormat;
  case Language::Python:
    return Backend::FuturePython;
  case Language::Rust:
    return Backend::FutureRust;
  case Language::Unknown:
    return Backend::Unknown;
  }
  return Backend::Unknown;
}

std::string BackendName(Backend B) {
  switch (B) {
  case Backend::ClangFormat:
    return "clang-format";
  case Backend::CMakeFormat:
    return "cmake-format";
  case Backend::SwiftFormat:
    return "swift-format";
  case Backend::FuturePython:
    return "future-python";
  case Backend::FutureRust:
    return "future-rust";
  case Backend::Unknown:
    return "unknown";
  }
  return "unknown";
}

Language DetectLanguageFromPath(std::string_view PathText) {
  if (Lower(fs::path(PathText).filename().string()) == "cmakelists.txt")
    return Language::CMake;
  const std::string Ext = Lower(fs::path(PathText).extension().string());
  if (Ext == ".cmake")
    return Language::CMake;
  if (Ext == ".swift")
    return Language::Swift;
  if (Ext == ".cs")
    return Language::CSharp;
  if (Ext == ".c")
    return Language::C;
  if (Ext == ".cc" || Ext == ".cpp" || Ext == ".cxx" || Ext == ".c++" ||
      Ext == ".h" || Ext == ".hh" || Ext == ".hpp" || Ext == ".hxx" ||
      Ext == ".h++")
    return Language::Cpp;
  if (Ext == ".py")
    return Language::Python;
  if (Ext == ".rs")
    return Language::Rust;
  return Language::Cpp;
}

bool IsSeparateValueOption(std::string_view Arg) {
  static constexpr std::string_view Options[] = {"-style",
                                                 "--style",
                                                 "-fallback-style",
                                                 "--fallback-style",
                                                 "-assume-filename",
                                                 "--assume-filename",
                                                 "-files",
                                                 "--files",
                                                 "-offset",
                                                 "--offset",
                                                 "-length",
                                                 "--length",
                                                 "-lines",
                                                 "--lines",
                                                 "-cursor",
                                                 "--cursor",
                                                 "-ferror-limit",
                                                 "--ferror-limit",
                                                 "-Wno-error",
                                                 "--Wno-error",
                                                 "-qualifier-alignment",
                                                 "--qualifier-alignment",
                                                 "--configuration",
                                                 "-c",
                                                 "--config-files",
                                                 "--line-width",
                                                 "--tab-size",
                                                 "--use-tabchars"};
  return std::find(std::begin(Options), std::end(Options), Arg) !=
         std::end(Options);
}

bool ConsumeValue(int &I, int Argc, const char **Argv, std::string &Value,
                  std::string_view Option, bool &Matched) {
  Matched = false;
  const std::string Arg = Argv[I];
  const std::string Prefix = std::string(Option) + "=";
  if (StartsWith(Arg, Prefix)) {
    Matched = true;
    Value = Arg.substr(Prefix.size());
    return true;
  }
  if (Arg == Option) {
    Matched = true;
    if (I + 1 >= Argc) {
      std::cerr << "error: missing value for " << Option << "\n";
      return false;
    }
    Value = Argv[++I];
    return true;
  }
  return true;
}

bool ParseArgs(int Argc, const char **Argv, Invocation &Inv) {
  for (int I = 1; I < Argc; ++I) {
    const std::string Arg = Argv[I];
    std::string Value;
    bool Matched = false;

    if (Arg == "--") {
      for (++I; I < Argc; ++I) {
        Inv.ForwardArgs.push_back({Argv[I], true});
        Inv.Files.push_back(Argv[I]);
      }
      break;
    }

    if (!ConsumeValue(I, Argc, Argv, Value, "--language", Matched))
      return false;
    if (Matched) {
      Inv.ExplicitLanguage = ParseLanguage(Value);
      if (Inv.ExplicitLanguage == Language::Unknown) {
        std::cerr << "error: unsupported language '" << Value << "'\n";
        return false;
      }
      continue;
    }
    if (!ConsumeValue(I, Argc, Argv, Value, "--clang-format-exe", Matched))
      return false;
    if (Matched) {
      Inv.ClangFormatExe = Value;
      continue;
    }
    if (!ConsumeValue(I, Argc, Argv, Value, "--cmake-format-exe", Matched))
      return false;
    if (Matched) {
      Inv.CMakeFormatExe = Value;
      continue;
    }
    if (!ConsumeValue(I, Argc, Argv, Value, "--swift-format-exe", Matched))
      return false;
    if (Matched) {
      Inv.SwiftFormatExe = Value;
      continue;
    }

    if (Arg == "-h" || Arg == "--help") {
      Inv.Help = true;
      continue;
    }
    if (Arg == "--list-languages") {
      Inv.ListLanguages = true;
      continue;
    }
    if (Arg == "--print-backends") {
      Inv.PrintBackends = true;
      continue;
    }
    if (Arg == "--print-dispatch") {
      Inv.PrintDispatch = true;
      continue;
    }
    if (Arg == "--version" || Arg == "-version") {
      Inv.Version = true;
      continue;
    }

    Inv.ForwardArgs.push_back({Arg, false});
    if (Arg == "-dump-config" || Arg == "--dump-config")
      continue;
    if (Arg == "--configuration")
      Inv.HasSwiftConfiguration = true;
    if (Arg == "-c" || Arg == "--config-files")
      Inv.HasCMakeConfiguration = true;
    if (StartsWith(Arg, "--configuration="))
      Inv.HasSwiftConfiguration = true;
    if (StartsWith(Arg, "--config-files="))
      Inv.HasCMakeConfiguration = true;
    if (StartsWith(Arg, "-style=") || StartsWith(Arg, "--style=")) {
      Inv.Style = Arg.substr(Arg.find('=') + 1);
      continue;
    }
    if (Arg == "-style" || Arg == "--style") {
      if (I + 1 >= Argc) {
        std::cerr << "error: missing value for " << Arg << "\n";
        return false;
      }
      Inv.Style = Argv[I + 1];
    }
    if (StartsWith(Arg, "-assume-filename=") ||
        StartsWith(Arg, "--assume-filename=")) {
      Inv.AssumeFilename = Arg.substr(Arg.find('=') + 1);
      continue;
    }
    if (Arg == "-assume-filename" || Arg == "--assume-filename") {
      if (I + 1 >= Argc) {
        std::cerr << "error: missing value for " << Arg << "\n";
        return false;
      }
      Inv.AssumeFilename = Argv[I + 1];
    }

    if (IsSeparateValueOption(Arg) && I + 1 < Argc) {
      Inv.ForwardArgs.push_back({Argv[++I], false});
      if (Arg == "--configuration")
        Inv.HasSwiftConfiguration = true;
      if (Arg == "-c" || Arg == "--config-files")
        Inv.HasCMakeConfiguration = true;
      continue;
    }

    if (!Arg.empty() && Arg[0] != '-') {
      Inv.ForwardArgs.back().IsFile = true;
      Inv.Files.push_back(Arg);
    }
  }
  return true;
}

void PrintHelp() {
  std::cout
      << "custom-format-test-driver: unified formatter test frontend\n\n"
      << "Usage: custom-format-test-driver [custom options] "
         "[formatter options] "
         "[files...]\n\n"
      << "Custom options:\n"
      << "  --language <auto|c|cpp|csharp|cmake|swift>\n"
      << "      Override language detection. Defaults to auto.\n"
      << "  --clang-format-exe <path>\n"
      << "      Override the C/C++/C# backend executable.\n"
      << "  --cmake-format-exe <path>\n"
      << "      Override the CMake backend executable.\n"
      << "  --swift-format-exe <path>\n"
      << "      Override the Swift backend executable.\n"
      << "  --list-languages\n"
      << "      List currently enabled and future planned languages.\n"
      << "  --print-backends\n"
      << "      Show backend mapping.\n"
      << "  --print-dispatch\n"
      << "      Print selected backend commands before running them.\n\n"
      << "  --version\n"
      << "      Print the zcf and LLVM versions.\n\n"
      << "Formatter options not listed here are forwarded to the selected "
         "backend.\n"
      << "C, C++, and C# use the repo-owned clang-format backend. CMake uses\n"
      << "cmake-format. Swift uses swift-format. CMake and Swift map common\n"
      << "clang-format style keys such as ColumnLimit, IndentWidth, UseTab,\n"
      << "and TabWidth into their backend configuration shapes.\n";
}

void PrintLanguages() {
  std::cout << "enabled:\n"
            << "  c        -> clang-format backend\n"
            << "  cpp      -> clang-format backend\n"
            << "  csharp   -> clang-format backend\n"
            << "  cmake    -> cmake-format backend\n"
            << "  swift    -> swift-format backend\n"
            << "future:\n"
            << "  python   -> not wired yet\n"
            << "  rust     -> not wired yet\n";
}

void PrintBackends() {
  std::cout
      << "clang-format backend:\n"
      << "  languages: c, cpp, csharp\n"
      << "  executable: zcf next to the test driver by default\n"
      << "cmake-format backend:\n"
      << "  languages: cmake\n"
      << "  executable: --cmake-format-exe, ZCF_CMAKE_FORMAT_EXE, or PATH\n"
      << "swift-format backend:\n"
      << "  languages: swift\n"
      << "  executable: --swift-format-exe, ZCF_SWIFT_FORMAT_EXE, or PATH\n";
}

std::vector<std::string> ArgsFromTokens(const std::vector<ForwardArg> &Tokens,
                                        bool IncludeFiles) {
  std::vector<std::string> Args;
  for (const ForwardArg &Token : Tokens) {
    if (IncludeFiles || !Token.IsFile)
      Args.push_back(Token.Value);
  }
  return Args;
}

fs::path FindSiblingExe(const char *Argv0, std::string Name) {
  fs::path CurrentExe = fs::absolute(Argv0);
  fs::path Candidate = CurrentExe.parent_path() / (Name + ExecutableSuffix());
  std::error_code EC;
  if (fs::is_regular_file(Candidate, EC))
    return Candidate;
  return Name + ExecutableSuffix();
}

std::string ResolveClangFormatExe(const Invocation &Inv, const char *Argv0) {
  if (!Inv.ClangFormatExe.empty())
    return Inv.ClangFormatExe;
  return FindSiblingExe(Argv0, "zcf").string();
}

std::string ResolveCMakeFormatExe(const Invocation &Inv) {
  if (!Inv.CMakeFormatExe.empty())
    return Inv.CMakeFormatExe;
  if (const char *Env = std::getenv("ZCF_CMAKE_FORMAT_EXE"))
    return Env;
  if (std::optional<fs::path> Found = FindOnPath("cmake-format"))
    return Found->string();
  return "";
}

std::string ResolveSwiftFormatExe(const Invocation &Inv) {
  if (!Inv.SwiftFormatExe.empty())
    return Inv.SwiftFormatExe;
  if (const char *Env = std::getenv("ZCF_SWIFT_FORMAT_EXE"))
    return Env;
  if (std::optional<fs::path> Found = FindOnPath("swift-format"))
    return Found->string();
  return "";
}

bool ReadFile(const fs::path &Path, std::string &Content) {
  std::ifstream Input(Path, std::ios::binary);
  if (!Input)
    return false;
  std::ostringstream Buffer;
  Buffer << Input.rdbuf();
  Content = Buffer.str();
  return true;
}

std::optional<fs::path>
FindClangFormatConfig(const std::vector<std::string> &Files) {
  fs::path Dir = Files.empty() ? fs::current_path()
                               : fs::path(Files.front()).parent_path();
  if (Dir.empty())
    Dir = fs::current_path();
  std::error_code EC;
  Dir = fs::absolute(Dir, EC);
  if (EC)
    return std::nullopt;

  for (;;) {
    for (const char *Name : {".clang-format", "_clang-format"}) {
      fs::path Candidate = Dir / Name;
      if (fs::is_regular_file(Candidate, EC))
        return Candidate;
    }
    fs::path Parent = Dir.parent_path();
    if (Parent == Dir || Parent.empty())
      break;
    Dir = Parent;
  }
  return std::nullopt;
}

std::optional<int> ExtractUnsigned(std::string_view Text, const char *Key) {
  const std::regex Pattern(std::string(Key) + R"(\s*:\s*([0-9]+))",
                           std::regex::icase);
  std::cmatch Match;
  const std::string Copy(Text);
  if (!std::regex_search(Copy.c_str(), Match, Pattern))
    return std::nullopt;
  return std::stoi(Match[1].str());
}

std::optional<std::string> ExtractWord(std::string_view Text, const char *Key) {
  const std::regex Pattern(std::string(Key) + R"(\s*:\s*([A-Za-z]+))",
                           std::regex::icase);
  std::cmatch Match;
  const std::string Copy(Text);
  if (!std::regex_search(Copy.c_str(), Match, Pattern))
    return std::nullopt;
  return Match[1].str();
}

void ApplyStyleName(std::string Name, MappedStyle &Style) {
  Name = Lower(Name);
  if (Name == "llvm" || Name == "google" || Name == "chromium" ||
      Name == "mozilla" || Name == "webkit" || Name == "gnu") {
    Style.LineLength = 80;
    Style.IndentWidth = 2;
    Style.UseTabs = false;
    Style.HasValues = true;
  } else if (Name == "microsoft") {
    Style.LineLength = 120;
    Style.IndentWidth = 4;
    Style.UseTabs = false;
    Style.HasValues = true;
  }
}

void ApplyClangStyleText(std::string_view Text, MappedStyle &Style) {
  if (std::optional<int> ColumnLimit = ExtractUnsigned(Text, "ColumnLimit")) {
    if (*ColumnLimit > 0)
      Style.LineLength = *ColumnLimit;
    Style.HasValues = true;
  }
  if (std::optional<int> IndentWidth = ExtractUnsigned(Text, "IndentWidth")) {
    Style.IndentWidth = *IndentWidth;
    Style.HasValues = true;
  }
  if (std::optional<int> TabWidth = ExtractUnsigned(Text, "TabWidth")) {
    Style.TabWidth = *TabWidth;
    Style.HasValues = true;
  }
  if (std::optional<std::string> UseTab = ExtractWord(Text, "UseTab")) {
    const std::string Value = Lower(*UseTab);
    Style.UseTabs = Value == "always" || Value == "forindentation";
    Style.HasValues = true;
  }
}

void ApplyClangStyle(const Invocation &Inv, MappedStyle &Style) {
  if (!Inv.Style)
    return;

  std::string StyleText = *Inv.Style;
  const std::string LowerStyle = Lower(StyleText);
  if (LowerStyle == "file" || StartsWith(LowerStyle, "file:")) {
    std::optional<fs::path> ConfigPath;
    if (StartsWith(LowerStyle, "file:"))
      ConfigPath = StyleText.substr(std::string("file:").size());
    else
      ConfigPath = FindClangFormatConfig(Inv.Files);

    if (ConfigPath) {
      std::string ConfigText;
      if (ReadFile(*ConfigPath, ConfigText))
        ApplyClangStyleText(ConfigText, Style);
    }
    return;
  }

  ApplyStyleName(StyleText, Style);
  ApplyClangStyleText(StyleText, Style);
}

std::optional<fs::path> WriteSwiftConfig(const MappedStyle &Style) {
  if (!Style.HasValues)
    return std::nullopt;

  const auto Ticks =
      std::chrono::steady_clock::now().time_since_epoch().count();
  fs::path Path = fs::temp_directory_path() /
                  ("zcf-swift-" + std::to_string(Ticks) + ".json");
  std::ofstream Output(Path, std::ios::binary | std::ios::trunc);
  if (!Output) {
    std::cerr << "error: unable to write temporary Swift configuration: "
              << Path.string() << "\n";
    return std::nullopt;
  }

  Output << "{\n"
         << "  \"version\": 1,\n"
         << "  \"lineLength\": " << Style.LineLength << ",\n"
         << "  \"indentation\": { \"" << (Style.UseTabs ? "tabs" : "spaces")
         << "\": " << (Style.UseTabs ? 1 : Style.IndentWidth) << " },\n"
         << "  \"tabWidth\": " << Style.TabWidth << "\n"
         << "}\n";
  return Path;
}

void AppendCMakeStyleArgs(const MappedStyle &Style,
                          std::vector<std::string> &Args) {
  if (!Style.HasValues)
    return;
  Args.push_back("--line-width");
  Args.push_back(std::to_string(Style.LineLength));
  Args.push_back("--tab-size");
  Args.push_back(std::to_string(Style.IndentWidth));
  if (Style.UseTabs) {
    Args.push_back("--use-tabchars");
    Args.push_back("True");
  }
}

bool IsUnsupportedSwiftOption(std::string_view Arg) {
  return Arg == "--output-replacements-xml" ||
         Arg == "-output-replacements-xml" || Arg == "-lines" ||
         StartsWith(Arg, "-lines=") || Arg == "--lines" ||
         StartsWith(Arg, "--lines=") || Arg == "-offset" ||
         StartsWith(Arg, "-offset=") || Arg == "--offset" ||
         StartsWith(Arg, "--offset=") || Arg == "-length" ||
         StartsWith(Arg, "-length=") || Arg == "--length" ||
         StartsWith(Arg, "--length=") || Arg == "-cursor" ||
         StartsWith(Arg, "-cursor=") || Arg == "--cursor" ||
         StartsWith(Arg, "--cursor=") || Arg == "-sort-includes" ||
         StartsWith(Arg, "-sort-includes=") || Arg == "--sort-includes" ||
         StartsWith(Arg, "--sort-includes=") || Arg == "-qualifier-alignment" ||
         StartsWith(Arg, "-qualifier-alignment=") ||
         Arg == "--qualifier-alignment" ||
         StartsWith(Arg, "--qualifier-alignment=");
}

bool BuildSwiftArgs(const Invocation &Inv,
                    const std::vector<std::string> &Input,
                    std::vector<std::string> &Output,
                    std::optional<fs::path> &TempConfig) {
  bool DumpConfig = false;
  bool Lint = false;
  for (size_t I = 0; I < Input.size(); ++I) {
    const std::string &Arg = Input[I];
    if (Arg == "-dump-config" || Arg == "--dump-config") {
      DumpConfig = true;
      continue;
    }
    if (Arg == "--dry-run" || Arg == "-n") {
      Lint = true;
      continue;
    }
    if (Arg == "-style" || Arg == "--style" || Arg == "-fallback-style" ||
        Arg == "--fallback-style") {
      ++I;
      continue;
    }
    if (StartsWith(Arg, "-style=") || StartsWith(Arg, "--style=") ||
        StartsWith(Arg, "-fallback-style=") ||
        StartsWith(Arg, "--fallback-style=")) {
      continue;
    }
    if (IsUnsupportedSwiftOption(Arg)) {
      std::cerr << "error: Swift backend does not support clang-format option '"
                << Arg << "' yet\n";
      return false;
    }
    Output.push_back(Arg);
  }

  if (DumpConfig) {
    Output.insert(Output.begin(), "dump-configuration");
    return true;
  }

  Output.insert(Output.begin(), Lint ? "lint" : "format");
  if (!Inv.HasSwiftConfiguration) {
    MappedStyle Style;
    ApplyClangStyle(Inv, Style);
    TempConfig = WriteSwiftConfig(Style);
    if (TempConfig) {
      Output.insert(Output.begin() + 1, TempConfig->string());
      Output.insert(Output.begin() + 1, "--configuration");
    }
  }
  return true;
}

int RunClangBackend(const Invocation &Inv, const char *Argv0,
                    const std::vector<std::string> &Args) {
  const std::string Exe = ResolveClangFormatExe(Inv, Argv0);
  if (Inv.PrintDispatch)
    std::cerr << "dispatch: " << Exe << " -> clang-format backend\n";
  return RunCommand(Exe, Args);
}

bool IsUnsupportedCMakeOption(std::string_view Arg) {
  return Arg == "--output-replacements-xml" ||
         Arg == "-output-replacements-xml" || Arg == "-lines" ||
         StartsWith(Arg, "-lines=") || Arg == "--lines" ||
         StartsWith(Arg, "--lines=") || Arg == "-offset" ||
         StartsWith(Arg, "-offset=") || Arg == "--offset" ||
         StartsWith(Arg, "--offset=") || Arg == "-length" ||
         StartsWith(Arg, "-length=") || Arg == "--length" ||
         StartsWith(Arg, "--length=") || Arg == "-cursor" ||
         StartsWith(Arg, "-cursor=") || Arg == "--cursor" ||
         StartsWith(Arg, "--cursor=") || Arg == "-sort-includes" ||
         StartsWith(Arg, "-sort-includes=") || Arg == "--sort-includes" ||
         StartsWith(Arg, "--sort-includes=") || Arg == "-qualifier-alignment" ||
         StartsWith(Arg, "-qualifier-alignment=") ||
         Arg == "--qualifier-alignment" ||
         StartsWith(Arg, "--qualifier-alignment=");
}

bool BuildCMakeArgs(const Invocation &Inv,
                    const std::vector<std::string> &Input,
                    std::vector<std::string> &Output) {
  bool SawDumpConfig = false;
  bool SawCheck = false;
  for (size_t I = 0; I < Input.size(); ++I) {
    const std::string &Arg = Input[I];
    if (Arg == "-dump-config" || Arg == "--dump-config") {
      SawDumpConfig = true;
      Output.push_back("--dump-config");
      continue;
    }
    if (Arg == "--dry-run" || Arg == "-n") {
      SawCheck = true;
      Output.push_back("--check");
      continue;
    }
    if (Arg == "-style" || Arg == "--style" || Arg == "-fallback-style" ||
        Arg == "--fallback-style") {
      ++I;
      continue;
    }
    if (StartsWith(Arg, "-style=") || StartsWith(Arg, "--style=") ||
        StartsWith(Arg, "-fallback-style=") ||
        StartsWith(Arg, "--fallback-style=")) {
      continue;
    }
    if (IsUnsupportedCMakeOption(Arg)) {
      std::cerr << "error: CMake backend does not support clang-format option '"
                << Arg << "' yet\n";
      return false;
    }
    Output.push_back(Arg);
  }

  if (!SawDumpConfig && !SawCheck && !Inv.HasCMakeConfiguration) {
    MappedStyle Style;
    ApplyClangStyle(Inv, Style);
    std::vector<std::string> StyleArgs;
    AppendCMakeStyleArgs(Style, StyleArgs);
    Output.insert(Output.begin(), StyleArgs.begin(), StyleArgs.end());
  }
  return true;
}

int RunCMakeBackend(const Invocation &Inv,
                    const std::vector<std::string> &Args) {
  const std::string Exe = ResolveCMakeFormatExe(Inv);
  if (Exe.empty()) {
    std::cerr << "error: CMake backend requires cmake-format. Install "
                 "cmake-format, put it on PATH, set ZCF_CMAKE_FORMAT_EXE, "
                 "or pass --cmake-format-exe.\n";
    return 1;
  }

  std::vector<std::string> CMakeArgs;
  if (!BuildCMakeArgs(Inv, Args, CMakeArgs))
    return 1;

  if (Inv.PrintDispatch)
    std::cerr << "dispatch: " << Exe << " -> cmake-format backend\n";
  return RunCommand(Exe, CMakeArgs);
}

int RunSwiftBackend(const Invocation &Inv,
                    const std::vector<std::string> &Args) {
  const std::string Exe = ResolveSwiftFormatExe(Inv);
  if (Exe.empty()) {
    std::cerr << "error: Swift backend requires swift-format. Install the "
                 "Swift toolchain, put swift-format on PATH, set "
                 "ZCF_SWIFT_FORMAT_EXE, or pass --swift-format-exe.\n";
    return 1;
  }

  std::vector<std::string> SwiftArgs;
  std::optional<fs::path> TempConfig;
  if (!BuildSwiftArgs(Inv, Args, SwiftArgs, TempConfig))
    return 1;

  if (Inv.PrintDispatch)
    std::cerr << "dispatch: " << Exe << " -> swift-format backend\n";
  const int Result = RunCommand(Exe, SwiftArgs);
  if (TempConfig) {
    std::error_code EC;
    fs::remove(*TempConfig, EC);
  }
  return Result;
}

int RunBackend(Backend B, const Invocation &Inv, const char *Argv0,
               const std::vector<std::string> &Args) {
  switch (B) {
  case Backend::ClangFormat:
    return RunClangBackend(Inv, Argv0, Args);
  case Backend::CMakeFormat:
    return RunCMakeBackend(Inv, Args);
  case Backend::SwiftFormat:
    return RunSwiftBackend(Inv, Args);
  case Backend::FuturePython:
    std::cerr << "error: Python backend is planned but not wired yet\n";
    return 1;
  case Backend::FutureRust:
    std::cerr << "error: Rust backend is planned but not wired yet\n";
    return 1;
  case Backend::Unknown:
    std::cerr << "error: unable to select formatter backend\n";
    return 1;
  }
  return 1;
}

Language DetectInvocationLanguage(const Invocation &Inv,
                                  const std::string &File = "") {
  if (Inv.ExplicitLanguage != Language::Auto)
    return Inv.ExplicitLanguage;
  if (!File.empty())
    return DetectLanguageFromPath(File);
  if (Inv.AssumeFilename)
    return DetectLanguageFromPath(*Inv.AssumeFilename);
  return Language::Cpp;
}

int RunFormattedInvocation(const Invocation &Inv, const char *Argv0) {
  if (Inv.Files.empty()) {
    const Language L = DetectInvocationLanguage(Inv);
    return RunBackend(BackendForLanguage(L), Inv, Argv0,
                      ArgsFromTokens(Inv.ForwardArgs, true));
  }

  std::vector<Backend> Backends;
  for (const std::string &File : Inv.Files) {
    const Language L = DetectInvocationLanguage(Inv, File);
    Backends.push_back(BackendForLanguage(L));
  }

  const bool OneBackend =
      std::all_of(Backends.begin(), Backends.end(),
                  [&](Backend B) { return B == Backends.front(); });
  if (OneBackend)
    return RunBackend(Backends.front(), Inv, Argv0,
                      ArgsFromTokens(Inv.ForwardArgs, true));

  std::vector<std::string> CommonArgs = ArgsFromTokens(Inv.ForwardArgs, false);
  int Result = 0;
  for (size_t I = 0; I < Inv.Files.size(); ++I) {
    std::vector<std::string> Args = CommonArgs;
    Args.push_back(Inv.Files[I]);
    const int BackendResult = RunBackend(Backends[I], Inv, Argv0, Args);
    if (BackendResult != 0 && Result == 0)
      Result = BackendResult;
  }
  return Result;
}

} // namespace

int main(int argc, const char **argv) {
  Invocation Inv;
  if (!ParseArgs(argc, argv, Inv))
    return 1;

  if (Inv.Help) {
    PrintHelp();
    return 0;
  }
  if (Inv.Version) {
    std::cout << "custom-format-test-driver (LLVM " << ZCF_LLVM_VERSION
              << ")\n";
    return 0;
  }
  if (Inv.ListLanguages) {
    PrintLanguages();
    return 0;
  }
  if (Inv.PrintBackends) {
    PrintBackends();
    return 0;
  }

  const Language L = Inv.ExplicitLanguage == Language::Auto
                         ? Language::Auto
                         : Inv.ExplicitLanguage;
  if (L == Language::Python || L == Language::Rust) {
    std::cerr << "error: " << LanguageName(L)
              << " is planned but intentionally left for a future backend\n";
    return 1;
  }

  return RunFormattedInvocation(Inv, argv[0]);
}

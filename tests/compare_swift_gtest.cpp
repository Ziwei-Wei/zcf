#include "format_comparison_support.h"

#include <sstream>

namespace {

TEST(SwiftBackendComparison, DispatchesToSwiftFormatWithMappedStyle) {
#ifndef _WIN32
  GTEST_SKIP() << "The deterministic fake swift-format script is Windows-only.";
#else
  const std::filesystem::path Dir =
      format_compare::CaseDirectory("swift/dispatch");
  const std::filesystem::path Input = Dir / "Sample.swift";
  const std::filesystem::path FakeSwiftFormat = Dir / "fake-swift-format.cmd";
  const std::filesystem::path CapturedConfig = Dir / "captured-config.json";
  const std::filesystem::path CapturedArgs = Dir / "captured-args.txt";

  format_compare::WriteFile(Input, R"(func f(){print("hello")})");

  std::ostringstream Script;
  Script << "@echo off\n"
         << "echo %* > \"" << CapturedArgs.string() << "\"\n"
         << "if \"%~1\"==\"dump-configuration\" (\n"
         << "  echo {\"version\":1}\n"
         << "  exit /b 0\n"
         << ")\n"
         << "if \"%~1\"==\"format\" (\n"
         << "  shift\n"
         << ")\n"
         << ":next\n"
         << "if \"%~1\"==\"\" goto run\n"
         << "if \"%~1\"==\"--configuration\" (\n"
         << "  set \"config=%~2\"\n"
         << "  shift\n"
         << "  shift\n"
         << "  goto next\n"
         << ")\n"
         << "set \"input=%~1\"\n"
         << "shift\n"
         << "goto next\n"
         << ":run\n"
         << "if defined config copy \"%config%\" \"" << CapturedConfig.string()
         << "\" >nul\n"
         << "if defined input type \"%input%\"\n"
         << "exit /b 0\n";
  format_compare::WriteFile(FakeSwiftFormat, Script.str());

  const format_compare::ProcessResult Result = format_compare::RunFormatter(
      format_compare::TestOptions.CustomFormat,
      {"--language=swift", "--swift-format-exe=" + FakeSwiftFormat.string(),
       "-style={BasedOnStyle: LLVM, ColumnLimit: 88, IndentWidth: 4}",
       Input.string()},
      "custom-swift-dispatch");

  ASSERT_EQ(Result.ExitCode, 0) << Result.Stderr;
  EXPECT_EQ(Result.Stdout, R"(func f(){print("hello")})");

  const std::string Args = format_compare::ReadFile(CapturedArgs);
  EXPECT_NE(Args.find("\"format\""), std::string::npos);
  EXPECT_NE(Args.find("\"--configuration\""), std::string::npos);
  EXPECT_NE(Args.find(Input.string()), std::string::npos);

  const std::string Config = format_compare::ReadFile(CapturedConfig);
  EXPECT_NE(Config.find("\"lineLength\": 88"), std::string::npos);
  EXPECT_NE(Config.find("\"spaces\": 4"), std::string::npos);
#endif
}

} // namespace

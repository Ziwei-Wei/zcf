#include "format_comparison_support.h"

#include <sstream>

namespace {

TEST(CMakeBackendComparison, DispatchesToCMakeFormatWithMappedStyle) {
#ifndef _WIN32
  GTEST_SKIP() << "The deterministic fake cmake-format script is Windows-only.";
#else
  const std::filesystem::path Dir =
      format_compare::CaseDirectory("cmake/dispatch");
  const format_compare::Fixture &Fixture = format_compare::GetFixture("cmake");
  const std::filesystem::path Input =
      format_compare::WriteFixture(Fixture, Dir);
  const std::filesystem::path FakeCMakeFormat = Dir / "fake-cmake-format.cmd";
  const std::filesystem::path CapturedArgs = Dir / "captured-args.txt";

  std::ostringstream Script;
  Script << "@echo off\n"
         << "echo %* > \"" << CapturedArgs.string() << "\"\n"
         << ":next\n"
         << "if \"%~1\"==\"\" goto run\n"
         << "if \"%~1\"==\"--dump-config\" (\n"
         << "  echo line_width = 80\n"
         << "  exit /b 0\n"
         << ")\n"
         << "if \"%~1\"==\"--line-width\" (\n"
         << "  shift\n"
         << "  shift\n"
         << "  goto next\n"
         << ")\n"
         << "if \"%~1\"==\"--tab-size\" (\n"
         << "  shift\n"
         << "  shift\n"
         << "  goto next\n"
         << ")\n"
         << "if \"%~1\"==\"--use-tabchars\" (\n"
         << "  shift\n"
         << "  shift\n"
         << "  goto next\n"
         << ")\n"
         << "set \"input=%~1\"\n"
         << "shift\n"
         << "goto next\n"
         << ":run\n"
         << "if defined input type \"%input%\"\n"
         << "exit /b 0\n";
  format_compare::WriteFile(FakeCMakeFormat, Script.str());

  const format_compare::ProcessResult Result = format_compare::RunFormatter(
      format_compare::TestOptions.CustomFormat,
      {"--language=cmake", "--cmake-format-exe=" + FakeCMakeFormat.string(),
       "-style={BasedOnStyle: LLVM, ColumnLimit: 88, IndentWidth: 4}",
       Input.string()},
      "custom-cmake-dispatch");

  ASSERT_EQ(Result.ExitCode, 0) << Result.Stderr;
  EXPECT_EQ(Result.Stdout, Fixture.Text);

  const std::string Args = format_compare::ReadFile(CapturedArgs);
  EXPECT_NE(Args.find("\"--line-width\" \"88\""), std::string::npos);
  EXPECT_NE(Args.find("\"--tab-size\" \"4\""), std::string::npos);
  EXPECT_NE(Args.find(Input.string()), std::string::npos);
#endif
}

} // namespace

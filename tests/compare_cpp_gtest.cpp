#include "format_language_comparison.h"

FORMAT_COMPARE_DEFINE_LANGUAGE_CORE(Cpp, ({"cpp-core", "cpp-modern"}))
FORMAT_COMPARE_DEFINE_LANGUAGE_FOCUSED(Cpp, ({"cpp-core", "cpp-modern"}))

namespace {

class CppCliBehaviorComparison
    : public ::testing::TestWithParam<format_compare::CliInvocationCase> {};

TEST_P(CppCliBehaviorComparison, MatchesUpstream) {
  format_compare::RunCliInvocationCase(GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    Cpp, CppCliBehaviorComparison,
    ::testing::ValuesIn(format_compare::CppCliInvocationCases()),
    [](const ::testing::TestParamInfo<format_compare::CliInvocationCase>
           &Info) { return format_compare::GTestName(Info.param.Name); });

TEST(CppCliBehaviorComparison, InPlaceFormattingMatchesUpstream) {
  const format_compare::Fixture &Cpp = format_compare::GetFixture("cpp-core");
  SCOPED_TRACE("CLI in-place formatting: -i should rewrite the input file "
               "using the same documented C++ fixture examples as upstream.");
  format_compare::RunInPlaceFormattingCase("cpp/cli/in-place", Cpp, "LLVM");
}

} // namespace

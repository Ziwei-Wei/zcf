#include "format_comparison_support.h"

namespace {

class DumpConfigComparison : public ::testing::TestWithParam<std::string> {};

TEST_P(DumpConfigComparison, MatchesUpstream) {
  const std::string &Style = GetParam();
  SCOPED_TRACE("Docs: -dump-config emits the complete YAML option surface for "
               "a predefined style.");
  format_compare::CompareInvocation("dump-config/" + Style,
                                    {"-style=" + Style, "-dump-config"});
}

INSTANTIATE_TEST_SUITE_P(Styles, DumpConfigComparison,
                         ::testing::ValuesIn(format_compare::BaseStyles()),
                         [](const ::testing::TestParamInfo<std::string> &Info) {
                           return format_compare::GTestName(Info.param);
                         });

} // namespace

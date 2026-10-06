#pragma once

#include "format_comparison_support.h"

#define FORMAT_COMPARE_DEFINE_LANGUAGE_CORE(SuiteToken, FixtureArgs)           \
  namespace {                                                                  \
  const std::vector<format_compare::StyleFixtureCase> &                        \
  SuiteToken##PresetCases() {                                                  \
    static const std::vector<format_compare::StyleFixtureCase> Cases =         \
        format_compare::BuildStyleFixtureCases FixtureArgs;                    \
    return Cases;                                                              \
  }                                                                            \
                                                                               \
  const std::vector<format_compare::StyleFixtureCase> &                        \
  SuiteToken##RoundTripCases() {                                               \
    static const std::vector<format_compare::StyleFixtureCase> Cases =         \
        format_compare::BuildFullConfigRoundTripCases FixtureArgs;             \
    return Cases;                                                              \
  }                                                                            \
                                                                               \
  class SuiteToken##PresetComparison                                           \
      : public ::testing::TestWithParam<format_compare::StyleFixtureCase> {};  \
                                                                               \
  TEST_P(SuiteToken##PresetComparison, MatchesUpstream) {                      \
    const format_compare::StyleFixtureCase &Case = GetParam();                 \
    SCOPED_TRACE("Predefined style: " + Case.Style);                           \
    format_compare::RunStyleFixtureCase(                                       \
        Case.Name, format_compare::GetFixture(Case.FixtureName), Case.Style);  \
  }                                                                            \
                                                                               \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      SuiteToken, SuiteToken##PresetComparison,                                \
      ::testing::ValuesIn(SuiteToken##PresetCases()),                          \
      [](const ::testing::TestParamInfo<format_compare::StyleFixtureCase>      \
             &Info) { return format_compare::GTestName(Info.param.Name); });   \
                                                                               \
  class SuiteToken##FullConfigRoundTripComparison                              \
      : public ::testing::TestWithParam<format_compare::StyleFixtureCase> {};  \
                                                                               \
  TEST_P(SuiteToken##FullConfigRoundTripComparison, MatchesUpstream) {         \
    const format_compare::StyleFixtureCase &Case = GetParam();                 \
    SCOPED_TRACE("Docs: clang-format -style=<name> -dump-config produces a "   \
                 "complete .clang-format file that should round-trip through " \
                 "-style=file:<path>.");                                       \
    const std::string Config = format_compare::UpstreamDumpConfig(Case.Style); \
    format_compare::RunConfigFixtureCase(                                      \
        Case.Name, format_compare::GetFixture(Case.FixtureName), Config);      \
  }                                                                            \
                                                                               \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      SuiteToken, SuiteToken##FullConfigRoundTripComparison,                   \
      ::testing::ValuesIn(SuiteToken##RoundTripCases()),                       \
      [](const ::testing::TestParamInfo<format_compare::StyleFixtureCase>      \
             &Info) { return format_compare::GTestName(Info.param.Name); });   \
  }

#define FORMAT_COMPARE_DEFINE_LANGUAGE_FOCUSED(SuiteToken, FixtureArgs)        \
  namespace {                                                                  \
  const std::vector<format_compare::FocusedStyleCase> &                        \
  SuiteToken##FocusedCases() {                                                 \
    static const std::vector<format_compare::FocusedStyleCase> Cases =         \
        format_compare::FocusedStyleCasesForFixtures FixtureArgs;              \
    return Cases;                                                              \
  }                                                                            \
                                                                               \
  class SuiteToken##FocusedOptionComparison                                    \
      : public ::testing::TestWithParam<format_compare::FocusedStyleCase> {};  \
                                                                               \
  TEST_P(SuiteToken##FocusedOptionComparison, MatchesUpstream) {               \
    const format_compare::FocusedStyleCase &Case = GetParam();                 \
    SCOPED_TRACE("Covered options: " + Case.CoveredOptions + "\n" +            \
                 Case.DocExample);                                             \
    format_compare::RunConfigFixtureCase(                                      \
        "focused/" + Case.Name, format_compare::GetFixture(Case.FixtureName),  \
        Case.Style);                                                           \
  }                                                                            \
                                                                               \
  INSTANTIATE_TEST_SUITE_P(                                                    \
      SuiteToken, SuiteToken##FocusedOptionComparison,                         \
      ::testing::ValuesIn(SuiteToken##FocusedCases()),                         \
      [](const ::testing::TestParamInfo<format_compare::FocusedStyleCase>      \
             &Info) { return format_compare::GTestName(Info.param.Name); });   \
  }

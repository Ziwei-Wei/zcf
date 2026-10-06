#pragma once

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace format_compare {

struct Options {
  std::filesystem::path CustomFormat;
  std::filesystem::path CustomClangFormat;
  std::filesystem::path UpstreamClangFormat;
  std::filesystem::path WorkDir;
};

struct ProcessResult {
  int ExitCode = -1;
  std::string Stdout;
  std::string Stderr;
};

struct Fixture {
  std::string Name;
  std::string FileName;
  std::string Purpose;
  std::string Text;
};

struct FocusedStyleCase {
  std::string Name;
  std::string FixtureName;
  std::string Style;
  std::string CoveredOptions;
  std::string DocExample;
};

struct StyleFixtureCase {
  std::string Name;
  std::string Style;
  std::string FixtureName;
};

struct CliInvocationCase {
  std::string Name;
  std::string FixtureName;
  std::string Purpose;
  std::vector<std::string> Arguments;
};

extern Options TestOptions;

std::string ReadFile(const std::filesystem::path &Path);
void WriteFile(const std::filesystem::path &Path, std::string_view Content);
std::string SafeName(std::string Name);
std::string GTestName(std::string Name);

ProcessResult RunFormatter(const std::filesystem::path &Exe,
                           const std::vector<std::string> &Arguments,
                           const std::string &Name);
void SaveFailure(const std::string &Name, const std::vector<std::string> &Args,
                 const ProcessResult &Custom, const ProcessResult &Upstream);
void CompareInvocation(const std::string &Name,
                       const std::vector<std::string> &Arguments);
std::string UpstreamDumpConfig(const std::string &Style);

std::filesystem::path WriteFixture(const Fixture &F,
                                   const std::filesystem::path &Directory);
const std::vector<Fixture> &Fixtures();
const Fixture &GetFixture(std::string_view Name);
const std::array<std::string, 7> &BaseStyles();
const std::vector<FocusedStyleCase> &FocusedStyleCases();
const std::vector<CliInvocationCase> &CppCliInvocationCases();

std::vector<StyleFixtureCase>
BuildStyleFixtureCases(std::initializer_list<std::string_view> FixtureNames);
std::vector<StyleFixtureCase> BuildFullConfigRoundTripCases(
    std::initializer_list<std::string_view> FixtureNames);
std::vector<FocusedStyleCase> FocusedStyleCasesForFixtures(
    std::initializer_list<std::string_view> FixtureNames);

std::filesystem::path CaseDirectory(const std::string &Name);
void RunStyleFixtureCase(const std::string &Name, const Fixture &F,
                         const std::string &Style);
void RunConfigFixtureCase(const std::string &Name, const Fixture &F,
                          std::string_view ConfigText);
void RunCliInvocationCase(const CliInvocationCase &Case);
void RunInPlaceFormattingCase(const std::string &Name, const Fixture &F,
                              const std::string &Style);

} // namespace format_compare

#ifndef CUSTOM_CLANG_FORMAT_P1_HOOKS_H
#define CUSTOM_CLANG_FORMAT_P1_HOOKS_H

#include "clang/Tooling/Core/Replacement.h"

#include <optional>
#include <utility>

namespace clang {
namespace format {

class AnnotatedLine;
class Environment;
struct FormatStyle;
struct FormatToken;
struct LineState;

namespace extensions {

void customizeP1AnnotatedLine(AnnotatedLine &Line, const FormatStyle &Style);
std::optional<bool> getP1SpaceRequiredBefore(const AnnotatedLine &Line,
                                             const FormatToken &Right);
std::optional<unsigned> getP1NewLineColumn(const LineState &State,
                                           const FormatStyle &Style);
std::pair<tooling::Replacements, unsigned>
runP1PostFormatPass(const Environment &Env, const FormatStyle &Style);

} // namespace extensions
} // namespace format
} // namespace clang

#endif

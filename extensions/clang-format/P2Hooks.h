#ifndef CUSTOM_CLANG_FORMAT_P2_HOOKS_H
#define CUSTOM_CLANG_FORMAT_P2_HOOKS_H

#include "clang/Tooling/Core/Replacement.h"

#include <optional>
#include <utility>

namespace clang::format {

class AnnotatedLine;
class Environment;
struct FormatStyle;
struct FormatToken;
struct LineState;

namespace extensions {

void customizeP2AnnotatedLine(AnnotatedLine &Line);
std::optional<bool> getP2SpaceRequiredBefore(const AnnotatedLine &Line,
                                             const FormatToken &Right);
std::optional<unsigned> getP2NewLineColumn(const LineState &State,
                                           const FormatStyle &Style);
std::pair<tooling::Replacements, unsigned>
runP2PostFormatPass(const Environment &Env, const FormatStyle &Style);

} // namespace extensions
} // namespace clang::format

#endif

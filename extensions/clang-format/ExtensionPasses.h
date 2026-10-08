#ifndef ZCF_EXTENSION_PASSES_H
#define ZCF_EXTENSION_PASSES_H

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

void customizeSyntaxLayout(AnnotatedLine &Line, const FormatStyle &Style);
void customizeDeclarationLayout(AnnotatedLine &Line);

std::optional<bool> getDeclaratorSpaceRequiredBefore(const AnnotatedLine &Line,
                                                     const FormatToken &Right);
std::optional<bool> getSpecifierSpaceRequiredBefore(const AnnotatedLine &Line,
                                                    const FormatToken &Right);

std::optional<unsigned> getSyntaxNewLineColumn(const LineState &State,
                                               const FormatStyle &Style);
std::optional<unsigned> getDeclarationNewLineColumn(const LineState &State,
                                                    const FormatStyle &Style);

// Returns true when clang-format requires a break between adjacent string
// literals inside the parenthesized list, so compacting it would override
// BreakAdjacentStringLiterals.
bool hasRequiredStringLiteralBreak(const FormatToken &LeftParen,
                                   const FormatStyle &Style);

std::pair<tooling::Replacements, unsigned>
runStructuralPostFormatPass(const Environment &Env, const FormatStyle &Style);
std::pair<tooling::Replacements, unsigned>
runBoundaryPostFormatPass(const Environment &Env, const FormatStyle &Style);

} // namespace extensions
} // namespace clang::format

#endif

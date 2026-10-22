#ifndef ZCF_EXTENSION_PASSES_H
#define ZCF_EXTENSION_PASSES_H

#include <optional>

namespace clang::format {

class AnnotatedLine;
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

// Token-layout helpers shared by the layout hooks.

bool isNonEmptyParen(const FormatToken &LeftParen);
// A non-empty parenthesized list after a callable expression, excluding
// declarations, attributes, conditions, and lambda or operator parameters.
bool isCallParen(const AnnotatedLine &Line, const FormatToken &LeftParen);
// Returns true when clang-format requires a break between adjacent string
// literals inside the parenthesized list, so compacting it would override
// BreakAdjacentStringLiterals.
bool hasRequiredStringLiteralBreak(const FormatToken &LeftParen,
                                   const FormatStyle &Style);

void forceBreakBefore(FormatToken &Token);
// Places each top-level list element and the closing parenthesis on its own
// line.
void forceMultilineList(FormatToken &LeftParen);
// TokenAnnotator computes TotalLength before extension hooks run. Adding its
// forced-break length from the first newly forced token keeps the formatter's
// single-line fast path and LineJoiner from discarding that break.
void addForcedBreakLength(FormatToken &FirstForcedBreak,
                          const FormatStyle &Style);

} // namespace extensions
} // namespace clang::format

#endif

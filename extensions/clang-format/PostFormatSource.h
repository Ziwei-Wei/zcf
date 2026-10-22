#ifndef ZCF_POST_FORMAT_SOURCE_H
#define ZCF_POST_FORMAT_SOURCE_H

#include "AffectedRangeManager.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Format/Format.h"
#include "clang/Lex/Token.h"
#include "clang/Tooling/Core/Replacement.h"
#include "llvm/ADT/StringRef.h"

#include <optional>
#include <string>
#include <vector>

namespace clang::format {

class Environment;

namespace extensions {

struct ExtensionStyle;

struct PendingReplacement {
  unsigned BeginOffset = 0;
  unsigned EndOffset = 0;
  std::string Text;
  std::string Context;
};

struct TokenInfo {
  Token Tok;
  StringRef Text;
  unsigned Offset = 0;
  unsigned EndOffset = 0;
  unsigned LineIndex = 0;
  unsigned BraceDepth = 0;
  unsigned ParenDepth = 0;
  unsigned SquareDepth = 0;
  bool Disabled = false;
};

struct PhysicalLine {
  unsigned StartOffset = 0;
  unsigned ContentEndOffset = 0;
  unsigned EndOffset = 0;
  std::vector<unsigned> Tokens;
  std::optional<unsigned> FirstToken;
  std::optional<unsigned> FirstNonCommentToken;
  std::optional<unsigned> LastNonCommentToken;
  bool BlankText = true;
  bool CommentOnly = false;
  bool InPPDirective = false;
  bool Disabled = false;
};

// Raw tokens and physical lines of LibFormat's output, scanned once and shared
// by every post-format rule. Matching tables hold the partner token index or
// -1.
struct PostFormatSource {
  StringRef Code;
  SourceLocation StartOfFile;
  StringRef DefaultLineEnding;
  std::vector<PhysicalLine> Lines;
  std::vector<TokenInfo> Tokens;
  std::vector<int> MatchingParens;
  std::vector<int> MatchingBraces;
  std::vector<int> MatchingSquares;
};

PostFormatSource scanPostFormatSource(const Environment &Env,
                                      const FormatStyle &Style);

// Converts queued edits to replacements, dropping exact duplicates and
// reporting overlapping edits from different rules.
tooling::Replacements
buildReplacements(const std::vector<PendingReplacement> &Pending,
                  const SourceManager &SourceManager,
                  SourceLocation StartOfFile);

void collectStructuralFixes(const PostFormatSource &Source,
                            const FormatStyle &Style,
                            const ExtensionStyle &Extensions,
                            AffectedRangeManager &AffectedRangeMgr,
                            std::vector<PendingReplacement> &Pending);
void collectBoundaryFixes(const PostFormatSource &Source,
                          const FormatStyle &Style,
                          const ExtensionStyle &Extensions,
                          AffectedRangeManager &AffectedRangeMgr,
                          std::vector<PendingReplacement> &Pending);

StringRef getLineText(StringRef Code, const PhysicalLine &Line);
StringRef getExistingText(StringRef Code, unsigned BeginOffset,
                          unsigned EndOffset);
StringRef getLineIndent(StringRef Code, const PhysicalLine &Line);

// Return Lines.size() when no such line exists.
unsigned previousNonBlankLine(const std::vector<PhysicalLine> &Lines,
                              unsigned LineIndex);
unsigned nextNonBlankLine(const std::vector<PhysicalLine> &Lines,
                          unsigned LineIndex);

// Return -1 when no such token exists.
int nextNonCommentToken(const std::vector<TokenInfo> &Tokens, int Index);
int nextNonCommentTokenOnLine(const std::vector<TokenInfo> &Tokens, int Index,
                              unsigned LineIndex);

// Uses the fallback range when the primary range is empty, such as when an
// edit inserts text between adjacent tokens.
bool rangeAffected(AffectedRangeManager &AffectedRangeMgr,
                   SourceLocation StartOfFile, unsigned BeginOffset,
                   unsigned EndOffset, unsigned FallbackBeginOffset,
                   unsigned FallbackEndOffset);
void queueReplacement(std::vector<PendingReplacement> &Pending, StringRef Code,
                      unsigned BeginOffset, unsigned EndOffset,
                      std::string Text, StringRef Context);

unsigned countNewlines(StringRef Text);
// Keeps the existing indentation and line-ending style while setting the number
// of line breaks.
std::string buildLeadingWhitespace(StringRef ExistingWhitespace,
                                   unsigned DesiredNewlines,
                                   StringRef DefaultLineEnding);
unsigned measureColumn(StringRef Text, unsigned TabWidth);
// Extends BaseIndent by Columns, emitting tabs when the style's UseTab setting
// allows them.
std::string buildIndentText(StringRef BaseIndent, unsigned Columns,
                            const FormatStyle &Style);

} // namespace extensions
} // namespace clang::format

#endif

#include "extensions/clang-format/PostFormatSource.h"

#include "extensions/clang-format/ExtensionStyle.h"
#include "llvm/ADT/StringRef.h"

#include <optional>
#include <string>
#include <vector>

namespace clang::format::extensions {
namespace {

bool isIdentifierLike(const TokenInfo &Token) {
  return Token.Tok.isOneOf(tok::identifier, tok::raw_identifier);
}

bool hasCommentOnLine(const PhysicalLine &Line,
                      const std::vector<TokenInfo> &Tokens) {
  for (unsigned TokenIndex : Line.Tokens) {
    if (Tokens[TokenIndex].Tok.is(tok::comment))
      return true;
  }
  return false;
}

std::optional<unsigned>
findOrdinaryLabelColon(const PhysicalLine &Line,
                       const std::vector<TokenInfo> &Tokens) {
  if (Line.BlankText || Line.CommentOnly || Line.InPPDirective ||
      Line.Disabled || !Line.FirstNonCommentToken ||
      !Line.LastNonCommentToken) {
    return std::nullopt;
  }

  const unsigned LabelIndex = *Line.FirstNonCommentToken;
  if (!isIdentifierLike(Tokens[LabelIndex]) ||
      Tokens[LabelIndex].Text == "case" ||
      Tokens[LabelIndex].Text == "default") {
    return std::nullopt;
  }

  const int ColonIndex = nextNonCommentTokenOnLine(
      Tokens, static_cast<int>(LabelIndex), Tokens[LabelIndex].LineIndex);
  if (ColonIndex < 0 || Tokens[ColonIndex].Tok.isNot(tok::colon) ||
      static_cast<unsigned>(ColonIndex) != *Line.LastNonCommentToken) {
    return std::nullopt;
  }
  return static_cast<unsigned>(ColonIndex);
}

std::optional<unsigned>
findAttributedLabelColon(const PhysicalLine &Line,
                         const std::vector<TokenInfo> &Tokens,
                         const std::vector<int> &MatchingSquares) {
  if (Line.BlankText || Line.CommentOnly || Line.InPPDirective ||
      Line.Disabled || !Line.FirstNonCommentToken ||
      !Line.LastNonCommentToken) {
    return std::nullopt;
  }

  const unsigned LineIndex = Tokens[*Line.FirstNonCommentToken].LineIndex;
  int Cursor = static_cast<int>(*Line.FirstNonCommentToken);
  bool SawAttribute = false;
  while (Cursor >= 0 && Tokens[Cursor].Tok.is(tok::l_square)) {
    const int InnerOpen = nextNonCommentTokenOnLine(Tokens, Cursor, LineIndex);
    if (InnerOpen < 0 || Tokens[InnerOpen].Tok.isNot(tok::l_square))
      break;
    const int OuterClose = MatchingSquares[Cursor];
    if (OuterClose < 0 || Tokens[OuterClose].LineIndex != LineIndex)
      return std::nullopt;
    SawAttribute = true;
    Cursor = nextNonCommentTokenOnLine(Tokens, OuterClose, LineIndex);
    if (Cursor < 0)
      return std::nullopt;
  }

  if (!SawAttribute || Cursor < 0 || !isIdentifierLike(Tokens[Cursor]))
    return std::nullopt;

  const int ColonIndex = nextNonCommentTokenOnLine(Tokens, Cursor, LineIndex);
  if (ColonIndex < 0 || Tokens[ColonIndex].Tok.isNot(tok::colon) ||
      static_cast<unsigned>(ColonIndex) != *Line.LastNonCommentToken) {
    return std::nullopt;
  }
  return static_cast<unsigned>(ColonIndex);
}

void collectBlankLineAroundLabelFixes(
    const std::vector<TokenInfo> &Tokens,
    const std::vector<PhysicalLine> &Lines,
    AffectedRangeManager &AffectedRangeMgr, SourceLocation StartOfFile,
    StringRef Code, StringRef DefaultLineEnding,
    std::vector<PendingReplacement> &Pending) {
  for (const PhysicalLine &Line : Lines) {
    const std::optional<unsigned> ColonIndex =
        findOrdinaryLabelColon(Line, Tokens);
    if (!ColonIndex)
      continue;

    const unsigned PrevLineIndex =
        previousNonBlankLine(Lines, Tokens[*ColonIndex].LineIndex);
    if (PrevLineIndex < Lines.size() && !Lines[PrevLineIndex].CommentOnly &&
        !Lines[PrevLineIndex].Disabled) {
      const unsigned BeginOffset = Lines[PrevLineIndex].ContentEndOffset;
      const unsigned EndOffset = Tokens[*Line.FirstToken].Offset;
      if (rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                        Tokens[*Line.FirstNonCommentToken].Offset,
                        Tokens[*ColonIndex].EndOffset)) {
        queueSingleBlankLine(Pending, Code, BeginOffset, EndOffset,
                             DefaultLineEnding,
                             "BlankLinesAroundLabels(before)");
      }
    }

    const unsigned NextLineIndex =
        nextNonBlankLine(Lines, Tokens[*ColonIndex].LineIndex);
    if (NextLineIndex >= Lines.size() || Lines[NextLineIndex].CommentOnly ||
        Lines[NextLineIndex].Disabled ||
        !Lines[NextLineIndex].FirstNonCommentToken) {
      continue;
    }

    const unsigned BeginOffset = Line.ContentEndOffset;
    const unsigned EndOffset = Tokens[*Lines[NextLineIndex].FirstToken].Offset;
    if (!rangeAffected(
            AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
            Tokens[*Lines[NextLineIndex].FirstNonCommentToken].Offset,
            Tokens[*Lines[NextLineIndex].FirstNonCommentToken].EndOffset)) {
      continue;
    }

    queueSingleBlankLine(Pending, Code, BeginOffset, EndOffset,
                         DefaultLineEnding, "BlankLinesAroundLabels(after)");
  }
}

void collectSwitchCaseBlockSeparationFixes(
    const std::vector<TokenInfo> &Tokens,
    const std::vector<PhysicalLine> &Lines,
    AffectedRangeManager &AffectedRangeMgr, SourceLocation StartOfFile,
    StringRef Code, StringRef DefaultLineEnding,
    std::vector<PendingReplacement> &Pending) {
  for (const PhysicalLine &Line : Lines) {
    if (Line.BlankText || Line.CommentOnly || Line.InPPDirective ||
        Line.Disabled || !Line.FirstNonCommentToken) {
      continue;
    }

    const TokenInfo &CaseToken = Tokens[*Line.FirstNonCommentToken];
    if (CaseToken.Text != "case" && CaseToken.Text != "default")
      continue;

    const unsigned PreviousLineIndex =
        previousNonBlankLine(Lines, CaseToken.LineIndex);
    if (PreviousLineIndex >= Lines.size())
      continue;
    const PhysicalLine &PreviousLine = Lines[PreviousLineIndex];
    if (PreviousLine.CommentOnly || PreviousLine.Disabled ||
        !PreviousLine.LastNonCommentToken) {
      continue;
    }

    const TokenInfo &PreviousToken = Tokens[*PreviousLine.LastNonCommentToken];
    if (PreviousToken.Tok.isNot(tok::r_brace) ||
        PreviousToken.BraceDepth != CaseToken.BraceDepth + 1) {
      continue;
    }

    const unsigned BeginOffset = PreviousLine.ContentEndOffset;
    const unsigned EndOffset = Tokens[*Line.FirstToken].Offset;
    if (!rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                       CaseToken.Offset, CaseToken.EndOffset)) {
      continue;
    }

    queueSingleBlankLine(Pending, Code, BeginOffset, EndOffset,
                         DefaultLineEnding, "SeparateSwitchCaseBlocks");
  }
}

void collectAttributedLabelIndentFixes(
    const std::vector<TokenInfo> &Tokens,
    const std::vector<PhysicalLine> &Lines,
    const std::vector<int> &MatchingSquares,
    AffectedRangeManager &AffectedRangeMgr, SourceLocation StartOfFile,
    StringRef Code, const FormatStyle &Style,
    std::vector<PendingReplacement> &Pending) {
  for (const PhysicalLine &Line : Lines) {
    if (!findAttributedLabelColon(Line, Tokens, MatchingSquares))
      continue;

    const unsigned FirstTokenIndex = Line.Tokens.front();
    const unsigned LabelDepth = Tokens[FirstTokenIndex].BraceDepth;
    int OpenBraceIndex = static_cast<int>(FirstTokenIndex) - 1;
    while (OpenBraceIndex >= 0 &&
           (Tokens[OpenBraceIndex].Tok.isNot(tok::l_brace) ||
            Tokens[OpenBraceIndex].BraceDepth + 1 != LabelDepth)) {
      --OpenBraceIndex;
    }
    if (OpenBraceIndex < 0)
      continue;

    const unsigned BeginOffset = Line.StartOffset;
    const unsigned EndOffset = Tokens[FirstTokenIndex].Offset;
    if (!rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                       Tokens[FirstTokenIndex].Offset,
                       Tokens[FirstTokenIndex].EndOffset)) {
      continue;
    }

    // Statements in a block are indented one level past the line that opens
    // the block, independent of namespace and ordinary-label indentation.
    const StringRef BlockIndent =
        getLineIndent(Code, Lines[Tokens[OpenBraceIndex].LineIndex]);
    queueReplacement(Pending, Code, BeginOffset, EndOffset,
                     buildIndentText(BlockIndent, Style.IndentWidth, Style),
                     "IndentAttributedLabels");
  }
}

void collectClosingDirectiveCommentFixes(
    const std::vector<TokenInfo> &, const std::vector<PhysicalLine> &Lines,
    AffectedRangeManager &AffectedRangeMgr, SourceLocation StartOfFile,
    StringRef Code, std::vector<PendingReplacement> &Pending) {
  for (const PhysicalLine &Line : Lines) {
    if (Line.BlankText || Line.CommentOnly || !Line.InPPDirective ||
        Line.Disabled) {
      continue;
    }

    const StringRef Text = getLineText(Code, Line);
    size_t Cursor = Text.find_first_not_of(" \t");
    if (Cursor == StringRef::npos || Text[Cursor] != '#')
      continue;
    ++Cursor;
    Cursor = Text.find_first_not_of(" \t", Cursor);
    if (Cursor == StringRef::npos)
      continue;

    const StringRef Remaining = Text.drop_front(Cursor);
    StringRef Directive;
    if (Remaining.starts_with("else"))
      Directive = "else";
    else if (Remaining.starts_with("endif"))
      Directive = "endif";
    else
      continue;
    const size_t DirectiveEnd = Cursor + Directive.size();
    if (DirectiveEnd < Text.size() && Text[DirectiveEnd] != ' ' &&
        Text[DirectiveEnd] != '\t' && Text[DirectiveEnd] != '/') {
      continue;
    }

    const size_t Comment = Text.find("//", DirectiveEnd);
    if (Comment == StringRef::npos ||
        !Text.slice(DirectiveEnd, Comment).trim().empty()) {
      continue;
    }

    const unsigned BeginOffset =
        Line.StartOffset + static_cast<unsigned>(DirectiveEnd);
    const unsigned EndOffset =
        Line.StartOffset + static_cast<unsigned>(Comment);
    const unsigned CommentEnd = Line.ContentEndOffset;
    if (!rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                       EndOffset, CommentEnd)) {
      continue;
    }

    queueReplacement(Pending, Code, BeginOffset, EndOffset, " ",
                     "SeparateClosingDirectiveComments");
  }
}

void collectDeletedFunctionSeparationFixes(
    const std::vector<TokenInfo> &Tokens,
    const std::vector<PhysicalLine> &Lines,
    AffectedRangeManager &AffectedRangeMgr, SourceLocation StartOfFile,
    StringRef Code, StringRef DefaultLineEnding,
    std::vector<PendingReplacement> &Pending) {
  for (const PhysicalLine &Line : Lines) {
    if (Line.Disabled || Line.InPPDirective || !Line.LastNonCommentToken)
      continue;
    bool IsDeletedDeclaration = false;
    for (unsigned TokenIndex : Line.Tokens) {
      if (Tokens[TokenIndex].Tok.isNot(tok::equal))
        continue;
      const int DeleteIndex = nextNonCommentTokenOnLine(
          Tokens, static_cast<int>(TokenIndex), Tokens[TokenIndex].LineIndex);
      if (DeleteIndex >= 0 && Tokens[DeleteIndex].Text == "delete") {
        IsDeletedDeclaration = true;
        break;
      }
    }
    if (!IsDeletedDeclaration)
      continue;

    const unsigned NextLineIndex =
        nextNonBlankLine(Lines, Tokens[*Line.LastNonCommentToken].LineIndex);
    if (NextLineIndex >= Lines.size() ||
        !Lines[NextLineIndex].FirstNonCommentToken ||
        Tokens[*Lines[NextLineIndex].FirstNonCommentToken].Tok.is(tok::r_brace))
      continue;

    const unsigned BeginOffset = Line.ContentEndOffset;
    const unsigned EndOffset = Tokens[*Lines[NextLineIndex].FirstToken].Offset;
    if (!rangeAffected(
            AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset, EndOffset,
            Tokens[*Lines[NextLineIndex].FirstNonCommentToken].EndOffset))
      continue;
    queueSingleBlankLine(Pending, Code, BeginOffset, EndOffset,
                         DefaultLineEnding, "SeparateDeletedFunctions");
  }
}

bool isQualifiedComponentLine(const PhysicalLine &Line,
                              const std::vector<TokenInfo> &Tokens) {
  if (Line.BlankText || Line.CommentOnly || Line.InPPDirective ||
      Line.Disabled || !Line.FirstNonCommentToken ||
      !Line.LastNonCommentToken) {
    return false;
  }
  const unsigned FirstIndex = *Line.FirstNonCommentToken;
  const unsigned LastIndex = *Line.LastNonCommentToken;
  if (!isIdentifierLike(Tokens[FirstIndex]) ||
      Tokens[LastIndex].Tok.isNot(tok::coloncolon) ||
      !hasCommentOnLine(Line, Tokens)) {
    return false;
  }
  for (unsigned TokenIndex : Line.Tokens) {
    const TokenInfo &Token = Tokens[TokenIndex];
    if (Token.Tok.is(tok::comment))
      continue;
    if (Token.Tok.isOneOf(tok::l_brace, tok::r_brace, tok::l_paren,
                          tok::r_paren, tok::semi)) {
      return false;
    }
  }
  return true;
}

std::optional<unsigned>
findFunctionNameLParenOnLine(const PhysicalLine &Line,
                             const std::vector<TokenInfo> &Tokens) {
  if (Line.BlankText || Line.CommentOnly || Line.InPPDirective ||
      Line.Disabled || !Line.FirstNonCommentToken ||
      !Line.LastNonCommentToken) {
    return std::nullopt;
  }
  if (!isIdentifierLike(Tokens[*Line.FirstNonCommentToken])) {
    return std::nullopt;
  }

  const int LParenIndex = nextNonCommentTokenOnLine(
      Tokens, static_cast<int>(*Line.FirstNonCommentToken),
      Tokens[*Line.FirstNonCommentToken].LineIndex);
  if (LParenIndex < 0 || Tokens[LParenIndex].Tok.isNot(tok::l_paren)) {
    return std::nullopt;
  }
  return static_cast<unsigned>(LParenIndex);
}

bool isFunctionDeclaratorTerminator(const std::vector<TokenInfo> &Tokens,
                                    const std::vector<PhysicalLine> &Lines,
                                    unsigned CloseParenIndex) {
  const int SameLineNext =
      nextNonCommentTokenOnLine(Tokens, static_cast<int>(CloseParenIndex),
                                Tokens[CloseParenIndex].LineIndex);
  if (SameLineNext >= 0)
    return Tokens[SameLineNext].Tok.isOneOf(tok::semi, tok::l_brace);

  const unsigned NextLineIndex =
      nextNonBlankLine(Lines, Tokens[CloseParenIndex].LineIndex);
  if (NextLineIndex >= Lines.size() || Lines[NextLineIndex].CommentOnly ||
      Lines[NextLineIndex].Disabled ||
      !Lines[NextLineIndex].FirstNonCommentToken) {
    return false;
  }
  return Tokens[*Lines[NextLineIndex].FirstNonCommentToken].Tok.isOneOf(
      tok::semi, tok::l_brace);
}

void collectQualifiedFunctionNameDeindentFixes(
    const std::vector<TokenInfo> &Tokens,
    const std::vector<PhysicalLine> &Lines,
    const std::vector<int> &MatchingParens,
    AffectedRangeManager &AffectedRangeMgr, SourceLocation StartOfFile,
    StringRef Code, std::vector<PendingReplacement> &Pending) {
  for (unsigned LineIndex = 0; LineIndex < Lines.size(); ++LineIndex) {
    const std::optional<unsigned> LParenIndex =
        findFunctionNameLParenOnLine(Lines[LineIndex], Tokens);
    if (!LParenIndex)
      continue;
    if (!Lines[LineIndex].FirstNonCommentToken ||
        Tokens[*Lines[LineIndex].FirstNonCommentToken].BraceDepth != 0)
      continue;

    const StringRef BaseIndent = getLineIndent(Code, Lines[LineIndex]);
    unsigned StartLineIndex = LineIndex;
    bool SawQualifiedComponent = false;
    while (StartLineIndex > 0) {
      const unsigned CandidateLineIndex = StartLineIndex - 1;
      if (Lines[CandidateLineIndex].BlankText ||
          Lines[CandidateLineIndex].CommentOnly ||
          Lines[CandidateLineIndex].InPPDirective ||
          Lines[CandidateLineIndex].Disabled ||
          getLineIndent(Code, Lines[CandidateLineIndex]) != BaseIndent ||
          !isQualifiedComponentLine(Lines[CandidateLineIndex], Tokens)) {
        break;
      }
      StartLineIndex = CandidateLineIndex;
      SawQualifiedComponent = true;
    }
    if (!SawQualifiedComponent)
      continue;

    const unsigned BeforeStartLine =
        previousNonBlankLine(Lines, StartLineIndex);
    if (BeforeStartLine >= Lines.size() ||
        (!BaseIndent.empty() &&
         getLineIndent(Code, Lines[BeforeStartLine]).size() >=
             BaseIndent.size())) {
      continue;
    }
    bool AssignmentContext = false;
    for (unsigned TokenIndex : Lines[BeforeStartLine].Tokens)
      AssignmentContext |= Tokens[TokenIndex].Tok.is(tok::equal);
    if (AssignmentContext)
      continue;

    if (Tokens[*LParenIndex].ParenDepth != 0 ||
        Tokens[*LParenIndex].SquareDepth != 0) {
      continue;
    }

    const int CloseParenIndex = MatchingParens[*LParenIndex];
    if (CloseParenIndex < 0 ||
        !isFunctionDeclaratorTerminator(
            Tokens, Lines, static_cast<unsigned>(CloseParenIndex))) {
      continue;
    }

    const unsigned EndLineIndex = Tokens[CloseParenIndex].LineIndex;
    bool SafeBlock = true;
    for (unsigned I = StartLineIndex; I <= EndLineIndex; ++I) {
      const PhysicalLine &Line = Lines[I];
      if (Line.BlankText || Line.CommentOnly || Line.InPPDirective ||
          Line.Disabled || !Line.FirstNonCommentToken) {
        SafeBlock = false;
        break;
      }
      const StringRef Indent = getLineIndent(Code, Line);
      if (!Indent.starts_with(BaseIndent)) {
        SafeBlock = false;
        break;
      }
    }
    if (!SafeBlock)
      continue;

    for (unsigned I = StartLineIndex; I <= EndLineIndex; ++I) {
      const PhysicalLine &Line = Lines[I];
      const unsigned BeginOffset = Line.StartOffset;
      const unsigned EndOffset = Tokens[*Line.FirstNonCommentToken].Offset;
      if (BeginOffset == EndOffset)
        continue;
      if (!rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                         Tokens[*Line.FirstNonCommentToken].Offset,
                         Tokens[*Line.FirstNonCommentToken].EndOffset)) {
        continue;
      }
      const StringRef Indent = getLineIndent(Code, Line);
      queueReplacement(Pending, Code, BeginOffset, EndOffset,
                       Indent.drop_front(BaseIndent.size()).str(),
                       "DeindentQualifiedFunctionNames");
    }

    LineIndex = EndLineIndex;
  }
}

} // namespace

void collectBoundaryFixes(const PostFormatSource &Source,
                          const FormatStyle &Style,
                          const ExtensionStyle &Extensions,
                          AffectedRangeManager &AffectedRangeMgr,
                          std::vector<PendingReplacement> &Pending) {
  const auto &Tokens = Source.Tokens;
  const auto &Lines = Source.Lines;
  const StringRef Code = Source.Code;
  const SourceLocation StartOfFile = Source.StartOfFile;
  const StringRef DefaultLineEnding = Source.DefaultLineEnding;

  if (Extensions.BlankLinesAroundLabels) {
    collectBlankLineAroundLabelFixes(Tokens, Lines, AffectedRangeMgr,
                                     StartOfFile, Code, DefaultLineEnding,
                                     Pending);
  }
  if (Extensions.SeparateSwitchCaseBlocks) {
    collectSwitchCaseBlockSeparationFixes(Tokens, Lines, AffectedRangeMgr,
                                          StartOfFile, Code, DefaultLineEnding,
                                          Pending);
  }
  if (Extensions.IndentAttributedLabels) {
    collectAttributedLabelIndentFixes(Tokens, Lines, Source.MatchingSquares,
                                      AffectedRangeMgr, StartOfFile, Code,
                                      Style, Pending);
  }
  if (Extensions.SeparateClosingDirectiveComments) {
    collectClosingDirectiveCommentFixes(Tokens, Lines, AffectedRangeMgr,
                                        StartOfFile, Code, Pending);
  }
  if (Extensions.SeparateDeletedFunctions) {
    collectDeletedFunctionSeparationFixes(Tokens, Lines, AffectedRangeMgr,
                                          StartOfFile, Code, DefaultLineEnding,
                                          Pending);
  }
  if (Extensions.DeindentQualifiedFunctionNames) {
    collectQualifiedFunctionNameDeindentFixes(
        Tokens, Lines, Source.MatchingParens, AffectedRangeMgr, StartOfFile,
        Code, Pending);
  }
}

} // namespace clang::format::extensions

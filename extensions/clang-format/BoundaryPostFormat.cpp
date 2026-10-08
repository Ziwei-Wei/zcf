#include "extensions/clang-format/ExtensionPasses.h"

#include "AffectedRangeManager.h"
#include "TokenAnalyzer.h"
#include "integration/clang-format/ExtensionContext.h"
#include "clang/Format/Format.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace clang::format::extensions {
namespace {

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
  std::optional<unsigned> FirstNonCommentToken;
  std::optional<unsigned> LastNonCommentToken;
  bool BlankText = true;
  bool CommentOnly = false;
  bool InPPDirective = false;
  bool Disabled = false;
};

bool isBlankText(StringRef Text) { return Text.trim(" \t\f\v").empty(); }

bool endsWithBackslash(StringRef Text) {
  Text = Text.rtrim(" \t\f\v");
  return !Text.empty() && Text.back() == '\\';
}

bool isIdentifierLike(const TokenInfo &Token) {
  return Token.Tok.isOneOf(tok::identifier, tok::raw_identifier);
}

std::vector<PhysicalLine> buildLines(StringRef Code) {
  std::vector<PhysicalLine> Lines;
  unsigned Start = 0;
  for (unsigned I = 0, E = Code.size(); I < E; ++I) {
    if (Code[I] == '\r' && I + 1 < E && Code[I + 1] == '\n') {
      Lines.push_back({Start,
                       I,
                       I + 2,
                       {},
                       std::nullopt,
                       std::nullopt,
                       isBlankText(Code.slice(Start, I)),
                       false,
                       false,
                       false});
      Start = I + 2;
      ++I;
    } else if (Code[I] == '\n') {
      Lines.push_back({Start,
                       I,
                       I + 1,
                       {},
                       std::nullopt,
                       std::nullopt,
                       isBlankText(Code.slice(Start, I)),
                       false,
                       false,
                       false});
      Start = I + 1;
    }
  }
  if (Start < Code.size() || Lines.empty()) {
    Lines.push_back({Start,
                     static_cast<unsigned>(Code.size()),
                     static_cast<unsigned>(Code.size()),
                     {},
                     std::nullopt,
                     std::nullopt,
                     isBlankText(Code.drop_front(Start)),
                     false,
                     false,
                     false});
  }
  return Lines;
}

unsigned findLineIndex(const std::vector<PhysicalLine> &Lines,
                       unsigned Offset) {
  auto It = std::upper_bound(Lines.begin(), Lines.end(), Offset,
                             [](unsigned Value, const PhysicalLine &Line) {
                               return Value < Line.StartOffset;
                             });
  if (It == Lines.begin())
    return 0;
  return static_cast<unsigned>(std::prev(It) - Lines.begin());
}

StringRef getLineText(StringRef Code, const PhysicalLine &Line) {
  return Code.slice(Line.StartOffset, Line.ContentEndOffset);
}

StringRef getExistingText(StringRef Code, unsigned BeginOffset,
                          unsigned EndOffset) {
  return Code.slice(BeginOffset, EndOffset);
}

unsigned previousNonBlankLine(const std::vector<PhysicalLine> &Lines,
                              unsigned LineIndex) {
  while (LineIndex > 0) {
    --LineIndex;
    if (!Lines[LineIndex].BlankText)
      return LineIndex;
  }
  return Lines.size();
}

unsigned nextNonBlankLine(const std::vector<PhysicalLine> &Lines,
                          unsigned LineIndex) {
  for (++LineIndex; LineIndex < Lines.size(); ++LineIndex) {
    if (!Lines[LineIndex].BlankText)
      return LineIndex;
  }
  return Lines.size();
}

int nextNonCommentTokenOnLine(const std::vector<TokenInfo> &Tokens, int Index,
                              unsigned LineIndex) {
  for (++Index; Index >= 0 && Index < static_cast<int>(Tokens.size());
       ++Index) {
    if (Tokens[Index].LineIndex != LineIndex)
      break;
    if (Tokens[Index].Tok.isNot(tok::comment))
      return Index;
  }
  return -1;
}

bool rangeAffected(AffectedRangeManager &AffectedRangeMgr,
                   SourceLocation StartOfFile, unsigned BeginOffset,
                   unsigned EndOffset, unsigned FallbackBeginOffset,
                   unsigned FallbackEndOffset) {
  if (EndOffset > BeginOffset) {
    return AffectedRangeMgr.affectsCharSourceRange(
        CharSourceRange::getCharRange(StartOfFile.getLocWithOffset(BeginOffset),
                                      StartOfFile.getLocWithOffset(EndOffset)));
  }
  if (FallbackEndOffset <= FallbackBeginOffset)
    return false;
  return AffectedRangeMgr.affectsCharSourceRange(CharSourceRange::getCharRange(
      StartOfFile.getLocWithOffset(FallbackBeginOffset),
      StartOfFile.getLocWithOffset(FallbackEndOffset)));
}

void queueReplacement(std::vector<PendingReplacement> &Pending, StringRef Code,
                      unsigned BeginOffset, unsigned EndOffset,
                      std::string Text, StringRef Context) {
  if (EndOffset < BeginOffset)
    return;
  if (getExistingText(Code, BeginOffset, EndOffset) == Text)
    return;
  Pending.push_back(
      {BeginOffset, EndOffset, std::move(Text), std::string(Context)});
}

unsigned countNewlines(StringRef Text) {
  return static_cast<unsigned>(Text.count('\n'));
}

StringRef getIndentSuffix(StringRef Whitespace) {
  const size_t Pos = Whitespace.find_last_of('\n');
  if (Pos == StringRef::npos)
    return Whitespace;
  return Whitespace.drop_front(Pos + 1);
}

StringRef detectLineEnding(StringRef Whitespace, StringRef DefaultLineEnding) {
  return Whitespace.contains("\r\n") ? "\r\n" : DefaultLineEnding;
}

std::string buildLeadingWhitespace(StringRef ExistingWhitespace,
                                   unsigned DesiredNewlines,
                                   StringRef DefaultLineEnding) {
  const StringRef LineEnding =
      detectLineEnding(ExistingWhitespace, DefaultLineEnding);
  std::string Result;
  for (unsigned I = 0; I < DesiredNewlines; ++I)
    Result += LineEnding.str();
  Result += getIndentSuffix(ExistingWhitespace).str();
  return Result;
}

StringRef getLineIndent(StringRef Code, const PhysicalLine &Line) {
  const StringRef LineText = getLineText(Code, Line);
  const size_t FirstContent = LineText.find_first_not_of(" \t");
  return FirstContent == StringRef::npos ? LineText
                                         : LineText.take_front(FirstContent);
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
      const unsigned EndOffset = Tokens[*Line.FirstNonCommentToken].Offset;
      if (rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                        Tokens[*Line.FirstNonCommentToken].Offset,
                        Tokens[*ColonIndex].EndOffset)) {
        const StringRef Existing =
            getExistingText(Code, BeginOffset, EndOffset);
        if (countNewlines(Existing) < 2) {
          queueReplacement(
              Pending, Code, BeginOffset, EndOffset,
              buildLeadingWhitespace(Existing, 2, DefaultLineEnding),
              "BlankLinesAroundLabels(before)");
        }
      }
    }

    const unsigned NextLineIndex =
        nextNonBlankLine(Lines, Tokens[*ColonIndex].LineIndex);
    if (NextLineIndex >= Lines.size() || Lines[NextLineIndex].CommentOnly ||
        Lines[NextLineIndex].Disabled ||
        !Lines[NextLineIndex].FirstNonCommentToken) {
      continue;
    }

    const unsigned BeginOffset = Tokens[*ColonIndex].EndOffset;
    const unsigned EndOffset =
        Tokens[*Lines[NextLineIndex].FirstNonCommentToken].Offset;
    if (!rangeAffected(
            AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
            Tokens[*Lines[NextLineIndex].FirstNonCommentToken].Offset,
            Tokens[*Lines[NextLineIndex].FirstNonCommentToken].EndOffset)) {
      continue;
    }

    const StringRef Existing = getExistingText(Code, BeginOffset, EndOffset);
    if (countNewlines(Existing) < 2) {
      queueReplacement(Pending, Code, BeginOffset, EndOffset,
                       buildLeadingWhitespace(Existing, 2, DefaultLineEnding),
                       "BlankLinesAroundLabels(after)");
    }
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
    const unsigned EndOffset = CaseToken.Offset;
    if (!rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                       CaseToken.Offset, CaseToken.EndOffset)) {
      continue;
    }

    const StringRef Existing = getExistingText(Code, BeginOffset, EndOffset);
    if (countNewlines(Existing) < 2) {
      queueReplacement(Pending, Code, BeginOffset, EndOffset,
                       buildLeadingWhitespace(Existing, 2, DefaultLineEnding),
                       "SeparateSwitchCaseBlocks");
    }
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
    std::string Indent;
    if (Style.UseTab == FormatStyle::UT_Never) {
      Indent = BlockIndent.str() + std::string(Style.IndentWidth, ' ');
    } else {
      const unsigned TabWidth = std::max(Style.TabWidth, 1u);
      unsigned Column = 0;
      for (char C : BlockIndent)
        Column = C == '\t' ? Column + TabWidth - Column % TabWidth : Column + 1;
      Column += Style.IndentWidth;
      Indent = std::string(Column / TabWidth, '\t') +
               std::string(Column % TabWidth, ' ');
    }
    queueReplacement(Pending, Code, BeginOffset, EndOffset, std::move(Indent),
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

void collectDeletedDeclarationSeparationFixes(
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
    const unsigned EndOffset =
        Tokens[*Lines[NextLineIndex].FirstNonCommentToken].Offset;
    if (!rangeAffected(
            AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset, EndOffset,
            Tokens[*Lines[NextLineIndex].FirstNonCommentToken].EndOffset))
      continue;
    const StringRef Existing = getExistingText(Code, BeginOffset, EndOffset);
    if (countNewlines(Existing) >= 2)
      continue;
    queueReplacement(Pending, Code, BeginOffset, EndOffset,
                     buildLeadingWhitespace(Existing, 2, DefaultLineEnding),
                     "SpaceAfterParenthesizedSpecifiers(separation)");
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
    StringRef Code, StringRef DefaultLineEnding, unsigned IndentWidth,
    std::vector<PendingReplacement> &Pending) {
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

    if (Tokens[CloseParenIndex].LineIndex == LineIndex) {
      const int FirstParameterIndex = nextNonCommentTokenOnLine(
          Tokens, static_cast<int>(*LParenIndex), LineIndex);
      int LastParameterIndex = -1;
      for (unsigned TokenIndex : Lines[LineIndex].Tokens) {
        if (TokenIndex >= *LParenIndex &&
            TokenIndex < static_cast<unsigned>(CloseParenIndex) &&
            Tokens[TokenIndex].Tok.isNot(tok::comment)) {
          LastParameterIndex = static_cast<int>(TokenIndex);
        }
      }
      if (FirstParameterIndex >= 0 && FirstParameterIndex != CloseParenIndex &&
          LastParameterIndex >= 0) {
        const std::string ParameterIndent = BaseIndent.empty()
                                                ? std::string(IndentWidth, ' ')
                                                : BaseIndent.str();
        queueReplacement(Pending, Code, Tokens[*LParenIndex].EndOffset,
                         Tokens[FirstParameterIndex].Offset,
                         DefaultLineEnding.str() + ParameterIndent,
                         "DeindentQualifiedFunctionNames(parameter)");
        queueReplacement(Pending, Code, Tokens[LastParameterIndex].EndOffset,
                         Tokens[CloseParenIndex].Offset,
                         DefaultLineEnding.str() + ParameterIndent,
                         "DeindentQualifiedFunctionNames(closing-parenthesis)");
      }
    }

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

void addQueuedReplacements(const std::vector<PendingReplacement> &Pending,
                           const SourceManager &SourceManager,
                           SourceLocation StartOfFile,
                           tooling::Replacements &Result) {
  std::vector<PendingReplacement> Sorted = Pending;
  std::sort(
      Sorted.begin(), Sorted.end(),
      [](const PendingReplacement &Left, const PendingReplacement &Right) {
        if (Left.BeginOffset != Right.BeginOffset)
          return Left.BeginOffset < Right.BeginOffset;
        if (Left.EndOffset != Right.EndOffset)
          return Left.EndOffset < Right.EndOffset;
        if (Left.Text != Right.Text)
          return Left.Text < Right.Text;
        return Left.Context < Right.Context;
      });

  std::optional<PendingReplacement> Previous;
  for (const PendingReplacement &Current : Sorted) {
    if (Previous) {
      if (Current.BeginOffset == Previous->BeginOffset &&
          Current.EndOffset == Previous->EndOffset &&
          Current.Text == Previous->Text) {
        continue;
      }
      if (Current.BeginOffset < Previous->EndOffset) {
        llvm::errs() << "BoundaryPostFormat conflicting replacements ["
                     << Current.Context << "] and [" << Previous->Context
                     << "] at offsets " << Current.BeginOffset << "-"
                     << Current.EndOffset << ".\n";
        continue;
      }
    }

    auto Error = Result.add(tooling::Replacement(
        SourceManager, StartOfFile.getLocWithOffset(Current.BeginOffset),
        Current.EndOffset - Current.BeginOffset, Current.Text));
    if (Error) {
      llvm::errs() << "BoundaryPostFormat failed to add replacement ["
                   << Current.Context << "] at offsets " << Current.BeginOffset
                   << "-" << Current.EndOffset << ": "
                   << llvm::toString(std::move(Error)) << "\n";
      continue;
    }
    Previous = Current;
  }
}

} // namespace

std::pair<tooling::Replacements, unsigned>
runBoundaryPostFormatPass(const Environment &Env, const FormatStyle &Style) {
  const auto *ExtensionStyle = getActiveExtensionStyleConst();
  if (!ExtensionStyle || Style.Language != FormatStyle::LK_Cpp)
    return {};

  const bool BlankLinesAroundLabelsEnabled =
      ExtensionStyle->BlankLinesAroundLabels;
  const bool IndentAttributedLabelsEnabled =
      ExtensionStyle->IndentAttributedLabels;
  const bool SeparateClosingDirectiveCommentsEnabled =
      ExtensionStyle->SeparateClosingDirectiveComments;
  const bool SeparateSwitchCaseBlocksEnabled =
      ExtensionStyle->SeparateSwitchCaseBlocks;
  const bool DeindentQualifiedFunctionNamesEnabled =
      ExtensionStyle->DeindentQualifiedFunctionNames;
  const bool SpaceAfterParenthesizedSpecifiersEnabled =
      ExtensionStyle->SpaceAfterParenthesizedSpecifiers;
  if (!BlankLinesAroundLabelsEnabled && !IndentAttributedLabelsEnabled &&
      !SeparateClosingDirectiveCommentsEnabled &&
      !DeindentQualifiedFunctionNamesEnabled &&
      !SpaceAfterParenthesizedSpecifiersEnabled &&
      !SeparateSwitchCaseBlocksEnabled) {
    return {};
  }

  const SourceManager &SourceManager = Env.getSourceManager();
  const FileID File = Env.getFileID();
  const SourceLocation StartOfFile = SourceManager.getLocForStartOfFile(File);
  const StringRef Code = SourceManager.getBufferData(File);
  const StringRef DefaultLineEnding = Code.contains("\r\n") ? "\r\n" : "\n";

  std::vector<PhysicalLine> Lines = buildLines(Code);
  std::vector<TokenInfo> Tokens;
  Tokens.reserve(Code.size() / 8);

  Lexer Lex(File, SourceManager.getBufferOrFake(File), SourceManager,
            getFormattingLangOpts(Style));
  Lex.SetCommentRetentionState(true);

  bool FormattingDisabled = false;
  unsigned BraceDepth = 0;
  unsigned ParenDepth = 0;
  unsigned SquareDepth = 0;
  for (Token RawToken; !Lex.LexFromRawLexer(RawToken);) {
    if (RawToken.is(tok::eof))
      break;
    const unsigned Length = RawToken.getLength();
    if (Length == 0)
      continue;

    const unsigned Offset = SourceManager.getFileOffset(RawToken.getLocation());
    const unsigned LineIndex = findLineIndex(Lines, Offset);
    const StringRef Text(Code.data() + Offset, Length);
    if (RawToken.is(tok::comment) && isClangFormatOn(Text))
      FormattingDisabled = false;

    Tokens.push_back({RawToken, Text, Offset, Offset + Length, LineIndex,
                      BraceDepth, ParenDepth, SquareDepth, FormattingDisabled});

    switch (RawToken.getKind()) {
    case tok::l_brace:
      ++BraceDepth;
      break;
    case tok::r_brace:
      if (BraceDepth > 0)
        --BraceDepth;
      break;
    case tok::l_paren:
      ++ParenDepth;
      break;
    case tok::r_paren:
      if (ParenDepth > 0)
        --ParenDepth;
      break;
    case tok::l_square:
      ++SquareDepth;
      break;
    case tok::r_square:
      if (SquareDepth > 0)
        --SquareDepth;
      break;
    default:
      break;
    }

    if (RawToken.is(tok::comment) && isClangFormatOff(Text))
      FormattingDisabled = true;
  }

  for (unsigned I = 0; I < Tokens.size(); ++I) {
    PhysicalLine &Line = Lines[Tokens[I].LineIndex];
    Line.Tokens.push_back(I);
    if (Tokens[I].Tok.isNot(tok::comment)) {
      if (!Line.FirstNonCommentToken)
        Line.FirstNonCommentToken = I;
      Line.LastNonCommentToken = I;
    }
    Line.Disabled = Line.Disabled || Tokens[I].Disabled;
  }

  bool InDirective = false;
  for (PhysicalLine &Line : Lines) {
    Line.CommentOnly = !Line.BlankText && !Line.FirstNonCommentToken;
    const bool StartsDirective =
        Line.FirstNonCommentToken &&
        Tokens[*Line.FirstNonCommentToken].Tok.is(tok::hash);
    if (InDirective || StartsDirective) {
      Line.InPPDirective = true;
      InDirective = endsWithBackslash(getLineText(Code, Line));
    } else {
      InDirective = false;
    }
  }

  std::vector<int> MatchingParens(Tokens.size(), -1);
  std::vector<int> MatchingSquares(Tokens.size(), -1);
  std::vector<int> ParenStack;
  std::vector<int> SquareStack;
  for (int I = 0, E = static_cast<int>(Tokens.size()); I < E; ++I) {
    switch (Tokens[I].Tok.getKind()) {
    case tok::l_paren:
      ParenStack.push_back(I);
      break;
    case tok::r_paren:
      if (!ParenStack.empty()) {
        MatchingParens[I] = ParenStack.back();
        MatchingParens[ParenStack.back()] = I;
        ParenStack.pop_back();
      }
      break;
    case tok::l_square:
      SquareStack.push_back(I);
      break;
    case tok::r_square:
      if (!SquareStack.empty()) {
        MatchingSquares[I] = SquareStack.back();
        MatchingSquares[SquareStack.back()] = I;
        SquareStack.pop_back();
      }
      break;
    default:
      break;
    }
  }

  AffectedRangeManager AffectedRangeMgr(SourceManager, Env.getCharRanges());
  std::vector<PendingReplacement> Pending;
  Pending.reserve(32);

  if (BlankLinesAroundLabelsEnabled) {
    collectBlankLineAroundLabelFixes(Tokens, Lines, AffectedRangeMgr,
                                     StartOfFile, Code, DefaultLineEnding,
                                     Pending);
  }
  if (SeparateSwitchCaseBlocksEnabled) {
    collectSwitchCaseBlockSeparationFixes(Tokens, Lines, AffectedRangeMgr,
                                          StartOfFile, Code, DefaultLineEnding,
                                          Pending);
  }
  if (IndentAttributedLabelsEnabled) {
    collectAttributedLabelIndentFixes(Tokens, Lines, MatchingSquares,
                                      AffectedRangeMgr, StartOfFile, Code,
                                      Style, Pending);
  }
  if (SeparateClosingDirectiveCommentsEnabled) {
    collectClosingDirectiveCommentFixes(Tokens, Lines, AffectedRangeMgr,
                                        StartOfFile, Code, Pending);
  }
  if (SpaceAfterParenthesizedSpecifiersEnabled) {
    collectDeletedDeclarationSeparationFixes(Tokens, Lines, AffectedRangeMgr,
                                             StartOfFile, Code,
                                             DefaultLineEnding, Pending);
  }
  if (DeindentQualifiedFunctionNamesEnabled) {
    collectQualifiedFunctionNameDeindentFixes(
        Tokens, Lines, MatchingParens, AffectedRangeMgr, StartOfFile, Code,
        DefaultLineEnding, Style.IndentWidth, Pending);
  }

  tooling::Replacements Result;
  addQueuedReplacements(Pending, SourceManager, StartOfFile, Result);
  return {Result, 0};
}

} // namespace clang::format::extensions

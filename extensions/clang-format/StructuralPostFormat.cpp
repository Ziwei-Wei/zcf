#include "extensions/clang-format/PostFormatSource.h"

#include "extensions/clang-format/ExtensionStyle.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace clang::format::extensions {
namespace {

struct MacroAlignEntry {
  unsigned LineIndex = 0;
  unsigned NameEndOffset = 0;
  unsigned ReplacementOffset = 0;
  unsigned ReplacementTokenIndex = 0;
  unsigned NameEndColumn = 0;
  unsigned ReplacementColumn = 0;
};

struct AggregateNode;

struct AggregateElement {
  std::string LeafText;
  std::unique_ptr<AggregateNode> Child;

  bool isLeaf() const { return Child == nullptr; }
};

struct AggregateNode {
  unsigned OpenTokenIndex = 0;
  unsigned CloseTokenIndex = 0;
  unsigned MaxBraceLevels = 1;
  std::vector<AggregateElement> Elements;
};

bool isControlKeyword(StringRef Text) {
  return Text == "if" || Text == "for" || Text == "while" || Text == "switch";
}

bool isConditionalDirectiveName(StringRef Text) {
  return Text == "if" || Text == "ifdef" || Text == "ifndef" ||
         Text == "elif" || Text == "elifdef" || Text == "elifndef" ||
         Text == "else" || Text == "endif";
}

bool isInlineLeafSafeToken(const TokenInfo &Token) {
  return Token.Tok.isNot(tok::comment) && Token.Tok.isNot(tok::semi) &&
         Token.Tok.isNot(tok::l_brace) && Token.Tok.isNot(tok::r_brace);
}

bool containsCodeAfterOnSameLine(const std::vector<TokenInfo> &Tokens,
                                 int Index) {
  const unsigned LineIndex = Tokens[Index].LineIndex;
  for (++Index; Index < static_cast<int>(Tokens.size()); ++Index) {
    if (Tokens[Index].LineIndex != LineIndex)
      break;
    if (Tokens[Index].Tok.isNot(tok::comment))
      return true;
  }
  return false;
}

std::optional<std::string> normalizeIntegerLiteralCase(StringRef Text) {
  if (Text.empty() || Text.front() == '.')
    return std::nullopt;

  enum class IntegerBase { Other, Binary, Hex };

  IntegerBase Base = IntegerBase::Other;
  unsigned PrefixLength = 0;
  if (Text.size() >= 2 && Text[0] == '0') {
    if (Text[1] == 'x' || Text[1] == 'X') {
      Base = IntegerBase::Hex;
      PrefixLength = 2;
    } else if (Text[1] == 'b' || Text[1] == 'B') {
      Base = IntegerBase::Binary;
      PrefixLength = 2;
    }
  }

  auto IsDigitForBase = [&](char C) {
    if (C == '\'')
      return true;
    switch (Base) {
    case IntegerBase::Hex:
      return llvm::isDigit(C) || (C >= 'a' && C <= 'f') ||
             (C >= 'A' && C <= 'F');
    case IntegerBase::Binary:
      return C == '0' || C == '1';
    case IntegerBase::Other:
      return llvm::isDigit(C);
    }
    return false;
  };

  unsigned DigitEnd = PrefixLength;
  while (DigitEnd < Text.size() && IsDigitForBase(Text[DigitEnd]))
    ++DigitEnd;
  if (DigitEnd == PrefixLength)
    return std::nullopt;

  if (DigitEnd < Text.size()) {
    const char FirstRemainder = Text[DigitEnd];
    if (Base == IntegerBase::Hex) {
      if (FirstRemainder == '.' || FirstRemainder == 'p' ||
          FirstRemainder == 'P') {
        return std::nullopt;
      }
    } else if (FirstRemainder == '.' || FirstRemainder == 'e' ||
               FirstRemainder == 'E') {
      return std::nullopt;
    }
  }

  unsigned StandardSuffixEnd = DigitEnd;
  while (StandardSuffixEnd < Text.size() &&
         (Text[StandardSuffixEnd] == 'u' || Text[StandardSuffixEnd] == 'U' ||
          Text[StandardSuffixEnd] == 'l' || Text[StandardSuffixEnd] == 'L')) {
    ++StandardSuffixEnd;
  }

  std::string Normalized;
  Normalized.reserve(Text.size());
  if (PrefixLength == 2) {
    Normalized.push_back('0');
    Normalized.push_back(Base == IntegerBase::Hex ? 'x' : 'b');
  }
  for (unsigned I = PrefixLength; I < DigitEnd; ++I) {
    const char C = Text[I];
    if (Base == IntegerBase::Hex && C >= 'a' && C <= 'f')
      Normalized.push_back(static_cast<char>(C - 'a' + 'A'));
    else
      Normalized.push_back(C);
  }
  for (unsigned I = DigitEnd; I < StandardSuffixEnd; ++I) {
    const char C = Text[I];
    if (C == 'u' || C == 'l')
      Normalized.push_back(static_cast<char>(C - 'a' + 'A'));
    else
      Normalized.push_back(C);
  }
  Normalized += Text.substr(StandardSuffixEnd).str();
  if (PrefixLength == 0)
    Normalized.insert(0, Text.substr(0, PrefixLength).str());

  if (Normalized == Text)
    return std::nullopt;
  return Normalized;
}

bool suppressBlankLineBefore(const std::vector<PhysicalLine> &Lines,
                             const std::vector<TokenInfo> &Tokens,
                             unsigned LineIndex, int KeywordTokenIndex) {
  if (KeywordTokenIndex < 0)
    return true;
  const PhysicalLine &Line = Lines[LineIndex];
  if (!Line.FirstNonCommentToken ||
      *Line.FirstNonCommentToken != static_cast<unsigned>(KeywordTokenIndex)) {
    return true;
  }

  const unsigned PrevLineIndex = previousNonBlankLine(Lines, LineIndex);
  if (PrevLineIndex >= Lines.size())
    return true;

  const PhysicalLine &PrevLine = Lines[PrevLineIndex];
  if (PrevLine.CommentOnly || !PrevLine.FirstNonCommentToken)
    return true;

  const StringRef PrevFirst = Tokens[*PrevLine.FirstNonCommentToken].Text;
  const StringRef PrevLast = Tokens[*PrevLine.LastNonCommentToken].Text;
  return PrevLast == "{" || PrevFirst == "else" || PrevFirst == "catch" ||
         PrevFirst == "do" || PrevFirst == "case" || PrevFirst == "default";
}

bool suppressBlankLineAfter(const std::vector<PhysicalLine> &Lines,
                            const std::vector<TokenInfo> &Tokens,
                            unsigned CloseLineIndex, int CloseTokenIndex) {
  if (CloseTokenIndex < 0 ||
      containsCodeAfterOnSameLine(Tokens, CloseTokenIndex))
    return true;

  const unsigned NextLineIndex = nextNonBlankLine(Lines, CloseLineIndex);
  if (NextLineIndex >= Lines.size())
    return true;

  const PhysicalLine &NextLine = Lines[NextLineIndex];
  if (NextLine.CommentOnly || !NextLine.FirstNonCommentToken)
    return true;

  const StringRef NextFirst = Tokens[*NextLine.FirstNonCommentToken].Text;
  return NextFirst == "}" || NextFirst == "else" || NextFirst == "catch" ||
         NextFirst == "while" || NextFirst == "case" || NextFirst == "default";
}

void queueBlankLineBefore(const std::vector<TokenInfo> &Tokens,
                          unsigned TokenIndex,
                          AffectedRangeManager &AffectedRangeMgr,
                          SourceLocation StartOfFile, StringRef Code,
                          StringRef DefaultLineEnding, StringRef Context,
                          std::vector<PendingReplacement> &Pending) {
  if (TokenIndex == 0)
    return;

  // Raw tokens include comments, so the range between adjacent tokens contains
  // only whitespace.
  const TokenInfo &Token = Tokens[TokenIndex];
  const unsigned BeginOffset = Tokens[TokenIndex - 1].EndOffset;
  const unsigned EndOffset = Token.Offset;
  if (!rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                     Token.Offset, Token.EndOffset)) {
    return;
  }
  queueSingleBlankLine(Pending, Code, BeginOffset, EndOffset, DefaultLineEnding,
                       Context);
}

void collectIntegerLiteralFixes(const std::vector<TokenInfo> &Tokens,
                                AffectedRangeManager &AffectedRangeMgr,
                                SourceLocation StartOfFile, StringRef Code,
                                std::vector<PendingReplacement> &Pending) {
  for (const TokenInfo &Token : Tokens) {
    if (Token.Disabled || Token.Tok.isNot(tok::numeric_constant))
      continue;
    if (!rangeAffected(AffectedRangeMgr, StartOfFile, Token.Offset,
                       Token.EndOffset, Token.Offset, Token.EndOffset)) {
      continue;
    }
    const bool CoveredByAggregate = std::any_of(
        Pending.begin(), Pending.end(),
        [&](const PendingReplacement &Replacement) {
          return Replacement.Context == "ExpandNestedAggregateBraces" &&
                 Token.Offset >= Replacement.BeginOffset &&
                 Token.EndOffset <= Replacement.EndOffset;
        });
    if (CoveredByAggregate)
      continue;

    if (std::optional<std::string> Normalized =
            normalizeIntegerLiteralCase(Token.Text)) {
      queueReplacement(Pending, Code, Token.Offset, Token.EndOffset,
                       std::move(*Normalized), "NormalizeIntegerLiteralCase");
    }
  }
}

void collectBlankLineFixes(const std::vector<TokenInfo> &Tokens,
                           const std::vector<PhysicalLine> &Lines,
                           const std::vector<int> &MatchingParens,
                           const std::vector<int> &MatchingBraces,
                           AffectedRangeManager &AffectedRangeMgr,
                           SourceLocation StartOfFile, StringRef Code,
                           StringRef DefaultLineEnding,
                           std::vector<PendingReplacement> &Pending) {
  for (int I = 0, E = static_cast<int>(Tokens.size()); I < E; ++I) {
    const TokenInfo &Keyword = Tokens[I];
    if (Keyword.Disabled || !isControlKeyword(Keyword.Text))
      continue;

    const PhysicalLine &StartLine = Lines[Keyword.LineIndex];
    if (StartLine.InPPDirective || StartLine.Disabled)
      continue;

    int Cursor = nextNonCommentToken(Tokens, I);
    if (Cursor < 0)
      continue;
    if (Keyword.Text == "if" && Tokens[Cursor].Text == "constexpr")
      Cursor = nextNonCommentToken(Tokens, Cursor);
    if (Cursor < 0)
      continue;

    int OpenBraceIndex = -1;
    if (Keyword.Text == "if" && Tokens[Cursor].Text == "consteval") {
      OpenBraceIndex = nextNonCommentToken(Tokens, Cursor);
      if (OpenBraceIndex < 0 || Tokens[OpenBraceIndex].Tok.isNot(tok::l_brace))
        continue;
    } else {
      if (Tokens[Cursor].Tok.isNot(tok::l_paren))
        continue;
      const int CloseParenIndex = MatchingParens[Cursor];
      if (CloseParenIndex < 0)
        continue;
      OpenBraceIndex = nextNonCommentToken(Tokens, CloseParenIndex);
      if (OpenBraceIndex < 0 || Tokens[OpenBraceIndex].Tok.isNot(tok::l_brace))
        continue;
    }

    if (Tokens[OpenBraceIndex].Disabled)
      continue;

    const int CloseBraceIndex = MatchingBraces[OpenBraceIndex];
    if (CloseBraceIndex < 0)
      continue;

    if (!suppressBlankLineBefore(Lines, Tokens, Keyword.LineIndex, I)) {
      queueBlankLineBefore(Tokens, *StartLine.FirstToken, AffectedRangeMgr,
                           StartOfFile, Code, DefaultLineEnding,
                           "BlankLinesAroundControlStatements(before)",
                           Pending);
    }

    if (!suppressBlankLineAfter(Lines, Tokens,
                                Tokens[CloseBraceIndex].LineIndex,
                                CloseBraceIndex)) {
      const unsigned NextLineIndex =
          nextNonBlankLine(Lines, Tokens[CloseBraceIndex].LineIndex);
      if (NextLineIndex < Lines.size() && Lines[NextLineIndex].FirstToken) {
        queueBlankLineBefore(
            Tokens, *Lines[NextLineIndex].FirstToken, AffectedRangeMgr,
            StartOfFile, Code, DefaultLineEnding,
            "BlankLinesAroundControlStatements(after)", Pending);
      }
    }
  }
}

std::optional<MacroAlignEntry>
parseObjectLikeDefine(const PhysicalLine &Line,
                      const std::vector<TokenInfo> &Tokens, StringRef Code,
                      unsigned TabWidth) {
  if (!Line.FirstNonCommentToken)
    return std::nullopt;
  const unsigned HashIndex = *Line.FirstNonCommentToken;
  if (Tokens[HashIndex].Tok.isNot(tok::hash))
    return std::nullopt;

  const int DirectiveIndex =
      nextNonCommentToken(Tokens, static_cast<int>(HashIndex));
  if (DirectiveIndex < 0 ||
      Tokens[DirectiveIndex].LineIndex != Tokens[HashIndex].LineIndex ||
      Tokens[DirectiveIndex].Text != "define") {
    return std::nullopt;
  }

  const int NameIndex = nextNonCommentToken(Tokens, DirectiveIndex);
  if (NameIndex < 0 ||
      Tokens[NameIndex].LineIndex != Tokens[HashIndex].LineIndex)
    return std::nullopt;

  const int ReplacementIndex = nextNonCommentToken(Tokens, NameIndex);
  if (ReplacementIndex < 0 ||
      Tokens[ReplacementIndex].LineIndex != Tokens[HashIndex].LineIndex ||
      Tokens[ReplacementIndex].Tok.is(tok::comment)) {
    return std::nullopt;
  }

  if (Tokens[ReplacementIndex].Tok.is(tok::l_paren) &&
      Tokens[ReplacementIndex].Offset == Tokens[NameIndex].EndOffset) {
    return std::nullopt;
  }

  const StringRef LineText = getLineText(Code, Line);
  const unsigned NameEndColumn = measureColumn(
      LineText.take_front(Tokens[NameIndex].EndOffset - Line.StartOffset),
      TabWidth);
  const unsigned ReplacementColumn = measureColumn(
      LineText.take_front(Tokens[ReplacementIndex].Offset - Line.StartOffset),
      TabWidth);
  return MacroAlignEntry{Tokens[HashIndex].LineIndex,
                         Tokens[NameIndex].EndOffset,
                         Tokens[ReplacementIndex].Offset,
                         static_cast<unsigned>(ReplacementIndex),
                         NameEndColumn,
                         ReplacementColumn};
}

void flushMacroGroup(const std::vector<MacroAlignEntry> &Group, StringRef Code,
                     const std::vector<TokenInfo> &Tokens,
                     AffectedRangeManager &AffectedRangeMgr,
                     SourceLocation StartOfFile,
                     std::vector<PendingReplacement> &Pending) {
  if (Group.size() < 2)
    return;

  unsigned TargetColumn = 0;
  for (const MacroAlignEntry &Entry : Group)
    TargetColumn = std::max(TargetColumn, Entry.ReplacementColumn);

  for (const MacroAlignEntry &Entry : Group) {
    const unsigned BeginOffset = Entry.NameEndOffset;
    const unsigned EndOffset = Entry.ReplacementOffset;
    const unsigned ReplacementEnd =
        Tokens[Entry.ReplacementTokenIndex].EndOffset;
    if (!rangeAffected(AffectedRangeMgr, StartOfFile, BeginOffset, EndOffset,
                       Entry.ReplacementOffset, ReplacementEnd)) {
      continue;
    }
    const unsigned SpaceCount =
        std::max(1u, TargetColumn - Entry.NameEndColumn);
    queueReplacement(Pending, Code, BeginOffset, EndOffset,
                     std::string(SpaceCount, ' '),
                     "AlignMacrosAcrossDirectives");
  }
}

void collectMacroAlignmentFixes(const std::vector<TokenInfo> &Tokens,
                                const std::vector<PhysicalLine> &Lines,
                                AffectedRangeManager &AffectedRangeMgr,
                                SourceLocation StartOfFile, StringRef Code,
                                unsigned TabWidth,
                                std::vector<PendingReplacement> &Pending) {
  std::vector<MacroAlignEntry> Group;
  auto Flush = [&] {
    flushMacroGroup(Group, Code, Tokens, AffectedRangeMgr, StartOfFile,
                    Pending);
    Group.clear();
  };

  for (const PhysicalLine &Line : Lines) {
    if (Line.BlankText || Line.CommentOnly || Line.Disabled ||
        !Line.FirstNonCommentToken) {
      Flush();
      continue;
    }
    const unsigned FirstIndex = *Line.FirstNonCommentToken;
    if (Tokens[FirstIndex].Tok.isNot(tok::hash) || !Line.InPPDirective) {
      Flush();
      continue;
    }

    const int DirectiveIndex =
        nextNonCommentToken(Tokens, static_cast<int>(FirstIndex));
    if (DirectiveIndex < 0 ||
        Tokens[DirectiveIndex].LineIndex != Tokens[FirstIndex].LineIndex) {
      Flush();
      continue;
    }

    const StringRef DirectiveName = Tokens[DirectiveIndex].Text;
    if (DirectiveName == "define") {
      if (std::optional<MacroAlignEntry> Entry =
              parseObjectLikeDefine(Line, Tokens, Code, TabWidth)) {
        Group.push_back(*Entry);
      } else {
        Flush();
      }
      continue;
    }

    if (isConditionalDirectiveName(DirectiveName))
      continue;

    Flush();
  }

  Flush();
}

std::unique_ptr<AggregateNode>
parseAggregateNode(const std::vector<TokenInfo> &Tokens,
                   const std::vector<int> &MatchingBraces,
                   unsigned OpenTokenIndex, StringRef Code,
                   bool NormalizeIntegerLiterals) {
  if (OpenTokenIndex >= Tokens.size() ||
      Tokens[OpenTokenIndex].Tok.isNot(tok::l_brace)) {
    return nullptr;
  }

  const int CloseIndex = MatchingBraces[OpenTokenIndex];
  if (CloseIndex < 0)
    return nullptr;

  const TokenInfo &Open = Tokens[OpenTokenIndex];
  const unsigned ElementBraceDepth = Open.BraceDepth + 1;
  const unsigned ElementParenDepth = Open.ParenDepth;
  const unsigned ElementSquareDepth = Open.SquareDepth;

  auto Node = std::make_unique<AggregateNode>();
  Node->OpenTokenIndex = OpenTokenIndex;
  Node->CloseTokenIndex = static_cast<unsigned>(CloseIndex);

  unsigned Cursor = OpenTokenIndex + 1;
  while (Cursor < Node->CloseTokenIndex) {
    const TokenInfo &Current = Tokens[Cursor];
    if (Current.Disabled || Current.Tok.is(tok::comment))
      return nullptr;

    if (Current.BraceDepth == ElementBraceDepth &&
        Current.ParenDepth == ElementParenDepth &&
        Current.SquareDepth == ElementSquareDepth &&
        Current.Tok.is(tok::comma)) {
      ++Cursor;
      continue;
    }

    AggregateElement Element;
    if (Current.Tok.is(tok::l_brace)) {
      Element.Child = parseAggregateNode(Tokens, MatchingBraces, Cursor, Code,
                                         NormalizeIntegerLiterals);
      if (!Element.Child)
        return nullptr;
      Node->MaxBraceLevels =
          std::max(Node->MaxBraceLevels, 1 + Element.Child->MaxBraceLevels);
      Cursor = Element.Child->CloseTokenIndex + 1;
    } else {
      const unsigned LeafStart = Cursor;
      unsigned LeafEnd = Cursor;
      if (Current.Tok.is(tok::period))
        return nullptr;

      while (Cursor < Node->CloseTokenIndex) {
        const TokenInfo &LeafToken = Tokens[Cursor];
        if (LeafToken.Disabled || LeafToken.Tok.is(tok::comment) ||
            !isInlineLeafSafeToken(LeafToken)) {
          return nullptr;
        }
        if (LeafToken.BraceDepth == ElementBraceDepth &&
            LeafToken.ParenDepth == ElementParenDepth &&
            LeafToken.SquareDepth == ElementSquareDepth &&
            LeafToken.Tok.is(tok::comma)) {
          break;
        }
        if (LeafToken.Tok.isOneOf(tok::l_brace, tok::r_brace))
          return nullptr;
        LeafEnd = Cursor;
        ++Cursor;
      }

      const StringRef LeafText =
          Code.slice(Tokens[LeafStart].Offset, Tokens[LeafEnd].EndOffset)
              .trim();
      if (LeafText.empty() || LeafText.contains('\n') ||
          LeafText.contains('\r'))
        return nullptr;
      Element.LeafText = LeafText.str();
      if (NormalizeIntegerLiterals) {
        const unsigned BaseOffset = Tokens[LeafStart].Offset;
        for (unsigned Index = LeafEnd + 1; Index-- > LeafStart;) {
          if (Tokens[Index].Tok.isNot(tok::numeric_constant))
            continue;
          const auto Normalized =
              normalizeIntegerLiteralCase(Tokens[Index].Text);
          if (!Normalized)
            continue;
          const unsigned RelativeOffset = Tokens[Index].Offset - BaseOffset;
          Element.LeafText.replace(RelativeOffset, Tokens[Index].Text.size(),
                                   *Normalized);
        }
      }
    }

    Node->Elements.push_back(std::move(Element));
  }

  return Node;
}

bool aggregateNodeHasOnlyLeaves(const AggregateNode &Node) {
  return !Node.Elements.empty() &&
         std::all_of(
             Node.Elements.begin(), Node.Elements.end(),
             [](const AggregateElement &Element) { return Element.isLeaf(); });
}

std::vector<std::string> renderAggregateNode(const AggregateNode &Node,
                                             StringRef BaseIndent,
                                             unsigned IndentLevel,
                                             const FormatStyle &Style) {
  const std::string Indent =
      buildIndentText(BaseIndent, IndentLevel * Style.IndentWidth, Style);
  const std::string ChildIndent =
      buildIndentText(BaseIndent, (IndentLevel + 1) * Style.IndentWidth, Style);

  if (Node.Elements.empty())
    return {Indent + "{}"};

  if (aggregateNodeHasOnlyLeaves(Node)) {
    std::string ScalarLine = Indent + "{";
    for (size_t I = 0; I < Node.Elements.size(); ++I) {
      if (I != 0)
        ScalarLine += ", ";
      ScalarLine += Node.Elements[I].LeafText;
    }
    ScalarLine += "}";
    return {std::move(ScalarLine)};
  }

  std::vector<std::string> Lines;
  Lines.push_back(Indent + "{");

  std::string PendingLeafLine;
  bool HasPendingLeafLine = false;
  for (size_t I = 0; I < Node.Elements.size(); ++I) {
    const AggregateElement &Element = Node.Elements[I];
    const bool HasNext = I + 1 < Node.Elements.size();

    if (Element.isLeaf()) {
      if (!HasPendingLeafLine) {
        PendingLeafLine = ChildIndent + Element.LeafText;
        HasPendingLeafLine = true;
      } else {
        PendingLeafLine += ", " + Element.LeafText;
      }
      if (!HasNext) {
        Lines.push_back(PendingLeafLine);
        HasPendingLeafLine = false;
      }
      continue;
    }

    const auto ChildLines =
        renderAggregateNode(*Element.Child, BaseIndent, IndentLevel + 1, Style);
    if (HasPendingLeafLine) {
      Lines.push_back(PendingLeafLine);
      Lines.back() += ",";
      HasPendingLeafLine = false;
    }
    Lines.insert(Lines.end(), ChildLines.begin(), ChildLines.end());
    if (HasNext)
      Lines.back() += ",";
  }

  Lines.push_back(Indent + "}");
  return Lines;
}

void collectExpandedAggregateFixes(const std::vector<TokenInfo> &Tokens,
                                   const std::vector<PhysicalLine> &Lines,
                                   const std::vector<int> &MatchingBraces,
                                   AffectedRangeManager &AffectedRangeMgr,
                                   SourceLocation StartOfFile, StringRef Code,
                                   StringRef DefaultLineEnding,
                                   bool NormalizeIntegerLiterals,
                                   const FormatStyle &Style,
                                   std::vector<PendingReplacement> &Pending) {
  for (unsigned I = 0; I < Tokens.size(); ++I) {
    const TokenInfo &EqualToken = Tokens[I];
    if (EqualToken.Disabled || EqualToken.Tok.isNot(tok::equal))
      continue;

    const int OpenBraceIndex = nextNonCommentToken(Tokens, static_cast<int>(I));
    if (OpenBraceIndex < 0 || Tokens[OpenBraceIndex].Tok.isNot(tok::l_brace) ||
        Tokens[OpenBraceIndex].Disabled) {
      continue;
    }
    if (!Code.slice(EqualToken.EndOffset, Tokens[OpenBraceIndex].Offset)
             .trim()
             .empty()) {
      continue;
    }

    const int CloseBraceIndex = MatchingBraces[OpenBraceIndex];
    if (CloseBraceIndex < 0)
      continue;

    const int TerminatorIndex = nextNonCommentToken(Tokens, CloseBraceIndex);
    if (TerminatorIndex < 0 || Tokens[TerminatorIndex].Tok.isNot(tok::semi))
      continue;

    std::unique_ptr<AggregateNode> Root = parseAggregateNode(
        Tokens, MatchingBraces, OpenBraceIndex, Code, NormalizeIntegerLiterals);
    if (!Root || Root->MaxBraceLevels < 3)
      continue;

    if (!rangeAffected(AffectedRangeMgr, StartOfFile, EqualToken.EndOffset,
                       Tokens[CloseBraceIndex].EndOffset,
                       Tokens[OpenBraceIndex].Offset,
                       Tokens[CloseBraceIndex].EndOffset)) {
      continue;
    }

    const StringRef BaseIndent =
        getLineIndent(Code, Lines[EqualToken.LineIndex]);
    const auto RenderedLines = renderAggregateNode(*Root, BaseIndent, 0, Style);
    std::string Replacement = DefaultLineEnding.str();
    for (size_t LineIndex = 0; LineIndex < RenderedLines.size(); ++LineIndex) {
      if (LineIndex != 0)
        Replacement += DefaultLineEnding.str();
      Replacement += RenderedLines[LineIndex];
    }

    queueReplacement(Pending, Code, EqualToken.EndOffset,
                     Tokens[CloseBraceIndex].EndOffset, std::move(Replacement),
                     "ExpandNestedAggregateBraces");
    I = static_cast<unsigned>(CloseBraceIndex);
  }
}

} // namespace

void collectStructuralFixes(const PostFormatSource &Source,
                            const FormatStyle &Style,
                            const ExtensionStyle &Extensions,
                            AffectedRangeManager &AffectedRangeMgr,
                            std::vector<PendingReplacement> &Pending) {
  const auto &Tokens = Source.Tokens;
  const auto &Lines = Source.Lines;
  const StringRef Code = Source.Code;
  const SourceLocation StartOfFile = Source.StartOfFile;
  const StringRef DefaultLineEnding = Source.DefaultLineEnding;

  if (Extensions.BlankLinesAroundControlStatements) {
    collectBlankLineFixes(Tokens, Lines, Source.MatchingParens,
                          Source.MatchingBraces, AffectedRangeMgr, StartOfFile,
                          Code, DefaultLineEnding, Pending);
  }
  if (Extensions.ExpandNestedAggregateBraces) {
    collectExpandedAggregateFixes(
        Tokens, Lines, Source.MatchingBraces, AffectedRangeMgr, StartOfFile,
        Code, DefaultLineEnding, Extensions.NormalizeIntegerLiteralCase, Style,
        Pending);
  }
  if (Extensions.NormalizeIntegerLiteralCase) {
    collectIntegerLiteralFixes(Tokens, AffectedRangeMgr, StartOfFile, Code,
                               Pending);
  }
  if (Extensions.AlignMacrosAcrossDirectives) {
    collectMacroAlignmentFixes(Tokens, Lines, AffectedRangeMgr, StartOfFile,
                               Code, std::max(Style.TabWidth, 1u), Pending);
  }
}

} // namespace clang::format::extensions

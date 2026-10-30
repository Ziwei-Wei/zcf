#include "extensions/clang-format/PostFormatSource.h"

#include "TokenAnalyzer.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

namespace clang::format::extensions {
namespace {

bool isBlankText(StringRef Text) { return Text.trim(" \t\f\v").empty(); }

bool endsWithBackslash(StringRef Text) {
  Text = Text.rtrim(" \t\f\v");
  return !Text.empty() && Text.back() == '\\';
}

std::vector<PhysicalLine> buildLines(StringRef Code) {
  std::vector<PhysicalLine> Lines;
  auto AddLine = [&](unsigned Start, unsigned ContentEnd, unsigned End) {
    PhysicalLine Line;
    Line.StartOffset = Start;
    Line.ContentEndOffset = ContentEnd;
    Line.EndOffset = End;
    Line.BlankText = isBlankText(Code.slice(Start, ContentEnd));
    Lines.push_back(std::move(Line));
  };

  unsigned Start = 0;
  for (unsigned I = 0, E = Code.size(); I < E; ++I) {
    if (Code[I] == '\r' && I + 1 < E && Code[I + 1] == '\n') {
      AddLine(Start, I, I + 2);
      Start = I + 2;
      ++I;
    } else if (Code[I] == '\n') {
      AddLine(Start, I, I + 1);
      Start = I + 1;
    }
  }
  if (Start < Code.size() || Lines.empty()) {
    const auto End = static_cast<unsigned>(Code.size());
    AddLine(Start, End, End);
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

void matchPairs(const std::vector<TokenInfo> &Tokens, tok::TokenKind Open,
                tok::TokenKind Close, std::vector<int> &Matching) {
  Matching.assign(Tokens.size(), -1);
  std::vector<int> Stack;
  for (int I = 0, E = static_cast<int>(Tokens.size()); I < E; ++I) {
    if (Tokens[I].Tok.is(Open)) {
      Stack.push_back(I);
    } else if (Tokens[I].Tok.is(Close) && !Stack.empty()) {
      Matching[I] = Stack.back();
      Matching[Stack.back()] = I;
      Stack.pop_back();
    }
  }
}

StringRef getIndentSuffix(StringRef Whitespace) {
  const size_t Pos = Whitespace.find_last_of('\n');
  if (Pos == StringRef::npos)
    return Whitespace;
  return Whitespace.drop_front(Pos + 1);
}

} // namespace

PostFormatSource scanPostFormatSource(const Environment &Env,
                                      const FormatStyle &Style) {
  const SourceManager &SourceManager = Env.getSourceManager();
  const FileID File = Env.getFileID();

  PostFormatSource Source;
  Source.StartOfFile = SourceManager.getLocForStartOfFile(File);
  Source.Code = SourceManager.getBufferData(File);
  Source.DefaultLineEnding = Source.Code.contains("\r\n") ? "\r\n" : "\n";
  Source.Lines = buildLines(Source.Code);
  auto &Lines = Source.Lines;
  auto &Tokens = Source.Tokens;
  Tokens.reserve(Source.Code.size() / 8);

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
    const StringRef Text(Source.Code.data() + Offset, Length);
    if (RawToken.is(tok::comment) && isClangFormatOn(Text))
      FormattingDisabled = false;

    Tokens.push_back({RawToken, Text, Offset, Offset + Length,
                      findLineIndex(Lines, Offset), BraceDepth, ParenDepth,
                      SquareDepth, FormattingDisabled});

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
    if (!Line.FirstToken)
      Line.FirstToken = I;
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
      InDirective = endsWithBackslash(getLineText(Source.Code, Line));
    } else {
      InDirective = false;
    }
  }

  matchPairs(Tokens, tok::l_paren, tok::r_paren, Source.MatchingParens);
  matchPairs(Tokens, tok::l_brace, tok::r_brace, Source.MatchingBraces);
  matchPairs(Tokens, tok::l_square, tok::r_square, Source.MatchingSquares);
  return Source;
}

tooling::Replacements
buildReplacements(const std::vector<PendingReplacement> &Pending,
                  const SourceManager &SourceManager,
                  SourceLocation StartOfFile) {
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

  tooling::Replacements Result;
  std::optional<PendingReplacement> Previous;
  for (const PendingReplacement &Current : Sorted) {
    if (Previous) {
      if (Current.BeginOffset == Previous->BeginOffset &&
          Current.EndOffset == Previous->EndOffset &&
          Current.Text == Previous->Text) {
        continue;
      }
      if (Current.BeginOffset < Previous->EndOffset) {
        llvm::errs() << "zcf post-format conflicting replacements ["
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
      llvm::errs() << "zcf post-format failed to add replacement ["
                   << Current.Context << "] at offsets " << Current.BeginOffset
                   << "-" << Current.EndOffset << ": "
                   << llvm::toString(std::move(Error)) << "\n";
      continue;
    }
    Previous = Current;
  }
  return Result;
}

StringRef getLineText(StringRef Code, const PhysicalLine &Line) {
  return Code.slice(Line.StartOffset, Line.ContentEndOffset);
}

StringRef getExistingText(StringRef Code, unsigned BeginOffset,
                          unsigned EndOffset) {
  return Code.slice(BeginOffset, EndOffset);
}

StringRef getLineIndent(StringRef Code, const PhysicalLine &Line) {
  const StringRef LineText = getLineText(Code, Line);
  const size_t FirstContent = LineText.find_first_not_of(" \t");
  return FirstContent == StringRef::npos ? LineText
                                         : LineText.take_front(FirstContent);
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

int nextNonCommentToken(const std::vector<TokenInfo> &Tokens, int Index) {
  for (++Index; Index >= 0 && Index < static_cast<int>(Tokens.size());
       ++Index) {
    if (Tokens[Index].Tok.isNot(tok::comment))
      return Index;
  }
  return -1;
}

int nextNonCommentTokenOnLine(const std::vector<TokenInfo> &Tokens, int Index,
                              unsigned LineIndex) {
  const int Next = nextNonCommentToken(Tokens, Index);
  return Next >= 0 && Tokens[Next].LineIndex == LineIndex ? Next : -1;
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

void queueSingleBlankLine(std::vector<PendingReplacement> &Pending,
                          StringRef Code, unsigned BeginOffset,
                          unsigned EndOffset, StringRef DefaultLineEnding,
                          StringRef Context) {
  const StringRef Existing = getExistingText(Code, BeginOffset, EndOffset);
  // Only whitespace may be rewritten; a range that reaches a comment or code
  // indicates a caller bug and must not delete it.
  if (!Existing.trim().empty())
    return;
  const std::string LineEnding =
      (Existing.contains("\r\n") ? StringRef("\r\n") : DefaultLineEnding).str();
  queueReplacement(Pending, Code, BeginOffset, EndOffset,
                   LineEnding + LineEnding + getIndentSuffix(Existing).str(),
                   Context);
}

unsigned lastTrailingCommentContinuation(const PostFormatSource &Source,
                                         unsigned Index, unsigned TabWidth) {
  const auto &Tokens = Source.Tokens;
  const auto &Lines = Source.Lines;
  auto IsLineComment = [&](unsigned I) {
    return Tokens[I].Tok.is(tok::comment) && Tokens[I].Text.starts_with("//");
  };
  auto Column = [&](unsigned I) {
    return measureColumn(Source.Code.slice(Lines[Tokens[I].LineIndex].StartOffset,
                                           Tokens[I].Offset),
                         TabWidth);
  };
  if (!IsLineComment(Index) || Lines[Tokens[Index].LineIndex].CommentOnly)
    return Index;
  const unsigned CommentColumn = Column(Index);
  unsigned Last = Index;
  while (Last + 1 < Tokens.size() && IsLineComment(Last + 1) &&
         Lines[Tokens[Last + 1].LineIndex].CommentOnly &&
         Tokens[Last + 1].LineIndex == Tokens[Last].LineIndex + 1 &&
         Column(Last + 1) == CommentColumn) {
    ++Last;
  }
  return Last;
}

unsigned measureColumn(StringRef Text, unsigned TabWidth) {
  unsigned Column = 0;
  for (char C : Text) {
    if (C == '\t')
      Column += TabWidth - (Column % TabWidth);
    else
      ++Column;
  }
  return Column;
}

std::string buildIndentText(StringRef BaseIndent, unsigned Columns,
                            const FormatStyle &Style) {
  if (Style.UseTab == FormatStyle::UT_Never)
    return BaseIndent.str() + std::string(Columns, ' ');
  const unsigned TabWidth = std::max(Style.TabWidth, 1u);
  const unsigned Column = measureColumn(BaseIndent, TabWidth) + Columns;
  return std::string(Column / TabWidth, '\t') +
         std::string(Column % TabWidth, ' ');
}

} // namespace clang::format::extensions

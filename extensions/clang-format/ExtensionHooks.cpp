#include "integration/clang-format/ExtensionHooks.h"
#include "extensions/clang-format/ExtensionPasses.h"
#include "extensions/clang-format/PostFormatSource.h"
#include "integration/clang-format/ExtensionContext.h"

#include "ContinuationIndenter.h"
#include "FormatToken.h"
#include "TokenAnalyzer.h"
#include "TokenAnnotator.h"

namespace clang::format::extensions {
namespace {

// The parameter list of an operator function such as `operator=(` or
// `operator()(`, but not an explicit member call like `value.operator=(`.
bool isOperatorParameterParen(const FormatToken &LeftParen) {
  if (LeftParen.isNot(TT_OverloadedOperatorLParen))
    return false;
  const auto *Keyword = LeftParen.getPreviousNonComment();
  while (Keyword && Keyword->isNot(tok::kw_operator))
    Keyword = Keyword->getPreviousNonComment();
  const auto *Before = Keyword ? Keyword->getPreviousNonComment() : nullptr;
  return Keyword && !(Before && Before->isOneOf(tok::period, tok::arrow));
}

bool isFunctionParameterParen(const FormatToken &LeftParen) {
  return (LeftParen.is(TT_FunctionDeclarationLParen) ||
          isOperatorParameterParen(LeftParen)) &&
         isNonEmptyParen(LeftParen);
}

bool isFunctionSignatureParen(const AnnotatedLine &Line,
                              const FormatToken &LeftParen) {
  return Line.MightBeFunctionDecl && isFunctionParameterParen(LeftParen);
}

bool isNamedParen(const FormatToken &LeftParen) {
  if (!isNonEmptyParen(LeftParen) ||
      LeftParen.isOneOf(TT_AttributeLParen, TT_ConditionLParen,
                        TT_FunctionTypeLParen, TT_LambdaDefinitionLParen,
                        TT_OverloadedOperatorLParen)) {
    return false;
  }

  const auto *Name = LeftParen.getPreviousNonComment();
  return Name && Name->isOneOf(tok::identifier, tok::r_paren, tok::r_square,
                               TT_TemplateCloser);
}

// Widths are measured from the formatted line start, not from input columns,
// so the decision does not change when a later run re-indents the line.
void compactCallIfItFits(const AnnotatedLine &Line, FormatToken &LeftParen,
                         const FormatStyle &FormatStyle) {
  if (FormatStyle.ColumnLimit == 0 || !LeftParen.MatchingParen ||
      !LeftParen.Next || LeftParen.Next == LeftParen.MatchingParen ||
      hasRequiredStringLiteralBreak(LeftParen, FormatStyle))
    return;

  bool HasNewline = false;
  bool InCall = false;
  unsigned Width = Line.Level * FormatStyle.IndentWidth;
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->is(tok::comment) || Token->IsMultiline ||
        !Token->Children.empty())
      return;
    if (Token != Line.First)
      Width += Token->SpacesRequiredBefore;
    Width += Token->ColumnWidth;
    HasNewline |= InCall && Token->NewlinesBefore > 0;
    if (Token == &LeftParen)
      InCall = true;
    if (Token == LeftParen.MatchingParen)
      break;
  }
  if (!HasNewline)
    return;

  const auto *Trailing = LeftParen.MatchingParen->Next;
  if (Trailing && Trailing->is(tok::comment))
    Width += Trailing->SpacesRequiredBefore + Trailing->ColumnWidth;
  if (Width > FormatStyle.ColumnLimit)
    return;

  for (auto *Token = LeftParen.Next; Token; Token = Token->Next) {
    Token->MustBreakBefore = false;
    Token->NewlinesBefore = 0;
    if (Token == LeftParen.MatchingParen)
      break;
  }
}

} // namespace

void customizeAnnotatedLine(AnnotatedLine &Line,
                            const FormatStyle &FormatStyle) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style)
    return;

  for (auto *Token = Line.First; Token; Token = Token->Next) {
    // A declaration-like line can still contain a call, but its own parameter
    // list is not one. AlignArrayOfStructures owns the breaks of an
    // array-of-structures line; compacting would undo them on a second run.
    if (Style->ProgressiveCallExpansion && isCallParen(Line, *Token) &&
        !(Line.MightBeFunctionDecl && Token->MightBeFunctionDeclParen) &&
        Line.Type != LT_ArrayOfStructInitializer) {
      compactCallIfItFits(Line, *Token, FormatStyle);
    }
    if (Style->ForceMultilineFunctionSignatures &&
        isFunctionSignatureParen(Line, *Token)) {
      forceMultilineList(*Token);
      // A short body must not merge onto the closing parenthesis on this run
      // and stay separate on the next one.
      if (Line.Last->is(TT_FunctionLBrace))
        addForcedBreakLength(*Token->Next, FormatStyle);
    }
  }
  customizeSyntaxLayout(Line, FormatStyle);
  customizeDeclarationLayout(Line, FormatStyle);
}

std::optional<unsigned>
getExtensionNewLineColumn(const LineState &State,
                          const FormatStyle &FormatStyle) {
  if (auto Column = getSyntaxNewLineColumn(State, FormatStyle))
    return Column;
  if (auto Column = getDeclarationNewLineColumn(State, FormatStyle))
    return Column;

  const auto *Style = getActiveExtensionStyleConst();
  const auto *RightParen = State.NextToken;
  if (!Style || !Style->ArgumentIndentedClosingParentheses || !RightParen ||
      RightParen->isNot(tok::r_paren) || !RightParen->MatchingParen) {
    return std::nullopt;
  }

  const auto &LeftParen = *RightParen->MatchingParen;
  if (!isFunctionParameterParen(LeftParen) && !isNamedParen(LeftParen)) {
    return std::nullopt;
  }
  return State.Stack.back().Indent;
}

bool separatesDefinitions(const ExtensionStyle &Extensions) {
  return Extensions.SeparateDefinitionBlocksFromAll ||
         Extensions.SeparateFunctionDeclarations ||
         Extensions.SeparatePreprocessorConditionalBlocks;
}

// Blank-line separation runs before LibFormat, like upstream
// SeparateDefinitionBlocks, so alignment of trailing comments and macros sees
// the separated blocks. The post-format pass repeats it for layout-dependent
// cases such as lambdas that only become multi-line while formatting.
std::pair<tooling::Replacements, unsigned>
runExtensionPreFormatPass(const Environment &Env, const FormatStyle &Style) {
  const auto *Extensions = getActiveExtensionStyleConst();
  if (!Extensions || !separatesDefinitions(*Extensions) ||
      Style.Language != FormatStyle::LK_Cpp) {
    return {};
  }

  const PostFormatSource Source = scanPostFormatSource(Env, Style);
  AffectedRangeManager AffectedRangeMgr(Env.getSourceManager(),
                                        Env.getCharRanges());
  std::vector<PendingReplacement> Pending;
  collectDefinitionSeparationFixes(Env, Style, Source, *Extensions,
                                   /*BeforeFormatting=*/true, AffectedRangeMgr,
                                   Pending);
  return {
      buildReplacements(Pending, Env.getSourceManager(), Source.StartOfFile),
      0};
}

// LLVM 20 decides whether a trailing comment after `}` ends a scope from the
// brace's original line break, so a short block it merges onto one line keeps
// its comment out of the alignment run until the next formatting pass. When
// such a brace was merged, formatting and the post-format pass are repeated
// once, after the extension blank lines exist, so that a single zcf run is
// already stable.
bool extensionNeedsReformat(const Environment &Env, const FormatStyle &Style) {
  const auto *Extensions = getActiveExtensionStyleConst();
  if (!Extensions || !Extensions->anyEnabled() ||
      Style.Language != FormatStyle::LK_Cpp ||
      Style.AlignTrailingComments.Kind != FormatStyle::TCAS_Always) {
    return false;
  }

  const PostFormatSource Source = scanPostFormatSource(Env, Style);
  const auto &Tokens = Source.Tokens;
  const auto &Lines = Source.Lines;
  auto HasTrailingComment = [&](const PhysicalLine &Line) {
    return !Line.Disabled && !Line.InPPDirective && !Line.CommentOnly &&
           Line.LastNonCommentToken &&
           Line.Tokens.back() != *Line.LastNonCommentToken;
  };
  // A blank line inserted after LibFormat aligned trailing comments leaves
  // them aligned across a gap wider than OverEmptyLines, which LibFormat does
  // not produce; reformatting realigns them now instead of on the next run.
  const unsigned TabWidth = std::max(Style.TabWidth, 1u);
  auto CommentColumn = [&](const PhysicalLine &Line) {
    return measureColumn(
        Source.Code.slice(Line.StartOffset, Tokens[Line.Tokens.back()].Offset),
        TabWidth);
  };
  auto IsPadded = [&](const PhysicalLine &Line) {
    return Tokens[Line.Tokens.back()].Offset -
               Tokens[*Line.LastNonCommentToken].EndOffset >
           1;
  };
  for (size_t LineIndex = 1; LineIndex < Lines.size(); ++LineIndex) {
    const PhysicalLine &Line = Lines[LineIndex];
    if (!HasTrailingComment(Line))
      continue;
    const unsigned PreviousIndex = previousNonBlankLine(Lines, LineIndex);
    if (PreviousIndex >= Lines.size() ||
        LineIndex - PreviousIndex - 1 <=
            Style.AlignTrailingComments.OverEmptyLines) {
      continue;
    }
    const PhysicalLine &Previous = Lines[PreviousIndex];
    if (HasTrailingComment(Previous) &&
        CommentColumn(Previous) == CommentColumn(Line) &&
        (IsPadded(Previous) || IsPadded(Line))) {
      return true;
    }
  }

  for (size_t LineIndex = 0; LineIndex < Lines.size(); ++LineIndex) {
    const PhysicalLine &Line = Lines[LineIndex];
    // Alignment only differs when an adjacent line also has a trailing
    // comment.
    if (!HasTrailingComment(Line) || !Line.FirstNonCommentToken ||
        !((LineIndex > 0 && HasTrailingComment(Lines[LineIndex - 1])) ||
          (LineIndex + 1 < Lines.size() &&
           HasTrailingComment(Lines[LineIndex + 1])))) {
      continue;
    }
    for (unsigned Index : Line.Tokens) {
      if (Index == *Line.FirstNonCommentToken ||
          Tokens[Index].Tok.isNot(tok::r_brace)) {
        continue;
      }
      // Only a block brace merged onto its header line, not a braced
      // initializer, is affected.
      const int Open = Source.MatchingBraces[Index];
      if (Open <= 0 || Tokens[Open].LineIndex != Tokens[Index].LineIndex)
        continue;
      const StringRef Before = Tokens[Open - 1].Text;
      if (Before == ")" || Before == "else" || Before == "do" ||
          Before == "try") {
        return true;
      }
    }
  }
  return false;
}

std::pair<tooling::Replacements, unsigned>
runExtensionPostFormatPass(const Environment &Env, const FormatStyle &Style) {
  const auto *Extensions = getActiveExtensionStyleConst();
  if (!Extensions || !Extensions->anyEnabled() ||
      Style.Language != FormatStyle::LK_Cpp) {
    return {};
  }

  // Both passes inspect the same LibFormat output, so they share one scan and
  // one replacement set; overlapping edits are resolved in one place.
  const PostFormatSource Source = scanPostFormatSource(Env, Style);
  AffectedRangeManager AffectedRangeMgr(Env.getSourceManager(),
                                        Env.getCharRanges());
  std::vector<PendingReplacement> Pending;
  collectStructuralFixes(Source, Style, *Extensions, AffectedRangeMgr, Pending);
  collectBoundaryFixes(Source, Style, *Extensions, AffectedRangeMgr, Pending);
  if (separatesDefinitions(*Extensions)) {
    collectDefinitionSeparationFixes(Env, Style, Source, *Extensions,
                                     /*BeforeFormatting=*/false,
                                     AffectedRangeMgr, Pending);
  }
  return {
      buildReplacements(Pending, Env.getSourceManager(), Source.StartOfFile),
      0};
}

} // namespace clang::format::extensions

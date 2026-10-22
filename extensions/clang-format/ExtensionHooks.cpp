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

bool isFunctionParameterParen(const FormatToken &LeftParen) {
  return LeftParen.is(TT_FunctionDeclarationLParen) &&
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

void compactCallIfItFits(FormatToken &LeftParen,
                         const FormatStyle &FormatStyle) {
  if (FormatStyle.ColumnLimit == 0 || !LeftParen.MatchingParen ||
      !LeftParen.Next || LeftParen.Next == LeftParen.MatchingParen ||
      hasRequiredStringLiteralBreak(LeftParen, FormatStyle))
    return;

  bool HasNewline = false;
  unsigned Width = LeftParen.Previous->OriginalColumn +
                   LeftParen.Previous->ColumnWidth + LeftParen.ColumnWidth;
  for (auto *Token = LeftParen.Next; Token; Token = Token->Next) {
    if (Token->is(tok::comment) || Token->IsMultiline ||
        !Token->Children.empty())
      return;
    HasNewline |= Token->NewlinesBefore > 0;
    Width += Token->SpacesRequiredBefore + Token->ColumnWidth;
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

bool isCtorMemberBracedInitializer(const AnnotatedLine &Line,
                                   const FormatToken &LeftBrace) {
  const auto *Member = LeftBrace.getPreviousNonComment();
  if (!Member || Member->isNot(tok::identifier))
    return false;
  const auto *BeforeMember = Member->getPreviousNonComment();
  return (BeforeMember && BeforeMember->isOneOf(TT_CtorInitializerColon,
                                                TT_CtorInitializerComma)) ||
         Member == Line.First;
}

} // namespace

void customizeAnnotatedLine(AnnotatedLine &Line,
                            const FormatStyle &FormatStyle) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style)
    return;

  for (auto *Token = Line.First; Token; Token = Token->Next) {
    // A declaration-like line can still contain a call, but its own parameter
    // list is not one.
    if (Style->ProgressiveCallExpansion && isCallParen(Line, *Token) &&
        !(Line.MightBeFunctionDecl && Token->MightBeFunctionDeclParen)) {
      compactCallIfItFits(*Token, FormatStyle);
    }
    if (Style->ForceMultilineFunctionSignatures &&
        isFunctionSignatureParen(Line, *Token)) {
      forceMultilineList(*Token);
    }
  }
  customizeSyntaxLayout(Line, FormatStyle);
  customizeDeclarationLayout(Line);
}

std::optional<bool> getSpaceRequiredBefore(const AnnotatedLine &Line,
                                           const FormatToken &Right) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style)
    return std::nullopt;
  if (Style->ContextSensitiveBracedInitializers && Right.is(tok::l_brace) &&
      Right.is(BK_BracedInit) && isCtorMemberBracedInitializer(Line, Right)) {
    return false;
  }
  if (auto Required = getDeclaratorSpaceRequiredBefore(Line, Right))
    return Required;
  return getSpecifierSpaceRequiredBefore(Line, Right);
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
  return {
      buildReplacements(Pending, Env.getSourceManager(), Source.StartOfFile),
      0};
}

} // namespace clang::format::extensions

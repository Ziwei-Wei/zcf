#include "extensions/clang-format/P1Hooks.h"
#include "extensions/clang-format/P2Hooks.h"
#include "integration/clang-format/ExtensionContext.h"
#include "integration/clang-format/ExtensionHooks.h"

#include "ContinuationIndenter.h"
#include "FormatToken.h"
#include "TokenAnnotator.h"

#include <algorithm>

#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

namespace clang::format::extensions {
namespace {

bool isNonEmptyList(const FormatToken &LeftParen) {
  return LeftParen.MatchingParen && LeftParen.Next &&
         LeftParen.Next != LeftParen.MatchingParen;
}

bool isFunctionParameterParen(const FormatToken &LeftParen) {
  return LeftParen.is(TT_FunctionDeclarationLParen) &&
         isNonEmptyList(LeftParen);
}

bool isFunctionSignatureParen(const AnnotatedLine &Line,
                              const FormatToken &LeftParen) {
  return Line.MightBeFunctionDecl && isFunctionParameterParen(LeftParen);
}

bool isFunctionCallParen(const AnnotatedLine &Line,
                         const FormatToken &LeftParen) {
  if (Line.InPPDirective || !isNonEmptyList(LeftParen) ||
      LeftParen.isNot(tok::l_paren) ||
      LeftParen.isOneOf(TT_AttributeLParen, TT_ConditionLParen,
                        TT_FunctionDeclarationLParen, TT_FunctionTypeLParen,
                        TT_LambdaDefinitionLParen,
                        TT_OverloadedOperatorLParen)) {
    return false;
  }

  const auto *Callee = LeftParen.getPreviousNonComment();
  if (!Callee)
    return false;
  if (Line.MightBeFunctionDecl && LeftParen.MightBeFunctionDeclParen)
    return false;
  return Callee->isOneOf(tok::identifier, tok::r_paren, tok::r_square,
                         TT_TemplateCloser);
}

bool isNamedParen(const FormatToken &LeftParen) {
  if (!isNonEmptyList(LeftParen) || LeftParen.isNot(tok::l_paren) ||
      LeftParen.isOneOf(TT_AttributeLParen, TT_ConditionLParen,
                        TT_FunctionTypeLParen, TT_LambdaDefinitionLParen,
                        TT_OverloadedOperatorLParen)) {
    return false;
  }

  const auto *Name = LeftParen.getPreviousNonComment();
  return Name && Name->isOneOf(tok::identifier, tok::r_paren, tok::r_square,
                               TT_TemplateCloser);
}

void forceMultilineList(FormatToken &LeftParen) {
  auto *RightParen = LeftParen.MatchingParen;
  auto *Current = LeftParen.Next;
  if (!RightParen || !Current || Current == RightParen)
    return;

  Current->MustBreakBefore = true;
  Current->CanBreakBefore = true;
  for (; Current && Current != RightParen; Current = Current->Next) {
    if (Current->is(tok::comma) &&
        Current->NestingLevel == LeftParen.NestingLevel && Current->Next &&
        Current->Next != RightParen) {
      Current->Next->MustBreakBefore = true;
      Current->Next->CanBreakBefore = true;
    }
  }
  RightParen->MustBreakBefore = true;
  RightParen->CanBreakBefore = true;
}

void compactCallIfItFits(FormatToken &LeftParen,
                         const FormatStyle &FormatStyle) {
  if (FormatStyle.ColumnLimit == 0 || !LeftParen.MatchingParen ||
      !LeftParen.Next || LeftParen.Next == LeftParen.MatchingParen)
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

  if (Style->BlankLineBeforeReturn && Line.First->is(tok::kw_return))
    Line.First->NewlinesBefore = std::max(Line.First->NewlinesBefore, 2u);

  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Style->ProgressiveCallExpansion && isFunctionCallParen(Line, *Token))
      compactCallIfItFits(*Token, FormatStyle);
    if (Style->ForceMultilineFunctionSignatures &&
        isFunctionSignatureParen(Line, *Token)) {
      forceMultilineList(*Token);
    }
  }
  customizeP1AnnotatedLine(Line, FormatStyle);
  customizeP2AnnotatedLine(Line);
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
  if (auto Required = getP1SpaceRequiredBefore(Line, Right))
    return Required;
  return getP2SpaceRequiredBefore(Line, Right);
}

std::optional<unsigned>
getExtensionNewLineColumn(const LineState &State,
                          const FormatStyle &FormatStyle) {
  if (auto Column = getP1NewLineColumn(State, FormatStyle))
    return Column;
  if (auto Column = getP2NewLineColumn(State, FormatStyle))
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
  auto Result = runP1PostFormatPass(Env, Style);
  auto P2 = runP2PostFormatPass(Env, Style);
  for (const auto &Replacement : P2.first) {
    if (auto Error = Result.first.add(Replacement)) {
      llvm::errs() << "P2 post-format replacement conflicts with P1: "
                   << llvm::toString(std::move(Error)) << "\n";
    }
  }
  Result.second += P2.second;
  return Result;
}

} // namespace clang::format::extensions

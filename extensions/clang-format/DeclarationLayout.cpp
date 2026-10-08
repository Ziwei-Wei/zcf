#include "extensions/clang-format/ExtensionPasses.h"

#include "ContinuationIndenter.h"
#include "FormatToken.h"
#include "TokenAnnotator.h"
#include "integration/clang-format/ExtensionContext.h"

namespace clang::format::extensions {
namespace {

bool isFunctionRefQualifier(const FormatToken &Token) {
  if (!Token.isOneOf(tok::amp, tok::ampamp))
    return false;
  const auto *Previous = Token.getPreviousNonComment();
  return Previous && Previous->is(tok::r_paren) && Previous->MatchingParen &&
         Previous->MatchingParen->is(TT_FunctionDeclarationLParen);
}

bool isQualifiedFunctionComponent(const FormatToken &Token) {
  const auto *Next = Token.getNextNonComment();
  const auto *Previous = Token.getPreviousNonComment();
  return Token.is(TT_FunctionDeclarationName) ||
         (Token.is(tok::identifier) && Next &&
          (Next->is(tok::coloncolon) || (Next->is(tok::l_paren) && Previous &&
                                         Previous->is(tok::coloncolon))));
}

bool containsAssignment(const AnnotatedLine &Line) {
  for (const auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->is(tok::equal))
      return true;
  }
  return false;
}

bool isExplicitConstructorParameterParen(const FormatToken &LeftParen) {
  const auto *Name = LeftParen.getPreviousNonComment();
  if (!Name || !Name->isOneOf(tok::identifier, TT_CtorDtorDeclName,
                              TT_FunctionDeclarationName)) {
    return false;
  }

  const auto *SpecifierClose = Name->getPreviousNonComment();
  if (!SpecifierClose || SpecifierClose->isNot(tok::r_paren) ||
      !SpecifierClose->MatchingParen) {
    return false;
  }

  const auto *Explicit = SpecifierClose->MatchingParen->getPreviousNonComment();
  return Explicit && Explicit->is(tok::kw_explicit);
}

void forceQualifiedFunctionParameters(AnnotatedLine &Line) {
  if (Line.Level != 0 || containsAssignment(Line))
    return;
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (!isQualifiedFunctionComponent(*Token))
      continue;
    auto *LeftParen = Token->getNextNonComment();
    if (!LeftParen || LeftParen->isNot(tok::l_paren) ||
        !LeftParen->MatchingParen || !LeftParen->Next ||
        LeftParen->Next == LeftParen->MatchingParen) {
      return;
    }
    LeftParen->Next->MustBreakBefore = true;
    LeftParen->Next->CanBreakBefore = true;
    LeftParen->MatchingParen->MustBreakBefore = true;
    LeftParen->MatchingParen->CanBreakBefore = true;
    return;
  }
}

void forceExplicitConstructorParameters(AnnotatedLine &Line) {
  for (auto *Explicit = Line.First; Explicit; Explicit = Explicit->Next) {
    if (Explicit->isNot(tok::kw_explicit))
      continue;
    auto *SpecifierParen = Explicit->getNextNonComment();
    if (!SpecifierParen || SpecifierParen->isNot(tok::l_paren) ||
        !SpecifierParen->MatchingParen)
      continue;
    auto *Name = SpecifierParen->MatchingParen->getNextNonComment();
    auto *LeftParen = Name ? Name->getNextNonComment() : nullptr;
    if (!Name || Name->isNot(tok::identifier) || !LeftParen ||
        LeftParen->isNot(tok::l_paren) || !LeftParen->MatchingParen ||
        !LeftParen->Next || LeftParen->Next == LeftParen->MatchingParen) {
      continue;
    }
    LeftParen->Next->MustBreakBefore = true;
    LeftParen->Next->CanBreakBefore = true;
    LeftParen->MatchingParen->MustBreakBefore = true;
    LeftParen->MatchingParen->CanBreakBefore = true;
  }
}

} // namespace

void customizeDeclarationLayout(AnnotatedLine &Line) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style)
    return;

  if (Style->BreakRefQualifierRequires) {
    for (auto *Token = Line.First; Token; Token = Token->Next) {
      if (!isFunctionRefQualifier(*Token))
        continue;
      Token->MustBreakBefore = true;
      Token->CanBreakBefore = true;
      auto *Requires = Token->getNextNonComment();
      if (Requires && Requires->is(tok::kw_noexcept)) {
        Requires->MustBreakBefore = false;
        Requires->CanBreakBefore = false;
        Requires->NewlinesBefore = 0;
        Requires = Requires->getNextNonComment();
        if (Requires && Requires->is(tok::l_paren) && Requires->MatchingParen) {
          Requires = Requires->MatchingParen->getNextNonComment();
        }
      }
      if (Requires && Requires->isOneOf(TT_RequiresClause, tok::kw_requires)) {
        Requires->MustBreakBefore = false;
        Requires->CanBreakBefore = false;
        Requires->NewlinesBefore = 0;
      }
    }
  }

  if (Style->DeindentQualifiedFunctionNames)
    forceQualifiedFunctionParameters(Line);
  if (Style->SpaceAfterParenthesizedSpecifiers)
    forceExplicitConstructorParameters(Line);
}

std::optional<unsigned> getDeclarationNewLineColumn(const LineState &State,
                                                    const FormatStyle &) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style || !State.NextToken || State.Stack.empty())
    return std::nullopt;

  if (Style->ArgumentIndentedClosingParentheses &&
      State.NextToken->is(tok::r_paren) && State.NextToken->MatchingParen &&
      isExplicitConstructorParameterParen(*State.NextToken->MatchingParen)) {
    return State.Stack.back().Indent;
  }

  if (Style->BreakRefQualifierRequires &&
      isFunctionRefQualifier(*State.NextToken))
    return State.FirstIndent;

  if (Style->DeindentQualifiedFunctionNames && State.Line->Level == 0 &&
      !containsAssignment(*State.Line) &&
      isQualifiedFunctionComponent(*State.NextToken))
    return State.FirstIndent;

  return std::nullopt;
}

} // namespace clang::format::extensions

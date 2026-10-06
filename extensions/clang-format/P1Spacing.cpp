#include "extensions/clang-format/P1Hooks.h"

#include "FormatToken.h"
#include "TokenAnnotator.h"
#include "integration/clang-format/ExtensionContext.h"

namespace clang::format::extensions {
namespace {

bool isDeclarationLikeLine(const AnnotatedLine &Line) {
  return Line.MustBeDeclaration || Line.MightBeFunctionDecl ||
         Line.startsWith(tok::kw_typedef);
}

bool isTemplateParameterPackEllipsis(const FormatToken &Right) {
  if (!Right.Next || Right.Next->isNot(tok::identifier))
    return false;

  const auto &Left = *Right.Previous;
  const auto *BeforeLeft = Left.getPreviousNonComment();
  if (!BeforeLeft)
    return false;

  if (Left.isOneOf(tok::kw_typename, tok::kw_class, tok::kw_auto))
    return BeforeLeft->isOneOf(tok::less, tok::comma, TT_TemplateOpener);

  if (Left.is(tok::identifier))
    return BeforeLeft->isOneOf(tok::less, tok::comma, TT_TemplateOpener);

  if (!Left.is(TT_TemplateCloser) || !Left.MatchingParen)
    return false;

  const auto *Concept = Left.MatchingParen->getPreviousNonComment();
  const auto *BeforeConcept =
      Concept ? Concept->getPreviousNonComment() : nullptr;
  return Concept && Concept->is(tok::identifier) && BeforeConcept &&
         BeforeConcept->isOneOf(tok::less, tok::comma, TT_TemplateOpener);
}

bool isUsingOperatorPackEllipsis(const AnnotatedLine &Line,
                                 const FormatToken &Right) {
  const auto &Left = *Right.Previous;
  return Line.startsWith(tok::kw_using) && Left.is(tok::r_paren) &&
         Left.MatchingParen &&
         Left.MatchingParen->is(TT_OverloadedOperatorLParen);
}

bool isVariadicFriendEllipsis(const AnnotatedLine &Line,
                              const FormatToken &Right) {
  const auto &Left = *Right.Previous;
  return Line.startsWith(tok::kw_friend) && Right.Next &&
         Right.Next->is(tok::semi) &&
         Left.isOneOf(tok::identifier, TT_TemplateCloser);
}

bool isPackIndexExpressionEllipsis(const FormatToken &Right) {
  if (!Right.Next || Right.Next->isNot(tok::l_square))
    return false;

  const auto &Left = *Right.Previous;
  const auto *BeforeLeft = Left.getPreviousNonComment();
  if (!BeforeLeft)
    return false;

  return BeforeLeft->isOneOf(tok::kw_return, tok::kw_co_return, tok::comma,
                             tok::equal, tok::l_paren, tok::l_brace,
                             tok::l_square, tok::question, tok::colon,
                             tok::semi) ||
         BeforeLeft->isUnaryOperator() || BeforeLeft->isBinaryOperator();
}

bool isGroupedPointerOrReferenceDeclarator(const FormatToken &LeftParen) {
  if (!LeftParen.is(tok::l_paren) || !LeftParen.MatchingParen ||
      LeftParen.Next == LeftParen.MatchingParen) {
    return false;
  }

  bool SawPointerOrReference = false;
  const unsigned GroupLevel = LeftParen.NestingLevel + 1;
  for (const auto *Token = LeftParen.Next;
       Token && Token != LeftParen.MatchingParen; Token = Token->Next) {
    if (Token->NestingLevel != GroupLevel)
      continue;

    if (Token->isPointerOrReference()) {
      SawPointerOrReference = true;
      continue;
    }
    if (Token->is(tok::identifier) || Token->is(tok::coloncolon) ||
        Token->is(TT_TemplateCloser) ||
        Token->canBePointerOrReferenceQualifier()) {
      continue;
    }
    return false;
  }
  return SawPointerOrReference;
}

const FormatToken *findEnclosingParen(const FormatToken &Token) {
  for (const auto *Current = Token.Previous; Current;
       Current = Current->Previous) {
    if (Current->is(tok::l_paren) && Current->MatchingParen &&
        Current->NestingLevel + 1 == Token.NestingLevel) {
      return Current;
    }
    if (Current->NestingLevel + 1 < Token.NestingLevel)
      break;
  }
  return nullptr;
}

bool isSalStyleAnnotation(const FormatToken &Token) {
  return Token.is(tok::identifier) && Token.TokenText.starts_with('_') &&
         Token.TokenText.ends_with('_');
}

bool isFunctionPointerParameterAnnotation(const FormatToken &Right) {
  const auto &Left = *Right.Previous;
  if (!isSalStyleAnnotation(Left) || !Right.MatchingParen ||
      Right.Next == Right.MatchingParen) {
    return false;
  }

  const auto *AfterAnnotation = Right.MatchingParen->getNextNonComment();
  if (!AfterAnnotation ||
      AfterAnnotation->isOneOf(tok::comma, tok::r_paren, tok::semi)) {
    return false;
  }

  const auto *EnclosingParen = findEnclosingParen(Left);
  if (!EnclosingParen || !EnclosingParen->Previous ||
      EnclosingParen->Previous->isNot(tok::r_paren) ||
      !EnclosingParen->Previous->MatchingParen) {
    return false;
  }

  return isGroupedPointerOrReferenceDeclarator(
      *EnclosingParen->Previous->MatchingParen);
}

bool needsFunctionPointerDeclaratorSpace(const AnnotatedLine &Line,
                                         const FormatToken &Right) {
  return isDeclarationLikeLine(Line) &&
         isGroupedPointerOrReferenceDeclarator(Right) && Right.MatchingParen &&
         Right.MatchingParen->Next &&
         Right.MatchingParen->Next->isOneOf(tok::l_paren, tok::l_square);
}

} // namespace

std::optional<bool> getP1SpaceRequiredBefore(const AnnotatedLine &Line,
                                             const FormatToken &Right) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style || Line.InPPDirective || Line.InMacroBody || !Right.Previous)
    return std::nullopt;

  if (Style->SpaceParameterPackEllipses && Right.is(tok::ellipsis)) {
    if (isTemplateParameterPackEllipsis(Right) ||
        isUsingOperatorPackEllipsis(Line, Right) ||
        isPackIndexExpressionEllipsis(Right) ||
        isVariadicFriendEllipsis(Line, Right)) {
      return true;
    }
  }

  if (Style->SpaceParameterPackEllipses && Right.is(tok::l_square) &&
      Right.Previous->is(tok::ellipsis)) {
    return false;
  }

  if (Style->SpaceAnnotationsAndFunctionPointers && Right.is(tok::l_paren)) {
    if (needsFunctionPointerDeclaratorSpace(Line, Right) ||
        isFunctionPointerParameterAnnotation(Right)) {
      return true;
    }
  }

  return std::nullopt;
}

} // namespace clang::format::extensions

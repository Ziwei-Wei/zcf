#include "extensions/clang-format/ExtensionPasses.h"

#include "FormatToken.h"
#include "TokenAnnotator.h"
#include "integration/clang-format/ExtensionContext.h"

namespace clang::format::extensions {
namespace {

bool isDeclarationLikeLine(const AnnotatedLine &Line) {
  return Line.MustBeDeclaration || Line.MightBeFunctionDecl ||
         Line.startsWith(tok::kw_typedef);
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

std::optional<bool> getDeclaratorSpaceRequiredBefore(const AnnotatedLine &Line,
                                                     const FormatToken &Right) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style || Line.InPPDirective || Line.InMacroBody || !Right.Previous)
    return std::nullopt;

  if (Style->SpaceAnnotationsAndFunctionPointers && Right.is(tok::l_paren)) {
    if (needsFunctionPointerDeclaratorSpace(Line, Right) ||
        isFunctionPointerParameterAnnotation(Right)) {
      return true;
    }
  }

  return std::nullopt;
}

} // namespace clang::format::extensions

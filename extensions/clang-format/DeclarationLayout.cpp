#include "extensions/clang-format/ExtensionPasses.h"

#include "ContinuationIndenter.h"
#include "FormatToken.h"
#include "TokenAnnotator.h"
#include "integration/clang-format/ExtensionContext.h"

namespace clang::format::extensions {
namespace {

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

// Upstream does not recognize a declaration whose qualified name is split by
// comments, such as `ns:: // note` followed by `Type::name(...)`, so its
// parameter list never receives TT_FunctionDeclarationLParen.
FormatToken *findCommentSplitQualifiedParameterParen(AnnotatedLine &Line) {
  if (Line.Level != 0 || containsAssignment(Line))
    return nullptr;
  bool SawComment = false;
  for (auto *Token = Line.First; Token; Token = Token->Next) {
    if (Token->NestingLevel != 0)
      continue;
    SawComment |= Token->is(tok::comment);
    if (Token->isNot(tok::l_paren))
      continue;
    const auto *Name = Token->getPreviousNonComment();
    const auto *Qualifier = Name ? Name->getPreviousNonComment() : nullptr;
    if (!SawComment || Token->is(TT_FunctionDeclarationLParen) || !Name ||
        Name->isNot(tok::identifier) || !Qualifier ||
        Qualifier->isNot(tok::coloncolon)) {
      return nullptr;
    }
    return isNonEmptyParen(*Token) ? Token : nullptr;
  }
  return nullptr;
}

// Applies upstream's BreakAfterReturnType decision, which LibFormat skips for
// the comment-split declarations found above. The break goes before the first
// qualified-name component, matching recognized qualified declarations.
void breakAfterCommentSplitReturnType(AnnotatedLine &Line,
                                      FormatToken &LeftParen,
                                      const FormatStyle &FormatStyle) {
  const bool IsDefinition = !Line.endsWith(tok::semi);
  switch (FormatStyle.BreakAfterReturnType) {
  case FormatStyle::RTBS_All:
  case FormatStyle::RTBS_TopLevel:
    break;
  case FormatStyle::RTBS_AllDefinitions:
  case FormatStyle::RTBS_TopLevelDefinitions:
    if (!IsDefinition)
      return;
    break;
  default:
    return;
  }

  auto *NameStart = LeftParen.getPreviousNonComment();
  while (auto *Qualifier = NameStart->getPreviousNonComment()) {
    if (Qualifier->isNot(tok::coloncolon))
      break;
    auto *Scope = Qualifier->getPreviousNonComment();
    if (Scope && Scope->is(TT_TemplateCloser) && Scope->MatchingParen)
      Scope = Scope->MatchingParen->getPreviousNonComment();
    if (!Scope || Scope->isNot(tok::identifier)) {
      NameStart = Qualifier;
      break;
    }
    NameStart = Scope;
  }
  if (NameStart == Line.First || NameStart->MustBreakBefore)
    return;
  forceBreakBefore(*NameStart);
  addForcedBreakLength(*NameStart, FormatStyle);
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
    if (Name && Name->is(tok::identifier) && LeftParen &&
        isNonEmptyParen(*LeftParen)) {
      forceMultilineList(*LeftParen);
    }
  }
}

} // namespace

void customizeDeclarationLayout(AnnotatedLine &Line,
                                const FormatStyle &FormatStyle) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style)
    return;

  if (Style->ForceMultilineFunctionSignatures) {
    forceExplicitConstructorParameters(Line);
    if (auto *LeftParen = findCommentSplitQualifiedParameterParen(Line)) {
      forceMultilineList(*LeftParen);
      breakAfterCommentSplitReturnType(Line, *LeftParen, FormatStyle);
    }
  }
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

  if (Style->DeindentQualifiedFunctionNames && State.Line->Level == 0 &&
      State.NextToken->NestingLevel == 0 && !containsAssignment(*State.Line) &&
      isQualifiedFunctionComponent(*State.NextToken))
    return State.FirstIndent;

  return std::nullopt;
}

} // namespace clang::format::extensions

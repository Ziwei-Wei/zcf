#include "extensions/clang-format/P2Hooks.h"

#include "FormatToken.h"
#include "TokenAnnotator.h"
#include "integration/clang-format/ExtensionContext.h"

namespace clang::format::extensions {
namespace {

bool isDeleteWithReasonParen(const FormatToken &Right) {
  const auto *Left = Right.getPreviousNonComment();
  if (!Left || Left->isNot(tok::kw_delete))
    return false;

  const auto *BeforeDelete = Left->getPreviousNonComment();
  return BeforeDelete && BeforeDelete->is(tok::equal);
}

} // namespace

std::optional<bool> getP2SpaceRequiredBefore(const AnnotatedLine &Line,
                                             const FormatToken &Right) {
  const auto *Style = getActiveExtensionStyleConst();
  if (!Style || !Style->SpaceAfterParenthesizedSpecifiers ||
      Line.InPPDirective || Line.InMacroBody || !Right.Previous) {
    return std::nullopt;
  }

  if (Right.isNot(tok::l_paren))
    return std::nullopt;

  if (Style->SpaceAfterParenthesizedSpecifiers) {
    const auto *Left = Right.getPreviousNonComment();
    if (Left && (Left->is(tok::kw_explicit) || isDeleteWithReasonParen(Right)))
      return true;
  }

  return std::nullopt;
}

} // namespace clang::format::extensions

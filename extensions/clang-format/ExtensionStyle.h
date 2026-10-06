#ifndef CUSTOM_CLANG_FORMAT_EXTENSION_STYLE_H
#define CUSTOM_CLANG_FORMAT_EXTENSION_STYLE_H

#include "llvm/Support/YAMLTraits.h"

namespace clang::format::extensions {

struct ExtensionStyle {
  bool ArgumentIndentedClosingParentheses = false;
  bool ForceMultilineFunctionSignatures = false;
  bool ContextSensitiveBracedInitializers = false;
  bool BlankLineBeforeReturn = false;
  bool BreakConstructorDestructorSpecifiers = false;
  bool CompactSingleArgumentCalls = false;
  bool ExpandNestedAggregateBraces = false;
  bool BreakRequiresExpressionBraces = false;
  bool SpaceParameterPackEllipses = false;
  bool SpaceAnnotationsAndFunctionPointers = false;
  bool NormalizeIntegerLiteralCase = false;
  bool BlankLinesAroundControlStatements = false;
  bool AlignMacrosAcrossDirectives = false;
  bool DeindentQualifiedFunctionNames = false;
  bool BreakRefQualifierRequires = false;
  bool SpaceAfterParenthesizedSpecifiers = false;
  bool BlankLinesAroundLabels = false;
  bool IndentAttributedLabels = false;
  bool SeparateClosingDirectiveComments = false;
  bool ProgressiveCallExpansion = false;
  bool ProgressiveArithmeticExpansion = false;
  bool CompactTwoOperandExpressions = false;
  bool BodyDrivenLambdaExpansion = false;
  bool VerticalTernaryExpressions = false;
  bool SeparateSwitchCaseBlocks = false;
  bool ScopeStyleNestedTemplates = false;

  static ExtensionStyle getWFormatPresetStyle() {
    ExtensionStyle Style;
    Style.ArgumentIndentedClosingParentheses = true;
    Style.ForceMultilineFunctionSignatures = true;
    Style.ContextSensitiveBracedInitializers = true;
    Style.BlankLineBeforeReturn = true;
    Style.BreakConstructorDestructorSpecifiers = true;
    Style.CompactSingleArgumentCalls = true;
    Style.ExpandNestedAggregateBraces = true;
    Style.BreakRequiresExpressionBraces = true;
    Style.SpaceParameterPackEllipses = true;
    Style.SpaceAnnotationsAndFunctionPointers = true;
    Style.NormalizeIntegerLiteralCase = true;
    Style.BlankLinesAroundControlStatements = true;
    Style.AlignMacrosAcrossDirectives = true;
    Style.DeindentQualifiedFunctionNames = true;
    Style.BreakRefQualifierRequires = true;
    Style.SpaceAfterParenthesizedSpecifiers = true;
    Style.BlankLinesAroundLabels = true;
    Style.IndentAttributedLabels = true;
    Style.SeparateClosingDirectiveComments = true;
    Style.ProgressiveCallExpansion = true;
    Style.ProgressiveArithmeticExpansion = true;
    Style.CompactTwoOperandExpressions = true;
    Style.BodyDrivenLambdaExpansion = true;
    Style.VerticalTernaryExpressions = true;
    Style.SeparateSwitchCaseBlocks = true;
    Style.ScopeStyleNestedTemplates = true;
    return Style;
  }

  bool anyEnabled() const {
    return ArgumentIndentedClosingParentheses ||
           ForceMultilineFunctionSignatures ||
           ContextSensitiveBracedInitializers || BlankLineBeforeReturn ||
           BreakConstructorDestructorSpecifiers || CompactSingleArgumentCalls ||
           ExpandNestedAggregateBraces || BreakRequiresExpressionBraces ||
           SpaceParameterPackEllipses || SpaceAnnotationsAndFunctionPointers ||
           NormalizeIntegerLiteralCase || BlankLinesAroundControlStatements ||
           AlignMacrosAcrossDirectives || DeindentQualifiedFunctionNames ||
           BreakRefQualifierRequires || SpaceAfterParenthesizedSpecifiers ||
           BlankLinesAroundLabels || IndentAttributedLabels ||
           SeparateClosingDirectiveComments || ProgressiveCallExpansion ||
           ProgressiveArithmeticExpansion || CompactTwoOperandExpressions ||
           BodyDrivenLambdaExpansion || VerticalTernaryExpressions ||
           SeparateSwitchCaseBlocks || ScopeStyleNestedTemplates;
  }
};

} // namespace clang::format::extensions

namespace llvm::yaml {

template <> struct MappingTraits<clang::format::extensions::ExtensionStyle> {
  static void mapping(IO &IO,
                      clang::format::extensions::ExtensionStyle &Style) {
    IO.mapOptional("ArgumentIndentedClosingParentheses",
                   Style.ArgumentIndentedClosingParentheses);
    IO.mapOptional("ForceMultilineFunctionSignatures",
                   Style.ForceMultilineFunctionSignatures);
    IO.mapOptional("ContextSensitiveBracedInitializers",
                   Style.ContextSensitiveBracedInitializers);
    IO.mapOptional("BlankLineBeforeReturn", Style.BlankLineBeforeReturn);
    IO.mapOptional("BreakConstructorDestructorSpecifiers",
                   Style.BreakConstructorDestructorSpecifiers);
    IO.mapOptional("CompactSingleArgumentCalls",
                   Style.CompactSingleArgumentCalls);
    IO.mapOptional("ExpandNestedAggregateBraces",
                   Style.ExpandNestedAggregateBraces);
    IO.mapOptional("BreakRequiresExpressionBraces",
                   Style.BreakRequiresExpressionBraces);
    IO.mapOptional("SpaceParameterPackEllipses",
                   Style.SpaceParameterPackEllipses);
    IO.mapOptional("SpaceAnnotationsAndFunctionPointers",
                   Style.SpaceAnnotationsAndFunctionPointers);
    IO.mapOptional("NormalizeIntegerLiteralCase",
                   Style.NormalizeIntegerLiteralCase);
    IO.mapOptional("BlankLinesAroundControlStatements",
                   Style.BlankLinesAroundControlStatements);
    IO.mapOptional("AlignMacrosAcrossDirectives",
                   Style.AlignMacrosAcrossDirectives);
    IO.mapOptional("DeindentQualifiedFunctionNames",
                   Style.DeindentQualifiedFunctionNames);
    IO.mapOptional("BreakRefQualifierRequires",
                   Style.BreakRefQualifierRequires);
    IO.mapOptional("SpaceAfterParenthesizedSpecifiers",
                   Style.SpaceAfterParenthesizedSpecifiers);
    IO.mapOptional("BlankLinesAroundLabels", Style.BlankLinesAroundLabels);
    IO.mapOptional("IndentAttributedLabels", Style.IndentAttributedLabels);
    IO.mapOptional("SeparateClosingDirectiveComments",
                   Style.SeparateClosingDirectiveComments);
    IO.mapOptional("ProgressiveCallExpansion", Style.ProgressiveCallExpansion);
    IO.mapOptional("ProgressiveArithmeticExpansion",
                   Style.ProgressiveArithmeticExpansion);
    IO.mapOptional("CompactTwoOperandExpressions",
                   Style.CompactTwoOperandExpressions);
    IO.mapOptional("BodyDrivenLambdaExpansion",
                   Style.BodyDrivenLambdaExpansion);
    IO.mapOptional("VerticalTernaryExpressions",
                   Style.VerticalTernaryExpressions);
    IO.mapOptional("SeparateSwitchCaseBlocks", Style.SeparateSwitchCaseBlocks);
    IO.mapOptional("ScopeStyleNestedTemplates",
                   Style.ScopeStyleNestedTemplates);
  }
};

} // namespace llvm::yaml

#endif

#ifndef ZCF_EXTENSION_STYLE_H
#define ZCF_EXTENSION_STYLE_H

#include "llvm/Support/YAMLTraits.h"

namespace clang::format::extensions {

struct ExtensionStyle {
#define ZCF_EXTENSION_OPTION(Name) bool Name = false;
#include "extensions/clang-format/ExtensionOptions.def"

  static ExtensionStyle getZCFPresetStyle() {
    ExtensionStyle Style;
#define ZCF_EXTENSION_OPTION(Name) Style.Name = true;
#include "extensions/clang-format/ExtensionOptions.def"
    // The preset leaves blank lines before return statements to the author.
    Style.BlankLineBeforeReturn = false;
    return Style;
  }

  bool anyEnabled() const {
#define ZCF_EXTENSION_OPTION(Name)                                             \
  if (Name)                                                                    \
    return true;
#include "extensions/clang-format/ExtensionOptions.def"
    return false;
  }
};

} // namespace clang::format::extensions

namespace llvm::yaml {

template <> struct MappingTraits<clang::format::extensions::ExtensionStyle> {
  static void mapping(IO &IO,
                      clang::format::extensions::ExtensionStyle &Style) {
#define ZCF_EXTENSION_OPTION(Name) IO.mapOptional(#Name, Style.Name);
#include "extensions/clang-format/ExtensionOptions.def"
  }
};

} // namespace llvm::yaml

#endif

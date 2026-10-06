#ifndef CUSTOM_CLANG_FORMAT_EXTENSION_CONTEXT_H
#define CUSTOM_CLANG_FORMAT_EXTENSION_CONTEXT_H

#include "extensions/clang-format/ExtensionStyle.h"
#include "clang/Format/Format.h"
#include "llvm/ADT/StringRef.h"

#include <map>

namespace clang::format::extensions {

class ExtensionContextScope {
public:
  ExtensionContextScope();
  ExtensionContextScope(const ExtensionContextScope &) = delete;
  ExtensionContextScope &operator=(const ExtensionContextScope &) = delete;
  ~ExtensionContextScope();

  void selectLanguage(FormatStyle::LanguageKind Language);

private:
  ExtensionContextScope *Previous;
  std::map<FormatStyle::LanguageKind, ExtensionStyle> Styles;
  ExtensionStyle DisabledStyle;
  ExtensionStyle *SelectedStyle;

  friend void mapExtensionConfiguration(llvm::yaml::IO &IO,
                                        FormatStyle::LanguageKind Language,
                                        llvm::StringRef BasedOnStyle);
  friend const ExtensionStyle *getActiveExtensionStyleConst();
  friend void enableWFormatPresetExtensions(FormatStyle::LanguageKind Language);
};

const ExtensionStyle *getActiveExtensionStyleConst();
void enableWFormatPresetExtensions(FormatStyle::LanguageKind Language);

} // namespace clang::format::extensions

#endif

#include "ExtensionContext.h"
#include "ExtensionHooks.h"

namespace clang::format::extensions {
namespace {

thread_local ExtensionContextScope *ActiveContext = nullptr;

} // namespace

ExtensionContextScope::ExtensionContextScope()
    : Previous(ActiveContext), SelectedStyle(&DisabledStyle) {
  ActiveContext = this;
}

ExtensionContextScope::~ExtensionContextScope() { ActiveContext = Previous; }

void ExtensionContextScope::selectLanguage(FormatStyle::LanguageKind Language) {
  if (Language != FormatStyle::LK_Cpp) {
    SelectedStyle = &DisabledStyle;
    return;
  }
  auto It = Styles.find(Language);
  if (It == Styles.end() && Language != FormatStyle::LK_None)
    It = Styles.find(FormatStyle::LK_None);
  SelectedStyle = It == Styles.end() ? &DisabledStyle : &It->second;
}

const ExtensionStyle *getActiveExtensionStyleConst() {
  return ActiveContext ? ActiveContext->SelectedStyle : nullptr;
}

void enableWFormatPresetExtensions(FormatStyle::LanguageKind Language) {
  if (ActiveContext)
    ActiveContext->Styles[Language] = ExtensionStyle::getWFormatPresetStyle();
}

void mapExtensionConfiguration(llvm::yaml::IO &IO,
                               FormatStyle::LanguageKind Language,
                               llvm::StringRef BasedOnStyle) {
  if (!ActiveContext)
    return;

  ExtensionStyle *Style = ActiveContext->SelectedStyle;
  if (!IO.outputting()) {
    if (Language == FormatStyle::LK_None &&
        BasedOnStyle.equals_insensitive("wformat")) {
      ActiveContext->Styles.clear();
    }
    auto [It, Inserted] = ActiveContext->Styles.try_emplace(Language);
    if (Inserted && Language != FormatStyle::LK_None) {
      const auto Default = ActiveContext->Styles.find(FormatStyle::LK_None);
      if (Default != ActiveContext->Styles.end())
        It->second = Default->second;
    }
    if (BasedOnStyle.equals_insensitive("wformat")) {
      It->second = ExtensionStyle::getWFormatPresetStyle();
    } else if (!BasedOnStyle.empty() &&
               !BasedOnStyle.equals_insensitive("inheritparentconfig")) {
      It->second = ExtensionStyle();
    }
    Style = &It->second;
  } else if (!Style->anyEnabled()) {
    return;
  }
  IO.mapOptional("WFormatExtensions", *Style);
}

} // namespace clang::format::extensions

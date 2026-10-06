#ifndef CUSTOM_CLANG_FORMAT_EXTENSION_HOOKS_H
#define CUSTOM_CLANG_FORMAT_EXTENSION_HOOKS_H

#include "clang/Format/Format.h"
#include "clang/Tooling/Core/Replacement.h"
#include "llvm/ADT/StringRef.h"

#include <optional>

namespace llvm::yaml {
class IO;
}

namespace clang::format {

class AnnotatedLine;
class Environment;
struct FormatToken;
struct LineState;

namespace extensions {

bool getExtensionPredefinedStyle(llvm::StringRef Name,
                                 FormatStyle::LanguageKind Language,
                                 FormatStyle *Style);
void mapExtensionConfiguration(llvm::yaml::IO &IO,
                               FormatStyle::LanguageKind Language,
                               llvm::StringRef BasedOnStyle);
void customizeAnnotatedLine(AnnotatedLine &Line, const FormatStyle &Style);
std::optional<bool> getSpaceRequiredBefore(const AnnotatedLine &Line,
                                           const FormatToken &Right);
std::optional<unsigned> getExtensionNewLineColumn(const LineState &State,
                                                  const FormatStyle &Style);
std::pair<tooling::Replacements, unsigned>
runExtensionPostFormatPass(const Environment &Env, const FormatStyle &Style);

} // namespace extensions
} // namespace clang::format

#endif

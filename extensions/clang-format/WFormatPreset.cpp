#include "integration/clang-format/ExtensionContext.h"
#include "integration/clang-format/ExtensionHooks.h"

#include "WFormatPresetConfig.h"

#include "clang/Format/Format.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/raw_ostream.h"

#include <system_error>
#include <utility>

namespace clang::format::extensions {

bool getExtensionPredefinedStyle(llvm::StringRef Name,
                                 FormatStyle::LanguageKind Language,
                                 FormatStyle *Style) {
  if (!Name.equals_insensitive("wformat"))
    return false;
  if (Language != FormatStyle::LK_Cpp) {
    *Style = getLLVMStyle(Language);
    return true;
  }
  if (!getActiveExtensionStyleConst())
    return false;

  FormatStyle Preset = getLLVMStyle(Language);
  const std::error_code Error = parseConfiguration(
      llvm::MemoryBufferRef(WFormatPresetConfiguration, "WFormat preset"),
      &Preset);
  if (Error) {
    llvm::errs() << "Failed to parse the embedded WFormat preset: "
                 << Error.message() << "\n";
    return false;
  }

  Preset.ColumnLimit = 140;
  Preset.AlignArrayOfStructures = FormatStyle::AIAS_Right;
  Preset.AlignConsecutiveAssignments.Enabled = false;
  Preset.AlignOperands = FormatStyle::OAS_DontAlign;
  Preset.BreakBeforeBinaryOperators = FormatStyle::BOS_All;
  Preset.BreakBinaryOperations = FormatStyle::BBO_RespectPrecedence;
  Preset.PenaltyBreakAssignment = 1000;
  enableWFormatPresetExtensions(Language);
  Preset.Language = Language;
  *Style = std::move(Preset);
  return true;
}

} // namespace clang::format::extensions

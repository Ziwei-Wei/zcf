cmake_minimum_required(VERSION 3.20)

project(Zcf LANGUAGES CXX)

if(NOT CUSTOM_CLANG_FORMAT_SOURCE_DIR)
  message(FATAL_ERROR "CUSTOM_CLANG_FORMAT_SOURCE_DIR is required")
endif()

if(NOT CUSTOM_UNIFIED_FORMAT_SOURCE_DIR)
  message(FATAL_ERROR "CUSTOM_UNIFIED_FORMAT_SOURCE_DIR is required")
endif()

if(NOT CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR)
  message(FATAL_ERROR "CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR is required")
endif()

if(NOT CUSTOM_CLANG_FORMAT_INCLUDE_DIR)
  message(FATAL_ERROR "CUSTOM_CLANG_FORMAT_INCLUDE_DIR is required")
endif()

if(NOT CUSTOM_CLANG_FORMAT_REPOSITORY_DIR)
  message(FATAL_ERROR "CUSTOM_CLANG_FORMAT_REPOSITORY_DIR is required")
endif()

if(NOT CUSTOM_CLANG_FORMAT_INTEGRATION_SOURCE_DIR)
  message(FATAL_ERROR "CUSTOM_CLANG_FORMAT_INTEGRATION_SOURCE_DIR is required")
endif()

if(NOT CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR)
  message(FATAL_ERROR "CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR is required")
endif()

if(NOT CUSTOM_WFORMAT_PRESET_INCLUDE_DIR)
  message(FATAL_ERROR "CUSTOM_WFORMAT_PRESET_INCLUDE_DIR is required")
endif()

if(NOT ZCF_OUTPUT_NAME)
  set(ZCF_OUTPUT_NAME zcf)
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

set(CUSTOM_FORMAT_SOURCES
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/AffectedRangeManager.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/BreakableToken.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/ContinuationIndenter.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/DefinitionBlockSeparator.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/Format.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/FormatToken.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/FormatTokenLexer.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/IntegerLiteralSeparatorFixer.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/MacroCallReconstructor.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/MacroExpander.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/MatchFilePath.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/NamespaceEndCommentsFixer.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/ObjCPropertyAttributeOrderFixer.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/QualifierAlignmentFixer.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/SortJavaScriptImports.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/TokenAnalyzer.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/TokenAnnotator.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/UnwrappedLineFormatter.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/UnwrappedLineParser.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/UsingDeclarationsSorter.cpp"
  "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/WhitespaceManager.cpp"
  "${CUSTOM_CLANG_FORMAT_INTEGRATION_SOURCE_DIR}/ExtensionContext.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/P0Hooks.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/P1Layout.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/P1Spacing.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/P1PostFormat.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/P2Layout.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/P2Spacing.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/P2PostFormat.cpp"
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/WFormatPreset.cpp")

add_library(zcfClangFormat STATIC
  ${CUSTOM_FORMAT_SOURCES})

target_compile_features(zcfClangFormat PUBLIC cxx_std_17)

target_include_directories(zcfClangFormat
  PUBLIC
    "${CUSTOM_CLANG_FORMAT_INCLUDE_DIR}"
    "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}"
    "${CUSTOM_CLANG_FORMAT_REPOSITORY_DIR}"
    "${CUSTOM_CLANG_FORMAT_INTEGRATION_SOURCE_DIR}"
    "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}"
    "${CUSTOM_WFORMAT_PRESET_INCLUDE_DIR}"
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS})

target_link_libraries(zcfClangFormat
  PUBLIC
    clangBasic
    clangLex
    clangToolingCore
    clangToolingInclusions
    LLVMSupport)

add_executable(zcf-local
  "${CUSTOM_CLANG_FORMAT_SOURCE_DIR}/ClangFormat.cpp")

set_target_properties(zcf-local PROPERTIES
  OUTPUT_NAME "${ZCF_OUTPUT_NAME}")

target_compile_features(zcf-local PRIVATE cxx_std_17)

target_include_directories(zcf-local
  PRIVATE
    "${CUSTOM_CLANG_FORMAT_SOURCE_DIR}"
    "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}"
    "${CUSTOM_CLANG_FORMAT_REPOSITORY_DIR}")

target_link_libraries(zcf-local
  PRIVATE
    zcfClangFormat
    clangRewrite
    clangToolingCore
    LLVMSupport)

add_executable(custom-format-test-driver-local
  "${CUSTOM_UNIFIED_FORMAT_SOURCE_DIR}/CustomFormat.cpp")

set_target_properties(custom-format-test-driver-local PROPERTIES
  OUTPUT_NAME "custom-format-test-driver")

target_compile_features(custom-format-test-driver-local PRIVATE cxx_std_17)
target_compile_definitions(custom-format-test-driver-local PRIVATE
  ZCF_LLVM_VERSION="${LLVM_PACKAGE_VERSION}")
add_dependencies(custom-format-test-driver-local zcf-local)

if(MSVC)
  foreach(_target zcfClangFormat zcf-local)
    target_compile_definitions(${_target} PRIVATE _HAS_EXCEPTIONS=0)
    target_compile_options(${_target} PRIVATE /bigobj /EHs-c- /GR-)
  endforeach()
endif()

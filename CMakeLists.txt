cmake_minimum_required(VERSION 3.20)

project(ZcfSuperbuild LANGUAGES NONE)

include(ExternalProject)

get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_is_multi_config)
  message(FATAL_ERROR
    "This scaffold intentionally uses a single-config generator so the "
    "ExternalProject output paths stay deterministic. Reconfigure with Ninja, "
    "for example: cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(LLVM_PROJECT_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/llvm-project"
  CACHE PATH
  "Path to an llvm-project checkout containing llvm/, clang/, and cmake/.")

set(LLVM_TARGETS_TO_BUILD
  "host"
  CACHE STRING
  "LLVM targets to build. Use host for smallest native clang-format builds.")

set(ZCF_OUTPUT_NAME
  "zcf"
  CACHE STRING
  "Public z-clang-format executable name.")

set(CUSTOM_CLANG_FORMAT_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/clang-format"
  CACHE PATH
  "Path to the repo-owned custom clang-format source directory.")

set(CUSTOM_UNIFIED_FORMAT_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/custom-format"
  CACHE PATH
  "Path to the repo-owned unified formatter frontend source directory.")

set(CUSTOM_LLVM_OVERLAY_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/build/llvm-overlay"
  CACHE PATH
  "Path to the generated LLVM worktree containing the extension bridge.")

set(CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR
  "${CUSTOM_LLVM_OVERLAY_SOURCE_DIR}/clang/lib/Format"
  CACHE PATH
  "Path to the generated overlay clang Format library source directory.")

set(CUSTOM_CLANG_FORMAT_INCLUDE_DIR
  "${CUSTOM_LLVM_OVERLAY_SOURCE_DIR}/clang/include"
  CACHE PATH
  "Path to the generated overlay public clang Format include directory.")

set(CUSTOM_CLANG_FORMAT_INTEGRATION_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/integration/clang-format"
  CACHE PATH
  "Path to the private LibFormat integration adapter.")

set(CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/extensions/clang-format"
  CACHE PATH
  "Path to extension-owned formatting behavior.")

set(CUSTOM_CPP_STYLE_EXAMPLE_SOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples"
  CACHE PATH
  "Directory containing the canonical C++ fixture used by the format-cpp-style-files target.")

set(CUSTOM_CPP_SIGNATURE_STYLE_FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples/wformat-signatures.clang-format"
  CACHE FILEPATH
  "Signature, initializer, and return target style file.")

set(CUSTOM_CPP_STRUCTURE_STYLE_FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples/wformat-structures.clang-format"
  CACHE FILEPATH
  "Structural layout, spacing, and post-format target style file.")

set(CUSTOM_CPP_COMPLETE_STYLE_FILE
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples/wformat-complete.clang-format"
  CACHE FILEPATH
  "Complete reviewed C++ target style file.")

set(CUSTOM_BUILD_JOBS
  ""
  CACHE STRING
  "Optional parallel build job count passed to child builds.")

set(CUSTOM_LLVM_DIR
  ""
  CACHE PATH
  "Optional path to an existing static LLVMConfig.cmake directory. If set, the LLVM prerequisite build is skipped.")

set(CUSTOM_LLVM_TABLEGEN
  ""
  CACHE FILEPATH
  "Optional path to llvm-tblgen when using CUSTOM_LLVM_DIR.")

option(CUSTOM_STATIC_MSVC_RUNTIME
  "Use /MT or /MTd with MSVC in addition to statically linking LLVM/Clang libs."
  OFF)

option(CUSTOM_ENABLE_ASSERTIONS
  "Enable LLVM/Clang assertions in the custom clang-format build."
  OFF)

find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell)

execute_process(
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_llvm_overlay.py"
    --llvm-project-source-dir "${LLVM_PROJECT_SOURCE_DIR}"
    --overlay-dir "${CUSTOM_LLVM_OVERLAY_SOURCE_DIR}"
  RESULT_VARIABLE _overlay_result
  OUTPUT_VARIABLE _overlay_output
  ERROR_VARIABLE _overlay_error)
if(NOT _overlay_result EQUAL 0)
  message(FATAL_ERROR
    "Failed to prepare the generated LLVM overlay.\n"
    "${_overlay_output}${_overlay_error}")
endif()

set(_wformat_preset_source
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples/wformat-target.clang-format")
set(_wformat_preset_template
  "${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}/WFormatPresetConfig.h.in")
set(_wformat_preset_include_dir "${CMAKE_BINARY_DIR}/generated")
set(_wformat_preset_header
  "${_wformat_preset_include_dir}/WFormatPresetConfig.h")
file(MAKE_DIRECTORY "${_wformat_preset_include_dir}")
file(READ "${_wformat_preset_source}" CUSTOM_WFORMAT_PRESET_CONFIG)
configure_file(
  "${_wformat_preset_template}"
  "${_wformat_preset_header}"
  @ONLY)
add_custom_target(generate-wformat-preset
  COMMAND ${CMAKE_COMMAND}
    "-DINPUT_FILE=${_wformat_preset_source}"
    "-DTEMPLATE_FILE=${_wformat_preset_template}"
    "-DOUTPUT_FILE=${_wformat_preset_header}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate-wformat-preset.cmake"
  BYPRODUCTS "${_wformat_preset_header}"
  DEPENDS
    "${_wformat_preset_source}"
    "${_wformat_preset_template}"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate-wformat-preset.cmake"
  VERBATIM)

add_custom_target(prepare-llvm-overlay
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_llvm_overlay.py"
    --llvm-project-source-dir "${LLVM_PROJECT_SOURCE_DIR}"
    --overlay-dir "${CUSTOM_LLVM_OVERLAY_SOURCE_DIR}"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_llvm_overlay.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/patches/0001-custom-format-extension-bridge.patch"
  USES_TERMINAL)

foreach(_required_path
    "llvm/CMakeLists.txt"
    "clang/CMakeLists.txt"
    "llvm/cmake/modules/AddLLVM.cmake")
  if(NOT EXISTS "${LLVM_PROJECT_SOURCE_DIR}/${_required_path}")
    message(FATAL_ERROR
      "LLVM_PROJECT_SOURCE_DIR must point at a full llvm-project checkout. "
      "Missing: ${LLVM_PROJECT_SOURCE_DIR}/${_required_path}")
  endif()
endforeach()

if(NOT EXISTS "${CUSTOM_CLANG_FORMAT_SOURCE_DIR}/ClangFormat.cpp")
  message(FATAL_ERROR
    "CUSTOM_CLANG_FORMAT_SOURCE_DIR must contain ClangFormat.cpp. "
    "Missing: ${CUSTOM_CLANG_FORMAT_SOURCE_DIR}/ClangFormat.cpp")
endif()

if(NOT EXISTS "${CUSTOM_UNIFIED_FORMAT_SOURCE_DIR}/CustomFormat.cpp")
  message(FATAL_ERROR
    "CUSTOM_UNIFIED_FORMAT_SOURCE_DIR must contain CustomFormat.cpp. "
    "Missing: ${CUSTOM_UNIFIED_FORMAT_SOURCE_DIR}/CustomFormat.cpp")
endif()

if(NOT EXISTS "${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/Format.cpp")
  message(FATAL_ERROR
    "CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR must contain Format.cpp. "
    "Missing: ${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}/Format.cpp")
endif()

if(NOT EXISTS "${CUSTOM_CLANG_FORMAT_INCLUDE_DIR}/clang/Format/Format.h")
  message(FATAL_ERROR
    "CUSTOM_CLANG_FORMAT_INCLUDE_DIR must contain clang/Format/Format.h. "
    "Missing: ${CUSTOM_CLANG_FORMAT_INCLUDE_DIR}/clang/Format/Format.h")
endif()

set(_llvm_build_dir "${CMAKE_BINARY_DIR}/llvm-build")
set(_clang_build_dir "${CMAKE_BINARY_DIR}/clang-build")
set(_local_tool_build_dir "${CMAKE_BINARY_DIR}/zcf-build")
set(_output_dir "${CMAKE_BINARY_DIR}/bin")
set(_zcf_exe
  "${_output_dir}/${ZCF_OUTPUT_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
set(_clang_format_compat_exe
  "${_output_dir}/clang-format${CMAKE_EXECUTABLE_SUFFIX}")
set(_local_zcf_exe
  "${_local_tool_build_dir}/${ZCF_OUTPUT_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
set(_local_custom_format_test_driver_exe
  "${_local_tool_build_dir}/custom-format-test-driver${CMAKE_EXECUTABLE_SUFFIX}")
set(_third_party_clang_format_exe
  "${_clang_build_dir}/bin/clang-format${CMAKE_EXECUTABLE_SUFFIX}")

set(_ep_generator_args CMAKE_GENERATOR "${CMAKE_GENERATOR}")
if(CMAKE_GENERATOR_PLATFORM)
  list(APPEND _ep_generator_args CMAKE_GENERATOR_PLATFORM "${CMAKE_GENERATOR_PLATFORM}")
endif()
if(CMAKE_GENERATOR_TOOLSET)
  list(APPEND _ep_generator_args CMAKE_GENERATOR_TOOLSET "${CMAKE_GENERATOR_TOOLSET}")
endif()

set(_common_cmake_args
  "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
  "-DBUILD_SHARED_LIBS:BOOL=OFF"
  "-DLLVM_BUILD_LLVM_DYLIB:BOOL=OFF"
  "-DLLVM_LINK_LLVM_DYLIB:BOOL=OFF"
  "-DLLVM_ENABLE_ASSERTIONS:BOOL=${CUSTOM_ENABLE_ASSERTIONS}"
  "-DLLVM_ENABLE_LIBXML2:BOOL=OFF"
  "-DLLVM_ENABLE_TERMINFO:BOOL=OFF"
  "-DLLVM_ENABLE_ZLIB:STRING=OFF"
  "-DLLVM_ENABLE_ZSTD:STRING=OFF")

if(WIN32 AND CUSTOM_STATIC_MSVC_RUNTIME)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND _common_cmake_args
      "-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreadedDebug")
  else()
    list(APPEND _common_cmake_args
      "-DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreaded")
  endif()
endif()

foreach(_tool_var CMAKE_C_COMPILER CMAKE_CXX_COMPILER CMAKE_MAKE_PROGRAM)
  if(DEFINED ${_tool_var} AND NOT "${${_tool_var}}" STREQUAL "")
    list(APPEND _common_cmake_args "-D${_tool_var}:FILEPATH=${${_tool_var}}")
  endif()
endforeach()

set(_parallel_args)
if(CUSTOM_BUILD_JOBS)
  list(APPEND _parallel_args --parallel "${CUSTOM_BUILD_JOBS}")
endif()

set(_clang_depends)
set(_clang_llvm_dir "${_llvm_build_dir}/lib/cmake/llvm")
set(_clang_tablegen_arg
  "-DLLVM_TABLEGEN:FILEPATH=${_llvm_build_dir}/bin/llvm-tblgen${CMAKE_EXECUTABLE_SUFFIX}")

if(CUSTOM_LLVM_DIR)
  if(NOT EXISTS "${CUSTOM_LLVM_DIR}/LLVMConfig.cmake")
    message(FATAL_ERROR
      "CUSTOM_LLVM_DIR must point at the directory containing LLVMConfig.cmake. "
      "Missing: ${CUSTOM_LLVM_DIR}/LLVMConfig.cmake")
  endif()
  set(_clang_llvm_dir "${CUSTOM_LLVM_DIR}")
  set(_clang_tablegen_arg)
  if(CUSTOM_LLVM_TABLEGEN)
    if(NOT EXISTS "${CUSTOM_LLVM_TABLEGEN}")
      message(FATAL_ERROR "CUSTOM_LLVM_TABLEGEN does not exist: ${CUSTOM_LLVM_TABLEGEN}")
    endif()
    set(_clang_tablegen_arg "-DLLVM_TABLEGEN:FILEPATH=${CUSTOM_LLVM_TABLEGEN}")
  endif()
else()
  ExternalProject_Add(llvm-static-prereqs
    ${_ep_generator_args}
    SOURCE_DIR "${LLVM_PROJECT_SOURCE_DIR}/llvm"
    BINARY_DIR "${_llvm_build_dir}"
    CMAKE_ARGS
      ${_common_cmake_args}
      "-DLLVM_ENABLE_PROJECTS:STRING="
      "-DLLVM_TARGETS_TO_BUILD:STRING=${LLVM_TARGETS_TO_BUILD}"
      "-DLLVM_INCLUDE_BENCHMARKS:BOOL=OFF"
      "-DLLVM_INCLUDE_EXAMPLES:BOOL=OFF"
      "-DLLVM_INCLUDE_TESTS:BOOL=OFF"
      "-DLLVM_INCLUDE_TOOLS:BOOL=ON"
      "-DLLVM_BUILD_TOOLS:BOOL=ON"
      "-DLLVM_BUILD_UTILS:BOOL=OFF"
      "-DLLVM_INSTALL_TOOLCHAIN_ONLY:BOOL=OFF"
    BUILD_COMMAND
      ${CMAKE_COMMAND} --build <BINARY_DIR>
        --target llvm-tblgen omp_gen
        LLVMSupport LLVMTargetParser LLVMFrontendOpenMP
        ${_parallel_args}
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE)
  set(_clang_depends DEPENDS llvm-static-prereqs)
endif()

ExternalProject_Add(clang-static-libs
  ${_ep_generator_args}
  ${_clang_depends}
  SOURCE_DIR "${LLVM_PROJECT_SOURCE_DIR}/clang"
  BINARY_DIR "${_clang_build_dir}"
  CMAKE_ARGS
    ${_common_cmake_args}
    "-DLLVM_DIR:PATH=${_clang_llvm_dir}"
    "-DLLVM_CMAKE_DIR:PATH=${_clang_llvm_dir}"
    ${_clang_tablegen_arg}
    "-DLLVM_INCLUDE_TESTS:BOOL=OFF"
    "-DCLANG_INCLUDE_TESTS:BOOL=OFF"
    "-DCLANG_ENABLE_ARCMT:BOOL=OFF"
    "-DCLANG_ENABLE_STATIC_ANALYZER:BOOL=OFF"
    "-DCLANG_ENABLE_LIBXML2:BOOL=OFF"
    "-DCLANG_LINK_CLANG_DYLIB:BOOL=OFF"
  BUILD_COMMAND
    ${CMAKE_COMMAND} --build <BINARY_DIR>
      --target clangBasic clangLex clangRewrite clangToolingCore
      clangToolingInclusions
      ${_parallel_args}
  INSTALL_COMMAND ""
  BUILD_ALWAYS TRUE
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
  USES_TERMINAL_INSTALL TRUE)

ExternalProject_Add(zcf-local
  ${_ep_generator_args}
  DEPENDS clang-static-libs prepare-llvm-overlay generate-wformat-preset
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/local-clang-format"
  BINARY_DIR "${_local_tool_build_dir}"
  CMAKE_ARGS
    "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
    "-DBUILD_SHARED_LIBS:BOOL=OFF"
    "-DLLVM_DIR:PATH=${_clang_llvm_dir}"
    "-DClang_DIR:PATH=${_clang_build_dir}/lib/cmake/clang"
    "-DCUSTOM_CLANG_FORMAT_SOURCE_DIR:PATH=${CUSTOM_CLANG_FORMAT_SOURCE_DIR}"
    "-DCUSTOM_UNIFIED_FORMAT_SOURCE_DIR:PATH=${CUSTOM_UNIFIED_FORMAT_SOURCE_DIR}"
    "-DCUSTOM_CLANG_FORMAT_REPOSITORY_DIR:PATH=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DCUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR:PATH=${CUSTOM_CLANG_FORMAT_ENGINE_SOURCE_DIR}"
    "-DCUSTOM_CLANG_FORMAT_INCLUDE_DIR:PATH=${CUSTOM_CLANG_FORMAT_INCLUDE_DIR}"
    "-DCUSTOM_CLANG_FORMAT_INTEGRATION_SOURCE_DIR:PATH=${CUSTOM_CLANG_FORMAT_INTEGRATION_SOURCE_DIR}"
    "-DCUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR:PATH=${CUSTOM_CLANG_FORMAT_EXTENSION_SOURCE_DIR}"
    "-DCUSTOM_WFORMAT_PRESET_INCLUDE_DIR:PATH=${_wformat_preset_include_dir}"
    "-DZCF_OUTPUT_NAME:STRING=${ZCF_OUTPUT_NAME}"
  BUILD_COMMAND
    ${CMAKE_COMMAND} --build <BINARY_DIR>
      --target zcf-local
      ${_parallel_args}
  INSTALL_COMMAND
    ${CMAKE_COMMAND} -E make_directory "${_output_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_local_zcf_exe}"
      "${_zcf_exe}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${_local_zcf_exe}"
      "${_clang_format_compat_exe}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${LLVM_PROJECT_SOURCE_DIR}/LICENSE.TXT"
      "${_output_dir}/LICENSE.txt"
  BUILD_ALWAYS TRUE
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
  USES_TERMINAL_INSTALL TRUE)

add_custom_target(zcf
  DEPENDS zcf-local)

add_custom_target(custom-format-test-driver
  COMMAND ${CMAKE_COMMAND} --build "${_local_tool_build_dir}"
    --target custom-format-test-driver-local
    ${_parallel_args}
  DEPENDS zcf-local
  USES_TERMINAL)

add_custom_target(third-party-clang-format
  COMMAND ${CMAKE_COMMAND} --build "${_clang_build_dir}"
    --target clang-format
    ${_parallel_args}
  DEPENDS clang-static-libs
  USES_TERMINAL)

ExternalProject_Add(format-comparison-gtest-project
  ${_ep_generator_args}
  DEPENDS custom-format-test-driver third-party-clang-format
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/format-comparison-gtest"
  BINARY_DIR "${CMAKE_BINARY_DIR}/format-comparison-gtest-build"
  CMAKE_ARGS
    "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
    "-DZCF_EXE:FILEPATH=${_zcf_exe}"
    "-DCUSTOM_FORMAT_TEST_DRIVER_EXE:FILEPATH=${_local_custom_format_test_driver_exe}"
    "-DCUSTOM_UPSTREAM_CLANG_FORMAT_EXE:FILEPATH=${_third_party_clang_format_exe}"
    "-DCUSTOM_GTEST_WORK_DIR:PATH=${CMAKE_BINARY_DIR}/format-comparison-gtest"
  BUILD_COMMAND
    ${CMAKE_COMMAND} --build <BINARY_DIR>
      --target run-format-comparison-gtest
      ${_parallel_args}
  INSTALL_COMMAND ""
  BUILD_ALWAYS TRUE
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE)

add_custom_target(compare-format-gtest
  DEPENDS format-comparison-gtest-project)

if(POWERSHELL_EXECUTABLE)
  add_custom_target(format-cpp-style-files
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate-cpp-style-files.ps1"
      -FormatterExe "${_zcf_exe}"
      -InputDir "${CUSTOM_CPP_STYLE_EXAMPLE_SOURCE_DIR}"
      -OutputDir "${CMAKE_BINARY_DIR}/cpp-style-files"
      -StyleFile "${CUSTOM_CPP_SIGNATURE_STYLE_FILE}"
    DEPENDS zcf
    USES_TERMINAL)

  add_custom_target(smoke-zcf
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke-zcf.ps1"
      -ZcfExe "${_zcf_exe}"
      -ClangFormatCompatExe "${_clang_format_compat_exe}"
      -WorkDir "${CMAKE_BINARY_DIR}/zcf-smoke"
    DEPENDS zcf
    USES_TERMINAL)

  add_custom_target(validate-signature-format
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/validate-signature-format.ps1"
      -FormatterExe "${_zcf_exe}"
      -StyleFile "${CUSTOM_CPP_SIGNATURE_STYLE_FILE}"
      -UpstreamStyleFile "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples/wformat-target.clang-format"
      -WorkDir "${CMAKE_BINARY_DIR}/signature-format-tests"
    DEPENDS zcf
    USES_TERMINAL)

  add_custom_target(validate-structure-format
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/validate-structure-format.ps1"
      -FormatterExe "${_zcf_exe}"
      -StyleFile "${CUSTOM_CPP_STRUCTURE_STYLE_FILE}"
      -UpstreamStyleFile "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples/wformat-target.clang-format"
      -WorkDir "${CMAKE_BINARY_DIR}/structure-format-tests"
    DEPENDS zcf
    USES_TERMINAL)

  add_custom_target(validate-complete-format
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/validate-complete-format.ps1"
      -FormatterExe "${_zcf_exe}"
      -StyleFile "${CUSTOM_CPP_COMPLETE_STYLE_FILE}"
      -UpstreamStyleFile "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp-style-examples/wformat-target.clang-format"
      -WorkDir "${CMAKE_BINARY_DIR}/complete-format-tests"
    DEPENDS zcf
    USES_TERMINAL)

  add_custom_target(compare-format
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
      -File "${CMAKE_CURRENT_SOURCE_DIR}/tests/compare-format.ps1"
      -CustomExe "${_zcf_exe}"
      -UpstreamExe "${_third_party_clang_format_exe}"
      -WorkDir "${CMAKE_BINARY_DIR}/format-comparison-tests"
    DEPENDS zcf third-party-clang-format
    USES_TERMINAL)
else()
  message(WARNING "PowerShell was not found; compare-format target will not be available.")
endif()

add_custom_target(print-output-path
  COMMAND ${CMAKE_COMMAND} -E echo "${_zcf_exe}")

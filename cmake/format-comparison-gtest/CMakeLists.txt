cmake_minimum_required(VERSION 3.20)

project(CustomClangFormatGTest LANGUAGES CXX)

include(FetchContent)

if(NOT ZCF_EXE)
  message(FATAL_ERROR "ZCF_EXE is required")
endif()

if(NOT CUSTOM_FORMAT_TEST_DRIVER_EXE)
  message(FATAL_ERROR "CUSTOM_FORMAT_TEST_DRIVER_EXE is required")
endif()

if(NOT CUSTOM_UPSTREAM_CLANG_FORMAT_EXE)
  message(FATAL_ERROR "CUSTOM_UPSTREAM_CLANG_FORMAT_EXE is required")
endif()

if(NOT CUSTOM_GTEST_WORK_DIR)
  set(CUSTOM_GTEST_WORK_DIR "${CMAKE_BINARY_DIR}/format-comparison-gtest")
endif()

set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(googletest)

add_executable(format-comparison-gtest
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/format_comparison_support.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_c_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_cmake_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_cpp_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_csharp_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_java_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_javascript_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_json_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_objc_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_proto_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_styles_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_swift_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_tablegen_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_textproto_gtest.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/compare_verilog_gtest.cpp")

target_compile_features(format-comparison-gtest PRIVATE cxx_std_17)
target_link_libraries(format-comparison-gtest PRIVATE GTest::gtest)

add_custom_target(run-format-comparison-gtest
  COMMAND "$<TARGET_FILE:format-comparison-gtest>"
    "--zcf=${ZCF_EXE}"
    "--custom-format-test-driver=${CUSTOM_FORMAT_TEST_DRIVER_EXE}"
    "--upstream-clang-format=${CUSTOM_UPSTREAM_CLANG_FORMAT_EXE}"
    "--work-dir=${CUSTOM_GTEST_WORK_DIR}"
  DEPENDS format-comparison-gtest
  USES_TERMINAL)

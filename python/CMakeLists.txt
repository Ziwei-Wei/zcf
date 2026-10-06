cmake_minimum_required(VERSION 3.20)

project(ZcfPythonPackage LANGUAGES NONE)

set(ZCF_EXECUTABLE
  ""
  CACHE FILEPATH
  "Path to the prebuilt zcf executable to include in the wheel.")

if(NOT ZCF_EXECUTABLE)
  if(WIN32)
    set(_zcf_name "zcf.exe")
  else()
    set(_zcf_name "zcf")
  endif()
  get_filename_component(_repo_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
  set(ZCF_EXECUTABLE "${_repo_root}/build/release/bin/${_zcf_name}")
endif()

if(NOT EXISTS "${ZCF_EXECUTABLE}")
  message(FATAL_ERROR
    "ZCF_EXECUTABLE must name a prebuilt zcf binary. Build the zcf target "
    "before creating a wheel. Missing: ${ZCF_EXECUTABLE}")
endif()

get_filename_component(_repo_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(_zcf_skill "${_repo_root}/.github/skills/zcf/SKILL.md")
if(NOT EXISTS "${_zcf_skill}")
  message(FATAL_ERROR "The packaged zcf agent skill was not found: ${_zcf_skill}")
endif()

install(PROGRAMS "${ZCF_EXECUTABLE}"
  DESTINATION bin
  COMPONENT python)

install(FILES "${_zcf_skill}"
  DESTINATION skills/zcf
  COMPONENT python)

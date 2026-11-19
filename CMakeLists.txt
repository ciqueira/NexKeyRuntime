cmake_minimum_required(VERSION 3.20)

# Match MSVC static runtime (/MT, /MTd) used by published nexkeyruntime.lib
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(NexKeyRuntimeWorkspace LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Canonical output directories under build/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(NEXKEY_BUILD_CLI "Build nexkeyctl command-line utility" ON)
option(NEXKEY_BUILD_EXAMPLES "Build minimal SDK examples (01-05)" ON)
option(NEXKEY_BUILD_OFX "Build OpenFX host plugins (fetches OpenFX SDK if OFX_ROOT is unset)" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexKeyRuntimeSetup.cmake")
nexkey_setup_runtime()

if(NEXKEY_BUILD_CLI)
  add_subdirectory(tools/nexkeyctl)
endif()

if(NEXKEY_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(NEXKEY_BUILD_OFX)
  add_subdirectory(sandbox/ofx-plugin)

  nexkey_find_nexcontrol()
  if(TARGET NexKeyRuntime::nexcontrol)
    add_subdirectory(examples/06-nexcontrol-ofx)
  else()
    message(STATUS
      "NexKeyRuntime: skipping examples/06-nexcontrol-ofx (optional libnexcontrol not found in ${CMAKE_CURRENT_SOURCE_DIR}/lib)")
  endif()
endif()

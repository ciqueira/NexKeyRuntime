cmake_minimum_required(VERSION 3.20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(NexKeyRuntimeExamples LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/NexKeyRuntimeSetup.cmake")
nexkey_setup_runtime()

message(STATUS "=== Building NexKeyRuntime Minimal Examples ===")

# Subdirectories for each isolated SDK feature:
add_subdirectory(01-minimal)
add_subdirectory(02-activation)
add_subdirectory(03-updates)
add_subdirectory(04-notices)
add_subdirectory(05-offline)

cmake_minimum_required(VERSION 3.20)

# Match MSVC runtime library (/MT, /MTd) for Windows builds
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(nexkeyctl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/NexKeyRuntimeSetup.cmake")
nexkey_setup_runtime()
nexkey_find_packager()

add_executable(nexkeyctl main.cpp)
target_include_directories(nexkeyctl PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)
target_link_libraries(nexkeyctl PRIVATE NexKeyRuntime::NexKeyRuntime)
if(TARGET NexKeyRuntime::Packager)
  target_link_libraries(nexkeyctl PRIVATE NexKeyRuntime::Packager)
endif()

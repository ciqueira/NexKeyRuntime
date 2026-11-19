cmake_minimum_required(VERSION 3.20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(updates_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/NexKeyRuntimeSetup.cmake")
nexkey_setup_runtime()

add_executable(updates-example main.c)
set_target_properties(updates-example PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(updates-example PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

target_link_libraries(updates-example PRIVATE NexKeyRuntime::NexKeyRuntime)

cmake_minimum_required(VERSION 3.20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(notices_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/NexKeyRuntimeSetup.cmake")
nexkey_setup_runtime()

add_executable(notices-example main.c)
set_target_properties(notices-example PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(notices-example PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

target_link_libraries(notices-example PRIVATE NexKeyRuntime::NexKeyRuntime)

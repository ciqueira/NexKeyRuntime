cmake_minimum_required(VERSION 3.20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(standalone_validator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/NexKeyRuntimeSetup.cmake")
nexkey_setup_runtime()

add_executable(license-validator main.cpp)

target_compile_definitions(license-validator PRIVATE
  MC_NEXKEY_ENABLED=1
  MC_NEXKEY_ENFORCE=1
  MC_NEXKEY_TENANT="mcnexus-sandbox"
  MC_NEXKEY_VARIANT="download:default"
)

target_include_directories(license-validator PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

target_link_libraries(license-validator PRIVATE NexKeyRuntime::NexKeyRuntime)

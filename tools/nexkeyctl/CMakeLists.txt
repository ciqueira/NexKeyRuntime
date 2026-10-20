cmake_minimum_required(VERSION 3.20)

# Match MSVC runtime library (/MT) for Windows builds
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(nexkeyctl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Look for NexKeyRuntime CMake package
find_package(NexKeyRuntime CONFIG QUIET)

# 2. Fallback: Search local paths if package was not found via CMAKE_PREFIX_PATH
if(NOT NexKeyRuntime_FOUND)
  find_library(NEXKEYRUNTIME_LIB NAMES nexkeyruntime libnexkeyruntime
    PATHS
      "${CMAKE_CURRENT_SOURCE_DIR}/../../build"
      "${CMAKE_CURRENT_SOURCE_DIR}/../../../build"
      "${CMAKE_CURRENT_SOURCE_DIR}/../../lib"
      "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/nexkeyruntime-0.5.4/lib"
      "${CMAKE_CURRENT_SOURCE_DIR}/../../../../MCPlugins/third_party/nexkeyruntime/lib/macos"
  )
  find_path(NEXKEYRUNTIME_INCLUDE_DIR NAMES nexkeyruntime/nexkeyruntime.h
    PATHS
      "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
      "${CMAKE_CURRENT_SOURCE_DIR}/../../../include"
  )

  if(NEXKEYRUNTIME_LIB AND NEXKEYRUNTIME_INCLUDE_DIR)
    add_library(NexKeyRuntime::NexKeyRuntime STATIC IMPORTED GLOBAL)
    set_target_properties(NexKeyRuntime::NexKeyRuntime PROPERTIES
      IMPORTED_LOCATION "${NEXKEYRUNTIME_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${NEXKEYRUNTIME_INCLUDE_DIR}"
    )
    set(NexKeyRuntime_FOUND TRUE)
  endif()
endif()

if(NOT NexKeyRuntime_FOUND)
  message(FATAL_ERROR "NexKeyRuntime library not found. Please provide -DCMAKE_PREFIX_PATH=<path_to_sdk_package> or compile the SDK first.")
endif()

add_executable(nexkeyctl main.cpp)
target_include_directories(nexkeyctl PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
target_link_libraries(nexkeyctl PRIVATE NexKeyRuntime::NexKeyRuntime)

if(APPLE)
  find_library(SODIUM_LIB NAMES sodium PATHS /opt/homebrew/lib /usr/local/lib)
  if(SODIUM_LIB)
    target_link_libraries(nexkeyctl PRIVATE "${SODIUM_LIB}")
  endif()
  target_link_libraries(nexkeyctl PRIVATE
    "-framework CoreFoundation"
    "-framework IOKit"
    "-framework Foundation"
    "-framework Security"
  )
endif()

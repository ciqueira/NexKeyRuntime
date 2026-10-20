cmake_minimum_required(VERSION 3.20)

project(notices_example LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if(NOT TARGET NexKeyRuntime::NexKeyRuntime)
  # 1. Look for NexKeyRuntime CMake package
  find_package(NexKeyRuntime CONFIG QUIET)

  # 2. Fallback: Search local paths if package was not found via CMAKE_PREFIX_PATH
  if(NOT NexKeyRuntime_FOUND)
    find_library(NEXKEYRUNTIME_LIB NAMES nexkeyruntime libnexkeyruntime
      PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../build"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../build"
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
    message(FATAL_ERROR "NexKeyRuntime library not found.")
  endif()
endif()

add_executable(notices-example main.c)

target_include_directories(notices-example PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(notices-example PRIVATE NexKeyRuntime::NexKeyRuntime)

if(APPLE)
  find_library(SODIUM_LIB NAMES sodium PATHS /opt/homebrew/lib /usr/local/lib)
  if(SODIUM_LIB)
    target_link_libraries(notices-example PRIVATE "${SODIUM_LIB}")
  endif()
  target_link_libraries(notices-example PRIVATE
    "c++"
    "-framework CoreFoundation"
    "-framework IOKit"
    "-framework Foundation"
    "-framework Security"
  )
endif()

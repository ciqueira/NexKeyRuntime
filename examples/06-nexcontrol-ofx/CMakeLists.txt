cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()
project(NexControlSamplePlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Locate OpenFX SDK
if(NOT DEFINED OFX_ROOT)
  set(POSSIBLE_OFX_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/openfx"
  )
  foreach(p IN LISTS POSSIBLE_OFX_PATHS)
    if(EXISTS "${p}/include/ofxCore.h")
      set(OFX_ROOT "${p}")
      break()
    endif()
  endforeach()
endif()

if(OFX_ROOT AND EXISTS "${OFX_ROOT}/include/ofxCore.h")
  message(STATUS "Using OpenFX from: ${OFX_ROOT}")
  set(OFX_INCLUDE_DIRS
    "${OFX_ROOT}/include"
    "${OFX_ROOT}/Support/include"
  )
  set(OFX_SUPPORT_SOURCES
    "${OFX_ROOT}/Support/Library/ofxsCore.cpp"
    "${OFX_ROOT}/Support/Library/ofxsImageEffect.cpp"
    "${OFX_ROOT}/Support/Library/ofxsInteract.cpp"
    "${OFX_ROOT}/Support/Library/ofxsLog.cpp"
    "${OFX_ROOT}/Support/Library/ofxsMultiThread.cpp"
    "${OFX_ROOT}/Support/Library/ofxsParams.cpp"
    "${OFX_ROOT}/Support/Library/ofxsProperty.cpp"
    "${OFX_ROOT}/Support/Library/ofxsPropertyValidation.cpp"
  )
else()
  include(FetchContent)
  FetchContent_Declare(
    openfx
    GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/openfx.git
    GIT_TAG main
  )
  FetchContent_GetProperties(openfx)
  if(NOT openfx_POPULATED)
    FetchContent_Populate(openfx)
  endif()
  set(OFX_INCLUDE_DIRS
    "${openfx_SOURCE_DIR}/include"
    "${openfx_SOURCE_DIR}/Support/include"
  )
  set(OFX_SUPPORT_SOURCES
    "${openfx_SOURCE_DIR}/Support/Library/ofxsCore.cpp"
    "${openfx_SOURCE_DIR}/Support/Library/ofxsImageEffect.cpp"
    "${openfx_SOURCE_DIR}/Support/Library/ofxsInteract.cpp"
    "${openfx_SOURCE_DIR}/Support/Library/ofxsLog.cpp"
    "${openfx_SOURCE_DIR}/Support/Library/ofxsMultiThread.cpp"
    "${openfx_SOURCE_DIR}/Support/Library/ofxsParams.cpp"
    "${openfx_SOURCE_DIR}/Support/Library/ofxsProperty.cpp"
    "${openfx_SOURCE_DIR}/Support/Library/ofxsPropertyValidation.cpp"
  )
endif()

# 2. Locate NexControl client library
if(NOT TARGET NexKeyRuntime::nexcontrol AND NOT TARGET nexcontrol)
  find_package(NexKeyRuntime CONFIG QUIET)

  if(NOT TARGET NexKeyRuntime::nexcontrol)
    find_library(NEXCONTROL_LIB NAMES nexcontrol libnexcontrol
      PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../build/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../build"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../build"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../build/src/nexcontrol"
    )
    if(NEXCONTROL_LIB)
      add_library(NexKeyRuntime::nexcontrol STATIC IMPORTED GLOBAL)
      set_target_properties(NexKeyRuntime::nexcontrol PROPERTIES
        IMPORTED_LOCATION "${NEXCONTROL_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
      )
    endif()
  endif()
endif()

add_library(NexControlSamplePlugin MODULE
    NexControlSamplePlugin.cpp
    NexControlSamplePlugin.h
    ${OFX_SUPPORT_SOURCES}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    target_compile_options(NexControlSamplePlugin PRIVATE -Wno-dynamic-exception-spec)
endif()

target_include_directories(NexControlSamplePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${OFX_INCLUDE_DIRS}
)

if(TARGET NexKeyRuntime::nexcontrol)
    target_link_libraries(NexControlSamplePlugin PRIVATE NexKeyRuntime::nexcontrol)
elseif(TARGET nexcontrol)
    target_link_libraries(NexControlSamplePlugin PRIVATE nexcontrol)
elseif(NEXCONTROL_LIB)
    target_link_libraries(NexControlSamplePlugin PRIVATE ${NEXCONTROL_LIB})
else()
    message(WARNING "NexControl client library not found. Set CMAKE_PREFIX_PATH or place libnexcontrol in ../../lib.")
endif()

if(APPLE)
    target_link_libraries(NexControlSamplePlugin PRIVATE "-framework AppKit" "-framework Foundation")
endif()

set_target_properties(NexControlSamplePlugin PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
)

if(APPLE)
    set_target_properties(NexControlSamplePlugin PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "ofx.bundle"
    )
endif()

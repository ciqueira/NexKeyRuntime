cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(NexControlSamplePlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Locate OpenFX SDK
if(NOT DEFINED OFX_ROOT)
  set(POSSIBLE_OFX_PATHS
    "$ENV{OFX_ROOT}"
    "$ENV{OPENFX_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/openfx"
  )
  foreach(p IN LISTS POSSIBLE_OFX_PATHS)
    if(p AND EXISTS "${p}/include/ofxCore.h")
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
    GIT_TAG OFX_Release_1.5.1
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
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/NexKeyRuntimeSetup.cmake")
nexkey_find_nexcontrol()

if(NOT TARGET NexKeyRuntime::nexcontrol AND NOT TARGET nexcontrol)
  message(FATAL_ERROR
    "NexControl client library (libnexcontrol.a / nexcontrol.lib) not found.\n"
    "Place it in ${NEXKEY_REPO_ROOT}/lib/ or pass -DCMAKE_PREFIX_PATH=<sdk_dir> or -DNEXCONTROL_LIB=<path>.")
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
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  ${OFX_INCLUDE_DIRS}
)

if(TARGET NexKeyRuntime::nexcontrol)
  target_link_libraries(NexControlSamplePlugin PRIVATE NexKeyRuntime::nexcontrol)
else()
  target_link_libraries(NexControlSamplePlugin PRIVATE nexcontrol)
endif()

if(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set_target_properties(NexControlSamplePlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
  )
endif()

set_target_properties(NexControlSamplePlugin PROPERTIES
  PREFIX ""
  SUFFIX ".ofx"
)

if(APPLE)
  set_target_properties(NexControlSamplePlugin PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "ofx.bundle"
    OUTPUT_NAME "NexControlSamplePlugin"
    SUFFIX ""
    LINK_FLAGS "-Wl,-exported_symbol,_OfxGetPlugin -Wl,-exported_symbol,_OfxGetNumberOfPlugins"
  )

  # DaVinci Resolve / OpenFX hosts on macOS scan Contents/MacOS/*.ofx.
  # CMake ignores SUFFIX on BUNDLE targets, so expose the .ofx symlink.
  add_custom_command(TARGET NexControlSamplePlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
      NexControlSamplePlugin
      "$<TARGET_FILE_DIR:NexControlSamplePlugin>/NexControlSamplePlugin.ofx"
    COMMENT "Creating Contents/MacOS/NexControlSamplePlugin.ofx symlink"
  )
elseif(WIN32)
  target_compile_definitions(NexControlSamplePlugin PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

  # Standard Windows OFX bundle directory structure:
  # NexControlSamplePlugin.ofx.bundle/Contents/Win64/NexControlSamplePlugin.ofx
  add_custom_command(TARGET NexControlSamplePlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:NexControlSamplePlugin>/NexControlSamplePlugin.ofx.bundle/Contents/Win64"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:NexControlSamplePlugin>"
      "$<TARGET_FILE_DIR:NexControlSamplePlugin>/NexControlSamplePlugin.ofx.bundle/Contents/Win64/NexControlSamplePlugin.ofx"
    COMMENT "Packaging NexControlSamplePlugin.ofx.bundle (Win64)"
  )
endif()

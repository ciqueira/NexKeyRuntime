cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(WIN32 AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
    "Supported build configurations (matches published Release /MT SDK library)")
endif()

project(NexKeySandboxPlugin LANGUAGES CXX)

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
  message(STATUS "Using local OpenFX from: ${OFX_ROOT}")
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

# 2. Locate NexKeyRuntime (required) and NexControl (optional)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/NexKeyRuntimeSetup.cmake")
nexkey_setup_runtime()
nexkey_find_nexcontrol()

# 3. Create OFX Plugin Target
add_library(NexKeySandboxPlugin MODULE
  src/CleanReferencePlugin.cpp
  ${OFX_SUPPORT_SOURCES}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
  target_compile_options(NexKeySandboxPlugin PRIVATE -Wno-dynamic-exception-spec)
endif()

target_compile_definitions(NexKeySandboxPlugin PRIVATE
  MC_NEXKEY_ENABLED=1
  MC_NEXKEY_ENFORCE=1
  MC_NEXKEY_TENANT="mcnexus-sandbox"
  MC_NEXKEY_VARIANT="default"
  MC_NEXKEY_BASE_URL="https://sdk-sandbox.mcnexus.app"
  MC_NEXKEY_NOTICE_CHANNEL=1
  PLUGIN_VERSION="1.0.0"
)

target_include_directories(NexKeySandboxPlugin PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  ${OFX_INCLUDE_DIRS}
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

target_link_libraries(NexKeySandboxPlugin PRIVATE NexKeyRuntime::NexKeyRuntime)

if(TARGET NexKeyRuntime::nexcontrol)
  target_compile_definitions(NexKeySandboxPlugin PRIVATE MC_NEXCONTROL_ENABLED=1)
  target_link_libraries(NexKeySandboxPlugin PRIVATE NexKeyRuntime::nexcontrol)
elseif(TARGET nexcontrol)
  target_compile_definitions(NexKeySandboxPlugin PRIVATE MC_NEXCONTROL_ENABLED=1)
  target_link_libraries(NexKeySandboxPlugin PRIVATE nexcontrol)
else()
  message(STATUS
    "NexKeySandboxPlugin: building without optional NexControl MIDI support (libnexcontrol not found)")
endif()

if(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set_target_properties(NexKeySandboxPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
  )
endif()

if(APPLE)
  target_link_libraries(NexKeySandboxPlugin PRIVATE "-framework AppKit")

  # Standard macOS OFX bundle directory structure:
  # NexKeySandboxPlugin.ofx.bundle/Contents/MacOS/NexKeySandboxPlugin.ofx
  set_target_properties(NexKeySandboxPlugin PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "ofx.bundle"
    OUTPUT_NAME "NexKeySandboxPlugin"
    SUFFIX ""
    LINK_FLAGS "-Wl,-exported_symbol,_OfxGetPlugin -Wl,-exported_symbol,_OfxGetNumberOfPlugins"
  )

  add_custom_command(TARGET NexKeySandboxPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
      NexKeySandboxPlugin
      $<TARGET_FILE_DIR:NexKeySandboxPlugin>/NexKeySandboxPlugin.ofx
  )
elseif(WIN32)
  target_compile_definitions(NexKeySandboxPlugin PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
  target_link_libraries(NexKeySandboxPlugin PRIVATE shell32)

  set_target_properties(NexKeySandboxPlugin PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    OUTPUT_NAME "NexKeySandboxPlugin"
  )

  # Standard Windows OFX bundle directory structure:
  # NexKeySandboxPlugin.ofx.bundle/Contents/Win64/NexKeySandboxPlugin.ofx
  add_custom_command(TARGET NexKeySandboxPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
      "$<TARGET_FILE_DIR:NexKeySandboxPlugin>/NexKeySandboxPlugin.ofx.bundle/Contents/Win64"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:NexKeySandboxPlugin>"
      "$<TARGET_FILE_DIR:NexKeySandboxPlugin>/NexKeySandboxPlugin.ofx.bundle/Contents/Win64/NexKeySandboxPlugin.ofx"
    COMMENT "Packaging NexKeySandboxPlugin.ofx.bundle (Win64)"
  )
else()
  set_target_properties(NexKeySandboxPlugin PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    OUTPUT_NAME "NexKeySandboxPlugin"
  )
endif()

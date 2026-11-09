cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

project(NexKeySandboxPlugin LANGUAGES CXX)

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

# 2. Locate NexKeyRuntime
if(NOT TARGET NexKeyRuntime::NexKeyRuntime)
  find_package(NexKeyRuntime CONFIG QUIET)

  if(NOT NexKeyRuntime_FOUND)
    find_library(NEXKEYRUNTIME_LIB NAMES libnexkeyruntime.a nexkeyruntime libnexkeyruntime
      PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../build"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../build"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../lib"
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

# 2b. Locate NexControl Client Library
if(NOT TARGET NexKeyRuntime::nexcontrol AND NOT TARGET nexcontrol)
  find_library(NEXCONTROL_LIB NAMES libnexcontrol.a nexcontrol libnexcontrol
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
  src
  ${OFX_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(NexKeySandboxPlugin PRIVATE NexKeyRuntime::NexKeyRuntime)
if(TARGET NexKeyRuntime::nexcontrol)
  target_link_libraries(NexKeySandboxPlugin PRIVATE NexKeyRuntime::nexcontrol)
endif()

if(APPLE)
  find_library(SODIUM_LIB NAMES sodium PATHS /opt/homebrew/lib /usr/local/lib)
  if(SODIUM_LIB)
    target_link_libraries(NexKeySandboxPlugin PRIVATE "${SODIUM_LIB}")
  endif()

  target_link_libraries(NexKeySandboxPlugin PRIVATE
    "-framework CoreFoundation"
    "-framework IOKit"
    "-framework Foundation"
    "-framework Security"
    "-framework AppKit"
  )

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
  set_target_properties(NexKeySandboxPlugin PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    OUTPUT_NAME "NexKeySandboxPlugin"
  )
else()
  set_target_properties(NexKeySandboxPlugin PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    OUTPUT_NAME "NexKeySandboxPlugin"
  )
endif()


# Shared CMake discovery and dependency setup for NexKeyRuntime consumers.
#
# Resolves NexKeyRuntime::NexKeyRuntime and optional NexKeyRuntime::nexcontrol
# from either an explicit CMAKE_PREFIX_PATH / NexKeyRuntime_DIR or the local
# repository layout (<repo>/lib/ and <repo>/include/).

include_guard(GLOBAL)

get_filename_component(NEXKEY_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

function(nexkey_setup_runtime)
  if(TARGET NexKeyRuntime::NexKeyRuntime)
    return()
  endif()

  find_package(Threads REQUIRED)

  # Only query an installed CMake package when the caller explicitly requested one.
  # Running find_package implicitly against <repo>/lib/cmake/NexKeyRuntime on
  # Windows fails inside NexKeyRuntimeTargets.cmake if only lib/ was copied
  # without bin/nexkeyruntime.dll.
  if(NOT "${CMAKE_PREFIX_PATH}" STREQUAL "" OR
     (DEFINED NexKeyRuntime_DIR AND NOT "${NexKeyRuntime_DIR}" STREQUAL ""))
    find_package(NexKeyRuntime CONFIG QUIET)
  endif()

  if(NOT TARGET NexKeyRuntime::NexKeyRuntime)
    set(_saved_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})

    find_library(NEXKEYRUNTIME_LIB
      NAMES nexkeyruntime libnexkeyruntime
      PATHS
        "${NEXKEY_REPO_ROOT}/lib"
        "${NEXKEY_REPO_ROOT}/build"
        "${NEXKEY_REPO_ROOT}/../build"
      NO_DEFAULT_PATH
    )

    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_saved_suffixes})

    if(NOT NEXKEYRUNTIME_LIB)
      find_library(NEXKEYRUNTIME_LIB NAMES nexkeyruntime libnexkeyruntime)
    endif()

    find_path(NEXKEYRUNTIME_INCLUDE_DIR
      NAMES nexkeyruntime/nexkeyruntime.h
      PATHS
        "${NEXKEY_REPO_ROOT}/include"
        "${NEXKEY_REPO_ROOT}/../include"
      NO_DEFAULT_PATH
    )

    if(NEXKEYRUNTIME_LIB AND NEXKEYRUNTIME_INCLUDE_DIR)
      add_library(NexKeyRuntime::NexKeyRuntime STATIC IMPORTED GLOBAL)
      set_target_properties(NexKeyRuntime::NexKeyRuntime PROPERTIES
        IMPORTED_LOCATION "${NEXKEYRUNTIME_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${NEXKEYRUNTIME_INCLUDE_DIR}"
        INTERFACE_COMPILE_FEATURES "cxx_std_17"
      )
      set_property(TARGET NexKeyRuntime::NexKeyRuntime APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES Threads::Threads
      )
      if(APPLE)
        set_property(TARGET NexKeyRuntime::NexKeyRuntime APPEND PROPERTY
          INTERFACE_LINK_LIBRARIES
            "-framework CoreFoundation"
            "-framework IOKit"
            "-framework Foundation"
            "-framework Security"
        )
      elseif(WIN32)
        set_property(TARGET NexKeyRuntime::NexKeyRuntime APPEND PROPERTY
          INTERFACE_LINK_LIBRARIES
            bcrypt
            winhttp
            advapi32
            crypt32
            user32
        )
      endif()
      message(STATUS "NexKeyRuntime: using static library ${NEXKEYRUNTIME_LIB}")
    else()
      message(FATAL_ERROR
        "NexKeyRuntime static library not found.\n"
        "Download the precompiled release for your platform from:\n"
        "  https://github.com/ciqueira/NexKeyRuntime/releases\n"
        "and place lib/libnexkeyruntime.a (macOS) or lib/nexkeyruntime.lib (Windows)\n"
        "inside: ${NEXKEY_REPO_ROOT}/lib/\n"
        "Alternatively, pass -DCMAKE_PREFIX_PATH=<sdk_dir> or -DNEXKEYRUNTIME_LIB=<path>.")
    endif()
  endif()
endfunction()

function(nexkey_find_nexcontrol)
  if(TARGET NexKeyRuntime::nexcontrol OR TARGET nexcontrol)
    return()
  endif()

  set(_saved_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})

  find_library(NEXCONTROL_LIB
    NAMES nexcontrol libnexcontrol
    PATHS
      "${NEXKEY_REPO_ROOT}/lib"
      "${NEXKEY_REPO_ROOT}/build/lib"
      "${NEXKEY_REPO_ROOT}/build"
      "${NEXKEY_REPO_ROOT}/../build/src/nexcontrol"
      "${NEXKEY_REPO_ROOT}/../build"
    NO_DEFAULT_PATH
  )

  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_saved_suffixes})

  if(NEXCONTROL_LIB)
    add_library(NexKeyRuntime::nexcontrol STATIC IMPORTED GLOBAL)
    set_target_properties(NexKeyRuntime::nexcontrol PROPERTIES
      IMPORTED_LOCATION "${NEXCONTROL_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${NEXKEY_REPO_ROOT}/include"
      INTERFACE_COMPILE_FEATURES "cxx_std_17"
    )
    if(APPLE)
      set_property(TARGET NexKeyRuntime::nexcontrol APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "-framework AppKit" "-framework Foundation"
      )
    elseif(WIN32)
      set_property(TARGET NexKeyRuntime::nexcontrol APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES user32
      )
    endif()
    message(STATUS "NexKeyRuntime: using NexControl library ${NEXCONTROL_LIB}")
  endif()
endfunction()

function(nexkey_find_packager)
  if(TARGET NexKeyRuntime::Packager OR TARGET nexkeypackager)
    return()
  endif()

  set(_saved_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})

  find_library(NEXKEYPACKAGER_LIB
    NAMES nexkeypackager libnexkeypackager
    PATHS
      "${NEXKEY_REPO_ROOT}/lib"
      "${NEXKEY_REPO_ROOT}/build/lib"
      "${NEXKEY_REPO_ROOT}/build"
      "${NEXKEY_REPO_ROOT}/../build"
    NO_DEFAULT_PATH
  )

  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_saved_suffixes})

  if(NEXKEYPACKAGER_LIB)
    add_library(NexKeyRuntime::Packager STATIC IMPORTED GLOBAL)
    set_target_properties(NexKeyRuntime::Packager PROPERTIES
      IMPORTED_LOCATION "${NEXKEYPACKAGER_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${NEXKEY_REPO_ROOT}/include"
      INTERFACE_COMPILE_FEATURES "cxx_std_17"
    )
    if(TARGET NexKeyRuntime::NexKeyRuntime)
      set_property(TARGET NexKeyRuntime::Packager APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES NexKeyRuntime::NexKeyRuntime
      )
    endif()
    message(STATUS "NexKeyRuntime: using Packager library ${NEXKEYPACKAGER_LIB}")
  endif()
endfunction()

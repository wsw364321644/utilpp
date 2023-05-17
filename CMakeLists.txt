cmake_minimum_required(VERSION 3.23)
include(FetchContent)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil" "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil/Modules" "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil/Projects")
list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE INTERNAL "CMAKE_MODULE_PATH")
include(ProjectDefinition)
include(ProjectImport)

set(VERSION "0.0.1")
project(utilpp VERSION ${VERSION})
enable_language(CXX ASM)

option(UTILPP_STATIC_CRT "SUTIL STATIC CRT Build ." OFF)
option(UTILPP_ENABLE_LIBUV "ENABLE UTIL DEPEND ON UV." OFF)
option(UTILPP_ENABLE_LOG "ENABLE UTIL LOG." OFF)

if(UTILPP_STATIC_CRT)
  if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  endif()

  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(UTILPP_ENABLE_LIBUV)
  FetchContent_Declare(
    libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG 0c1fa696aa502eb749c2c4735005f41ba00a27b8 # v1.44.2
    GIT_SUBMODULES_RECURSE true

    # FIND_PACKAGE_ARGS NAMES libuv
  )
  FetchContent_MakeAvailable(libuv)
endif()

add_subdirectory(deps/fmt)
set(SPDLOG_WCHAR_SUPPORT ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO ON CACHE BOOL "" FORCE)
add_subdirectory(deps/spdlog)

add_subdirectory(src/sutils)
add_subdirectory(src/simple_logger)
if(WIN32)
  add_subdirectory(src/windows_util)
endif()
if(UTILPP_ENABLE_LIBUV)
  add_subdirectory(src/ipc_util)
endif()
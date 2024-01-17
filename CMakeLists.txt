cmake_minimum_required(VERSION 3.24)
include(FetchContent)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil" "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil/Modules" "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil/Projects")
# list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE INTERNAL "CMAKE_MODULE_PATH")
# include(ProjectDefinition)
# include(ProjectImport)
set(VERSION "0.0.1")
project(utilpp VERSION ${VERSION})
enable_language(CXX ASM)

option(UTILPP_STATIC_CRT "SUTIL STATIC CRT Build ." OFF)
option(UTILPP_WITH_STATIC_LIBUV "USE UV_a." OFF)
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

FetchContent_Declare(
  fmt
  GIT_REPOSITORY git@ssh.github.com:fmtlib/fmt.git
  GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281 #10.2.1
)
FetchContent_MakeAvailable(fmt)

set(SPDLOG_WCHAR_SUPPORT ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO ON CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY git@ssh.github.com:gabime/spdlog.git
  GIT_TAG 7c02e204c92545f869e2f04edaab1f19fe8b19fd #1.13.0
)
FetchContent_MakeAvailable(spdlog)

add_subdirectory(src/sutils)
add_subdirectory(src/simple_logger)
add_subdirectory(src/rapidjson)
add_subdirectory(src/better-enums)

if(WIN32)
  add_subdirectory(src/windows_util)
endif()

find_package(LIBUV)
if(LIBUV_FOUND)
  add_subdirectory(src/ipc_util)
endif()

find_package(CURL)
if(CURL_FOUND)
  add_subdirectory(src/net_util)
endif()

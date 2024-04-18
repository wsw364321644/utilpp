cmake_minimum_required(VERSION 3.24)
include(FetchContent)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil" "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil/Modules" "${CMAKE_CURRENT_SOURCE_DIR}/deps/OCmakeUtil/Projects")
# list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE INTERNAL "CMAKE_MODULE_PATH")
# include(ProjectDefinition)
# include(ProjectImport)
set(VERSION "0.0.1")
project(utilpp VERSION ${VERSION})
enable_language(C CXX ASM)

option(UTILPP_STATIC_CRT "SUTIL STATIC CRT Build ." OFF)
option(UTILPP_WITH_STATIC_LIBUV "USE UV_a." OFF)
#option(UTILPP_ENABLE_LOG "ENABLE UTIL LOG." OFF)

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

#FetchContent_Declare(
#  fmt
#  GIT_REPOSITORY git@ssh.github.com:fmtlib/fmt.git
#  GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281 #10.2.1
#)
#FetchContent_MakeAvailable(fmt)

set(SPDLOG_WCHAR_SUPPORT ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY git@ssh.github.com:gabime/spdlog.git
  GIT_TAG 7c02e204c92545f869e2f04edaab1f19fe8b19fd #1.13.0
)
FetchContent_MakeAvailable(spdlog)

set(JSON_Install TRUE)
FetchContent_Declare(
  nlohmann-json
  GIT_REPOSITORY git@ssh.github.com:nlohmann/json.git
  GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 # 3.11.3
)
FetchContent_MakeAvailable(nlohmann-json)

set(JSON_VALIDATOR_INSTALL TRUE)
FetchContent_Declare(
  json-schema-validator
  GIT_REPOSITORY git@ssh.github.com:pboettch/json-schema-validator.git
  GIT_TAG 349cba9f7e3cb423bbc1811bdd9f6770f520b468 # 2.3.0
)
FetchContent_MakeAvailable(json-schema-validator)


add_subdirectory(src/simple_util)
add_subdirectory(src/simple_utilpp)
add_subdirectory(src/simple_logger)
add_subdirectory(src/rapidjson)
add_subdirectory(src/better-enums)
add_subdirectory(src/task_manager)
add_subdirectory(src/rpc_parser)

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

find_package(SDL2)
if(SDL2_FOUND)
  add_subdirectory(src/sdl_helper)
endif()
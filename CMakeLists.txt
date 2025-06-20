cmake_minimum_required(VERSION 3.24)
include(FetchContent)

set(VERSION "0.0.1")
project(utilpp VERSION ${VERSION})
include(GNUInstallDirs)
enable_language(C CXX ASM)

option(UTILPP_STATIC_CRT "determine build STATIC CRT Build ." OFF)
option(UTILPP_GIT_SSH "git via ssh" OFF)
option(UTILPP_ENABLE_IPC "enable ipc_util" OFF)
option(UTILPP_WITH_STATIC_LIBUV "ipc_util use libuv::uv_a" OFF)
option(UTILPP_ENABLE_NET "enable net_util" OFF)
option(UTILPP_WITH_STATIC_LIBCURL "net_util use static libcurl" OFF)
option(UTILPP_ENABLE_STEAM "enable steam_util" OFF)
option(UTILPP_ENABLE_WEBSOCKET "enable websocket_util" OFF)

# option(UTILPP_ENABLE_LOG "ENABLE UTIL LOG." OFF)
if(PROJECT_IS_TOP_LEVEL)
  if(UTILPP_STATIC_CRT)
    if(MSVC)
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
  endif()
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

FetchContent_Declare(
  OCmakeUtil
  GIT_REPOSITORY https://github.com/wsw364321644/OCmakeUtil.git
  GIT_TAG f2475b3370b751f8d3fa914e191daabc7a764e32
)

set(SPDLOG_WCHAR_SUPPORT ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_PIC ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG 8e5613379f5140fefb0b60412fbf1f5406e7c7f8 # 1.15.0
)

set(JSON_Install TRUE)
FetchContent_Declare(
  nlohmann-json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 # 3.11.3
)

set(JSON_VALIDATOR_INSTALL TRUE)
set(nlohmann_json_schema_validator_SHARED_LIBS OFF)
FetchContent_Declare(
  json-schema-validator
  GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
  GIT_TAG 349cba9f7e3cb423bbc1811bdd9f6770f520b468 # 2.3.0
)

# FetchContent_Declare(
# gcem
# GIT_REPOSITORY git@github.com:kthohr/gcem.git
# GIT_TAG 012ae73c6d0a2cb09ffe86475f5c6fba3926e200 # 1.18.0
# )
set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TF_BUILD_PROFILER ON CACHE BOOL "" FORCE)
set(TF_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  taskflow
  GIT_REPOSITORY https://github.com/taskflow/taskflow.git
  GIT_TAG 12f8bd4e970ab27fd3dee3bffa24b5b48b54ba39 # 3.6.0
)

FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG 24b78782bd6ca5a5853ef46917708806112dc142
)

FetchContent_MakeAvailable(OCmakeUtil spdlog nlohmann-json json-schema-validator taskflow concurrentqueue)
include(ResourceDownload)
include(ProjectImport)

set_target_properties(nlohmann_json_schema_validator PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_subdirectory(src/simple_util)
add_subdirectory(src/simple_utilpp_header)
add_subdirectory(src/simple_utilpp)
add_subdirectory(src/simple_logger)
add_subdirectory(src/task_manager)
add_subdirectory(src/rpc_parser)

if(WIN32)
  add_subdirectory(src/windows_util)
endif()

if(UTILPP_STATIC_CRT)
  set(STATIC_CRT STATIC_CRT)
else()
  unset(STATIC_CRT)
endif()

if(UTILPP_GIT_SSH)
  set(SSH SSH)
else()
  unset(SSH)
endif()

if(UTILPP_ENABLE_IPC)
  ImportProject(libuv ${STATIC_CRT} ${SSH} FIND TAG be6b81a352d17513c95be153afcb3148f1a451cd) # 1.47.0
  add_subdirectory(src/ipc_util)
endif()

if(UTILPP_ENABLE_NET)
  if(UTILPP_WITH_STATIC_LIBCURL)
    set(STATIC STATIC)
  else()
    unset(STATIC)
  endif()

  ImportProject(CURL ${STATIC_CRT} ${STATIC} ${SSH} FIND TAG 45d2ff6f8521524cbfa22e8be6a71a55578ccc4c) # 8.4.0
  add_subdirectory(src/net_util)
endif()

if(UTILPP_ENABLE_WEBSOCKET)
  add_subdirectory(src/websocket_util)
endif()

if(UTILPP_ENABLE_STEAM)
  ImportTarget(ValveFileVDF ${SSH} TAG 277a5836e76b59d958847beb169ae1ac04238d19) # v1.1.1
  add_subdirectory(src/steam_util)
endif()

if(UTILPP_ENABLE_STEAM AND UTILPP_ENABLE_WEBSOCKET AND UTILPP_ENABLE_NET)
  add_subdirectory(src/steam_protobufs)
  add_subdirectory(src/steam_msg)
endif()
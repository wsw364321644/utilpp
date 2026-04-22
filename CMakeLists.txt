cmake_minimum_required(VERSION 3.24)
include(FetchContent)

set(VERSION "0.0.1")
project(UTILPP VERSION ${VERSION})
include(GNUInstallDirs)
enable_language(C CXX ASM)
set(CMAKE_DEBUG_POSTFIX d)
option(UTILPP_STATIC_CRT "determine build STATIC CRT Build ." OFF)
option(UTILPP_GIT_SSH "git via ssh" OFF)
option(UTILPP_ENABLE_IPC "enable ipc_util" OFF)
option(UTILPP_WITH_STATIC_LIBUV "ipc_util use libuv::uv_a" OFF)
option(UTILPP_ENABLE_NET "enable net_util" OFF)
option(UTILPP_WITH_STATIC_LIBCURL "net_util use static libcurl" OFF)
option(UTILPP_ENABLE_STEAM "enable steam_util" OFF)
option(UTILPP_ENABLE_WEBSOCKET "enable websocket_util" OFF)
option(UTILPP_ENABLE_ZLIB "enable module depend on zlib" OFF)
option(UTILPP_ENABLE_CPUID "enable system info cpuid feature" OFF)
option(UTILPP_DISABLE_INSTALL "disable installation" OFF)

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
  GIT_TAG main
)
FetchContent_MakeAvailable(OCmakeUtil)
include(ResourceDownload)
include(ProjectImport)
include(ProgramImport)
include(TargetImport)

if(UTILPP_ENABLE_ZLIB)
  ImportProject(ZLIB STATIC ${STATIC_CRT} ${SSH} FIND TAG v1.3.2)
endif()

if(UTILPP_ENABLE_CPUID)
  ImportProject(cpuid ${STATIC_CRT} STATIC ${SSH} TAG v0.8.0)
endif()

if(UTILPP_ENABLE_NET)
  ImportProject(CapnProto ${STATIC_CRT} STATIC ${SSH} TAG v1.3.0)
endif()

ImportProject(RapidJSON ${SSH} TAG origin/master)
ImportProject(simdjson ${STATIC_CRT} ${SSH} TAG v4.2.4)
ImportProject(spdlog ${STATIC_CRT} STATIC ${SSH} TAG v1.17.0)
ImportProject(concurrentqueue ${SSH} TAG master)
ImportProject(magic_enum ${SSH} TAG master)

# FetchContent_Declare(
# gcem
# GIT_REPOSITORY git@github.com:kthohr/gcem.git
# GIT_TAG 012ae73c6d0a2cb09ffe86475f5c6fba3926e200 # 1.18.0
# )
set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TF_BUILD_PROFILER OFF CACHE BOOL "" FORCE)
set(TF_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  taskflow
  GIT_REPOSITORY https://github.com/taskflow/taskflow.git
  GIT_TAG v4.0.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(taskflow)

FetchContent_Declare(
  ctre
  GIT_REPOSITORY https://github.com/hanickadot/compile-time-regular-expressions.git
  GIT_TAG v3.10.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ctre)

add_subdirectory(src/simple_util)
add_subdirectory(src/simple_utilpp_header)
add_subdirectory(src/simple_utilpp)
add_subdirectory(src/simple_logger)
add_subdirectory(src/task_manager)
add_subdirectory(src/rpc_parser)
add_subdirectory(src/crypto_lib_helper)
add_subdirectory(src/system_info_util)

if(UTILPP_ENABLE_ZLIB)
  add_subdirectory(src/simple_hash)
endif()

if(WIN32)
  add_subdirectory(src/windows_util)
endif()

if(UTILPP_ENABLE_IPC)
  ImportProject(libuv ${STATIC_CRT} ${SSH} TAG v1.51.0)
  add_subdirectory(src/ipc_util)
endif()

if(UTILPP_ENABLE_NET)
  if(UTILPP_WITH_STATIC_LIBCURL)
    set(STATIC STATIC)
  else()
    unset(STATIC)
  endif()

  ImportProject(CURL ${STATIC_CRT} ${STATIC} ${SSH} TAG curl-8_18_0)
  add_subdirectory(src/net_util)
endif()

if(UTILPP_ENABLE_WEBSOCKET)
  ImportProject(libwebsockets ${STATIC_CRT} SSH TAG v4.5.2)
  add_subdirectory(src/websocket_util)
endif()

if(UTILPP_ENABLE_STEAM)
  ImportProject(ValveFileVDF TAG v1.1.1)
  ImportProject(absl ${STATIC_CRT} ${SSH} TAG 20260107.1)
  ImportProject(Protobuf ${STATIC_CRT} ${SSH} TAG v34.0)
  ImportProject(re2 ${STATIC_CRT} ${SSH} TAG 2025-11-05)
  ImportProgram(SteamLanguageParser ${SSH} TAG origin/master) # path too long
  ImportProject(steamdatapp ${STATIC_CRT} ${SSH} TAG main)

  add_subdirectory(src/steam_util)
endif()

if(UTILPP_ENABLE_STEAM AND UTILPP_ENABLE_WEBSOCKET AND UTILPP_ENABLE_NET)
  set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.1
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(jwt-cpp)
  ImportProject(SQLite3 ${STATIC_CRT} URL https://www.sqlite.org/2025/sqlite-amalgamation-3500200.zip)
  ImportProject(sqlpp23 ${SSH} TAG 0.66)
  ImportProgram(Python3 URL https://mirrors.aliyun.com/python-release/windows/python-3.13.5-embed-amd64.zip)

  add_subdirectory(src/steam_msg)
endif()

if(NOT UTILPP_DISABLE_INSTALL)
  ExportFromInstall(${PROJECT_NAME})
endif()
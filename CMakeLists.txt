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
option(UTILPP_ENABLE_CPUID "enable system info cpuid feature" OFF)

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
  GIT_TAG 44a2f675ef4468380a8d7609fb7b0c63ec914893
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
  GIT_TAG v1.15.3
  GIT_SHALLOW TRUE
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
  GIT_TAG v3.10.0
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG origin/master
  GIT_SHALLOW TRUE
)

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "")
set(SIMDJSON_BUILD_STATIC_LIB ON CACHE INTERNAL "")
FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG v3.13.0
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(OCmakeUtil spdlog taskflow concurrentqueue simdjson)
include(ResourceDownload)
include(ProjectImport)
include(ProgramImport)
include(TargetImport)

ImportTarget(RapidJSON ${SSH} TAG origin/master)

if(UTILPP_ENABLE_CPUID)
  ImportProject(cpuid ${STATIC_CRT} STATIC ${SSH} TAG v0.8.0)
endif()

add_subdirectory(src/simple_util)
add_subdirectory(src/simple_utilpp_header)
add_subdirectory(src/simple_utilpp)
add_subdirectory(src/simple_logger)
add_subdirectory(src/task_manager)
add_subdirectory(src/rpc_parser)
add_subdirectory(src/crypto_lib_helper)
add_subdirectory(src/system_info_util)

if(WIN32)
  add_subdirectory(src/windows_util)
endif()

if(UTILPP_ENABLE_IPC)
  ImportProject(libuv ${STATIC_CRT} ${SSH} TAG be6b81a352d17513c95be153afcb3148f1a451cd) # 1.47.0
  add_subdirectory(src/ipc_util)
endif()

if(UTILPP_ENABLE_NET)
  if(UTILPP_WITH_STATIC_LIBCURL)
    set(STATIC STATIC)
  else()
    unset(STATIC)
  endif()

  ImportProject(CURL ${STATIC_CRT} ${STATIC} ${SSH} TAG 45d2ff6f8521524cbfa22e8be6a71a55578ccc4c) # 8.4.0
  add_subdirectory(src/net_util)
endif()

if(UTILPP_ENABLE_WEBSOCKET)
  add_subdirectory(src/websocket_util)
endif()

if(UTILPP_ENABLE_STEAM)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/steam_msg/generated/SteamMsg)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/steam_util/generated)

  if(NOT DEFINED GEN_STEAM_LANGUAGE OR GEN_STEAM_LANGUAGE)
    set(WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} ${CMAKE_GENERATOR_ARGV} ${CMAKE_CURRENT_SOURCE_DIR}/SteamMsgGenerator
      -B BuildSteamMsgGenerator
      WORKING_DIRECTORY ${WORKING_DIRECTORY}
      RESULT_VARIABLE result
    )

    if(NOT ${result} EQUAL 0)
      message(FATAL_ERROR "Failed to generate SteamMsgGenerator")
    endif()

    set(WORKING_DIRECTORY ${WORKING_DIRECTORY}/BuildSteamMsgGenerator)
    execute_process(COMMAND ${CMAKE_COMMAND} --build . --target install --config Release
      WORKING_DIRECTORY ${WORKING_DIRECTORY}
      RESULT_VARIABLE result
    )

    if(NOT ${result} EQUAL 0)
      message(FATAL_ERROR "Failed to build SteamMsgGenerator")
    endif()

    ResourceDownload(
      SteamKit
      GIT_REPOSITORY https://github.com/SteamRE/SteamKit.git
      GIT_TAG 47fa00b0a1886dfdb24a59c24ebd4d4555ebcb2d
    )

    execute_process(COMMAND ${WORKING_DIRECTORY}/rundir/bin/SteamLanguageParser.exe
      ${SteamKit_DIR}/Resources/SteamLanguage/steammsg.steamd
      -o ${CMAKE_CURRENT_SOURCE_DIR}/src/steam_msg/generated/SteamMsg
      -e ${CMAKE_CURRENT_SOURCE_DIR}/src/steam_util/generated/SteamLanguage.h
      WORKING_DIRECTORY ${SteamKit_DIR}/Resources/SteamLanguage
      RESULT_VARIABLE result
    )

    if(NOT ${result} EQUAL 0)
      message(FATAL_ERROR "Failed to gen steamlanguage")
    endif()

    set(GEN_STEAM_LANGUAGE FALSE CACHE BOOL "Generate Steam Language C++ files" FORCE)
  endif()

  ImportTarget(ValveFileVDF ${SSH} TAG 277a5836e76b59d958847beb169ae1ac04238d19) # v1.1.1
  add_subdirectory(src/steam_util)
endif()

if(UTILPP_ENABLE_STEAM AND UTILPP_ENABLE_WEBSOCKET AND UTILPP_ENABLE_NET)
  add_subdirectory(src/steam_protobufs)
  add_subdirectory(src/steam_msg)
endif()
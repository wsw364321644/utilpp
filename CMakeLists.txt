cmake_minimum_required(VERSION 3.24)
include(FetchContent)

set(VERSION "0.0.1")
project(utilpp VERSION ${VERSION})
enable_language(C CXX ASM)

option(STATIC_CRT "SUTIL STATIC CRT Build ." OFF)
option(UTILPP_WITH_STATIC_LIBUV "USE UV_a." OFF)

# option(UTILPP_ENABLE_LOG "ENABLE UTIL LOG." OFF)
if(PROJECT_IS_TOP_LEVEL)
  if(STATIC_CRT)
    if(MSVC)
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
      add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()

    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
  endif()
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

FetchContent_Declare(
  OCmakeUtil
  GIT_REPOSITORY git@github.com:wsw364321644/OCmakeUtil.git
  GIT_TAG 24b87522ba7b9a771f51f2030021dec72935d5c9
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
  GIT_REPOSITORY git@github.com:gabime/spdlog.git
  GIT_TAG 8e5613379f5140fefb0b60412fbf1f5406e7c7f8 # 1.15.0
)

set(JSON_Install TRUE)
FetchContent_Declare(
  nlohmann-json
  GIT_REPOSITORY git@github.com:nlohmann/json.git
  GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 # 3.11.3
)

set(JSON_VALIDATOR_INSTALL TRUE)
set(nlohmann_json_schema_validator_SHARED_LIBS OFF)
FetchContent_Declare(
  json-schema-validator
  GIT_REPOSITORY git@github.com:pboettch/json-schema-validator.git
  GIT_TAG 349cba9f7e3cb423bbc1811bdd9f6770f520b468 # 2.3.0
)

FetchContent_Declare(
  gcem
  GIT_REPOSITORY git@github.com:kthohr/gcem.git
  GIT_TAG 012ae73c6d0a2cb09ffe86475f5c6fba3926e200 # 1.18.0
)

set(TF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TF_BUILD_PROFILER ON CACHE BOOL "" FORCE)
set(TF_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  taskflow
  GIT_REPOSITORY git@github.com:taskflow/taskflow.git
  GIT_TAG 12f8bd4e970ab27fd3dee3bffa24b5b48b54ba39 # 3.6.0
)

FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY git@github.com:cameron314/concurrentqueue.git
  GIT_TAG 2f09da73d22a47dc8a89cdd4fc4c3bfae07f4284
)

FetchContent_MakeAvailable(OCmakeUtil spdlog nlohmann-json json-schema-validator taskflow gcem concurrentqueue)
set_target_properties(nlohmann_json_schema_validator PROPERTIES POSITION_INDEPENDENT_CODE ON)

# if(STATIC_CRT)
#   ImportProject(TBB STATIC_CRT SSH FIND TAG 0c0ff192a2304e114bc9e6557582dfba101360ff) # 2022.0.0
# else()
#   ImportProject(TBB SSH FIND TAG 0c0ff192a2304e114bc9e6557582dfba101360ff) # 2022.0.0
# endif()

add_subdirectory(src/simple_util)
add_subdirectory(src/simple_utilpp_header)
add_subdirectory(src/simple_utilpp)
add_subdirectory(src/simple_logger)
add_subdirectory(src/task_manager)
add_subdirectory(src/rpc_parser)

if(WIN32)
  add_subdirectory(src/windows_util)
endif()

find_package(libuv)

if(libuv_FOUND)
  add_subdirectory(src/ipc_util)
endif()

find_package(CURL)

if(CURL_FOUND)
  add_subdirectory(src/net_util)
endif()

add_subdirectory(src/steam_util)
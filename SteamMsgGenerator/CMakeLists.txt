cmake_minimum_required(VERSION 3.5...3.24)
include(FetchContent)

set(VERSION "0.0.1")
project(SteamMsgGenerator VERSION ${VERSION})
include(GNUInstallDirs)
enable_language(C CXX ASM)

if(PROJECT_IS_TOP_LEVEL)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  endif()

  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/rundir"
      CACHE STRING "Directory to install after building" FORCE)
  endif()
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY git@github.com:jarro2783/cxxopts.git
  GIT_TAG v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

set(UTILPP_GIT_SSH ON CACHE BOOL "" FORCE)
set(UTILPP_STATIC_CRT ON CACHE BOOL "" FORCE)
set(UTILPP_ENABLE_NET ON CACHE BOOL "" FORCE)
set(UTILPP_WITH_STATIC_LIBCURL ON CACHE BOOL "" FORCE)
add_subdirectory(.. utilpp)

ImportProject(absl ${STATIC_CRT} SSH FIND TAG 20260107.0)

FetchContent_Declare(
  re2
  GIT_REPOSITORY git@github.com:google/re2.git
  GIT_TAG 2025-11-05
)
FetchContent_MakeAvailable(re2)
add_subdirectory(src/SteamLanguageParser)

if(PROJECT_IS_TOP_LEVEL)
  if(WIN32)
    find_package(absl CONFIG REQUIRED)
    file(GLOB dll_list "${absl_DIR}/../../../bin/*.dll")
    install(
      FILES ${dll_list}
      DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
    )
  endif()

  install(TARGETS SteamLanguageParser
    EXPORT run-export
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()
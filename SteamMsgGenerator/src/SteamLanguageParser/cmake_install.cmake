# Install script for directory: C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/rundir")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/bin" TYPE EXECUTABLE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/bin/Debug/SteamLanguageParser.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/bin" TYPE EXECUTABLE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/bin/Release/SteamLanguageParser.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/bin" TYPE EXECUTABLE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/bin/MinSizeRel/SteamLanguageParser.exe")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/bin" TYPE EXECUTABLE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/bin/RelWithDebInfo/SteamLanguageParser.exe")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/include" TYPE FILE FILES
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/CodeGenerator.h"
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/LanguageParser.h"
      )
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/include" TYPE FILE FILES
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/CodeGenerator.h"
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/LanguageParser.h"
      )
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/include" TYPE FILE FILES
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/CodeGenerator.h"
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/LanguageParser.h"
      )
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/include" TYPE FILE FILES
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/CodeGenerator.h"
      "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/public/LanguageParser.h"
      )
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser/SteamLanguageParserTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser/SteamLanguageParserTargets.cmake"
         "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/CMakeFiles/Export/4c4c4d2cb21697a9932cc6ba986e7c6b/SteamLanguageParserTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser/SteamLanguageParserTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser/SteamLanguageParserTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser" TYPE FILE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/CMakeFiles/Export/4c4c4d2cb21697a9932cc6ba986e7c6b/SteamLanguageParserTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser" TYPE FILE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/CMakeFiles/Export/4c4c4d2cb21697a9932cc6ba986e7c6b/SteamLanguageParserTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser" TYPE FILE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/CMakeFiles/Export/4c4c4d2cb21697a9932cc6ba986e7c6b/SteamLanguageParserTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser" TYPE FILE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/CMakeFiles/Export/4c4c4d2cb21697a9932cc6ba986e7c6b/SteamLanguageParserTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/SteamLanguageParser/lib/cmake/SteamLanguageParser" TYPE FILE FILES "C:/Project/sonkwo_client_runtime/build/_deps/utilpp-src/SteamMsgGenerator/src/SteamLanguageParser/CMakeFiles/Export/4c4c4d2cb21697a9932cc6ba986e7c6b/SteamLanguageParserTargets-release.cmake")
  endif()
endif()


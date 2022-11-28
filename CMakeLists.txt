cmake_minimum_required (VERSION 3.23)
set(VERSION "0.0.1")
project(sutil VERSION ${VERSION})
enable_language(CXX ASM )

include(${CMAKE_CURRENT_SOURCE_DIR}/Cmake/CMakeLists.txt)

option (SUTIL_STATIC_CRT "SUTIL STATIC CRT Build ." OFF)

if(SUTIL_STATIC_CRT)
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


add_subdirectory (deps/fmt)
set(SPDLOG_WCHAR_SUPPORT ON CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO ON CACHE BOOL "" FORCE)
add_subdirectory (deps/spdlog)

NewTargetSource()

AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/src/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

foreach(folderpath IN LISTS PublicIncludeFolders)
    target_include_directories( ${TARGET_NAME}
        PUBLIC ${folderpath}
    )
endforeach()
foreach(folderpath IN LISTS PrivateIncludeFolders)
    target_include_directories( ${TARGET_NAME}
        PRIVATE ${folderpath}
    )
endforeach()
foreach(folderpath IN LISTS InterfaceIncludeFolders)
    target_include_directories( ${TARGET_NAME}
        INTERFACE ${folderpath}
    )
endforeach()


target_link_libraries (${TARGET_NAME} PUBLIC spdlog::spdlog)




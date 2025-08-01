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

if(NOT DEFINED GEN_SQLPP_STRUCT OR GEN_SQLPP_STRUCT)
    ImportProgram(Python3 URL https://mirrors.aliyun.com/python-release/windows/python-3.13.5-embed-amd64.zip)
    find_package(Python3 REQUIRED)
    FetchContent_GetProperties(sqlpp23
        SOURCE_DIR sqlpp23_SOURCE_DIR
    )
    execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install pyparsing
    )
    execute_process(COMMAND ${Python3_EXECUTABLE} ${sqlpp23_SOURCE_DIR}/scripts/ddl2cpp -split-tables
        ${CMAKE_CURRENT_SOURCE_DIR}/SteamDDL.sql ${CMAKE_CURRENT_SOURCE_DIR}/generated/SteamMsg
        utilpp::steam
        RESULT_VARIABLE result
    )

    if(NOT ${result} EQUAL 0)
        message(FATAL_ERROR "Failed to gen steamlanguage")
    endif()

    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/SteamDDL.sql" SQL_CREATE_STEAM_CLIENT_TABLE)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/SteamDDl.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/generated/SteamMsg/SteamDDl.h"
        @ONLY
    )

    set(GEN_SQLPP_STRUCT FALSE CACHE BOOL "Generate SQLPP orm C++ files" FORCE)
endif()

NewTargetSource()
AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/generated")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_lib TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)

    target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::simple_utilpp_header)
    target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::steam_util)
    target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::steam_protobufs)
    target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::crypto_lib_helper)
    target_link_libraries(${TARGET_NAME} PRIVATE ZLIB::ZLIB)
    target_link_libraries(${TARGET_NAME} PRIVATE concurrentqueue)
    target_link_libraries(${TARGET_NAME} PRIVATE jwt-cpp::jwt-cpp)
    target_link_libraries(${TARGET_NAME} PRIVATE sqlpp23::sqlpp23)

    AddTargetInclude(${TARGET_NAME})
    AddTargetInstall(${TARGET_NAME} UTILPP)
endfunction()

set(TARGET_NAME steam_msg)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DSTEAM_MSG_API_EXPORTS)
target_compile_definitions(${TARGET_NAME} PUBLIC -DENABLE_STEAM_MSG)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_logger)
target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::net_util)
target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::websocket_util)
target_link_libraries(${TARGET_NAME} PRIVATE SQLite3::SQLite3)
set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_lib(${TARGET_NAME})

set(TARGET_NAME steam_msg_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PUBLIC -DSTEAM_MSG_API_NODLL)
target_compile_definitions(${TARGET_NAME} PUBLIC -DENABLE_STEAM_MSG)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp_a)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_logger_a)
target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::net_util_a)
target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::websocket_util_a)
target_link_libraries(${TARGET_NAME} PRIVATE SQLite3::SQLite3_a)

add_lib(${TARGET_NAME})
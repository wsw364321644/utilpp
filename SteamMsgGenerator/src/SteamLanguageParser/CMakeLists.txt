NewTargetSource()

AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

set(TARGET_NAME SteamLanguageParser)
add_executable(${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::net_util)

if(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::windows_util)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE cxxopts::cxxopts)
target_link_libraries(${TARGET_NAME} PRIVATE re2::re2)

AddTargetInclude(${TARGET_NAME})
AddTargetInstall(${TARGET_NAME} UTILPP)

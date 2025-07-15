

NewTargetSource()
AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_lib TARGET_NAME)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "specification")

    target_link_libraries(${TARGET_NAME} PRIVATE RapidJSON)

    AddTargetInclude(${TARGET_NAME})
    AddTargetInstall(${TARGET_NAME} UTILPP)
endfunction()

set(TARGET_NAME rpc_parser)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DRPC_PARSER_EXPORTS)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_logger)

add_lib(${TARGET_NAME})

set(TARGET_NAME rpc_parser_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PUBLIC -DRPC_PARSER_NODLL)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp_a)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_logger_a)

add_lib(${TARGET_NAME})
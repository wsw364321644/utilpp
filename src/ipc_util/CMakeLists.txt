NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_ipc_util TARGET_NAME)
    set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

    AddTargetInclude(${TARGET_NAME})

    target_link_libraries(${TARGET_NAME} PRIVATE simple_logger)
    target_link_libraries(${TARGET_NAME} PRIVATE rpc_parser_a)
    if(UTILPP_WITH_STATIC_LIBUV)
        target_link_libraries(${TARGET_NAME} PRIVATE libuv::uv_a)
    else()
        target_link_libraries(${TARGET_NAME} PRIVATE libuv::uv)
    endif()

    install(TARGETS ${TARGET_NAME}
        EXPORT ${TARGET_NAME}Targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
    )

    install(EXPORT ${TARGET_NAME}Targets
        FILE ${TARGET_NAME}Targets.cmake
        NAMESPACE UTILPP::
        DESTINATION lib/cmake/${TARGET_NAME}
    )
endfunction()

set(TARGET_NAME ipc_util)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DIPC_UTIL_EXPORTS)
target_link_libraries(${TARGET_NAME} PUBLIC simple_utilpp)
add_ipc_util(${TARGET_NAME})

set(TARGET_NAME ipc_util_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DIPC_UTIL_NODLL)
target_link_libraries(${TARGET_NAME} PUBLIC simple_utilpp_a)
add_ipc_util(${TARGET_NAME})

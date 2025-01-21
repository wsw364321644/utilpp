NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_simple_utilpp TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
    set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${TARGET_NAME} PUBLIC simple_utilpp_header)
    AddTargetInclude(${TARGET_NAME})

    # if(UTILPP_ENABLE_LOG)
    # target_compile_definitions(${TARGET_NAME} PRIVATE UTILPP_ENABLE_LOG)
    # target_link_libraries (${TARGET_NAME} PRIVATE simple_logger)
    # endif()
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

set(TARGET_NAME simple_utilpp)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DSIMPLE_UTIL_API_EXPORTS)
target_link_libraries(${TARGET_NAME} PUBLIC simple_util_include)
target_link_libraries(${TARGET_NAME} PUBLIC simple_util_obj)
add_simple_utilpp(${TARGET_NAME})

set(TARGET_NAME simple_utilpp_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PUBLIC -DSIMPLE_UTIL_API_NODLL)
target_link_libraries(${TARGET_NAME} PUBLIC simple_util_a)
add_simple_utilpp(${TARGET_NAME})
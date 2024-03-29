set( TARGET_NAME simple_util )
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})


function(add_simple_util TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES C_STANDARD 23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED OFF)
    set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    
    AddTargetInclude(${TARGET_NAME})
    
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

add_library (${TARGET_NAME} SHARED  ${SourceFiles} )
target_compile_definitions(${TARGET_NAME} PRIVATE -DSIMPLE_UTIL_API_EXPORTS)
add_simple_util(${TARGET_NAME})

add_library (${TARGET_NAME}_a STATIC  ${SourceFiles} )
target_compile_definitions(${TARGET_NAME}_a PUBLIC -DSIMPLE_UTIL_API_NODLL)
add_simple_util(${TARGET_NAME}_a)

add_library (${TARGET_NAME}_obj OBJECT  ${SourceFiles} )
target_compile_definitions(${TARGET_NAME}_obj PRIVATE -DSIMPLE_UTIL_API_EXPORTS)
add_simple_util(${TARGET_NAME}_obj)
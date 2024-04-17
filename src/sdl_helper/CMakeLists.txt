
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})


function(add_sdl_helper TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES C_STANDARD 23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED OFF)
    set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE C)
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

set( TARGET_NAME sdl_helper )
add_library (${TARGET_NAME} SHARED  ${SourceFiles} )
add_sdl_helper(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} PRIVATE simple_util)
target_link_libraries(${TARGET_NAME} PUBLIC SDL2::SDL2)

set( TARGET_NAME sdl_helper_a )
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )
add_sdl_helper(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} PRIVATE simple_util_a)
target_link_libraries(${TARGET_NAME} PUBLIC SDL2::SDL2-static)
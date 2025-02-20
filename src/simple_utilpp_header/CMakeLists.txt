NewTargetSource()
AddSourceFolder(INCLUDE RECURSE INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/public")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})

function(configure_library TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
    set_target_properties(${TARGET_NAME} PROPERTIES INTERFACE_COMPILE_FEATURES  cxx_std_20)
    target_compile_features(${TARGET_NAME} INTERFACE cxx_std_20)
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

set( TARGET_NAME simple_utilpp_header)
add_library (${TARGET_NAME} INTERFACE  ${SourceFiles} )
configure_library(${TARGET_NAME})

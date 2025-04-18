set( TARGET_NAME windows_util)
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})

function(add_windows_util TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES C_STANDARD 23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

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

set(TARGET_NAME windows_util_a)
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )
target_compile_definitions(${TARGET_NAME} PUBLIC -DWINDOWS_UTIL_NODLL)
target_link_libraries (${TARGET_NAME} PUBLIC simple_utilpp_a)
add_windows_util(${TARGET_NAME})

set(TARGET_NAME windows_util)
add_library (${TARGET_NAME} SHARED  ${SourceFiles} )
target_compile_definitions(${TARGET_NAME} PRIVATE -DWINDOWS_UTIL_EXPORTS)
target_link_libraries (${TARGET_NAME} PUBLIC simple_utilpp)
add_windows_util(${TARGET_NAME})

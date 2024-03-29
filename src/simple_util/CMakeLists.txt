set( TARGET_NAME simple_util )
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )

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
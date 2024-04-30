set( TARGET_NAME windows_util)
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

AddTargetInclude(${TARGET_NAME})

target_link_libraries (${TARGET_NAME} PUBLIC simple_utilpp_a)


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
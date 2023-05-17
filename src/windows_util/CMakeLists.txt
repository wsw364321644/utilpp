set( TARGET_NAME windows_util)
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

AddTargetInclude(${TARGET_NAME})

target_link_libraries (${TARGET_NAME} PUBLIC sutils)
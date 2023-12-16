set( TARGET_NAME sutils )
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )


set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

AddTargetInclude(${TARGET_NAME})

target_link_libraries (${TARGET_NAME} PRIVATE fmt-header-only)

if(UTILPP_ENABLE_LOG)
    target_compile_definitions(${TARGET_NAME} PRIVATE UTILPP_ENABLE_LOG)
    target_link_libraries (${TARGET_NAME} PRIVATE simple_logger)
endif()
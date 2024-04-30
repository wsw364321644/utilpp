set( TARGET_NAME simple_logger )
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)

AddTargetInclude(${TARGET_NAME})

target_link_libraries (${TARGET_NAME} PUBLIC spdlog::spdlog)
target_link_libraries (${TARGET_NAME} PRIVATE simple_utilpp_a)


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
set(TARGET_NAME net_util)
NewTargetSource()

AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

add_library(${TARGET_NAME} STATIC ${SourceFiles})
AddTargetInclude(${TARGET_NAME})

set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
target_compile_definitions(${TARGET_NAME} PUBLIC -DWIN32_LEAN_AND_MEAN)

target_link_libraries(${TARGET_NAME} PRIVATE simple_utilpp)
target_link_libraries(${TARGET_NAME} PRIVATE simple_logger)
target_link_libraries(${TARGET_NAME} PRIVATE CURL::libcurl)


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
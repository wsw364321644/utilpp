function(add_lib TARGET_NAME)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

    target_link_libraries(${TARGET_NAME} PRIVATE Taskflow)
    target_link_libraries(${TARGET_NAME} PRIVATE concurrentqueue)

    # target_link_libraries(${TARGET_NAME} PRIVATE TBB::tbb)
    AddTargetInclude(${TARGET_NAME})
    AddTargetInstall(${TARGET_NAME} UTILPP)
endfunction()

set(TASK_LIB_TARGET_NAME taskflow)
FetchContent_GetProperties(${TASK_LIB_TARGET_NAME})

if(${TASK_LIB_TARGET_NAME}_POPULATED)
    NewTargetSource()
    AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
    AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
    source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

    set(TARGET_NAME task_manager)
    add_library(${TARGET_NAME} SHARED ${SourceFiles})
    target_compile_definitions(${TARGET_NAME} PRIVATE -DTASK_MANAGER_API_EXPORTS)
    target_link_libraries(${TARGET_NAME} PRIVATE simple_utilpp)
    target_link_libraries(${TARGET_NAME} PRIVATE simple_logger)
    add_lib(${TARGET_NAME})

    set(TARGET_NAME task_manager_a)
    add_library(${TARGET_NAME} STATIC ${SourceFiles})
    target_compile_definitions(${TARGET_NAME} PUBLIC -DTASK_MANAGER_API_NODLL)
    target_link_libraries(${TARGET_NAME} PRIVATE simple_utilpp_a)
    target_link_libraries(${TARGET_NAME} PRIVATE simple_logger_a)
    add_lib(${TARGET_NAME})
endif()

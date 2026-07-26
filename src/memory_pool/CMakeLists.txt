NewTargetSource()
AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_lib TARGET_NAME)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

    target_link_libraries(${TARGET_NAME} PUBLIC RapidJSON::RapidJSON)
    # conflict with static crt
    #target_link_libraries(${TARGET_NAME} PRIVATE mimalloc-static)
    AddTargetInclude(${TARGET_NAME})
    add_library(${PROJECT_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

    if(NOT UTILPP_DISABLE_INSTALL)
        AddTargetInstall(${TARGET_NAME} ${PROJECT_NAME})
    endif()
endfunction()

set(TARGET_NAME memory_pool)
add_library(${TARGET_NAME} SHARED )
target_compile_definitions(${TARGET_NAME} PRIVATE -DMEMORY_POOL_EXPORTS)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp)
target_link_libraries(${TARGET_NAME} PUBLIC simdjson::simdjson)
add_lib(${TARGET_NAME})

set(TARGET_NAME memory_pool_a)
add_library(${TARGET_NAME} STATIC)
target_compile_definitions(${TARGET_NAME} PUBLIC -DMEMORY_POOL_NODLL)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp_a)
target_link_libraries(${TARGET_NAME} PUBLIC simdjson::simdjson_static)
add_lib(${TARGET_NAME})

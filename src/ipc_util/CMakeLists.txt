set(TARGET_NAME ipc_util)
NewTargetSource()

AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_ipc_util TARGET_NAME)
    set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "ipc")

    AddTargetInclude(${TARGET_NAME})

    target_link_libraries(${TARGET_NAME} PUBLIC sutils)

    if(UTILPP_WITH_STATIC_LIBUV)
        message(STATUS "uv_a")
        target_link_libraries(${TARGET_NAME} PRIVATE uv_a)
    else()
        message(STATUS "uv")
        target_link_libraries(${TARGET_NAME} PRIVATE uv)
    endif()
endfunction()

add_library(${TARGET_NAME} SHARED ${SourceFiles})
add_ipc_util(${TARGET_NAME})

add_library(${TARGET_NAME}_a STATIC ${SourceFiles})
add_ipc_util(${TARGET_NAME}_a)

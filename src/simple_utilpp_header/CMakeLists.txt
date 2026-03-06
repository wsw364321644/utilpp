NewTargetSource()
AddSourceFolder(RECURSE INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/public")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(configure_library TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
    target_compile_features(${TARGET_NAME} INTERFACE cxx_std_20)

    AddTargetInclude(${TARGET_NAME})
    add_library(${PROJECT_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

    if(NOT UTILPP_DISABLE_INSTALL)
        AddTargetInstall(${TARGET_NAME} ${PROJECT_NAME})
    endif()
endfunction()

set(TARGET_NAME simple_utilpp_header)
add_library(${TARGET_NAME} INTERFACE ${SourceFiles})
configure_library(${TARGET_NAME})

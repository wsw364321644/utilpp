
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_simple_hook_helper TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES C_STANDARD 23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMT.LIB;msvcrt")
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/NODEFAULTLIB:LIBCMTD.LIB;msvcrtd")

    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    AddTargetInclude(${TARGET_NAME})

    
    target_link_libraries(${TARGET_NAME} PRIVATE MbedTLS::mbedcrypto)
    target_link_libraries(${TARGET_NAME} PRIVATE gcem)

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
endfunction()

set(TARGET_NAME simple_hook_helper)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
add_simple_hook_helper(${TARGET_NAME})
target_compile_definitions(${TARGET_NAME} PRIVATE -DHOOK_HELPER_EXPORTS)
target_link_libraries(${TARGET_NAME} PRIVATE ipc_util)
target_link_libraries(${TARGET_NAME} PRIVATE windows_util)
target_link_libraries(${TARGET_NAME} PUBLIC SDL2::SDL2)

set(TARGET_NAME simple_hook_helper_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
add_simple_hook_helper(${TARGET_NAME})
target_compile_definitions(${TARGET_NAME} PUBLIC -DHOOK_HELPER_NODLL)
target_link_libraries(${TARGET_NAME} PRIVATE ipc_util_a)
target_link_libraries(${TARGET_NAME} PRIVATE windows_util_a)
target_link_libraries(${TARGET_NAME} PUBLIC SDL2::SDL2-static)
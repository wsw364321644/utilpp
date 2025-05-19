NewTargetSource()
AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_lib TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
    set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    target_link_libraries(${TARGET_NAME} PRIVATE nlohmann_json::nlohmann_json)
    target_link_libraries(${TARGET_NAME} PRIVATE ValveFileVDF::ValveFileVDF)
    target_link_libraries(${TARGET_NAME} PRIVATE concurrentqueue)

    AddTargetInclude(${TARGET_NAME})
    AddTargetInstall(${TARGET_NAME} UTILPP)
endfunction()

set(TARGET_NAME steam_util)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DSTEAM_UTIL_API_EXPORTS)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp)

if(TARGET UTILPP::net_util)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DSTEAM_UTIL_HAS_NET)
    target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::net_util)
endif()

if(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::windows_util)
endif()

add_lib(${TARGET_NAME})

set(TARGET_NAME steam_util_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PUBLIC -DSTEAM_UTIL_API_NODLL)
target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::simple_utilpp_a)

if(TARGET UTILPP::net_util_a)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DSTEAM_UTIL_HAS_NET)
    target_link_libraries(${TARGET_NAME} PUBLIC UTILPP::net_util_a)
endif()

if(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE UTILPP::windows_util_a)
endif()

add_lib(${TARGET_NAME})
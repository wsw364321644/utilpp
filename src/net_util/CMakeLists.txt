find_package(CapnProto REQUIRED)

if(NOT NET_UTIL_GENERATE_CAPNP_FILES)
    execute_process(COMMAND ${CapnProto_DIR}/../../../bin/capnp
        compile
        --output=${CapnProto_DIR}/../../../bin/capnpc-c++:private/Downloader
        ./download-file.capnp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE result)
        message(STATUS "result --output=${CAPNPC_CXX_EXECUTABLE}:private/Downloader")
    set(NET_UTIL_GENERATE_CAPNP_FILES ON CACHE BOOL "net uitl Generate capnp files")
endif()

NewTargetSource()

AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_lib TARGET_NAME)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

    if(WIN32)
        set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMT.LIB /NODEFAULTLIB:msvcrt")
        set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/NODEFAULTLIB:LIBCMTD.LIB /NODEFAULTLIB:msvcrtd")
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE CURL::libcurl)
    target_link_libraries(${TARGET_NAME} PRIVATE concurrentqueue)
    target_link_libraries(${TARGET_NAME} PRIVATE CapnProto::kj)
    target_link_libraries(${TARGET_NAME} PRIVATE CapnProto::capnp)
    
    AddTargetInclude(${TARGET_NAME})
    AddTargetInstall(${TARGET_NAME} UTILPP)
endfunction()

set(TARGET_NAME net_util)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DSIMPLE_NET_API_EXPORTS)
target_link_libraries(${TARGET_NAME} PUBLIC simple_utilpp)
target_link_libraries(${TARGET_NAME} PRIVATE simple_logger)
add_lib(${TARGET_NAME})

set(TARGET_NAME net_util_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PUBLIC -DSIMPLE_NET_API_NODLL)
target_link_libraries(${TARGET_NAME} PUBLIC simple_utilpp_a)
target_link_libraries(${TARGET_NAME} PRIVATE simple_logger_a)
add_lib(${TARGET_NAME})
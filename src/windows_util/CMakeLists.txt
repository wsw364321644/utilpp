set(TARGET_NAME windows_util)
NewTargetSource()
AddSourceFolder(RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${SourceFiles})

function(add_windows_util TARGET_NAME)
    set_target_properties(${TARGET_NAME} PROPERTIES C_STANDARD 23)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "utilpp")

    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

    if(OPENGL_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE OpenGL::GL)

        if(GLUT_FOUND)
            target_link_libraries(${TARGET_NAME} PRIVATE GLUT::GLUT)
        endif()

        if(GLFW_FOUND)
            target_link_libraries(${TARGET_NAME} PRIVATE GLFW::GLFW)
        endif()
    endif()


    AddTargetInclude(${TARGET_NAME})
    AddTargetInstall(${TARGET_NAME} UTILPP)
endfunction()

find_package(OpenGL)
find_package(GLUT)
find_package(GLFW)

set(TARGET_NAME windows_util_a)
add_library(${TARGET_NAME} STATIC ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PUBLIC -DWINDOWS_UTIL_NODLL)
target_link_libraries(${TARGET_NAME} PUBLIC simple_utilpp_a)
add_windows_util(${TARGET_NAME})

set(TARGET_NAME windows_util)
add_library(${TARGET_NAME} SHARED ${SourceFiles})
target_compile_definitions(${TARGET_NAME} PRIVATE -DWINDOWS_UTIL_EXPORTS)
target_link_libraries(${TARGET_NAME} PUBLIC simple_utilpp)
add_windows_util(${TARGET_NAME})
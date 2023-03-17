set( TARGET_NAME windows_util)
NewTargetSource()
AddSourceFolder(INCLUDE RECURSE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/public")
AddSourceFolder(RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/private")
source_group( TREE ${PROJECT_SOURCE_DIR}  FILES ${SourceFiles})
add_library (${TARGET_NAME} STATIC  ${SourceFiles} )

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
set_target_properties( ${TARGET_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

foreach(folderpath IN LISTS PublicIncludeFolders)
    target_include_directories( ${TARGET_NAME}
        PUBLIC ${folderpath}
    )
endforeach()
foreach(folderpath IN LISTS PrivateIncludeFolders)
    target_include_directories( ${TARGET_NAME}
        PRIVATE ${folderpath}
    )
endforeach()
foreach(folderpath IN LISTS InterfaceIncludeFolders)
    target_include_directories( ${TARGET_NAME}
        INTERFACE ${folderpath}
    )
endforeach()

target_link_libraries (${TARGET_NAME} PUBLIC sutils)
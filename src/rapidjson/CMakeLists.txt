set(TARGET_NAME rapidjson)
FetchContent_GetProperties(${TARGET_NAME})

if(${TARGET_NAME}_POPULATED)
    NewTargetSource()
    AddSourceFolder(INCLUDE RECURSE INTERFACE "${${TARGET_NAME}_SOURCE_DIR}/include")

    source_group(TREE ${${TARGET_NAME}_SOURCE_DIR} FILES ${SourceFiles})
    add_library(${TARGET_NAME} INTERFACE ${SourceFiles})
    set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(${TARGET_NAME} INTERFACE -DNOMINMAX)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "data")

    foreach(folderpath IN LISTS PublicIncludeFolders)
        target_include_directories(${TARGET_NAME}
            PUBLIC ${folderpath}
        )
    endforeach()

    foreach(folderpath IN LISTS PrivateIncludeFolders)
        target_include_directories(${TARGET_NAME}
            PRIVATE ${folderpath}
        )
    endforeach()

    foreach(folderpath IN LISTS InterfaceIncludeFolders)
        target_include_directories(${TARGET_NAME}
            INTERFACE ${folderpath}
        )
    endforeach()
endif()

set(TARGET_NAME better-enums)
FetchContent_GetProperties(${TARGET_NAME})

if(${TARGET_NAME}_POPULATED)
    NewTargetSource()
    AddSourceFolder(INCLUDE INTERFACE "${${TARGET_NAME}_SOURCE_DIR}")

    source_group(TREE ${${TARGET_NAME}_SOURCE_DIR} FILES ${SourceFiles})
    add_library(${TARGET_NAME} INTERFACE ${SourceFiles})
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

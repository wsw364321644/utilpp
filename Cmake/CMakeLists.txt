cmake_minimum_required (VERSION 3.20)

FUNCTION (EXCLUDE_FILES_FROM_DIR_IN_LIST _InFileList _excludeDirName )
  foreach (ITR ${_InFileList})
    if ("${ITR}" MATCHES "(.*)${_excludeDirName}(.*)")                   # Check if the item matches the directory name in _excludeDirName

      list (REMOVE_ITEM _InFileList ${ITR})                              # Remove the item from the list
    endif ("${ITR}" MATCHES "(.*)${_excludeDirName}(.*)")

  endforeach(ITR)
  set(EXCLUDED_FILES ${_InFileList} PARENT_SCOPE)                          # Return the SOURCE_FILES variable to the calling parent
ENDFUNCTION (EXCLUDE_FILES_FROM_DIR_IN_LIST)

macro(SearchSourceFiles FolderPath IsRecurse)	
	set(temppath ${FolderPath})
	cmake_path(APPEND  temppath  "*.h"  OUTPUT_VARIABLE  TmpHHeader)
	cmake_path(APPEND  temppath  "*.hpp" OUTPUT_VARIABLE  TmpHppHeader)
	cmake_path(APPEND  temppath  "*.c" OUTPUT_VARIABLE  TmpC)
	cmake_path(APPEND  temppath  "*.cc" OUTPUT_VARIABLE  TmpCC)
	cmake_path(APPEND  temppath  "*.cpp" OUTPUT_VARIABLE  TmpCpp)
	cmake_path(APPEND  temppath  "*.s" OUTPUT_VARIABLE  TmpS)
	cmake_path(APPEND  temppath  "*.asm" OUTPUT_VARIABLE  TmpAsm)
	cmake_path(APPEND  temppath  "*.ico" OUTPUT_VARIABLE  TmpIcon)
	cmake_path(APPEND  temppath  "*.rc" OUTPUT_VARIABLE  TmpRC)

  cmake_path(APPEND  temppath  "*.config" OUTPUT_VARIABLE  TmpConfig)
  cmake_path(APPEND  temppath  "*.xaml" OUTPUT_VARIABLE  TmpXAML)
  cmake_path(APPEND  temppath  "*.cs" OUTPUT_VARIABLE  TmpCS)
  cmake_path(APPEND  temppath  "*.resx" OUTPUT_VARIABLE  TmpRESX)
  cmake_path(APPEND  temppath  "*.settings" OUTPUT_VARIABLE  TmpSettings)
	if(${IsRecurse})
		file(GLOB_RECURSE TmpSource LIST_DIRECTORIES false CONFIGURE_DEPENDS  ${TmpHHeader} ${TmpHppHeader} ${TmpC} ${TmpCC} ${TmpCpp}  ${TmpIcon} ${TmpRC})
	else()
		file(GLOB TmpSource LIST_DIRECTORIES false CONFIGURE_DEPENDS  ${TmpHHeader} ${TmpHppHeader} ${TmpC} ${TmpCC} ${TmpCpp}  ${TmpIcon} ${TmpRC})
	endif()

	if(${IsRecurse})
		file(GLOB_RECURSE TmpAsmSource LIST_DIRECTORIES false CONFIGURE_DEPENDS  ${TmpS} ${TmpAsm})
	else()
		file(GLOB TmpAsmSource LIST_DIRECTORIES false CONFIGURE_DEPENDS  ${TmpS} ${TmpAsm})
	endif()
  
  set_property(SOURCE TmpAsmSource APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
  

  list(APPEND TmpSource  ${TmpAsmSource})

	if (WIN32)
		EXCLUDE_FILES_FROM_DIR_IN_LIST("${TmpSource}" "Linux")
    EXCLUDE_FILES_FROM_DIR_IN_LIST("${TmpSource}" "linux")
		#message(STATUS "EXCLUDED_FILES ${EXCLUDED_FILES}")
	elseif (UNIX)
		EXCLUDE_FILES_FROM_DIR_IN_LIST("${TmpSource}" "Windows")
    EXCLUDE_FILES_FROM_DIR_IN_LIST("${TmpSource}" "windows")
	endif()

	#message(STATUS "EXCLUDED_FILES ${EXCLUDED_FILES}")
	if (NOT EXCLUDED_FILES STREQUAL "")
		set(SourceFiles "${SourceFiles};${EXCLUDED_FILES}")
	endif()
endmacro(SearchSourceFiles)

macro(AddSourceFolder)
    set(options INCLUDE RECURSE)
    set(oneValueArgs  )
    set(multiValueArgs  PUBLIC PRIVATE INTERFACE)
    cmake_parse_arguments(AddSourceFolder "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

	if(AddSourceFolder_INCLUDE)
		foreach(LETTER ${AddSourceFolder_PRIVATE})
			list(APPEND PrivateIncludeFolders ${LETTER}) 
			SearchSourceFiles(${LETTER} ${AddSourceFolder_RECURSE})
		endforeach()
	
		foreach(LETTER ${AddSourceFolder_PUBLIC})
			list(APPEND PublicIncludeFolders ${LETTER}) 
			SearchSourceFiles(${LETTER} ${AddSourceFolder_RECURSE})
		endforeach()
	
		foreach(LETTER ${AddSourceFolder_INTERFACE})
			list(APPEND InterfaceIncludeFolders ${LETTER})
			SearchSourceFiles(${LETTER} ${AddSourceFolder_RECURSE})
		endforeach()
	else()
		foreach(LETTER ${AddSourceFolder_UNPARSED_ARGUMENTS})
			SearchSourceFiles(${LETTER} ${AddSourceFolder_RECURSE})
		endforeach()
	endif()

endmacro(AddSourceFolder)

macro(NewTargetSource)
set(SourceFiles "")
set(PrivateIncludeFolders "")
set(PublicIncludeFolders "")
set(InterfaceIncludeFolders "")
endmacro(NewTargetSource)


MACRO(ADD_DELAYLOAD_FLAGS flagsVar)
  SET(dlls "${ARGN}")
  FOREACH(dll ${dlls})
    SET(${flagsVar} "${${flagsVar}} /DELAYLOAD:${dll}.dll")
  ENDFOREACH()
ENDMACRO()

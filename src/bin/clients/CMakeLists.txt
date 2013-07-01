include(ExternalProject)

###############################################
# Add library dependencies here as dep_<folder>
SET(dep_stinger_server stinger_core stinger_util)

#################################################################
# Iterate over each library and build appropriately based on type
FILE(GLOB subs RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)
FOREACH(subdir ${subs})
  IF(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${subdir})
    IF(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/CMakeLists.txt)
      MESSAGE("BIN: CMAKE SUB FOUND ${subdir}")
      ADD_SUBDIRECTORY(${subdir})
    ELSEIF(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/configure)
      MESSAGE("BIN: AUTOTOOLS SUB FOUND ${subdir}")
      ExternalProject_Add(
	"${subdir}"
	DOWNLOAD_COMMAND ""
	SOURCE_DIR "${subdir}"
	CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/configure --prefix=${CMAKE_BINARY_DIR}
	BUILD_COMMAND ${MAKE}
      )
    ELSE()
      MESSAGE("BIN: DEFAULT SUBDIR FOUND ${subdir} TODO")
    ENDIF()
  ENDIF()
ENDFOREACH()

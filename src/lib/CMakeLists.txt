include(ExternalProject)

###############################################
# Add library dependencies here as dep_<folder>
SET(dep_stinger_utils stinger_core int_hm_seq string fmemopen compat)
SET(dep_kv_store string)
SET(dep_mongoose dl)

################################################
# make sure full-empty emulation isn't optimized
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_LIST_DIR}/stinger_core/src/x86_full_empty.c PROPERTIES COMPILE_FLAGS -O0)

#################################################################
# Iterate over each library and build appropriately based on type
FILE(GLOB subs RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)
FOREACH(subdir ${subs})
  IF(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${subdir})

    # If the directory has a CMakeFile this gets used
    IF(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/CMakeLists.txt)
      MESSAGE("LIB: CMAKE SUB FOUND ${subdir}")
      ADD_SUBDIRECTORY(${subdir})

    # Libraries using autotools are installed to the output directory
    ELSEIF(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/configure)
      MESSAGE("LIB: AUTOTOOLS SUB FOUND ${subdir}")
      ExternalProject_Add(
	"${subdir}"
	DOWNLOAD_COMMAND ""
	SOURCE_DIR "${subdir}"
	CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/configure --prefix=${CMAKE_BINARY_DIR}
	BUILD_COMMAND ${MAKE}
      )

    # Otherwise the default build process is to take all of the source files and build static and dynamic 
    # libraries out of them and copy the headers into the output directory.  The name used will simply be
    # the name of the folder.
    ELSE()
      MESSAGE("LIB: DEFAULT SUBDIR FOUND ${subdir}")

      # Glob and compile and protobuf sources
      IF(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/proto)
	IF(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${subdir}/proto)
	  FILE(GLOB protos ABSOLUTE ${CMAKE_CURRENT_LIST_DIR}/${subdir}/proto/*.proto)
	  CUST_PROTOBUF_GENERATE_CPP(protosrc protohead ${protos})
	  SET(realprotosrc)
	  FOREACH(protoout ${protosrc})
	    GET_FILENAME_COMPONENT(protofile ${protoout} NAME)
	    LIST(APPEND realprotosrc ${CMAKE_CURRENT_BINARY_DIR}/${subdir}/proto/${protofile})
	  ENDFOREACH()
	  SET_SOURCE_FILES_PROPERTIES(${realprotosrc} PROPERTIES GENERATED TRUE)
	  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
	  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/${subdir})
	ENDIF()
      ENDIF()

      # Glob the c++ source
      IF (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/src)
	FILE(GLOB srcs RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/${subdir}/src/*)
	INCLUDE_DIRECTORIES(${subdir}/inc)

	# Build the shared library
	ADD_LIBRARY(${subdir} SHARED ${srcs} ${realprotosrc} ${protohead})
	TARGET_LINK_LIBRARIES(${subdir} ${dep_${subdir}})
	ADD_CUSTOM_COMMAND(TARGET ${subdir}
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/include/${subdir}"
	  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/${subdir}/inc" "${CMAKE_BINARY_DIR}/include/${subdir}"
	  )

	# Build the static library
	ADD_LIBRARY(${subdir}_static STATIC ${srcs} ${realprotosrc} ${protohead})
	TARGET_LINK_LIBRARIES(${subdir}_static ${dep_${subdir}})
	ADD_CUSTOM_COMMAND(TARGET ${subdir}_static
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/include/${subdir}"
	  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/${subdir}/inc" "${CMAKE_BINARY_DIR}/include/${subdir}"
	  )
	SET_TARGET_PROPERTIES(${subdir}_static PROPERTIES OUTPUT_NAME "${subdir}")
      ELSE()
	IF (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/include)
	MESSAGE("  here we are  ")
	  ADD_CUSTOM_TARGET(${subdir} 
	    ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/include/${subdir}"
	    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/${subdir}/include" "${CMAKE_BINARY_DIR}/include/${subdir}"
	    )
	ENDIF()
      ENDIF()

    ENDIF()
  ENDIF()
ENDFOREACH()

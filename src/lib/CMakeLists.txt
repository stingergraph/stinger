include(ExternalProject)

##################################################
# Add library dependencies here as libdep_<folder>
SET(libdep_stinger_utils stinger_core int_hm_seq string fmemopen compat)
SET(libdep_kv_store string)
SET(libdep_mongoose dl)
IF(APPLE)
  SET(libdep_stinger_net ${CMAKE_BINARY_DIR}/lib/libprotobuf.dylib stinger_core)
ELSE()
  SET(libdep_stinger_net ${CMAKE_BINARY_DIR}/lib/libprotobuf.so stinger_core)
ENDIF()

#################################################
# Add target dependencies here as tgtdep_<folder>
SET(tgtdep_default protobuf)
SET(tgtdep_stinger_net stinger_core)

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
      SET(realprotosrc "")
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
      TARGET_LINK_LIBRARIES(${subdir} ${libdep_${subdir}})
      ADD_DEPENDENCIES(${subdir} ${tgtdep_default})
      FOREACH(dep_target ${tgtdep_${subdir}})
	ADD_DEPENDENCIES(${subdir} ${dep_target})
      ENDFOREACH()
	ADD_CUSTOM_COMMAND(TARGET ${subdir}
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/include/${subdir}"
	  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/${subdir}/inc" "${CMAKE_BINARY_DIR}/include/${subdir}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/${subdir}/proto" "${CMAKE_BINARY_DIR}/include/${subdir}/proto"
	  )

	# Build the static library
	ADD_LIBRARY(${subdir}_static STATIC ${srcs} ${realprotosrc} ${protohead})
      TARGET_LINK_LIBRARIES(${subdir}_static ${libdep_${subdir}})
      ADD_DEPENDENCIES(${subdir}_static ${tgtdep_default})
      FOREACH(dep_target ${tgtdep_${subdir}})
	ADD_DEPENDENCIES(${subdir}_static ${dep_target})
      ENDFOREACH()
	ADD_CUSTOM_COMMAND(TARGET ${subdir}_static
	  POST_BUILD
	  COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/include/${subdir}"
	  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/${subdir}/inc" "${CMAKE_BINARY_DIR}/include/${subdir}"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/${subdir}/proto" "${CMAKE_BINARY_DIR}/include/${subdir}/proto"
	  )
	SET_TARGET_PROPERTIES(${subdir}_static PROPERTIES OUTPUT_NAME "${subdir}")
      ELSE()
	IF (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/include)
	  ADD_CUSTOM_TARGET(${subdir} 
	    ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/include/${subdir}"
	    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/${subdir}/include" "${CMAKE_BINARY_DIR}/include/${subdir}"
	    )
	ENDIF()
      ENDIF()

    ENDIF()
  ENDIF()
ENDFOREACH()

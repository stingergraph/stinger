
###############################################
# Add library dependencies here as dep_<folder>
SET(libdep_default int_hm_seq int_ht_seq stinger_core stinger_utils string)
SET(libdep_depold_community int_hm_seq int_ht_seq)

IF(APPLE)
  SET(libdep_protobuf_test ${CMAKE_BINARY_DIR}/lib/libprotobuf.dylib)
ELSEIF(CYGWIN OR WIN32)
  SET(libdep_protobuf_test ${CMAKE_BINARY_DIR}/lib/libprotobuf.dll.a)
ELSE()
  SET(libdep_protobuf_test ${CMAKE_BINARY_DIR}/lib/libprotobuf.so)
ENDIF()

SET(tgtdep_default int_hm_seq int_ht_seq stinger_core stinger_utils rt string)
SET(tgtdep_protobuf_test protobuf)
SET(tgtdep_depold_community int_hm_seq int_ht_seq)

#################################################################
# Iterate over each project and build appropriately based on type
FILE(GLOB subs RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)
FOREACH(subdir ${subs})
  IF(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${subdir})
    IF(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${subdir}/CMakeLists.txt)
      MESSAGE("BIN/STANDALONE: CMAKE SUB FOUND ${subdir}")
      ADD_SUBDIRECTORY(${subdir})
    ELSE()
      MESSAGE("BIN/STANDALONE: DEFAULT SUBDIR FOUND ${subdir}")
      FILE(GLOB srcs ${CMAKE_CURRENT_LIST_DIR}/${subdir}/*.c)
      FILE(GLOB cxx_srcs ${CMAKE_CURRENT_LIST_DIR}/${subdir}/*.cc)
      FILE(GLOB src ${CMAKE_CURRENT_LIST_DIR}/${subdir}/src/*)
      INCLUDE_DIRECTORIES(${subdir})
      MESSAGE("source files:${srcs} ${cxx_srcs} ${src}")
      ADD_EXECUTABLE(${subdir} ${srcs} ${cxx_srcs} ${src})

      ADD_DEPENDENCIES(${subdir} ${tgtdep_default})
      TARGET_LINK_LIBRARIES(${subdir} ${libdep_default})

      FOREACH(dep_library ${tgtdep_${subdir}})
        MESSAGE("dependency: ${dep_library}")
	ADD_DEPENDENCIES(${subdir} ${dep_library})
      ENDFOREACH()
      TARGET_LINK_LIBRARIES(${subdir} ${libdep_${subdir}})

      #IF(DEFINED ${libdep_${subdir}})
	#ADD_DEPENDENCIES(${subdir} ${libdep_${subdir}})
      #ENDIF()

#      FOREACH(dep_library ${libdep_${subdir}})
#       MESSAGE("looking for ${libdep_library} in ${CMAKE_BINARY_DIR}/lib")
#	FIND_LIBRARY(lib_dependency_path ${libdep_library} ${CMAKE_BINARY_DIR}/lib)
#	MESSAGE("found ${lib_dependency_path}")
#	TARGET_LINK_LIBRARIES(${subdir} ${lib_dependency_path})
#      ENDFOREACH()

    ENDIF()
  ENDIF()
ENDFOREACH()

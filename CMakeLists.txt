CMAKE_MINIMUM_REQUIRED (VERSION 2.8.11)

PROJECT (STINGER)
SET(STINGER_VERSION_MAJOR 2013)
SET(STINGER_VERSION_MINOR 06)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

SET(STINGER_NAME_USE_SQLITE FALSE CACHE BOOL "Use sqlite for virtual to physmap")
IF(STINGER_NAME_USE_SQLITE)
  add_definitions(-DNAME_USE_SQLITE)
ENDIF()

SET(STINGER_USE_TCP FALSE CACHE BOOL "Use TCP instead of Unix Sockets")
IF(STINGER_USE_TCP)
  add_definitions(-DSTINGER_USE_TCP)
ENDIF()

FIND_PACKAGE( OpenMP )
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

LINK_LIBRARIES(m)

IF(APPLE)
  # Apple doesn't support librt, using CoreServices instead
  LINK_LIBRARIES(/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices)
ELSEIF(WIN32)
  # Not sure what to do here as far as librt...?
  LINK_LIBRARIES(rt)
ELSE()
  LINK_LIBRARIES(rt)
ENDIF()

# Doxygen
find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
endif(DOXYGEN_FOUND)

# Custom protobuf to use STINGER distributed version
function(CUST_PROTOBUF_GENERATE_CPP SUBDIR SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}/proto/${FIL_WE}.pb.cc")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}/proto/${FIL_WE}.pb.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}/proto/${FIL_WE}.pb.cc"
             "${CMAKE_CURRENT_BINARY_DIR}/${SUBDIR}/proto/${FIL_WE}.pb.h"
      COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/protoc
      ARGS --cpp_out  ${CMAKE_CURRENT_BINARY_DIR} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      COMMENT "Running C++ protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x")

INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/include")

ADD_SUBDIRECTORY(src)

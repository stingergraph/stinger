CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

PROJECT (STINGER)
SET(STINGER_VERSION_MAJOR 2013)
SET(STINGER_VERSION_MINOR 06)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

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

# Custom protobuf to use STINGER distributed version
function(CUST_PROTOBUF_GENERATE_CPP SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    
    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
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

LIST(APPEND CMAKE_C_FLAGS "-std=c99")

INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/include")

ADD_SUBDIRECTORY(src)

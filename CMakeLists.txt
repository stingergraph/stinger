cmake_minimum_required (VERSION 2.8.11)
project (STINGER)

include (ExternalProject)
include (CTest)

set(STINGER_VERSION_MAJOR 2013)
set(STINGER_VERSION_MINOR 06)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
#set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "--no-as-needed -ldl")
set(STINGER_NAME_USE_SQLITE FALSE CACHE BOOL "Use sqlite for virtual to physmap")
if(STINGER_NAME_USE_SQLITE)
  add_definitions(-DNAME_USE_SQLITE)
endif()

set(STINGER_USE_TCP FALSE CACHE BOOL "Use TCP instead of Unix Sockets")
if(STINGER_USE_TCP)
  add_definitions(-DSTINGER_USE_TCP)
endif()

find_package( OpenMP )
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

link_libraries(m)

option(BUILD_SHARED_LIBS "Build libraries as SHARED" ON)

IF(APPLE)
  # Apple doesn't support librt, using CoreServices instead
  link_libraries(/System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices)
  set(CMAKE_MACOSX_RPATH ON)
ELSEIF(WIN32)
  # Not sure what to do here as far as librt...?
  link_libraries(rt)
ELSE()
  link_libraries(rt)
  link_libraries(dl)
ENDIF()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu9x")

# Doxygen
if(BUILD_DOCUMENTATION)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  endif()
endif()

#protobuf
ExternalProject_Add(protobuf
  PREFIX              ${CMAKE_BINARY_DIR}/external
  URL                 file://${CMAKE_SOURCE_DIR}/external/protobuf-2.5.0.tar.gz
  URL_MD5             b751f772bdeb2812a2a8e7202bf1dae8
  UPDATE_COMMAND      ""
  PATCH_COMMAND       ""
  CONFIGURE_COMMAND   CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ${CMAKE_BINARY_DIR}/external/src/protobuf/configure
    --prefix=${CMAKE_BINARY_DIR}
    --enable-shared
  BUILD_COMMAND make
  INSTALL_COMMAND make install
)

#libconfig
ExternalProject_Add(libconfig
  PREFIX              ${CMAKE_BINARY_DIR}/external
  URL                 file://${CMAKE_SOURCE_DIR}/external/libconfig-1.5.tar.gz
  URL_MD5             a939c4990d74e6fc1ee62be05716f633
  UPDATE_COMMAND      ""
  PATCH_COMMAND       ""
  CONFIGURE_COMMAND   CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} ${CMAKE_BINARY_DIR}/external/src/libconfig/configure
    --prefix=${CMAKE_BINARY_DIR}
    --enable-shared
    --disable-examples
  BUILD_COMMAND cp ${CMAKE_BINARY_DIR}/external/src/libconfig/lib/libconfig.h ${CMAKE_BINARY_DIR}/external/src/libconfig-build/lib/ && make
  INSTALL_COMMAND make install
)

if(APPLE)
  set(SHARED_LIB_EXT .dylib)
else()
  set(SHARED_LIB_EXT .so)
endif()

add_library(_libconfig_library STATIC IMPORTED)
add_dependencies(_libconfig_library libconfig)
set_target_properties(_libconfig_library PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libconfig${SHARED_LIB_EXT})

add_library(_libconfig++_library STATIC IMPORTED)
add_dependencies(_libconfig++_library libconfig)
set_target_properties(_libconfig++_library PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libconfig++${SHARED_LIB_EXT})

add_library(_protobuf_library STATIC IMPORTED)
add_dependencies(_protobuf_library protobuf)
set_target_properties(_protobuf_library PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libprotobuf${SHARED_LIB_EXT})

add_library(_protobuf_lite_library STATIC IMPORTED)
add_dependencies(_protobuf_lite_library protobuf)
set_target_properties(_protobuf_lite_library PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libprotobuf-lite${SHARED_LIB_EXT})

add_library(_protoc_library STATIC IMPORTED)
add_dependencies(_protoc_library protobuf)
set_target_properties(_protoc_library PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libprotoc${SHARED_LIB_EXT})

# Memory size configuration bits for stinger_core
set(STINGER_DEFAULT_VERTICES "(1L<<24)" CACHE STRING "Default number of vertices")
set(STINGER_DEFAULT_NUMETYPES "5" CACHE STRING "Default number of edge types")
set(STINGER_DEFAULT_NUMVTYPES "128" CACHE STRING "Default number of vertex types")
set(STINGER_DEFAULT_NEB_FACTOR "4" CACHE STRING "Default number of edge blocks per vertex")
set(STINGER_EDGEBLOCKSIZE "14" CACHE STRING "Number of edges per edge block")
set(STINGER_NAME_STR_MAX "255" CACHE STRING "Max string length in physmap")

configure_file(${CMAKE_SOURCE_DIR}/lib/stinger_core/inc/stinger_defs.h.in ${CMAKE_BINARY_DIR}/include/stinger_core/stinger_defs.h @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/lib/stinger_core/inc/stinger_names.h.in ${CMAKE_BINARY_DIR}/include/stinger_core/stinger_names.h @ONLY)
#file(COPY ${CMAKE_SOURCE_DIR}/lib/stinger_core/inc/stinger_defs.h DESTINATION ${CMAKE_BINARY_DIR}/include/stinger_core/)
#file(COPY ${CMAKE_SOURCE_DIR}/lib/stinger_core/inc/stinger_names.h DESTINATION ${CMAKE_BINARY_DIR}/include/stinger_core/)

include_directories("${CMAKE_BINARY_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/external/googletest/include/")

add_subdirectory(external/googletest)
add_subdirectory(lib)
add_subdirectory(src)

enable_testing()
add_test(NamesTest ${CMAKE_BINARY_DIR}/bin/stinger_names_test)
add_test(StingerCoreTest ${CMAKE_BINARY_DIR}/bin/stinger_core_test)
add_test(StingerPhysmapTest ${CMAKE_BINARY_DIR}/bin/stinger_physmap_test)
add_test(StingerTraversalTest ${CMAKE_BINARY_DIR}/bin/stinger_traversal_test)
add_test(StingerPagerankTest ${CMAKE_BINARY_DIR}/bin/stinger_pagerank_test)

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} -E env CTEST_OUTPUT_ON_FAILURE=1 --verbose DEPENDS stinger_names_test stinger_core_test stinger_physmap_test stinger_traversal_test)


set(sources
	src/core_util.c
	src/stinger.c
	src/stinger_deprecated.c
	src/stinger_names.c
	src/stinger_names_sqlite.c
	src/stinger_physmap.c
	src/stinger_shared.c
	src/stinger_vertex.c
	src/xmalloc.c
)
set(headers
	inc/core_util.h
	inc/stinger.h
	inc/stinger_atomics.h
	inc/stinger_deprecated.h
	inc/stinger_error.h
	inc/stinger_internal.h
	inc/stinger_physmap.h
	inc/stinger_return.h
	inc/stinger_shared.h
	inc/stinger_traversal.h
	inc/stinger_vertex.h
	inc/x86_full_empty.h
	inc/xmalloc.h
	inc/stinger_batch_insert.h
)
set(unoptimized_sources
	src/x86_full_empty.c
)

set(config
	stinger_names.h
	stinger_defs.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/stinger_core")

include_directories("${CMAKE_BINARY_DIR}/include/stinger_core")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")

set_source_files_properties(${config} PROPERTIES GENERATED TRUE)
set_source_files_properties(${unoptimized_sources} PROPERTIES COMPILE_FLAGS -O0)
add_library(stinger_core SHARED ${sources} ${unoptimized_sources} ${headers} ${config})
if(OPENMP_FOUND)
  target_compile_definitions(stinger_core PUBLIC _GLIBCXX_PARALLEL)
endif()
target_link_libraries(stinger_core compat)

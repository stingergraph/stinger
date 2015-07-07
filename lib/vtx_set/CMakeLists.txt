set(sources
	src/vtx_set.c
)
set(headers
	inc/vtx_set.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/vtx_set")

include_directories("${CMAKE_BINARY_DIR}/include/vtx_set")

add_library(vtx_set SHARED ${sources} ${headers})
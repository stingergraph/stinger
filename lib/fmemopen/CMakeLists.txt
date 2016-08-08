set(sources
	src/fmemopen.c
)
set(headers
	inc/fmemopen.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/fmemopen")

include_directories("${CMAKE_BINARY_DIR}/include/fmemopen")

add_library(fmemopen SHARED ${sources} ${headers})
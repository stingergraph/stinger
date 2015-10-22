set(sources
	src/fmemopen.c
)
set(headers
	inc/fmemopen.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/fmemopen")

include_directories("${CMAKE_BINARY_DIR}/include/fmemopen")

add_library(fmemopen SHARED ${sources} ${headers})
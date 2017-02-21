set(sources
	src/int64vector.c
)
set(headers
	inc/int64vector.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/int64vector")

include_directories("${CMAKE_BINARY_DIR}/include/int64vector")

add_library(int64vector ${sources} ${headers})
set(sources
	src/astring.c
)
set(headers
	inc/astring.h)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/string")

include_directories("${CMAKE_BINARY_DIR}/include/string")

add_library(string SHARED ${sources} ${headers})

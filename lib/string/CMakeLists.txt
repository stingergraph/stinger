set(sources
	src/astring.c
)
set(headers
	inc/astring.h)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/string")

include_directories("${CMAKE_BINARY_DIR}/include/string")

add_library(string SHARED ${sources} ${headers})

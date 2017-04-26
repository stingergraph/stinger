set(sources
	src/sqlite3.c
)
set(headers
	inc/sqlite3.h
)

publish_headers(headers "${CMAKE_BINARY_DIR}/include/sqlite")

include_directories("${CMAKE_BINARY_DIR}/include/sqlite")

add_library(sqlite ${sources} ${headers})
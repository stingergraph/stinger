set(sources
	src/sqlite3.c
)
set(headers
	inc/sqlite3.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/sqlite")

include_directories("${CMAKE_BINARY_DIR}/include/sqlite")

add_library(sqlite SHARED ${sources} ${headers})
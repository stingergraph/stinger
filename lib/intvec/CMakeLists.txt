set(sources
	src/int64vector.c
)
set(headers
	inc/int64vector.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/int64vector")

include_directories("${CMAKE_BINARY_DIR}/include/int64vector")

add_library(int64vector SHARED ${sources} ${headers})
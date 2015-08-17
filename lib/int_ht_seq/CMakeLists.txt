set(sources
	src/int-ht-seq.c
)
set(headers
	inc/int_ht_seq.h
)

file(COPY ${headers} DESTINATION "${CMAKE_BINARY_DIR}/include/int_ht_seq")

include_directories("${CMAKE_BINARY_DIR}/include/int_ht_seq")
include_directories("${CMAKE_BINARY_DIR}/include/stinger_utils")

add_library(int_ht_seq SHARED ${sources} ${headers})